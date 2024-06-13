#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <type_traits>

/**
 * @brief Basic concurrent message queue for communicating between threads.
 */
template <typename T>
class MessageQueue {
public:
    /**
     * @brief push
     * @param message
     */
    void push(T message)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push_back(std::move(message));
        //_cond.notify_one();
    }

    /**
     * @brief pop
     * @return
     */
    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_queue.empty()) {
            return std::nullopt;
        }
        T message = std::move(_queue.front());
        _queue.pop_front();
        return message;
    }

    /**
     * @brief empty
     * @return
     */
    bool empty()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }

    /**
     * @brief popAll
     * @return
     */
    std::deque<T> popAll()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        std::deque<T> queue = _queue;
        _queue.clear();
        return queue;
    }

    /**
     * @brief pushAll
     * @param queue
     */
    void pushAll(std::deque<T> queue)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto value : queue) {
            _queue.push_back(value);
        }
    }

private:
    std::mutex _mutex;
    std::condition_variable _cond;
    std::deque<T> _queue;
};

#endif // MESSAGEQUEUE_H
