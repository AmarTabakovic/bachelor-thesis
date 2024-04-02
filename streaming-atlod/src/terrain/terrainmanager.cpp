#include "terrainmanager.h"
#include "terraintile.h"

#include "../util.h"

TerrainManager::TerrainManager()
{
    _terrainShader = Shader("../src/glsl/terrain.vert", "../src/glsl/terrain.frag");
    _skirtShader = Shader("../src/glsl/skirt.vert", "../src/glsl/skirt.frag");

    Util::checkGlError("SHADER FAILED");

    _tileSideLength = 128;

    _terrainShader.use();
    _terrainShader.setInt("overlayTexture", 0);
    _terrainShader.setInt("heightmapTexture", 1);
    _terrainShader.setFloat("textureWidth", 512);
    _terrainShader.setFloat("textureHeight", 512);
    _terrainShader.setFloat("tileWidth", _tileSideLength);

    _skirtShader.use();
    _skirtShader.setInt("overlayTexture", 0);
    _skirtShader.setInt("heightmapTexture", 1);
    _skirtShader.setFloat("textureWidth", 512);
    _skirtShader.setFloat("textureHeight", 512);
    _skirtShader.setFloat("tileWidth", _tileSideLength);

    Util::checkGlError("SHADER FAILED");

    _maxZoom = 5;
    _minZoom = 0;
}

void TerrainManager::setup()
{
    _gridMesh = new GridMesh(_tileSideLength);
    _gridMesh->load();

    /*_gridMesh1 = new GridMesh(_tileSideLength * 2);
    _gridMesh1->load();

    _gridMesh2 = new GridMesh(_tileSideLength * 4);
    _gridMesh2->load();*/

    _skirtMesh = new SkirtMesh(_tileSideLength);
    _skirtMesh->load();

    _root = new TerrainTile(glm::vec3(0, 0, 0), this, 0, { 0, 0 });
}

void TerrainManager::render(Camera& camera)
{
    //_terrainShader.use();

    // glBindVertexArray(_gridMesh->_vao);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _gridMesh->_ebo);

    _root->update(camera);
    _root->render(camera);
}
