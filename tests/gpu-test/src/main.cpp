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
#include <gpu/gl/GLBackend.h>

#include <gl/QOpenGLContextWrapper.h>
#include <gl/QOpenGLDebugLoggerWrapper.h>
#include <gl/GLHelpers.h>

#include <GLMHelpers.h>
#include <PathUtils.h>
#include <NumericalConstants.h>

#include <GeometryCache.h>
#include <DeferredLightingEffect.h>
#include <TextureCache.h>

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
        return (unsigned int)times.size() - 1;
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


const char* VERTEX_SHADER = R"SHADER(

layout(location = 0) in vec4 inPosition;
layout(location = 3) in vec2 inTexCoord0;

struct TransformObject {
    mat4 _model;
    mat4 _modelInverse;
};

layout(location=15) in ivec2 _drawCallInfo;

uniform samplerBuffer transformObjectBuffer;

TransformObject getTransformObject() {
    int offset = 8 * _drawCallInfo.x;
    TransformObject object;
    object._model[0] = texelFetch(transformObjectBuffer, offset);
    object._model[1] = texelFetch(transformObjectBuffer, offset + 1);
    object._model[2] = texelFetch(transformObjectBuffer, offset + 2);
    object._model[3] = texelFetch(transformObjectBuffer, offset + 3);

    object._modelInverse[0] = texelFetch(transformObjectBuffer, offset + 4);
    object._modelInverse[1] = texelFetch(transformObjectBuffer, offset + 5);
    object._modelInverse[2] = texelFetch(transformObjectBuffer, offset + 6);
    object._modelInverse[3] = texelFetch(transformObjectBuffer, offset + 7);

    return object;
}

struct TransformCamera {
    mat4 _view;
    mat4 _viewInverse;
    mat4 _projectionViewUntranslated;
    mat4 _projection;
    mat4 _projectionInverse;
    vec4 _viewport;
};

layout(std140) uniform transformCameraBuffer {
    TransformCamera _camera;
};

TransformCamera getTransformCamera() {
    return _camera;
}

// the interpolated normal
out vec2 _texCoord0;

void main(void) {
    _texCoord0 = inTexCoord0.st;

    // standard transform
    TransformCamera cam = getTransformCamera();
    TransformObject obj = getTransformObject();
    { // transformModelToClipPos
        vec4 eyeWAPos;
        { // _transformModelToEyeWorldAlignedPos
            highp mat4 _mv = obj._model;
            _mv[3].xyz -= cam._viewInverse[3].xyz;
            highp vec4 _eyeWApos = (_mv * inPosition);
            eyeWAPos = _eyeWApos;
        }
        gl_Position = cam._projectionViewUntranslated * eyeWAPos;
    }

})SHADER";

const char* FRAGMENT_SHADER = R"SHADER(

uniform sampler2D originalTexture;

in vec2 _texCoord0;

layout(location = 0) out vec4 _fragColor0;

void main(void) {
    //_fragColor0 = vec4(_texCoord0, 0.0, 1.0);
    _fragColor0 = texture(originalTexture, _texCoord0);
}
)SHADER";


gpu::ShaderPointer makeShader(const std::string & vertexShaderSrc, const std::string & fragmentShaderSrc, const gpu::Shader::BindingSet & bindings) {
    auto vs = gpu::Shader::createVertex(vertexShaderSrc);
    auto fs = gpu::Shader::createPixel(fragmentShaderSrc);
    auto shader = gpu::Shader::createProgram(vs, fs);
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

    QOpenGLContextWrapper _qGlContext;
    QSize _size;
    
    gpu::ContextPointer _context;
    gpu::PipelinePointer _pipeline;
    glm::mat4 _projectionMatrix;
    RateCounter fps;
    QTime _time;
    glm::mat4 _camera;

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
        setGLFormatVersion(format);
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
        format.setOption(QSurfaceFormat::DebugContext);
        //format.setSwapInterval(0);

        setFormat(format);

        _qGlContext.setFormat(format);
        _qGlContext.create();

        show();
        makeCurrent();
        setupDebugLogger(this);

        gpu::Context::init<gpu::gl::GLBackend>();
        _context = std::make_shared<gpu::Context>();
        makeCurrent();
        auto shader = makeShader(unlit_vert, unlit_frag, gpu::Shader::BindingSet{});
        auto state = std::make_shared<gpu::State>();
        state->setMultisampleEnable(true);
        state->setDepthTest(gpu::State::DepthTest { true });
        _pipeline = gpu::Pipeline::create(shader, state);
        


        // Clear screen
        gpu::Batch batch;
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 1.0, 0.0, 0.5, 1.0 });
        _context->render(batch);
        
        DependencyManager::set<GeometryCache>();
        DependencyManager::set<TextureCache>();
        DependencyManager::set<DeferredLightingEffect>();

        resize(QSize(800, 600));
        
        _time.start();
    }

    virtual ~QTestWindow() {
    }

    void updateCamera() {
        float t = _time.elapsed() * 1e-4f;
        glm::vec3 unitscale { 1.0f };
        glm::vec3 up { 0.0f, 1.0f, 0.0f };

        float distance = 3.0f;
        glm::vec3 camera_position { distance * sinf(t), 0.5f, distance * cosf(t) };

        static const vec3 camera_focus(0);
        static const vec3 camera_up(0, 1, 0);
        _camera = glm::inverse(glm::lookAt(camera_position, camera_focus, up));
    }


    void drawFloorGrid(gpu::Batch& batch) {
        auto geometryCache = DependencyManager::get<GeometryCache>();
        // Render grid on xz plane (not the optimal way to do things, but w/e)
        // Note: GeometryCache::renderGrid will *not* work, as it is apparenly unaffected by batch rotations and renders xy only
        static const std::string GRID_INSTANCE = "Grid";
        static auto compactColor1 = toCompactColor(vec4 { 0.35f, 0.25f, 0.15f, 1.0f });
        static auto compactColor2 = toCompactColor(vec4 { 0.15f, 0.25f, 0.35f, 1.0f });
        static std::vector<glm::mat4> transforms;
        static gpu::BufferPointer colorBuffer;
        if (!transforms.empty()) {
            transforms.reserve(200);
            colorBuffer = std::make_shared<gpu::Buffer>();
            for (int i = 0; i < 100; ++i) {
                {
                    glm::mat4 transform = glm::translate(mat4(), vec3(0, -1, -50 + i));
                    transform = glm::scale(transform, vec3(100, 1, 1));
                    transforms.push_back(transform);
                    colorBuffer->append(compactColor1);
                }

                {
                    glm::mat4 transform = glm::mat4_cast(quat(vec3(0, PI / 2.0f, 0)));
                    transform = glm::translate(transform, vec3(0, -1, -50 + i));
                    transform = glm::scale(transform, vec3(100, 1, 1));
                    transforms.push_back(transform);
                    colorBuffer->append(compactColor2);
                }
            }
        }
        auto pipeline = geometryCache->getSimplePipeline();
        for (auto& transform : transforms) {
            batch.setModelTransform(transform);
            batch.setupNamedCalls(GRID_INSTANCE, [=](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
                batch.setViewTransform(_camera);
                batch.setPipeline(_pipeline);
                geometryCache->renderWireShapeInstances(batch, GeometryCache::Line, data.count(), colorBuffer);
            });
        }
    }

    void drawSimpleShapes(gpu::Batch& batch) {
        auto geometryCache = DependencyManager::get<GeometryCache>();
        static const size_t ITEM_COUNT = 1000;
        static const float SHAPE_INTERVAL = (PI * 2.0f) / ITEM_COUNT;
        static const float ITEM_INTERVAL = SHAPE_INTERVAL / TYPE_COUNT;

        static const gpu::Element POSITION_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
        static const gpu::Element NORMAL_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
        static const gpu::Element COLOR_ELEMENT { gpu::VEC4, gpu::NUINT8, gpu::RGBA };

        static std::vector<Transform> transforms;
        static std::vector<vec4> colors;
        static gpu::BufferPointer colorBuffer;
        static gpu::BufferView colorView;
        static gpu::BufferView instanceXfmView;
        if (!colorBuffer) {
            colorBuffer = std::make_shared<gpu::Buffer>();

            static const float ITEM_RADIUS = 20;
            static const vec3 ITEM_TRANSLATION { 0, 0, -ITEM_RADIUS };
            for (size_t i = 0; i < TYPE_COUNT; ++i) {
                GeometryCache::Shape shape = SHAPE[i];
                GeometryCache::ShapeData shapeData = geometryCache->_shapes[shape];
                //indirectCommand._count
                float startingInterval = ITEM_INTERVAL * i;
                for (size_t j = 0; j < ITEM_COUNT; ++j) {
                    float theta = j * SHAPE_INTERVAL + startingInterval;
                    auto transform = glm::rotate(mat4(), theta, Vectors::UP);
                    transform = glm::rotate(transform, (randFloat() - 0.5f) * PI / 4.0f, Vectors::UNIT_X);
                    transform = glm::translate(transform, ITEM_TRANSLATION);
                    transform = glm::scale(transform, vec3(randFloat() / 2.0f + 0.5f));
                    transforms.push_back(transform);
                    auto color = vec4 { randomColorValue(64), randomColorValue(64), randomColorValue(64), 255 };
                    color /= 255.0f;
                    colors.push_back(color);
                    colorBuffer->append(toCompactColor(color));
                }
            }
            colorView = gpu::BufferView(colorBuffer, COLOR_ELEMENT);
        }

        batch.setViewTransform(_camera);
        batch.setPipeline(_pipeline);
        batch.setInputFormat(getInstancedSolidStreamFormat());
        for (size_t i = 0; i < TYPE_COUNT; ++i) {
            GeometryCache::Shape shape = SHAPE[i];
            GeometryCache::ShapeData shapeData = geometryCache->_shapes[shape];
            batch.setInputBuffer(gpu::Stream::COLOR, colorView);
            for (size_t j = 0; j < ITEM_COUNT; ++j) {
                batch.setModelTransform(transforms[j]);
                shapeData.draw(batch);
            }
        }
    }

    void drawCenterShape(gpu::Batch& batch) {
        // Render unlit cube + sphere
        static auto startUsecs = usecTimestampNow();
        float seconds = getSeconds(startUsecs);
        seconds /= 4.0f;
        batch.setModelTransform(Transform());
        batch._glColor4f(0.8f, 0.25f, 0.25f, 1.0f);

        bool wire = (seconds - floorf(seconds) > 0.5f);
        auto geometryCache = DependencyManager::get<GeometryCache>();
        int shapeIndex = ((int)seconds) % TYPE_COUNT;
        if (wire) {
            geometryCache->renderWireShape(batch, SHAPE[shapeIndex]);
        } else {
            geometryCache->renderShape(batch, SHAPE[shapeIndex]);
        }

        batch.setModelTransform(Transform().setScale(2.05f));
        batch._glColor4f(1, 1, 1, 1);
        geometryCache->renderWireCube(batch);
    }

    void drawTerrain(gpu::Batch& batch) {
        auto geometryCache = DependencyManager::get<GeometryCache>();
        static std::once_flag once;
        static gpu::BufferPointer vertexBuffer { std::make_shared<gpu::Buffer>() };
        static gpu::BufferPointer indexBuffer { std::make_shared<gpu::Buffer>() };

        static gpu::BufferView positionView;
        static gpu::BufferView textureView;
        static gpu::Stream::FormatPointer vertexFormat { std::make_shared<gpu::Stream::Format>() };

        static gpu::TexturePointer texture;
        static gpu::PipelinePointer pipeline;
        std::call_once(once, [&] {
            static const uint SHAPE_VERTEX_STRIDE = sizeof(glm::vec4) * 2; // position, normals, textures
            static const uint SHAPE_TEXTURES_OFFSET = sizeof(glm::vec4);
            static const gpu::Element POSITION_ELEMENT { gpu::VEC3, gpu::FLOAT, gpu::XYZ };
            static const gpu::Element TEXTURE_ELEMENT { gpu::VEC2, gpu::FLOAT, gpu::UV };
            std::vector<vec4> vertices;
            const int MINX = -1000;
            const int MAXX = 1000;

            // top
            vertices.push_back(vec4(MAXX, 0, MAXX, 1));
            vertices.push_back(vec4(MAXX, MAXX, 0, 0));

            vertices.push_back(vec4(MAXX, 0, MINX, 1));
            vertices.push_back(vec4(MAXX, 0, 0, 0));

            vertices.push_back(vec4(MINX, 0, MINX, 1));
            vertices.push_back(vec4(0, 0, 0, 0));

            vertices.push_back(vec4(MINX, 0, MAXX, 1));
            vertices.push_back(vec4(0, MAXX, 0, 0));

            vertexBuffer->append(vertices);
            indexBuffer->append(std::vector<uint16_t>({ 0, 1, 2, 2, 3, 0 }));

            positionView = gpu::BufferView(vertexBuffer, 0, vertexBuffer->getSize(), SHAPE_VERTEX_STRIDE, POSITION_ELEMENT);
            textureView = gpu::BufferView(vertexBuffer, SHAPE_TEXTURES_OFFSET, vertexBuffer->getSize(), SHAPE_VERTEX_STRIDE, TEXTURE_ELEMENT);
            texture = DependencyManager::get<TextureCache>()->getImageTexture("C:/Users/bdavis/Git/openvr/samples/bin/cube_texture.png");
            // texture = DependencyManager::get<TextureCache>()->getImageTexture("H:/test.png");
            //texture = DependencyManager::get<TextureCache>()->getImageTexture("H:/crate_blue.fbm/lambert8SG_Normal_OpenGL.png");

            auto shader = makeShader(VERTEX_SHADER, FRAGMENT_SHADER, gpu::Shader::BindingSet {});
            auto state = std::make_shared<gpu::State>();
            state->setMultisampleEnable(false);
            state->setDepthTest(gpu::State::DepthTest { true });
            pipeline = gpu::Pipeline::create(shader, state);
            vertexFormat->setAttribute(gpu::Stream::POSITION);
            vertexFormat->setAttribute(gpu::Stream::TEXCOORD);
        });

        static auto start = usecTimestampNow();
        auto now = usecTimestampNow();
        if ((now - start) > USECS_PER_SECOND * 1) {
            start = now;
            texture->incremementMinMip();
        }

        batch.setPipeline(pipeline);
        batch.setInputBuffer(gpu::Stream::POSITION, positionView);
        batch.setInputBuffer(gpu::Stream::TEXCOORD, textureView);
        batch.setIndexBuffer(gpu::UINT16, indexBuffer, 0);
        batch.setInputFormat(vertexFormat);

        batch.setResourceTexture(0, texture);
        batch.setModelTransform(glm::translate(glm::mat4(), vec3(0, -0.1, 0)));
        batch.drawIndexed(gpu::TRIANGLES, 6, 0);

        batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getBlueTexture());
        batch.setModelTransform(glm::translate(glm::mat4(), vec3(0, -0.2, 0)));
        batch.drawIndexed(gpu::TRIANGLES, 6, 0);
    }

    void draw() {
        // Attempting to draw before we're visible and have a valid size will
        // produce GL errors.
        if (!isVisible() || _size.width() <= 0 || _size.height() <= 0) {
            return;
        }
        updateCamera();
        makeCurrent();
        
        gpu::Batch batch;
        batch.resetStages();
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 0.0f, 0.1f, 0.2f, 1.0f });
        batch.clearDepthFramebuffer(1e4);
        batch.setViewportTransform({ 0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio() });
        batch.setProjectionTransform(_projectionMatrix);
        
        batch.setViewTransform(_camera);
        batch.setPipeline(_pipeline);
        batch.setModelTransform(Transform());

        //drawFloorGrid(batch);
        //drawSimpleShapes(batch);
        //drawCenterShape(batch);
        drawTerrain(batch);

        _context->render(batch);
        _qGlContext.swapBuffers(this);
        
        fps.increment();
        if (fps.elapsed() >= 0.5f) {
            qDebug() << "FPS: " << fps.rate();
            fps.reset();
        }
    }
    
    void makeCurrent() {
        _qGlContext.makeCurrent(this);
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
    auto timer = new QTimer(&app);
    timer->setInterval(0);
    app.connect(timer, &QTimer::timeout, &app, [&] {
        window.draw();
    });
    timer->start();
    app.exec();
    return 0;
}

#include "main.moc"

