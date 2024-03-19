#include "terraintile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#include "../util.h"
#include "gridmesh.h"
#include "terrainmanager.h"

#include <filesystem>
#include <iostream>
#include <webp/decode.h>

TerrainTile::TerrainTile(glm::vec3 worldSpaceCenterPos, TerrainManager* manager, unsigned zoom, std::pair<unsigned, unsigned> tileKey)
{
    _worldSpaceCenterPos = worldSpaceCenterPos;
    _terrainManager = manager;
    _zoom = zoom;
    _tileKey = tileKey;
}

void TerrainTile::update(Camera& camera)
{
    updateRec(camera, 0);
}

void TerrainTile::render(Camera& camera)
{
    renderRec(camera, 0);
}

bool TerrainTile::shouldSplit(Camera& camera)
{
    glm::vec3 tempVec = _worldSpaceCenterPos - camera.position();
    float distSquared = glm::dot(tempVec, tempVec);
    float lodMetric = 100 * (1.0f / (float)(1 << _zoom));
    if (distSquared < lodMetric * lodMetric)
        return true;
    return false;
}

void TerrainTile::updateRec(Camera& camera, unsigned level)
{
    /* - Check visibility
     * - Check distance
     * - Check heightmap/overlay availability
     */

    _visible = true;

    glm::vec3 p1 = _worldSpaceCenterPos - ((_terrainManager->_tileSideLength - 1) * (1.0f / (float)(1 << level))) / 2;
    glm::vec3 p2 = _worldSpaceCenterPos + ((_terrainManager->_tileSideLength - 1) * (1.0f / (float)(1 << level))) / 2;
    _visible = camera.insideViewFrustum(p1, p2);

    if (!_visible)
        return;

    bool split = shouldSplit(camera);

    if (!split || level == _terrainManager->_maxZoom) {
        if (!_heightmapLoaded) {
            std::cout << "--- Tile " << _tileKey.first << ", " << _tileKey.second << std::endl;
            std::cout << "Loading heightmap" << std::endl;
            std::string path = "../data/maptiler/dem/" + std::to_string(level) + "/" + std::to_string(std::get<0>(_tileKey)) + "/" + std::to_string(std::get<1>(_tileKey)) + ".webp";
            loadHeightmap(path);
            _heightmapLoaded = true;
        }

        if (!_overlayLoaded) {
            std::cout << "Loading overlay" << std::endl;
            std::string path = "../data/maptiler/overlay/" + std::to_string(level) + "/" + std::to_string(std::get<0>(_tileKey)) + "/" + std::to_string(std::get<1>(_tileKey)) + ".jpg";
            loadOverlay(path);
            _overlayLoaded = true;
        }

        _toRender = true;

    }

    else if (level < _terrainManager->_maxZoom) {
        unsigned tileKeyX = _tileKey.first;
        unsigned tileKeyY = _tileKey.second;
        float sideLength = (_terrainManager->_tileSideLength - 1) * (1.0f / (float)(1 << level));
        if (_topLeftChild == nullptr) {
            std::pair<unsigned, unsigned> newTileKey = std::make_pair(2 * tileKeyX, 2 * tileKeyY);

            float newX = _worldSpaceCenterPos.x - sideLength * 0.25;
            float newZ = _worldSpaceCenterPos.z - sideLength * 0.25;

            glm::vec3 newPos(newX, 0, newZ);

            std::cout << "CURRENT WS CENTER " << _worldSpaceCenterPos.x << ", " << _worldSpaceCenterPos.z << std::endl;
            std::cout << "LEVEL " << level << std::endl;
            std::cout << "TILE SIDE LENGTH" << _terrainManager->_tileSideLength << std::endl;
            std::cout << "CALCULATED SIDE LENGTH " << sideLength << std::endl;

            _topLeftChild = new TerrainTile(newPos, _terrainManager, level + 1, newTileKey);
        }
        if (_topRightChild == nullptr) {
            std::pair<unsigned, unsigned> newTileKey = std::make_pair(2 * tileKeyX + 1, 2 * tileKeyY);

            float newX = _worldSpaceCenterPos.x + sideLength * 0.25;
            float newZ = _worldSpaceCenterPos.z - sideLength * 0.25;

            glm::vec3 newPos(newX, 0, newZ);
            _topRightChild = new TerrainTile(newPos, _terrainManager, level + 1, newTileKey);
        }
        if (_bottomLeftChild == nullptr) {
            std::pair<unsigned, unsigned> newTileKey = std::make_pair(2 * tileKeyX, 2 * tileKeyY + 1);

            float newX = _worldSpaceCenterPos.x - sideLength * 0.25;
            float newZ = _worldSpaceCenterPos.z + sideLength * 0.25;

            glm::vec3 newPos(newX, 0, newZ);

            _bottomLeftChild = new TerrainTile(newPos, _terrainManager, level + 1, newTileKey);
        }
        if (_bottomRightChild == nullptr) {
            std::pair<unsigned, unsigned> newTileKey = std::make_pair(2 * tileKeyX + 1, 2 * tileKeyY + 1);

            float newX = _worldSpaceCenterPos.x + sideLength * 0.25;
            float newZ = _worldSpaceCenterPos.z + sideLength * 0.25;

            glm::vec3 newPos(newX, 0, newZ);

            _bottomRightChild = new TerrainTile(newPos, _terrainManager, level + 1, newTileKey);
        }

        _topLeftChild->updateRec(camera, level + 1);
        _topRightChild->updateRec(camera, level + 1);
        _bottomLeftChild->updateRec(camera, level + 1);
        _bottomRightChild->updateRec(camera, level + 1);
    }
}

void TerrainTile::renderRec(Camera& camera, unsigned level)
{
    /* For each visible tile, make draw call */

    if (_toRender) {
        glBindVertexArray(_terrainManager->_gridMesh->_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _terrainManager->_gridMesh->_ebo);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _overlayTextureId);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _heightmapTextureId);

        Util::checkGlError("TILE TEXTURE BIND FAILED");

        _terrainManager->_terrainShader.use();

        if (level % 3 == 0) {
            _terrainManager->_terrainShader.setVec3("terrainColor", glm::vec3(1, 0, 0));
            _terrainManager->_skirtShader.setVec3("terrainColor", glm::vec3(1, 0, 0));
        } else if (level % 3 == 1) {
            _terrainManager->_terrainShader.setVec3("terrainColor", glm::vec3(0, 1, 0));
            _terrainManager->_skirtShader.setVec3("terrainColor", glm::vec3(0, 1, 0));

        } else {
            _terrainManager->_terrainShader.setVec3("terrainColor", glm::vec3(0, 0, 1));
            _terrainManager->_skirtShader.setVec3("terrainColor", glm::vec3(0, 0, 1));
        }

        _terrainManager->_terrainShader.setVec3("worldSpaceCenterPos", _worldSpaceCenterPos);
        _terrainManager->_terrainShader.setFloat("zoom", _zoom);

        _terrainManager->_gridMesh->render();

        glBindVertexArray(_terrainManager->_skirtMesh->_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _terrainManager->_skirtMesh->_ebo);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _overlayTextureId);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _heightmapTextureId);

        Util::checkGlError("Test");

        _terrainManager->_skirtShader.use();

        _terrainManager->_skirtShader.setVec3("worldSpaceCenterPos", _worldSpaceCenterPos);
        _terrainManager->_skirtShader.setFloat("zoom", _zoom);

        _terrainManager->_skirtMesh->render();

        _toRender = false;
        _visible = false;

    } else if (level < _terrainManager->_maxZoom) {

        if (_topLeftChild != nullptr && _topLeftChild->_visible) {
            _topLeftChild->renderRec(camera, level + 1);
        }
        if (_topRightChild != nullptr && _topRightChild->_visible) {
            _topRightChild->renderRec(camera, level + 1);
        }
        if (_bottomLeftChild != nullptr && _bottomLeftChild->_visible) {
            _bottomLeftChild->renderRec(camera, level + 1);
        }
        if (_bottomRightChild != nullptr && _bottomRightChild->_visible) {
            _bottomRightChild->renderRec(camera, level + 1);
        }
    }
}

// void TerrainTile::loadQuantizedMesh() {}

/**
 * @brief TerrainTile::loadHeightmap
 *
 * Currently designed for WebP heightmaps
 */
void TerrainTile::loadHeightmap(const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Unable to open file " << fileName << std::endl;
        std::exit(1);
    }

    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> fileData(fileSize);
    if (!file.read(fileData.data(), fileSize)) {
        std::cerr << "Error: Unable to read file " << fileName << std::endl;
        std::exit(1);
    }

    std::vector<unsigned char> imageData;
    int width, height;

    if (WebPGetInfo((const uint8_t*)(fileData.data()), fileSize, &width, &height) != 1) {
        std::cerr << "Error: Failed to get WebP image info" << std::endl;
        std::exit(1);
    }

    imageData.resize(width * height * 3); /* RGB format */

    if (WebPDecodeRGBInto((const uint8_t*)(fileData.data()), fileSize,
            imageData.data(), width * height * 3, width * 3)
        == nullptr) {
        std::cerr << "Error: Failed to decode WebP image" << std::endl;
        std::exit(1);
    }

    glGenTextures(1, &_heightmapTextureId);
    glBindTexture(GL_TEXTURE_2D, _heightmapTextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData.data());
}

/**
 * @brief TerrainTile::loadOverlay
 *
 * Currently designed for JPG overlays
 */
void TerrainTile::loadOverlay(const std::string& fileName)
{
    int width, height, nrChannels;
    unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        checkImageSquare(width, height);

        glGenTextures(1, &_overlayTextureId);
        glBindTexture(GL_TEXTURE_2D, _overlayTextureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        Util::checkGlError("OVERLAY LOAD FAILED");

        glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        std::cerr << "Failed opening overlay texture" << std::endl;
        std::exit(1);
    }

    stbi_image_free(data);
}

void TerrainTile::loadHeightmapWebP(const std::string& fileName)
{
}

void TerrainTile::checkImageSquare(int imgWidth, int imgHeight)
{
    if (imgWidth != imgHeight) {
        std::cout << "Image not square" << std::endl;
        std::exit(1);
    }
}
