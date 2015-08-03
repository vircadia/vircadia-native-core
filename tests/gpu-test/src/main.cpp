//
//  main.cpp
//  tests/gpu-test/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QLoggingCategory>

#include <gpu/Context.h>
#include <gpu/Batch.h>
#include <gpu/Stream.h>
#include <gpu/StandardShaderLib.h>
//#include <shared/ViewFrustrum.h>

#include <gpu/GLBackend.h>

//#include <QOpenGLBuffer>
#include <QOpenGLContext>
//#include <QOpenGLDebugLogger>
//#include <QOpenGLShaderProgram>
//#include <QOpenGLTexture>
//#include <QOpenGLVertexArrayObject>
#include <QResizeEvent>
#include <QTime>
#include <QTimer>
#include <QWindow>


#include <cstdio>


#include <PathUtils.h>

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
            return NAN;
        }
        return (float) count() / elapsed();
    }
};

const char * basicVS =
" varying vec3 pos;         "
" void main(void) {         "
"     gl_Position.xyz = pos;    "
" } ";
const char * basicFS =
" void main(void) {         "
"   gl_FragColor.xyz = vec3(0.7, 0.2, 0.5); "
" } ";

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

    QOpenGLContext* _qGlContext{ nullptr };
    QSize _size;
    
    gpu::ContextPointer _context;
    gpu::PipelinePointer _pipeline;
    gpu::BufferPointer _buffer;
    gpu::Stream::FormatPointer _format;
    
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
        format.setVersion(4, 5);
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
        format.setOption(QSurfaceFormat::DebugContext);

        setFormat(format);

        _qGlContext = new QOpenGLContext;
        _qGlContext->setFormat(format);
        _qGlContext->create();

        show();
        makeCurrent();

        gpu::Context::init<gpu::GLBackend>();
        _context = std::make_shared<gpu::Context>();
        
        // Clear screen
        gpu::Batch batch;
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 1.0, 0.0, 0.5, 1.0 });
        _context->render(batch);
        
        // Create default shaders
        
        std::string vsSource (basicVS);
        std::string fsSource (basicFS);
        
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(vsSource));
        auto fs = gpu::ShaderPointer(gpu::Shader::createPixel(fsSource));
        auto shader = gpu::ShaderPointer(gpu::Shader::createProgram(vs, fs));
        
        gpu::Shader::BindingSet bindings;
        if (!gpu::Shader::makeProgram(*shader, bindings)) {
            printf("Could not compile shader");
            exit(-1);
        }
        
//        auto shader = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        
        auto state = std::make_shared<gpu::State>();
        _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(shader, state));
        
        float z = 10.0f;
        const glm::vec3 vertices[] = {
            { -1.0f, 1.0f, z },
            { 1.0f, -1.0f, z },
            { -1.0f, -1.0f, z }
        };
        _buffer = std::make_shared<gpu::Buffer>(sizeof(vertices), (const gpu::Byte*)vertices);
        _format = std::make_shared<gpu::Stream::Format>();
        _format->setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ));
        
        
//        {
//            QOpenGLDebugLogger* logger = new QOpenGLDebugLogger(this);
//            logger->initialize(); // initializes in the current context, i.e. ctx
//            logger->enableMessages();
//            connect(logger, &QOpenGLDebugLogger::messageLogged, this, [&](const QOpenGLDebugMessage & debugMessage) {
//                qDebug() << debugMessage;
//            });
//            //        logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
//        }
//        qDebug() << (const char*)glGetString(GL_VERSION);

        //_textRenderer[0] = TextRenderer::getInstance(SANS_FONT_FAMILY, 12, false);
        //_textRenderer[1] = TextRenderer::getInstance(SERIF_FONT_FAMILY, 12, false,
        //    TextRenderer::SHADOW_EFFECT);
        //_textRenderer[2] = TextRenderer::getInstance(MONO_FONT_FAMILY, 48, -1,
        //    false, TextRenderer::OUTLINE_EFFECT);
        //_textRenderer[3] = TextRenderer::getInstance(INCONSOLATA_FONT_FAMILY, 24);

//        glEnable(GL_BLEND);
//        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//        glClearColor(0.2f, 0.2f, 0.2f, 1);
//        glDisable(GL_DEPTH_TEST);

        makeCurrent();
        
        _context->syncCache();

        setFramePosition(QPoint(-1000, 0));
        resize(QSize(800, 600));
    }

    virtual ~QTestWindow() {
    }

    void draw();
    void makeCurrent() {
        _qGlContext->makeCurrent(this);
    }

protected:

    void resizeEvent(QResizeEvent* ev) override {
        resizeWindow(ev->size());
    }
};

#ifndef SERIF_FONT_FAMILY
#define SERIF_FONT_FAMILY "Times New Roman"
#endif

//static const wchar_t* EXAMPLE_TEXT = L"Hello";
//static const wchar_t* EXAMPLE_TEXT = L"\xC1y Hello 1.0\ny\xC1 line 2\n\xC1y";
static const glm::uvec2 QUAD_OFFSET(10, 10);

static const glm::vec3 COLORS[4] = {
    { 1.0, 1.0, 1.0 },
    { 0.5, 1.0, 0.5 },
    { 1.0, 0.5, 0.5 },
    { 0.5, 0.5, 1.0 }
};


void QTestWindow::draw() {
    if (!isVisible()) {
        return;
    }

    makeCurrent();
    
    gpu::Batch batch;
    static int frameNum = 0;
    frameNum++;
    batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 1.0, float(frameNum % 60)/60.0f, 0.5, 1.0 });
//    _context->render(batch);
    
    
////    batch.clear();
//    batch.setViewportTransform({ 0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio() });
//    
    glm::quat cubeOrientation;
//
    batch.setViewTransform(Transform());
    batch.setProjectionTransform(glm::mat4());
//    batch.setProjectionTransform(_viewFrustrum->getProjection());
    batch.setModelTransform(Transform().setRotation(cubeOrientation));
    batch.setPipeline(_pipeline);
    batch.setInputBuffer(gpu::Stream::POSITION, _buffer, 0, 3);
    batch.setInputFormat(_format);
    batch.draw(gpu::TRIANGLES, 3);
    _context->render(batch);
//
////    gpu::Stream::Format format;
////    format.setAttribute(gpu::Stream::POSITION, gpu::Stream::POSITION, gpu::Element(gpu::Vec3, gpu::FLOAT, gpu::XYZ));
////    batch.setInputBuffer(gpu::Stream::POSITION, _trianglePosBuffer, )
//
//    
////    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
////    glViewport(0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio());
//
////    _context->swapBuffers(this);
////    glFinish();
//    
    _qGlContext->swapBuffers(this);

    fps.increment();
    if (fps.elapsed() >= 2.0f) {
        qDebug() << "FPS: " << fps.rate();  // This prints out the frames per 2 secs (ie. half of the actual fps)  bug...?
        fps.reset();
    }
}

int main(int argc, char** argv) {    
    QGuiApplication app(argc, argv);
    QTestWindow window;
    QTimer timer;
    timer.setInterval(1000 / 120.0f);
    app.connect(&timer, &QTimer::timeout, &app, [&] {
        window.draw();
    });
    timer.start();
    app.exec();
    return 0;
}

#include "main.moc"
