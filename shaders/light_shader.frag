#version 460 core
out vec4 FragColor;
in vec4 vertexColor;
in vec2 texCoord;
uniform vec4 lightColor;
uniform sampler2D sampleTex;
void main()
{
	FragColor = texture(sampleTex, texCoord) * vertexColor * lightColor;
}