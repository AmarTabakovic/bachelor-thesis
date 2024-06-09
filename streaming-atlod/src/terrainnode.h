#ifndef TERRAINODE_H
#define TERRAINODE_H

#include "camera.h"

#include "xyztilekey.h"
#include <chrono>
#include <glm/vec3.hpp>
#include <string>
#include <utility>
#include <vector>

class TerrainManager;

/**
 * @brief
 */
class TerrainNode {
public:
    enum TileState {
        GPU_READY, /* Data is on the GPU, ready to render */
        LOADING_FROM_MEMORY, /* Data is being loaded from CPU to GPU */
        CPU_READY, /* Data is on the CPU, ready to be loaded on GPU */
        LOADING_FROM_DISK, /* Data is being loaded from disk to main memory */
        CACHED, /* Data is on disk, can be loaded in */
        DOWNLOADING_FROM_API, /* Data is being downloaded */
        DOWNLOADABLE, /* Data is not on disk, but might be downloadable via API */
        UNDOWNLOADABLE /* Data was attempted to be downloaded, but API returned
                          e.g. a 204 return code */
    };

    // TerrainTile(glm::vec3 worldSpaceCenterPos, TerrainManager* manager, unsigned zoom, std::pair<unsigned, unsigned> tileKey, TerrainTile* parent);
    TerrainNode(XYZTileKey tileKey);
    bool horizonCulled(Camera& camera);
    void generateMinMaxHeight();

    // private:

    void generateAabb();
    void generateAabbZoom0();
    void generateAabbZoom1();
    void generateProjectedGridPoints();
    void generateHorizonPoints();

    glm::vec3 getHeight(unsigned x, unsigned y);

    float getScaledHeight(unsigned x, unsigned y);

    std::chrono::system_clock::time_point _lastUsedTimeStamp;

    XYZTileKey _xyzTileKey;
    glm::vec3 _wgs86CenterPos;

    glm::vec3 _aabbP1, _aabbP2;

    // glm::vec3 _wgs86AabbP1, _wgs86AabbP2;

    float _minHeight, _maxHeight;

    std::vector<glm::vec3> _projectedGridPoints;
    std::vector<glm::vec3> _horizonCullingPoints;

    unsigned char *_heightData, _textureData;

    unsigned _overlayTextureId, _heightmapTextureId; /* For OpenGL texture objects */
};

#endif // TERRAINNODE_H
