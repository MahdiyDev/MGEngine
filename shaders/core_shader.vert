#version 460 core
in vec3 vertexPos;
in vec4 vertexColor;

out vec4 fragColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	fragColor = vertexColor;
   gl_Position = projection * view * model * vec4(vertexPos, 1.0);
}