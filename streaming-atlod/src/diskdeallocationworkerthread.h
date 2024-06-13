#ifndef DISKDEALLOCATIONDWORKERTHREAD_H
#define DISKDEALLOCATIONDWORKERTHREAD_H

#include "messagequeue.h"
#include "xyztilekey.h"
#include <thread>

enum DiskDeallocationResponseType {
    UNLOAD_OK, /* Loaded */
    UNLOAD_ERROR, /* Something wrong happened */
};

enum DiskDeallocationRequestType {
    UNLOAD_REQUEST,
    UNLOAD_STOP_THREAD
};

struct DiskDeallocationRequest {
    XYZTileKey tileKey;
    DiskDeallocationRequestType type;
};

/**
 * @brief The DiskDeallocationResponse class
 */
struct DiskDeallocationResponse {
    DiskDeallocationResponseType type;
    XYZTileKey tileKey;
};

/**
 * @brief The DiskDeallocationWorkerThread class
 */
class DiskDeallocationWorkerThread {
public:
    DiskDeallocationWorkerThread(MessageQueue<DiskDeallocationRequest>* requestQueue, MessageQueue<DiskDeallocationResponse>* doneQueue);

    void postRequest(XYZTileKey tileKey);
    void startInAnotherThread();
    void run();

    void processAllRequests();
    void processRequest();
    void evictFromDiskCache(DiskDeallocationRequest& request, DiskDeallocationResponse& response);

    MessageQueue<DiskDeallocationRequest>* _requestQueue;
    MessageQueue<DiskDeallocationResponse>* _doneQueue;

    std::thread _thread;
};

#endif // DISKDEALLOCATIONDWORKERTHREAD_H
