#include "skirtmesh.h"
#include "util.h"

/**
 * @brief SkirtMesh::SkirtMesh
 * @param sideLength
 */
SkirtMesh::SkirtMesh(unsigned sideLength)
{
    _sideLength = sideLength;
}

/**
 * @brief SkirtMesh::load
 */
void SkirtMesh::load()
{
    loadVertices();
    loadIndices();
}

/**
 * @brief SkirtMesh::render
 */
void SkirtMesh::render()
{
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLE_STRIP, _indices.size() * sizeof(unsigned int), GL_UNSIGNED_INT, (void*)0);
    Util::checkGlError("temp");
    glDisable(GL_PRIMITIVE_RESTART);
}

/**
 * @brief SkirtMesh::loadVertices
 */
void SkirtMesh::loadVertices()
{

    /*
     * The vertex-pairs (original vertex and duplicate vertex) are added in
     * the following order.
     *
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
     *
     * We insert the duplicate vertex before the original vertex to make
     * the index buffer loading easier.
     */

    /* Vertices 0 - 3 */
    for (int j = 0; j < _sideLength; j++) {
        float x = (-(float)_sideLength / 2.0f + (float)_sideLength * j / (float)_sideLength) + 0.5;
        float z = (-(float)_sideLength / 2.0f + (float)_sideLength * 0 / (float)_sideLength) + 0.5;

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(1); /* Duplicate vertex true */

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(0); /* Duplicate vertex false */
    }

    /* Vertices 4 - 5 */
    for (int i = 1; i < _sideLength - 1; i++) {
        float x = (-(float)_sideLength / 2.0f + (float)_sideLength * (float)(_sideLength - 1) / (float)_sideLength) + 0.5;
        float z = (-(float)_sideLength / 2.0f + (float)_sideLength * i / (float)_sideLength) + 0.5;

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(1); /* Duplicate vertex true */

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(0); /* Duplicate vertex false */
    }

    /* Vertices 6 - 9 */
    for (int j = _sideLength - 1; j >= 0; j--) {
        float x = (-(float)_sideLength / 2.0f + (float)_sideLength * j / (float)_sideLength) + 0.5;
        float z = (-(float)_sideLength / 2.0f + (float)_sideLength * (float)(_sideLength - 1) / (float)_sideLength) + 0.5;

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(1); /* Duplicate vertex true */

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(0); /* Duplicate vertex false */
    }

    /* Vertices 10 - 11 */
    for (int i = _sideLength - 2; i >= 1; i--) {
        float x = (-(float)_sideLength / 2.0f + (float)_sideLength * 0 / (float)_sideLength) + 0.5;
        float z = (-(float)_sideLength / 2.0f + (float)_sideLength * i / (float)_sideLength) + 0.5;

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(1); /* Duplicate vertex true */

        _vertices.push_back(x); /* Position x */
        _vertices.push_back(z); /* Position z */
        _vertices.push_back(0); /* Duplicate vertex false */
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

/**
 * @brief SkirtMesh::loadIndices
 */
void SkirtMesh::loadIndices()
{
    for (unsigned i = 0; i < _vertices.size() / 3; i++) {
        _indices.push_back(i);
    }

    /* Wrap the skirt around at the origin */
    _indices.push_back(0);
    _indices.push_back(1);

    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(unsigned), &_indices[0], GL_STATIC_DRAW);
    Util::checkGlError("SKIRTMESH EBO LOAD FAILED");
}

/**
 * @brief SkirtMesh::unload
 */
void SkirtMesh::unload()
{
    /* TODO */
}
