#include "terrainnode.h"

// #define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#include "globalconstants.h"
#include "mapprojections.h"
#include <filesystem>
#include <iostream>
#include <limits>
#include <webp/decode.h>

/**
 * @brief TerrainNode::TerrainNode
 * @param tileKey
 */
TerrainNode::TerrainNode(XYZTileKey tileKey)
    : _xyzTileKey(tileKey)
    , _lastUsedTimeStamp(std::chrono::system_clock::now())
    , _minHeight(std::numeric_limits<float>::max())
    , _maxHeight(std::numeric_limits<float>::lowest())
{
    _projectedGridPoints.reserve(9);
    _horizonCullingPoints.reserve(9);
}

/**
 * @brief TerrainNode::generateMinMaxHeight
 */
void TerrainNode::generateMinMaxHeight()
{
    for (int i = 0; i < 512; i++) {
        for (int j = 0; j < 512; j++) {
            _minHeight = glm::min(_minHeight, getScaledHeight(i, j));
            _maxHeight = glm::max(_maxHeight, getScaledHeight(i, j));
        }
    }
}

/**
 * @brief TerrainNode::getHeight
 * @param x
 * @param y
 * @return
 */
glm::vec3 TerrainNode::getHeight(unsigned x, unsigned y)
{
    unsigned index = (y * 512 + x) * 3;
    return glm::vec3(_heightData[index],
        _heightData[index + 1],
        _heightData[index + 2]);
}

/**
 * @brief TerrainNode::getScaledHeight
 * @param x
 * @param y
 * @return
 */
float TerrainNode::getScaledHeight(unsigned x, unsigned y)
{
    glm::vec3 height = getHeight(x, y);
    return (-10000.0f + ((height.x * 256.0f * 256.0f + height.y * 256.0f + height.z) * 0.1f)) * GlobalConstants::HEIGHT_SCALE;
}

/**
 * @brief TerrainNode::horizonCulled
 * @param camera
 * @return
 */
bool TerrainNode::horizonCulled(Camera& camera)
{
    glm::vec3 cv = glm::vec3(camera.position()) / GlobalConstants::GLOBE_RADII;

    double vhMagnitudeSquared = glm::dot(cv, cv) - 1.0;

    for (auto p : _horizonCullingPoints) {
        glm::vec3 t = p / GlobalConstants::GLOBE_RADII;
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
 * @brief TerrainNode::generateProjectedGridPoints
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
    float pow2Level = (float)(1 << _xyzTileKey.z());
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec2 pTemp = MapProjections::inverseWebMercator(glm::vec2(_xyzTileKey.x() + i * 0.5f, _xyzTileKey.y() + j * 0.5f) / pow2Level);
            unsigned coordX = glm::floor(i * (511.0 / 3.0));
            unsigned coordY = glm::floor(j * (511.0 / 3.0));
            float height = getScaledHeight(coordX, coordY);
            glm::vec3 spherePos = MapProjections::geodeticToCartesian(GlobalConstants::GLOBE_RADII_SQUARED, glm::vec3(pTemp.x, height, pTemp.y));

            _projectedGridPoints.push_back(spherePos);
        }
    }
}

/**
 * @brief TerrainNode::generateHorizonPoints
 */
void TerrainNode::generateHorizonPoints()
{
    float pow2Level = (float)(1 << _xyzTileKey.z());
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec2 pTemp = MapProjections::inverseWebMercator(glm::vec2(_xyzTileKey.x() + i * 0.5f, _xyzTileKey.y() + j * 0.5f) / pow2Level);
            glm::vec3 spherePos = MapProjections::geodeticToCartesian(GlobalConstants::GLOBE_RADII_SQUARED, glm::vec3(pTemp.x, _maxHeight + 6.5f, pTemp.y));
            _horizonCullingPoints.push_back(spherePos);
        }
    }
}

/**
 * @brief TerrainNode::generateAabb
 */
void TerrainNode::generateAabb()
{

    unsigned zoom = _xyzTileKey.z();
    if (zoom == 0)
        generateAabbZoom0();
    else if (zoom == 1)
        generateAabbZoom1();
    else {
        float maxX = std::numeric_limits<float>::lowest(),
              maxY = std::numeric_limits<float>::lowest(),
              maxZ = std::numeric_limits<float>::lowest();

        float minX = std::numeric_limits<float>::max(),
              minY = std::numeric_limits<float>::max(),
              minZ = std::numeric_limits<float>::max();

        float pow2Level = (1 << zoom);

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                glm::vec2 lonLat = MapProjections::inverseWebMercator(glm::vec2((float)_xyzTileKey.x() + (float)i * 0.5f, (float)_xyzTileKey.y() + (float)j * 0.5f) / pow2Level);
                glm::vec3 spherePos = MapProjections::geodeticToCartesian(GlobalConstants::GLOBE_RADII_SQUARED, glm::vec3(lonLat.x, _minHeight, lonLat.y));
                maxX = std::max(maxX, spherePos.x);
                maxY = std::max(maxY, spherePos.y);
                maxZ = std::max(maxZ, spherePos.z);
                minX = std::min(minX, spherePos.x);
                minY = std::min(minY, spherePos.y);
                minZ = std::min(minZ, spherePos.z);

                spherePos = MapProjections::geodeticToCartesian(GlobalConstants::GLOBE_RADII_SQUARED, glm::vec3(lonLat.x, _maxHeight, lonLat.y));
                maxX = std::max(maxX, spherePos.x);
                maxY = std::max(maxY, spherePos.y);
                maxZ = std::max(maxZ, spherePos.z);
                minX = std::min(minX, spherePos.x);
                minY = std::min(minY, spherePos.y);
                minZ = std::min(minZ, spherePos.z);
            }
        }

        _aabbP1 = glm::vec3(minX, minY, minZ);
        _aabbP2 = glm::vec3(maxX, maxY, maxZ);
    }
}

/**
 * @brief TerrainNode::generateAabbZoom0
 */
void TerrainNode::generateAabbZoom0()
{
    _aabbP2 = GlobalConstants::GLOBE_RADII;
    _aabbP1 = GlobalConstants::GLOBE_RADII * -1.0f;
}

/**
 * @brief TerrainNode::generateAabbZoom1
 */
void TerrainNode::generateAabbZoom1()
{
    float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
    float minZ = -GlobalConstants::GLOBE_RADII.z;
    float maxZ = GlobalConstants::GLOBE_RADII.z;
    if (_xyzTileKey.x() == 0 && _xyzTileKey.y() == 0) {
        minX = -GlobalConstants::GLOBE_RADII.x;
        maxY = GlobalConstants::GLOBE_RADII.y;
    } else if (_xyzTileKey.x() == 1 && _xyzTileKey.y() == 0) {
        maxX = GlobalConstants::GLOBE_RADII.x;
        maxY = GlobalConstants::GLOBE_RADII.y;
    } else if (_xyzTileKey.x() == 0 && _xyzTileKey.y() == 1) {
        minX = -GlobalConstants::GLOBE_RADII.x;
        minY = -GlobalConstants::GLOBE_RADII.y;
    } else if (_xyzTileKey.x() == 1 && _xyzTileKey.y() == 1) {
        maxX = GlobalConstants::GLOBE_RADII.x;
        minY = -GlobalConstants::GLOBE_RADII.y;
    }

    _aabbP1 = glm::vec3(minX, minY, minZ);
    _aabbP2 = glm::vec3(maxX, maxY, maxZ);
}
