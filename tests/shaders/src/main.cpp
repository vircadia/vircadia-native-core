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

#include <gl/QOpenGLContextWrapper.h>

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

#include <entities-renderer/textured_particle_frag.h>
#include <entities-renderer/textured_particle_vert.h>

#include <render-utils/overlay3D_vert.h>
#include <render-utils/overlay3D_frag.h>

#include <model/skybox_vert.h>
#include <model/skybox_frag.h>

#include <gpu/DrawTransformUnitQuad_vert.h>
#include <gpu/DrawTexcoordRectTransformUnitQuad_vert.h>
#include <gpu/DrawViewportQuadTransformTexcoord_vert.h>
#include <gpu/DrawTexture_frag.h>
#include <gpu/DrawTextureOpaque_frag.h>
#include <gpu/DrawColoredTexture_frag.h>

#include <render-utils/sdf_text3D_vert.h>
#include <render-utils/sdf_text3D_frag.h>

#include <entities-renderer/paintStroke_vert.h>
#include <entities-renderer/paintStroke_frag.h>

#include <entities-renderer/polyvox_vert.h>
#include <entities-renderer/polyvox_frag.h>

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

        testShaderBuild(skybox_vert, skybox_frag);
        testShaderBuild(simple_vert, simple_frag);
        testShaderBuild(simple_vert, simple_textured_frag);
        testShaderBuild(simple_vert, simple_textured_unlit_frag);
        testShaderBuild(deferred_light_vert, directional_ambient_light_frag);
        testShaderBuild(deferred_light_vert, directional_skybox_light_frag);
        testShaderBuild(standardTransformPNTC_vert, standardDrawTexture_frag);
        testShaderBuild(standardTransformPNTC_vert, DrawTextureOpaque_frag);

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
        testShaderBuild(textured_particle_vert, textured_particle_frag);
/* FIXME: Bring back the ssao shader tests
        testShaderBuild(gaussian_blur_vertical_vert, gaussian_blur_frag);
        testShaderBuild(gaussian_blur_horizontal_vert, gaussian_blur_frag);
        testShaderBuild(ambient_occlusion_vert, ambient_occlusion_frag);
        testShaderBuild(ambient_occlusion_vert, occlusion_blend_frag);
*/

        testShaderBuild(overlay3D_vert, overlay3D_frag);

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
