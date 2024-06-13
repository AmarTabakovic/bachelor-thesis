#ifndef TERRAINMANAGER_H
#define TERRAINMANAGER_H

#include <list>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "aabbmesh.h"
#include "camera.h"
#include "gridmesh.h"
#include "loadworkerthread.h"
#include "lrucache.h"
#include "messagequeue.h"
#include "polemesh.h"
#include "renderstatistics.h"
#include "shader.h"
#include "skirtmesh.h"
#include "diskdeallocationworkerthread.h"
#include "xyztilekey.h"
class TerrainNode;

enum TileResolution {
    LOW,
    MEDIUM,
    HIGH
};

/**
 * @brief The terrain manager manages a collection of terrain tiles.
 */
class TerrainManager {
public:
    TerrainManager(RenderStatistics& stats, unsigned lowResMesh, unsigned mediumResMesh, unsigned highResMesh, unsigned memCacheSize, unsigned diskCacheSize);
    void setup();
    void shutdown();
    void render(Camera& camera, bool wireframe, bool aabb, bool& collision, float& verticalCollisionOffset);

    // private:
    void initDiskCache();
    void renderNode(Camera& camera, TerrainNode* node, TileResolution resolution, bool wireframe, bool aabb);

    void collectRenderable(Camera& camera, XYZTileKey currentTileKey, std::queue<std::string>& visibleTiles, XYZTileKey& minimumDistanceTileKey, float& minimumDistance);
    void requestChildren(XYZTileKey tileKey);
    void updateMinimumDistanceTileKey(Camera& camera, XYZTileKey currentTileKey, XYZTileKey& minimumDistanceTileKey, float& minimumDistance);
    bool checkCollision(Camera& camera, XYZTileKey minimumDistanceTileKey, float& verticalCollisionOffset);

    bool allChildrenExistant(XYZTileKey tileKey);
    bool shouldSplit(Camera& camera, XYZTileKey currentTileKey);
    bool hasChildren(XYZTileKey tileKey);

    void processSingleDoneQueueElement();
    void processAllDoneQueue();
    void processAllUnloadDoneQueue();

    float computeBaseDistWithLatitude(XYZTileKey tileKey);

    void initTerrainNode(LoadResponse response);
    void requestNode(XYZTileKey tileKey);

    void loadHeightmapTexture();
    void loadOverlayTexture();

    bool checkEviction(XYZTileKey tileKey, TerrainNode* tile);

    std::unordered_set<XYZTileKey> _loadingTiles;

    LRUCache<XYZTileKey, TerrainNode*> _memoryCache;
    LRUCache<XYZTileKey, void*> _diskCache; /* Key only LRU cache for tiles
                                             * on disk */

    /* ============================= Threading ============================= */
    unsigned _numLoadWorkers;
    MessageQueue<LoadResponse>* _doneQueue;
    std::vector<MessageQueue<LoadRequest>*> _loadRequestQueues;
    std::vector<LoadWorkerThread*> _loadWorkerThreads;

    MessageQueue<DiskDeallocationRequest>* _unloadRequestQueue;
    MessageQueue<DiskDeallocationResponse>* _unloadDoneQueue;
    DiskDeallocationWorkerThread* _unloadWorker;
    int _currentLoadThread = 0;

    /* Contains tile keys for nodes that are currenly in the disk unload
     * queue. Nodes whpse tile keys inside this set cannot be downloaded from
     * the web API while inside it, otherwise race conditions or file system
     * inconsistencies might occur. Since the nodes currently being evicted
     * are LRU, they are unlikely to be requested soon anyway. */
    std::unordered_set<XYZTileKey> _currentDiskCacheEvictions;

    /* Contains tile keys which cannot be loaded (i.e. API doesn't serve them,
     * for e.g. oceans at high zoom levels). This is so that each we don't
     * waste unneccessary requests if a tile cannot be loaded anyway. */
    std::unordered_set<XYZTileKey> _unloadableTileKeys;

    TerrainNode* _root;

    /* ======================== Meshes and shaders ========================= */
    Shader _terrainShader;
    Shader _skirtShader;
    Shader _poleShader;
    Shader _aabbShader;

    GridMesh* _gridMeshLow;
    GridMesh* _gridMeshMedium;
    GridMesh* _gridMeshHigh;
    SkirtMesh* _skirtMeshLow;
    SkirtMesh* _skirtMeshMedium;
    SkirtMesh* _skirtMeshHigh;
    AABBMesh* _aabbMesh;

    PoleMesh* _poleMesh;

    RenderStatistics& _stats;

    // glm::vec3 _globeRadiiSquared;

    float _timeToLiveMillis = 1000.0f * 5;

    unsigned _tileSideLengthHighRes, _tileSideLengthLowRes, _tileSideLengthMediumRes;
    unsigned _heightmapWidth, _heightmapHeight;
    unsigned _overlayWidth, _overlayHeight;
    unsigned _numberOfRequestedTiles = 0;

    bool _offlineWait = false;
    std::chrono::system_clock::time_point _lastNetworkError;
};

#endif // TERRAINMANAGER_H
