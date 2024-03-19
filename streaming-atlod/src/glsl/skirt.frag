#version 330 core

in vec3 FragPosition;
in vec3 FragPos2;

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

uniform vec3 worldSpaceCenterPos;
uniform float textureWidth;
uniform float textureHeight;
uniform float tileWidth;

uniform float yScale;
uniform float zoom;

vec3 calculateAmbient(vec3 lightColor, float strength);
vec3 calculateDiffuse(vec3 lightColor);
float calculateFog(float density);
float calculateHeight(vec3 heightSample);

void main()
{
    vec3 color;
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

   if (useWire < 0.5f) {

       float tw = tileWidth;
       if (zoom != 0) tw -= 1;

       vec2 texPos = vec2((FragPosition.x + 0.5 * tw) / (tw),
                          (FragPosition.z + 0.5 * tw) / (tw));

       color = texture(overlayTexture, texPos).rgb;

       vec3 ambient = calculateAmbient(lightColor, 0.5f);
       vec3 diffuse = calculateDiffuse(lightColor);
       //vec3 diffuse = vec3(0,0,0);
       color = (ambient + diffuse) * color;

       /*if (doFog > 0.5) {
           vec3 fogColour = skyColor;
           float fogFactor = calculateFog(fogDensity);
           color = mix(fogColour, color, fogFactor);
       }*/

    } else {
        color = terrainColor.rgb;
    }
    FragColor = vec4(color, 1.0f);
}

vec3 calculateAmbient(vec3 lightColor, float strength) {
    return strength * lightColor;
}

vec3 calculateDiffuse(vec3 lightColor) {
    vec2 texPos = vec2((FragPosition.x + 0.5 * tileWidth)/ (tileWidth),
                       (FragPosition.z + 0.5 * tileWidth)/ (tileWidth));

    /* Based on
     * https://www.slideshare.net/repii/terrain-rendering-in-frostbite-using-procedural-shader-splatting-presentation?type=powerpoint */
    vec3 leftHeight = texture(heightmapTexture, texPos - vec2(1.0 / tileWidth, 0)).rgb;
    vec3 rightHeight = texture(heightmapTexture, texPos + vec2(1.0 / tileWidth, 0)).rgb;
    vec3 upHeight = texture(heightmapTexture, texPos + vec2(0, 1.0 / tileWidth)).rgb;
    vec3 downHeight = texture(heightmapTexture, texPos - vec2(0, 1.0 / tileWidth)).rgb;

    float leftHeightF = calculateHeight(leftHeight);
    float rightHeightF = calculateHeight(rightHeight);
    float upHeightF = calculateHeight(upHeight);
    float downHeightF = calculateHeight(downHeight);

    float dx = (leftHeightF - rightHeightF);
    float dz = (downHeightF - upHeightF);

    vec3 normal = normalize(vec3(dx, 2.0f, dz));

    vec3 lightDir = normalize(-lightDirection);

    float diff = max(dot(normal, lightDir), 0.0f);
    return diff * lightColor;
}

/* The distance fog concept is based on the following resource:
 * https://opengl-notes.readthedocs.io/en/latest/topics/texturing/aliasing.html */
float calculateFog(float density) {
    float dist = length(cameraPos - FragPos2);
    float fogFactor = exp(-density * dist);
    return clamp(fogFactor, 0.0f, 1.0f);
}

float calculateHeight(vec3 heightSample) {
    return /*-10000*/ + ((heightSample.r * 256 * 256 + heightSample.g * 256 + heightSample.b) * 0.1) / 30; // / 2;
}
