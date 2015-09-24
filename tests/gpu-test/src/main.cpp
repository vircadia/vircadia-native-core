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
#include <QtGui/QOpenGLDebugLogger>

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

static const size_t TYPE_COUNT = 4;
static GeometryCache::Shape SHAPE[TYPE_COUNT] = {
    GeometryCache::Icosahedron,
    GeometryCache::Cube,
    GeometryCache::Sphere,
    GeometryCache::Tetrahedron,
    //GeometryCache::Line,
};

gpu::Stream::FormatPointer& getInstancedSolidStreamFormat();

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
        format.setVersion(4, 3);
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
        format.setOption(QSurfaceFormat::DebugContext);
        format.setSwapInterval(0);

        setFormat(format);

        _qGlContext = new QOpenGLContext;
        _qGlContext->setFormat(format);
        _qGlContext->create();

        show();
        makeCurrent();
        QOpenGLDebugLogger* logger = new QOpenGLDebugLogger(this);
        logger->initialize(); // initializes in the current context, i.e. ctx
        connect(logger, &QOpenGLDebugLogger::messageLogged, [](const QOpenGLDebugMessage& message){
            qDebug() << message;
        });
        logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);


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
        // Attempting to draw before we're visible and have a valid size will
        // produce GL errors.
        if (!isVisible() || _size.width() <= 0 || _size.height() <= 0) {
            return;
        }
        makeCurrent();
        
        gpu::Batch batch;
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 0.0f, 0.0f, 0.0f, 1.0f });
        batch.clearDepthFramebuffer(1e4);
        batch.setViewportTransform({ 0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio() });
        batch.setProjectionTransform(_projectionMatrix);
        
        float t = _time.elapsed() * 1e-3f;
        glm::vec3 unitscale { 1.0f };
        glm::vec3 up { 0.0f, 1.0f, 0.0f };

        float distance = 3.0f;
        glm::vec3 camera_position{ distance * sinf(t), 0.0f, distance * cosf(t) };

        static const vec3 camera_focus(0);
        static const vec3 camera_up(0, 1, 0);
        glm::mat4 camera = glm::inverse(glm::lookAt(camera_position, camera_focus, up));
        batch.setViewTransform(camera);
        batch.setPipeline(_pipeline);
        batch.setModelTransform(Transform());

        auto geometryCache = DependencyManager::get<GeometryCache>();

        // Render grid on xz plane (not the optimal way to do things, but w/e)
        // Note: GeometryCache::renderGrid will *not* work, as it is apparenly unaffected by batch rotations and renders xy only
        {
            static const std::string GRID_INSTANCE = "Grid";
            static auto compactColor1 = toCompactColor(vec4{ 0.35f, 0.25f, 0.15f, 1.0f });
            static auto compactColor2 = toCompactColor(vec4{ 0.15f, 0.25f, 0.35f, 1.0f });
            static gpu::BufferPointer transformBuffer; 
            static gpu::BufferPointer colorBuffer;
            if (!transformBuffer) {
                transformBuffer = std::make_shared<gpu::Buffer>();
                colorBuffer = std::make_shared<gpu::Buffer>();
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
            }
            
            batch.setupNamedCalls(GRID_INSTANCE, 200, [=](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
                batch.setViewTransform(camera);
                batch.setModelTransform(Transform());
                batch.setPipeline(_pipeline);
                batch._glUniform1i(_instanceLocation, 1);
                geometryCache->renderWireShapeInstances(batch, GeometryCache::Line, data._count, transformBuffer, colorBuffer);
                batch._glUniform1i(_instanceLocation, 0);
            });
        }

        {
            static const size_t ITEM_COUNT = 1000;
            static const float SHAPE_INTERVAL = (PI * 2.0f) / ITEM_COUNT;
            static const float ITEM_INTERVAL = SHAPE_INTERVAL / TYPE_COUNT;

            static const gpu::Element POSITION_ELEMENT{ gpu::VEC3, gpu::FLOAT, gpu::XYZ };
            static const gpu::Element NORMAL_ELEMENT{ gpu::VEC3, gpu::FLOAT, gpu::XYZ };
            static const gpu::Element COLOR_ELEMENT{ gpu::VEC4, gpu::NUINT8, gpu::RGBA };
            static const gpu::Element TRANSFORM_ELEMENT{ gpu::MAT4, gpu::FLOAT, gpu::XYZW };


            static std::vector<Transform> transforms;
            static std::vector<vec4> colors;
            static gpu::BufferPointer indirectBuffer;
            static gpu::BufferPointer transformBuffer;
            static gpu::BufferPointer colorBuffer;
            static gpu::BufferView colorView; 
            static gpu::BufferView instanceXfmView; 

            if (!transformBuffer) {
                transformBuffer = std::make_shared<gpu::Buffer>();
                colorBuffer = std::make_shared<gpu::Buffer>();
                indirectBuffer = std::make_shared<gpu::Buffer>();

                static const float ITEM_RADIUS = 20;
                static const vec3 ITEM_TRANSLATION{ 0, 0, -ITEM_RADIUS };
                for (size_t i = 0; i < TYPE_COUNT; ++i) {
                    GeometryCache::Shape shape = SHAPE[i];
                    GeometryCache::ShapeData shapeData = geometryCache->_shapes[shape];
                    {
                        gpu::Batch::DrawIndexedIndirectCommand indirectCommand;
                        indirectCommand._count = shapeData._indexCount;
                        indirectCommand._instanceCount = ITEM_COUNT;
                        indirectCommand._baseInstance = i * ITEM_COUNT;
                        indirectCommand._firstIndex = shapeData._indexOffset / 2;
                        indirectCommand._baseVertex = 0;
                        indirectBuffer->append(indirectCommand);
                    }

                    //indirectCommand._count
                    float startingInterval = ITEM_INTERVAL * i;
                    for (size_t j = 0; j < ITEM_COUNT; ++j) {
                        float theta = j * SHAPE_INTERVAL + startingInterval;
                        auto transform = glm::rotate(mat4(), theta, Vectors::UP);
                        transform = glm::rotate(transform, (randFloat() - 0.5f) * PI / 4.0f, Vectors::UNIT_X);
                        transform = glm::translate(transform, ITEM_TRANSLATION);
                        transform = glm::scale(transform, vec3(randFloat() / 2.0f + 0.5f));
                        transformBuffer->append(transform);
                        transforms.push_back(transform);
                        auto color = vec4{ randomColorValue(64), randomColorValue(64), randomColorValue(64), 255 };
                        color /= 255.0f;
                        colors.push_back(color);
                        colorBuffer->append(toCompactColor(color));
                    }
                }
                colorView = gpu::BufferView(colorBuffer, COLOR_ELEMENT);
                instanceXfmView = gpu::BufferView(transformBuffer, TRANSFORM_ELEMENT);
            }

#if 1
            GeometryCache::ShapeData shapeData = geometryCache->_shapes[GeometryCache::Icosahedron];
            {
                batch.setViewTransform(camera);
                batch.setModelTransform(Transform());
                batch.setPipeline(_pipeline);
                batch._glUniform1i(_instanceLocation, 1);
                batch.setInputFormat(getInstancedSolidStreamFormat());
                batch.setInputBuffer(gpu::Stream::COLOR, colorView);
                batch.setInputBuffer(gpu::Stream::INSTANCE_XFM, instanceXfmView);
                batch.setIndirectBuffer(indirectBuffer);
                shapeData.setupBatch(batch);
                batch.multiDrawIndexedIndirect(TYPE_COUNT, gpu::TRIANGLES);
                batch._glUniform1i(_instanceLocation, 0);
            }
#else
            batch.setViewTransform(camera);
            batch.setPipeline(_pipeline);
            for (size_t i = 0; i < TYPE_COUNT; ++i) {
                GeometryCache::Shape shape = SHAPE[i];
                for (size_t j = 0; j < ITEM_COUNT; ++j) {
                    int index = i * ITEM_COUNT + j;
                    batch.setModelTransform(transforms[index]);
                    const vec4& color = colors[index];
                    batch._glColor4f(color.r, color.g, color.b, 1.0);
                    geometryCache->renderShape(batch, shape);
                }
            }
#endif
        }

        // Render unlit cube + sphere
        static auto startUsecs = usecTimestampNow(); 
        float seconds = getSeconds(startUsecs);

        seconds /= 4.0f;
        int shapeIndex = ((int)seconds) % TYPE_COUNT;
        bool wire = (seconds - floorf(seconds) > 0.5f);
        batch.setModelTransform(Transform());
        batch._glColor4f(0.8f, 0.25f, 0.25f, 1.0f);

        if (wire) {
            geometryCache->renderWireShape(batch, SHAPE[shapeIndex]);
        } else {
            geometryCache->renderShape(batch, SHAPE[shapeIndex]);
        }
        
        batch.setModelTransform(Transform().setScale(2.05f));
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

