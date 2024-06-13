#ifndef UTIL_H
#define UTIL_H

#include <glm/vec3.hpp>
#include <string>

namespace Util {
void checkGlError(const std::string& message);
std::string vec3ToString(const glm::vec3& vec);
}

#endif // UTIL_H
