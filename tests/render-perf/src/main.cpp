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
#include <sstream>

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
#include <AssetClient.h>

#include <gl/OffscreenGLCanvas.h>
#include <gl/OglplusHelpers.h>
#include <gl/GLHelpers.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gl/QOpenGLDebugLoggerWrapper.h>

#include <gpu/gl/GLBackend.h>
#include <gpu/gl/GLFramebuffer.h>
#include <gpu/gl/GLTexture.h>

#include <WebEntityItem.h>
#include <OctreeUtils.h>
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

#include "Camera.hpp"
#include "TextOverlay.hpp"

static const QString LAST_SCENE_KEY = "lastSceneFile";
static const QString LAST_LOCATION_KEY = "lastLocation";

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

static QString toHumanSize(size_t size, size_t maxUnit = std::numeric_limits<size_t>::max()) {
    static const std::vector<QString> SUFFIXES{ { "B", "KB", "MB", "GB", "TB", "PB" } };
    const size_t maxIndex = std::min(maxUnit, SUFFIXES.size() - 1);
    size_t suffixIndex = 0;

    while (suffixIndex < maxIndex && size > 1024) {
        size >>= 10;
        ++suffixIndex;
    }

    return QString("%1 %2").arg(size).arg(SUFFIXES[suffixIndex]);
}



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

    std::map<void*, std::function<void()>> _postUpdateLambdas;
    void pushPostUpdateLambda(void* key, std::function<void()> func) override {
        _postUpdateLambdas[key] = func;
    }

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
        QThread::currentThread()->setPriority(QThread::HighestPriority);
        AbstractViewStateInterface::setInstance(this);
        _octree = DependencyManager::set<EntityTreeRenderer>(false, this, nullptr);
        _octree->init();
        // Prevent web entities from rendering
        REGISTER_ENTITY_TYPE_WITH_FACTORY(Web, WebEntityItem::factory)

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
        resize(QSize(800, 600));
        show();
        makeCurrent();
        glewExperimental = true;
        glewInit();
        glGetError();
        setupDebugLogger(this);
#ifdef Q_OS_WIN
        wglSwapIntervalEXT(0);
#endif
        {
            makeCurrent();
            _quadProgram = loadDefaultShader();
            _plane = loadPlane(_quadProgram);
            _textOverlay = new TextOverlay(glm::uvec2(800, 600));
            glViewport(0, 0, 800, 600);
        }

        _camera.movementSpeed = 50.0f;


        // GPU library init
        {
            _offscreenContext = new OffscreenGLCanvas();
            _offscreenContext->create(_context.getContext());
            _offscreenContext->makeCurrent();
            gpu::Context::init<gpu::gl::GLBackend>();
            _gpuContext = std::make_shared<gpu::Context>();
        }

        // Render engine library init
        {
            _offscreenContext->makeCurrent();
            DependencyManager::get<DeferredLightingEffect>()->init();
            _renderEngine->addJob<RenderShadowTask>("RenderShadowTask", _cullFunctor);
            _renderEngine->addJob<RenderDeferredTask>("RenderDeferredTask", _cullFunctor);
            _renderEngine->load();
            _renderEngine->registerScene(_main3DScene);
        }

        reloadScene();
        restorePosition();

        _elapsed.start();
        QTimer* timer = new QTimer(this);
        timer->setInterval(0);
        connect(timer, &QTimer::timeout, this, [this] {
            draw();
        });
        timer->start();
        _ready = true;
    }

    virtual ~QTestWindow() {
        ResourceManager::cleanup();
        try { _quadProgram.reset(); } catch (std::runtime_error&) {}
        try { _plane.reset(); } catch (std::runtime_error&) {}
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        switch (event->key()) {
        case Qt::Key_F1:
            importScene();
            return;

        case Qt::Key_F2:
            reloadScene();
            return;

        case Qt::Key_F4:
            toggleStereo();
            return;

        case Qt::Key_F5:
            goTo();
            return;

        case Qt::Key_F6:
            savePosition();
            return;

        case Qt::Key_F7:
            restorePosition();
            return;

        case Qt::Key_F8:
            resetPosition();
            return;

        case Qt::Key_F9:
            toggleCulling();
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

    static bool cull(const RenderArgs* args, const AABox& bounds) {
        float renderAccuracy = calculateRenderAccuracy(args->getViewFrustum().getPosition(), bounds, args->_sizeScale, args->_boundaryLevelAdjust);
        return (renderAccuracy > 0.0f);
    }

    void draw() {
        if (!_ready) {
            return;
        }
        if (!isVisible()) {
            return;
        }
        update();

        _offscreenContext->makeCurrent();

        RenderArgs renderArgs(_gpuContext, _octree.data(), DEFAULT_OCTREE_SIZE_SCALE,
            0, RenderArgs::DEFAULT_RENDER_MODE,
            RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);

        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        QSize windowSize = size();
        framebufferCache->setFrameBufferSize(windowSize);
        // Viewport is assigned to the size of the framebuffer
        renderArgs._viewport = ivec4(0, 0, windowSize.width(), windowSize.height());

        renderArgs.setViewFrustum(_viewFrustum);

        renderArgs._context->enableStereo(_stereoEnabled);
        if (_stereoEnabled) {
            mat4 eyeOffsets[2];
            mat4 eyeProjections[2];
            for (size_t i = 0; i < 2; ++i) {
                eyeProjections[i] = _viewFrustum.getProjection();
            }
            renderArgs._context->setStereoProjections(eyeProjections);
            renderArgs._context->setStereoViews(eyeOffsets);
        }

        // Final framebuffer that will be handled to the display-plugin
        {
            auto finalFramebuffer = framebufferCache->getFramebuffer();
            renderArgs._blitFramebuffer = finalFramebuffer;
        }

        _gpuContext->beginFrame(renderArgs._blitFramebuffer);
        gpu::doInBatch(renderArgs._context, [&](gpu::Batch& batch) {
            batch.resetStages();
        });
        render(&renderArgs);
        _gpuContext->endFrame();
        GLuint glTex;
        {
            auto gpuTex = renderArgs._blitFramebuffer->getRenderBuffer(0);
            glTex = gpu::Backend::getGPUObject<gpu::gl::GLTexture>(*gpuTex)->_id;
        }

        makeCurrent();
        {
            glBindTexture(GL_TEXTURE_2D, glTex);
            _quadProgram->Use();
            _plane->Use();
            _plane->Draw();
            glBindVertexArray(0);
        }

        {
            //_textOverlay->render();
        }

        _context.swapBuffers(this);

        _offscreenContext->makeCurrent();
        framebufferCache->releaseFramebuffer(renderArgs._blitFramebuffer);
        renderArgs._blitFramebuffer.reset();
        _fpsCounter.increment();
        static size_t _frameCount { 0 };
        ++_frameCount;
        if (_elapsed.elapsed() >= 500) {
            _fps = _fpsCounter.rate();
            updateText();
            _frameCount = 0;
            _elapsed.restart();
        }
    }

private:
    class EntityUpdateOperator : public RecurseOctreeOperator {
    public:
        EntityUpdateOperator(const qint64& now) : now(now) {}
        bool preRecursion(OctreeElementPointer element) override { return true; }
        bool postRecursion(OctreeElementPointer element) override {
            EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
            entityTreeElement->forEachEntity([&](EntityItemPointer entityItem) {
                if (!entityItem->isParentIDValid()) {
                    return;  // we weren't able to resolve a parent from _parentID, so don't save this entity.
                }
                entityItem->update(now);
            });
            return true;
        }

        const qint64& now;
    };




    void updateText() {
        //qDebug() << "FPS " << fps.rate();
        {
            _textBlocks.erase(TextBlock::Info);
            auto& infoTextBlock = _textBlocks[TextBlock::Info];
            infoTextBlock.push_back({ vec2(98, 10), "FPS: ", TextOverlay::alignRight });
            infoTextBlock.push_back({ vec2(100, 10), std::to_string((uint32_t)_fps), TextOverlay::alignLeft });
            infoTextBlock.push_back({ vec2(98, 30), "Culling: ", TextOverlay::alignRight });
            infoTextBlock.push_back({ vec2(100, 30), _cullingEnabled ? "Enabled" : "Disabled", TextOverlay::alignLeft });

            setTitle(QString("FPS %1 Culling %2 TextureMemory GPU %3 CPU %4")
                .arg(_fps).arg(_cullingEnabled)
                .arg(toHumanSize(gpu::Context::getTextureGPUMemoryUsage(), 2))
                .arg(toHumanSize(gpu::Texture::getTextureCPUMemoryUsage(), 2)));
        }

        _textOverlay->beginTextUpdate();
        for (const auto& e : _textBlocks) {
            for (const auto& b : e.second) {
                _textOverlay->addText(b.text, b.position, b.alignment);
            }
        }
        _textOverlay->endTextUpdate();
    }

    void update() {
        auto now = usecTimestampNow();
        static auto last = now;

        float delta = now - last;
        // Update the camera
        _camera.update(delta / USECS_PER_SECOND);
        {
            _viewFrustum = ViewFrustum();
            _viewFrustum.setProjection(_camera.matrices.perspective);
            auto view = glm::inverse(_camera.matrices.view);
            _viewFrustum.setPosition(glm::vec3(view[3]));
            _viewFrustum.setOrientation(glm::quat_cast(view));
            // Failing to do the calculation of the bound planes causes everything to be considered inside the frustum
            if (_cullingEnabled) {
                _viewFrustum.calculate();
            }
        }

        getEntities()->setViewFrustum(_viewFrustum);
        EntityUpdateOperator updateOperator(now);
        //getEntities()->getTree()->recurseTreeWithOperator(&updateOperator);
        {
            PROFILE_RANGE_EX("PreRenderLambdas", 0xffff0000, (uint64_t)0);
            for (auto& iter : _postUpdateLambdas) {
                iter.second();
            }
            _postUpdateLambdas.clear();
        }

        last = now;
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

    bool makeCurrent() {
        bool currentResult = _context.makeCurrent(this);
        Q_ASSERT(currentResult);
        return currentResult;
    }

    void resizeWindow(const QSize& size) {
        _size = size;
        _camera.setAspectRatio((float)_size.width() / (float)_size.height());
        if (!_ready) {
            return;
        }
        _textOverlay->resize(toGlm(_size));
        makeCurrent();
        glViewport(0, 0, size.width(), size.height());
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

                    glm::vec4 v = glm::vec4(
                        orientationRegex.cap(1).toFloat(),
                        orientationRegex.cap(2).toFloat(),
                        orientationRegex.cap(3).toFloat(),
                        orientationRegex.cap(4).toFloat());
                    if (!glm::any(glm::isnan(v))) {
                        _camera.setRotation(glm::normalize(glm::quat(v.w, v.x, v.y, v.z)));
                    }
                }
            }
        }
    }

    void importScene(const QString& fileName) {
        auto assetClient = DependencyManager::get<AssetClient>();
        QFileInfo fileInfo(fileName);
        QString atpPath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".atp";
        qDebug() << atpPath;
        QFileInfo atpPathInfo(atpPath);
        if (atpPathInfo.exists()) {
            QString atpUrl = QUrl::fromLocalFile(atpPath).toString();
            ResourceManager::setUrlPrefixOverride("atp:/", atpUrl + "/");
        }
        _settings.setValue(LAST_SCENE_KEY, fileName);
        _octree->clear();
        _octree->getTree()->readFromURL(fileName);
    }

    void importScene() {
        auto lastScene = _settings.value(LAST_SCENE_KEY);
        QString openDir;
        if (lastScene.isValid()) {
            QFileInfo lastSceneInfo(lastScene.toString());
            if (lastSceneInfo.absoluteDir().exists()) {
                openDir = lastSceneInfo.absolutePath();
            }
        }

        QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Open File"), openDir, tr("Hifi Exports (*.json *.svo)"));
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

    void reloadScene() {
        QVariant lastScene = _settings.value(LAST_SCENE_KEY);
        if (lastScene.isValid()) {
            importScene(lastScene.toString());
        }
    }

    void savePosition() {
        // /-17.2049,-8.08629,-19.4153/0,-0.48551,0,0.874231
        glm::quat q = _camera.getOrientation();
        glm::vec3 v = _camera.position;
        QString viewpoint = QString("/%1,%2,%3/%4,%5,%6,%7").
            arg(v.x).arg(v.y).arg(v.z).
            arg(q.x).arg(q.y).arg(q.z).arg(q.w);
        _settings.setValue(LAST_LOCATION_KEY, viewpoint);
    }

    void restorePosition() {
        // /-17.2049,-8.08629,-19.4153/0,-0.48551,0,0.874231
        QVariant viewpoint = _settings.value(LAST_LOCATION_KEY);
        if (viewpoint.isValid()) {
            parsePath(viewpoint.toString());
        }
    }

    void resetPosition() {
        _camera.yaw = 0;
        _camera.setPosition(vec3());
    }

    void toggleCulling() {
        _cullingEnabled = !_cullingEnabled;
    }

    void toggleStereo() {
        _stereoEnabled = !_stereoEnabled;
    }

    QSharedPointer<EntityTreeRenderer> getEntities() {
        return _octree;
    }

private:
    render::CullFunctor _cullFunctor { [&](const RenderArgs* args, const AABox& bounds)->bool{
        if (_cullingEnabled) {
            return cull(args, bounds);
        } else {
            return true;
        }
    } };

    struct TextElement {
        const glm::vec2 position;
        const std::string text;
        TextOverlay::TextAlign alignment;
    };

    enum TextBlock {
        Help,
        Info,
    };

    std::map<TextBlock, std::list<TextElement>> _textBlocks;

    gpu::ContextPointer _gpuContext; // initialized during window creation
    render::EnginePointer _renderEngine { new render::Engine() };
    render::ScenePointer _main3DScene { new render::Scene(glm::vec3(-0.5f * (float)TREE_SCALE), (float)TREE_SCALE) };
    OffscreenGLCanvas* _offscreenContext { nullptr };
    QOpenGLContextWrapper _context;
    QSize _size;
    RateCounter<200> _fpsCounter;
    QSettings _settings;

    ProgramPtr _quadProgram;
    ShapeWrapperPtr _plane;

    QWindowCamera _camera;
    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    ViewFrustum _shadowViewFrustum; // current state of view frustum, perspective, orientation, etc.
    model::SunSkyStage _sunSkyStage;
    model::LightPointer _globalLight { std::make_shared<model::Light>() };
    QElapsedTimer _elapsed;
    bool _ready { false };
    float _fps { 0 };
    TextOverlay* _textOverlay;
    bool _cullingEnabled { true };
    bool _stereoEnabled { false };
    QSharedPointer<EntityTreeRenderer> _octree;
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
