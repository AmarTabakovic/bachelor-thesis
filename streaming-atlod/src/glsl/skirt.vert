#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 FragPosition;
out vec3 FragPos2;

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

void main()
{
    float tw = tileWidth;

    if (zoom != 0) tw -=1;

    vec2 texPos = vec2((aPos.x + 0.5f * tw) / tw,
                        (aPos.y + 0.5f * tw) / tw);

    vec3 height = texture(heightmapTexture, texPos).rgb;

    /* Maptiler Terrain RGB decoding formula:
     *
     *       elevation = -10000 + ((R * 256 * 256 + G * 256 + B) * 0.1)
     */
    float y = /*-10000*/ + ((height.r * 256 * 256 + height.g * 256 + height.b) * 0.1) / 30 / 4;

    int iZoom = int(zoom);
    int pow2Zoom = 1 << iZoom;

    vec3 actualPos = (vec3(aPos.x, 0, aPos.y) * (1.0 / float(pow2Zoom)) + worldSpaceCenterPos);
    actualPos.y = y;

    /* Check if current vertex is the "duplicate" vertex for constructing
     * the skirt */
    if (int(aPos.z) % 2 == 1) actualPos.y -= 20;

    FragPosition = vec3(model * vec4((actualPos - worldSpaceCenterPos) * pow2Zoom, 1.0));
    FragPos2 = vec3(model * vec4(actualPos, 1.0));
    gl_Position = projection * view * model * vec4(actualPos, 1.0);
}
