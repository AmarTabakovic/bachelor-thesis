#ifndef GLOBALCONSTANTS_H
#define GLOBALCONSTANTS_H

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

namespace GlobalConstants {
const glm::vec3 GLOBE_RADII_SQUARED(100000.0f);
const glm::vec3 GLOBE_RADII(glm::sqrt(GLOBE_RADII_SQUARED));

const float CAMERA_NEAR = 0.01f;
const float CAMERA_FAR = 1300.0f;
}

#endif // GLOBALCONSTANTS_H
