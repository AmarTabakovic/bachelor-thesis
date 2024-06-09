#ifndef AABBMESH_H
#define AABBMESH_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
#include <limits>
#include <vector>

class AABBMesh {
public:
    AABBMesh();
    void load();
    void render();
    void unload();

    void loadVertices();
    void loadIndices();

    // private:
    std::vector<unsigned> _indices;
    std::vector<float> _vertices;
    unsigned _vao, _vbo, _ebo;
};

#endif // AABB_H
