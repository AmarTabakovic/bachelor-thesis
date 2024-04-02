#version 330 core
layout (location = 0) in vec3 aPos;

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
uniform vec3 worldSpaceCenterPos;
uniform float textureWidth;
uniform float textureHeight;
uniform float tileWidth;
uniform float zoom;
uniform vec2 tileKey;

vec3 geodeticSurfaceNormal(vec3 geodetic);
vec3 geodeticToCartesian(vec3 globeRadiiSquared, vec3 geodetic);

void main()
{

    float tw = tileWidth;

    tw -=1;

    vec2 aPos1 = vec2((aPos.x + 0.5f * tw) / tw,
                        (aPos.y + 0.5f * tw) / tw);

    float globalX = (tileKey.x + aPos1.x) / float(1 << int(zoom));
    float globalZ = (tileKey.y + aPos1.y) / float(1 << int(zoom));

    float pi = 3.141592;

    vec3 height = texture(heightmapTexture, aPos1).rgb;

    /* Maptiler Terrain RGB decoding formula:
     *
     *       elevation = -10000 + ((R * 256 * 256 + G * 256 + B) * 0.1)
     */
    float y = /*-10000 +*/ ((height.r * 256.0f * 256.0f + height.g * 256.0f + height.b) * 0.1f) / 30.0f / 4.0f;

    /* Check if current vertex is the "duplicate" vertex for constructing
     * the skirt */
    if (int(aPos.z) % 2 == 1) y -= 2.5f;

    float lon = (globalX * 360.0f - 180.0f) * -1;
    float lat = atan(exp(pi * (1.0f - 2.0f * globalZ))) * 2.0f - pi / 2.0f;
    lat = lat * 180.0f / pi;

    float latRad = radians(lat);
    float lonRad = radians(lon);

    vec3 spherePos = geodeticToCartesian(vec3(1000,1000,1000), vec3(lonRad, y, latRad));

    mercXZ = aPos1;
    inNormal = geodeticSurfaceNormal(vec3(lonRad, y, latRad));

    gl_Position = projection * view * model * vec4(spherePos, 1.0);

    FragPos2 = vec3(0,0,0);
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
