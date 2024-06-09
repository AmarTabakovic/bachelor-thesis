#include "aabbmesh.h"

#include "util.h"
#include <iostream>

AABBMesh::AABBMesh()

{

}

void AABBMesh::load()
{
    loadVertices();
}

void AABBMesh::render()
{
    glDisable(GL_CULL_FACE);
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}

void AABBMesh::unload()
{
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
}

void AABBMesh::loadVertices()
{

    /* Vertices are constant and small enough, so we can simply hardcode
     * the vertices here. */
    float vertices[] = {
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f
    };

    glGenVertexArrays(1, &_vao);
    Util::checkGlError("fail -2");
    Util::checkGlError("fail -1");

    glBindVertexArray(_vao);
    glGenBuffers(1, &_vbo);
    Util::checkGlError("fail 0");
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    Util::checkGlError("fail14");
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
    Util::checkGlError("fail 2");
    glEnableVertexAttribArray(0);
    Util::checkGlError("fail 3");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    Util::checkGlError("fail 4");
    Util::checkGlError("fail 5");
}

void AABBMesh::loadIndices()
{
}
