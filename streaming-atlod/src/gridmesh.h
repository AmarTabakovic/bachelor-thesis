#ifndef GRIDMESH_H
#define GRIDMESH_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <limits>
#include <vector>

#include "util.h"

class GridMesh
{
public:
    static constexpr GLuint RESTART_INDEX = std::numeric_limits<GLuint>::max();

    GridMesh(unsigned sideLength);
    void load();
    void render();
    void unload();

    // private:
    unsigned _sideLength;
    std::vector<unsigned> _indices;
    std::vector<float> _vertices;
    unsigned _vao, _vbo, _ebo;

    void loadVertices();
    void loadIndices();
};

#endif // GRIDMESH_H
