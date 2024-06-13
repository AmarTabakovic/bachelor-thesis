#ifndef MAPPROJECTIONS_H
#define MAPPROJECTIONS_H

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/**
 * Geographic map projections.
 *
 * Some of the projections were taken from the book
 * "3D Engine Design for Virtual Globes" by Kevin Ring and TODO.
 */
namespace MapProjections {

/**
 * @brief geodeticSurfaceNormal
 * @param p
 * @param globeRadiiSquared
 * @return
 */
inline glm::vec3 geodeticSurfaceNormal(glm::vec3 p, glm::vec3 globeRadiiSquared)
{
    glm::vec3 normal = p * (glm::vec3(1.0f) / globeRadiiSquared);
    return glm::normalize(normal);
}

/**
 * @brief toGeodetic2D
 * @param position
 * @param globeRadiiSquared
 * @return
 */
inline glm::vec2 toGeodetic2D(glm::vec3 position, glm::vec3 globeRadiiSquared)
{
    glm::vec3 n = geodeticSurfaceNormal(position, globeRadiiSquared);
    return glm::vec2(glm::atan2(n.z, n.x) * -1, glm::asin(n.y / glm::length(n)));
}

/**
 * @brief geodeticSurfaceNormal
 * @param geodetic
 * @return
 */
inline glm::vec3 geodeticSurfaceNormal(glm::vec3 geodetic)
{
    float cosLat = glm::cos(geodetic.z);

    return glm::vec3(cosLat * glm::cos(geodetic.x), glm::sin(geodetic.z), cosLat * glm::sin(geodetic.x));
}

/**
 * @brief geodeticToCartesian
 * @param globeRadiiSquared
 * @param geodetic
 * @return
 */
inline glm::vec3 geodeticToCartesian(glm::vec3 globeRadiiSquared, glm::vec3 geodetic)
{
    glm::vec3 n = geodeticSurfaceNormal(geodetic);
    glm::vec3 k = globeRadiiSquared * n;
    float gamma = glm::sqrt(k.x * n.x + k.y * n.y + k.z * n.z);

    glm::vec3 rSurface = k / gamma;
    return rSurface + (geodetic.y * n);
}

/**
 * @brief inverseWebMercator
 * @param globalTileXY
 * @return
 */
inline glm::vec2 inverseWebMercator(glm::vec2 globalTileXY)
{
    float lon = glm::radians((globalTileXY.x * 360.0f - 180.0f) * -1.0f);
    float lat = glm::radians((glm::atan(glm::exp(glm::pi<float>() * (1.0f - 2.0f * globalTileXY.y))) * 2.0f - glm::pi<float>() / 2.0f) * 180.0f / glm::pi<float>());
    return glm::vec2(lon, lat);
}

/**
 * @brief webMercator
 * @param lonLat
 * @return
 */
inline glm::vec2 webMercator(glm::vec2 lonLat)
{
    float lon = lonLat.x;
    float lat = lonLat.y;

    float x = (glm::degrees(lon) + 180.0f) / 360.0f;

    float radLat = lat;
    float mercN = glm::log(glm::tan(glm::pi<float>() / 4.0f + radLat / 2.0f));
    float y = 0.5f - mercN / (2.0f * glm::pi<float>());

    return glm::vec2(x, y);
}
}

#endif // MAPPROJECTIONS_H
