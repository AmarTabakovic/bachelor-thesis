#version 400 core
layout (location = 0) in vec2 aPos;

out vec3 FragPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec2 offset;
uniform sampler2D heightmapTexture;
uniform float textureWidth;
uniform float textureHeight;
uniform float tileWidth;
uniform float zoom;
uniform vec2 tileKey;
uniform vec3 globeRadiusSquared;

uniform float globeRadiusY;
uniform float poleRadius;
uniform float heightDelta;
uniform float isNorthPole;

precision highp float;

void main()
{
    float y = globeRadiusY - heightDelta;
    vec2 newPos = aPos;
    if (isNorthPole <= 0.5) {
        newPos =  newPos * -1;
        y *= -1;
    }

    vec3 pos = vec3(newPos.x * (poleRadius + 0.2), y, newPos.y * (poleRadius + 0.2));

    gl_Position = projection * view * model * vec4(pos, 1.0);
    FragPosition = pos;
}

