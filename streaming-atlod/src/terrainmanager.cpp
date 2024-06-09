#include "terrainmanager.h"

#include "terrainnode.h"

#include "../stb_image.h"
#include "configmanager.h"
#include "mapprojections.h"
#include "util.h"
#include <algorithm>
#include <filesystem>
#include <regex>

/**
 * @brief TerrainManager::TerrainManager
 */
TerrainManager::TerrainManager(RenderStatistics& stats, unsigned lowResMesh, unsigned mediumResMesh, unsigned highResMesh, unsigned memCacheSize, unsigned diskCacheSize)
    : _stats(stats)
    , _memoryCache(ConfigManager::getInstance()->_memoryCacheSize)
    , _diskCache(ConfigManager::getInstance()->_diskCacheSize)
{
    _terrainShader = Shader("../src/glsl/terrain.vert", "../src/glsl/terrain.frag");
    _skirtShader = Shader("../src/glsl/skirt.vert", "../src/glsl/skirt.frag");
    _poleShader = Shader("../src/glsl/pole.vert", "../src/glsl/pole.frag");
    _aabbShader = Shader("../src/glsl/aabb.vert", "../src/glsl/aabb.frag");

    _tileSideLengthHighRes = ConfigManager::getInstance()->_highMeshRes;
    _tileSideLengthLowRes = ConfigManager::getInstance()->_lowMeshRes;
    _tileSideLengthMediumRes = ConfigManager::getInstance()->_mediumMeshRes;

    _globeRadiiSquared = glm::vec3(100000.0f, 100000.0f, 100000.0f);

    /* Uniforms defined once */
    _terrainShader.use();
    _terrainShader.setInt("overlayTexture", 0);
    _terrainShader.setInt("heightmapTexture", 1);
    _terrainShader.setFloat("textureWidth", 512);
    _terrainShader.setFloat("textureHeight", 512);
    _terrainShader.setVec3("globeRadiusSquared", _globeRadiiSquared);

    _skirtShader.use();
    _skirtShader.setInt("overlayTexture", 0);
    _skirtShader.setInt("heightmapTexture", 1);
    _skirtShader.setFloat("textureWidth", 512);
    _skirtShader.setFloat("textureHeight", 512);
    _skirtShader.setVec3("globeRadiusSquared", _globeRadiiSquared);

    glm::vec3 circleMeshBorder = MapProjections::geodeticToCartesian(_globeRadiiSquared,
        glm::vec3(0, 0, glm::radians(85.0511f)));
    glm::vec3 circleMeshCenter = glm::vec3(0, glm::sqrt(_globeRadiiSquared).y, 0);
    float circleMeshRadius = glm::length(circleMeshCenter - circleMeshBorder);
    float circleMeshHeightDifference = circleMeshCenter.y - circleMeshBorder.y;

    _poleShader.use();
    _poleShader.setFloat("globeRadiusY", glm::sqrt(_globeRadiiSquared.y));
    _poleShader.setFloat("poleRadius", circleMeshRadius);
    _poleShader.setFloat("heightDelta", circleMeshHeightDifference);

    /* Define threads */
    _numLoadWorkers = ConfigManager::getInstance()->_numLoadWorkers;
    _loadRequestQueues.reserve(_numLoadWorkers);
    _loadWorkerThreads.reserve(_numLoadWorkers);

    _doneQueue = new MessageQueue<LoadResponse>;

    for (int i = 0; i < _numLoadWorkers; i++) {
        MessageQueue<LoadRequest>* loadRequestQueue = new MessageQueue<LoadRequest>;
        _loadRequestQueues.push_back(loadRequestQueue);
        _loadWorkerThreads.push_back(new LoadWorkerThread(loadRequestQueue, _doneQueue));
    }

    _unloadRequestQueue = new MessageQueue<UnloadRequest>;
    _unloadDoneQueue = new MessageQueue<UnloadResponse>;

    _unloadWorker = new UnloadWorkerThread(_unloadRequestQueue, _unloadDoneQueue);

    Util::checkGlError("SHADER FAILED");

    _maxZoom = 14;
    _minZoom = 0;
}

/**
 * @brief TerrainManager::setup
 */
void TerrainManager::setup()
{

    _gridMeshLow = new GridMesh(_tileSideLengthLowRes);
    _gridMeshLow->load();

    _skirtMeshLow = new SkirtMesh(_tileSideLengthLowRes);
    _skirtMeshLow->load();

    _gridMeshMedium = new GridMesh(_tileSideLengthMediumRes);
    _gridMeshMedium->load();

    _skirtMeshMedium = new SkirtMesh(_tileSideLengthMediumRes);
    _skirtMeshMedium->load();

    _gridMeshHigh = new GridMesh(_tileSideLengthHighRes);
    _gridMeshHigh->load();

    _skirtMeshHigh = new SkirtMesh(_tileSideLengthHighRes);
    _skirtMeshHigh->load();

    _poleMesh = new PoleMesh(30);
    _poleMesh->load();

    _aabbMesh = new AABBMesh();
    _aabbMesh->load();

    initDiskCache();

    /* Start threads */
    for (int i = 0; i < _numLoadWorkers; i++) {
        _loadWorkerThreads[i]->startInAnotherThread();
    }

    _unloadWorker->startInAnotherThread();

    /* Load root node */
    requestTile(XYZTileKey(0, 0, 0));
}

void TerrainManager::initTerrainTile(LoadResponse response)
{
    TerrainNode* tile = response.tile;

    glGenTextures(1, &tile->_overlayTextureId);
    glBindTexture(GL_TEXTURE_2D, tile->_overlayTextureId);

    Util::checkGlError("OVERLAY LOAD FAILED");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, response.overlayWidth, response.overlayHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, response.overlayData);
    glGenerateMipmap(GL_TEXTURE_2D);

    Util::checkGlError("OVERLAY LOAD FAILED");

    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &tile->_heightmapTextureId);
    glBindTexture(GL_TEXTURE_2D, tile->_heightmapTextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, response.heightWidth, response.heightHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, response.heightData);

    Util::checkGlError("HEIGHT LOAD FAILED");

    glBindTexture(GL_TEXTURE_2D, 0);

    /* Free overlay main memory, but keep height data for later usage */
    stbi_image_free(response.overlayData);
    // delete[] response.heightData;

    auto result = _memoryCache.put(tile->_xyzTileKey.string(), tile);
    if (result.evicted) {
        auto evictedKey = result.evictedItem.value().first;
        auto* evictedTile = result.evictedItem.value().second;

        /* If a node has children loaded or is the root node, we
         * do not want to deallocate it and insert it back,
         * and continue doing the above until finding a deallocatable
         * node. This policy is mainly intended for small cache sizes.
         *
         * This might seem problematic
         * at first sight, since it
         * 1. Sort of goes around the LRU policy by reinserting
         *    a LRU item at the front.
         * 2. Risks a long waiting time if many items in the rear
         *    are candidates for not getting evicted.
         *
         * But in practice, if the cache size is large enough,
         * it is not too problematic. Since we always traverse the tree
         * top-down, the LRU nodes tend to be leaf nodes, which makes
         * this while loop occur rarely.
         */
        while (!checkEviction(evictedKey, evictedTile)) {
            std::cout << "CHECK EVICTION FAILED " << evictedKey << std::endl;
            result = _memoryCache.put(evictedKey, evictedTile);
            evictedKey = result.evictedItem.value().first;
            evictedTile = result.evictedItem.value().second;
        }

        glDeleteTextures(1, &evictedTile->_heightmapTextureId);
        glDeleteTextures(1, &evictedTile->_overlayTextureId);
        delete[] evictedTile->_heightData;
        // evictedTile->unloadHeightmap();
        // evictedTile->unloadOverlay();

        delete evictedTile;
    }

    /* Do the same as above but for disk eviction */
    auto diskResult = _diskCache.put(tile->_xyzTileKey.string(), nullptr);
    if (diskResult.evicted) {
        auto evictedKey = diskResult.evictedItem.value().first;
        while (!checkEviction(evictedKey, nullptr) || _loadingTiles.count(evictedKey)) {
            std::cout << "DISK CHECK EVICTION FAILED " << evictedKey << std::endl;
            diskResult = _diskCache.put(evictedKey, nullptr);
            evictedKey = diskResult.evictedItem.value().first;
        }
        _currentDiskCacheEvictions.insert(evictedKey);
        // std::cout << "Inserted for eviction disk cache: " << evictedKey << std::endl;
        _unloadRequestQueue->push({ evictedKey, UNLOAD_REQUEST });
    }
}

/* TOCO: Create checkDiskEviction*/
bool TerrainManager::checkEviction(std::string tileKey, TerrainNode* tile)
{
    return !hasChildren(XYZTileKey(tileKey)) && tileKey != "0/0/0";
}

void TerrainManager::processSingleDoneQueueElement()
{

    std::optional<LoadResponse> responseMaybe = _doneQueue->pop();

    /* Empty response */
    if (!responseMaybe)
        return;

    LoadResponse response = responseMaybe.value();
    _numberOfRequestedTiles--;
    _loadingTiles.erase(response.tileKey.string());

    /* Handle potential errors or unloadable tiles */
    if (response.type == LOAD_UNLOADABLE) {
        /* Dealloc on failure */
        _unloadableTileKeys.insert(response.tileKey.string());
        if (response.heightData != nullptr) {
            delete[] response.heightData;
        }
        if (response.overlayData != nullptr) {
            stbi_image_free(response.overlayData);
        }
        delete response.tile;
        return;
    }

    initTerrainTile(response);
}

/**
 * @brief TerrainManager::collectRenderable
 * @param camera
 * @param currentTileKey
 * @param visibleTiles
 */
void TerrainManager::collectRenderable(Camera& camera, XYZTileKey currentTileKey, std::queue<std::string>& visibleTiles, XYZTileKey& minimumDistanceTileKey, float& minimumDistance)
{
    /* Put current tile to front of disk cache */
    _diskCache.get(currentTileKey.string());

    auto result = _memoryCache.get(currentTileKey.string());
    if (!result) {
        std::cerr << "Should not happen 3";
        std::exit(1);
    }
    TerrainNode* currentTile = result.value();
    currentTile->_lastUsedTimeStamp = std::chrono::system_clock::now();

    unsigned level = currentTileKey._z;

    bool visible = true;

    if (level <= 2)
        visible = true;
    else
        visible = camera.insideViewFrustum(currentTile->_aabbP1, currentTile->_aabbP2);

    /* We do horizon culling only from level 2 upwards */
    if (level >= 2)
        visible = visible && !currentTile->horizonCulled(camera);

    if (!visible)
        return;

    _stats.traversedNodes++;

    bool split = shouldSplit(camera, currentTileKey) && level < _maxZoom;

    if (!split) {
        updateMinimumDistanceTileKey(camera, currentTileKey, minimumDistanceTileKey, minimumDistance);
        visibleTiles.push(currentTileKey.string());
    } else {
        if (!allChildrenExistant(currentTileKey)) {
            visibleTiles.push(currentTileKey.string());
            updateMinimumDistanceTileKey(camera, currentTileKey, minimumDistanceTileKey, minimumDistance);

            /* Post nodes to request queue if they do not exist or are not
             * being loaded yet */
            requestChildren(currentTileKey);

        } else {
            /* Traverse four children (only if they are loaded) */
            collectRenderable(camera, currentTileKey.topLeftChild(), visibleTiles, minimumDistanceTileKey, minimumDistance);
            collectRenderable(camera, currentTileKey.topRightChild(), visibleTiles, minimumDistanceTileKey, minimumDistance);
            collectRenderable(camera, currentTileKey.bottomLeftChild(), visibleTiles, minimumDistanceTileKey, minimumDistance);
            collectRenderable(camera, currentTileKey.bottomRightChild(), visibleTiles, minimumDistanceTileKey, minimumDistance);
        }
    }
}

/**
 * @brief TerrainManager::updateMinimumDistanceTileKey
 * @param camera
 * @param currentTileKey
 * @param minimumDistanceTileKey
 * @param minimumDistance
 */
void TerrainManager::updateMinimumDistanceTileKey(Camera& camera, XYZTileKey currentTileKey, XYZTileKey& minimumDistanceTileKey, float& minimumDistance)
{

    glm::vec2 cameraLonLat = MapProjections::toGeodetic2D(camera.position(), _globeRadiiSquared);
    glm::vec2 cameraMerc = MapProjections::webMercator(cameraLonLat);

    float pow2Zoom = (float)(1 << currentTileKey._z);
    float minLon = (currentTileKey._x) / pow2Zoom;
    float maxLon = (currentTileKey._x + 1) / pow2Zoom;
    float minLat = (currentTileKey._y) / pow2Zoom;
    float maxLat = (currentTileKey._y + 1) / pow2Zoom;

    if (cameraMerc.x >= minLon && cameraMerc.x <= maxLon && cameraMerc.y >= minLat && cameraMerc.y <= maxLat) {
        minimumDistanceTileKey = currentTileKey;
    }
}

/**
 * @brief TerrainManager::requestChildren
 * @param tileKey
 */
void TerrainManager::requestChildren(XYZTileKey tileKey)
{
    requestTile(tileKey.topLeftChild());
    requestTile(tileKey.topRightChild());
    requestTile(tileKey.bottomLeftChild());
    requestTile(tileKey.bottomRightChild());
}

/**
 * @brief TerrainManager::requestTile
 * @param tileKey
 */
void TerrainManager::requestTile(XYZTileKey tileKey)
{
    if (!_memoryCache.contains(tileKey.string())
        && !_loadingTiles.count(tileKey.string())
        && !_unloadableTileKeys.count(tileKey.string())
        && !_currentDiskCacheEvictions.count(tileKey.string())) {
        _loadingTiles.insert(tileKey.string());

        LoadRequestType requestType = _diskCache.contains(tileKey.string()) ? LOAD_REQUEST_DISK_CACHE : LOAD_REQUEST;
        bool offlineMode = false;
        _loadRequestQueues[_currentLoadThread]->push({ tileKey, requestType, offlineMode });

        _currentLoadThread = (_currentLoadThread + 1) % _numLoadWorkers;
        _numberOfRequestedTiles++;
    }
}

/**
 * @brief TerrainManager::allChildrenExistant
 * @param tileKey
 * @return
 */
bool TerrainManager::allChildrenExistant(XYZTileKey tileKey)
{
    return _memoryCache.contains(tileKey.topLeftChild().string())
        && _memoryCache.contains(tileKey.topRightChild().string())
        && _memoryCache.contains(tileKey.bottomLeftChild().string())
        && _memoryCache.contains(tileKey.bottomRightChild().string());
}

/**
 * @brief TerrainManager::shouldSplit
 * @param camera
 * @param currentTileKey
 * @return
 */
bool TerrainManager::shouldSplit(Camera& camera, XYZTileKey currentTileKey)
{
    float pow2Level = (float)(1 << currentTileKey._z);
    TerrainNode* tile = _memoryCache.get(currentTileKey.string()).value();

    /* Iterate through grid points, check distance */
    for (auto p : tile->_projectedGridPoints) {
        glm::vec3 tempVec = p - camera.position();
        float distSquared = glm::dot(tempVec, tempVec);

        float baseDist = computeBaseDistWithLatitude(currentTileKey);

        float currentBaseDist = baseDist / pow2Level;
        if (distSquared <= currentBaseDist * currentBaseDist)
            return true;
    }
    return false;
}

float TerrainManager::computeBaseDistWithLatitude(XYZTileKey tileKey)
{
    float baseDist = glm::sqrt(_globeRadiiSquared.x) * 3.5;
    float pow2Level = (float)(1 << tileKey._z);

    if (tileKey._z >= 3 && std::abs(tileKey._y / pow2Level - 0.5f) >= 0.3) {
        baseDist *= 0.52;
    } else if (tileKey._z >= 3 && std::abs(tileKey._y / pow2Level - 0.5f) >= 0.4) {
        baseDist *= 0.41;
    }

    /*if (tileKey._z >= 10)
        baseDist *= 1.5;*/

    return baseDist;
}

/**
 * @brief TerrainManager::initDiskCache
 *
 * The disk cache is organized as follows:
 * - "path/dem/x_y_z.webp" for webp heightmaps
 * - "path/overlay/x_y_z.jpg" for jpg overlays
 */
void TerrainManager::initDiskCache()
{
    /* Steps:
     * - First traverse through all overlay tiles
     *      - If an overlay tile exists and a corresponding heightmap tile
     *        as well, put the tile key into a temporary list, otherwise
     *        delete overlay from disk
     * - Then traverse through all heightmap tiles
     *      - Skip if tile key already in temporary list
     *      - Delete heightmap if tile key not in temporary list
     * - Check temporary list size
     *      - If it exceeds disk cache capacity, remove until under capacity
     * - Put the tile keys from the temporary list to the in memory disk cache
     */
    std::string cacheLocation = ConfigManager::getInstance()->_diskCachePath;
    std::string demFolder = "dem/";
    std::string overlayFolder = "overlay/";

    if (!std::filesystem::exists(cacheLocation)) {
        std::filesystem::create_directory(cacheLocation);
    }

    if (!std::filesystem::exists(cacheLocation + demFolder)) {
        std::filesystem::create_directory(cacheLocation + demFolder);
    }

    if (!std::filesystem::exists(cacheLocation + overlayFolder)) {
        std::filesystem::create_directory(cacheLocation + overlayFolder);
    }

    std::unordered_map<std::string, std::string> jpgFiles;
    std::unordered_map<std::string, std::string> webpFiles;

    std::unordered_map<std::string, std::string> traversed;

    /* First traverse overlay images */
    std::regex filePattern(R"(^(\d+)_(\d+)_(\d+)\.(jpg)$)");
    for (auto& entry : std::filesystem::directory_iterator(cacheLocation + overlayFolder)) {
        auto path = entry.path();
        std::smatch matches;
        std::string filename = path.filename().string();

        if (std::regex_match(filename, matches, filePattern)) {
            std::string baseName = matches[1].str() + "_" + matches[2].str() + "_" + matches[3].str();
            std::string tileKey = matches[1].str() + "/" + matches[2].str() + "/" + matches[3].str();
            if (std::filesystem::exists(cacheLocation + demFolder + baseName + ".webp"))
                traversed[tileKey] = baseName;
            else {
                std::cout << "Remove overlay that has no heightmap " << tileKey << std::endl;
                std::filesystem::remove(cacheLocation + overlayFolder + baseName + ".jpg");
            }
        }
    }

    /* Then the heightmap images */
    filePattern = std::regex(R"(^(\d+)_(\d+)_(\d+)\.(webp)$)");
    for (auto& entry : std::filesystem::directory_iterator(cacheLocation + demFolder)) {
        auto path = entry.path();
        std::smatch matches;
        std::string filename = path.filename().string();

        if (std::regex_match(filename, matches, filePattern)) {
            std::string baseName = matches[1].str() + "_" + matches[2].str() + "_" + matches[3].str();
            std::string tileKey = matches[1].str() + "/" + matches[2].str() + "/" + matches[3].str();
            if (!traversed.count(tileKey)) {
                std::cout << "Remove heightmap that has no overlay" << std::endl;
                std::filesystem::remove(cacheLocation + demFolder + baseName + ".webp");
            }
        }
    }

    for (auto& entry : traversed) {
        auto diskResult = _diskCache.put(entry.first, nullptr);
        if (diskResult.evicted) {
            auto evictedKey = diskResult.evictedItem.value().first;
            auto evictedFile = traversed[evictedKey];
            while (!checkEviction(evictedKey, nullptr)) {
                std::cout << "DISK CHECK EVICTION FAILED " << evictedKey << std::endl;
                diskResult = _diskCache.put(evictedKey, nullptr);
                evictedKey = diskResult.evictedItem.value().first;
                evictedFile = traversed[evictedKey];
            }
            // std::cout << "Inserted for eviction disk cache: " << evictedKey << std::endl;
            std::cout << "STARTUP EVICT DISK CACHE " << evictedFile << std::endl;
            std::filesystem::remove(cacheLocation + demFolder + evictedFile + ".webp");
            std::filesystem::remove(cacheLocation + overlayFolder + evictedFile + ".jpg");
        }
    }
}

void TerrainManager::processAllDoneQueue()
{
    std::deque<LoadResponse> responses = _doneQueue->popAll();

    for (auto response : responses) {
        _numberOfRequestedTiles--;
        _loadingTiles.erase(response.tileKey.string());

        if (response.origin == LOAD_ORIGIN_API) {
            _stats.apiRequests += 2;
        }

        /* Handle potential errors or unloadable tiles */
        if (response.type == LOAD_UNLOADABLE || response.type == LOAD_TIMEOUT) {

            if (response.type == LOAD_UNLOADABLE)
                _unloadableTileKeys.insert(response.tileKey.string());

            if (response.heightData != nullptr) {
                delete[] response.heightData;
            }
            if (response.overlayData != nullptr) {
                stbi_image_free(response.overlayData);
            }
            delete response.tile;
            continue;
        }
        initTerrainTile(response);
    }
}

void TerrainManager::processAllUnloadDoneQueue()
{
    std::deque<UnloadResponse> responses = _unloadDoneQueue->popAll();

    for (auto response : responses) {
        _currentDiskCacheEvictions.erase(response.tileKey.string());
        if (response.type != UNLOAD_OK) {
            std::cerr << "Something went wrong disk dealloc" << std::endl;
        }
    }
}

void TerrainManager::render(Camera& camera, bool wireframe, bool aabb, bool& collision, float& verticalCollisionOffset)
{

    /* Reset stats */
    _stats.drawCalls = 0;
    _stats.renderedTriangles = 0;
    _stats.deepestZoomLevel = 0;
    _stats.visibleTiles = 0;
    _stats.traversedNodes = 0;

    /* Process concurrent message queues */
    processAllDoneQueue();
    processAllUnloadDoneQueue();

    /* Wait until the root node is loaded */
    if (!_memoryCache.contains("0/0/0"))
        return;

    std::queue<std::string> visibleTiles;
    XYZTileKey minimumDistanceTileKey("0/0/0");
    float minimumDistance = 99999.9f;
    collectRenderable(camera, XYZTileKey(0, 0, 0), visibleTiles, minimumDistanceTileKey, minimumDistance);

    collision = checkCollision(camera, minimumDistanceTileKey, verticalCollisionOffset);

    _stats.visibleTiles = visibleTiles.size();

    /* Render all visible tiles (front-to-back) */
    while (!visibleTiles.empty()) {
        auto topKey = visibleTiles.front();
        visibleTiles.pop();

        auto result = _memoryCache.get(topKey);
        TerrainNode* tile;
        if (!result) {
            std::cerr << "Should not happen";
            std::exit(1);
        }
        tile = result.value();

        _stats.deepestZoomLevel = std::max(tile->_xyzTileKey._z, _stats.deepestZoomLevel);

        if (XYZTileKey(topKey)._z <= 9) // 8
            renderTile(camera, tile, LOW, wireframe, aabb);
        else if (XYZTileKey(topKey)._z <= 11) // 10
            renderTile(camera, tile, MEDIUM, wireframe, aabb);
        else
            renderTile(camera, tile, HIGH, wireframe, aabb);
    }

    /* Render north and south poles */
    glBindVertexArray(_poleMesh->_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _poleMesh->_ebo);

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    _poleShader.use();
    _poleShader.setFloat("isNorthPole", (float)true);
    _poleMesh->render();

    /* TODO: This is a temporary hacky fix */
    _poleShader.setFloat("isNorthPole", (float)false);
    glCullFace(GL_FRONT);
    _poleMesh->render();
    glCullFace(GL_BACK);

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    _stats.drawCalls += 2;
    _stats.renderedTriangles += _poleMesh->_numRadians;

    _stats.currentlyRequested = _numberOfRequestedTiles;
    _stats.numberOfDiskCacheEntries = _diskCache.size();
    _stats.numberOfNodes = _memoryCache.size();
}

/**
 * @brief TerrainManager::renderTile
 * @param camera
 * @param tile
 */
void TerrainManager::renderTile(Camera& camera, TerrainNode* tile, TileResolution resolution, bool wireframe, bool aabb)
{
    unsigned zoom = tile->_xyzTileKey._z;

    unsigned sideLength;
    GridMesh* gridMesh;
    SkirtMesh* skirtMesh;

    switch (resolution) {
    case LOW:
        sideLength = _tileSideLengthLowRes;
        gridMesh = _gridMeshLow;
        skirtMesh = _skirtMeshLow;
        break;
    case MEDIUM:
        sideLength = _tileSideLengthMediumRes;
        gridMesh = _gridMeshMedium;
        skirtMesh = _skirtMeshMedium;
        break;
    case HIGH:
        sideLength = _tileSideLengthHighRes;
        gridMesh = _gridMeshHigh;
        skirtMesh = _skirtMeshHigh;
        break;
    }

    glBindVertexArray(skirtMesh->_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skirtMesh->_ebo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tile->_overlayTextureId);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tile->_heightmapTextureId);

    _skirtShader.use();

    _skirtShader.setFloat("tileWidth", sideLength);
    _skirtShader.setFloat("zoom", zoom);
    _skirtShader.setVec2("tileKey", glm::vec2((float)tile->_xyzTileKey._x, (float)tile->_xyzTileKey._y));

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        skirtMesh->render();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        skirtMesh->render();
    }

    glBindVertexArray(gridMesh->_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridMesh->_ebo);

    _terrainShader.use();

    /* Set colors for debug wireframe view */
    if (wireframe) {
        if (zoom % 3 == 0) {
            _terrainShader.setVec3("terrainColor", glm::vec3(1, 0, 0));
        } else if (zoom % 3 == 1) {
            _terrainShader.setVec3("terrainColor", glm::vec3(0, 1, 0));

        } else {
            _terrainShader.setVec3("terrainColor", glm::vec3(0, 0, 1));
        }
    }

    _terrainShader.setFloat("tileWidth", sideLength);
    _terrainShader.setFloat("zoom", zoom);
    _terrainShader.setVec2("tileKey", glm::vec2((float)tile->_xyzTileKey._x, (float)tile->_xyzTileKey._y));

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        gridMesh->render();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        gridMesh->render();
    }

    if (aabb) {
        _aabbShader.use();
        _aabbShader.setVec3("center", (tile->_aabbP1 + tile->_aabbP2) / 2.0f);
        glm::vec3 size = (tile->_aabbP2 - tile->_aabbP1);
        glm::vec3 center = (tile->_aabbP1 + tile->_aabbP2) / 2.0f;
        glm::mat4 transform = glm::translate(glm::mat4(1), center) * glm::scale(glm::mat4(1), size);
        _aabbShader.setMat4("model", transform);

        if (zoom % 3 == 0) {
            _aabbShader.setVec3("color", glm::vec3(1, 0, 0));
        } else if (zoom % 3 == 1) {
            _aabbShader.setVec3("color", glm::vec3(0, 1, 0));

        } else {
            _aabbShader.setVec3("color", glm::vec3(0, 0, 1));
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        _aabbMesh->render();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        _stats.drawCalls++;
        _stats.renderedTriangles += 12;
    }

    Util::checkGlError("Error while rendering node");

    _stats.drawCalls += 2;
    _stats.renderedTriangles += (sideLength * sideLength * 2) + (sideLength * 4 * 2);
}

bool TerrainManager::checkCollision(Camera& camera, XYZTileKey minimumDistanceTileKey, float& verticalCollisionOffset)
{

    auto result = _memoryCache.get(minimumDistanceTileKey.string());
    TerrainNode* currentTile = result.value();
    glm::vec2 lonlat = MapProjections::toGeodetic2D(camera.position(), _globeRadiiSquared);
    unsigned minimumDistZoom = minimumDistanceTileKey._z;
    float minDistZoomPow2 = (float)(1 << minimumDistZoom);
    float minLon = (minimumDistanceTileKey._x) / minDistZoomPow2;
    float maxLon = (minimumDistanceTileKey._x + 1) / minDistZoomPow2;
    float minLat = (minimumDistanceTileKey._y) / minDistZoomPow2;
    float maxLat = (minimumDistanceTileKey._y + 1) / minDistZoomPow2;

    glm::vec2 camMerc = MapProjections::webMercator(lonlat);
    float heightX = ((camMerc.x - minLon) / (maxLon - minLon));
    float heightY = ((camMerc.y - minLat) / (maxLat - minLat));
    glm::vec2 lonLatTex = MapProjections::inverseWebMercator(glm::vec2(heightX, heightY));

    verticalCollisionOffset = 0.0;

    if (heightX >= 0 && heightX <= 1 && heightY >= 0 && heightY <= 1) {

        /* Get nearest height value.
         * This could be improved in the future with bilinear interpolation. */
        float height = currentTile->getScaledHeight(heightX * 511, heightY * 511);

        glm::vec3 projected = MapProjections::geodeticToCartesian(_globeRadiiSquared, glm::vec3(lonLatTex.x, height + 0.04, lonLatTex.y));

        float distCamera = glm::length(camera.position());
        float distHeight = glm::length(projected);

        if (glm::length(projected) > distCamera) {
            verticalCollisionOffset = distHeight - distCamera;
            return true;
        }
        return false;
    }
    return false;
}

bool TerrainManager::hasChildren(XYZTileKey tileKey)
{
    return _memoryCache.contains(tileKey.topLeftChild().string())
        || _memoryCache.contains(tileKey.topRightChild().string())
        || _memoryCache.contains(tileKey.bottomLeftChild().string())
        || _memoryCache.contains(tileKey.bottomRightChild().string());
}

void TerrainManager::shutdown()
{
    /* Shut down worker threads */
    /*_unloadDoneQueue->push({});
    for (auto* q : _loadRequestQueues) {
        q->push({});
    }*/

    /* Deallocate nodes */
}
