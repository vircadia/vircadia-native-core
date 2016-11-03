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

#include <gl/Config.h>
#include <gl/Context.h>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QThreadPool>


#include <QtGui/QGuiApplication>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>


#include <shared/RateCounter.h>
#include <shared/NetworkUtils.h>
#include <shared/FileLogger.h>
#include <shared/FileUtils.h>
#include <LogHandler.h>
#include <AssetClient.h>

#include <gpu/gl/GLBackend.h>
#include <gpu/gl/GLFramebuffer.h>
#include <gpu/gl/GLTexture.h>
#include <gpu/StandardShaderLib.h>

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
            //rotate(delta.x * -0.01f);
            rotate(delta * -0.01f);
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

const char* SRGB_TO_LINEAR_FRAG = R"SCRIBE(

uniform sampler2D colorMap;

in vec2 varTexCoord0;

out vec4 outFragColor;

void main(void) {
    outFragColor = vec4(pow(texture(colorMap, varTexCoord0).rgb, vec3(2.2)), 1.0);
}
)SCRIBE";

extern QThread* RENDER_THREAD;

class RenderThread : public GenericThread {
    using Parent = GenericThread;
public:
    gl::Context _context;
    gpu::PipelinePointer _presentPipeline;
    gpu::ContextPointer _gpuContext; // initialized during window creation
    std::atomic<size_t> _presentCount;
    QElapsedTimer _elapsed;
    std::atomic<uint16_t> _fps{ 1 };
    RateCounter<200> _fpsCounter;
    std::mutex _mutex;
    std::shared_ptr<gpu::Backend> _backend;
    std::vector<uint64_t> _frameTimes;
    size_t _frameIndex { 0 };
    std::mutex _frameLock;
    std::queue<gpu::FramePointer> _pendingFrames;
    gpu::FramePointer _activeFrame;
    QSize _size;
    static const size_t FRAME_TIME_BUFFER_SIZE{ 8192 };

    void submitFrame(const gpu::FramePointer& frame) {
        std::unique_lock<std::mutex> lock(_frameLock);
        _pendingFrames.push(frame);
    }

    void initialize(QWindow* window, gl::Context& initContext) {
        setObjectName("RenderThread");
        _context.setWindow(window);
        _context.create();
        _context.makeCurrent();
        window->setSurfaceType(QSurface::OpenGLSurface);
        _context.makeCurrent(_context.qglContext(), window);
#ifdef Q_OS_WIN
        wglSwapIntervalEXT(0);
#endif
        // GPU library init
        gpu::Context::init<gpu::gl::GLBackend>();
        _gpuContext = std::make_shared<gpu::Context>();
        _backend = _gpuContext->getBackend();
        _context.makeCurrent();
        DependencyManager::get<DeferredLightingEffect>()->init();
        _context.makeCurrent();
        initContext.create();
        _context.doneCurrent();
        std::unique_lock<std::mutex> lock(_mutex);
        Parent::initialize();
        _context.moveToThread(_thread);
    }

    void setup() override {
        RENDER_THREAD = QThread::currentThread();

        // Wait until the context has been moved to this thread
        {
            std::unique_lock<std::mutex> lock(_mutex);
        }

        _context.makeCurrent();
        _frameTimes.resize(FRAME_TIME_BUFFER_SIZE, 0);
        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::Shader::createPixel(std::string(SRGB_TO_LINEAR_FRAG));
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::BindingSet slotBindings;
            gpu::Shader::makeProgram(*program, slotBindings);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            _presentPipeline = gpu::Pipeline::create(program, state);
        }

        //_textOverlay = new TextOverlay(glm::uvec2(800, 600));
        glViewport(0, 0, 800, 600);
        (void)CHECK_GL_ERROR();
        _elapsed.start();
    }

    void shutdown() override {
        _activeFrame.reset();
        while (!_pendingFrames.empty()) {
            _gpuContext->consumeFrameUpdates(_pendingFrames.front());
            _pendingFrames.pop();
        }
        _presentPipeline.reset();
        _gpuContext.reset();
    }

    void renderFrame(gpu::FramePointer& frame) {
        ++_presentCount;
        _context.makeCurrent();
        _backend->recycle();
        _backend->syncCache();
        if (frame && !frame->batches.empty()) {
            _gpuContext->executeFrame(frame);

            {
                
                auto geometryCache = DependencyManager::get<GeometryCache>();
                gpu::Batch presentBatch;
                presentBatch.setViewportTransform({ 0, 0, _size.width(), _size.height() });
                presentBatch.enableStereo(false);
                presentBatch.resetViewTransform();
                presentBatch.setFramebuffer(gpu::FramebufferPointer());
                presentBatch.setResourceTexture(0, frame->framebuffer->getRenderBuffer(0));
                presentBatch.setPipeline(_presentPipeline);
                presentBatch.draw(gpu::TRIANGLE_STRIP, 4);
                _gpuContext->executeBatch(presentBatch);
            }
            (void)CHECK_GL_ERROR();
        }
        _context.makeCurrent();
        _context.swapBuffers();
        _fpsCounter.increment();
        static size_t _frameCount{ 0 };
        ++_frameCount;
        if (_elapsed.elapsed() >= 500) {
            _fps = _fpsCounter.rate();
            _frameCount = 0;
            _elapsed.restart();
        }
        (void)CHECK_GL_ERROR();
        _context.doneCurrent();
    }

    void report() {
        uint64_t total = 0;
        for (const auto& t : _frameTimes) {
            total += t;
        }
        auto averageFrameTime = total / FRAME_TIME_BUFFER_SIZE;
        qDebug() << "Average frame " << averageFrameTime;

        std::list<uint64_t> sortedHighFrames;
        for (const auto& t : _frameTimes) {
            if (t > averageFrameTime * 6) {
                sortedHighFrames.push_back(t);
            }
        }

        sortedHighFrames.sort();
        for (const auto& t : sortedHighFrames) {
            qDebug() << "Long frame " << t;
        }
    }

    bool process() override {
        std::queue<gpu::FramePointer> pendingFrames;
        {
            std::unique_lock<std::mutex> lock(_frameLock);
            pendingFrames.swap(_pendingFrames);
        }

        while (!pendingFrames.empty()) {
            _activeFrame = pendingFrames.front();
            if (_activeFrame) {
                _gpuContext->consumeFrameUpdates(_activeFrame);
            }
            pendingFrames.pop();
        }

        if (!_activeFrame) {
            QThread::msleep(1);
            return true;
        }

        {
            auto start = usecTimestampNow();
            renderFrame(_activeFrame);
            auto duration = usecTimestampNow() - start;
            auto frameBufferIndex = _frameIndex % FRAME_TIME_BUFFER_SIZE;
            _frameTimes[frameBufferIndex] = duration;
            ++_frameIndex;
            if (0 == _frameIndex % FRAME_TIME_BUFFER_SIZE) {
                report();
            }
        }
        return true;
    }
};

// Background Render Data & rendering functions
class BackgroundRenderData {
public:
    typedef render::Payload<BackgroundRenderData> Payload;
    typedef Payload::DataPointer Pointer;
    static render::ItemID _item; // unique WorldBoxRenderData
};

render::ItemID BackgroundRenderData::_item = 0;
QSharedPointer<FileLogger> logger;

namespace render {
    template <> const ItemKey payloadGetKey(const BackgroundRenderData::Pointer& stuff) {
        return ItemKey::Builder::background();
    }

    template <> const Item::Bound payloadGetBound(const BackgroundRenderData::Pointer& stuff) {
        return Item::Bound();
    }

    template <> void payloadRender(const BackgroundRenderData::Pointer& background, RenderArgs* args) {
        Q_ASSERT(args->_batch);
        gpu::Batch& batch = *args->_batch;

        // Background rendering decision
        auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
        auto backgroundMode = skyStage->getBackgroundMode();

        switch (backgroundMode) {
        case model::SunSkyStage::SKY_BOX: {
            auto skybox = skyStage->getSkybox();
            if (skybox) {
                PerformanceTimer perfTimer("skybox");
                skybox->render(batch, args->getViewFrustum());
                break;
            }
        }
        default:
            // this line intentionally left blank
            break;
        }
    }
}

// Create a simple OpenGL window that renders text in various ways
class QTestWindow : public QWindow, public AbstractViewStateInterface {

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

    bool isAboutToQuit() const override { return false; }
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

    bool isHMDMode() const override {
        return false;
    }

public:
    //"/-17.2049,-8.08629,-19.4153/0,0.881994,0,-0.47126"
    static void setup() {
        DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
        DependencyManager::registerInheritance<SpatialParentFinder, ParentFinder>();
        DependencyManager::set<AddressManager>();
        DependencyManager::set<NodeList>(NodeType::Agent);
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
        installEventFilter(this);
        _camera.movementSpeed = 50.0f;
        QThreadPool::globalInstance()->setMaxThreadCount(2);
        QThread::currentThread()->setPriority(QThread::HighestPriority);
        AbstractViewStateInterface::setInstance(this);
        _octree = DependencyManager::set<EntityTreeRenderer>(false, this, nullptr);
        _octree->init();
        // Prevent web entities from rendering
        REGISTER_ENTITY_TYPE_WITH_FACTORY(Web, WebEntityItem::factory);

        DependencyManager::set<ParentFinder>(_octree->getTree());
        getEntities()->setViewFrustum(_viewFrustum);
        auto nodeList = DependencyManager::get<LimitedNodeList>();
        NodePermissions permissions;
        permissions.setAll(true);
        nodeList->setPermissions(permissions);

        ResourceManager::init();

        setFlags(Qt::MSWindowsOwnDC | Qt::Window | Qt::Dialog | Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint);
        _size = QSize(800, 600);
        _renderThread._size = _size;
        setGeometry(QRect(QPoint(), _size));
        create();
        show();
        QCoreApplication::processEvents();
        // Create the initial context
        _renderThread.initialize(this, _initContext);
        _initContext.makeCurrent();

        // FIXME use a wait condition
        QThread::msleep(1000);
        _renderThread.submitFrame(gpu::FramePointer());
        _initContext.makeCurrent();
        // Render engine init
        _renderEngine->addJob<RenderShadowTask>("RenderShadowTask", _cullFunctor);
        _renderEngine->addJob<RenderDeferredTask>("RenderDeferredTask", _cullFunctor);
        _renderEngine->load();
        _renderEngine->registerScene(_main3DScene);

        // Render engine library init
        reloadScene();
        restorePosition();

        QTimer* timer = new QTimer(this);
        timer->setInterval(0);
        connect(timer, &QTimer::timeout, this, [this] {
            draw();
        });
        timer->start();
        _ready = true;
    }

    virtual ~QTestWindow() {
        getEntities()->shutdown(); // tell the entities system we're shutting down, so it will stop running scripts
        _renderEngine.reset();
        _main3DScene.reset();
        EntityTreePointer tree = getEntities()->getTree();
        tree->setSimulation(nullptr);
        DependencyManager::destroy<AnimationCache>();
        DependencyManager::destroy<FramebufferCache>();
        DependencyManager::destroy<TextureCache>();
        DependencyManager::destroy<ModelCache>();
        DependencyManager::destroy<GeometryCache>();
        DependencyManager::destroy<ScriptCache>();
        ResourceManager::cleanup();
        // remove the NodeList from the DependencyManager
        DependencyManager::destroy<NodeList>();
    }

    void loadCommands(const QString& filename) {
        QFileInfo fileInfo(filename);
        if (!fileInfo.exists()) {
            return;
        }
        _commandPath = fileInfo.absolutePath();
        _commands = FileUtils::readLines(filename);
        _commandIndex = 0;
    }

protected:

    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Close) {
            _renderThread.terminate();
        }

        return QWindow::eventFilter(obj, event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        switch (event->key()) {
        case Qt::Key_F1:
            importScene();
            return;

        case Qt::Key_F2:
            reloadScene();
            return;

        case Qt::Key_F4:
            cycleMode();
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

        case Qt::Key_Home:
            gpu::Texture::setAllowedGPUMemoryUsage(0);
            return;

        case Qt::Key_End:
            gpu::Texture::setAllowedGPUMemoryUsage(MB_TO_BYTES(256));
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

    uint16_t _fps;
    void draw() {
        if (!_ready) {
            return;
        }
        if (!isVisible()) {
            return;
        }
        if (_renderCount.load() != 0 && _renderCount.load() >= _renderThread._presentCount.load()) {
            return;
        }
        _renderCount = _renderThread._presentCount.load();
        update();

        RenderArgs renderArgs(_renderThread._gpuContext, _octree, DEFAULT_OCTREE_SIZE_SCALE,
            0, RenderArgs::DEFAULT_RENDER_MODE,
            RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);


        QSize windowSize = _size;
        if (_renderMode == NORMAL) {
            renderArgs._context->enableStereo(false);
        } else {
            renderArgs._context->enableStereo(true);
            mat4 eyeOffsets[2];
            mat4 eyeProjections[2];
            if (_renderMode == STEREO) {
                for (size_t i = 0; i < 2; ++i) {
                    eyeProjections[i] = _viewFrustum.getProjection();
                }
            } else if (_renderMode == HMD) {
                eyeOffsets[0][3] = vec4 { -0.0327499993, 0.0, 0.0149999997, 1.0 };
                eyeOffsets[1][3] = vec4 { 0.0327499993, 0.0, 0.0149999997, 1.0 };
                eyeProjections[0][0] = vec4 { 0.759056330, 0.000000000, 0.000000000, 0.000000000 };
                eyeProjections[0][1] = vec4 { 0.000000000, 0.682773232, 0.000000000, 0.000000000 };
                eyeProjections[0][2] = vec4 { -0.0580431037, -0.00619550655, -1.00000489, -1.00000000 };
                eyeProjections[0][3] = vec4 { 0.000000000, 0.000000000, -0.0800003856, 0.000000000 };
                eyeProjections[1][0] = vec4 { 0.752847493, 0.000000000, 0.000000000, 0.000000000 };
                eyeProjections[1][1] = vec4 { 0.000000000, 0.678060353, 0.000000000, 0.000000000 };
                eyeProjections[1][2] = vec4 { 0.0578232110, -0.00669418881, -1.00000489, -1.000000000 };
                eyeProjections[1][3] = vec4 { 0.000000000, 0.000000000, -0.0800003856, 0.000000000 };
                windowSize = { 2048, 2048 };
            }
            renderArgs._context->setStereoProjections(eyeProjections);
            renderArgs._context->setStereoViews(eyeOffsets);
        }

        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        framebufferCache->setFrameBufferSize(windowSize);
        
        renderArgs._blitFramebuffer = framebufferCache->getFramebuffer();
        // Viewport is assigned to the size of the framebuffer
        renderArgs._viewport = ivec4(0, 0, windowSize.width(), windowSize.height());
        renderArgs.setViewFrustum(_viewFrustum);

        // Final framebuffer that will be handled to the display-plugin
        render(&renderArgs);

        if (_fps != _renderThread._fps) {
            _fps = _renderThread._fps;
            updateText();
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
        QString title = QString("FPS %1 Culling %2 TextureMemory GPU %3 CPU %4 Max GPU %5")
            .arg(_fps).arg(_cullingEnabled)
            .arg(toHumanSize(gpu::Context::getTextureGPUMemoryUsage(), 2))
            .arg(toHumanSize(gpu::Texture::getTextureCPUMemoryUsage(), 2))
            .arg(toHumanSize(gpu::Texture::getAllowedGPUMemoryUsage(), 2));
        setTitle(title);
#if 0
        {
            _textBlocks.erase(TextBlock::Info);
            auto& infoTextBlock = _textBlocks[TextBlock::Info];
            infoTextBlock.push_back({ vec2(98, 10), "FPS: ", TextOverlay::alignRight });
            infoTextBlock.push_back({ vec2(100, 10), std::to_string((uint32_t)_fps), TextOverlay::alignLeft });
            infoTextBlock.push_back({ vec2(98, 30), "Culling: ", TextOverlay::alignRight });
            infoTextBlock.push_back({ vec2(100, 30), _cullingEnabled ? "Enabled" : "Disabled", TextOverlay::alignLeft });
        }
#endif

#if 0
        _textOverlay->beginTextUpdate();
        for (const auto& e : _textBlocks) {
            for (const auto& b : e.second) {
                _textOverlay->addText(b.text, b.position, b.alignment);
            }
        }
        _textOverlay->endTextUpdate();
#endif
    }

    void runCommand(const QString& command) {
        qDebug() << "Running command: " << command;
        QStringList commandParams = command.split(QRegularExpression(QString("\\s")));
        QString verb = commandParams[0].toLower();
        if (verb == "loop") {
            if (commandParams.length() > 1) {
                int maxLoops = commandParams[1].toInt();
                if (maxLoops < ++_commandLoops) {
                    qDebug() << "Exceeded loop count";
                    return;
                }
            }
            _commandIndex = 0;
        } else if (verb == "wait") {
            if (commandParams.length() < 2) {
                qDebug() << "No wait time specified";
                return;
            }
            int seconds = commandParams[1].toInt();
            _nextCommandTime = usecTimestampNow() + seconds * USECS_PER_SECOND;
        } else if (verb == "load") {
            if (commandParams.length() < 2) {
                qDebug() << "No load file specified";
                return;
            }
            QString file = commandParams[1];
            if (QFileInfo(file).isRelative()) {
                file = _commandPath + "/" + file;
            }
            if (!QFileInfo(file).exists()) {
                qDebug() << "Cannot find scene file " + file;
                return;
            }

            importScene(file);
        } else if (verb == "go") {
            if (commandParams.length() < 2) {
                qDebug() << "No destination specified for go command";
                return;
            }
            parsePath(commandParams[1]);
        } else {
            qDebug() << "Unknown command " << command;
        }
    }

    void runNextCommand(quint64 now) {
        if (_commands.empty()) {
            return;
        }

        if (_commandIndex >= _commands.size()) {
            _commands.clear();
            return;
        }

        if (now < _nextCommandTime) {
            return;
        }

        _nextCommandTime = 0;
        QString command = _commands[_commandIndex++];
        runCommand(command);
    }

    void update() {
        auto now = usecTimestampNow();
        static auto last = now;

        runNextCommand(now);

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

        getEntities()->update();

        // The pending changes collecting the changes here
        render::PendingChanges pendingChanges;

        // FIXME: Move this out of here!, Background / skybox should be driven by the enityt content just like the other entities
        // Background rendering decision
        if (!render::Item::isValidID(BackgroundRenderData::_item)) {
            auto backgroundRenderData = std::make_shared<BackgroundRenderData>();
            auto backgroundRenderPayload = std::make_shared<BackgroundRenderData::Payload>(backgroundRenderData);
            BackgroundRenderData::_item = _main3DScene->allocateID();
            pendingChanges.resetItem(BackgroundRenderData::_item, backgroundRenderPayload);
        }
        // Setup the current Zone Entity lighting
        {
            auto stage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
            DependencyManager::get<DeferredLightingEffect>()->setGlobalLight(stage->getSunLight());
        }

        {
            PerformanceTimer perfTimer("SceneProcessPendingChanges");
            _main3DScene->enqueuePendingChanges(pendingChanges);

            _main3DScene->processPendingChangesQueue();
        }

    }

    void render(RenderArgs* renderArgs) {
        auto& gpuContext = renderArgs->_context;
        gpuContext->beginFrame();
        gpu::doInBatch(gpuContext, [&](gpu::Batch& batch) {
            batch.resetStages();
        });
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
        auto frame = gpuContext->endFrame();
        frame->framebuffer = renderArgs->_blitFramebuffer;
        frame->framebufferRecycler = [](const gpu::FramebufferPointer& framebuffer){ 
            DependencyManager::get<FramebufferCache>()->releaseFramebuffer(framebuffer);
        };
        _renderThread.submitFrame(frame);
        if (!_renderThread.isThreaded()) {
            _renderThread.process();
        }
        

    }

    void resizeWindow(const QSize& size) {
        _size = size;
        _camera.setAspectRatio((float)_size.width() / (float)_size.height());
        if (!_ready) {
            return;
        }
        _renderThread._size = size;
        //_textOverlay->resize(toGlm(_size));
        //glViewport(0, 0, size.width(), size.height());
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
        _settings.setValue(LAST_SCENE_KEY, fileName);
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
        _camera.yawPitch = vec3(0);
        _camera.setPosition(vec3());
    }

    void toggleCulling() {
        _cullingEnabled = !_cullingEnabled;
    }

    void cycleMode() {
        static auto defaultProjection = Camera().matrices.perspective;
        _renderMode = (RenderMode)((_renderMode + 1) % RENDER_MODE_COUNT);
        if (_renderMode == HMD) {
            _camera.matrices.perspective[0] = vec4 { 0.759056330, 0.000000000, 0.000000000, 0.000000000 };
            _camera.matrices.perspective[1] = vec4 { 0.000000000, 0.682773232, 0.000000000, 0.000000000 };
            _camera.matrices.perspective[2] = vec4 { -0.0580431037, -0.00619550655, -1.00000489, -1.00000000 };
            _camera.matrices.perspective[3] = vec4 { 0.000000000, 0.000000000, -0.0800003856, 0.000000000 };
        } else {
            _camera.matrices.perspective = defaultProjection;
            _camera.setAspectRatio((float)_size.width() / (float)_size.height());
        }
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

    render::EnginePointer _renderEngine { new render::Engine() };
    render::ScenePointer _main3DScene { new render::Scene(glm::vec3(-0.5f * (float)TREE_SCALE), (float)TREE_SCALE) };
    QSize _size;
    QSettings _settings;

    std::atomic<size_t> _renderCount;
    gl::OffscreenContext _initContext;
    RenderThread _renderThread;
    QWindowCamera _camera;
    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    ViewFrustum _shadowViewFrustum; // current state of view frustum, perspective, orientation, etc.
    model::SunSkyStage _sunSkyStage;
    model::LightPointer _globalLight { std::make_shared<model::Light>() };
    bool _ready { false };

    QStringList _commands;
    QString _commandPath;
    int _commandLoops { 0 };
    int _commandIndex { -1 };
    uint64_t _nextCommandTime { 0 };

    //TextOverlay* _textOverlay;
    static bool _cullingEnabled;

    enum RenderMode {
        NORMAL = 0,
        STEREO,
        HMD,
        RENDER_MODE_COUNT
    };
    RenderMode _renderMode { NORMAL };
    QSharedPointer<EntityTreeRenderer> _octree;
};

bool QTestWindow::_cullingEnabled = true;

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType)type, context, message);

    if (!logMessage.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(logMessage.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
        logger->addMessage(qPrintable(logMessage + "\n"));
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
    logger.reset(new FileLogger());

    qInstallMessageHandler(messageHandler);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);
    QTestWindow::setup();
    QTestWindow window;
    //window.loadCommands("C:/Users/bdavis/Git/dreaming/exports/commands.txt");
    app.exec();
    return 0;
}

#include "main.moc"
