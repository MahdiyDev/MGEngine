#include "mge_gl.h"
#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>
#include <fstream>
#include <math.h>
#include <sstream>

#define MAX_ATTRIB_LOCATION		3
#define MAX_BUFFER_ELEMENTS		256

typedef enum {
	VERTICE_LOCATION = 0,
	COLOR_LOCATION,
	TEXTURE_LOCATION,
} AttribLocations;

typedef enum {
	MGEGL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 = 0,
} MgeGL_PixelFormat;

typedef struct VertexData {
	float* vertices;
	unsigned char* colors;
	float* texcoords;
	unsigned int* indices;
} VertexData;

typedef struct GlData {
	struct {
		int vertexCounter;
		unsigned int VBO[4];
		unsigned int VAO;
		unsigned int defaultTexture;
		unsigned int programID;
		unsigned int AttribLoc[MAX_ATTRIB_LOCATION];
		unsigned char colorr, colorg, colorb, colora;
		float texcoordx, texcoordy;

		VertexData vertexBuffer;
		float currentDepth;

		int framebufferWidth;
		int framebufferHeight;
	} State;
} GlData;

const char* vertexShaderCode = 
	"#version 460 core\n"
	"in vec3 aPos;\n"
	"in vec4 aColor;\n"
	"in vec2 aTexCoord;\n"
	"out vec4 vertexColor;\n"
	"out vec2 texCoord;\n"
	"uniform mat4 model;\n"
	"uniform mat4 view;\n"
	"uniform mat4 projection;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
	"	vertexColor = aColor;\n"
	"	texCoord = aTexCoord;\n"
	"}\n\0";

const char* fragmentShaderCode = 
	"#version 460 core\n"
	"out vec4 FragColor;\n"
	"in vec4 vertexColor;\n"
	"in vec2 texCoord;\n"
	"uniform sampler2D sampleTex;\n"
	"void main()\n"
	"{\n"
	"	FragColor = texture(sampleTex, texCoord) * vertexColor;\n"
	"}\n\0";

// float texCoords[] = {
//	0.0f, 0.0f,  // lower-left corner  
//	1.0f, 0.0f,  // lower-right corner
//	0.5f, 1.0f   // top-center corner
// };

// unsigned int indices[] = {
// 	0, 1, 3, // first triangle
// 	1, 2, 3  // second triangle
// };

GlData MGEGL = { 0 };

void MgeGL_Init(int width, int height)
{
	// Zeroing data
	MGEGL.State.framebufferWidth = width;
	MGEGL.State.framebufferHeight = height;
	MGEGL.State.vertexCounter = 0;
	MGEGL.State.currentDepth = -1.0f;
	MGEGL.State.vertexBuffer.vertices = (float*)malloc(MAX_BUFFER_ELEMENTS*3*sizeof(float));
	MGEGL.State.vertexBuffer.colors = (unsigned char*)malloc(MAX_BUFFER_ELEMENTS*4*sizeof(unsigned char));
	MGEGL.State.vertexBuffer.texcoords = (float*)malloc(MAX_BUFFER_ELEMENTS*2*sizeof(float));
	MGEGL.State.vertexBuffer.indices = (unsigned int*)malloc(MAX_BUFFER_ELEMENTS*6*sizeof(unsigned int));

	for (int i = 0; i < MAX_BUFFER_ELEMENTS*3; i++)MGEGL.State.vertexBuffer.vertices[i] = 0.0f;
	for (int i = 0; i < MAX_BUFFER_ELEMENTS*4; i++)MGEGL.State.vertexBuffer.colors[i] = 0;
	for (int i = 0; i < MAX_BUFFER_ELEMENTS*2; i++)MGEGL.State.vertexBuffer.texcoords[i] = 0.0f;

	int k = 0;

	for (int j = 0; j < (6*MAX_BUFFER_ELEMENTS); j += 6)
	{
		MGEGL.State.vertexBuffer.indices[j + 0] = 4*k + 0;
		MGEGL.State.vertexBuffer.indices[j + 1] = 4*k + 1;
		MGEGL.State.vertexBuffer.indices[j + 2] = 4*k + 2;
		MGEGL.State.vertexBuffer.indices[j + 3] = 4*k + 0;
		MGEGL.State.vertexBuffer.indices[j + 4] = 4*k + 2;
		MGEGL.State.vertexBuffer.indices[j + 5] = 4*k + 3;

		k++;
	}

	// unsigned char pixels[4] = { 255, 255, 255, 255 };
	int texWidth, texHeight;
	unsigned char* pixels = stbi_load("assests/wall.jpg", &texWidth, &texHeight, 0, 0);
	MGEGL.State.defaultTexture = MgeGL_LoadTexture(pixels, texWidth, texHeight, MGEGL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
	stbi_image_free(pixels);

	// Init Shader
	unsigned int vertex = MgeGL_LoadShader(vertexShaderCode, GL_VERTEX_SHADER, "vertex");
	unsigned int fragment = MgeGL_LoadShader(fragmentShaderCode, GL_FRAGMENT_SHADER, "fragment");
	MGEGL.State.programID = MgeGL_CreateShaderProgram(vertex, fragment);

	MGEGL.State.AttribLoc[VERTICE_LOCATION] = MgeGL_GetAttribLoc("aPos");
	MGEGL.State.AttribLoc[COLOR_LOCATION] = MgeGL_GetAttribLoc("aColor");
	MGEGL.State.AttribLoc[TEXTURE_LOCATION] = MgeGL_GetAttribLoc("aTexCoord");

	glGenVertexArrays(1, &MGEGL.State.VAO);
	glBindVertexArray(MGEGL.State.VAO);

	// unsigned int EBO;
	// glGenBuffers(1, &EBO);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	// glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// positions
	glGenBuffers(1, &MGEGL.State.VBO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, MAX_BUFFER_ELEMENTS*3*sizeof(float), MGEGL.State.vertexBuffer.vertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(MGEGL.State.AttribLoc[VERTICE_LOCATION], 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(MGEGL.State.AttribLoc[VERTICE_LOCATION]);
	// colors
	glGenBuffers(1, &MGEGL.State.VBO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, MAX_BUFFER_ELEMENTS*4*sizeof(unsigned char), MGEGL.State.vertexBuffer.colors, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(MGEGL.State.AttribLoc[COLOR_LOCATION], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
	glEnableVertexAttribArray(MGEGL.State.AttribLoc[COLOR_LOCATION]);
	// textures
	glGenBuffers(1, &MGEGL.State.VBO[2]);
	glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[2]);
	glBufferData(GL_ARRAY_BUFFER, MAX_BUFFER_ELEMENTS*2*sizeof(float), MGEGL.State.vertexBuffer.texcoords, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(MGEGL.State.AttribLoc[TEXTURE_LOCATION], 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(MGEGL.State.AttribLoc[TEXTURE_LOCATION]);

	glGenBuffers(1, &MGEGL.State.VBO[3]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MGEGL.State.VBO[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_BUFFER_ELEMENTS*6*sizeof(unsigned int), MGEGL.State.vertexBuffer.indices, GL_STATIC_DRAW);

	glUseProgram(MGEGL.State.programID);
}

int MgeGL_LoadTexture(const void *data, int width, int height, int format, int mipmapCount)
{
	unsigned int id;

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (data != NULL)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		TRACE_LOG(LOG_INFO, "could not load texture");
	}

	// MgeGL_Uniform1i(MGEGL.State.programID, "sampleTex", 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}

void MgeGL_Close()
{
	glDisableVertexAttribArray(MGEGL.State.AttribLoc[VERTICE_LOCATION]);
	glDisableVertexAttribArray(MGEGL.State.AttribLoc[COLOR_LOCATION]);
	glDisableVertexAttribArray(MGEGL.State.AttribLoc[TEXTURE_LOCATION]);
	glDeleteBuffers(1, &MGEGL.State.VBO[0]);
	glDeleteBuffers(1, &MGEGL.State.VBO[1]);
	glDeleteBuffers(1, &MGEGL.State.VBO[2]);
	glDeleteBuffers(1, &MGEGL.State.VBO[3]);
	glDeleteVertexArrays(1, &MGEGL.State.VAO);
	glDeleteProgram(MGEGL.State.programID);

	free(MGEGL.State.vertexBuffer.vertices);
	free(MGEGL.State.vertexBuffer.colors);
	free(MGEGL.State.vertexBuffer.texcoords);
	free(MGEGL.State.vertexBuffer.indices);
}

void MgeGL_Begin(int mode)
{
	MgeGL_Draw(mode);
	MGEGL.State.vertexCounter = 0;
	MGEGL.State.currentDepth = -1.0f;
}

void MgeGL_End()
{
	MGEGL.State.currentDepth += (1.0f/20000.0f);
}


Matrix model = Matrix_Identity();
void MgeGL_Draw(int mode)
{
	model = Matrix_Rotate(Vector3 {1.0f, 1.0f, 0.0f}, -55.0f*Mge_GetTime()*DEG2RAD);
	Matrix view = Matrix_Identity();
	view = Matrix_Multiply(view, Matrix_Translate(0.0f, 0.0f, -6.0f));
	Matrix projection = MatrixPerspective(
		45.0f*DEG2RAD,
		(float)MGEGL.State.framebufferWidth / (float)MGEGL.State.framebufferHeight,
		0.1f, 100.0f
	);
	// Matrix projection = MatrixOrtho(0.0f, 800.0f, 600.0f, 0.0f, 0.0f, 1.0f);

	// projection = Matrix_Multiply(projection, MatrixOrtho(0.0f, 800.0f, 0.0f, 600.0f, 0.1f, 100.0f));

	MgeGL_UniformMatrix4fv(MGEGL.State.programID, "model", model);
	MgeGL_UniformMatrix4fv(MGEGL.State.programID, "view", view);
	MgeGL_UniformMatrix4fv(MGEGL.State.programID, "projection", projection);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, MGEGL.State.defaultTexture);

	glUseProgram(MGEGL.State.programID);
	glBindVertexArray(MGEGL.State.VAO);

	// positions
	glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter*3*sizeof(float), MGEGL.State.vertexBuffer.vertices);
	// colors
	glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[1]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter*4*sizeof(unsigned char), MGEGL.State.vertexBuffer.colors);
	// textures
	glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[2]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter*2*sizeof(float), MGEGL.State.vertexBuffer.texcoords);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MGEGL.State.VBO[3]);

	if (mode == MGEGL_LINES || mode == MGEGL_TRIANGLES)
	{
		glDrawArrays(mode, 0, MGEGL.State.vertexCounter);
	}
	else
	{
		glDrawElements(GL_TRIANGLES, MGEGL.State.vertexCounter/4*6, GL_UNSIGNED_INT, 0);
	}
	glBindVertexArray(0);
}

void MgeGL_Color4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w)
{
	MGEGL.State.colorr = x;
	MGEGL.State.colorg = y;
	MGEGL.State.colorb = z;
	MGEGL.State.colora = w;
}

void MgeGL_TexCoord2f(float x, float y)
{
	MGEGL.State.texcoordx = x;
	MGEGL.State.texcoordy = y;
}

void MgeGL_Vertex2i(int x, int y)
{
	MgeGL_Vertex3f((float)x, (float)y, MGEGL.State.currentDepth);
}

void MgeGL_Vertex2f(float x, float y)
{
	MgeGL_Vertex3f(x, y, MGEGL.State.currentDepth);
}

void MgeGL_Vertex3f(float x, float y, float z)
{
	// Adding position
	MGEGL.State.vertexBuffer.vertices[3*MGEGL.State.vertexCounter+0] = x;
	MGEGL.State.vertexBuffer.vertices[3*MGEGL.State.vertexCounter+1] = y;
	MGEGL.State.vertexBuffer.vertices[3*MGEGL.State.vertexCounter+2] = z;
	// Adding color
	MGEGL.State.vertexBuffer.colors[4*MGEGL.State.vertexCounter+0] = MGEGL.State.colorr;
	MGEGL.State.vertexBuffer.colors[4*MGEGL.State.vertexCounter+1] = MGEGL.State.colorg;
	MGEGL.State.vertexBuffer.colors[4*MGEGL.State.vertexCounter+2] = MGEGL.State.colorb;
	MGEGL.State.vertexBuffer.colors[4*MGEGL.State.vertexCounter+3] = MGEGL.State.colora;
	// Adding texture
	MGEGL.State.vertexBuffer.texcoords[2*MGEGL.State.vertexCounter+0] = MGEGL.State.texcoordx;
	MGEGL.State.vertexBuffer.texcoords[2*MGEGL.State.vertexCounter+1] = MGEGL.State.texcoordy;

	MGEGL.State.vertexCounter++;
}

int MgeGL_GetAttribLoc(const char* name)
{
	return glGetAttribLocation(MGEGL.State.programID, name);
}

void MgeGL_Uniform1i(int programID, const char* name, const int value)
{
	glUniform1i(glGetUniformLocation(programID, name), value);
}

void MgeGL_Uniform3fv(int programID, const char* name, const Vector3& value)
{
	float vectorValue[3] = {
		value.x, value.y, value.z
	};
	glUniform3fv(glGetUniformLocation(programID, name), 1, vectorValue);
}

void MgeGL_Uniform4fv(int programID, const char* name, const Vector4& value)
{
	float vectorValue[4] = {
		value.x, value.y, value.z, value.w
	};
	glUniform4fv(glGetUniformLocation(programID, name), 1, vectorValue);
}

void MgeGL_UniformMatrix4fv(int programID, const char* name, const Matrix& value)
{
	float matValue[16] = {
		value.m0,  value.m1,  value.m2,  value.m3,
		value.m4,  value.m5,  value.m6,  value.m7,
		value.m8,  value.m9,  value.m10, value.m11,
		value.m12, value.m13, value.m14, value.m15
	};
	glUniformMatrix4fv(glGetUniformLocation(programID, name), 1, GL_FALSE, matValue);
}

std::string MgeGL_ReadShaderFromFile(const char* file_path)
{
	std::string line;
	std::stringstream streamCode;
	std::ifstream myfile(file_path);

	if (myfile.is_open()) {
		TRACE_LOG(LOG_INFO, "Shader: open file: %s", file_path);

		while (getline(myfile, line)) {
			streamCode << line << "\n";
		}
		myfile.close();
	} else {
		TRACE_LOG(LOG_ERROR, "Shader: Unable to open file: %s", file_path);
	}

	return streamCode.str();
}

unsigned int MgeGL_LoadShader(const char* code, unsigned int shaderType, std::string typeName)
{
	unsigned int shader;
	int success;
	char infoLog[512];

	shader = glCreateShader(shaderType);
	TRACE_LOG(LOG_INFO, "Shader: %s shader created", typeName.c_str());
	if (shader == 0) {
		TRACE_LOG(LOG_ERROR, "Shader: Unable to create shader: %s", typeName.c_str());
		return EXIT_FAILURE;
	}
	glShaderSource(shader, 1, &code, NULL);
	glCompileShader(shader);
	TRACE_LOG(LOG_INFO, "Shader: %s shader compiled", typeName.c_str());

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		TRACE_LOG(LOG_ERROR, "Shader: %s shader not compiled Error: %s",
			typeName.c_str(), infoLog);
	}
	return shader;
}

unsigned int MgeGL_CreateShaderProgram(unsigned int vertex, unsigned int fragment)
{
	int success;
	char infoLog[512];
	unsigned int programID;

	programID = glCreateProgram();
	glAttachShader(programID, vertex);
	glAttachShader(programID, fragment);
	glLinkProgram(programID);

	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(programID, 512, NULL, infoLog);
		TRACE_LOG(LOG_ERROR, "Shader: program not created\n%s", infoLog);
	}
	TRACE_LOG(LOG_INFO, "Shader: program created");

	// Deleting shaders
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	return programID;
}

void MgeGL_ClearColor(Color color)
{
	float cr = (float)color.r / 255;
	float cg = (float)color.g / 255;
	float cb = (float)color.b / 255;
	float ca = (float)color.a / 255;
	glClearColor(cr, cg, cb, ca);
}

void MgeGL_ClearScreenBuffers(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MgeGL_Viewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

void MgeGL_Load_Extensions(void* loader)
{
	if (gladLoadGLLoader((GLADloadproc)loader) == 0) {
		TRACE_LOG(LOG_WARNING, "GLAD: Cannot load OpenGL extensions");
	} else {
		TRACE_LOG(LOG_INFO, "GLAD: OpenGL extensions loaded successfully");
	}

	TRACE_LOG(LOG_INFO, "GL: OpenGL device information:");
	TRACE_LOG(LOG_INFO, "	> Vendor:   %s", glGetString(GL_VENDOR));
	TRACE_LOG(LOG_INFO, "	> Renderer: %s", glGetString(GL_RENDERER));
	TRACE_LOG(LOG_INFO, "	> Version:  %s", glGetString(GL_VERSION));
	TRACE_LOG(LOG_INFO, "	> GLSL:	 %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

	MgeGL_EnableDepthTest();
}

void MgeGL_EnableDepthTest(void) { glEnable(GL_DEPTH_TEST); }
