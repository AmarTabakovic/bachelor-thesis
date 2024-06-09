#ifndef UNLOADWORKERTHREAD_H
#define UNLOADWORKERTHREAD_H

#include "messagequeue.h"
#include "xyztilekey.h"
#include <thread>

enum UnloadResponseType {
    UNLOAD_OK, /* Loaded */
    UNLOAD_ERROR, /* Something wrong happened */
};

enum UnloadRequestType {
    UNLOAD_REQUEST,
    UNLOAD_STOP_THREAD
};

struct UnloadRequest {
    XYZTileKey tileKey;
    UnloadRequestType type;
};

/**
 * @brief The UnloadResponse class
 */
struct UnloadResponse {
    UnloadResponseType type;
    XYZTileKey tileKey;
};

/**
 * @brief The UnloadWorkerThread class
 */
class UnloadWorkerThread {
public:
    UnloadWorkerThread(MessageQueue<UnloadRequest>* requestQueue, MessageQueue<UnloadResponse>* doneQueue);

    void postRequest(XYZTileKey tileKey);
    void startInAnotherThread();
    void run();

    void processAllRequests();
    void processRequest();
    void evictFromDiskCache(UnloadRequest& request, UnloadResponse& response);

    MessageQueue<UnloadRequest>* _requestQueue;
    MessageQueue<UnloadResponse>* _doneQueue;

    std::thread _thread;
};

#endif // UNLOADWORKERTHREAD_H
