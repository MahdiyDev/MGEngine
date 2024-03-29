#include "mge.h"
#include "mge_gl.h"

void Draw_Line(Vector2 startPos, Vector2 endPos, Color color)
{
	MgeGL_Begin(MGEGL_LINES);
		MgeGL_Color4ub(color.r, color.g, color.b, color.a);
        MgeGL_Vertex2f(startPos.x, startPos.y);
        MgeGL_Vertex2f(endPos.x, endPos.y);
	MgeGL_End();
	// glm::vec3 lineColor = glm::vec3(color.r, color.g, color.b);
	// glm::mat4 model = glm::mat4(1.0f);
	// glm::mat4 view = glm::mat4(1.0f);
	// glm::mat4 projection = glm::mat4(1.0f);

	// glData.vertices = {
	// 	startPos.x, startPos.y, startPos.z,
	// 	endPos.x, endPos.y, endPos.z,
	// };
	
	// glData.lineShader->Set_Position_Buffer(glData.vertices);

	// glData.lineShader->Use();
	// glData.lineShader->Set_Mat4("model", model);
	// glData.lineShader->Set_Mat4("view", view);
	// glData.lineShader->Set_Mat4("projection", projection);
	// glData.lineShader->Set_Vec3("color", lineColor);
	// glData.lineShader->DrawArrays();
}
