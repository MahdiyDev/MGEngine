#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"
#include "mge.h"
#include "mge_math.h"

int main(int argc, char** argv)
{
	int WIDTH = 800, HEIGHT = 600;
    Init_Window(WIDTH, HEIGHT, "Hello this is test");

	// Load_Shader(Vector3{0,0,0}, Vector3{100,100,0}, RED);
	Line line_x(glm::vec3{0,0,0}, glm::vec3{0, 0.5, 0});
	Line line_y(glm::vec3{0,0,0}, glm::vec3{0.5, 0, 0});
	Line line_z(glm::vec3{0,0,0}, glm::vec3{0, 0, 0.5});
	line_x.setColor(glm::vec3 {RED.r, RED.g, RED.b});
	line_y.setColor(glm::vec3 {GREEN.r, GREEN.g, GREEN.b});
	line_z.setColor(glm::vec3 {BLUE.r, BLUE.g, BLUE.b});
    while (!Window_Should_Close()) {
        Begin_Drawing();
        	Clear_Background(GRAY);
			line_x.draw();
			line_y.draw();
			line_z.draw();
			glm::mat4 projection = glm::mat4(1.0f);
			projection = glm::perspective(
				glm::radians(45.0f),
				(float)WIDTH / (float)HEIGHT,
				0.1f, 100.0f);
			glm::mat4 view = glm::mat4(1.0f);
			view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, Get_Time() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
			line_x.setMVP(projection * view * model);
			line_y.setMVP(projection * view * model);
			line_z.setMVP(projection * view * model);
			// Draw_Line();
        End_Drawing();
    }

    Close_Window();
    return 0;
}
