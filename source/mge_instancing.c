// Instanced model drawing: one glDrawElementsInstanced call per mesh paints
// every copy. The per-instance model matrix lives in a single GPU buffer bound
// onto the model's mesh VAOs as a mat4 attribute (locations 4..7, divisor 1);
// the vertex shader multiplies it in on top of the batcher's view/projection.
//
//   ModelBatch f = Mge_LoadModelBatch(model, transforms, count);
//   Mge_DrawModelBatch(f, light, camera);   // inside Mge_BeginMode3D
//   Mge_UnloadModelBatch(&f);

#include "mge.h"
#include "mge_gl.h"
#include "mge_math.h"

#include <glad/glad.h>
#include <stdlib.h>
#include <string.h>

#define INSTANCE_MATRIX_LOC 4  // occupies 4, 5, 6, 7

static const char* instVert =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 2) in vec2 aTexCoord;\n"
    "layout(location = 3) in vec3 aNormal;\n"
    "layout(location = 4) in mat4 instanceModel;\n"  // 4..7
    "out vec2 vTexCoord;\n"
    "out vec3 vFragPos;\n"
    "out vec3 vNormal;\n"
    "uniform mat4 modelview;\n"   // view only, from the batcher
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "    vec4 world = instanceModel * vec4(aPos, 1.0);\n"
    "    vFragPos = world.xyz;\n"
    "    vNormal = mat3(instanceModel) * aNormal;\n"
    "    vTexCoord = aTexCoord;\n"
    "    gl_Position = projection * modelview * world;\n"
    "}\n";

static const char* instFrag =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 vTexCoord;\n"
    "in vec3 vFragPos;\n"
    "in vec3 vNormal;\n"
    "uniform sampler2D sampleTex;\n"
    "uniform vec3 viewPos;\n"
    "uniform int  lightType;\n"   // 0 directional, 1 point
    "uniform vec3 lightPos;\n"
    "uniform vec3 lightDir;\n"
    "uniform vec3 lightColor;\n"
    "uniform float ambient;\n"
    "uniform float diffuseS;\n"
    "uniform float specularS;\n"
    "uniform float shininess;\n"
    "void main()\n"
    "{\n"
    "    vec3 N = normalize(vNormal);\n"
    "    vec3 L = (lightType == 0) ? normalize(-lightDir) : normalize(lightPos - vFragPos);\n"
    "    vec3 V = normalize(viewPos - vFragPos);\n"
    "    vec3 H = normalize(L + V);\n"   // Blinn-Phong halfway vector
    "    float diff = max(dot(N, L), 0.0);\n"
    "    float spec = (diff > 0.0) ? pow(max(dot(N, H), 0.0), max(shininess, 1.0)) : 0.0;\n"
    "    vec4 base = texture(sampleTex, vTexCoord);\n"
    "    vec3 lit = (ambient + diffuseS * diff) * lightColor + specularS * spec * lightColor;\n"
    "    FragColor = vec4(lit * base.rgb, base.a);\n"
    "}\n";

static unsigned int s_program = 0;

static void EnsureResources(void)
{
    if (s_program != 0)
        return;

    s_program = MgeGL_CreateShaderProgram(
        MgeGL_LoadShader(instVert, GL_VERTEX_SHADER, "instancing vertex"),
        MgeGL_LoadShader(instFrag, GL_FRAGMENT_SHADER, "instancing fragment"));
}

static unsigned int diffuse_texture(const Mesh* m)
{
    for (int i = 0; i < m->textureCount; i++)
        if (m->textures[i].type == MESH_TEXTURE_DIFFUSE)
            return m->textures[i].texture.id;
    return 0;
}

// pack `count` column-major matrices into a flat float buffer for the GPU
static float* pack_matrices(const Matrix* transforms, int count)
{
    float* data = malloc((size_t)count * 16 * sizeof(float));
    if (data == NULL)
        return NULL;
    for (int i = 0; i < count; i++) {
        float16 f = MatrixToFloatV(transforms[i]);
        memcpy(data + (size_t)i * 16, f.v, 16 * sizeof(float));
    }
    return data;
}

ModelBatch Mge_LoadModelBatch(Model model, const Matrix* transforms, int count)
{
    ModelBatch batch = { 0 };
    batch.model = model;

    if (transforms == NULL || count <= 0 || model.meshCount == 0)
        return batch;

    float* data = pack_matrices(transforms, count);
    if (data == NULL)
        return batch;

    batch.count = count;

    glGenBuffers(1, &batch.instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, batch.instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)count * 16 * sizeof(float), data, GL_DYNAMIC_DRAW);
    free(data);

    // attach the buffer as an instanced mat4 to every mesh VAO
    const GLsizei stride = 16 * sizeof(float);
    for (int i = 0; i < model.meshCount; i++) {
        unsigned int vao = model.meshes[i].vao;
        if (vao == 0)
            continue;
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, batch.instanceVBO);
        for (int c = 0; c < 4; c++) {
            glEnableVertexAttribArray(INSTANCE_MATRIX_LOC + c);
            glVertexAttribPointer(INSTANCE_MATRIX_LOC + c, 4, GL_FLOAT, GL_FALSE, stride,
                (void*)(size_t)(c * 4 * sizeof(float)));
            glVertexAttribDivisor(INSTANCE_MATRIX_LOC + c, 1);
        }
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return batch;
}

void Mge_UpdateModelBatch(ModelBatch* batch, const Matrix* transforms, int count)
{
    if (batch == NULL || batch->instanceVBO == 0 || transforms == NULL || count <= 0)
        return;
    if (count > batch->count)
        count = batch->count;

    float* data = pack_matrices(transforms, count);
    if (data == NULL)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, batch->instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)count * 16 * sizeof(float), data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    free(data);
}

void Mge_DrawModelBatch(ModelBatch batch, Light light, Camera3D camera)
{
    if (batch.count <= 0 || batch.instanceVBO == 0 || batch.model.meshCount == 0)
        return;

    EnsureResources();
    MgeGL_SetShader(s_program);

    MgeGL_UniformMatrix4fv("modelview", MgeGL_GetMatrixModelview());
    MgeGL_UniformMatrix4fv("projection", MgeGL_GetMatrixProjection());
    MgeGL_Uniform3fv("viewPos", camera.position);
    MgeGL_Uniform1i("lightType", (light.type == LIGHT_DIRECTIONAL) ? 0 : 1);
    MgeGL_Uniform3fv("lightPos", light.position);
    MgeGL_Uniform3fv("lightDir", light.direction);
    MgeGL_Uniform3fv("lightColor", light.color);
    MgeGL_Uniform1f("ambient", light.ambient);
    MgeGL_Uniform1f("diffuseS", light.diffuse);
    MgeGL_Uniform1f("specularS", light.specular);
    MgeGL_Uniform1f("shininess", 32.0f);
    MgeGL_Uniform1i("sampleTex", 0);

    glActiveTexture(GL_TEXTURE0);
    for (int i = 0; i < batch.model.meshCount; i++) {
        const Mesh* m = &batch.model.meshes[i];
        if (m->vao == 0 || m->indexCount <= 0)
            continue;
        unsigned int tex = diffuse_texture(m);
        glBindTexture(GL_TEXTURE_2D, (tex != 0) ? tex : MgeGL_GetWhiteTexture());
        glBindVertexArray(m->vao);
        glDrawElementsInstanced(GL_TRIANGLES, m->indexCount, GL_UNSIGNED_INT, 0, batch.count);
        MgeGL_RegisterDrawCall(); // one call, batch.count melons
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    MgeGL_SetShader(MgeGL_GetDefaultShaderId());
}

void Mge_UnloadModelBatch(ModelBatch* batch)
{
    if (batch == NULL || batch->instanceVBO == 0)
        return;

    for (int i = 0; i < batch->model.meshCount; i++) {
        unsigned int vao = batch->model.meshes[i].vao;
        if (vao == 0)
            continue;
        glBindVertexArray(vao);
        for (int c = 0; c < 4; c++)
            glDisableVertexAttribArray(INSTANCE_MATRIX_LOC + c);
    }
    glBindVertexArray(0);

    glDeleteBuffers(1, &batch->instanceVBO);
    batch->instanceVBO = 0;
    batch->count = 0;
    // batch->model is not owned -- leave it for the caller to unload
}
