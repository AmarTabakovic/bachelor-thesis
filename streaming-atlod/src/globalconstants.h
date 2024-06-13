#ifndef GLOBALCONSTANTS_H
#define GLOBALCONSTANTS_H

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace GlobalConstants {
const glm::vec3 GLOBE_RADII_SQUARED(100000.0f);
const glm::vec3 GLOBE_RADII(glm::sqrt(GLOBE_RADII_SQUARED));
const float HEIGHT_SCALE = GLOBE_RADII.x / 6378137.0f;

const float CAMERA_NEAR = 0.01f;
const float CAMERA_FAR = 1300.0f;

const std::string OVERLAY_DIR_NAME = "overlay/";
const std::string HEIGHTDATA_DIR_NAME = "heightdata/";
}

#endif // GLOBALCONSTANTS_H
