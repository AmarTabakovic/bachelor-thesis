#include "unloadworkerthread.h"
#include "configmanager.h"
#include <filesystem>

UnloadWorkerThread::UnloadWorkerThread(MessageQueue<UnloadRequest>* requestQueue, MessageQueue<UnloadResponse>* doneQueue)
{
    _requestQueue = requestQueue;
    _doneQueue = doneQueue;
}

void UnloadWorkerThread::postRequest(XYZTileKey tileKey)
{
}
void UnloadWorkerThread::startInAnotherThread()
{
    _thread = std::thread(&UnloadWorkerThread::run, this);
    _thread.detach();
}
void UnloadWorkerThread::run()
{
    while (true) {
        // processRequest();
        processAllRequests();
    }
}

void UnloadWorkerThread::processAllRequests()
{
    auto requests = _requestQueue->popAll();

    std::deque<UnloadResponse> responses;

    for (auto request : requests) {

        UnloadResponse response = { UNLOAD_OK, request.tileKey };

        evictFromDiskCache(request, response);

        responses.push_back(response);

        _doneQueue->push(response);
    }
}

void UnloadWorkerThread::processRequest()
{
}

void UnloadWorkerThread::evictFromDiskCache(UnloadRequest& request, UnloadResponse& response)
{
    XYZTileKey tileKey = request.tileKey;
    std::string overlayFileName = ConfigManager::getInstance()->_diskCachePath + "overlay/"
        + std::to_string(tileKey._x)
        + "_" + std::to_string(tileKey._y)
        + "_" + std::to_string(tileKey._z) + ".jpg";

    std::string heightmapFileName = ConfigManager::getInstance()->_diskCachePath + "dem/"
        + std::to_string(tileKey._x)
        + "_" + std::to_string(tileKey._y)
        + "_" + std::to_string(tileKey._z) + ".webp";

    bool noError = std::filesystem::remove(overlayFileName);
    noError = noError && std::filesystem::remove(heightmapFileName);
    // bool noError = true;

    // std::cout << "DISK DELETE " << tileKey.string() << std::endl;

    if (!noError) {
        response.type = UNLOAD_ERROR;
    } else {
        response.type = UNLOAD_OK;
    }
}
