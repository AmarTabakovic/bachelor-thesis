#ifndef MAPPROJECTIONS_H
#define MAPPROJECTIONS_H

#include <cmath>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/**
 * Geographic map projections.
 *
 * Some of the projections were taken from the book
 * "3D Engine Design for Virtual Globes" by Kevin Ring and TODO.
 */
namespace MapProjections {

const float PI = 3.141592f;

glm::vec3 geodeticSurfaceNormal(glm::vec3 geodetic)
{
    float cosLat = cos(geodetic.z);

    return glm::vec3(cosLat * cos(geodetic.x), sin(geodetic.z), cosLat * sin(geodetic.x));
}

glm::vec3 geodeticToCartesian(glm::vec3 globeRadiiSquared, glm::vec3 geodetic)
{
    glm::vec3 n = geodeticSurfaceNormal(geodetic);
    glm::vec3 k = globeRadiiSquared * n;
    float gamma = sqrt(k.x * n.x + k.y * n.y + k.z * n.z);

    glm::vec3 rSurface = k / gamma;
    return rSurface + (geodetic.y * n);
}

glm::vec2 globalTileXZToLonLat(glm::vec2 globalTileXZ)
{
    float lon = glm::radians((globalTileXZ.x * 360.0f - 180.0f) * -1);
    float lat = glm::radians((atan(exp(PI * (1.0f - 2.0f * globalTileXZ.y))) * 2.0f - PI / 2.0f) * 180.0f / PI);
    return glm::vec2(lon, lat);
}

}

#endif // MAPPROJECTIONS_H
