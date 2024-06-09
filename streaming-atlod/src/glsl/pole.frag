#version 400 core

in vec3 FragPosition;

out vec4 FragColor;

uniform vec3 lightDirection;
uniform vec3 cameraPos;

uniform vec3 skyColor;
uniform vec3 terrainColor;
//uniform float doTexture;

uniform float doFog;
uniform float fogDensity;

uniform float useWire;
uniform vec3 inColor;

uniform sampler2D overlayTexture;
uniform sampler2D heightmapTexture;

uniform float textureWidth;
uniform float textureHeight;
uniform float tileWidth;

uniform float yScale;
uniform float zoom;

uniform vec2 tileKey;

uniform float globeRadiusY;
uniform float poleRadius;
uniform float heightDelta;
uniform float isNorthPole;


float calculateFog(float density);

void main()
{
    vec3 color = vec3(13,17,27) / 255;
    if (isNorthPole <= 0.5) {
        color = vec3(1.0,1.0,1.0);
    }
    if (doFog > 0.5) {
        vec3 fogColour = vec3(97, 154, 232) / 255.0f;
        float fogFactor = calculateFog(fogDensity);
        color = mix(fogColour, color, fogFactor);
    }
    FragColor = vec4(color, 1.0f);
}

float calculateFog(float density) {
    float start = 0.8;
    float end = 3.5;
    float dist = length(cameraPos - FragPosition);
    if (dist < start) return 1.0f;
    //float dist = length(FragPos2 - cameraPos);
    //float fogFactor = pow(exp(-density * dist), 8);
    float scale = (dist - start) / (end - start);
    float fogFactor = pow(scale, 1);
    return 1.0f - clamp(fogFactor, 0.0f, 0.5f);
}


