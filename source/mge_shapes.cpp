#include "mge.h"
#include "Shader.h"
#include "mge_gl.h"
#include <cstdio>

extern MgeGL_Data glData;

void Draw_Line(Vector3 startPos, Vector3 endPos, Color color)
{
	glm::vec3 lineColor = glm::vec3(color.r, color.g, color.b);
	glm::mat4 MVP = glm::mat4(1.0f);

	glData.vertices = {
		startPos.x, startPos.y, startPos.z,
		endPos.x, endPos.y, endPos.z,
	};
	
	glData.lineShader->Set_Vertices(glData.vertices, 3, 3);

	glData.lineShader->Use();
	glData.lineShader->SetMat4("MVP", MVP);
	glData.lineShader->SetVec3("color", lineColor);
	glData.lineShader->DrawArrays();
}
