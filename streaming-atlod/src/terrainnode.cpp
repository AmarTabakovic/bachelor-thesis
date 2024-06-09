#include "terrainnode.h"

// #define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#include "mapprojections.h"
#include <filesystem>
#include <iostream>
#include <webp/decode.h>

TerrainNode::TerrainNode(XYZTileKey tileKey)
    : _xyzTileKey(tileKey)
{
    _lastUsedTimeStamp = std::chrono::system_clock::now();
    _projectedGridPoints.reserve(9);
    _horizonCullingPoints.reserve(9);
}

void TerrainNode::generateMinMaxHeight()
{
    _minHeight = 9999, _maxHeight = -9999;
    for (int i = 0; i < 512; i++) {
        for (int j = 0; j < 512; j++) {
            _minHeight = glm::min(_minHeight, getScaledHeight(i, j));
            _maxHeight = glm::max(_maxHeight, getScaledHeight(i, j));
        }
    }
}

glm::vec3 TerrainNode::getHeight(unsigned x, unsigned y)
{
    unsigned index = (y * 512 + x) * 3;
    return glm::vec3(_heightData[index],
        _heightData[index + 1],
        _heightData[index + 2]);
}

float TerrainNode::getScaledHeight(unsigned x, unsigned y)
{
    glm::vec3 height = getHeight(x, y);
    return (-10000.0f + ((height.x * 256.0f * 256.0f + height.y * 256.0f + height.z) * 0.1f)) / 20169.51;
}

/**
 * @brief TerrainTile::horizonCulled
 * @param camera
 * @return
 */
bool TerrainNode::horizonCulled(Camera& camera)
{
    glm::vec3 globeRadiusSquared = glm::vec3(100000, 100000, 100000);
    glm::vec3 globeRadius = glm::sqrt(globeRadiusSquared);

    glm::vec3 cv = glm::vec3(camera.position()) / globeRadius;

    double vhMagnitudeSquared = glm::dot(cv, cv) - 1.0;

    for (auto p : _horizonCullingPoints) {
        glm::vec3 t = p / globeRadius;
        glm::vec3 vt = t - cv;
        float vtMagnitudeSquared = glm::dot(vt, vt);
        float vtDotVc = -1.0f * glm::dot(vt, cv);
        bool isOccluded = vtDotVc > vhMagnitudeSquared && vtDotVc * vtDotVc / vtMagnitudeSquared > vhMagnitudeSquared;

        if (!isOccluded)
            return false;
    }

    return true;
}

/**
 * @brief TerrainTile::generateProjectedGridPoints
 *
 * The grid points are laid out as follows before being projected to the WGS84
 * ellipsoid:
 *
 * * - - - * - - - *
 * |               |
 * |               |
 * |               |
 * *       *       *
 * |               |
 * |               |
 * |               |
 * * - - - * - - - *
 */
void TerrainNode::generateProjectedGridPoints()
{
    float pow2Level = (float)(1 << _xyzTileKey._z);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec2 pTemp = MapProjections::inverseWebMercator(glm::vec2(_xyzTileKey._x + i * 0.5f, _xyzTileKey._y + j * 0.5f) / pow2Level);
            glm::vec3 globeRadius = glm::vec3(100000, 100000, 100000);
            // float estimatedHeight = 1.05; // 0.85;
            //*(1 / pow2Level);

            // glm::vec3 spherePos = MapProjections::geodeticToCartesian(globeRadius, glm::vec3(pTemp.x, _maxHeight, pTemp.y));
            unsigned coordX = glm::floor(i * (511.0 / 3.0));
            unsigned coordY = glm::floor(j * (511.0 / 3.0));
            float height = getScaledHeight(coordX, coordY);
            glm::vec3 spherePos = MapProjections::geodeticToCartesian(globeRadius, glm::vec3(pTemp.x, height, pTemp.y));

            _projectedGridPoints.push_back(spherePos);
        }
    }
}

void TerrainNode::generateHorizonPoints()
{
    float pow2Level = (float)(1 << _xyzTileKey._z);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec2 pTemp = MapProjections::inverseWebMercator(glm::vec2(_xyzTileKey._x + i * 0.5f, _xyzTileKey._y + j * 0.5f) / pow2Level);
            glm::vec3 globeRadius = glm::vec3(100000, 100000, 100000);
            // float estimatedHeight = 1.05; // 0.85;
            //*(1 / pow2Level);

            glm::vec3 spherePos = MapProjections::geodeticToCartesian(globeRadius, glm::vec3(pTemp.x, _maxHeight + 5.5f, pTemp.y));

            _horizonCullingPoints.push_back(spherePos);
        }
    }
}

void TerrainNode::generateAabb()
{

    unsigned zoom = _xyzTileKey._z;
    if (zoom == 0)
        generateAabbZoom0();
    else if (zoom == 1)
        generateAabbZoom1();
    else {
        float maxX = -9999, maxY = -9999, maxZ = -9999;
        float minX = 9999, minY = 9999, minZ = 9999;
        glm::vec3 globeRadius = glm::vec3(100000.0f, 100000.0f, 100000.0f);
        float pow2Level = (1 << zoom);

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                // for (int k = 0; k < 3; k++) {
                // float height = getScaledHeight(std::floor(i * 255.5), std::floor(j * 255.5)) * (1.0f / pow2Level);
                glm::vec2 lonLat = MapProjections::inverseWebMercator(glm::vec2((float)_xyzTileKey._x + (float)i * 0.5f, (float)_xyzTileKey._y + (float)j * 0.5f) / pow2Level);
                glm::vec3 spherePos = MapProjections::geodeticToCartesian(globeRadius, glm::vec3(lonLat.x, /*0 * ((float)k - 1.0f * 0) **/ _minHeight /* (1.0f / pow2Level)*/, lonLat.y));
                maxX = std::max(maxX, spherePos.x);
                maxY = std::max(maxY, spherePos.y);
                maxZ = std::max(maxZ, spherePos.z);
                minX = std::min(minX, spherePos.x);
                minY = std::min(minY, spherePos.y);
                minZ = std::min(minZ, spherePos.z);

                spherePos = MapProjections::geodeticToCartesian(globeRadius, glm::vec3(lonLat.x, /*0 * ((float)k - 1.0f * 0) **/ _maxHeight /* (1.0f / pow2Level)*/, lonLat.y));
                maxX = std::max(maxX, spherePos.x);
                maxY = std::max(maxY, spherePos.y);
                maxZ = std::max(maxZ, spherePos.z);
                minX = std::min(minX, spherePos.x);
                minY = std::min(minY, spherePos.y);
                minZ = std::min(minZ, spherePos.z);
                // }
            }
        }

        _aabbP1 = glm::vec3(minX, minY, minZ);
        //-glm::vec3(0.5, 0.5, 0.5);
        _aabbP2 = glm::vec3(maxX, maxY, maxZ);
        //+glm::vec3(0.5, 0.5, 0.5);
    }
}

void TerrainNode::generateAabbZoom0()
{
    glm::vec3 globeRadius = glm::sqrt(glm::vec3(100000.0f, 100000.0f, 100000.0f));
    _aabbP2 = globeRadius;
    _aabbP1 = globeRadius * -1.0f;
}

/**
 * @brief TerrainTile::generateAabbZoom1
 */
void TerrainNode::generateAabbZoom1()
{
    glm::vec3 globeRadius = glm::vec3(100000.0f, 100000.0f, 100000.0f);
    globeRadius = glm::sqrt(globeRadius);
    globeRadius += 100.0f;
    float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
    float minZ = -globeRadius.z;
    float maxZ = globeRadius.z;
    if (_xyzTileKey._x == 0 && _xyzTileKey._y == 0) {
        minX = -globeRadius.x;
        maxY = globeRadius.y;
    } else if (_xyzTileKey._x == 1 && _xyzTileKey._y == 0) {
        maxX = globeRadius.x;
        maxY = globeRadius.y;
    } else if (_xyzTileKey._x == 0 && _xyzTileKey._y == 1) {
        minX = -globeRadius.x;
        minY = -globeRadius.y;
    } else if (_xyzTileKey._x == 1 && _xyzTileKey._y == 1) {
        maxX = globeRadius.x;
        minY = -globeRadius.y;
    }

    _aabbP1 = glm::vec3(minX, minY, minZ);
    _aabbP2 = glm::vec3(maxX, maxY, maxZ);
}
