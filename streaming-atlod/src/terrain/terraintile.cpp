#include "terraintile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#include "../util.h"
#include "gridmesh.h"
#include "terrainmanager.h"

#include "../mapprojections.h"
#include <filesystem>
#include <iostream>
#include <webp/decode.h>

unsigned TerrainTile::requestCount = 0;

TerrainTile::TerrainTile(glm::vec3 worldSpaceCenterPos, TerrainManager* manager, unsigned zoom, std::pair<unsigned, unsigned> tileKey)
{
    _worldSpaceCenterPos = worldSpaceCenterPos;
    _terrainManager = manager;
    _zoom = zoom;
    _tileKey = tileKey;

    generateAabb();
}

void TerrainTile::update(Camera& camera)
{
    // std::cout << requestCount << std::endl;
    updateRec(camera, 0);
}

void TerrainTile::render(Camera& camera)
{
    renderRec(camera, 0);
}

bool TerrainTile::shouldSplit(Camera& camera)
{
    /* TODO: exact split metric should be thought out well */
    glm::vec2 globalTileKeyCenter((_tileKey.first + 0.5f) / (1 << _zoom), (_tileKey.second + 0.5) / (1 << _zoom));
    glm::vec2 p1Temp = MapProjections::globalTileXZToLonLat(globalTileKeyCenter);
    glm::vec3 spherePos = MapProjections::geodeticToCartesian(glm::vec3(1000, 1000, 1000), glm::vec3(p1Temp.x, 0, p1Temp.y));

    glm::vec3 tempVec = spherePos - camera.position();
    // tempVec.y = 0;
    float distSquared = glm::dot(tempVec, tempVec);
    float hypSquared = _terrainManager->_tileSideLength * _terrainManager->_tileSideLength * 0.75 * 0.75;
    float oneOverLevelSquared = (1.0f / (float)(1 << _zoom)) * (1.0f / (float)(1 << _zoom));
    float lodMetricSquared = hypSquared * oneOverLevelSquared;
    if (distSquared < lodMetricSquared)
        return true;
    return false;
}

void TerrainTile::generateAabb()
{
    float pow2Level = (float)(1 << _zoom);

    glm::vec2 globalTileKeyCenter((_tileKey.first + 0.5f) / pow2Level, (_tileKey.second + 0.5f) / pow2Level);

    float globalTileKeyP1X = globalTileKeyCenter.x + (0.5f / pow2Level);
    float globalTileKeyP1Y = globalTileKeyCenter.y + (0.5f / pow2Level);

    float globalTileKeyP2X = globalTileKeyCenter.x - (0.5f / pow2Level);
    float globalTileKeyP2Y = globalTileKeyCenter.y - (0.5f / pow2Level);

    float globalTileKeyP3X = globalTileKeyCenter.x + (0.5f / pow2Level);
    float globalTileKeyP3Y = globalTileKeyCenter.y - (0.5f / pow2Level);

    float globalTileKeyP4X = globalTileKeyCenter.x - (0.5f / pow2Level);
    float globalTileKeyP4Y = globalTileKeyCenter.y + (0.5f / pow2Level);

    glm::vec2 p1Temp = MapProjections::globalTileXZToLonLat(glm::vec2(globalTileKeyP1X, globalTileKeyP1Y));
    glm::vec2 p2Temp = MapProjections::globalTileXZToLonLat(glm::vec2(globalTileKeyP2X, globalTileKeyP2Y));
    glm::vec2 p3Temp = MapProjections::globalTileXZToLonLat(glm::vec2(globalTileKeyP3X, globalTileKeyP3Y));
    glm::vec2 p4Temp = MapProjections::globalTileXZToLonLat(glm::vec2(globalTileKeyP4X, globalTileKeyP4Y));

    /* TODO: The radius is hardcoded here just for testing, should be changed */
    /* TODO: Read the y from heightmap, might have to use max and min heights? */
    glm::vec3 spherePosP1 = MapProjections::geodeticToCartesian(glm::vec3(1000, 1000, 1000), glm::vec3(p1Temp.x, 0, p1Temp.y));
    glm::vec3 spherePosP2 = MapProjections::geodeticToCartesian(glm::vec3(1000, 1000, 1000), glm::vec3(p2Temp.x, 0, p2Temp.y));
    glm::vec3 spherePosP3 = MapProjections::geodeticToCartesian(glm::vec3(1000, 1000, 1000), glm::vec3(p3Temp.x, 0, p3Temp.y));
    glm::vec3 spherePosP4 = MapProjections::geodeticToCartesian(glm::vec3(1000, 1000, 1000), glm::vec3(p4Temp.x, 0, p4Temp.y));

    float maxX = std::max({ spherePosP1.x,
        spherePosP2.x,
        spherePosP3.x,
        spherePosP4.x });

    float maxY = std::max({ spherePosP1.y,
        spherePosP2.y,
        spherePosP3.y,
        spherePosP4.y });

    float maxZ = std::max({ spherePosP1.z,
        spherePosP2.z,
        spherePosP3.z,
        spherePosP4.z });

    float minX = std::min({ spherePosP1.x,
        spherePosP2.x,
        spherePosP3.x,
        spherePosP4.x });

    float minY = std::min({ spherePosP1.y,
        spherePosP2.y,
        spherePosP3.y,
        spherePosP4.y });

    float minZ = std::min({ spherePosP1.z,
        spherePosP2.z,
        spherePosP3.z,
        spherePosP4.z });

    _aabbP2 = glm::vec3(maxX, maxY, maxZ);
    _aabbP1 = glm::vec3(minX, minY, minZ);
}

void TerrainTile::updateRec(Camera& camera, unsigned level)
{
    /* - Check visibility
     * - Check distance
     * - Check heightmap/overlay availability
     */

    _visible = camera.insideViewFrustum(_aabbP1, _aabbP2);

    if (!_visible)
        return;

    bool split = shouldSplit(camera);

    if (!split || level == _terrainManager->_maxZoom) {
        /* TODO: Multithreaded and networked loading */
        if (!_heightmapLoaded) {
            std::cout << "--- Tile " << _tileKey.first << ", " << _tileKey.second << std::endl;
            std::cout << "Loading heightmap" << std::endl;
            std::string path = "../../data/maptiler/dem/" + std::to_string(level) + "/" + std::to_string(std::get<0>(_tileKey)) + "/" + std::to_string(std::get<1>(_tileKey)) + ".webp";
            loadHeightmap(path);
            _heightmapLoaded = true;
            requestCount++;
        }

        if (!_overlayLoaded) {
            std::cout << "Loading overlay" << std::endl;
            std::string path = "../../data/maptiler/overlay/" + std::to_string(level) + "/" + std::to_string(std::get<0>(_tileKey)) + "/" + std::to_string(std::get<1>(_tileKey)) + ".jpg";
            loadOverlay(path);
            _overlayLoaded = true;
            requestCount++;
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
    _terrainManager->_terrainShader.use();

    if (_toRender) {
        // if (level >= 3) {
        /* TODO: Move these binds to gridmesh class */
        //  glBindVertexArray(_terrainManager->_gridMesh1->_vao);
        //  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _terrainManager->_gridMesh1->_ebo);
        //  _terrainManager->_terrainShader.setFloat("tileWidth", _terrainManager->_tileSideLength * 2);
        //} else {
        glBindVertexArray(_terrainManager->_gridMesh->_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _terrainManager->_gridMesh->_ebo);
        //_terrainManager->_terrainShader.setFloat("tileWidth", _terrainManager->_tileSideLength);
        //}

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _overlayTextureId);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _heightmapTextureId);

        Util::checkGlError("TILE TEXTURE BIND FAILED");

        if (level % 3 == 0) {
            _terrainManager->_terrainShader.setVec3("terrainColor", glm::vec3(1, 0, 0));
        } else if (level % 3 == 1) {
            _terrainManager->_terrainShader.setVec3("terrainColor", glm::vec3(0, 1, 0));

        } else {
            _terrainManager->_terrainShader.setVec3("terrainColor", glm::vec3(0, 0, 1));
        }

        _terrainManager->_terrainShader.setVec3("worldSpaceCenterPos", _worldSpaceCenterPos);
        _terrainManager->_terrainShader.setFloat("zoom", _zoom);
        _terrainManager->_terrainShader.setVec2("tileKey", glm::vec2((float)_tileKey.first, (float)_tileKey.second));

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
        _terrainManager->_skirtShader.setVec2("tileKey", glm::vec2((float)_tileKey.first, (float)_tileKey.second));

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
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
        std::cerr << "Image not square" << std::endl;
        std::exit(1);
    }
}
