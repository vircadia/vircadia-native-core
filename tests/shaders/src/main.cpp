//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <mutex>

#include <gpu/gl/GLBackend.h>

#include <QLoggingCategory>
#include <QResizeEvent>
#include <QTimer>
#include <QWindow>
#include <QElapsedTimer>
#include <QDir>
#include <QGuiApplication>

#include <gl/GLHelpers.h>
#include <gl/GLShaders.h>

#include <gl/QOpenGLContextWrapper.h>

#include "simple_vert.h"
#include "simple_frag.h"
#include "simple_textured_frag.h"
#include "simple_textured_unlit_frag.h"

#include "deferred_light_vert.h"
#include "deferred_light_point_vert.h"
#include "deferred_light_spot_vert.h"

#include "directional_ambient_light_frag.h"
#include "directional_skybox_light_frag.h"

#include "standardTransformPNTC_vert.h"
#include "standardDrawTexture_frag.h"

#include "model_vert.h"
#include "model_shadow_vert.h"
#include "model_normal_map_vert.h"
#include "model_lightmap_vert.h"
#include "model_lightmap_normal_map_vert.h"
#include "skin_model_vert.h"
#include "skin_model_shadow_vert.h"
#include "skin_model_normal_map_vert.h"

#include "model_frag.h"
#include "model_shadow_frag.h"
#include "model_normal_map_frag.h"
#include "model_normal_specular_map_frag.h"
#include "model_specular_map_frag.h"
#include "model_lightmap_frag.h"
#include "model_lightmap_normal_map_frag.h"
#include "model_lightmap_normal_specular_map_frag.h"
#include "model_lightmap_specular_map_frag.h"
#include "model_translucent_frag.h"

#include "textured_particle_frag.h"
#include "textured_particle_vert.h"

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"

#include "skybox_vert.h"
#include "skybox_frag.h"

#include "DrawTransformUnitQuad_vert.h"
#include "DrawTexcoordRectTransformUnitQuad_vert.h"
#include "DrawViewportQuadTransformTexcoord_vert.h"
#include "DrawTexture_frag.h"
#include "DrawTextureOpaque_frag.h"
#include "DrawColoredTexture_frag.h"

#include "sdf_text3D_vert.h"
#include "sdf_text3D_frag.h"

#include "paintStroke_vert.h"
#include "paintStroke_frag.h"

#include "polyvox_vert.h"
#include "polyvox_frag.h"

// Create a simple OpenGL window that renders text in various ways
class QTestWindow : public QWindow {
    Q_OBJECT
    QOpenGLContextWrapper _context;

protected:
    void renderText();

public:
    QTestWindow() {
        setSurfaceType(QSurface::OpenGLSurface);
        QSurfaceFormat format = getDefaultOpenGLSurfaceFormat();
        setFormat(format);
        _context.setFormat(format);
        _context.create();

        show();
        makeCurrent();
        gpu::Context::init<gpu::gl::GLBackend>();
        makeCurrent();
        resize(QSize(800, 600));
    }

    virtual ~QTestWindow() {
    }

    void draw();
    void makeCurrent() {
        _context.makeCurrent(this);
    }
};



const std::string VERTEX_SHADER_DEFINES{ R"GLSL(
#version 410 core
#define GPU_VERTEX_SHADER
#define GPU_TRANSFORM_IS_STEREO
#define GPU_TRANSFORM_STEREO_CAMERA
#define GPU_TRANSFORM_STEREO_CAMERA_INSTANCED
#define GPU_TRANSFORM_STEREO_SPLIT_SCREEN
)GLSL" };

const std::string PIXEL_SHADER_DEFINES{ R"GLSL(
#version 410 core
#define GPU_PIXEL_SHADER
#define GPU_TRANSFORM_IS_STEREO
#define GPU_TRANSFORM_STEREO_CAMERA
#define GPU_TRANSFORM_STEREO_CAMERA_INSTANCED
#define GPU_TRANSFORM_STEREO_SPLIT_SCREEN
)GLSL" };

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

void QTestWindow::draw() {
    if (!isVisible()) {
        return;
    }

    makeCurrent();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static std::once_flag once;
    std::call_once(once, [&]{
        testShaderBuild(sdf_text3D_vert, sdf_text3D_frag.h";

        testShaderBuild(DrawTransformUnitQuad_vert, DrawTexture_frag.h";
        testShaderBuild(DrawTexcoordRectTransformUnitQuad_vert, DrawTexture_frag.h";
        testShaderBuild(DrawViewportQuadTransformTexcoord_vert, DrawTexture_frag.h";
        testShaderBuild(DrawTransformUnitQuad_vert, DrawTextureOpaque_frag.h";
        testShaderBuild(DrawTransformUnitQuad_vert, DrawColoredTexture_frag.h";

        testShaderBuild(skybox_vert, skybox_frag.h";
        testShaderBuild(simple_vert, simple_frag.h";
        testShaderBuild(simple_vert, simple_textured_frag.h";
        testShaderBuild(simple_vert, simple_textured_unlit_frag.h";
        testShaderBuild(deferred_light_vert, directional_ambient_light_frag.h";
        testShaderBuild(deferred_light_vert, directional_skybox_light_frag.h";
        testShaderBuild(standardTransformPNTC_vert, standardDrawTexture_frag.h";
        testShaderBuild(standardTransformPNTC_vert, DrawTextureOpaque_frag.h";

        testShaderBuild(model_vert, model_frag.h";
        testShaderBuild(model_normal_map_vert, model_normal_map_frag.h";
        testShaderBuild(model_vert, model_specular_map_frag.h";
        testShaderBuild(model_normal_map_vert, model_normal_specular_map_frag.h";
        testShaderBuild(model_vert, model_translucent_frag.h";
        testShaderBuild(model_normal_map_vert, model_translucent_frag.h";
        testShaderBuild(model_lightmap_vert, model_lightmap_frag.h";
        testShaderBuild(model_lightmap_normal_map_vert, model_lightmap_normal_map_frag.h";
        testShaderBuild(model_lightmap_vert, model_lightmap_specular_map_frag.h";
        testShaderBuild(model_lightmap_normal_map_vert, model_lightmap_normal_specular_map_frag.h";

        testShaderBuild(skin_model_vert, model_frag.h";
        testShaderBuild(skin_model_normal_map_vert, model_normal_map_frag.h";
        testShaderBuild(skin_model_vert, model_specular_map_frag.h";
        testShaderBuild(skin_model_normal_map_vert, model_normal_specular_map_frag.h";
        testShaderBuild(skin_model_vert, model_translucent_frag.h";
        testShaderBuild(skin_model_normal_map_vert, model_translucent_frag.h";

        testShaderBuild(model_shadow_vert, model_shadow_frag.h";
        testShaderBuild(textured_particle_vert, textured_particle_frag.h";
/* FIXME: Bring back the ssao shader tests
        testShaderBuild(gaussian_blur_vertical_vert, gaussian_blur_frag.h";
        testShaderBuild(gaussian_blur_horizontal_vert, gaussian_blur_frag.h";
        testShaderBuild(ambient_occlusion_vert, ambient_occlusion_frag.h";
        testShaderBuild(ambient_occlusion_vert, occlusion_blend_frag.h";
*/

        testShaderBuild(overlay3D_vert, overlay3D_frag.h";

        testShaderBuild(paintStroke_vert,paintStroke_frag.h";
        testShaderBuild(polyvox_vert, polyvox_frag.h";

    });
    _context.swapBuffers(this);
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (!message.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(message.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#else 
        std::cout << message.toLocal8Bit().constData() << std::endl;
#endif
    }
}

const char * LOG_FILTER_RULES = R"V0G0N(
hifi.gpu=true
)V0G0N";

int main(int argc, char** argv) {    
    QGuiApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);
    QTestWindow window;
    QTimer timer;
    timer.setInterval(1);
    app.connect(&timer, &QTimer::timeout, &app, [&] {
        window.draw();
    });
    timer.start();
    app.exec();
    return 0;
}

#include "main.moc"
