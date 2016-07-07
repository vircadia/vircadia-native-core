//
//  Created by Bradley Austin Davis on 2016/07/01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <string>
#include <vector>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QtCore/QThread>

#include <QtGui/QGuiApplication>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>

#include <shared/RateCounter.h>

#include <gl/GLHelpers.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gl/QOpenGLDebugLoggerWrapper.h>

#include <gpu/gl/GLBackend.h>
#include <gpu/gl/GLFramebuffer.h>

#include <render/Engine.h>
#include <Model.h>
#include <model/Stage.h>
#include <TextureCache.h>
#include <FramebufferCache.h>
#include <model-networking/ModelCache.h>
#include <GeometryCache.h>
#include <DeferredLightingEffect.h>
#include <RenderShadowTask.h>
#include <RenderDeferredTask.h>
#include <OctreeConstants.h>

#include <EntityTreeRenderer.h>
#include <AbstractViewStateInterface.h>
#include <AddressManager.h>
#include <SceneScriptingInterface.h>

static const QString LAST_SCENE_KEY = "lastSceneFile";

class ParentFinder : public SpatialParentFinder {
public:
    EntityTreePointer _tree;
    ParentFinder(EntityTreePointer tree) : _tree(tree) {}

    SpatiallyNestableWeakPointer find(QUuid parentID, bool& success, SpatialParentTree* entityTree = nullptr) const override {
        SpatiallyNestableWeakPointer parent;

        if (parentID.isNull()) {
            success = true;
            return parent;
        }

        // search entities
        if (entityTree) {
            parent = entityTree->findByID(parentID);
        } else {
            parent = _tree ? _tree->findEntityByEntityItemID(parentID) : nullptr;
        }

        if (!parent.expired()) {
            success = true;
            return parent;
        }

        success = false;
        return parent;
    }
};

class Camera {
protected:
    float fov { 60.0f };
    float znear { 0.1f }, zfar { 512.0f };
    float aspect { 1.0f };

    void updateViewMatrix() {
        matrices.view = glm::inverse(glm::translate(glm::mat4(), position) * glm::mat4_cast(getOrientation()));
    }

    glm::quat getOrientation() const {
        return glm::angleAxis(yaw, Vectors::UP);
    }
public:
    float yaw { 0 };
    glm::vec3 position;

    float rotationSpeed { 1.0f };
    float movementSpeed { 1.0f };

    struct Matrices {
        glm::mat4 perspective;
        glm::mat4 view;
    } matrices;
    enum Key {
        RIGHT,
        LEFT,
        UP,
        DOWN,
        BACK,
        FORWARD,
        KEYS_SIZE,
        INVALID = -1,
    };

    std::bitset<KEYS_SIZE> keys;

    Camera() {
        matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    }

    bool moving() {
        return keys.any();
    }

    void setFieldOfView(float fov) {
        this->fov = fov;
        matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    }

    void setAspectRatio(const glm::vec2& size) {
        setAspectRatio(size.x / size.y);
    }

    void setAspectRatio(float aspect) {
        this->aspect = aspect;
        matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    }

    void setPerspective(float fov, const glm::vec2& size, float znear = 0.1f, float zfar = 512.0f) {
        setPerspective(fov, size.x / size.y, znear, zfar);
    }

    void setPerspective(float fov, float aspect, float znear = 0.1f, float zfar = 512.0f) {
        this->aspect = aspect;
        this->fov = fov;
        this->znear = znear;
        this->zfar = zfar;
        matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    };

    void rotate(const float delta) {
        yaw += delta;
        updateViewMatrix();
    }

    void setPosition(const glm::vec3& position) {
        this->position = position;
        updateViewMatrix();
    }

    // Translate in the Z axis of the camera
    void dolly(float delta) {
        auto direction = glm::vec3(0, 0, delta);
        translate(direction);
    }

    // Translate in the XY plane of the camera
    void translate(const glm::vec2& delta) {
        auto move = glm::vec3(delta.x, delta.y, 0);
        translate(move);
    }

    void translate(const glm::vec3& delta) {
        position += getOrientation() * delta;
        updateViewMatrix();
    }

    void update(float deltaTime) {
        if (moving()) {
            glm::vec3 camFront = getOrientation() * Vectors::FRONT;
            glm::vec3 camRight = getOrientation() * Vectors::RIGHT;
            glm::vec3 camUp = getOrientation() * Vectors::UP;
            float moveSpeed = deltaTime * movementSpeed;

            if (keys[FORWARD]) {
                position += camFront * moveSpeed;
            }
            if (keys[BACK]) {
                position -= camFront * moveSpeed;
            }
            if (keys[LEFT]) {
                position -= camRight * moveSpeed;
            }
            if (keys[RIGHT]) {
                position += camRight * moveSpeed;
            }
            if (keys[UP]) {
                position += camUp * moveSpeed;
            }
            if (keys[DOWN]) {
                position -= camUp * moveSpeed;
            }
            updateViewMatrix();
        }
    }
};

class QWindowCamera : public Camera {
    Key forKey(int key) {
        switch (key) {
        case Qt::Key_W: return FORWARD;
        case Qt::Key_S: return BACK;
        case Qt::Key_A: return LEFT;
        case Qt::Key_D: return RIGHT;
        case Qt::Key_E: return UP;
        case Qt::Key_C: return DOWN;
        default: break;
        }
        return INVALID;
    }

    vec2 _lastMouse;
public:
    void onKeyPress(QKeyEvent* event) {
        Key k = forKey(event->key());
        if (k == INVALID) {
            return;
        }
        keys.set(k);
    }

    void onKeyRelease(QKeyEvent* event) {
        Key k = forKey(event->key());
        if (k == INVALID) {
            return;
        }
        keys.reset(k);
    }

    void onMouseMove(QMouseEvent* event) {
        vec2 mouse = toGlm(event->localPos());
        vec2 delta = mouse - _lastMouse;
        auto buttons = event->buttons();
        if (buttons & Qt::RightButton) {
            dolly(delta.y * 0.01f);
        } else if (buttons & Qt::LeftButton) {
            rotate(delta.x * -0.01f);
        } else if (buttons & Qt::MiddleButton) {
            delta.y *= -1.0f;
            translate(delta * -0.01f);
        }

        _lastMouse = mouse;
    }
};

// Create a simple OpenGL window that renders text in various ways
class QTestWindow : public QWindow, public AbstractViewStateInterface {
    Q_OBJECT

protected:
    void copyCurrentViewFrustum(ViewFrustum& viewOut) const override {
        viewOut = _viewFrustum;
    }

    void copyShadowViewFrustum(ViewFrustum& viewOut) const override {
        viewOut = _shadowViewFrustum;
    }

    QThread* getMainThread() override {
        return QThread::currentThread();
    }

    PickRay computePickRay(float x, float y) const override {
        return PickRay();
    }

    glm::vec3 getAvatarPosition() const override {
        return vec3();
    }

    void postLambdaEvent(std::function<void()> f) override {}
    qreal getDevicePixelRatio() override {
        return 1.0f;
    }

    render::ScenePointer getMain3DScene() override {
        return _main3DScene;
    }
    render::EnginePointer getRenderEngine() override {
        return _renderEngine;
    }

    void pushPostUpdateLambda(void* key, std::function<void()> func) override {}

public:
    //"/-17.2049,-8.08629,-19.4153/0,0.881994,0,-0.47126"
    static void setup() {
        DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
        DependencyManager::registerInheritance<SpatialParentFinder, ParentFinder>();
        DependencyManager::set<AddressManager>();
        DependencyManager::set<NodeList>(NodeType::Agent, 0);
        DependencyManager::set<DeferredLightingEffect>();
        DependencyManager::set<ResourceCacheSharedItems>();
        DependencyManager::set<TextureCache>();
        DependencyManager::set<FramebufferCache>();
        DependencyManager::set<GeometryCache>();
        DependencyManager::set<ModelCache>();
        DependencyManager::set<AnimationCache>();
        DependencyManager::set<ModelBlender>();
        DependencyManager::set<PathUtils>();
        DependencyManager::set<SceneScriptingInterface>();
    }

    QTestWindow() {
        AbstractViewStateInterface::setInstance(this);
        _octree = DependencyManager::set<EntityTreeRenderer>(false, this, nullptr);
        _octree->init();
        DependencyManager::set<ParentFinder>(_octree->getTree());
        getEntities()->setViewFrustum(_viewFrustum);
        auto nodeList = DependencyManager::get<LimitedNodeList>();
        NodePermissions permissions;
        permissions.setAll(true);
        nodeList->setPermissions(permissions);

        ResourceManager::init();
        setSurfaceType(QSurface::OpenGLSurface);
        auto format = getDefaultOpenGLSurfaceFormat();
        format.setOption(QSurfaceFormat::DebugContext);
        setFormat(format);

        _context.setFormat(format);
        _context.create();

        show();
        makeCurrent();
        glewInit();
        glGetError();
#ifdef Q_OS_WIN
        wglSwapIntervalEXT(0);
#endif
        _camera.movementSpeed = 3.0f;

        setupDebugLogger(this);
        qDebug() << (const char*)glGetString(GL_VERSION);

        // GPU library init
        {
            gpu::Context::init<gpu::gl::GLBackend>();
            _gpuContext = std::make_shared<gpu::Context>();
        }

        // Render engine library init
        {
            makeCurrent();
            DependencyManager::get<DeferredLightingEffect>()->init();
            _renderEngine->addJob<RenderShadowTask>("RenderShadowTask", _cullFunctor);
            _renderEngine->addJob<RenderDeferredTask>("RenderDeferredTask", _cullFunctor);
            _renderEngine->load();
            _renderEngine->registerScene(_main3DScene);
        }

        QVariant lastScene = _settings.value(LAST_SCENE_KEY);
        if (lastScene.isValid()) {
            auto result = QMessageBox::question(nullptr, "Question", "Load last scene " + lastScene.toString());
            if (result != QMessageBox::No) {
                importScene(lastScene.toString());
            }
        }

        resize(QSize(800, 600));
        _elapsed.start();

        QTimer* timer = new QTimer(this);
        timer->setInterval(0);
        connect(timer, &QTimer::timeout, this, [this] {
            draw();
        });
        timer->start();
    }

    virtual ~QTestWindow() {
        ResourceManager::cleanup();
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        switch (event->key()) {
        case Qt::Key_F1:
            importScene();
            return;

        case Qt::Key_F2:
            goTo();
            return;

        default:
            break;
        }
        _camera.onKeyPress(event);
    }

    void keyReleaseEvent(QKeyEvent* event) override {
        _camera.onKeyRelease(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        _camera.onMouseMove(event);
    }

    void resizeEvent(QResizeEvent* ev) override {
        resizeWindow(ev->size());
    }

private:

    static bool cull(const RenderArgs* renderArgs, const AABox& box) {
        return true;
    }

    void update() {
        auto now = usecTimestampNow();
        static auto last = now;

        float delta = now - last;
        // Update the camera
        _camera.update(delta / USECS_PER_SECOND);


        // load the view frustum
        {
            _viewFrustum.setProjection(_camera.matrices.perspective);
            auto view = glm::inverse(_camera.matrices.view);
            _viewFrustum.setPosition(glm::vec3(view[3]));
            _viewFrustum.setOrientation(glm::quat_cast(view));
        }
        last = now;
    }

    void draw() {
        if (!isVisible()) {
            return;
        }
        update();

        makeCurrent();
#define RENDER_SCENE 1

#if RENDER_SCENE
        RenderArgs renderArgs(_gpuContext, _octree.data(), DEFAULT_OCTREE_SIZE_SCALE,
            0, RenderArgs::DEFAULT_RENDER_MODE,
            RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);

        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        QSize windowSize = size();
        framebufferCache->setFrameBufferSize(windowSize);
        // Viewport is assigned to the size of the framebuffer
        renderArgs._viewport = ivec4(0, 0, windowSize.width(), windowSize.height());

        renderArgs.setViewFrustum(_viewFrustum);

        // Final framebuffer that will be handled to the display-plugin
        {
            auto finalFramebuffer = framebufferCache->getFramebuffer();
            renderArgs._blitFramebuffer = finalFramebuffer;
        }

        render(&renderArgs);

        {
            gpu::gl::GLFramebuffer* framebuffer = gpu::Backend::getGPUObject<gpu::gl::GLFramebuffer>(*renderArgs._blitFramebuffer);
            auto fbo = framebuffer->_id;
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            const auto& vp = renderArgs._viewport;
            glBlitFramebuffer(vp.x, vp.y, vp.z, vp.w, vp.x, vp.y, vp.z, vp.w, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
#else 
        glClearColor(0.0f, 0.5f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
#endif

        _context.swapBuffers(this);

#if RENDER_SCENE
        framebufferCache->releaseFramebuffer(renderArgs._blitFramebuffer);
        renderArgs._blitFramebuffer.reset();
        gpu::doInBatch(renderArgs._context, [&](gpu::Batch& batch) {
            batch.resetStages();
        });
#endif
        fps.increment();
        static size_t _frameCount { 0 };
        ++_frameCount;
        if (_elapsed.elapsed() >= 4000) {
            qDebug() << "FPS " << fps.rate();
            _frameCount = 0;
            _elapsed.restart();
        }

        if (0 == ++_frameCount % 100) {
        }
    }

    void render(RenderArgs* renderArgs) {
        PROFILE_RANGE(__FUNCTION__);
        PerformanceTimer perfTimer("draw");
        // The pending changes collecting the changes here
        render::PendingChanges pendingChanges;
        // Setup the current Zone Entity lighting
        DependencyManager::get<DeferredLightingEffect>()->setGlobalLight(_sunSkyStage.getSunLight());
        {
            PerformanceTimer perfTimer("SceneProcessPendingChanges");
            _main3DScene->enqueuePendingChanges(pendingChanges);
            _main3DScene->processPendingChangesQueue();
        }

        // For now every frame pass the renderContext
        {
            PerformanceTimer perfTimer("EngineRun");
            _renderEngine->getRenderContext()->args = renderArgs;
            // Before the deferred pass, let's try to use the render engine
            _renderEngine->run();
        }
    }

    void makeCurrent() {
        _context.makeCurrent(this);
    }

    void resizeWindow(const QSize& size) {
        _size = size;
        _camera.setAspectRatio((float)_size.width() / (float)_size.height());
    }

    void parsePath(const QString& viewpointString) {
        static const QString FLOAT_REGEX_STRING = "([-+]?[0-9]*\\.?[0-9]+(?:[eE][-+]?[0-9]+)?)";
        static const QString SPACED_COMMA_REGEX_STRING = "\\s*,\\s*";
        static const QString POSITION_REGEX_STRING = QString("\\/") + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
            FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING + FLOAT_REGEX_STRING + "\\s*(?:$|\\/)";
        static const QString QUAT_REGEX_STRING = QString("\\/") + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
            FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING + FLOAT_REGEX_STRING + SPACED_COMMA_REGEX_STRING +
            FLOAT_REGEX_STRING + "\\s*$";

        static QRegExp orientationRegex(QUAT_REGEX_STRING);
        static QRegExp positionRegex(POSITION_REGEX_STRING);

        if (positionRegex.indexIn(viewpointString) != -1) {
            // we have at least a position, so emit our signal to say we need to change position
            glm::vec3 newPosition(positionRegex.cap(1).toFloat(),
                positionRegex.cap(2).toFloat(),
                positionRegex.cap(3).toFloat());
            _camera.setPosition(newPosition);

            if (!glm::any(glm::isnan(newPosition))) {
                // we may also have an orientation
                if (viewpointString[positionRegex.matchedLength() - 1] == QChar('/')
                    && orientationRegex.indexIn(viewpointString, positionRegex.matchedLength() - 1) != -1) {
                    //glm::vec4 v = glm::vec4(
                    //    orientationRegex.cap(1).toFloat(),
                    //    orientationRegex.cap(2).toFloat(),
                    //    orientationRegex.cap(3).toFloat(),
                    //    orientationRegex.cap(4).toFloat());
                    //if (!glm::any(glm::isnan(v))) {
                    //    _camera.setRotation(glm::normalize(glm::quat(v.w, v.x, v.y, v.z)));
                    //}
                }
            }
        }
    }

    void importScene(const QString& fileName) {
        _settings.setValue(LAST_SCENE_KEY, fileName);
        _octree->clear();
        _octree->getTree()->readFromURL(fileName);
    }

    void importScene() {
        QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Open File"), "/home", tr("Hifi Exports (*.json *.svo)"));
        if (fileName.isNull()) {
            return;
        }
        importScene(fileName);
    }

    void goTo() {
        QString destination = QInputDialog::getText(nullptr, tr("Go To Location"), "Enter path");
        if (destination.isNull()) {
            return;
        }
        parsePath(destination);
    }

private:
    render::CullFunctor _cullFunctor { cull };
    gpu::ContextPointer _gpuContext; // initialized during window creation
    render::EnginePointer _renderEngine { new render::Engine() };
    render::ScenePointer _main3DScene { new render::Scene(glm::vec3(-0.5f * (float)TREE_SCALE), (float)TREE_SCALE) };
    QOpenGLContextWrapper _context;
    QSize _size;
    RateCounter<> fps;
    QSettings _settings;

    QWindowCamera _camera;
    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    ViewFrustum _shadowViewFrustum; // current state of view frustum, perspective, orientation, etc.
    model::SunSkyStage _sunSkyStage;
    model::LightPointer _globalLight { std::make_shared<model::Light>() };
    QElapsedTimer _elapsed;
    QSharedPointer<EntityTreeRenderer> _octree;
    QSharedPointer<EntityTreeRenderer> getEntities() {
        return _octree;
    }
};

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (!message.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(message.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
        std::cout << message.toLocal8Bit().constData() << std::endl;
    }
}

const char * LOG_FILTER_RULES = R"V0G0N(
hifi.gpu=true
)V0G0N";

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("RenderPerf");
    QCoreApplication::setOrganizationName("High Fidelity");
    QCoreApplication::setOrganizationDomain("highfidelity.com");

    qInstallMessageHandler(messageHandler);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);
    QTestWindow::setup();
    QTestWindow window;
    app.exec();
    return 0;
}

#include "main.moc"
