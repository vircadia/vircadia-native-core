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
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>

#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>

#include <gpu/Context.h>
#include <gpu/Batch.h>
#include <gpu/Stream.h>
#include <gpu/StandardShaderLib.h>
#include <gpu/GLBackend.h>

// Must come after GL headers
#include <QtGui/QOpenGLContext>

#include <GLMHelpers.h>
#include <PathUtils.h>
#include <GeometryCache.h>
#include <DeferredLightingEffect.h>
#include <NumericalConstants.h>

#include "unlit_frag.h"
#include "unlit_vert.h"

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

uint32_t toCompactColor(const glm::vec4& color);

gpu::ShaderPointer makeShader(const std::string & vertexShaderSrc, const std::string & fragmentShaderSrc, const gpu::Shader::BindingSet & bindings) {
    auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(vertexShaderSrc));
    auto fs = gpu::ShaderPointer(gpu::Shader::createPixel(fragmentShaderSrc));
    auto shader = gpu::ShaderPointer(gpu::Shader::createProgram(vs, fs));
    if (!gpu::Shader::makeProgram(*shader, bindings)) {
        printf("Could not compile shader\n");
        exit(-1);
    }
    return shader;
}

float getSeconds(quint64 start = 0) {
    auto usecs = usecTimestampNow() - start;
    auto msecs = usecs / USECS_PER_MSEC;
    float seconds = (float)msecs / MSECS_PER_SECOND;
    return seconds;
}



// Creates an OpenGL window that renders a simple unlit scene using the gpu library and GeometryCache
// Should eventually get refactored into something that supports multiple gpu backends.
class QTestWindow : public QWindow {
    Q_OBJECT

    QOpenGLContext* _qGlContext{ nullptr };
    QSize _size;
    
    gpu::ContextPointer _context;
    gpu::PipelinePointer _pipeline;
    glm::mat4 _projectionMatrix;
    RateCounter fps;
    QTime _time;
    int _instanceLocation{ -1 };

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
        format.setSwapInterval(0);

        setFormat(format);

        _qGlContext = new QOpenGLContext;
        _qGlContext->setFormat(format);
        _qGlContext->create();

        show();
        makeCurrent();

        gpu::Context::init<gpu::GLBackend>();
        _context = std::make_shared<gpu::Context>();
        
        auto shader = makeShader(unlit_vert, unlit_frag, gpu::Shader::BindingSet{});
        auto state = std::make_shared<gpu::State>();
        state->setMultisampleEnable(true);
        state->setDepthTest(gpu::State::DepthTest { true });
        _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(shader, state));
        _instanceLocation = _pipeline->getProgram()->getUniforms().findLocation("Instanced");
        
        // Clear screen
        gpu::Batch batch;
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 1.0, 0.0, 0.5, 1.0 });
        _context->render(batch);
        
        DependencyManager::set<GeometryCache>();
        DependencyManager::set<DeferredLightingEffect>();

        resize(QSize(800, 600));
        
        _time.start();
    }

    virtual ~QTestWindow() {
    }

    void draw() {
        static auto startTime = usecTimestampNow();

        if (!isVisible()) {
            return;
        }
        makeCurrent();
        
        gpu::Batch batch;
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 0.0f, 0.0f, 0.0f, 1.0f });
        batch.clearDepthFramebuffer(1e4);
        batch.setViewportTransform({ 0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio() });
        batch.setProjectionTransform(_projectionMatrix);
        
        double t = _time.elapsed() * 1e-3;
        glm::vec3 unitscale { 1.0f };
        glm::vec3 up { 0.0f, 1.0f, 0.0f };

        glm::vec3 camera_position { 1.5f * sinf(t), 0.0f, 1.5f * cos(t) };

        static const vec3 camera_focus(0);
        static const vec3 camera_up(0, 1, 0);
        glm::mat4 camera = glm::inverse(glm::lookAt(camera_position, camera_focus, up));
        batch.setViewTransform(camera);
        batch.setPipeline(_pipeline);
        batch.setModelTransform(Transform());

        auto geometryCache = DependencyManager::get<GeometryCache>();
        
        // Render grid on xz plane (not the optimal way to do things, but w/e)
        // Note: GeometryCache::renderGrid will *not* work, as it is apparenly unaffected by batch rotations and renders xy only
        static const std::string GRID_INSTANCE = "Grid";
        static auto compactColor1 = toCompactColor(vec4{ 0.35f, 0.25f, 0.15f, 1.0f });
        static auto compactColor2 = toCompactColor(vec4{ 0.15f, 0.25f, 0.35f, 1.0f });
        auto transformBuffer = batch.getNamedBuffer(GRID_INSTANCE, 0);
        auto colorBuffer = batch.getNamedBuffer(GRID_INSTANCE, 1);
        for (int i = 0; i < 100; ++i) {
            {
                glm::mat4 transform = glm::translate(mat4(), vec3(0, -1, -50 + i));
                transform = glm::scale(transform, vec3(100, 1, 1));
                transformBuffer->append(transform);
                colorBuffer->append(compactColor1);
            }

            {
                glm::mat4 transform = glm::mat4_cast(quat(vec3(0, PI / 2.0f, 0)));
                transform = glm::translate(transform, vec3(0, -1, -50 + i));
                transform = glm::scale(transform, vec3(100, 1, 1));
                transformBuffer->append(transform);
                colorBuffer->append(compactColor2);
            }
        }

        batch.setupNamedCalls(GRID_INSTANCE, 200, [=](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
            batch.setViewTransform(camera);
            batch.setModelTransform(Transform());
            batch.setPipeline(_pipeline);
            auto& xfm = data._buffers[0];
            auto& color = data._buffers[1];
            batch._glUniform1i(_instanceLocation, 1);
            geometryCache->renderWireShapeInstances(batch, GeometryCache::Line, data._count, xfm, color);
            batch._glUniform1i(_instanceLocation, 0);
        });



        // Render unlit cube + sphere

        static GeometryCache::Shape SHAPE[] = {
            GeometryCache::Cube,
            GeometryCache::Sphere,
            GeometryCache::Tetrahedron,
            GeometryCache::Icosahedron,
        };

        static auto startUsecs = usecTimestampNow(); 
        float seconds = getSeconds(startUsecs);
        seconds /= 4.0;
        int shapeIndex = ((int)seconds) % 4;
        bool wire = seconds - floor(seconds) > 0.5f;
        batch.setModelTransform(Transform());
        batch._glColor4f(0.8f, 0.25f, 0.25f, 1.0f);

        if (wire) {
            geometryCache->renderWireShape(batch, SHAPE[shapeIndex]);
        } else {
            geometryCache->renderShape(batch, SHAPE[shapeIndex]);
        }
        
        batch.setModelTransform(Transform().setScale(1.05f));
        batch._glColor4f(1, 1, 1, 1);
        geometryCache->renderWireCube(batch);

        _context->render(batch);
        _qGlContext->swapBuffers(this);
        
        fps.increment();
        if (fps.elapsed() >= 0.5f) {
            qDebug() << "FPS: " << fps.rate();
            fps.reset();
        }
    }
    
    void makeCurrent() {
        _qGlContext->makeCurrent(this);
    }

protected:
    void resizeEvent(QResizeEvent* ev) override {
        resizeWindow(ev->size());
        
        float fov_degrees = 60.0f;
        float aspect_ratio = (float)_size.width() / _size.height();
        float near_clip = 0.1f;
        float far_clip = 1000.0f;
        _projectionMatrix = glm::perspective(glm::radians(fov_degrees), aspect_ratio, near_clip, far_clip);
    }
};

int main(int argc, char** argv) {    
    QGuiApplication app(argc, argv);
    QTestWindow window;
    QTimer timer;
    timer.setInterval(0);
    app.connect(&timer, &QTimer::timeout, &app, [&] {
        window.draw();
    });
    timer.start();
    app.exec();
    return 0;
}

#include "main.moc"
