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

inline glm::vec3 scaleToGeodeticSurface(glm::vec3 p, glm::vec3 globeRadiiSquared)
{
    glm::dvec3 globeRadiiSquaredD(globeRadiiSquared);
    glm::dvec3 globeRadiiToTheFourth(glm::dvec3(globeRadiiSquared) * glm::dvec3(globeRadiiSquared));
    glm::dvec3 oneOverRadiiSquared(glm::dvec3(1.0) / glm::dvec3(globeRadiiSquared));
    double beta = 1.0 / glm::sqrt((p.x * p.x) * oneOverRadiiSquared.x + (p.y * p.y) * oneOverRadiiSquared.y + (p.z * p.z) * oneOverRadiiSquared.z);

    double n = glm::length(glm::dvec3(
        beta * p.x * oneOverRadiiSquared.x,
        beta * p.y * oneOverRadiiSquared.y,
        beta * p.z * oneOverRadiiSquared.z));

    double alpha = (1.0 - beta) * (glm::length(p) / n);

    double x2 = p.x * p.x;
    double y2 = p.y * p.y;
    double z2 = p.z * p.z;

    double da = 0.0, db = 0.0, dc = 0.0;
    double s = 0.0;
    double dSdA = 1.0;

    do {
        alpha -= (s / dSdA);
        da = 1.0 + (alpha * oneOverRadiiSquared.x);
        db = 1.0 + (alpha * oneOverRadiiSquared.y);
        dc = 1.0 + (alpha * oneOverRadiiSquared.z);

        double da2 = da * da;
        double db2 = db * db;
        double dc2 = dc * dc;

        double da3 = da * da2;
        double db3 = db * db2;
        double dc3 = dc * dc2;

        s = x2 / (globeRadiiSquaredD.x * da2)
            + y2 / (globeRadiiSquaredD.y * db2)
            + z2 / (globeRadiiSquaredD.z * dc2) - 1.0;

        dSdA = -2.0 * (x2 / (globeRadiiToTheFourth.x * da3) + y2 / (globeRadiiToTheFourth.y * db3) + z2 / (globeRadiiToTheFourth.z * dc3));

    } while (glm::abs(s) > 1e-10);

    return glm::vec3((float)(p.x / da), (float)(p.y / db), (float)(p.z / dc));
};

inline glm::vec3 geodeticSurfaceNormal(glm::vec3 p, glm::vec3 globeRadiiSquared)
{
    glm::vec3 normal = p * (glm::vec3(1.0f) / globeRadiiSquared);
    return glm::normalize(normal);
}

inline glm::vec2 toGeodetic2D(glm::vec3 position, glm::vec3 globeRadiiSquared)
{
    glm::vec3 n = geodeticSurfaceNormal(position, globeRadiiSquared);
    return glm::vec2(glm::atan2(n.z, n.x) * -1, glm::asin(n.y / glm::length(n)));
}

inline glm::vec3 toGeodetic3D(glm::vec3 position, glm::vec3 globeRadiiSquared)
{
    glm::vec3 p = scaleToGeodeticSurface(position, globeRadiiSquared);
    glm::vec3 h = position - p;
    float height = glm::sign(glm::dot(h, position)) * glm::length(h);
    glm::vec2 lonlat = toGeodetic2D(p, globeRadiiSquared);
    return glm::vec3(lonlat.x, height, lonlat.y);
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

inline glm::vec2 inverseWebMercator(glm::vec2 globalTileXY)
{
    float lon = glm::radians((globalTileXY.x * 360.0f - 180.0f) * -1.0f);
    float lat = glm::radians((glm::atan(glm::exp(glm::pi<float>() * (1.0f - 2.0f * globalTileXY.y))) * 2.0f - glm::pi<float>() / 2.0f) * 180.0f / glm::pi<float>());
    return glm::vec2(lon, lat);
}

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
