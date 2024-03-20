#include "mge.h"
#include "Shader.h"

Shader lineShader(
	"shaders/line_shader.vert",
	"shaders/line_shader.frag"
);

void Draw_Line(Vector3 startPos, Vector3 endPos, Color color)
{
	auto lineColor = glm::vec3(color.r, color.g, color.b);
	auto MVP = glm::mat4(1.0f);

	float vertices[] = {
		startPos.x,	startPos.y,	startPos.z,
		endPos.x,	endPos.y,	endPos.z,
	};

	lineShader.Set_Vertices(vertices, 3, 3);
	lineShader.SetValue("MVP", MVP);
	lineShader.SetValue("color", lineColor);
	lineShader.DrawArrays();
	lineShader.CleanUp();
}
