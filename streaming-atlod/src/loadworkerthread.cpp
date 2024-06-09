#include "loadworkerthread.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#include "gridmesh.h"
#include "terrainmanager.h"
#include "util.h"

#include "configmanager.h"
#include "mapprojections.h"
#include <curl/curl.h>
#include <filesystem>
#include <iostream>
#include <webp/decode.h>

LoadWorkerThread::LoadWorkerThread(MessageQueue<LoadRequest>* requestQueue, MessageQueue<LoadResponse>* doneQueue)
{
    _requestQueue = requestQueue;
    _doneQueue = doneQueue;
}

void LoadWorkerThread::startInAnotherThread()
{
    _thread = std::thread(&LoadWorkerThread::run, this);
    _thread.detach();
}

void LoadWorkerThread::run()
{
    while (!_stopThread) {
        processAllRequests();
    }
}

void LoadWorkerThread::processAllRequests()
{
    auto requests = _requestQueue->popAll();

    std::deque<LoadResponse> responses;

    for (auto request : requests) {
        if (request.type == LOAD_STOP_THREAD) {
            _stopThread = true;
        }

        LoadResponse response = { LOAD_OK, request.tileKey, nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, LOAD_ORIGIN_DISK_CACHE };
        fetchHeightmap(request, response);

        if (response.type != LOAD_OK) {
            _doneQueue->push(response);
            continue;
        }

        fetchOverlay(request, response);

        if (response.type == LOAD_OK) {
            TerrainNode* newTile = new TerrainNode(request.tileKey);
            newTile->_heightData = response.heightData;

            newTile->generateMinMaxHeight();
            newTile->generateAabb();
            newTile->generateProjectedGridPoints();
            newTile->generateHorizonPoints();

            response.tile = newTile;
        }

        responses.push_back(response);

        _doneQueue->push(response);
    }

    /* if (_stopThread) {
         _doneQueue->push();
     }*/
}

void LoadWorkerThread::processRequest()
{

    auto requestMaybe = _requestQueue->pop();

    /* Empty queue, simply return without blocking */
    if (!requestMaybe) {
        return;
    }

    LoadRequest request = requestMaybe.value();
    TerrainNode* newTile = new TerrainNode(request.tileKey);

    LoadResponse response = { LOAD_OK, request.tileKey, nullptr, nullptr, newTile };

    fetchHeightmap(request, response);

    if (response.type != LOAD_OK) {
        _doneQueue->push(response);
        return;
    }

    fetchOverlay(request, response);

    _doneQueue->push(response);
}

void LoadWorkerThread::fetchHeightmap(LoadRequest& request, LoadResponse& response)
{
    if (request.type == LOAD_REQUEST_DISK_CACHE) {
        // std::cout << "Load DEM from disk " << request.tileKey.string() << std::endl;

        loadHeightmapFromDisk(request, response);
        response.origin = LOAD_ORIGIN_DISK_CACHE;
    } else if (!request.offlineMode) {
        loadHeightmapFromApi(request, response);
        response.origin = LOAD_ORIGIN_API;
    } else { /* Offline mode */
        response.type = LOAD_UNLOADABLE;
    }
}

void LoadWorkerThread::fetchOverlay(LoadRequest& request, LoadResponse& response)
{
    if (request.type == LOAD_REQUEST_DISK_CACHE) {
        loadOverlayFromDisk(request, response);
        response.origin = LOAD_ORIGIN_DISK_CACHE;
    } else if (!request.offlineMode) {
        loadOverlayFromApi(request, response);
        response.origin = LOAD_ORIGIN_API;
    } else {
        std::cerr << "OFFLINE MODE AND OVERLAY TEXTURE NOT IN DISK CACHE" << std::endl;
        response.type = LOAD_UNLOADABLE;
    }
}

void LoadWorkerThread::loadHeightmapFromDisk(LoadRequest& request, LoadResponse& response)
{
    XYZTileKey tileKey = request.tileKey;
    std::string fileName = ConfigManager::getInstance()->_diskCachePath + "dem/" + std::to_string(tileKey._x) + "_" + std::to_string(tileKey._y) + "_" + std::to_string(tileKey._z) + ".webp";

    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Unable to open file " << fileName << " (cache heightmap) " << tileKey.string() << std::endl;
        response.type = LOAD_UNLOADABLE;
        std::exit(1);
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

void LoadWorkerThread::loadOverlayFromDisk(LoadRequest& request, LoadResponse& response)
{
    XYZTileKey tileKey = request.tileKey;
    int width, height, nrChannels;
    std::string fileName = ConfigManager::getInstance()->_diskCachePath + "overlay/" + std::to_string(tileKey._x) + "_" + std::to_string(tileKey._y) + "_" + std::to_string(tileKey._z) + ".jpg";
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

size_t writeData(void* ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

void LoadWorkerThread::loadHeightmapFromApi(LoadRequest& request, LoadResponse& response)
{
    CURL* curl;
    CURLcode res;
    XYZTileKey tileKey = request.tileKey;

    curl = curl_easy_init();
    std::string responseData;
    std::string url = ConfigManager::getInstance()->_heightDataServiceUrl
        + std::to_string(tileKey._z) + "/"
        + std::to_string(tileKey._x) + "/"
        + std::to_string(tileKey._y) + ".webp?key=" + ConfigManager::getInstance()->_heightDataServiceKey;

    std::cout << url << std::endl;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        int httpStatusCode = 0;
        int retCode = curl_easy_perform(curl);

        if (retCode == CURLE_OPERATION_TIMEDOUT) {
            std::cout << "TIMEOUT HEIGHTMAP" << std::endl;
            response.type = LOAD_TIMEOUT;
            return;
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);
        if (retCode == CURLE_ABORTED_BY_CALLBACK) {
            std::cerr << "CURLE_ABORTED_BY_CALLBACK" << std::endl;
            std::exit(1);
        } else if (httpStatusCode == 204) {
            std::cerr << "Tile is empty" << std::endl;
            response.type = LOAD_UNLOADABLE;
            return;
        } else if (httpStatusCode == 0) {
            std::cerr << "Status code " << httpStatusCode << " in loading heightmap " << tileKey.string() << std::endl;
            response.type = LOAD_TIMEOUT;
            return;

        } else if (httpStatusCode != 200) {
            std::cerr << "Status code " << httpStatusCode << " in loading heightmap " << tileKey.string() << std::endl;
            std::exit(1);
        }

        else {
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

            std::string filePath = ConfigManager::getInstance()->_diskCachePath + "dem/"
                + std::to_string(tileKey._x) + "_"
                + std::to_string(tileKey._y) + "_"
                + std::to_string(tileKey._z) + ".webp";

            // std::cout << "Successfully decoded WEBP image with dimensions: " << width << " x " << height << std::endl;

            std::ofstream file(filePath, /*std::ios::out |*/ std::ios::binary);
            if (file.is_open()) {
                file.write(responseData.c_str(), responseData.size());
                file.close();
                // std::cout << "WEBP image successfully saved to " << filePath << std::endl;
            } else {
                std::cerr << "Failed to open file for writing. " << tileKey.string() << std::endl;
                std::exit(1);
            }

            response.heightData = data;
            response.heightWidth = width;
            response.heightHeight = height;
            response.type = LOAD_OK;
            curl_easy_cleanup(curl);
        }
    } else {
        std::cerr << "Curl failed" << std::endl;
        std::exit(1);
    }
}

void LoadWorkerThread::loadOverlayFromApi(LoadRequest& request, LoadResponse& response)
{
    CURL* curl;
    CURLcode res;
    XYZTileKey tileKey = request.tileKey;

    curl = curl_easy_init();
    std::string responseData;
    std::string url = ConfigManager::getInstance()->_overlayDataServiceUrl
        + std::to_string(tileKey._z) + "/"
        + std::to_string(tileKey._x) + "/"
        + std::to_string(tileKey._y) + ".jpg?key=" + ConfigManager::getInstance()->_overlayDataServiceKey;
    ;

    std::cout << url << std::endl;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        int httpStatusCode = 0;
        int retCode = curl_easy_perform(curl);

        if (retCode == CURLE_OPERATION_TIMEDOUT) {
            std::cout << "TIMEOUT HEIGHTMAP" << std::endl;
            response.type = LOAD_TIMEOUT;
            return;
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);
        if (retCode == CURLE_ABORTED_BY_CALLBACK) {
            std::cerr << "CURLE_ABORTED_BY_CALLBACK" << std::endl;
            std::exit(1);
        } else if (httpStatusCode == 204) {
            std::cerr << "Tile is empty" << std::endl;
            response.type = LOAD_UNLOADABLE;
            return;
        } else if (httpStatusCode == 0) {
            std::cerr << "Status code " << httpStatusCode << " in loading heightmap " << tileKey.string() << std::endl;
            response.type = LOAD_TIMEOUT;
            return;

        } else if (httpStatusCode != 200) {
            std::cerr << "Status code " << httpStatusCode << " in loading overlay" << std::endl;
            std::exit(1);
        } else {

            /* TODO: Store jpg on disk cache */

            int width, height, nrChannels;
            unsigned char* data = stbi_load_from_memory((unsigned char*)responseData.c_str(), responseData.size(), &width, &height, &nrChannels, 0);
            if (data) {
                std::string filePath = ConfigManager::getInstance()->_diskCachePath + "overlay/"
                    + std::to_string(tileKey._x) + "_"
                    + std::to_string(tileKey._y) + "_"
                    + std::to_string(tileKey._z) + ".jpg";
                std::ofstream file(filePath, std::ios::out | std::ios::binary);
                if (file.is_open()) {
                    file.write(responseData.c_str(), responseData.size());
                    file.close();
                    // std::cout << "JPEG image saved to " << filePath << std::endl;
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
                response.type = LOAD_UNLOADABLE;
                std::exit(1);
            }
        }
    } else {
        std::cerr << "Curl failed" << std::endl;
        std::exit(1);
        curl_easy_cleanup(curl);
    }
}
