//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <mutex>

#include <gpu/GLBackend.h>

#include <QLoggingCategory>
#include <QResizeEvent>
#include <QTimer>
#include <QWindow>
#include <QElapsedTimer>
#include <QDir>
#include <QGuiApplication>

#include <gl/GLHelpers.h>

#include <gl/QOpenGLDebugLoggerWrapper.h>
#include <gl/QOpenGLContextWrapper.h>

#include "../model/Skybox_vert.h"
#include "../model/Skybox_frag.h"

#include "simple_vert.h"
#include "simple_frag.h"
#include "simple_textured_frag.h"
#include "simple_textured_emisive_frag.h"

#include "deferred_light_vert.h"
#include "deferred_light_limited_vert.h"

#include "directional_light_frag.h"

#include "directional_ambient_light_frag.h"

#include "directional_skybox_light_frag.h"

#include "point_light_frag.h"
#include "spot_light_frag.h"

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

#include "untextured_particle_frag.h"
#include "untextured_particle_vert.h"
#include "textured_particle_frag.h"
#include "textured_particle_vert.h"


#include "hit_effect_vert.h"
#include "hit_effect_frag.h"

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"

#include "Skybox_vert.h"
#include "Skybox_frag.h"

#include "stars_vert.h"
#include "stars_frag.h"
#include "starsGrid_frag.h"

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

        gpu::Context::init<gpu::GLBackend>();
        setupDebugLogger(this);
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

void testShaderBuild(const char* vs_src, const char * fs_src) {
    auto vs = gpu::Shader::createVertex(std::string(vs_src));
    auto fs = gpu::Shader::createPixel(std::string(fs_src));
    auto pr = gpu::Shader::createProgram(vs, fs);
    if (!gpu::Shader::makeProgram(*pr)) {
        throw std::runtime_error("Failed to compile shader");
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
        testShaderBuild(sdf_text3D_vert, sdf_text3D_frag);

        testShaderBuild(DrawTransformUnitQuad_vert, DrawTexture_frag);
        testShaderBuild(DrawTexcoordRectTransformUnitQuad_vert, DrawTexture_frag);
        testShaderBuild(DrawViewportQuadTransformTexcoord_vert, DrawTexture_frag);
        testShaderBuild(DrawTransformUnitQuad_vert, DrawTextureOpaque_frag);
        testShaderBuild(DrawTransformUnitQuad_vert, DrawColoredTexture_frag);

        testShaderBuild(Skybox_vert, Skybox_frag);
        testShaderBuild(simple_vert, simple_frag);
        testShaderBuild(simple_vert, simple_textured_frag);
        testShaderBuild(simple_vert, simple_textured_emisive_frag);
        testShaderBuild(deferred_light_vert, directional_light_frag);
        testShaderBuild(deferred_light_vert, directional_ambient_light_frag);
        testShaderBuild(deferred_light_vert, directional_skybox_light_frag);
        testShaderBuild(deferred_light_limited_vert, point_light_frag);
        testShaderBuild(deferred_light_limited_vert, spot_light_frag);
        testShaderBuild(standardTransformPNTC_vert, standardDrawTexture_frag);
        testShaderBuild(standardTransformPNTC_vert, DrawTextureOpaque_frag);
        
        testShaderBuild(standardTransformPNTC_vert, starsGrid_frag);
        testShaderBuild(stars_vert, stars_frag);

        testShaderBuild(model_vert, model_frag);
        testShaderBuild(model_normal_map_vert, model_normal_map_frag);
        testShaderBuild(model_vert, model_specular_map_frag);
        testShaderBuild(model_normal_map_vert, model_normal_specular_map_frag);
        testShaderBuild(model_vert, model_translucent_frag);
        testShaderBuild(model_normal_map_vert, model_translucent_frag);
        testShaderBuild(model_lightmap_vert, model_lightmap_frag);
        testShaderBuild(model_lightmap_normal_map_vert, model_lightmap_normal_map_frag);
        testShaderBuild(model_lightmap_vert, model_lightmap_specular_map_frag);
        testShaderBuild(model_lightmap_normal_map_vert, model_lightmap_normal_specular_map_frag);

        testShaderBuild(skin_model_vert, model_frag);
        testShaderBuild(skin_model_normal_map_vert, model_normal_map_frag);
        testShaderBuild(skin_model_vert, model_specular_map_frag);
        testShaderBuild(skin_model_normal_map_vert, model_normal_specular_map_frag);
        testShaderBuild(skin_model_vert, model_translucent_frag);
        testShaderBuild(skin_model_normal_map_vert, model_translucent_frag);

        testShaderBuild(model_shadow_vert, model_shadow_frag);
        testShaderBuild(untextured_particle_vert, untextured_particle_frag);
        testShaderBuild(textured_particle_vert, textured_particle_frag);
/* FIXME: Bring back the ssao shader tests
        testShaderBuild(gaussian_blur_vertical_vert, gaussian_blur_frag);
        testShaderBuild(gaussian_blur_horizontal_vert, gaussian_blur_frag);
        testShaderBuild(ambient_occlusion_vert, ambient_occlusion_frag);
        testShaderBuild(ambient_occlusion_vert, occlusion_blend_frag);
*/
        testShaderBuild(hit_effect_vert, hit_effect_frag);

        testShaderBuild(overlay3D_vert, overlay3D_frag);

        testShaderBuild(Skybox_vert, Skybox_frag);
        
        testShaderBuild(paintStroke_vert,paintStroke_frag);
        testShaderBuild(polyvox_vert, polyvox_frag);

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
