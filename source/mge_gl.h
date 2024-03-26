#pragma once

#include "mge.h"
#include "mge_math.h"
#include "mge_utils.h"

#include <glad/glad.h>

#define MGEGL_LINES		0x0001
#define MGEGL_TRIANGLES	0x0002
#define MGEGL_QUADS		0x0003

#ifndef MGEGL_DEFAULT_BATCH_MAX_TEXTURE_UNITS
    #define MGEGL_DEFAULT_BATCH_MAX_TEXTURE_UNITS       4      // Maximum number of textures units that can be activated on batch drawing (SetShaderValueTexture())
#endif
#ifndef MGEGL_DEFAULT_BATCH_DRAWCALLS
    #define MGEGL_DEFAULT_BATCH_DRAWCALLS             256      // Default number of batch draw calls (by state changes: mode, texture)
#endif

typedef struct MgeGL_VertexBuffer {
    int elementCount;           // Number of elements in the buffer (QUADS)

    float *vertices;            // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    float *texcoords;           // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    unsigned char *colors;
    unsigned int vaoId;         // OpenGL Vertex Array Object id
    unsigned int vboId[4];      // OpenGL Vertex Buffer Objects id (4 types of vertex data)
} MgeGL_VertexBuffer;

typedef struct MgeGL_DrawCall {
	int mode;
	int vertexCount;
	unsigned int textureId;
	int vertexAlignment;
} MgeGL_DrawCall;

typedef struct MgeGL_RenderBatch {
	float currentDepth;
	int currentBuffer;
	int drawCounter;
	int bufferCount;
	MgeGL_VertexBuffer *vertexBuffer;
	MgeGL_DrawCall *draws;
} MgeGL_RenderBatch;

typedef struct MgeGL_Data {
	MgeGL_RenderBatch *currentBatch;

	struct {
		int vertexCounter;
		unsigned int currentShaderId;
		unsigned int defaultTextureId;
		unsigned char colorr, colorg, colorb, colora;
		bool transformRequired;
		Matrix transform;
		Matrix projection;
		Matrix modelview;
		Matrix projectionStereo[2];
        Matrix viewOffsetStereo[2];
		float texcoordx, texcoordy;
		bool stereoRender;
		int framebufferWidth;
        int framebufferHeight;
		int *currentShaderLocs;
		unsigned int activeTextureId[MGEGL_DEFAULT_BATCH_MAX_TEXTURE_UNITS];
	} State;

	struct {
		bool vao;
	} ExtSupported;
} MgeGL_Data;

typedef enum {
    MGEGL_SHADER_LOC_VERTEX_POSITION = 0,  // Shader location: vertex attribute: position
    MGEGL_SHADER_LOC_VERTEX_TEXCOORD01,    // Shader location: vertex attribute: texcoord01
    MGEGL_SHADER_LOC_VERTEX_TEXCOORD02,    // Shader location: vertex attribute: texcoord02
    MGEGL_SHADER_LOC_VERTEX_NORMAL,        // Shader location: vertex attribute: normal
    MGEGL_SHADER_LOC_VERTEX_TANGENT,       // Shader location: vertex attribute: tangent
    MGEGL_SHADER_LOC_VERTEX_COLOR,         // Shader location: vertex attribute: color
    MGEGL_SHADER_LOC_MATRIX_MVP,           // Shader location: matrix uniform: model-view-projection
    MGEGL_SHADER_LOC_MATRIX_VIEW,          // Shader location: matrix uniform: view (camera transform)
    MGEGL_SHADER_LOC_MATRIX_PROJECTION,    // Shader location: matrix uniform: projection
    MGEGL_SHADER_LOC_MATRIX_MODEL,         // Shader location: matrix uniform: model (transform)
    MGEGL_SHADER_LOC_MATRIX_NORMAL,        // Shader location: matrix uniform: normal
    MGEGL_SHADER_LOC_VECTOR_VIEW,          // Shader location: vector uniform: view
    MGEGL_SHADER_LOC_COLOR_DIFFUSE,        // Shader location: vector uniform: diffuse color
    MGEGL_SHADER_LOC_COLOR_SPECULAR,       // Shader location: vector uniform: specular color
    MGEGL_SHADER_LOC_COLOR_AMBIENT,        // Shader location: vector uniform: ambient color
    MGEGL_SHADER_LOC_MAP_ALBEDO,           // Shader location: sampler2d texture: albedo (same as: MGEGL_SHADER_LOC_MAP_DIFFUSE)
    MGEGL_SHADER_LOC_MAP_METALNESS,        // Shader location: sampler2d texture: metalness (same as: MGEGL_SHADER_LOC_MAP_SPECULAR)
    MGEGL_SHADER_LOC_MAP_NORMAL,           // Shader location: sampler2d texture: normal
    MGEGL_SHADER_LOC_MAP_ROUGHNESS,        // Shader location: sampler2d texture: roughness
    MGEGL_SHADER_LOC_MAP_OCCLUSION,        // Shader location: sampler2d texture: occlusion
    MGEGL_SHADER_LOC_MAP_EMISSION,         // Shader location: sampler2d texture: emission
    MGEGL_SHADER_LOC_MAP_HEIGHT,           // Shader location: sampler2d texture: height
    MGEGL_SHADER_LOC_MAP_CUBEMAP,          // Shader location: samplerCube texture: cubemap
    MGEGL_SHADER_LOC_MAP_IRRADIANCE,       // Shader location: samplerCube texture: irradiance
    MGEGL_SHADER_LOC_MAP_PREFILTER,        // Shader location: samplerCube texture: prefilter
    MGEGL_SHADER_LOC_MAP_BRDF              // Shader location: sampler2d texture: brdf
} MgeGL_ShaderLocationIndex;

#define MGEGL_SHADER_LOC_MAP_DIFFUSE       MGEGL_SHADER_LOC_MAP_ALBEDO
#define MGEGL_SHADER_LOC_MAP_SPECULAR      MGEGL_SHADER_LOC_MAP_METALNESS

void MgeGL_Clear_Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void MgeGL_Clear_Screen_Buffers(void);
void MgeGL_Load_Extensions(void* loader);
void MgeGL_Enable_Depth_Test(void);
bool MgeGL_CheckRenderBatchLimit(int vCount);
void MgeGL_DrawRenderBatch(MgeGL_RenderBatch *batch);
void MgeGL_Viewport(int x, int y, int width, int height);
void MgeGL_SetMatrixModelview(Matrix view);
void MgeGL_SetMatrixProjection(Matrix projection);

void MgeGL_Init(void);
void MgeGL_Close(void);
void MgeGL_Begin(int mode);
void MgeGL_End(void);
void MgeGL_Color4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w);
void MgeGL_Vertex2f(float x, float y);
void MgeGL_Vertex3f(float x, float y, float z);

// #define MGEGL_IMLEMENTATION
#ifdef MGEGL_IMLEMENTATION
MgeGL_Data glData = {0};

void MgeGL_Init(void){
}

void MgeGL_Close(void)
{
}

void MgeGL_Begin(int mode)
{
}

void MgeGL_End(void)
{
}

void MgeGL_Color4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w)
{
	glData.State.colorr = x;
	glData.State.colorg = y;
	glData.State.colorb = z;
	glData.State.colora = w;
}

void MgeGL_Vertex2f(float x, float y)
{
	MgeGL_Vertex3f(x, y, glData.currentBatch->currentDepth);
}

void MgeGL_Vertex3f(float x, float y, float z)
{
	float tx = x;
	float ty = y;
	float tz = z;	
	// Transform provided vector if required
	if (glData.State.transformRequired)
	{
		tx = 
			glData.State.transform.m0*x + glData.State.transform.m4*y + 
			glData.State.transform.m8*z + glData.State.transform.m12;
		ty = 
			glData.State.transform.m1*x + glData.State.transform.m5*y +
			glData.State.transform.m9*z + glData.State.transform.m13;
		tz =
			glData.State.transform.m2*x + glData.State.transform.m6*y + 
			glData.State.transform.m10*z + glData.State.transform.m14;
	}	
	// WARNING: We can't break primitives when launching a new batch.
	// MGEGL_LINES comes in pairs, MGEGL_TRIANGLES come in groups of 3 vertices and MGEGL_QUADS come in groups of 4 vertices.
	// We must check current draw.mode when a new vertex is required and finish the batch only if the draw.mode draw.vertexCount is %2, %3 or %4
	if (
		glData.State.vertexCounter > 
		(glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].elementCount*4 - 4)
	)
	{
	    if ((glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].mode == MGEGL_LINES) &&
	        (glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].vertexCount%2 == 0))
	    {
	        // Reached the maximum number of vertices for RL_LINES drawing
	        // Launch a draw call but keep current state for next vertices comming
	        // NOTE: We add +1 vertex to the check for security
	        MgeGL_CheckRenderBatchLimit(2 + 1);
	    }
	    else if ((glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].mode == MGEGL_TRIANGLES) &&
	        (glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].vertexCount%3 == 0))
	    {
	        MgeGL_CheckRenderBatchLimit(3 + 1);
	    }
	    else if ((glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].mode == MGEGL_QUADS) &&
	        (glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].vertexCount%4 == 0))
	    {
	        MgeGL_CheckRenderBatchLimit(4 + 1);
	    }
	}	
	// Add vertices
	glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].vertices[3*glData.State.vertexCounter] = tx;
	glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].vertices[3*glData.State.vertexCounter + 1] = ty;
	glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].vertices[3*glData.State.vertexCounter + 2] = tz;	
	// Add current texcoord
	glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].texcoords[2*glData.State.vertexCounter] = glData.State.texcoordx;
	glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].texcoords[2*glData.State.vertexCounter + 1] = glData.State.texcoordy;	
	// WARNING: By default rlVertexBuffer struct does not store normals	
	// Add current color
	glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].colors[4*glData.State.vertexCounter] = glData.State.colorr;
	glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].colors[4*glData.State.vertexCounter + 1] = glData.State.colorg;
	glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].colors[4*glData.State.vertexCounter + 2] = glData.State.colorb;
	glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].colors[4*glData.State.vertexCounter + 3] = glData.State.colora;	
	glData.State.vertexCounter++;
	glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].vertexCount++;
}

void MgeGL_Clear_Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    float cr = (float)r / 255;
    float cg = (float)g / 255;
    float cb = (float)b / 255;
    float ca = (float)a / 255;

    glClearColor(cr, cg, cb, ca);
}

bool MgeGL_CheckRenderBatchLimit(int vCount)
{
	bool overflow = false;

    if ((glData.State.vertexCounter + vCount) >=
        (glData.currentBatch->vertexBuffer[glData.currentBatch->currentBuffer].elementCount*4))
    {
		overflow = true;

        // Store current primitive drawing mode and texture id
		int currentMode = glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].mode;
		int currentTexture = glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].textureId;

		MgeGL_DrawRenderBatch(glData.currentBatch);    // NOTE: Stereo rendering is checked inside

        // Restore state of last batch so we can continue adding vertices
		glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].mode = currentMode;
		glData.currentBatch->draws[glData.currentBatch->drawCounter - 1].textureId = currentTexture;
    }

	return overflow;
}

void MgeGL_SetMatrixModelview(Matrix view)
{
    glData.State.modelview = view;
}

void MgeGL_SetMatrixProjection(Matrix projection)
{
    glData.State.projection = projection;
}

void MgeGL_DrawRenderBatch(MgeGL_RenderBatch *batch)
{
    // Update batch vertex buffers
    //------------------------------------------------------------------------------------------------------------
    // NOTE: If there is not vertex data, buffers doesn't need to be updated (vertexCount > 0)
    // TODO: If no data changed on the CPU arrays --> No need to re-update GPU arrays (use a change detector flag?)
    if (glData.State.vertexCounter > 0)
    {
        // Activate elements VAO
        if (glData.ExtSupported.vao) glBindVertexArray(batch->vertexBuffer[batch->currentBuffer].vaoId);

        // Vertex positions buffer
        glBindBuffer(GL_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, glData.State.vertexCounter*3*sizeof(float), batch->vertexBuffer[batch->currentBuffer].vertices);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*4*batch->vertexBuffer[batch->currentBuffer].elementCount, batch->vertexBuffer[batch->currentBuffer].vertices, GL_DYNAMIC_DRAW);  // Update all buffer

        // Texture coordinates buffer
        glBindBuffer(GL_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, glData.State.vertexCounter*2*sizeof(float), batch->vertexBuffer[batch->currentBuffer].texcoords);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*4*batch->vertexBuffer[batch->currentBuffer].elementCount, batch->vertexBuffer[batch->currentBuffer].texcoords, GL_DYNAMIC_DRAW); // Update all buffer

        // Colors buffer
        glBindBuffer(GL_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, glData.State.vertexCounter*4*sizeof(unsigned char), batch->vertexBuffer[batch->currentBuffer].colors);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*4*batch->vertexBuffer[batch->currentBuffer].elementCount, batch->vertexBuffer[batch->currentBuffer].colors, GL_DYNAMIC_DRAW);    // Update all buffer

        // NOTE: glMapBuffer() causes sync issue.
        // If GPU is working with this buffer, glMapBuffer() will wait(stall) until GPU to finish its job.
        // To avoid waiting (idle), you can call first glBufferData() with NULL pointer before glMapBuffer().
        // If you do that, the previous data in PBO will be discarded and glMapBuffer() returns a new
        // allocated pointer immediately even if GPU is still working with the previous data.

        // Another option: map the buffer object into client's memory
        // Probably this code could be moved somewhere else...
        // batch->vertexBuffer[batch->currentBuffer].vertices = (float *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
        // if (batch->vertexBuffer[batch->currentBuffer].vertices)
        // {
            // Update vertex data
        // }
        // glUnmapBuffer(GL_ARRAY_BUFFER);

        // Unbind the current VAO
        if (glData.ExtSupported.vao) glBindVertexArray(0);
    }
    //------------------------------------------------------------------------------------------------------------

    // Draw batch vertex buffers (considering VR stereo if required)
    //------------------------------------------------------------------------------------------------------------
    Matrix matProjection = glData.State.projection;
    Matrix matModelView = glData.State.modelview;

    int eyeCount = 1;
    if (glData.State.stereoRender) eyeCount = 2;

    for (int eye = 0; eye < eyeCount; eye++)
    {
        if (eyeCount == 2)
        {
            // Setup current eye viewport (half screen width)
            MgeGL_Viewport(eye*glData.State.framebufferWidth/2, 0, glData.State.framebufferWidth/2, glData.State.framebufferHeight);

            // Set current eye view offset to modelview matrix
            MgeGL_SetMatrixModelview(Matrix_Multiply(matModelView, glData.State.viewOffsetStereo[eye]));
            // Set current eye projection matrix
            MgeGL_SetMatrixProjection(glData.State.projectionStereo[eye]);
        }

        // Draw buffers
        if (glData.State.vertexCounter > 0)
        {
            // Set current shader and upload current MVP matrix
            glUseProgram(glData.State.currentShaderId);

            // Create modelview-projection matrix and upload to shader
            Matrix matMVP = Matrix_Multiply(glData.State.modelview, glData.State.projection);
            float matMVPfloat[16] = {
                matMVP.m0, matMVP.m1, matMVP.m2, matMVP.m3,
                matMVP.m4, matMVP.m5, matMVP.m6, matMVP.m7,
                matMVP.m8, matMVP.m9, matMVP.m10, matMVP.m11,
                matMVP.m12, matMVP.m13, matMVP.m14, matMVP.m15
            };
            glUniformMatrix4fv(glData.State.currentShaderLocs[MGEGL_SHADER_LOC_MATRIX_MVP], 1, false, matMVPfloat);

            if (glData.ExtSupported.vao) glBindVertexArray(batch->vertexBuffer[batch->currentBuffer].vaoId);
            else
            {
                // Bind vertex attrib: position (shader-location = 0)
                glBindBuffer(GL_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[0]);
                glVertexAttribPointer(glData.State.currentShaderLocs[MGEGL_SHADER_LOC_VERTEX_POSITION], 3, GL_FLOAT, 0, 0, 0);
                glEnableVertexAttribArray(glData.State.currentShaderLocs[MGEGL_SHADER_LOC_VERTEX_POSITION]);

                // Bind vertex attrib: texcoord (shader-location = 1)
                glBindBuffer(GL_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[1]);
                glVertexAttribPointer(glData.State.currentShaderLocs[MGEGL_SHADER_LOC_VERTEX_TEXCOORD01], 2, GL_FLOAT, 0, 0, 0);
                glEnableVertexAttribArray(glData.State.currentShaderLocs[MGEGL_SHADER_LOC_VERTEX_TEXCOORD01]);

                // Bind vertex attrib: color (shader-location = 3)
                glBindBuffer(GL_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[2]);
                glVertexAttribPointer(glData.State.currentShaderLocs[MGEGL_SHADER_LOC_VERTEX_COLOR], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
                glEnableVertexAttribArray(glData.State.currentShaderLocs[MGEGL_SHADER_LOC_VERTEX_COLOR]);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[3]);
            }

            // Setup some default shader values
            glUniform4f(glData.State.currentShaderLocs[MGEGL_SHADER_LOC_COLOR_DIFFUSE], 1.0f, 1.0f, 1.0f, 1.0f);
            glUniform1i(glData.State.currentShaderLocs[MGEGL_SHADER_LOC_MAP_DIFFUSE], 0);  // Active default sampler2D: texture0

            // Activate additional sampler textures
            // Those additional textures will be common for all draw calls of the batch
            for (int i = 0; i < MGEGL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++)
            {
                if (glData.State.activeTextureId[i] > 0)
                {
                    glActiveTexture(GL_TEXTURE0 + 1 + i);
                    glBindTexture(GL_TEXTURE_2D, glData.State.activeTextureId[i]);
                }
            }

            // Activate default sampler2D texture0 (one texture is always active for default batch shader)
            // NOTE: Batch system accumulates calls by texture0 changes, additional textures are enabled for all the draw calls
            glActiveTexture(GL_TEXTURE0);

            for (int i = 0, vertexOffset = 0; i < batch->drawCounter; i++)
            {
                // Bind current draw call texture, activated as GL_TEXTURE0 and Bound to sampler2D texture0 by default
                glBindTexture(GL_TEXTURE_2D, batch->draws[i].textureId);

                if ((batch->draws[i].mode == MGEGL_LINES) || (batch->draws[i].mode == MGEGL_TRIANGLES)) glDrawArrays(batch->draws[i].mode, vertexOffset, batch->draws[i].vertexCount);
                else
                {
                    // We need to define the number of indices to be processed: elementCount*6
                    // NOTE: The final parameter tells the GPU the offset in bytes from the
                    // start of the index buffer to the location of the first index to process
                    glDrawElements(GL_TRIANGLES, batch->draws[i].vertexCount/4*6, GL_UNSIGNED_INT, (GLvoid *)(vertexOffset/4*6*sizeof(GLuint)));
                }

                vertexOffset += (batch->draws[i].vertexCount + batch->draws[i].vertexAlignment);
            }

            if (!glData.ExtSupported.vao)
            {
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }

            glBindTexture(GL_TEXTURE_2D, 0);    // Unbind textures
        }

        if (glData.ExtSupported.vao) glBindVertexArray(0); // Unbind VAO

        glUseProgram(0);    // Unbind shader program
    }

    // Restore viewport to default measures
    if (eyeCount == 2) MgeGL_Viewport(0, 0, glData.State.framebufferWidth, glData.State.framebufferHeight);
    //------------------------------------------------------------------------------------------------------------

    // Reset batch buffers
    //------------------------------------------------------------------------------------------------------------
    // Reset vertex counter for next frame
    glData.State.vertexCounter = 0;

    // Reset depth for next draw
    batch->currentDepth = -1.0f;

    // Restore projection/modelview matrices
    glData.State.projection = matProjection;
    glData.State.modelview = matModelView;

    // Reset glData.currentBatch->draws array
    for (int i = 0; i < MGEGL_DEFAULT_BATCH_DRAWCALLS; i++)
    {
        batch->draws[i].mode = MGEGL_QUADS;
        batch->draws[i].vertexCount = 0;
        batch->draws[i].textureId = glData.State.defaultTextureId;
    }

    // Reset active texture units for next batch
    for (int i = 0; i < MGEGL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++) glData.State.activeTextureId[i] = 0;

    // Reset draws counter to one draw for the batch
    batch->drawCounter = 1;
    //------------------------------------------------------------------------------------------------------------

    // Change to next buffer in the list (in case of multi-buffering)
    batch->currentBuffer++;
    if (batch->currentBuffer >= batch->bufferCount) batch->currentBuffer = 0;
}

void MgeGL_Clear_Screen_Buffers(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MgeGL_Load_Extensions(void* loader)
{
    if (gladLoadGLLoader((GLADloadproc)loader) == 0) {
        TRACE_LOG(LOG_WARNING, "GLAD: Cannot load OpenGL extensions");
    } else {
        TRACE_LOG(LOG_INFO, "GLAD: OpenGL extensions loaded successfully");
    }

    MgeGL_Enable_Depth_Test();
}

void MgeGL_Enable_Depth_Test(void) { glEnable(GL_DEPTH_TEST); }

void MgeGL_Viewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}
#endif
