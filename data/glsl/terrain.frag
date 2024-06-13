#version 400 core

in vec3 FragPosition;
in vec3 FragPos2;

//in vec2 lonlat;
//in vec2 mercXZ;
//in vec2 globalNormalizedXZx
in vec2 inTexCoords;
in vec3 inNormal;

out vec4 FragColor;

uniform vec3 lightDirection;
uniform vec3 cameraPos;

uniform vec3 skyColor;
uniform vec3 terrainColor;
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

float calculateFog(float density);
vec3 calculateAmbient(vec3 lightColor, float strength);

//vec3 calculateDiffuse(vec3 lightColor);
//vec3 rotateVector(vec3 a, vec3 b);

void main()
{
    vec3 color;
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

   if (useWire < 0.5f) {
       color = texture(overlayTexture, inTexCoords).rgb;

       vec3 ambient = calculateAmbient(lightColor, 0.1f);

       if (doFog > 0.5) {
           vec3 fogColour = vec3(97, 154, 232) / 255.0f;
           float fogFactor = calculateFog(fogDensity);
           color = mix(fogColour, color, fogFactor);
       }

    } else {
        color = terrainColor.rgb;
    }
    FragColor = vec4(color, 1.0f);
}

vec3 calculateAmbient(vec3 lightColor, float strength) {
    return strength * lightColor;
}

//vec3 calculateDiffuse(vec3 lightColor) {
//    float tw = tileWidth;
//    if (zoom != 0) tw -= 1;
//
//    vec2 texPos = mercXZ;//vec2((FragPosition.x + 0.5 * tw)/ (tw),
//                    //   (FragPosition.z + 0.5 * tw)/ (tw));
//
//    /* Based on
//     * https://www.slideshare.net/repii/terrain-rendering-in-frostbite-using-procedural-shader-splatting-presentation?type=powerpoint */
//    vec3 leftHeight = texture(heightmapTexture, texPos - vec2(1.0 / tw, 0)).rgb;
//    vec3 rightHeight = texture(heightmapTexture, texPos + vec2(1.0 / tw, 0)).rgb;
//    vec3 upHeight = texture(heightmapTexture, texPos + vec2(0, 1.0 / tw)).rgb;
//    vec3 downHeight = texture(heightmapTexture, texPos - vec2(0, 1.0 / tw)).rgb;
//
//    float leftHeightF = calculateHeight(leftHeight);
//    float rightHeightF = calculateHeight(rightHeight);
//    float upHeightF = calculateHeight(upHeight);
//    float downHeightF = calculateHeight(downHeight);
//
//    float dx = (leftHeightF - rightHeightF);
//    float dz = (downHeightF - upHeightF);
//
//    vec3 normal = normalize(vec3(dx, 2.0f, dz));
//    normal = rotateVector(normal, inNormal);
//
//    vec3 lightDir = normalize(-lightDirection);
//
//    float diff = max(dot(normal, lightDir), 0.0f);
//    return diff * lightColor;
//}

/* The distance fog concept is based on the following resource:
 * https://opengl-notes.readthedocs.io/en/latest/topics/texturing/aliasing.html */
float calculateFog(float density) {
    float start = 0.8;
    float end = 3.5;
    float dist = length(cameraPos - FragPos2);
    if (dist < start) return 1.0f;
    float scale = (dist - start) / (end - start);
    float fogFactor = pow(scale, 1);
    return 1.0f - clamp(fogFactor, 0.0f, 0.5f);
}

// vec3 rotateVector(vec3 a, vec3 b) {
//    vec3 up = vec3(0.0, 1.0, 0.0);
//    vec3 axis = cross(b, up);
//    float cosTheta = dot(b, up);
//    float angle = acos(cosTheta);
//
//    /* Build the skew-symmetric matrix for the axis */
//    mat3 K = mat3(0, -axis.z, axis.y,
//                  axis.z, 0, -axis.x,
//                  -axis.y, axis.x, 0);
//
//    /* Rodrigues' rotation formula */
//    mat3 I = mat3(1.0);
//    mat3 R = I + sin(angle) * K + (1.0 - cos(angle)) * K * K;
//
//    return R * a;
//}
