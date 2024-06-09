#include "concurrentmessagequeue.h"

ConcurrentMessageQueue::ConcurrentMessageQueue()
{

}

void ConcurrentMessageQueue::post(const std::function<void(void*)>& callback, void* message)
{
    std::lock_guard lg(_queueMutex);
    _queue.push_back({ callback, message });
    _queueCond.notify_one();
}

void ConcurrentMessageQueue::processQueue()
{
}

void ConcurrentMessageQueue::run()
{
    std::deque<ConcurrentMessageQueueData> current;

    do {

        std::unique_lock ul(_queueMutex);

        if (!_queue.empty()) {
            current = _queue; /* Copy the queue's current contents for
                                 further processing */
            _queue.clear();
        } else {
            _queueCond.wait(ul);

            current = _queue; /* Copy the queue's current contents for
                                 further processing */
            _queue.clear();
        }

        ul.unlock();

        processCurrentQueue(current);

    } while (_state == RUNNING);

    {
        std::lock_guard lg(_queueMutex);
        _state = STOPPED;
    }
}

void ConcurrentMessageQueue::processCurrentQueue(std::deque<ConcurrentMessageQueueData> currentQueue)
{
    for (int i = 0; i < currentQueue.size(); i++) {
        if (_state == STOPPING) {
            std::lock_guard lg(_queueMutex);
            // TODO
            break;
        }

        auto message = currentQueue[i];
        // TODO: Should very likely be pointers if I want to check whether
        //       there is a callback
        if (message.callback) {
            message.callback(message.message);
        } else if (_onMessage) {
            _onMessage(message.message);
        }
    }
}

void ConcurrentMessageQueue::startInOtherThread()
{
    {
        std::lock_guard lg(_queueMutex);

        if (_state != STOPPED) {
            std::cerr << "MQ already running" << std::endl;
        }

        _state = RUNNING;
    }

    _thread = std::thread(&ConcurrentMessageQueue::run);
    _thread.detach();
}

void ConcurrentMessageQueue::terminate()
{
    post([this](void*) { _state = STOPPED; });
    if (_thread.joinable()) {
        _thread.join();
    }
}

void ConcurrentMessageQueue::stop(void* data)
{
    _state = STOPPING;
}
