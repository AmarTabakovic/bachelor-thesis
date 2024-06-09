#version 400 core
layout (location = 0) in vec2 aPos;

out vec3 FragPosition;
out vec3 FragPos2;

out vec2 lonlat;
out vec2 mercXZ;
out vec2 globalNormalizedXZ;
out vec3 inNormal;

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

vec3 geodeticSurfaceNormal(vec3 geodetic);
vec3 geodeticToCartesian(vec3 globeRadiiSquared, vec3 geodetic);

precision highp float;

void main()
{

    float tw = tileWidth;

    tw -=1;

    /* Normalized positions [0,1] */
    vec2 aPos1 = vec2((aPos.x + 0.5f * tw) / tw,
                        (aPos.y + 0.5f * tw) / tw);

    float globalX = (tileKey.x + aPos1.x) / float(1 << int(zoom));
    float globalZ = (tileKey.y + aPos1.y) / float(1 << int(zoom));

    float pi = 3.1415926538;

    vec3 height = texture(heightmapTexture, aPos1).rgb;

    /* Maptiler Terrain RGB decoding formula:
     *
     *       elevation = -10000 + ((R * 256 * 256 + G * 256 + B) * 0.1)
     *
     */
    float y = (height.r * 256 * 256 + height.g * 256 + height.b);
    float scaleFactor = 1.0f / 600.0f;
    y = y * scaleFactor;

    height = height * 255;

    y = -10000 + (((height.r * 256.0f * 256.0f * 0.1) + (height.g * 256.0f * 0.1) + (height.b * 0.1)));

    y = (y / 20169.51);

    float lon = (globalX * 360.0f - 180.0f) * -1;
    float lat = atan(exp(pi * (1.0f - 2.0f * globalZ))) * 2.0f - pi / 2.0f;
    lat = lat * 180.0f / pi;

    float latRad = radians(lat);
    float lonRad = radians(lon);

    vec3 spherePos = geodeticToCartesian(globeRadiusSquared, vec3(lonRad, y, latRad));

    mercXZ = aPos1;
    inNormal = geodeticSurfaceNormal(vec3(lonRad, 0, latRad));

    gl_Position = projection * view * model * vec4(spherePos, 1.0);

    FragPos2 = spherePos;
    FragPosition = FragPos2;

}

vec3 geodeticSurfaceNormal(vec3 geodetic) {
    float cosLat = cos(geodetic.z);

    return vec3(cosLat * cos(geodetic.x), sin(geodetic.z), cosLat * sin(geodetic.x));
}

vec3 geodeticToCartesian(vec3 globeRadiiSquared, vec3 geodetic) {
    vec3 n = geodeticSurfaceNormal(geodetic);
    vec3 k = globeRadiiSquared * n;
    float gamma = sqrt(k.x * n.x + k.y * n.y + k.z * n.z);

    vec3 rSurface = k / gamma;
    return rSurface + (geodetic.y * n);
}
