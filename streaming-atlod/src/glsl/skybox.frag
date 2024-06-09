#version 400 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform vec3 cameraPos;

void main()
{
    vec4 skyColor = vec4(0.5,0.75,0.9,0);//texture(skybox, TexCoords);
    //skyColor = vec4(1.0,1.0,1.0,1.0);
    skyColor = texture(skybox, TexCoords);
    vec4 black = vec4(0,0,0,1.0);
    vec4 orange = vec4(0.5,0.2,0.2,1.0);

    float minDist = 320;
    float maxDist = 360;
    float distToCenter = sqrt(dot(cameraPos, cameraPos));
    float clampedDist = clamp(distToCenter, minDist, maxDist);
    float normalized = (clampedDist - minDist) / ((maxDist - minDist));
    vec4 color = mix(skyColor, black, normalized);
    //color = skyColor;
    color = vec4(1.0f,1.0f,1.0f,1.0f);
    FragColor = color;
}
