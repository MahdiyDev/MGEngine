#include "glm/ext/matrix_float4x4.hpp"
#include "mge.h"
#include "Shader.h"

Shader* lineShader;

glm::vec3 lineColor;
glm::mat4 MVP;

void Load_Shader(Vector3 startPos, Vector3 endPos, Color color)
{
	lineShader = new Shader(
		"shaders/line_shader.vert",
		"shaders/line_shader.frag"
	);
	lineColor = glm::vec3(color.r, color.g, color.b);
	MVP = glm::mat4(1.0f);

	float vertices[] = {
		startPos.x,	startPos.y,	startPos.z,
		endPos.x,	endPos.y,	endPos.z,
	};

	lineShader->Set_Vertices(vertices, 3, 3);
}

void Draw_Line(void)
{
	lineShader->Use();
	lineShader->SetValue("MVP", MVP);
	lineShader->SetValue("color", lineColor);
	lineShader->DrawArrays();
	// lineShader->CleanUp();
}
