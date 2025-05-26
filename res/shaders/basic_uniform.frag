#version 460 core

out vec4 FragColor;
uniform vec4 uniform_Color;

void main()
{
    FragColor = uniform_Color;
}
