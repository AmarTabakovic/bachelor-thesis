#version 400 core
layout (location = 0) in vec3 aPos;

out vec3 FragPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 size;
uniform vec3 center;

precision highp float;

void main()
{
    vec3 pos = aPos / 2.0f;
    gl_Position = projection * view * model * vec4(pos, 1.0);
    FragPosition = aPos;
}
