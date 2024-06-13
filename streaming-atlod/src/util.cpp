#include "util.h"

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

void Util::checkGlError(const std::string& message)
{
    GLenum error = glGetError();
    if (error != 0) {
        std::cerr << "Error: " << message << std::endl;
        std::cerr << "OpenGL error code: " << error << std::endl;
        std::exit(1);
    }
}

/**
 * @brief Util::vec3ToString
 * @param vec
 * @return
 */
std::string Util::vec3ToString(const glm::vec3& vec)
{
    return "(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ")";
}
