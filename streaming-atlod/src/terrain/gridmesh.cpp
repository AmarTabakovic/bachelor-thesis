#include "gridmesh.h"

GridMesh::GridMesh(unsigned sideLength)
{
    _sideLength = sideLength;
}

void GridMesh::load()
{
    loadVertices();
    loadIndices();
}

void GridMesh::render()
{

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLE_STRIP, _indices.size() * sizeof(unsigned int), GL_UNSIGNED_INT, (void*)0);
    Util::checkGlError("temp");
    glDisable(GL_PRIMITIVE_RESTART);
}

void GridMesh::loadVertices()
{
    /* Create VAO and VBO and load vertices */
    for (int i = 0; i < _sideLength; i++) {
        for (int j = 0; j < _sideLength; j++) {
            /* Load vertices around center point */
            float x = (-(float)_sideLength / 2.0f + (float)_sideLength * j / (float)_sideLength) + 0.5; /* TODO +0.5 probably wrong*/
            float z = (-(float)_sideLength / 2.0f + (float)_sideLength * i / (float)_sideLength) + 0.5;

            _vertices.push_back(x); /* Position x */
            _vertices.push_back(z); /* Position z */
        }
    }

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    Util::checkGlError("GRIDMESH VAO GEN FAILED");

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), &_vertices[0], GL_STATIC_DRAW);

    Util::checkGlError("GRIDMESH VBO LOAD FAILED");

    /* Position attribute */
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Util::checkGlError("GRIDMESH VAO LOAD FAILED");
}

void GridMesh::loadIndices()
{
    for (unsigned int i = 0; i < _sideLength - 1; i++) {
        for (unsigned int j = 0; j < _sideLength; j++) {
            _indices.push_back(j + _sideLength * i);
            _indices.push_back(j + _sideLength * (i + 1));
        }
        _indices.push_back(RESTART_INDEX);
    }

    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(unsigned), &_indices[0], GL_STATIC_DRAW);
}

void GridMesh::unload()
{
    /* TODO */
}
