#include "diskdeallocationworkerthread.h"
#include "configmanager.h"
#include "globalconstants.h"
#include <filesystem>

DiskDeallocationWorkerThread::DiskDeallocationWorkerThread(MessageQueue<DiskDeallocationRequest>* requestQueue, MessageQueue<DiskDeallocationResponse>* doneQueue)
    : _requestQueue(requestQueue)
    , _doneQueue(doneQueue)
{
}

void DiskDeallocationWorkerThread::postRequest(XYZTileKey tileKey)
{
}
void DiskDeallocationWorkerThread::startInAnotherThread()
{
    _thread = std::thread(&DiskDeallocationWorkerThread::run, this);
    _thread.detach();
}
void DiskDeallocationWorkerThread::run()
{
    while (true) {
        processAllRequests();
    }
}

void DiskDeallocationWorkerThread::processAllRequests()
{
    auto requests = _requestQueue->popAll();

    for (auto request : requests) {
        DiskDeallocationResponse response = { UNLOAD_OK, request.tileKey };
        evictFromDiskCache(request, response);
        _doneQueue->push(response);
    }
}

void DiskDeallocationWorkerThread::evictFromDiskCache(DiskDeallocationRequest& request, DiskDeallocationResponse& response)
{
    XYZTileKey tileKey = request.tileKey;
    std::string overlayFileName = ConfigManager::getInstance()->diskCachePath() + GlobalConstants::OVERLAY_DIR_NAME
        + std::to_string(tileKey.x())
        + "_" + std::to_string(tileKey.y())
        + "_" + std::to_string(tileKey.z()) + ".jpg";

    std::string heightmapFileName = ConfigManager::getInstance()->diskCachePath() + GlobalConstants::HEIGHTDATA_DIR_NAME
        + std::to_string(tileKey.x())
        + "_" + std::to_string(tileKey.y())
        + "_" + std::to_string(tileKey.z()) + ".webp";

    bool noError = std::filesystem::remove(overlayFileName);
    noError = noError && std::filesystem::remove(heightmapFileName);

    if (!noError) {
        response.type = UNLOAD_ERROR;
    } else {
        response.type = UNLOAD_OK;
    }
}
