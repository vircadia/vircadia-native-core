//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <mutex>

#include <gpu/GLBackend.h>

#include <QOpenGLDebugLogger>

#include <QLoggingCategory>
#include <QResizeEvent>
#include <QTimer>
#include <QWindow>
#include <QElapsedTimer>
#include <QDir>
#include <QGuiApplication>

#include "../model/Skybox_vert.h"
#include "../model/Skybox_frag.h"

#include "simple_vert.h"
#include "simple_frag.h"
#include "simple_textured_frag.h"
#include "simple_textured_emisive_frag.h"

#include "deferred_light_vert.h"
#include "deferred_light_limited_vert.h"

#include "directional_light_frag.h"
#include "directional_light_shadow_map_frag.h"
#include "directional_light_cascaded_shadow_map_frag.h"

#include "directional_ambient_light_frag.h"
#include "directional_ambient_light_shadow_map_frag.h"
#include "directional_ambient_light_cascaded_shadow_map_frag.h"

#include "directional_skybox_light_frag.h"
#include "directional_skybox_light_shadow_map_frag.h"
#include "directional_skybox_light_cascaded_shadow_map_frag.h"

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

#include "ambient_occlusion_vert.h"
#include "ambient_occlusion_frag.h"
#include "gaussian_blur_vertical_vert.h"
#include "gaussian_blur_horizontal_vert.h"
#include "gaussian_blur_frag.h"
#include "occlusion_blend_frag.h"

#include "hit_effect_vert.h"
#include "hit_effect_frag.h"

#include "overlay3D_vert.h"
#include "overlay3D_frag.h"

#include "SkyFromSpace_vert.h"
#include "SkyFromSpace_frag.h"
#include "SkyFromAtmosphere_vert.h"
#include "SkyFromAtmosphere_frag.h"

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

class RateCounter {
    std::vector<float> times;
    QElapsedTimer timer;
public:
    RateCounter() {
        timer.start();
    }

    void reset() {
        times.clear();
    }

    unsigned int count() const {
        return times.size() - 1;
    }

    float elapsed() const {
        if (times.size() < 1) {
            return 0.0f;
        }
        float elapsed = *times.rbegin() - *times.begin();
        return elapsed;
    }

    void increment() {
        times.push_back(timer.elapsed() / 1000.0f);
    }

    float rate() const {
        if (elapsed() == 0.0f) {
            return 0.0f;
        }
        return (float) count() / elapsed();
    }
};


const QString& getQmlDir() {
    static QString dir;
    if (dir.isEmpty()) {
        QDir path(__FILE__);
        path.cdUp();
        dir = path.cleanPath(path.absoluteFilePath("../../../interface/resources/qml/")) + "/";
        qDebug() << "Qml Path: " << dir;
    }
    return dir;
}

// Create a simple OpenGL window that renders text in various ways
class QTestWindow : public QWindow {
    Q_OBJECT

    QOpenGLContext* _context{ nullptr };
    QSize _size;
    //TextRenderer* _textRenderer[4];
    RateCounter fps;

protected:
    void renderText();

private:
    void resizeWindow(const QSize& size) {
        _size = size;
    }

public:
    QTestWindow() {
        setSurfaceType(QSurface::OpenGLSurface);

        QSurfaceFormat format;
        // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
        format.setDepthBufferSize(16);
        format.setStencilBufferSize(8);
        format.setVersion(4, 1);
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
        format.setOption(QSurfaceFormat::DebugContext);

        setFormat(format);

        _context = new QOpenGLContext;
        _context->setFormat(format);
        _context->create();

        show();
        makeCurrent();

        gpu::Context::init<gpu::GLBackend>();


        {
            QOpenGLDebugLogger* logger = new QOpenGLDebugLogger(this);
            logger->initialize(); // initializes in the current context, i.e. ctx
            logger->enableMessages();
            connect(logger, &QOpenGLDebugLogger::messageLogged, this, [&](const QOpenGLDebugMessage & debugMessage) {
                qDebug() << debugMessage;
            });
            //        logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
        }
        qDebug() << (const char*)glGetString(GL_VERSION);

        //_textRenderer[0] = TextRenderer::getInstance(SANS_FONT_FAMILY, 12, false);
        //_textRenderer[1] = TextRenderer::getInstance(SERIF_FONT_FAMILY, 12, false,
        //    TextRenderer::SHADOW_EFFECT);
        //_textRenderer[2] = TextRenderer::getInstance(MONO_FONT_FAMILY, 48, -1,
        //    false, TextRenderer::OUTLINE_EFFECT);
        //_textRenderer[3] = TextRenderer::getInstance(INCONSOLATA_FONT_FAMILY, 24);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.2f, 0.2f, 0.2f, 1);
        glDisable(GL_DEPTH_TEST);

        makeCurrent();

//        setFramePosition(QPoint(-1000, 0));
        resize(QSize(800, 600));
    }

    virtual ~QTestWindow() {
    }

    void draw();
    void makeCurrent() {
        _context->makeCurrent(this);
    }

protected:
    void resizeEvent(QResizeEvent* ev) override {
        resizeWindow(ev->size());
    }
};

void testShaderBuild(const char* vs_src, const char * fs_src) {
    auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(vs_src)));
    auto fs = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(fs_src)));
    auto pr = gpu::ShaderPointer(gpu::Shader::createProgram(vs, fs));
    gpu::Shader::makeProgram(*pr);
}

void QTestWindow::draw() {
    if (!isVisible()) {
        return;
    }

    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio());

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
        testShaderBuild(deferred_light_vert, directional_light_shadow_map_frag);
        testShaderBuild(deferred_light_vert, directional_light_cascaded_shadow_map_frag);
        testShaderBuild(deferred_light_vert, directional_ambient_light_frag);
        testShaderBuild(deferred_light_vert, directional_ambient_light_shadow_map_frag);
        testShaderBuild(deferred_light_vert, directional_ambient_light_cascaded_shadow_map_frag);
        testShaderBuild(deferred_light_vert, directional_skybox_light_frag);
        testShaderBuild(deferred_light_vert, directional_skybox_light_shadow_map_frag);
        testShaderBuild(deferred_light_vert, directional_skybox_light_cascaded_shadow_map_frag);
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

        testShaderBuild(gaussian_blur_vertical_vert, gaussian_blur_frag);
        testShaderBuild(gaussian_blur_horizontal_vert, gaussian_blur_frag);
        testShaderBuild(ambient_occlusion_vert, ambient_occlusion_frag);
        testShaderBuild(ambient_occlusion_vert, occlusion_blend_frag);
        
        testShaderBuild(hit_effect_vert, hit_effect_frag);

        testShaderBuild(overlay3D_vert, overlay3D_frag);

        testShaderBuild(SkyFromSpace_vert, SkyFromSpace_frag);
        testShaderBuild(SkyFromAtmosphere_vert, SkyFromAtmosphere_frag);
        
        testShaderBuild(Skybox_vert, Skybox_frag);
        
        testShaderBuild(paintStroke_vert,paintStroke_frag);

    });

    _context->swapBuffers(this);
    glFinish();

    fps.increment();
    if (fps.elapsed() >= 2.0f) {
        qDebug() << "FPS: " << fps.rate();
        fps.reset();
    }
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
