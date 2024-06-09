#ifndef CONCURRENTLRUCACHE_H
#define CONCURRENTLRUCACHE_H

#include "lrucache.h"
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <unordered_map>

template <typename K, typename V>
class ConcurrentLRUCache {
private:
    unsigned _capacity;
    LRUCache<K, V> _lruCache;

    std::mutex _mutex;
    std::condition_variable _cond;

public:
    ConcurrentLRUCache(int capacity)
        : _capacity(capacity)
        , _lruCache(_capacity)
    {
    }

    std::optional<V> get(const K& key)
    {

        std::lock_guard<std::mutex> lock(_mutex);
        return _lruCache.get(key);
    }

    bool contains(const K& key)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _lruCache.contains(key);
    }

    PutResult<K, V> put(const K& key, const V& value)
    {

        std::lock_guard<std::mutex> lock(_mutex);
        PutResult<K, V> result = _lruCache.put(key, value);
        return result;
    }
};

#endif // CONCURRENTLRUCACHE_H
