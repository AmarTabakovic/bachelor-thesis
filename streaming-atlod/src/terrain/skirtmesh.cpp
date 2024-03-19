#include "skirtmesh.h"
#include "../util.h"

SkirtMesh::SkirtMesh(unsigned sideLength)
{
    _sideLength = sideLength;
}

void SkirtMesh::load()
{
    loadVertices();
    loadIndices();
}

void SkirtMesh::render()
{
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLE_STRIP, _indices.size() * sizeof(unsigned int), GL_UNSIGNED_INT, (void*)0);
    Util::checkGlError("temp");
    glDisable(GL_PRIMITIVE_RESTART);
}

void SkirtMesh::loadVertices()
{

    /*
     * 0 - - - 1 - - - 2 - - - 3
     * |                       |
     * |                       |
     * |                       |
     * 11                      4
     * |                       |
     * |                       |
     * |                       |
     * 10                      5
     * |                       |
     * |                       |
     * |                       |
     * 9 - - - 8 - - - 7 - - - 6
     */

    /* Create VAO and VBO and load vertices */
    for (int j = 0; j < _sideLength; j++) {
        float x = (-(float)_sideLength / 2.0f + (float)_sideLength * j / (float)_sideLength) + 0.5; /* TODO +0.5 probably wrong*/
        float z = (-(float)_sideLength / 2.0f + (float)_sideLength * 0 / (float)_sideLength) + 0.5;

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(0);

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(1);
    }

    for (int i = 1; i < _sideLength - 1; i++) {
        float x = (-(float)_sideLength / 2.0f + (float)_sideLength * (float)(_sideLength - 1) / (float)_sideLength) + 0.5; /* TODO +0.5 probably wrong*/
        float z = (-(float)_sideLength / 2.0f + (float)_sideLength * i / (float)_sideLength) + 0.5;

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(0);

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(1);
    }

    for (int j = _sideLength - 1; j >= 0; j--) {
        float x = (-(float)_sideLength / 2.0f + (float)_sideLength * j / (float)_sideLength) + 0.5; /* TODO +0.5 probably wrong*/
        float z = (-(float)_sideLength / 2.0f + (float)_sideLength * (float)(_sideLength - 1) / (float)_sideLength) + 0.5;

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(0);

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(1);
    }

    for (int i = _sideLength - 2; i >= 1; i--) {
        float x = (-(float)_sideLength / 2.0f + (float)_sideLength * 0 / (float)_sideLength) + 0.5; /* TODO +0.5 probably wrong*/
        float z = (-(float)_sideLength / 2.0f + (float)_sideLength * i / (float)_sideLength) + 0.5;

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(0);

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(1);
    }

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    Util::checkGlError("SKIRTMESH VAO GEN FAILED");

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), &_vertices[0], GL_STATIC_DRAW);

    Util::checkGlError("SKIRTMESH VBO LOAD FAILED");

    /* Position attribute */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Util::checkGlError("SKIRTMESH VAO LOAD FAILED");
}

void SkirtMesh::loadIndices()
{
    for (unsigned i = 0; i < _vertices.size() / 3; i++) {
        _indices.push_back(i);
        //_indices.push_back(i);
    }

    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(unsigned), &_indices[0], GL_STATIC_DRAW);
    Util::checkGlError("SKIRTMESH EBO LOAD FAILED");
}

void SkirtMesh::unload()
{
    /* TODO */
}
