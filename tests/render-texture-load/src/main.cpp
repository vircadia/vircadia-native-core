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
#include <QtCore/QThreadPool>
#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <QtGui/QGuiApplication>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>


#include "GLIHelpers.h"
#include <shared/RateCounter.h>
#include <AssetClient.h>
#include <PathUtils.h>

#include <gpu/gl/GLBackend.h>
#include <gpu/gl/GLFramebuffer.h>
#include <gpu/gl/GLTexture.h>
#include <gpu/StandardShaderLib.h>

#include <AddressManager.h>
#include <NodeList.h>
#include <TextureCache.h>
#include <FramebufferCache.h>
#include <GeometryCache.h>

#include <gl/Config.h>
#include <gl/Context.h>

extern QThread* RENDER_THREAD;

static const QString DATA_SET = "https://hifi-content.s3.amazonaws.com/austin/textures.zip";
static QDir DATA_DIR = QDir(QString("h:/textures"));
static QTemporaryDir* DOWNLOAD_DIR = nullptr;

class FileDownloader : public QObject {
    Q_OBJECT
public:
    using Handler = std::function<void(const QByteArray& data)>;

    FileDownloader(QUrl url, const Handler& handler, QObject *parent = 0) : QObject(parent), _handler(handler) {
        connect(&_accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileDownloaded(QNetworkReply*)));
        _accessManager.get(QNetworkRequest(url));
    }

    void waitForDownload() {
        while (!_complete) {
            QCoreApplication::processEvents();
        }
    }

    private slots:
    void fileDownloaded(QNetworkReply* pReply) {
        _handler(pReply->readAll());
        pReply->deleteLater();
        _complete = true;
    }

private:
    QNetworkAccessManager _accessManager;
    Handler _handler;
    bool _complete { false };
};

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
    size_t _frameIndex;
    std::mutex _frameLock;
    std::queue<gpu::FramePointer> _pendingFrames;
    gpu::FramePointer _activeFrame;
    QSize _size;
    static const size_t FRAME_TIME_BUFFER_SIZE{ 1024 };

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
        // GPU library init
        gpu::Context::init<gpu::gl::GLBackend>();
        _gpuContext = std::make_shared<gpu::Context>();
        _backend = _gpuContext->getBackend();
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
        glewExperimental = true;
        glewInit();
        glGetError();

        //wglSwapIntervalEXT(0);
        _frameTimes.resize(FRAME_TIME_BUFFER_SIZE, 0);
        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::StandardShaderLib::getDrawTexturePS();
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

        std::list<std::pair<uint64_t, size_t>> sortedHighFrames;
        for (size_t i = 0; i < _frameTimes.size(); ++i) {
            const auto& t = _frameTimes[i];
            if (t > averageFrameTime * 6) {
                sortedHighFrames.push_back({ t, i } );
            }
        }

        sortedHighFrames.sort();
        for (const auto& p : sortedHighFrames) {
            qDebug() << "Long frame " << p.first << " " << p.second;
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

// Create a simple OpenGL window that renders text in various ways
class QTestWindow : public QWindow {
public:
    //"/-17.2049,-8.08629,-19.4153/0,0.881994,0,-0.47126"
    static void setup() {
        DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
        //DependencyManager::registerInheritance<SpatialParentFinder, ParentFinder>();
        DependencyManager::set<AddressManager>();
        DependencyManager::set<NodeList>(NodeType::Agent);
        DependencyManager::set<ResourceCacheSharedItems>();
        DependencyManager::set<TextureCache>();
        DependencyManager::set<FramebufferCache>();
        DependencyManager::set<GeometryCache>();
        DependencyManager::set<ModelCache>();
        DependencyManager::set<PathUtils>();
    }

    struct TextureLoad {
        uint32_t time;
        QString file;
        QString src;
    };

    QTestWindow() {

        _currentTexture = _textures.end();
        {
            QStringList stringList;
            QFile textFile(DATA_DIR.path() + "/loads.txt");
            textFile.open(QFile::ReadOnly);
            //... (open the file for reading, etc.)
            QTextStream textStream(&textFile);
            while (true) {
                QString line = textStream.readLine();
                if (line.isNull())
                    break;
                else
                    stringList.append(line);
            }

            for (QString s : stringList) {
                auto index = s.indexOf(" ");
                QString timeStr = s.left(index);
                auto time = timeStr.toUInt();
                QString path = DATA_DIR.path() + "/" + s.right(s.length() - index).trimmed();
                qDebug() << "Path " << path;
                if (!QFileInfo(path).exists()) {
                    continue;
                }
                _textureLoads.push({ time, path, s });
            }
        }

        installEventFilter(this);
        QThreadPool::globalInstance()->setMaxThreadCount(2);
        QThread::currentThread()->setPriority(QThread::HighestPriority);
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
        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::StandardShaderLib::getDrawTexturePS();
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setScissorEnable(true);
            _simplePipeline = gpu::Pipeline::create(program, state);
        }

        QTimer* timer = new QTimer(this);
        timer->setInterval(0);
        connect(timer, &QTimer::timeout, this, [this] {
            draw();
        });
        timer->start();
        _ready = true;
    }

    virtual ~QTestWindow() {
        DependencyManager::destroy<FramebufferCache>();
        DependencyManager::destroy<TextureCache>();
        DependencyManager::destroy<ModelCache>();
        DependencyManager::destroy<GeometryCache>();
        ResourceManager::cleanup();
    }

protected:

    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Close) {
            _renderThread.terminate();
        }

        return QWindow::eventFilter(obj, event);
    }

    void keyPressEvent(QKeyEvent* event) override {
    }

    void keyReleaseEvent(QKeyEvent* event) override {
    }

    void mouseMoveEvent(QMouseEvent* event) override {
    }

    void resizeEvent(QResizeEvent* ev) override {
        resizeWindow(ev->size());
    }

private:
    std::queue<TextureLoad> _textureLoads;
    std::list<gpu::TexturePointer> _textures;
    std::list<gpu::TexturePointer>::iterator _currentTexture;

    uint16_t _fps;
    gpu::PipelinePointer _simplePipeline;

    void draw() {
        if (!_ready) {
            return;
        }
        if (!isVisible()) {
            return;
        }
        if (_renderCount.load() != 0 && _renderCount.load() >= _renderThread._presentCount.load()) {
            QThread::usleep(1);
            return;
        }
        _renderCount = _renderThread._presentCount.load();
        update();

        QSize windowSize = _size;
        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        framebufferCache->setFrameBufferSize(windowSize);

        // Final framebuffer that will be handled to the display-plugin
        render();

        if (_fps != _renderThread._fps) {
            _fps = _renderThread._fps;
            updateText();
        }
    }

    void updateText() {
        setTitle(QString("FPS %1").arg(_fps));
    }

    void update() {
        auto now = usecTimestampNow();
        static auto last = now;
        auto delta = (now - last) / USECS_PER_MSEC;
        if (!_textureLoads.empty()) {
            const auto& front = _textureLoads.front();
            if (delta >= front.time) {
                QFileInfo fileInfo(front.file);
                if (!fileInfo.exists()) {
                    qDebug() << "Missing file " << front.file;
                } else {
                    qDebug() << "Loading " << front.src;
                    auto file = front.file.toLocal8Bit().toStdString();
                    processTexture(file.c_str());
                    _textures.push_back(DependencyManager::get<TextureCache>()->getImageTexture(front.file));
                }
                _textureLoads.pop();
                if (_textureLoads.empty()) {
                    qDebug() << "Done";
                }
            }
        }
    }

    void render() {
        auto& gpuContext = _renderThread._gpuContext;
        gpuContext->beginFrame();
        gpu::doInBatch(gpuContext, [&](gpu::Batch& batch) {
            batch.resetStages();
        });
        PROFILE_RANGE(__FUNCTION__);
        auto framebuffer = DependencyManager::get<FramebufferCache>()->getFramebuffer();

        gpu::doInBatch(gpuContext, [&](gpu::Batch& batch) {
            batch.enableStereo(false);
            batch.setFramebuffer(framebuffer);
            batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, vec4(1, 0, 0, 1));
            auto vpsize = framebuffer->getSize();
            auto vppos = ivec2(0);
            batch.setViewportTransform(ivec4(vppos, vpsize));
            if (_currentTexture != _textures.end()) {
                ++_currentTexture;
            }
            if (_currentTexture == _textures.end()) {
                _currentTexture = _textures.begin();
            }
            if (_currentTexture != _textures.end()) {
                batch.setResourceTexture(0, *_currentTexture);
            }
            batch.setPipeline(_simplePipeline);
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        });

        auto frame = gpuContext->endFrame();
        frame->framebuffer = framebuffer;
        frame->framebufferRecycler = [](const gpu::FramebufferPointer& framebuffer) {
            DependencyManager::get<FramebufferCache>()->releaseFramebuffer(framebuffer);
        };
        _renderThread.submitFrame(frame);
        if (!_renderThread.isThreaded()) {
            _renderThread.process();
        }
    }

    void resizeWindow(const QSize& size) {
        _size = size;
        if (!_ready) {
            return;
        }
        _renderThread._size = size;
    }

private:
    QSize _size;
    std::atomic<size_t> _renderCount;
    gl::OffscreenContext _initContext;
    RenderThread _renderThread;
    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    bool _ready { false };
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

void unzipTestData(const QByteArray& zipData) {
    DOWNLOAD_DIR = new QTemporaryDir();
    QTemporaryDir& tempDir = *DOWNLOAD_DIR;
    QTemporaryFile zipFile;

    if (zipFile.open()) {
        zipFile.write(zipData);
        zipFile.close();
    }
    qDebug() << zipFile.fileName();
    if (!tempDir.isValid()) {
        qFatal("Unable to create temp dir");
    }
    DATA_DIR = QDir(tempDir.path());

    //auto files = JlCompress::getFileList(zipData);
    auto files = JlCompress::extractDir(zipFile.fileName(), DATA_DIR.path());
    qDebug() << DATA_DIR.path();
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("RenderPerf");
    QCoreApplication::setOrganizationName("High Fidelity");
    QCoreApplication::setOrganizationDomain("highfidelity.com");
    qInstallMessageHandler(messageHandler);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);

    if (!DATA_DIR.exists()) {
        FileDownloader(DATA_SET, [&](const QByteArray& data) {
            qDebug() << "Fetched size " << data.size();
            unzipTestData(data);
        }).waitForDownload();
    }


    QTestWindow::setup();
    QTestWindow window;
    app.exec();
    return 0;
}

#include "main.moc"
