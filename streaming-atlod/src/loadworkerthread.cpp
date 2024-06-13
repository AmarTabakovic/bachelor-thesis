#include "loadworkerthread.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#include "gridmesh.h"
#include "terrainmanager.h"
#include "util.h"

#include "configmanager.h"
#include "globalconstants.h"
#include "mapprojections.h"

#include <filesystem>
#include <iostream>
#include <webp/decode.h>

/**
 * @brief LoadWorkerThread::LoadWorkerThread
 * @param requestQueue
 * @param doneQueue
 */
LoadWorkerThread::LoadWorkerThread(MessageQueue<LoadRequest>* requestQueue, MessageQueue<LoadResponse>* doneQueue)
{
    _requestQueue = requestQueue;
    _doneQueue = doneQueue;
    _curl = curl_easy_init();
}

/**
 * @brief LoadWorkerThread::startInAnotherThread
 */
void LoadWorkerThread::startInAnotherThread()
{
    _thread = std::thread(&LoadWorkerThread::run, this);
    _thread.detach();
}

/**
 * @brief LoadWorkerThread::run
 */
void LoadWorkerThread::run()
{
    while (!_stopThread) {
        processAllRequests();
    }
}

/**
 * @brief LoadWorkerThread::processAllRequests
 */
void LoadWorkerThread::processAllRequests()
{
    auto requests = _requestQueue->popAll();

    bool returnNetworkErrors = false;

    for (auto request : requests) {
        if (request.type == LOAD_STOP_THREAD) {
            _stopThread = true;
        }
        LoadResponse response = { LOAD_OK, request.tileKey, nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, LOAD_ORIGIN_DISK_CACHE };

        /* If at any point we got a network error in the current queue processing,
         * do not bother with the rest of the requests, simply return error
         * for now */
        if (returnNetworkErrors) {
            response.type = LOAD_ERROR;
            _doneQueue->push(response);
            continue;
        }

        fetchHeightmap(request, response);

        if (response.type != LOAD_OK) {

            /* Found a network error, we now skip all items in the queue
             * in the next iterations */
            if (response.type == LOAD_ERROR) {
                returnNetworkErrors = true;
            }

            _doneQueue->push(response);
            continue;
        }

        fetchOverlay(request, response);

        if (response.type != LOAD_OK) {

            /* Found a network error, we now skip all items in the queue
             * in the next iterations */
            if (response.type == LOAD_ERROR) {
                returnNetworkErrors = true;
            }

            _doneQueue->push(response);
            continue;
        }

        if (response.type == LOAD_OK) {
            TerrainNode* newTile = new TerrainNode(request.tileKey);
            newTile->_heightData = response.heightData;

            newTile->generateMinMaxHeight();
            newTile->generateAabb();
            newTile->generateProjectedGridPoints();
            newTile->generateHorizonPoints();

            response.node = newTile;
        }

        _doneQueue->push(response);
    }

    if (_stopThread) {
        _doneQueue->push({ LOAD_STOPPED_THREAD, XYZTileKey(0, 0, 0), nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, LOAD_ORIGIN_DISK_CACHE });
    }
}

/**
 * @brief LoadWorkerThread::fetchHeightmap
 * @param request
 * @param response
 */
void LoadWorkerThread::fetchHeightmap(LoadRequest& request, LoadResponse& response)
{
    if (request.type == LOAD_REQUEST_DISK_CACHE) {
        loadHeightmapFromDisk(request, response);
        response.origin = LOAD_ORIGIN_DISK_CACHE;
    } else if (!request.offlineMode) {
        loadHeightmapFromApi(request, response);
        response.origin = LOAD_ORIGIN_API;
    } else { /* Offline mode, we cannot request new tiles */
        response.type = LOAD_UNLOADABLE;
    }
}

/**
 * @brief LoadWorkerThread::fetchOverlay
 * @param request
 * @param response
 */
void LoadWorkerThread::fetchOverlay(LoadRequest& request, LoadResponse& response)
{
    if (request.type == LOAD_REQUEST_DISK_CACHE) {
        loadOverlayFromDisk(request, response);
        response.origin = LOAD_ORIGIN_DISK_CACHE;
    } else if (!request.offlineMode) {
        loadOverlayFromApi(request, response);
        response.origin = LOAD_ORIGIN_API;
    } else { /* Offline mode, we cannot request new tiles */
        response.type = LOAD_UNLOADABLE;
    }
}

/**
 * @brief LoadWorkerThread::loadHeightmapFromDisk
 * @param request
 * @param response
 */
void LoadWorkerThread::loadHeightmapFromDisk(LoadRequest& request, LoadResponse& response)
{
    XYZTileKey tileKey = request.tileKey;
    std::string fileName = ConfigManager::getInstance()->diskCachePath() + GlobalConstants::HEIGHTDATA_DIR_NAME + std::to_string(tileKey.x()) + "_" + std::to_string(tileKey.y()) + "_" + std::to_string(tileKey.z()) + ".webp";

    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Unable to open file " << fileName << " (cache heightmap) " << tileKey.string() << std::endl;
        response.type = LOAD_UNLOADABLE;
        return;
    }

    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> fileData(fileSize);
    if (!file.read(fileData.data(), fileSize)) {
        std::cerr << "Error: Unable to read file " << fileName << " (cache)" << std::endl;
        response.type = LOAD_UNLOADABLE;
        return;
    }

    unsigned char* imageData = nullptr;
    int width, height;

    if (WebPGetInfo((const uint8_t*)(fileData.data()), fileSize, &width, &height) != 1) {
        std::cerr << "Error: Failed to get WebP image info (cache)" << std::endl;
        response.type = LOAD_UNLOADABLE;
        return;
    }

    imageData = new unsigned char[width * height * 3];

    if (WebPDecodeRGBInto((const uint8_t*)(fileData.data()), fileSize,
            imageData, width * height * 3, width * 3)
        == nullptr) {
        std::cerr << "Error: Failed to decode WebP image (cache)" << std::endl;
        delete[] imageData;
        response.type = LOAD_UNLOADABLE;
        return;
    }

    response.heightData = imageData;
    response.heightWidth = width;
    response.heightHeight = height;
    response.type = LOAD_OK;
}

/**
 * @brief LoadWorkerThread::loadOverlayFromDisk
 * @param request
 * @param response
 */
void LoadWorkerThread::loadOverlayFromDisk(LoadRequest& request, LoadResponse& response)
{
    XYZTileKey tileKey = request.tileKey;
    int width, height, nrChannels;
    std::string fileName = ConfigManager::getInstance()->diskCachePath() + GlobalConstants::OVERLAY_DIR_NAME + std::to_string(tileKey.x()) + "_" + std::to_string(tileKey.y()) + "_" + std::to_string(tileKey.z()) + ".jpg";
    unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        response.overlayData = data;
        response.overlayWidth = width;
        response.overlayHeight = height;
        response.overlayNrChannels = nrChannels;
        response.type = LOAD_OK;
    } else {
        std::cerr << "Failed opening overlay texture from cache " << tileKey.string() << std::endl;
        std::exit(1);
        response.type = LOAD_UNLOADABLE;
    }
}

/**
 * @brief writeData
 * @param ptr
 * @param size
 * @param nmemb
 * @param data
 * @return
 */
size_t writeData(void* ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

/**
 * @brief LoadWorkerThread::loadHeightmapFromApi
 * @param request
 * @param response
 */
void LoadWorkerThread::loadHeightmapFromApi(LoadRequest& request, LoadResponse& response)
{

    CURLcode res;
    XYZTileKey tileKey = request.tileKey;

    std::string responseData;
    std::string url = ConfigManager::getInstance()->heightDataServiceUrl()
        + std::to_string(tileKey.z()) + "/"
        + std::to_string(tileKey.x()) + "/"
        + std::to_string(tileKey.y()) + ".webp?key=" + ConfigManager::getInstance()->heightDataServiceKey();

    if (_curl) {
        curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, writeData);
        curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &responseData);
        curl_easy_setopt(_curl, CURLOPT_TIMEOUT, 5L);

        int httpStatusCode = 0;
        int retCode = curl_easy_perform(_curl);

        if (retCode == CURLE_OPERATION_TIMEDOUT) {
            std::cout << "Heightmap timeout" << std::endl;
            response.type = LOAD_TIMEOUT;
            return;
        }

        curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);

        if (retCode != CURLE_OK) { /* Network error */
            response.type = LOAD_ERROR;
            return;
        } else if (httpStatusCode == 204) { /* Empty tile */
            response.type = LOAD_UNLOADABLE;
            return;
        } else if (httpStatusCode != 200) { /* Bad error, should never happen */
            std::cerr << "Error: Status code " << httpStatusCode << " while loading heightmap " << tileKey.string() << std::endl;
            std::exit(1);
        } else {
            int width, height;
            if (WebPGetInfo((const uint8_t*)(responseData.data()), responseData.size(), &width, &height) != 1) {
                std::cerr << "Error: Failed to get WebP image info" << std::endl;
                response.type = LOAD_UNLOADABLE;
                std::exit(1);
                return;
            }

            unsigned char* data = new unsigned char[width * height * 3];

            if (WebPDecodeRGBInto((const uint8_t*)(responseData.data()), responseData.size(),
                    data, width * height * 3, width * 3)
                == nullptr) {
                std::cerr << "Error: Failed to decode WebP image" << std::endl;
                delete[] data;
                response.type = LOAD_UNLOADABLE;
                std::exit(1);
                return;
            }

            std::string filePath = ConfigManager::getInstance()->diskCachePath() + GlobalConstants::HEIGHTDATA_DIR_NAME
                + std::to_string(tileKey.x()) + "_"
                + std::to_string(tileKey.y()) + "_"
                + std::to_string(tileKey.z()) + ".webp";

            std::ofstream file(filePath, /*std::ios::out |*/ std::ios::binary);
            if (file.is_open()) {
                file.write(responseData.c_str(), responseData.size());
                file.close();
            } else {
                std::cerr << "Failed to open file for writing. " << tileKey.string() << std::endl;
                std::exit(1);
            }
            response.heightData = data;
            response.heightWidth = width;
            response.heightHeight = height;
            response.type = LOAD_OK;
        }
    } else {
        std::cerr << "Curl failed" << std::endl;
        std::exit(1);
    }
}

/**
 * @brief LoadWorkerThread::loadOverlayFromApi
 * @param request
 * @param response
 */
void LoadWorkerThread::loadOverlayFromApi(LoadRequest& request, LoadResponse& response)
{
    XYZTileKey tileKey = request.tileKey;

    std::string responseData;
    std::string url = ConfigManager::getInstance()->overlayDataServiceUrl()
        + std::to_string(tileKey.z()) + "/"
        + std::to_string(tileKey.x()) + "/"
        + std::to_string(tileKey.y()) + ".jpg?key=" + ConfigManager::getInstance()->overlayDataServiceKey();

    if (!_curl) {
        std::cerr << "Curl failed" << std::endl;
        std::exit(1);
    }

    curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, writeData);
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &responseData);
    curl_easy_setopt(_curl, CURLOPT_TIMEOUT, 5L);

    int httpStatusCode = 0;
    int retCode = curl_easy_perform(_curl);

    if (retCode == CURLE_OPERATION_TIMEDOUT) {
        std::cout << "Heightmap timeout" << std::endl;
        response.type = LOAD_TIMEOUT;
        return;
    }

    curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);

    if (retCode != CURLE_OK) {
        std::cerr << "Network error" << std::endl;
        response.type = LOAD_ERROR;
        return;
    } else if (httpStatusCode == 204) {
        std::cerr << "Tile is empty" << std::endl;
        response.type = LOAD_UNLOADABLE;
        return;
    } else if (httpStatusCode != 200) {
        std::cerr << "Status code " << httpStatusCode << " in loading overlay" << std::endl;
        response.type = LOAD_ERROR;
        return;
    }

    int width, height, nrChannels;
    unsigned char* data = stbi_load_from_memory((unsigned char*)responseData.c_str(), responseData.size(), &width, &height, &nrChannels, 0);
    if (data) {
        std::string filePath = ConfigManager::getInstance()->diskCachePath() + GlobalConstants::OVERLAY_DIR_NAME
            + std::to_string(tileKey.x()) + "_"
            + std::to_string(tileKey.y()) + "_"
            + std::to_string(tileKey.z()) + ".jpg";
        std::ofstream file(filePath, std::ios::out | std::ios::binary);
        if (file.is_open()) {
            file.write(responseData.c_str(), responseData.size());
            file.close();
        } else {
            std::cerr << "Failed to open file for writing." << std::endl;
            std::exit(1);
        }

        response.overlayData = data;
        response.overlayWidth = width;
        response.overlayHeight = height;
        response.overlayNrChannels = nrChannels;
        response.type = LOAD_OK;
    } else {
        std::cerr << "Failed opening overlay texture" << std::endl;
        std::exit(1);
    }
}
