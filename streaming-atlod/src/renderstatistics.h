#ifndef RENDERSTATISTICS_H
#define RENDERSTATISTICS_H

/**
 * @brief Simple data struct to hold rendering statistics;
 */
struct RenderStatistics {
    unsigned renderedTriangles = 0;
    unsigned drawCalls = 0;
    unsigned apiRequests = 0;
    unsigned numberOfNodes = 0;
    unsigned deepestZoomLevel = 0;
    unsigned currentlyRequested = 0;
    unsigned visibleTiles = 0;
    unsigned traversedNodes = 0;
    unsigned numberOfDiskCacheEntries = 0;
    bool waitDealloc = false;
};

#endif // RENDERSTATISTICS_H
