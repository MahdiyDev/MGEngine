#include "mge_gl.h"
#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <cstdio>
#include <glad/glad.h>
#include <imgui.h>
#include <math.h>

typedef struct MgeGL_DrawCall {
	int mode;
	int vertexAlignment;
	int vertexCount;
} MgeGL_DrawCall;

typedef struct GlData {
	struct {
		int vertexCounter;
		unsigned int VBO[4];
		unsigned int VAO;
		unsigned int defaultTexture;
		unsigned int defaultShaderID;
		unsigned int currentShaderID;
		unsigned int AttribLoc[MAX_ATTRIB_LOCATION];
		unsigned char colorr, colorg, colorb, colora;
		float texcoordx, texcoordy;

		Matrix modelview, projection, transform;
		Matrix* currentMatrix;
		int currentMatrixMode;
		Matrix stack[MGEGL_MAX_MATRIX_STACK_SIZE];
		int stackCounter;
		bool transformRequired;

		VertexData vertexBuffer;
		float currentDepth;

		int framebufferWidth;
		int framebufferHeight;

		// int mode; // removed soon
		MgeGL_DrawCall* draws;
		int drawCounter;
	} State;
} GlData;

const char* vertexShaderCode = "#version 460 core\n"
							   "in vec3 aPos;\n"
							   "in vec4 aColor;\n"
							   "in vec2 aTexCoord;\n"
							   "out vec4 vertexColor;\n"
							   "out vec2 texCoord;\n"
							   "uniform mat4 modelview;\n"
							   "uniform mat4 projection;\n"
							   "void main()\n"
							   "{\n"
							   "	gl_Position = projection * modelview  * vec4(aPos, 1.0);\n"
							   "	vertexColor = aColor;\n"
							   "	texCoord = aTexCoord;\n"
							   "}\n\0";

const char* fragmentShaderCode = "#version 460 core\n"
								 "out vec4 FragColor;\n"
								 "in vec4 vertexColor;\n"
								 "in vec2 texCoord;\n"
								 "uniform sampler2D sampleTex;\n"
								 "void main()\n"
								 "{\n"
								 "	FragColor = texture(sampleTex, texCoord) * vertexColor;\n"
								 "}\n\0";

GlData MGEGL = { 0 };

void MgeGL_Init(int width, int height)
{
	// Zeroing data
	MGEGL.State.framebufferWidth = width;
	MGEGL.State.framebufferHeight = height;
	MGEGL.State.vertexCounter = 0;
	MGEGL.State.currentDepth = -1.0f;

	MGEGL.State.vertexBuffer.elementCount = MAX_BUFFER_ELEMENTS;
	MGEGL.State.vertexBuffer.vertices = (float*)malloc(MAX_BUFFER_ELEMENTS * 3 * sizeof(float));
	MGEGL.State.vertexBuffer.colors = (unsigned char*)malloc(MAX_BUFFER_ELEMENTS * 4 * sizeof(unsigned char));
	MGEGL.State.vertexBuffer.texcoords = (float*)malloc(MAX_BUFFER_ELEMENTS * 2 * sizeof(float));
	MGEGL.State.vertexBuffer.indices = (unsigned int*)malloc(MAX_BUFFER_ELEMENTS * 6 * sizeof(unsigned int));

	for (int i = 0; i < MAX_BUFFER_ELEMENTS * 3; i++)
		MGEGL.State.vertexBuffer.vertices[i] = 0.0f;
	for (int i = 0; i < MAX_BUFFER_ELEMENTS * 4; i++)
		MGEGL.State.vertexBuffer.colors[i] = 0;
	for (int i = 0; i < MAX_BUFFER_ELEMENTS * 2; i++)
		MGEGL.State.vertexBuffer.texcoords[i] = 0.0f;

	int k = 0;

	for (int j = 0; j < (6 * MAX_BUFFER_ELEMENTS); j += 6) {
		MGEGL.State.vertexBuffer.indices[j + 0] = 4 * k + 0;
		MGEGL.State.vertexBuffer.indices[j + 1] = 4 * k + 1;
		MGEGL.State.vertexBuffer.indices[j + 2] = 4 * k + 2;
		MGEGL.State.vertexBuffer.indices[j + 3] = 4 * k + 0;
		MGEGL.State.vertexBuffer.indices[j + 4] = 4 * k + 2;
		MGEGL.State.vertexBuffer.indices[j + 5] = 4 * k + 3;

		k++;
	}

	MGEGL.State.draws = (MgeGL_DrawCall*)malloc(MGEGL_DEFAULT_DRAWCALLS * sizeof(MgeGL_DrawCall));

	for (int i = 0; i < MGEGL_DEFAULT_DRAWCALLS; i++) {
		MGEGL.State.draws[i].mode = MGEGL_QUADS;
		MGEGL.State.draws[i].vertexAlignment = 0;
		MGEGL.State.draws[i].vertexCount = 0;
		// MGEGL.State.draws[i].vaoId = 0;
		// MGEGL.State.draws[i].shaderId = 0;
		// MGEGL.State.draws[i].textureId = MGEGL.State.defaultTextureId;
		// MGEGL.State.draws[i].MGEGL.State.projection = rlMatrixIdentity();
		// MGEGL.State.draws[i].MGEGL.State.modelview = rlMatrixIdentity();
	}

	MGEGL.State.drawCounter = 1;

	MGEGL.State.modelview = Matrix_Identity();
	MGEGL.State.projection = Matrix_Identity();
	MGEGL.State.currentMatrix = &MGEGL.State.modelview;

	// Load default texture
	unsigned char pixels[4] = { 255, 255, 255, 255 };
	int texWidth = 1, texHeight = 1;
	MGEGL.State.defaultTexture = MgeGL_LoadTexture(pixels, texWidth, texHeight, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);

	// Init Shader
	unsigned int vertex = MgeGL_LoadShader(vertexShaderCode, GL_VERTEX_SHADER, "vertex");
	unsigned int fragment = MgeGL_LoadShader(fragmentShaderCode, GL_FRAGMENT_SHADER, "fragment");
	MGEGL.State.defaultShaderID = MgeGL_CreateShaderProgram(vertex, fragment);
	MGEGL.State.currentShaderID = MGEGL.State.defaultShaderID;

	MGEGL.State.AttribLoc[VERTICE_LOCATION] = MgeGL_GetAttribLoc("aPos");
	MGEGL.State.AttribLoc[COLOR_LOCATION] = MgeGL_GetAttribLoc("aColor");
	MGEGL.State.AttribLoc[TEXTURE_LOCATION] = MgeGL_GetAttribLoc("aTexCoord");

	glGenVertexArrays(1, &MGEGL.State.VAO);
	glBindVertexArray(MGEGL.State.VAO);

	// positions
	glGenBuffers(1, &MGEGL.State.VBO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, MAX_BUFFER_ELEMENTS * 3 * sizeof(float), MGEGL.State.vertexBuffer.vertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(MGEGL.State.AttribLoc[VERTICE_LOCATION], 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(MGEGL.State.AttribLoc[VERTICE_LOCATION]);
	// colors
	glGenBuffers(1, &MGEGL.State.VBO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, MAX_BUFFER_ELEMENTS * 4 * sizeof(unsigned char), MGEGL.State.vertexBuffer.colors, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(MGEGL.State.AttribLoc[COLOR_LOCATION], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
	glEnableVertexAttribArray(MGEGL.State.AttribLoc[COLOR_LOCATION]);
	// textures
	glGenBuffers(1, &MGEGL.State.VBO[2]);
	glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[2]);
	glBufferData(GL_ARRAY_BUFFER, MAX_BUFFER_ELEMENTS * 2 * sizeof(float), MGEGL.State.vertexBuffer.texcoords, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(MGEGL.State.AttribLoc[TEXTURE_LOCATION], 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(MGEGL.State.AttribLoc[TEXTURE_LOCATION]);

	glGenBuffers(1, &MGEGL.State.VBO[3]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MGEGL.State.VBO[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_BUFFER_ELEMENTS * 6 * sizeof(unsigned int), MGEGL.State.vertexBuffer.indices, GL_STATIC_DRAW);

	glUseProgram(MGEGL.State.currentShaderID);
}

unsigned int MgeGL_GetDefaultShaderId()
{
	return MGEGL.State.defaultShaderID;
}

void MgeGL_SetShader(unsigned int id)
{
	if (MGEGL.State.currentShaderID != id) {
		MgeGL_Draw();
		MGEGL.State.currentShaderID = id;
	}
}

int MgeGL_LoadTexture(const void* data, int width, int height, int format, int mipmapCount)
{
	unsigned int id;

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (data != NULL) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	} else {
		TRACE_LOG(LOG_INFO, "could not load texture");
	}

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
	glDeleteProgram(MGEGL.State.currentShaderID);

	free(MGEGL.State.vertexBuffer.vertices);
	free(MGEGL.State.vertexBuffer.colors);
	free(MGEGL.State.vertexBuffer.texcoords);
	free(MGEGL.State.vertexBuffer.indices);
	free(MGEGL.State.draws);
}

void MgeGL_Begin(int mode)
{
	if (MGEGL.State.draws[MGEGL.State.drawCounter - 1].mode != mode) {
		if (MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount > 0) {
			if (MGEGL.State.draws[MGEGL.State.drawCounter - 1].mode == MGEGL_LINES) MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexAlignment = ((MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount < 4)? MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount : MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount%4);
			else if (MGEGL.State.draws[MGEGL.State.drawCounter - 1].mode == MGEGL_TRIANGLES) MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexAlignment = ((MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount < 4)? 1 : (4 - (MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount%4)));
			else MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexAlignment = 0;

			if (!MgeGL_CheckRenderBatchLimit(MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexAlignment))
			{
				MGEGL.State.vertexCounter += MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexAlignment;
				MGEGL.State.drawCounter++;
			}
		}

		if (MGEGL.State.drawCounter >= MGEGL_DEFAULT_DRAWCALLS) MgeGL_Draw();

		MGEGL.State.draws[MGEGL.State.drawCounter - 1].mode = mode;
		MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount = 0;
	}

	// printf("MGEGL.State.drawCounter: %d\n", MGEGL.State.vertexCounter);
}

bool MgeGL_CheckRenderBatchLimit(int vCount)
{
	bool overflow = false;

	if ((MGEGL.State.vertexCounter + vCount) >=
		(MGEGL.State.vertexBuffer.elementCount*4))
	{
		overflow = true;

		// Store current primitive drawing mode and texture id
		int currentMode = MGEGL.State.draws[MGEGL.State.drawCounter - 1].mode;
		// int currentTexture = MGEGL.State.draws[MGEGL.State.drawCounter - 1].textureId;

		MgeGL_Draw();

		// Restore state of last batch so we can continue adding vertices
		MGEGL.State.draws[MGEGL.State.drawCounter - 1].mode = currentMode;
		// MGEGL.State.draws[MGEGL.State.drawCounter - 1].textureId = currentTexture;
	}

	return overflow;
}

void MgeGL_End()
{
	MGEGL.State.currentDepth += (1.0f / 20000.0f);
}

void MgeGL_Draw()
{
	if (MGEGL.State.vertexCounter > 0)
	{
		MgeGL_UniformMatrix4fv("modelview", MGEGL.State.modelview);
		MgeGL_UniformMatrix4fv("projection", MGEGL.State.projection);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, MGEGL.State.defaultTexture);

		glUseProgram(MGEGL.State.currentShaderID);
		glBindVertexArray(MGEGL.State.VAO);

		// positions
		glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter * 3 * sizeof(float), MGEGL.State.vertexBuffer.vertices);
		// colors
		glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[1]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter * 4 * sizeof(unsigned char), MGEGL.State.vertexBuffer.colors);
		// textures
		glBindBuffer(GL_ARRAY_BUFFER, MGEGL.State.VBO[2]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, MGEGL.State.vertexCounter * 2 * sizeof(float), MGEGL.State.vertexBuffer.texcoords);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MGEGL.State.VBO[3]);

		for (int i = 0, vertexOffset = 0; i < MGEGL.State.drawCounter; i++)
		{
			// glBindTexture(GL_TEXTURE_2D, batch->draws[i].textureId);
			if (
				MGEGL.State.draws[i].mode == MGEGL_LINES ||
				MGEGL.State.draws[i].mode == MGEGL_TRIANGLES
			) {
				glDrawArrays(MGEGL.State.draws[i].mode, vertexOffset, MGEGL.State.draws[i].vertexCount);
			} else {
				glDrawElements(GL_TRIANGLES, MGEGL.State.draws[i].vertexCount / 4 * 6, GL_UNSIGNED_INT, (GLvoid *)(vertexOffset/4*6*sizeof(GLuint)));
			}
			vertexOffset += (MGEGL.State.draws[i].vertexCount + MGEGL.State.draws[i].vertexAlignment);
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
	}
	// glUseProgram(0);

	MGEGL.State.vertexCounter = 0;
	MGEGL.State.currentDepth = -1.0f;

	for (int i = 0; i < MGEGL_DEFAULT_DRAWCALLS; i++)
	{
		MGEGL.State.draws[i].mode = MGEGL_QUADS;
		MGEGL.State.draws[i].vertexCount = 0;
		// MGEGL.State.draws[i].textureId = RLGL.State.defaultTextureId;
	}
	MGEGL.State.drawCounter = 1;
}

void MgeGL_SetTexture(unsigned int id)
{
	// glBindTexture(GL_TEXTURE_2D, id);
	MGEGL.State.defaultTexture = id;
}

void MgeGL_MatrixMode(int mode)
{
	if (mode == MGEGL_PROJECTION)
		MGEGL.State.currentMatrix = &MGEGL.State.projection;
	else if (mode == MGEGL_MODELVIEW)
		MGEGL.State.currentMatrix = &MGEGL.State.modelview;

	MGEGL.State.currentMatrixMode = mode;
}

void MgeGL_LoadIdentity(void)
{
	*MGEGL.State.currentMatrix = Matrix_Identity();
}

void MgeGL_Translatef(float x, float y, float z)
{
	Matrix matTranslation = {
		1.0f, 0.0f, 0.0f, x,
		0.0f, 1.0f, 0.0f, y,
		0.0f, 0.0f, 1.0f, z,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	// NOTE: We transpose matrix with multiplication order
	*MGEGL.State.currentMatrix = Matrix_Multiply(matTranslation, *MGEGL.State.currentMatrix);
}

void MgeGL_Rotatef(float angle, float x, float y, float z)
{
	Matrix matRotation = Matrix_Identity();

	// Axis vector (x, y, z) normalization
	float lengthSquared = x * x + y * y + z * z;
	if ((lengthSquared != 1.0f) && (lengthSquared != 0.0f)) {
		float inverseLength = 1.0f / sqrtf(lengthSquared);
		x *= inverseLength;
		y *= inverseLength;
		z *= inverseLength;
	}

	// Rotation matrix generation
	float sinres = sinf(DEG2RAD * angle);
	float cosres = cosf(DEG2RAD * angle);
	float t = 1.0f - cosres;

	matRotation.m0 = x * x * t + cosres;
	matRotation.m1 = y * x * t + z * sinres;
	matRotation.m2 = z * x * t - y * sinres;
	matRotation.m3 = 0.0f;

	matRotation.m4 = x * y * t - z * sinres;
	matRotation.m5 = y * y * t + cosres;
	matRotation.m6 = z * y * t + x * sinres;
	matRotation.m7 = 0.0f;

	matRotation.m8 = x * z * t + y * sinres;
	matRotation.m9 = y * z * t - x * sinres;
	matRotation.m10 = z * z * t + cosres;
	matRotation.m11 = 0.0f;

	matRotation.m12 = 0.0f;
	matRotation.m13 = 0.0f;
	matRotation.m14 = 0.0f;
	matRotation.m15 = 1.0f;

	// NOTE: We transpose matrix with multiplication order
	*MGEGL.State.currentMatrix = Matrix_Multiply(matRotation, *MGEGL.State.currentMatrix);
}

void MgeGL_MultMatrixf(const float* matf)
{
	// Matrix creation from array
	Matrix mat = {
		matf[0], matf[4], matf[8], matf[12],
		matf[1], matf[5], matf[9], matf[13],
		matf[2], matf[6], matf[10], matf[14],
		matf[3], matf[7], matf[11], matf[15]
	};

	*MGEGL.State.currentMatrix = Matrix_Multiply(*MGEGL.State.currentMatrix, mat);
}

// Push the current matrix into MGEGL.State.stack
void MgeGL_PushMatrix(void)
{
	if (MGEGL.State.stackCounter >= MGEGL_MAX_MATRIX_STACK_SIZE)
		TRACE_LOG(LOG_ERROR, "MGEGL: Matrix stack overflow (MGEGL_MAX_MATRIX_STACK_SIZE)");

	if (MGEGL.State.currentMatrixMode == MGEGL_MODELVIEW) {
		MGEGL.State.transformRequired = true;
		MGEGL.State.currentMatrix = &MGEGL.State.transform;
	}

	MGEGL.State.stack[MGEGL.State.stackCounter] = *MGEGL.State.currentMatrix;
	MGEGL.State.stackCounter++;
}

// Pop lattest inserted matrix from MGEGL.State.stack
void MgeGL_PopMatrix(void)
{
	if (MGEGL.State.stackCounter > 0) {
		Matrix mat = MGEGL.State.stack[MGEGL.State.stackCounter - 1];
		*MGEGL.State.currentMatrix = mat;
		MGEGL.State.stackCounter--;
	}

	if ((MGEGL.State.stackCounter == 0) && (MGEGL.State.currentMatrixMode == MGEGL_MODELVIEW)) {
		MGEGL.State.currentMatrix = &MGEGL.State.modelview;
		MGEGL.State.transformRequired = false;
	}
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
	float tx = x;
	float ty = y;
	float tz = z;

	// Transform provided vector if required
	if (MGEGL.State.transformRequired) {
		tx = MGEGL.State.transform.m0 * x + MGEGL.State.transform.m4 * y + MGEGL.State.transform.m8 * z + MGEGL.State.transform.m12;
		ty = MGEGL.State.transform.m1 * x + MGEGL.State.transform.m5 * y + MGEGL.State.transform.m9 * z + MGEGL.State.transform.m13;
		tz = MGEGL.State.transform.m2 * x + MGEGL.State.transform.m6 * y + MGEGL.State.transform.m10 * z + MGEGL.State.transform.m14;
	}

	if (MGEGL.State.vertexCounter > (MGEGL.State.vertexBuffer.elementCount*4 - 4))
	{
		if ((MGEGL.State.draws[MGEGL.State.drawCounter - 1].mode == MGEGL_LINES) &&
			(MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount%2 == 0))
		{
			// Reached the maximum number of vertices for MGEGL_LINES drawing
			// Launch a draw call but keep current state for next vertices comming
			// NOTE: We add +1 vertex to the check for security
			MgeGL_CheckRenderBatchLimit(2 + 1);
		}
		else if ((MGEGL.State.draws[MGEGL.State.drawCounter - 1].mode == MGEGL_TRIANGLES) &&
			(MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount%3 == 0))
		{
			MgeGL_CheckRenderBatchLimit(3 + 1);
		}
		else if ((MGEGL.State.draws[MGEGL.State.drawCounter - 1].mode == MGEGL_QUADS) &&
			(MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount%4 == 0))
		{
			MgeGL_CheckRenderBatchLimit(4 + 1);
		}
	}

	// Adding position
	MGEGL.State.vertexBuffer.vertices[3 * MGEGL.State.vertexCounter + 0] = tx;
	MGEGL.State.vertexBuffer.vertices[3 * MGEGL.State.vertexCounter + 1] = ty;
	MGEGL.State.vertexBuffer.vertices[3 * MGEGL.State.vertexCounter + 2] = tz;
	// Adding color
	MGEGL.State.vertexBuffer.colors[4 * MGEGL.State.vertexCounter + 0] = MGEGL.State.colorr;
	MGEGL.State.vertexBuffer.colors[4 * MGEGL.State.vertexCounter + 1] = MGEGL.State.colorg;
	MGEGL.State.vertexBuffer.colors[4 * MGEGL.State.vertexCounter + 2] = MGEGL.State.colorb;
	MGEGL.State.vertexBuffer.colors[4 * MGEGL.State.vertexCounter + 3] = MGEGL.State.colora;
	// Adding texture
	MGEGL.State.vertexBuffer.texcoords[2 * MGEGL.State.vertexCounter + 0] = MGEGL.State.texcoordx;
	MGEGL.State.vertexBuffer.texcoords[2 * MGEGL.State.vertexCounter + 1] = MGEGL.State.texcoordy;

	MGEGL.State.vertexCounter++;
	MGEGL.State.draws[MGEGL.State.drawCounter - 1].vertexCount++;
}

void MgeGL_Frustum(double left, double right, double bottom, double top, double znear, double zfar)
{
	Matrix matFrustum = { 0 };

	float rl = (float)(right - left);
	float tb = (float)(top - bottom);
	float fn = (float)(zfar - znear);

	matFrustum.m0 = ((float)znear * 2.0f) / rl;
	matFrustum.m1 = 0.0f;
	matFrustum.m2 = 0.0f;
	matFrustum.m3 = 0.0f;

	matFrustum.m4 = 0.0f;
	matFrustum.m5 = ((float)znear * 2.0f) / tb;
	matFrustum.m6 = 0.0f;
	matFrustum.m7 = 0.0f;

	matFrustum.m8 = ((float)right + (float)left) / rl;
	matFrustum.m9 = ((float)top + (float)bottom) / tb;
	matFrustum.m10 = -((float)zfar + (float)znear) / fn;
	matFrustum.m11 = -1.0f;

	matFrustum.m12 = 0.0f;
	matFrustum.m13 = 0.0f;
	matFrustum.m14 = -((float)zfar * (float)znear * 2.0f) / fn;
	matFrustum.m15 = 0.0f;

	*MGEGL.State.currentMatrix = Matrix_Multiply(*MGEGL.State.currentMatrix, matFrustum);
}

void MgeGL_Ortho(double left, double right, double bottom, double top, double znear, double zfar)
{
	Matrix matOrtho = { 0 };

	float rl = (float)(right - left);
	float tb = (float)(top - bottom);
	float fn = (float)(zfar - znear);

	matOrtho.m0 = 2.0f / rl;
	matOrtho.m1 = 0.0f;
	matOrtho.m2 = 0.0f;
	matOrtho.m3 = 0.0f;
	matOrtho.m4 = 0.0f;
	matOrtho.m5 = 2.0f / tb;
	matOrtho.m6 = 0.0f;
	matOrtho.m7 = 0.0f;
	matOrtho.m8 = 0.0f;
	matOrtho.m9 = 0.0f;
	matOrtho.m10 = -2.0f / fn;
	matOrtho.m11 = 0.0f;
	matOrtho.m12 = -((float)left + (float)right) / rl;
	matOrtho.m13 = -((float)top + (float)bottom) / tb;
	matOrtho.m14 = -((float)zfar + (float)znear) / fn;
	matOrtho.m15 = 1.0f;

	*MGEGL.State.currentMatrix = Matrix_Multiply(*MGEGL.State.currentMatrix, matOrtho);
}

int MgeGL_GetAttribLoc(const char* name)
{
	return glGetAttribLocation(MGEGL.State.currentShaderID, name);
}

void MgeGL_Uniform1i(const char* name, const int value)
{
	glUniform1i(glGetUniformLocation(MGEGL.State.currentShaderID, name), value);
}

void MgeGL_Uniform3fv(const char* name, const Vector3& value)
{
	float vectorValue[3] = {
		value.x, value.y, value.z
	};
	glUniform3fv(glGetUniformLocation(MGEGL.State.currentShaderID, name), 1, vectorValue);
}

void MgeGL_Uniform4fv(const char* name, const Vector4& value)
{
	float vectorValue[4] = {
		value.x, value.y, value.z, value.w
	};
	glUniform4fv(glGetUniformLocation(MGEGL.State.currentShaderID, name), 1, vectorValue);
}

void MgeGL_UniformMatrix4fv(const char* name, const Matrix& value)
{
	float matValue[16] = {
		value.m0, value.m1, value.m2, value.m3,
		value.m4, value.m5, value.m6, value.m7,
		value.m8, value.m9, value.m10, value.m11,
		value.m12, value.m13, value.m14, value.m15
	};
	glUniformMatrix4fv(glGetUniformLocation(MGEGL.State.currentShaderID, name), 1, GL_FALSE, matValue);
}

unsigned int MgeGL_LoadShader(const char* code, unsigned int shaderType, const char* typeName)
{
	unsigned int shader;
	int success;
	char infoLog[512];

	shader = glCreateShader(shaderType);
	TRACE_LOG(LOG_INFO, "Shader: %s shader created", typeName);
	if (shader == 0) {
		TRACE_LOG(LOG_ERROR, "Shader: Unable to create shader: %s", typeName);
		return EXIT_FAILURE;
	}
	glShaderSource(shader, 1, &code, NULL);
	glCompileShader(shader);
	TRACE_LOG(LOG_INFO, "Shader: %s shader compiled", typeName);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		TRACE_LOG(LOG_ERROR, "Shader: %s shader not compiled Error: %s",
			typeName, infoLog);
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

void MgeGL_UnloadShaderProgram(unsigned int id)
{
	glDeleteProgram(id);

	TRACE_LOG(LOG_INFO, "Shader: [ID %i] Unloaded shader program data from VRAM (GPU)", id);
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
}

void MgeGL_EnableDepthTest(void) { glEnable(GL_DEPTH_TEST); }
void MgeGL_DisableDepthTest(void) { glDisable(GL_DEPTH_TEST); }
