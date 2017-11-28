#include "renderer.h"

#include <mutex>

#include <QtCore/QDebug>

#include <gl/Config.h>
#include <gl/GLShaders.h>

#include <glm/gtc/matrix_transform.hpp>

#if 0
#include <gpu/DrawTransformUnitQuad_vert.h>
#include <gpu/DrawTexcoordRectTransformUnitQuad_vert.h>
#include <gpu/DrawViewportQuadTransformTexcoord_vert.h>
#include <gpu/DrawTexture_frag.h>
#include <gpu/DrawTextureOpaque_frag.h>
#include <gpu/DrawColoredTexture_frag.h>
#include <render-utils/simple_vert.h>
#include <render-utils/simple_frag.h>
#include <render-utils/simple_textured_frag.h>
#include <render-utils/simple_textured_unlit_frag.h>

#include <render-utils/deferred_light_vert.h>
#include <render-utils/deferred_light_point_vert.h>
#include <render-utils/deferred_light_spot_vert.h>

#include <render-utils/directional_ambient_light_frag.h>
#include <render-utils/directional_skybox_light_frag.h>

#include <render-utils/standardTransformPNTC_vert.h>
#include <render-utils/standardDrawTexture_frag.h>

#include <render-utils/model_vert.h>
#include <render-utils/model_shadow_vert.h>
#include <render-utils/model_normal_map_vert.h>
#include <render-utils/model_lightmap_vert.h>
#include <render-utils/model_lightmap_normal_map_vert.h>
#include <render-utils/skin_model_vert.h>
#include <render-utils/skin_model_shadow_vert.h>
#include <render-utils/skin_model_normal_map_vert.h>

#include <render-utils/model_frag.h>
#include <render-utils/model_shadow_frag.h>
#include <render-utils/model_normal_map_frag.h>
#include <render-utils/model_normal_specular_map_frag.h>
#include <render-utils/model_specular_map_frag.h>
#include <render-utils/model_lightmap_frag.h>
#include <render-utils/model_lightmap_normal_map_frag.h>
#include <render-utils/model_lightmap_normal_specular_map_frag.h>
#include <render-utils/model_lightmap_specular_map_frag.h>
#include <render-utils/model_translucent_frag.h>
#include <render-utils/overlay3D_vert.h>
#include <render-utils/overlay3D_frag.h>

#include <render-utils/sdf_text3D_vert.h>
#include <render-utils/sdf_text3D_frag.h>

#include <model/skybox_vert.h>
#include <model/skybox_frag.h>
#include <entities-renderer/textured_particle_frag.h>
#include <entities-renderer/textured_particle_vert.h>
#include <entities-renderer/paintStroke_vert.h>
#include <entities-renderer/paintStroke_frag.h>
#include <entities-renderer/polyvox_vert.h>
#include <entities-renderer/polyvox_frag.h>
#endif

#if defined(GVR)
#include <shared/Bilateral.h>
#include "GoogleVRHelpers.h"

static const uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;
static const gvr_rectf fullscreen = {0, 1, 0, 1};

template <typename F>
void withFrameBuffer(gvr::Frame& frame, int32_t index, F f) {
    frame.BindBuffer(index);
    f();
    frame.Unbind();
}

std::array<gvr::BufferViewport, 2> buildViewports(const std::unique_ptr<gvr::GvrApi> &gvrapi) {
    return { {gvrapi->CreateBufferViewport(), gvrapi->CreateBufferViewport()} };
}

// Computes a texture size that has approximately half as many pixels. This is
// equivalent to scaling each dimension by approximately sqrt(2)/2.
static gvr::Sizei HalfPixelCount(const gvr::Sizei &in) {
    // Scale each dimension by sqrt(2)/2 ~= 7/10ths.
    gvr::Sizei out;
    out.width = (7 * in.width) / 10;
    out.height = (7 * in.height) / 10;
    return out;
}

void testShaderBuild(const char* vs_src, const char * fs_src) {
    std::string error;
    GLuint vs, fs;
    if (!gl::compileShader(GL_VERTEX_SHADER, vs_src, VERTEX_SHADER_DEFINES, vs, error) ||
        !gl::compileShader(GL_FRAGMENT_SHADER, fs_src, PIXEL_SHADER_DEFINES, fs, error)) {
        throw std::runtime_error("Failed to compile shader");
    }
    auto pr = gl::compileProgram({ vs, fs }, error);
    if (!pr) {
        throw std::runtime_error("Failed to link shader");
    }
}

#endif

static const float kZNear = 1.0f;
static const float kZFar = 100.0f;

static const char *kSimepleVertexShader = R"glsl(
#version 300 es
#extension GL_OVR_multiview2 : enable

layout(num_views=2) in;

layout(location = 0) in vec4 a_Position;

out vec4 v_Color;

void main() {
    v_Color = vec4(a_Position.xyz, 1.0);
    gl_Position = vec4(a_Position.xyz, 1.0);
}
)glsl";

static const char *kPassthroughFragmentShader = R"glsl(
#version 300 es
precision mediump float;
in vec4 v_Color;
out vec4 FragColor;

void main() { FragColor = v_Color; }
)glsl";

static void CheckGLError(const char* label) {
    int gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        qWarning("GL error @ %s: %d", label, gl_error);
        // Crash immediately to make OpenGL errors obvious.
        abort();
    }
}

// Contains vertex, normal and other data.
#if 0
namespace cube {
    const std::array<float, 108> CUBE_COORDS{{
                                                 // Front face
                                                 -1.0f, 1.0f, 1.0f,
                                                 -1.0f, -1.0f, 1.0f,
                                                 1.0f, 1.0f, 1.0f,
                                                 -1.0f, -1.0f, 1.0f,
                                                 1.0f, -1.0f, 1.0f,
                                                 1.0f, 1.0f, 1.0f,

                                                 // Right face
                                                 1.0f, 1.0f, 1.0f,
                                                 1.0f, -1.0f, 1.0f,
                                                 1.0f, 1.0f, -1.0f,
                                                 1.0f, -1.0f, 1.0f,
                                                 1.0f, -1.0f, -1.0f,
                                                 1.0f, 1.0f, -1.0f,

                                                 // Back face
                                                 1.0f, 1.0f, -1.0f,
                                                 1.0f, -1.0f, -1.0f,
                                                 -1.0f, 1.0f, -1.0f,
                                                 1.0f, -1.0f, -1.0f,
                                                 -1.0f, -1.0f, -1.0f,
                                                 -1.0f, 1.0f, -1.0f,

                                                 // Left face
                                                 -1.0f, 1.0f, -1.0f,
                                                 -1.0f, -1.0f, -1.0f,
                                                 -1.0f, 1.0f, 1.0f,
                                                 -1.0f, -1.0f, -1.0f,
                                                 -1.0f, -1.0f, 1.0f,
                                                 -1.0f, 1.0f, 1.0f,

                                                 // Top face
                                                 -1.0f, 1.0f, -1.0f,
                                                 -1.0f, 1.0f, 1.0f,
                                                 1.0f, 1.0f, -1.0f,
                                                 -1.0f, 1.0f, 1.0f,
                                                 1.0f, 1.0f, 1.0f,
                                                 1.0f, 1.0f, -1.0f,

                                                 // Bottom face
                                                 1.0f, -1.0f, -1.0f,
                                                 1.0f, -1.0f, 1.0f,
                                                 -1.0f, -1.0f, -1.0f,
                                                 1.0f, -1.0f, 1.0f,
                                                 -1.0f, -1.0f, 1.0f,
                                                 -1.0f, -1.0f, -1.0f
                                             }};

    const std::array<float, 108> CUBE_COLORS{{
                                                 // front, green
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,

                                                 // right, blue
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,

                                                 // back, also green
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,
                                                 0.0f, 0.5273f, 0.2656f,

                                                 // left, also blue
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,
                                                 0.0f, 0.3398f, 0.9023f,

                                                 // top, red
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f,

                                                 // bottom, also red
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f,
                                                 0.8359375f, 0.17578125f, 0.125f
                                             }};

    const std::array<float, 108> CUBE_NORMALS{{
                                                  // Front face
                                                  0.0f, 0.0f, 1.0f,
                                                  0.0f, 0.0f, 1.0f,
                                                  0.0f, 0.0f, 1.0f,
                                                  0.0f, 0.0f, 1.0f,
                                                  0.0f, 0.0f, 1.0f,
                                                  0.0f, 0.0f, 1.0f,

                                                  // Right face
                                                  1.0f, 0.0f, 0.0f,
                                                  1.0f, 0.0f, 0.0f,
                                                  1.0f, 0.0f, 0.0f,
                                                  1.0f, 0.0f, 0.0f,
                                                  1.0f, 0.0f, 0.0f,
                                                  1.0f, 0.0f, 0.0f,

                                                  // Back face
                                                  0.0f, 0.0f, -1.0f,
                                                  0.0f, 0.0f, -1.0f,
                                                  0.0f, 0.0f, -1.0f,
                                                  0.0f, 0.0f, -1.0f,
                                                  0.0f, 0.0f, -1.0f,
                                                  0.0f, 0.0f, -1.0f,

                                                  // Left face
                                                  -1.0f, 0.0f, 0.0f,
                                                  -1.0f, 0.0f, 0.0f,
                                                  -1.0f, 0.0f, 0.0f,
                                                  -1.0f, 0.0f, 0.0f,
                                                  -1.0f, 0.0f, 0.0f,
                                                  -1.0f, 0.0f, 0.0f,

                                                  // Top face
                                                  0.0f, 1.0f, 0.0f,
                                                  0.0f, 1.0f, 0.0f,
                                                  0.0f, 1.0f, 0.0f,
                                                  0.0f, 1.0f, 0.0f,
                                                  0.0f, 1.0f, 0.0f,
                                                  0.0f, 1.0f, 0.0f,

                                                  // Bottom face
                                                  0.0f, -1.0f, 0.0f,
                                                  0.0f, -1.0f, 0.0f,
                                                  0.0f, -1.0f, 0.0f,
                                                  0.0f, -1.0f, 0.0f,
                                                  0.0f, -1.0f, 0.0f,
                                                  0.0f, -1.0f, 0.0f
                                              }};
}
#endif

namespace triangle {
    static std::array<float, 9> TRIANGLE_VERTS {{
                                              -0.5f, -0.5f, 0.0f,
                                              0.5f, -0.5f, 0.0f,
                                              0.0f, 0.5f, 0.0f
                                          }};
}

const std::string VERTEX_SHADER_DEFINES{ R"GLSL(
#version 300 es
#extension GL_EXT_clip_cull_distance : enable
#define GPU_VERTEX_SHADER
#define GPU_SSBO_TRANSFORM_OBJECT 1
#define GPU_TRANSFORM_IS_STEREO
#define GPU_TRANSFORM_STEREO_CAMERA
#define GPU_TRANSFORM_STEREO_CAMERA_INSTANCED
#define GPU_TRANSFORM_STEREO_SPLIT_SCREEN
)GLSL" };

const std::string PIXEL_SHADER_DEFINES{ R"GLSL(
#version 300 es
precision mediump float;
#define GPU_PIXEL_SHADER
#define GPU_TRANSFORM_IS_STEREO
#define GPU_TRANSFORM_STEREO_CAMERA
#define GPU_TRANSFORM_STEREO_CAMERA_INSTANCED
#define GPU_TRANSFORM_STEREO_SPLIT_SCREEN
)GLSL" };


#if defined(GVR)
NativeRenderer::NativeRenderer(gvr_context *vrContext) :
    _gvrapi(new gvr::GvrApi(vrContext, false)),
    _viewports(buildViewports(_gvrapi)),
    _gvr_viewer_type(_gvrapi->GetViewerType())
{
    start = std::chrono::system_clock::now();
}

void NativeRenderer::InitializeVR() {
    _gvrapi->InitializeGl();
    bool multiviewEnabled = _gvrapi->IsFeatureSupported(GVR_FEATURE_MULTIVIEW);
    qWarning() << "QQQ" << __FUNCTION__ << "Multiview enabled " << multiviewEnabled;
    // Because we are using 2X MSAA, we can render to half as many pixels and
    // achieve similar quality.
    _renderSize = HalfPixelCount(_gvrapi->GetMaximumEffectiveRenderTargetSize());

    std::vector<gvr::BufferSpec> specs;
    specs.push_back(_gvrapi->CreateBufferSpec());
    specs[0].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
    specs[0].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_DEPTH_16);
    specs[0].SetSamples(2);
    gvr::Sizei half_size = {_renderSize.width / 2, _renderSize.height};
    specs[0].SetMultiviewLayers(2);
    specs[0].SetSize(half_size);

    _swapchain.reset(new gvr::SwapChain(_gvrapi->CreateSwapChain(specs)));
    _viewportlist.reset(new gvr::BufferViewportList(_gvrapi->CreateEmptyBufferViewportList()));
}

void NativeRenderer::PrepareFramebuffer() {
    const gvr::Sizei recommended_size = HalfPixelCount(
        _gvrapi->GetMaximumEffectiveRenderTargetSize());
    if (_renderSize.width != recommended_size.width ||
        _renderSize.height != recommended_size.height) {
        // We need to resize the framebuffer. Note that multiview uses two texture
        // layers, each with half the render width.
        gvr::Sizei framebuffer_size = recommended_size;
        framebuffer_size.width /= 2;
        _swapchain->ResizeBuffer(0, framebuffer_size);
        _renderSize = recommended_size;
    }
}
#else
NativeRenderer::NativeRenderer(void *vrContext)
{
    start = std::chrono::system_clock::now();
}
#endif


/**
 * Converts a raw text file, saved as a resource, into an OpenGL ES shader.
 *
 * @param type  The type of shader we will be creating.
 * @param resId The resource ID of the raw text file.
 * @return The shader object handler.
 */
int LoadGLShader(int type, const char *shadercode) {
    GLuint result = 0;
    std::string shaderError;
    static const std::string SHADER_DEFINES;
    if (!gl::compileShader(type, shadercode, SHADER_DEFINES, result, shaderError)) {
        qWarning() << "QQQ" << __FUNCTION__ << "Shader compile failure" << shaderError.c_str();
    }
    return result;
}

void NativeRenderer::InitializeGl() {
    qDebug() << "QQQ" << __FUNCTION__;

#if defined(GVR)
    InitializeVR();
#endif

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    const uint32_t vertShader = LoadGLShader(GL_VERTEX_SHADER, kSimepleVertexShader);
    const uint32_t fragShader = LoadGLShader(GL_FRAGMENT_SHADER, kPassthroughFragmentShader);
    std::string error;
    _cubeProgram = gl::compileProgram({ vertShader, fragShader }, error);
    CheckGLError("build program");

    glGenBuffers(1, &_cubeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _cubeBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, triangle::TRIANGLE_VERTS.data(), GL_STATIC_DRAW);
    /*
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 108 * 3, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 108 * 0, sizeof(float) * 108, cube::CUBE_COORDS.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 108 * 1, sizeof(float) * 108, cube::CUBE_COLORS.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 108 * 2, sizeof(float) * 108, cube::CUBE_NORMALS.data());
     */
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CheckGLError("upload vertices");

    glGenVertexArrays(1, &_cubeVao);
    glBindBuffer(GL_ARRAY_BUFFER, _cubeBuffer);
    glBindVertexArray(_cubeVao);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    /*
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, (const void*)(sizeof(float) * 108 * 1) );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, (const void*)(sizeof(float) * 108 * 2));
    glEnableVertexAttribArray(2);
     */
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

#if defined(GVR)
    PrepareFramebuffer();

    // A client app does its rendering here.
    gvr::ClockTimePoint target_time = gvr::GvrApi::GetTimePointNow();
    target_time.monotonic_system_time_nanos += kPredictionTimeWithoutVsyncNanos;

    using namespace googlevr;
    using namespace bilateral;
    const auto gvrHeadPose = _gvrapi->GetHeadSpaceFromStartSpaceRotation(target_time);
    _head_view = toGlm(gvrHeadPose);
    _viewportlist->SetToRecommendedBufferViewports();


    glm::mat4 eye_views[2];
    for_each_side([&](bilateral::Side side) {
        int eye = index(side);
        const gvr::Eye gvr_eye = eye == 0 ? GVR_LEFT_EYE : GVR_RIGHT_EYE;
        const auto& eyeView = eye_views[eye] = toGlm(_gvrapi->GetEyeFromHeadMatrix(gvr_eye)) * _head_view;
        auto& viewport = _viewports[eye];

        _viewportlist->GetBufferViewport(eye, &viewport);
        viewport.SetSourceUv(fullscreen);
        viewport.SetSourceLayer(eye);
        _viewportlist->SetBufferViewport(eye, viewport);
        const auto &mvc = _modelview_cube[eye] = eyeView * _model_cube;
        const auto &mvf = _modelview_floor[eye] = eyeView * _model_floor;
        const gvr_rectf fov = viewport.GetSourceFov();
        const glm::mat4 perspective = perspectiveMatrixFromView(fov, kZNear, kZFar);
        _modelview_projection_cube[eye] = perspective * mvc;
        _modelview_projection_floor[eye] = perspective * mvf;
        _light_pos_eye_space[eye] = glm::vec3(eyeView * _light_pos_world_space);
    });


    gvr::Frame frame = _swapchain->AcquireFrame();
    withFrameBuffer(frame, 0, [&]{
        glClearColor(v.r, v.g, v.b, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, _renderSize.width / 2, _renderSize.height);
        glUseProgram(_cubeProgram);
        glBindVertexArray(_cubeVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    });

    frame.Submit(*_viewportlist, gvrHeadPose);
    CheckGLError("onDrawFrame");
#else
    constexpr size_t eye = 0;
    const glm::mat4 eyeView{ 1 };
    const auto &mvc = _modelview_cube[eye] = eyeView * _model_cube;
    const auto &mvf = _modelview_floor[eye] = eyeView * _model_floor;
    const glm::mat4 perspective = glm::perspective(60.0f, 1.0f, kZNear, kZFar);
    _modelview_projection_cube[eye] = perspective * mvc;
    _modelview_projection_floor[eye] = perspective * mvf;
    _light_pos_eye_space[eye] = glm::vec3(eyeView * _light_pos_world_space);

    glClearColor(v.r, v.g, v.b, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glViewport(0, 0, _renderSize.width / 2, _renderSize.height);
    glUseProgram(_cubeProgram);
    glBindVertexArray(_cubeVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
#endif
}

void NativeRenderer::OnTriggerEvent() {
    qDebug() << "QQQ" << __FUNCTION__;
}

void NativeRenderer::OnPause() {
    qDebug() << "QQQ" << __FUNCTION__;
#if defined(GVR)
    _gvrapi->PauseTracking();
#endif
}

void NativeRenderer::OnResume() {
    qDebug() << "QQQ" << __FUNCTION__;
#if defined(GVR)
    _gvrapi->ResumeTracking();
    _gvrapi->RefreshViewerProfile();
#endif
}
