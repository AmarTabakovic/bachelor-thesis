#ifndef POLEMESH_H
#define POLEMESH_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <limits>
#include <vector>

class PoleMesh
{
public:
    static constexpr GLuint RESTART_INDEX = std::numeric_limits<GLuint>::max();

    PoleMesh(unsigned numRadians);

    void load();
    void render();
    void unload();

    unsigned _numRadians;
    std::vector<unsigned> _indices;
    std::vector<float> _vertices;
    unsigned _vao, _vbo, _ebo;

    void loadVertices();
    void loadIndices();
};

#endif // POLEMESH_H
