#ifndef CONCURRENTMESSAGEQUEUE_H
#define CONCURRENTMESSAGEQUEUE_H

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

/**
 * Some experimental code trying to understand and reproduce the message queue
 * from the book "3D Engine Design for Virtual Globes" by Kevin Ring and Patrick
 * Cozzi, but this is currently not used anywhere, so...
 */

enum ConcurrentMessageQueueState {
    RUNNING,
    STOPPING,
    STOPPED
};

struct ConcurrentMessageQueueData {
    std::function<void(void*)> callback;
    void* message;
};

class ConcurrentMessageQueue {
public:
    ConcurrentMessageQueue();
    void startInOtherThread();
    void processQueue();
    void terminate();
    void post(const std::function<void(void*)>& callback, void* message);
    void run();
    void processCurrentQueue(std::deque<ConcurrentMessageQueueData> currentQueue);
    void processMessage();
    void stop(void* data);

    std::mutex _queueMutex;
    std::mutex _messageMutex;
    std::condition_variable _queueCond;
    std::deque<ConcurrentMessageQueueData> _queue;
    ConcurrentMessageQueueState _state;
    std::thread _thread;
    std::function<void(void*)> _onMessage;
};

#endif // CONCURRENTMESSAGEQUEUE_H
