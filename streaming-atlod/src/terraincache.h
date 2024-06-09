#ifndef TERRAINCACHE_H
#define TERRAINCACHE_H

#include <string>

/**
 * @brief Manager for caching the terrain from and to disk.
 */
class TerrainCache
{
public:
    TerrainCache();

    // private:
    std::string _cachePath;
    unsigned _maxSizeInBytes;
};

#endif // TERRAINCACHE_H
