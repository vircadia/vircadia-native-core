#include "renderer.h"

#include <mutex>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QDebug>

#include <gl/Config.h>
#include <gl/GLShaders.h>

static const char *kSimepleVertexShader = R"glsl(#version 300 es
#extension GL_OVR_multiview2 : enable

layout(num_views=2) in;

layout(location = 0) in vec4 a_Position;

out vec4 v_Color;

void main() {
    v_Color = vec4(a_Position.xyz, 1.0);
    gl_Position = vec4(a_Position.xyz, 1.0);
}
)glsl";

static const char *kPassthroughFragmentShader = R"glsl(#version 300 es
precision mediump float;
in vec4 v_Color;
out vec4 FragColor;

void main() { FragColor = v_Color; }
)glsl";


int LoadGLShader(int type, const char *shadercode) {
    GLuint result = 0;
    std::string shaderError;
    static const std::string SHADER_DEFINES;
    if (!gl::compileShader(type, shadercode, SHADER_DEFINES, result, shaderError)) {
        qWarning() << "QQQ" << __FUNCTION__ << "Shader compile failure" << shaderError.c_str();
    }
    return result;
}

static void CheckGLError(const char* label) {
    int gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        qWarning("GL error @ %s: %d", label, gl_error);
        // Crash immediately to make OpenGL errors obvious.
        abort();
    }
}

// Contains vertex, normal and other data.
namespace triangle {
    static std::array<float, 9> TRIANGLE_VERTS {{
                                              -0.5f, -0.5f, 0.0f,
                                              0.5f, -0.5f, 0.0f,
                                              0.0f, 0.5f, 0.0f
                                          }};
}


void NativeRenderer::InitializeGl() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    const uint32_t vertShader = LoadGLShader(GL_VERTEX_SHADER, kSimepleVertexShader);
    const uint32_t fragShader = LoadGLShader(GL_FRAGMENT_SHADER, kPassthroughFragmentShader);
    std::string error;
    _program = gl::compileProgram({ vertShader, fragShader }, error);
    CheckGLError("build program");

    glGenBuffers(1, &_geometryBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _geometryBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, triangle::TRIANGLE_VERTS.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CheckGLError("upload vertices");

    glGenVertexArrays(1, &_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _geometryBuffer);
    glBindVertexArray(_vao);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CheckGLError("build vao ");
}


void NativeRenderer::DrawFrame() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start);
    glm::vec3 v;
    v.r = (float) (now.count() % 1000) / 1000.0f;
    v.g = 1.0f - v.r;
    v.b = 1.0f;

    glClearColor(v.r, v.g, v.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(_program);
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void NativeRenderer::OnTriggerEvent() {
    qDebug() << "QQQ" << __FUNCTION__;
}

void NativeRenderer::OnPause() {
    qDebug() << "QQQ" << __FUNCTION__;
}

void NativeRenderer::OnResume() {
    qDebug() << "QQQ" << __FUNCTION__;
}
