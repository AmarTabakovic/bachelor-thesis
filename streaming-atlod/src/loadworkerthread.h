#ifndef LOADWORKERTHREAD_H
#define LOADWORKERTHREAD_H

#include "messagequeue.h"
#include "terrainnode.h"
#include "xyztilekey.h"
#include <thread>

enum LoadResponseType {
    LOAD_OK, /* Loaded */
    LOAD_ERROR, /* Something wrong happened */
    LOAD_UNLOADABLE, /* For e.g. oceans that have a max zoom level of 5 or so */
    LOAD_TIMEOUT, /* API timed out */
    LOAD_STOPPED_THREAD
};

enum LoadResponseOrigin {
    LOAD_ORIGIN_DISK_CACHE,
    LOAD_ORIGIN_API
};

enum LoadRequestType {
    LOAD_REQUEST,
    LOAD_REQUEST_DISK_CACHE,
    LOAD_STOP_THREAD
};

struct LoadRequest {
    XYZTileKey tileKey;
    LoadRequestType type;
    bool offlineMode;
};

/**
 * @brief The TerrainTileResponse class
 *
 * Terrain data is heavy, so we store them as heap-allocated pointers.
 * The main thread should take care to deallocate these pointers once done using them.
 */
struct LoadResponse {
    LoadResponseType type;
    XYZTileKey tileKey;
    unsigned char* heightData; /* Must be deallocated with delete[] imageData */
    unsigned char* overlayData; /* Must be deallocated with stbi_image_free(overlayData) */
    TerrainNode* tile;
    int overlayWidth, overlayHeight, overlayNrChannels;
    int heightWidth, heightHeight;
    LoadResponseOrigin origin;
};

/**
 * @brief The LoadWorkerThread class
 */
class LoadWorkerThread
{
public:
    LoadWorkerThread(MessageQueue<LoadRequest>* requestQueue, MessageQueue<LoadResponse>* doneQueue);

    void postRequest(XYZTileKey tileKey);
    void startInAnotherThread();
    void run();

    void processAllRequests();
    void processRequest();
    void fetchOverlay(LoadRequest& request, LoadResponse& response);
    void fetchHeightmap(LoadRequest& request, LoadResponse& response);

    void loadHeightmapFromDisk(LoadRequest& request, LoadResponse& response);
    void loadOverlayFromDisk(LoadRequest& request, LoadResponse& response);
    void loadHeightmapFromApi(LoadRequest& request, LoadResponse& response);
    void loadOverlayFromApi(LoadRequest& request, LoadResponse& response);

    MessageQueue<LoadRequest>* _requestQueue;
    MessageQueue<LoadResponse>* _doneQueue;

    std::thread _thread;
    bool _stopThread = false;

    bool _offlineMode = true;
};

#endif // LOADWORKERTHREAD_H
