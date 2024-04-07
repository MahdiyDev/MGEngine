#version 460 core
out vec4 finalColor;
in vec4 fragColor;

void main()
{
   finalColor = fragColor;
}