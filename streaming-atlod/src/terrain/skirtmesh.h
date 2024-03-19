#ifndef SKIRTMESH_H
#define SKIRTMESH_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <limits>
#include <vector>

class SkirtMesh
{
public:
public:
    static constexpr GLuint RESTART_INDEX = std::numeric_limits<GLuint>::max();

    SkirtMesh(unsigned sideLength);
    void load();
    void render();
    void unload();

    unsigned _sideLength;
    std::vector<unsigned> _indices;
    std::vector<float> _vertices;
    unsigned _vao, _vbo, _ebo;

    void loadVertices();
    void loadIndices();
};

#endif // SKIRTMESH_H
