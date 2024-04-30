#version 460 core
in vec3 aPos;
in vec4 aColor;
in vec2 aTexCoord;
out vec4 vertexColor;
out vec2 texCoord;
uniform mat4 modelview;
uniform mat4 projection;
void main()
{
	gl_Position = projection * modelview  * vec4(aPos, 1.0);
	vertexColor = aColor;
	texCoord = aTexCoord;
}