#include "polemesh.h"

#include "glm/ext/scalar_constants.hpp"
#include "util.h"
#include <glm/glm.hpp>
#include <iostream>

PoleMesh::PoleMesh(unsigned numRadians)
    : _numRadians(numRadians)
{
}

void PoleMesh::load()
{
    loadVertices();
    loadIndices();
}
void PoleMesh::render()
{
    Util::checkGlError("hmm");
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);
    Util::checkGlError("PRIMITIVE RESTART");
    glBindVertexArray(_vao);
    Util::checkGlError("VAO");
    glDrawElements(GL_TRIANGLE_STRIP, _indices.size() * sizeof(unsigned int), GL_UNSIGNED_INT, (void*)0);
    Util::checkGlError("DRAW FAILED");
    glDisable(GL_PRIMITIVE_RESTART);
}

void PoleMesh::unload()
{
}

void PoleMesh::loadVertices()
{
    /* Center of the pole */
    _vertices.push_back(0);
    _vertices.push_back(0);

    for (unsigned i = 0; i < _numRadians; i++) {
        float currentAngle = glm::radians(i * (360.0f / _numRadians));
        float x = glm::cos(currentAngle);
        float z = glm::sin(currentAngle);
        _vertices.push_back(x);
        _vertices.push_back(z);
    }

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    Util::checkGlError("POLEMESH VAO GEN FAILED");

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), &_vertices[0], GL_STATIC_DRAW);

    Util::checkGlError("POLEMESH VBO LOAD FAILED");

    /* Position attribute */
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Util::checkGlError("POLEMESH VAO LOAD FAILED");
}

void PoleMesh::loadIndices()
{
    for (unsigned i = 0; i < _numRadians; i++) {
        _indices.push_back(i + 1);
        _indices.push_back(0);
    }
    _indices.push_back(1);

    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(unsigned), &_indices[0], GL_STATIC_DRAW);
    Util::checkGlError("POLEMESH LOAD INDICES FAILED");
}
