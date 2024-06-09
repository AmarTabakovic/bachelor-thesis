#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <functional>
#include <list>
#include <optional>
#include <tuple>
#include <unordered_map>

template <typename K, typename V>
struct PutResult {
    bool evicted;
    std::optional<std::pair<K, V>> evictedItem;
};

/**
 * @brief The LRUCache class
 *
 * This LRU cache container is based on Java's LinkedHashMap class.
 * In the background it uses a std::list and a std::unordered_map.
 *
 * TODO: Method for batch removal after 75% fill rate?
 * TODO: Method for periodic eviction of n elements
 */
template <typename K, typename V>
class LRUCache {
private:
    unsigned _capacity;
    std::list<std::pair<K, V>> _items;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> _cache;
    float _evictWhenLoad = 1.0f;
    std::function<bool(const K&, const V&)> _canEvict;

public:
    LRUCache(int capacity)
        : _capacity(capacity)
    {
        _cache.reserve(capacity);
    }

    std::optional<V> get(const K& key)
    {
        /* Item not found */
        if (_cache.find(key) == _cache.end()) {
            return std::nullopt;
        }

        _items.splice(_items.begin(), _items, _cache[key]);
        return _cache[key]->second;
    }

    unsigned size()
    {
        return _items.size();
    }

    bool contains(const K& key) const
    {
        return _cache.find(key) != _cache.end();
    }

    PutResult<K, V> put(const K& key, const V& value)
    {
        PutResult<K, V> result { false, std::nullopt };

        /* Update existing item */
        if (_cache.find(key) != _cache.end()) {
            _items.splice(_items.begin(), _items, _cache[key]);
            _cache[key]->second = value;
            return result;
        }

        /* Evict LRU item */
        if (_items.size() >= _evictWhenLoad * _capacity) {
            auto last = _items.back();
            _cache.erase(last.first);
            result.evicted = true;
            result.evictedItem = last;
            _items.pop_back();
        }

        /* Insert new item */
        _items.emplace_front(key, value);
        _cache[key] = _items.begin();
        return result;
    }
};

#endif // LRUCACHE_H
