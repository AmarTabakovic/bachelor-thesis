#version 400 core

in vec3 FragPosition;

out vec4 FragColor;
uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0f);
}



