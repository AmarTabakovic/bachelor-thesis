#ifndef TERRAINMANAGER_H
#define TERRAINMANAGER_H

#include <deque>
#include <utility>
#include <vector>

#include "../camera.h"
#include "../shader.h"
#include "gridmesh.h"
#include "skirtmesh.h"

class TerrainTile;

/**
 * @brief The terrain manager manages a collection of terrain tiles.
 */
class TerrainManager {
public:
    TerrainManager();
    void setup();

    void update();
    void render(Camera& camera);

    // private:

    Shader _terrainShader;
    Shader _skirtShader;

    TerrainTile* _root;

    GridMesh* _gridMesh;
    GridMesh* _gridMesh1;
    GridMesh* _gridMesh2;
    SkirtMesh* _skirtMesh;

    unsigned _maxZoom;
    unsigned _minZoom;

    unsigned _tileSideLength;
    unsigned _heightmapWidth, _heightmapHeight;
    unsigned _overlayWidth, _overlayHeight;
};

#endif // TERRAINMANAGER_H
