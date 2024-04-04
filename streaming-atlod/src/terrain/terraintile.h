#ifndef TERRAINTILE_H
#define TERRAINTILE_H

#include "../camera.h"

#include <glm/vec3.hpp>
#include <string>
#include <utility>
#include <vector>

class TerrainManager;

/**
 * @brief
 */
class TerrainTile
{
public:
    enum TileState {
        GPU_READY, /* Data is on the GPU, ready to render */
        LOADING_FROM_MEMORY, /* Data is being loaded from CPU to GPU */
        CPU_READY, /* Data is on the CPU, ready to be loaded on GPU */
        LOADING_FROM_DISK, /* Data is being loaded from disk to main memory */
        CACHED, /* Data is on disk, can be loaded in */
        LOADING_FROM_API, /* Data is being downloaded */
        DOWNLOADABLE, /* Data is not on disk, but might be downloadable via API */
        UNDOWNLOADABLE /* Data was attempted to be downloaded, but API returned
                          e.g. a 204 return code */
    };

    static unsigned requestCount;

    TerrainTile(glm::vec3 worldSpaceCenterPos, TerrainManager* manager, unsigned zoom, std::pair<unsigned, unsigned> tileKey);
    void loadOverlay(const std::string& fileName); /* TODO temporary with file */
    void loadHeightmap(const std::string& fileName);
    void unloadOverlay();
    void unloadHeightmap();

    void update(Camera& camera);
    void render(Camera& camera);
    void updateRec(Camera& camera, unsigned level);
    void renderRec(Camera& camera, unsigned level);
    bool shouldSplit(Camera& camera);

    /* Pointers to children */
    TerrainTile* _topLeftChild = nullptr;
    TerrainTile* _topRightChild = nullptr;
    TerrainTile* _bottomLeftChild = nullptr;
    TerrainTile* _bottomRightChild = nullptr;

    TileState _state = DOWNLOADABLE;

    // private:

    void checkImageSquare(int imgWidth, int imgHeight);

    void loadHeightmapWebP(const std::string& fileName);

    glm::vec3 _worldSpaceCenterPos; /* TODO: Rename this? Or specify more precisely what this is */
    glm::vec3 _wgs86CenterPos;
    glm::vec2 _globalTileSpaceCenterPos; /**/

    glm::vec3 _aabbP1, _aabbP2;
    glm::vec3 _wgs86AabbP1, _wgs86AabbP2;

    std::pair<unsigned, unsigned> _tileKey; /* XYZ key */
    unsigned _zoom; /* I.e. LOD level */

    bool _visible = false, _toRender = false;

    bool _heightmapLoaded = false, _overlayLoaded = false;

    TerrainManager* _terrainManager; /* Pointer to terrain manager owning
                                        this tile */
    unsigned _overlayTextureId, _heightmapTextureId; /* For OpenGL texture objects */
};

#endif // TERRAINTILE_H
