//
//  Application.cpp
//  interface/src
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include <gl/Config.h>
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Qt>
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QMimeData>
#include <QtCore/QThreadPool>

#include <QtGui/QScreen>
#include <QtGui/QImage>
#include <QtGui/QWheelEvent>
#include <QtGui/QWindow>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QDesktopServices>

#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QLocalServer>

#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickWindow>

#include <QtWidgets/QActionGroup>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>

#include <QtMultimedia/QMediaPlayer>

#include <QtNetwork/QNetworkDiskCache>

#include <gl/Config.h>
#include <gl/QOpenGLContextWrapper.h>

#include <shared/JSONHelpers.h>

#include <ResourceScriptingInterface.h>
#include <AccountManager.h>
#include <AddressManager.h>
#include <BuildInfo.h>
#include <AssetClient.h>
#include <AssetUpload.h>
#include <AutoUpdater.h>
#include <AudioInjectorManager.h>
#include <CursorManager.h>
#include <DeferredLightingEffect.h>
#include <display-plugins/DisplayPlugin.h>
#include <EntityScriptingInterface.h>
#include <ErrorDialog.h>
#include <Finally.h>
#include <FramebufferCache.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <gpu/GLBackend.h>
#include <HFActionEvent.h>
#include <HFBackEvent.h>
#include <InfoView.h>
#include <input-plugins/InputPlugin.h>
#include <controllers/UserInputMapper.h>
#include <controllers/StateController.h>
#include <LogHandler.h>
#include <MainWindow.h>
#include <MessagesClient.h>
#include <ModelEntityItem.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <ObjectMotionState.h>
#include <OctalCode.h>
#include <OctreeSceneStats.h>
#include <OffscreenUi.h>
#include <gl/OffscreenGLCanvas.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <PhysicsEngine.h>
#include <PhysicsHelpers.h>
#include <plugins/PluginContainer.h>
#include <plugins/PluginManager.h>
#include <RenderableWebEntityItem.h>
#include <RenderShadowTask.h>
#include <RenderDeferredTask.h>
#include <ResourceCache.h>
#include <SceneScriptingInterface.h>
#include <RecordingScriptingInterface.h>
#include <ScriptCache.h>
#include <SoundCache.h>
#include <ScriptEngines.h>
#include <TextureCache.h>
#include <Tooltip.h>
#include <udt/PacketHeaders.h>
#include <UserActivityLogger.h>
#include <UUID.h>
#include <VrMenu.h>
#include <recording/Deck.h>
#include <recording/Recorder.h>
#include <QmlWebWindowClass.h>
#include <Preferences.h>
#include <display-plugins/CompositorHelper.h>

#include "AnimDebugDraw.h"
#include "AudioClient.h"
#include "audio/AudioScope.h"
#include "avatar/AvatarManager.h"
#include "CrashHandler.h"
#include "input-plugins/SpacemouseManager.h"
#include "devices/DdeFaceTracker.h"
#include "devices/EyeTracker.h"
#include "devices/Faceshift.h"
#include "devices/Leapmotion.h"
#include "DiscoverabilityManager.h"
#include "GLCanvas.h"
#include "InterfaceActionFactory.h"
#include "InterfaceLogging.h"
#include "LODManager.h"
#include "Menu.h"
#include "ModelPackager.h"
#include "PluginContainerProxy.h"
#include "scripting/AccountScriptingInterface.h"
#include "scripting/AssetMappingsScriptingInterface.h"
#include "scripting/AudioDeviceScriptingInterface.h"
#include "scripting/ClipboardScriptingInterface.h"
#include "scripting/DesktopScriptingInterface.h"
#include "scripting/GlobalServicesScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "scripting/LocationScriptingInterface.h"
#include "scripting/MenuScriptingInterface.h"
#include "scripting/SettingsScriptingInterface.h"
#include "scripting/WebWindowClass.h"
#include "scripting/WindowScriptingInterface.h"
#include "scripting/ControllerScriptingInterface.h"
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif
#include "Stars.h"
#include "ui/AddressBarDialog.h"
#include "ui/AvatarInputs.h"
#include "ui/DialogsManager.h"
#include "ui/LoginDialog.h"
#include "ui/overlays/Cube3DOverlay.h"
#include "ui/Snapshot.h"
#include "ui/StandAloneJSConsole.h"
#include "ui/Stats.h"
#include "ui/UpdateDialog.h"
#include "Util.h"
#include "InterfaceParentFinder.h"



// ON WIndows PC, NVidia Optimus laptop, we want to enable NVIDIA GPU
// FIXME seems to be broken.
#if defined(Q_OS_WIN)
extern "C" {
 _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

using namespace std;

static QTimer locationUpdateTimer;
static QTimer balanceUpdateTimer;
static QTimer identityPacketTimer;
static QTimer pingTimer;

static const QString SNAPSHOT_EXTENSION  = ".jpg";
static const QString SVO_EXTENSION  = ".svo";
static const QString SVO_JSON_EXTENSION  = ".svo.json";
static const QString JS_EXTENSION  = ".js";
static const QString FST_EXTENSION  = ".fst";
static const QString FBX_EXTENSION  = ".fbx";
static const QString OBJ_EXTENSION  = ".obj";
static const QString AVA_JSON_EXTENSION = ".ava.json";

static const int MSECS_PER_SEC = 1000;
static const int MIRROR_VIEW_TOP_PADDING = 5;
static const int MIRROR_VIEW_LEFT_PADDING = 10;
static const int MIRROR_VIEW_WIDTH = 265;
static const int MIRROR_VIEW_HEIGHT = 215;
static const float MIRROR_FULLSCREEN_DISTANCE = 0.389f;
static const float MIRROR_REARVIEW_DISTANCE = 0.722f;
static const float MIRROR_REARVIEW_BODY_DISTANCE = 2.56f;
static const float MIRROR_FIELD_OF_VIEW = 30.0f;

static const quint64 TOO_LONG_SINCE_LAST_SEND_DOWNSTREAM_AUDIO_STATS = 1 * USECS_PER_SECOND;

static const QString INFO_HELP_PATH = "html/interface-welcome.html";
static const QString INFO_EDIT_ENTITIES_PATH = "html/edit-commands.html";

static const unsigned int THROTTLED_SIM_FRAMERATE = 15;
static const int THROTTLED_SIM_FRAME_PERIOD_MS = MSECS_PER_SECOND / THROTTLED_SIM_FRAMERATE;

static const uint32_t INVALID_FRAME = UINT32_MAX;

static const float PHYSICS_READY_RANGE = 3.0f; // how far from avatar to check for entities that aren't ready for simulation

#ifndef __APPLE__
static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#else
// Temporary fix to Qt bug: http://stackoverflow.com/questions/16194475
static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).append("/script.js");
#endif

const QString DEFAULT_SCRIPTS_JS_URL = "http://s3.amazonaws.com/hifi-public/scripts/defaultScripts.js";
Setting::Handle<int> maxOctreePacketsPerSecond("maxOctreePPS", DEFAULT_MAX_OCTREE_PPS);

const QHash<QString, Application::AcceptURLMethod> Application::_acceptedExtensions {
    { SNAPSHOT_EXTENSION, &Application::acceptSnapshot },
    { SVO_EXTENSION, &Application::importSVOFromURL },
    { SVO_JSON_EXTENSION, &Application::importSVOFromURL },
    { JS_EXTENSION, &Application::askToLoadScript },
    { FST_EXTENSION, &Application::askToSetAvatarUrl },
    { AVA_JSON_EXTENSION, &Application::askToWearAvatarAttachmentUrl }
};

class DeadlockWatchdogThread : public QThread {
public:
    static const unsigned long HEARTBEAT_CHECK_INTERVAL_SECS = 1;
    static const unsigned long HEARTBEAT_UPDATE_INTERVAL_SECS = 1;
    static const unsigned long HEARTBEAT_REPORT_INTERVAL_USECS = 5 * USECS_PER_SECOND;
    static const unsigned long MAX_HEARTBEAT_AGE_USECS = 30 * USECS_PER_SECOND;
    static const int WARNING_ELAPSED_HEARTBEAT = 500 * USECS_PER_MSEC; // warn if elapsed heartbeat average is large
    static const int HEARTBEAT_SAMPLES = 100000; // ~5 seconds worth of samples
    
    // Set the heartbeat on launch
    DeadlockWatchdogThread() {
        setObjectName("Deadlock Watchdog");
        // Give the heartbeat an initial value
        _heartbeat = usecTimestampNow();
        connect(qApp, &QCoreApplication::aboutToQuit, [this] {
            _quit = true;
        });
    }

    void updateHeartbeat() {
        auto now = usecTimestampNow();
        auto elapsed = now - _heartbeat;
        _movingAverage.addSample(elapsed);
        _heartbeat = now;
    }

    void deadlockDetectionCrash() {
        uint32_t* crashTrigger = nullptr;
        *crashTrigger = 0xDEAD10CC;
    }

    void run() override {
        while (!_quit) {
            QThread::sleep(HEARTBEAT_UPDATE_INTERVAL_SECS);

            uint64_t lastHeartbeat = _heartbeat; // sample atomic _heartbeat, because we could context switch away and have it updated on us
            uint64_t now = usecTimestampNow();
            auto lastHeartbeatAge = (now > lastHeartbeat) ? now - lastHeartbeat : 0;
            auto sinceLastReport = (now > _lastReport) ? now - _lastReport : 0;
            auto elapsedMovingAverage = _movingAverage.getAverage();

            if (elapsedMovingAverage > _maxElapsedAverage) {
                qDebug() << "DEADLOCK WATCHDOG NEW maxElapsedAverage:"
                    << "lastHeartbeatAge:" << lastHeartbeatAge
                    << "elapsedMovingAverage:" << elapsedMovingAverage
                    << "maxElapsed:" << _maxElapsed
                    << "PREVIOUS maxElapsedAverage:" << _maxElapsedAverage
                    << "NEW maxElapsedAverage:" << elapsedMovingAverage
                    << "samples:" << _movingAverage.getSamples();
                _maxElapsedAverage = elapsedMovingAverage;
            }
            if (lastHeartbeatAge > _maxElapsed) {
                qDebug() << "DEADLOCK WATCHDOG NEW maxElapsed:"
                    << "lastHeartbeatAge:" << lastHeartbeatAge
                    << "elapsedMovingAverage:" << elapsedMovingAverage
                    << "PREVIOUS maxElapsed:" << _maxElapsed
                    << "NEW maxElapsed:" << lastHeartbeatAge
                    << "maxElapsedAverage:" << _maxElapsedAverage
                    << "samples:" << _movingAverage.getSamples();
                _maxElapsed = lastHeartbeatAge;
            }
            if ((sinceLastReport > HEARTBEAT_REPORT_INTERVAL_USECS) || (elapsedMovingAverage > WARNING_ELAPSED_HEARTBEAT)) {
                qDebug() << "DEADLOCK WATCHDOG STATUS -- lastHeartbeatAge:" << lastHeartbeatAge
                         << "elapsedMovingAverage:" << elapsedMovingAverage
                         << "maxElapsed:" << _maxElapsed
                         << "maxElapsedAverage:" << _maxElapsedAverage
                         << "samples:" << _movingAverage.getSamples();
                _lastReport = now;
            }

#ifdef NDEBUG
            if (lastHeartbeatAge > MAX_HEARTBEAT_AGE_USECS) {
                qDebug() << "DEADLOCK DETECTED -- "
                         << "lastHeartbeatAge:" << lastHeartbeatAge
                         << "[ lastHeartbeat :" << lastHeartbeat
                         << "now:" << now << " ]"
                         << "elapsedMovingAverage:" << elapsedMovingAverage
                         << "maxElapsed:" << _maxElapsed
                         << "maxElapsedAverage:" << _maxElapsedAverage
                         << "samples:" << _movingAverage.getSamples();
                deadlockDetectionCrash();
            }
#endif
        }
    }

    static std::atomic<uint64_t> _heartbeat;
    static std::atomic<uint64_t> _lastReport;
    static std::atomic<uint64_t> _maxElapsed;
    static std::atomic<int> _maxElapsedAverage;
    static ThreadSafeMovingAverage<int, HEARTBEAT_SAMPLES> _movingAverage;

    bool _quit { false };
};

std::atomic<uint64_t> DeadlockWatchdogThread::_heartbeat;
std::atomic<uint64_t> DeadlockWatchdogThread::_lastReport;
std::atomic<uint64_t> DeadlockWatchdogThread::_maxElapsed;
std::atomic<int> DeadlockWatchdogThread::_maxElapsedAverage;
ThreadSafeMovingAverage<int, DeadlockWatchdogThread::HEARTBEAT_SAMPLES> DeadlockWatchdogThread::_movingAverage;

#ifdef Q_OS_WIN
class MyNativeEventFilter : public QAbstractNativeEventFilter {
public:
    static MyNativeEventFilter& getInstance() {
        static MyNativeEventFilter staticInstance;
        return staticInstance;
    }

    bool nativeEventFilter(const QByteArray &eventType, void* msg, long* result) Q_DECL_OVERRIDE {
        if (eventType == "windows_generic_MSG") {
            MSG* message = (MSG*)msg;

            if (message->message == UWM_IDENTIFY_INSTANCES) {
                *result = UWM_IDENTIFY_INSTANCES;
                return true;
            }

            if (message->message == UWM_SHOW_APPLICATION) {
                MainWindow* applicationWindow = qApp->getWindow();
                if (applicationWindow->isMinimized()) {
                    applicationWindow->showNormal();  // Restores to windowed or maximized state appropriately.
                }
                qApp->setActiveWindow(applicationWindow);  // Flashes the taskbar icon if not focus.
                return true;
            }

            if (message->message == WM_COPYDATA) {
                COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)(message->lParam);
                QUrl url = QUrl((const char*)(pcds->lpData));
                if (url.isValid() && url.scheme() == HIFI_URL_SCHEME) {
                    DependencyManager::get<AddressManager>()->handleLookupString(url.toString());
                    return true;
                }
            }
        }
        return false;
    }
};
#endif

enum CustomEventTypes {
    Lambda = QEvent::User + 1,
    Paint = Lambda + 1,
};

class LambdaEvent : public QEvent {
    std::function<void()> _fun;
public:
    LambdaEvent(const std::function<void()> & fun) :
    QEvent(static_cast<QEvent::Type>(Lambda)), _fun(fun) {
    }
    LambdaEvent(std::function<void()> && fun) :
    QEvent(static_cast<QEvent::Type>(Lambda)), _fun(fun) {
    }
    void call() { _fun(); }
};

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType) type, context, message);

    if (!logMessage.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(logMessage.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
        qApp->getLogger()->addMessage(qPrintable(logMessage + "\n"));
    }
}

bool setupEssentials(int& argc, char** argv) {
    unsigned int listenPort = 0; // bind to an ephemeral port by default
    const char** constArgv = const_cast<const char**>(argv);
    const char* portStr = getCmdOption(argc, constArgv, "--listenPort");
    if (portStr) {
        listenPort = atoi(portStr);
    }
    // Set build version
    QCoreApplication::setApplicationVersion(BuildInfo::VERSION);

    Setting::preInit();

    bool previousSessionCrashed = CrashHandler::checkForAndHandleCrash();
    CrashHandler::writeRunningMarkerFiler();
    qAddPostRoutine(CrashHandler::deleteRunningMarkerFile);

    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    DependencyManager::registerInheritance<AvatarHashMap, AvatarManager>();
    DependencyManager::registerInheritance<EntityActionFactoryInterface, InterfaceActionFactory>();
    DependencyManager::registerInheritance<SpatialParentFinder, InterfaceParentFinder>();

    Setting::init();

    // Set dependencies
    DependencyManager::set<ScriptEngines>();
    DependencyManager::set<Preferences>();
    DependencyManager::set<recording::Deck>();
    DependencyManager::set<recording::Recorder>();
    DependencyManager::set<AddressManager>();
    DependencyManager::set<NodeList>(NodeType::Agent, listenPort);
    DependencyManager::set<GeometryCache>();
    DependencyManager::set<ModelCache>();
    DependencyManager::set<ScriptCache>();
    DependencyManager::set<SoundCache>();
    DependencyManager::set<Faceshift>();
    DependencyManager::set<DdeFaceTracker>();
    DependencyManager::set<EyeTracker>();
    DependencyManager::set<AudioClient>();
    DependencyManager::set<AudioScope>();
    DependencyManager::set<DeferredLightingEffect>();
    DependencyManager::set<TextureCache>();
    DependencyManager::set<FramebufferCache>();
    DependencyManager::set<AnimationCache>();
    DependencyManager::set<ModelBlender>();
    DependencyManager::set<AvatarManager>();
    DependencyManager::set<LODManager>();
    DependencyManager::set<StandAloneJSConsole>();
    DependencyManager::set<DialogsManager>();
    DependencyManager::set<BandwidthRecorder>();
    DependencyManager::set<ResourceCacheSharedItems>();
    DependencyManager::set<DesktopScriptingInterface>();
    DependencyManager::set<EntityScriptingInterface>(true);
    DependencyManager::set<RecordingScriptingInterface>();
    DependencyManager::set<WindowScriptingInterface>();
    DependencyManager::set<HMDScriptingInterface>();
    DependencyManager::set<ResourceScriptingInterface>();


#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    DependencyManager::set<SpeechRecognizer>();
#endif
    DependencyManager::set<DiscoverabilityManager>();
    DependencyManager::set<SceneScriptingInterface>();
    DependencyManager::set<OffscreenUi>();
    DependencyManager::set<AutoUpdater>();
    DependencyManager::set<PathUtils>();
    DependencyManager::set<InterfaceActionFactory>();
    DependencyManager::set<AudioInjectorManager>();
    DependencyManager::set<MessagesClient>();
    DependencyManager::set<UserInputMapper>();
    DependencyManager::set<controller::ScriptingInterface, ControllerScriptingInterface>();
    DependencyManager::set<InterfaceParentFinder>();
    DependencyManager::set<EntityTreeRenderer>(true, qApp, qApp);
    DependencyManager::set<CompositorHelper>();
    return previousSessionCrashed;
}

// FIXME move to header, or better yet, design some kind of UI manager
// to take care of highlighting keyboard focused items, rather than
// continuing to overburden Application.cpp
Cube3DOverlay* _keyboardFocusHighlight{ nullptr };
int _keyboardFocusHighlightID{ -1 };
PluginContainer* _pluginContainer;


// FIXME hack access to the internal share context for the Chromium helper
// Normally we'd want to use QWebEngine::initialize(), but we can't because
// our primary context is a QGLWidget, which can't easily be initialized to share
// from a QOpenGLContext.
//
// So instead we create a new offscreen context to share with the QGLWidget,
// and manually set THAT to be the shared context for the Chromium helper
OffscreenGLCanvas* _chromiumShareContext { nullptr };
Q_GUI_EXPORT void qt_gl_set_global_share_context(QOpenGLContext *context);

Setting::Handle<int> sessionRunTime{ "sessionRunTime", 0 };

Application::Application(int& argc, char** argv, QElapsedTimer& startupTimer) :
    QApplication(argc, argv),
    _window(new MainWindow(desktop())),
    _sessionRunTimer(startupTimer),
    _previousSessionCrashed(setupEssentials(argc, argv)),
    _undoStackScriptingInterface(&_undoStack),
    _frameCount(0),
    _fps(60.0f),
    _physicsEngine(new PhysicsEngine(Vectors::ZERO)),
    _entityClipboardRenderer(false, this, this),
    _entityClipboard(new EntityTree()),
    _lastQueriedTime(usecTimestampNow()),
    _mirrorViewRect(QRect(MIRROR_VIEW_LEFT_PADDING, MIRROR_VIEW_TOP_PADDING, MIRROR_VIEW_WIDTH, MIRROR_VIEW_HEIGHT)),
    _previousScriptLocation("LastScriptLocation", DESKTOP_LOCATION),
    _fieldOfView("fieldOfView", DEFAULT_FIELD_OF_VIEW_DEGREES),
    _scaleMirror(1.0f),
    _rotateMirror(0.0f),
    _raiseMirror(0.0f),
    _enableProcessOctreeThread(true),
    _lastNackTime(usecTimestampNow()),
    _lastSendDownstreamAudioStats(usecTimestampNow()),
    _aboutToQuit(false),
    _notifiedPacketVersionMismatchThisDomain(false),
    _maxOctreePPS(maxOctreePacketsPerSecond.get()),
    _lastFaceTrackerUpdate(0)
{
    // FIXME this may be excessivly conservative.  On the other hand
    // maybe I'm used to having an 8-core machine
    // Perhaps find the ideal thread count  and subtract 2 or 3
    // (main thread, present thread, random OS load)
    // More threads == faster concurrent loads, but also more concurrent
    // load on the GPU until we can serialize GPU transfers (off the main thread)
    QThreadPool::globalInstance()->setMaxThreadCount(2);
    thread()->setPriority(QThread::HighPriority);
    thread()->setObjectName("Main Thread");

    setInstance(this);

    auto controllerScriptingInterface = DependencyManager::get<controller::ScriptingInterface>().data();
    _controllerScriptingInterface = dynamic_cast<ControllerScriptingInterface*>(controllerScriptingInterface);

    _entityClipboard->createRootElement();

    _pluginContainer = new PluginContainerProxy();
#ifdef Q_OS_WIN
    installNativeEventFilter(&MyNativeEventFilter::getInstance());
#endif

    _logger = new FileLogger(this);  // After setting organization name in order to get correct directory

    qInstallMessageHandler(messageHandler);

    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "styles/Inconsolata.otf");
    _window->setWindowTitle("Interface");

    Model::setAbstractViewStateInterface(this); // The model class will sometimes need to know view state details from us

    auto nodeList = DependencyManager::get<NodeList>();

    // Set up a watchdog thread to intentionally crash the application on deadlocks
    _deadlockWatchdogThread = new DeadlockWatchdogThread();
    _deadlockWatchdogThread->start();

    qCDebug(interfaceapp) << "[VERSION] Build sequence:" << qPrintable(applicationVersion());

    _bookmarks = new Bookmarks();  // Before setting up the menu

    // start the nodeThread so its event loop is running
    QThread* nodeThread = new QThread(this);
    nodeThread->setObjectName("NodeList Thread");
    nodeThread->start();

    // make sure the node thread is given highest priority
    nodeThread->setPriority(QThread::TimeCriticalPriority);

    // setup a timer for domain-server check ins
    QTimer* domainCheckInTimer = new QTimer(nodeList.data());
    connect(domainCheckInTimer, &QTimer::timeout, nodeList.data(), &NodeList::sendDomainServerCheckIn);
    domainCheckInTimer->start(DOMAIN_SERVER_CHECK_IN_MSECS);

    // put the NodeList and datagram processing on the node thread
    nodeList->moveToThread(nodeThread);

    // Model background downloads need to happen on the Datagram Processor Thread.  The idle loop will
    // emit checkBackgroundDownloads to cause the ModelCache to check it's queue for requested background
    // downloads.
    QSharedPointer<ModelCache> modelCacheP = DependencyManager::get<ModelCache>();
    ResourceCache* modelCache = modelCacheP.data();
    connect(this, &Application::checkBackgroundDownloads, modelCache, &ResourceCache::checkAsynchronousGets);

    // put the audio processing on a separate thread
    QThread* audioThread = new QThread();
    audioThread->setObjectName("Audio Thread");

    auto audioIO = DependencyManager::get<AudioClient>();
    audioIO->setPositionGetter([this]{ return getMyAvatar()->getPositionForAudio(); });
    audioIO->setOrientationGetter([this]{ return getMyAvatar()->getOrientationForAudio(); });

    audioIO->moveToThread(audioThread);
    recording::Frame::registerFrameHandler(AudioConstants::getAudioFrameName(), [=](recording::Frame::ConstPointer frame) {
        audioIO->handleRecordedAudioInput(frame->data);
    });

    connect(audioIO.data(), &AudioClient::inputReceived, [](const QByteArray& audio){
        static auto recorder = DependencyManager::get<recording::Recorder>();
        if (recorder->isRecording()) {
            static const recording::FrameType AUDIO_FRAME_TYPE = recording::Frame::registerFrameType(AudioConstants::getAudioFrameName());
            recorder->recordFrame(AUDIO_FRAME_TYPE, audio);
        }
    });

    auto& audioScriptingInterface = AudioScriptingInterface::getInstance();
    connect(audioThread, &QThread::started, audioIO.data(), &AudioClient::start);
    connect(audioIO.data(), &AudioClient::destroyed, audioThread, &QThread::quit);
    connect(audioThread, &QThread::finished, audioThread, &QThread::deleteLater);
    connect(audioIO.data(), &AudioClient::muteToggled, this, &Application::audioMuteToggled);
    connect(audioIO.data(), &AudioClient::mutedByMixer, &audioScriptingInterface, &AudioScriptingInterface::mutedByMixer);
    connect(audioIO.data(), &AudioClient::receivedFirstPacket, &audioScriptingInterface, &AudioScriptingInterface::receivedFirstPacket);
    connect(audioIO.data(), &AudioClient::disconnected, &audioScriptingInterface, &AudioScriptingInterface::disconnected);
    connect(audioIO.data(), &AudioClient::muteEnvironmentRequested, [](glm::vec3 position, float radius) {
        auto audioClient = DependencyManager::get<AudioClient>();
        auto myAvatarPosition = DependencyManager::get<AvatarManager>()->getMyAvatar()->getPosition();
        float distance = glm::distance(myAvatarPosition, position);
        bool shouldMute = !audioClient->isMuted() && (distance < radius);

        if (shouldMute) {
            audioClient->toggleMute();
            AudioScriptingInterface::getInstance().environmentMuted();
        }
    });

    audioThread->start();

    ResourceManager::init();
    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    // Setup MessagesClient
    auto messagesClient = DependencyManager::get<MessagesClient>();
    QThread* messagesThread = new QThread;
    messagesThread->setObjectName("Messages Client Thread");
    messagesClient->moveToThread(messagesThread);
    connect(messagesThread, &QThread::started, messagesClient.data(), &MessagesClient::init);
    messagesThread->start();

    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    connect(&domainHandler, SIGNAL(hostnameChanged(const QString&)), SLOT(domainChanged(const QString&)));
    connect(&domainHandler, SIGNAL(resetting()), SLOT(resettingDomain()));
    connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(clearDomainOctreeDetails()));

    // update our location every 5 seconds in the metaverse server, assuming that we are authenticated with one
    const qint64 DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS = 5 * MSECS_PER_SEC;

    auto discoverabilityManager = DependencyManager::get<DiscoverabilityManager>();
    connect(&locationUpdateTimer, &QTimer::timeout, discoverabilityManager.data(), &DiscoverabilityManager::updateLocation);
    connect(&locationUpdateTimer, &QTimer::timeout,
        DependencyManager::get<AddressManager>().data(), &AddressManager::storeCurrentAddress);
    locationUpdateTimer.start(DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS);

    // if we get a domain change, immediately attempt update location in metaverse server
    connect(&nodeList->getDomainHandler(), &DomainHandler::connectedToDomain,
        discoverabilityManager.data(), &DiscoverabilityManager::updateLocation);

    connect(nodeList.data(), &NodeList::nodeAdded, this, &Application::nodeAdded);
    connect(nodeList.data(), &NodeList::nodeKilled, this, &Application::nodeKilled);
    connect(nodeList.data(), &NodeList::uuidChanged, getMyAvatar(), &MyAvatar::setSessionUUID);
    connect(nodeList.data(), &NodeList::uuidChanged, this, &Application::setSessionUUID);
    connect(nodeList.data(), &NodeList::limitOfSilentDomainCheckInsReached, nodeList.data(), &NodeList::reset);
    connect(nodeList.data(), &NodeList::packetVersionMismatch, this, &Application::notifyPacketVersionMismatch);

    // connect to appropriate slots on AccountManager
    AccountManager& accountManager = AccountManager::getInstance();

    const qint64 BALANCE_UPDATE_INTERVAL_MSECS = 5 * MSECS_PER_SEC;

    connect(&balanceUpdateTimer, &QTimer::timeout, &accountManager, &AccountManager::updateBalance);
    balanceUpdateTimer.start(BALANCE_UPDATE_INTERVAL_MSECS);

    connect(&accountManager, &AccountManager::balanceChanged, this, &Application::updateWindowTitle);

    auto dialogsManager = DependencyManager::get<DialogsManager>();
    connect(&accountManager, &AccountManager::authRequired, dialogsManager.data(), &DialogsManager::showLoginDialog);
    connect(&accountManager, &AccountManager::usernameChanged, this, &Application::updateWindowTitle);

    // set the account manager's root URL and trigger a login request if we don't have the access token
    accountManager.setIsAgent(true);
    accountManager.setAuthURL(NetworkingConstants::METAVERSE_SERVER_URL);

    // sessionRunTime will be reset soon by loadSettings. Grab it now to get previous session value.
    // The value will be 0 if the user blew away settings this session, which is both a feature and a bug.
    UserActivityLogger::getInstance().launch(applicationVersion(), _previousSessionCrashed, sessionRunTime.get());

    // once the event loop has started, check and signal for an access token
    QMetaObject::invokeMethod(&accountManager, "checkAndSignalForAccessToken", Qt::QueuedConnection);

    auto addressManager = DependencyManager::get<AddressManager>();

    // use our MyAvatar position and quat for address manager path
    addressManager->setPositionGetter([this]{ return getMyAvatar()->getPosition(); });
    addressManager->setOrientationGetter([this]{ return getMyAvatar()->getOrientation(); });

    connect(addressManager.data(), &AddressManager::hostChanged, this, &Application::updateWindowTitle);
    connect(this, &QCoreApplication::aboutToQuit, addressManager.data(), &AddressManager::storeCurrentAddress);

    // Save avatar location immediately after a teleport.
    connect(getMyAvatar(), &MyAvatar::positionGoneTo,
        DependencyManager::get<AddressManager>().data(), &AddressManager::storeCurrentAddress);

    auto scriptEngines = DependencyManager::get<ScriptEngines>().data();
    scriptEngines->registerScriptInitializer([this](ScriptEngine* engine){
        registerScriptEngineWithApplicationServices(engine);
    });

    connect(scriptEngines, &ScriptEngines::scriptCountChanged, scriptEngines, [this] {
        auto scriptEngines = DependencyManager::get<ScriptEngines>();
        if (scriptEngines->getRunningScripts().isEmpty()) {
            getMyAvatar()->clearScriptableSettings();
        }
    }, Qt::QueuedConnection);

    connect(scriptEngines, &ScriptEngines::scriptsReloading, scriptEngines, [this] {
        getEntities()->reloadEntityScripts();
    }, Qt::QueuedConnection);

    connect(scriptEngines, &ScriptEngines::scriptLoadError,
        scriptEngines, [](const QString& filename, const QString& error){
        OffscreenUi::warning(nullptr, "Error Loading Script", filename + " failed to load.");
    }, Qt::QueuedConnection);

#ifdef _WIN32
    WSADATA WsaData;
    int wsaresult = WSAStartup(MAKEWORD(2, 2), &WsaData);
#endif

    // tell the NodeList instance who to tell the domain server we care about
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
        << NodeType::EntityServer << NodeType::AssetServer << NodeType::MessagesMixer);

    // connect to the packet sent signal of the _entityEditSender
    connect(&_entityEditSender, &EntityEditPacketSender::packetSent, this, &Application::packetSent);

    // send the identity packet for our avatar each second to our avatar mixer
    connect(&identityPacketTimer, &QTimer::timeout, getMyAvatar(), &MyAvatar::sendIdentityPacket);
    identityPacketTimer.start(AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS);

    ResourceCache::setRequestLimit(3);

    _glWidget = new GLCanvas();
    getApplicationCompositor().setRenderingWidget(_glWidget);
    _window->setCentralWidget(_glWidget);

    _window->restoreGeometry();
    _window->setVisible(true);

    _glWidget->setFocusPolicy(Qt::StrongFocus);
    _glWidget->setFocus();

#ifdef Q_OS_MAC
    auto cursorTarget = _window; // OSX doesn't seem to provide for hiding the cursor only on the GL widget
#else
    // On windows and linux, hiding the top level cursor also means it's invisible when hovering over the 
    // window menu, which is a pain, so only hide it for the GL surface
    auto cursorTarget = _glWidget;
#endif
    cursorTarget->setCursor(Qt::BlankCursor);

    // enable mouse tracking; otherwise, we only get drag events
    _glWidget->setMouseTracking(true);
    _glWidget->makeCurrent();
    _glWidget->initializeGL();

    _chromiumShareContext = new OffscreenGLCanvas();
    _chromiumShareContext->create(_glWidget->context()->contextHandle());
    _chromiumShareContext->makeCurrent();
    qt_gl_set_global_share_context(_chromiumShareContext->getContext());

    _offscreenContext = new OffscreenGLCanvas();
    _offscreenContext->create(_glWidget->context()->contextHandle());
    _offscreenContext->makeCurrent();
    initializeGL();
    _offscreenContext->makeCurrent();
    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    // Tell our entity edit sender about our known jurisdictions
    _entityEditSender.setServerJurisdictions(&_entityServerJurisdictions);

    // For now we're going to set the PPS for outbound packets to be super high, this is
    // probably not the right long term solution. But for now, we're going to do this to
    // allow you to move an entity around in your hand
    _entityEditSender.setPacketsPerSecond(3000); // super high!!

    _overlays.init(); // do this before scripts load
    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

    // hook up bandwidth estimator
    QSharedPointer<BandwidthRecorder> bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();
    connect(nodeList.data(), &LimitedNodeList::dataSent,
        bandwidthRecorder.data(), &BandwidthRecorder::updateOutboundData);
    connect(&nodeList->getPacketReceiver(), &PacketReceiver::dataReceived,
        bandwidthRecorder.data(), &BandwidthRecorder::updateInboundData);

    // FIXME -- I'm a little concerned about this.
    connect(getMyAvatar()->getSkeletonModel().get(), &SkeletonModel::skeletonLoaded,
        this, &Application::checkSkeleton, Qt::QueuedConnection);

    // Setup the userInputMapper with the actions
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    connect(userInputMapper.data(), &UserInputMapper::actionEvent, [this](int action, float state) {
        using namespace controller;
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        if (offscreenUi->navigationFocused()) {
            auto actionEnum = static_cast<Action>(action);
            int key = Qt::Key_unknown;
            static int lastKey = Qt::Key_unknown;
            bool navAxis = false;
            switch (actionEnum) {
                case Action::UI_NAV_VERTICAL:
                    navAxis = true;
                    if (state > 0.0f) {
                        key = Qt::Key_Up;
                    } else if (state < 0.0f) {
                        key = Qt::Key_Down;
                    }
                    break;

                case Action::UI_NAV_LATERAL:
                    navAxis = true;
                    if (state > 0.0f) {
                        key = Qt::Key_Right;
                    } else if (state < 0.0f) {
                        key = Qt::Key_Left;
                    }
                    break;

                case Action::UI_NAV_GROUP:
                    navAxis = true;
                    if (state > 0.0f) {
                        key = Qt::Key_Tab;
                    } else if (state < 0.0f) {
                        key = Qt::Key_Backtab;
                    }
                    break;

                case Action::UI_NAV_BACK:
                    key = Qt::Key_Escape;
                    break;

                case Action::UI_NAV_SELECT:
                    key = Qt::Key_Return;
                    break;
                default:
                    break;
            }

            if (navAxis) {
                if (lastKey != Qt::Key_unknown) {
                    QKeyEvent event(QEvent::KeyRelease, lastKey, Qt::NoModifier);
                    sendEvent(offscreenUi->getWindow(), &event);
                    lastKey = Qt::Key_unknown;
                }

                if (key != Qt::Key_unknown) {
                    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
                    sendEvent(offscreenUi->getWindow(), &event);
                    lastKey = key;
                }
            } else if (key != Qt::Key_unknown) {
                if (state) {
                    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
                    sendEvent(offscreenUi->getWindow(), &event);
                } else {
                    QKeyEvent event(QEvent::KeyRelease, key, Qt::NoModifier);
                    sendEvent(offscreenUi->getWindow(), &event);
                }
                return;
            }
        }

        if (action == controller::toInt(controller::Action::RETICLE_CLICK)) {
            auto reticlePos = getApplicationCompositor().getReticlePosition();
            QPoint globalPos(reticlePos.x, reticlePos.y);

            // FIXME - it would be nice if this was self contained in the _compositor or Reticle class
            auto localPos = isHMDMode() ? globalPos : _glWidget->mapFromGlobal(globalPos);
            if (state) {
                QMouseEvent mousePress(QEvent::MouseButtonPress, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                sendEvent(_glWidget, &mousePress);
                _reticleClickPressed = true;
            } else {
                QMouseEvent mouseRelease(QEvent::MouseButtonRelease, localPos, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                sendEvent(_glWidget, &mouseRelease);
                _reticleClickPressed = false;
            }
            return; // nothing else to do
        }

        if (state) {
            if (action == controller::toInt(controller::Action::TOGGLE_MUTE)) {
                DependencyManager::get<AudioClient>()->toggleMute();
            } else if (action == controller::toInt(controller::Action::CYCLE_CAMERA)) {
                cycleCamera();
            } else if (action == controller::toInt(controller::Action::CONTEXT_MENU)) {
                auto reticlePosition = getApplicationCompositor().getReticlePosition();
                offscreenUi->toggleMenu(_glWidget->mapFromGlobal(QPoint(reticlePosition.x, reticlePosition.y)));
            } else if (action == controller::toInt(controller::Action::RETICLE_X)) {
                auto oldPos = getApplicationCompositor().getReticlePosition();
                getApplicationCompositor().setReticlePosition({ oldPos.x + state, oldPos.y });
            } else if (action == controller::toInt(controller::Action::RETICLE_Y)) {
                auto oldPos = getApplicationCompositor().getReticlePosition();
                getApplicationCompositor().setReticlePosition({ oldPos.x, oldPos.y + state });
            } else if (action == controller::toInt(controller::Action::TOGGLE_OVERLAY)) {
                toggleOverlays();
            }
        }
    });

    // A new controllerInput device used to reflect current values from the application state
    _applicationStateDevice = std::make_shared<controller::StateController>();

    _applicationStateDevice->addInputVariant(QString("InHMD"), controller::StateController::ReadLambda([]() -> float {
        return (float)qApp->isHMDMode();
    }));
    _applicationStateDevice->addInputVariant(QString("SnapTurn"), controller::StateController::ReadLambda([]() -> float {
        return (float)qApp->getMyAvatar()->getSnapTurn();
    }));
    _applicationStateDevice->addInputVariant(QString("Grounded"), controller::StateController::ReadLambda([]() -> float {
        return (float)qApp->getMyAvatar()->getCharacterController()->onGround();
    }));
    _applicationStateDevice->addInputVariant(QString("NavigationFocused"), controller::StateController::ReadLambda([]() -> float {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        return offscreenUi->navigationFocused() ? 1.0 : 0.0;
    }));

    userInputMapper->registerDevice(_applicationStateDevice);

    // Setup the keyboardMouseDevice and the user input mapper with the default bindings
    userInputMapper->registerDevice(_keyboardMouseDevice->getInputDevice());
    userInputMapper->loadDefaultMapping(userInputMapper->getStandardDeviceID());

    // force the model the look at the correct directory (weird order of operations issue)
    scriptEngines->setScriptsLocation(scriptEngines->getScriptsLocation());
    // do this as late as possible so that all required subsystems are initialized
    scriptEngines->loadScripts();
    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    loadSettings();
    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    int SAVE_SETTINGS_INTERVAL = 10 * MSECS_PER_SECOND; // Let's save every seconds for now
    connect(&_settingsTimer, &QTimer::timeout, this, &Application::saveSettings);
    connect(&_settingsThread, SIGNAL(started()), &_settingsTimer, SLOT(start()));
    connect(&_settingsThread, SIGNAL(finished()), &_settingsTimer, SLOT(stop()));
    _settingsTimer.moveToThread(&_settingsThread);
    _settingsTimer.setSingleShot(false);
    _settingsTimer.setInterval(SAVE_SETTINGS_INTERVAL);
    _settingsThread.start();

    if (Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson)) {
        getMyAvatar()->setBoomLength(MyAvatar::ZOOM_MIN);  // So that camera doesn't auto-switch to third person.
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::IndependentMode)) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
        cameraMenuChanged();
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::CameraEntityMode)) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
        cameraMenuChanged();
    }

    // set the local loopback interface for local sounds from audio scripts
    AudioScriptingInterface::getInstance().setLocalAudioInterface(audioIO.data());

    this->installEventFilter(this);

    // initialize our face trackers after loading the menu settings
    auto faceshiftTracker = DependencyManager::get<Faceshift>();
    faceshiftTracker->init();
    connect(faceshiftTracker.data(), &FaceTracker::muteToggled, this, &Application::faceTrackerMuteToggled);
#ifdef HAVE_DDE
    auto ddeTracker = DependencyManager::get<DdeFaceTracker>();
    ddeTracker->init();
    connect(ddeTracker.data(), &FaceTracker::muteToggled, this, &Application::faceTrackerMuteToggled);
#endif

#ifdef HAVE_IVIEWHMD
    auto eyeTracker = DependencyManager::get<EyeTracker>();
    eyeTracker->init();
    setActiveEyeTracker();
#endif

    auto applicationUpdater = DependencyManager::get<AutoUpdater>();
    connect(applicationUpdater.data(), &AutoUpdater::newVersionIsAvailable, dialogsManager.data(), &DialogsManager::showUpdateDialog);
    applicationUpdater->checkForUpdate();

    // Now that menu is initalized we can sync myAvatar with it's state.
    getMyAvatar()->updateMotionBehaviorFromMenu();

// FIXME spacemouse code still needs cleanup
#if 0
    // the 3Dconnexion device wants to be initiliazed after a window is displayed.
    SpacemouseManager::getInstance().init();
#endif

    // If the user clicks an an entity, we will check that it's an unlocked web entity, and if so, set the focus to it
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    connect(entityScriptingInterface.data(), &EntityScriptingInterface::clickDownOnEntity,
        [this, entityScriptingInterface](const EntityItemID& entityItemID, const MouseEvent& event) {
        if (_keyboardFocusedItem != entityItemID) {
            _keyboardFocusedItem = UNKNOWN_ENTITY_ID;
            auto properties = entityScriptingInterface->getEntityProperties(entityItemID);
            if (EntityTypes::Web == properties.getType() && !properties.getLocked()) {
                auto entity = entityScriptingInterface->getEntityTree()->findEntityByID(entityItemID);
                RenderableWebEntityItem* webEntity = dynamic_cast<RenderableWebEntityItem*>(entity.get());
                if (webEntity) {
                    webEntity->setProxyWindow(_window->windowHandle());
                    _keyboardFocusedItem = entityItemID;
                    _lastAcceptedKeyPress = usecTimestampNow();
                    if (_keyboardFocusHighlightID < 0 || !getOverlays().isAddedOverlay(_keyboardFocusHighlightID)) {
                        _keyboardFocusHighlight = new Cube3DOverlay();
                        _keyboardFocusHighlight->setAlpha(1.0f);
                        _keyboardFocusHighlight->setBorderSize(1.0f);
                        _keyboardFocusHighlight->setColor({ 0xFF, 0xEF, 0x00 });
                        _keyboardFocusHighlight->setIsSolid(false);
                        _keyboardFocusHighlight->setPulseMin(0.5);
                        _keyboardFocusHighlight->setPulseMax(1.0);
                        _keyboardFocusHighlight->setColorPulse(1.0);
                        _keyboardFocusHighlight->setIgnoreRayIntersection(true);
                        _keyboardFocusHighlight->setDrawInFront(true);
                    }
                    _keyboardFocusHighlight->setRotation(webEntity->getRotation());
                    _keyboardFocusHighlight->setPosition(webEntity->getPosition());
                    _keyboardFocusHighlight->setDimensions(webEntity->getDimensions() * 1.05f);
                    _keyboardFocusHighlight->setVisible(true);
                    _keyboardFocusHighlightID = getOverlays().addOverlay(_keyboardFocusHighlight);
                }
            }
            if (_keyboardFocusedItem == UNKNOWN_ENTITY_ID && _keyboardFocusHighlight) {
                _keyboardFocusHighlight->setVisible(false);
            }

        }
    });

    connect(entityScriptingInterface.data(), &EntityScriptingInterface::deletingEntity,
        [=](const EntityItemID& entityItemID) {
        if (entityItemID == _keyboardFocusedItem) {
            _keyboardFocusedItem = UNKNOWN_ENTITY_ID;
            if (_keyboardFocusHighlight) {
                _keyboardFocusHighlight->setVisible(false);
            }
        }
    });

    // If the user clicks somewhere where there is NO entity at all, we will release focus
    connect(getEntities(), &EntityTreeRenderer::mousePressOffEntity,
        [=](const RayToEntityIntersectionResult& entityItemID, const QMouseEvent* event) {
        _keyboardFocusedItem = UNKNOWN_ENTITY_ID;
        if (_keyboardFocusHighlight) {
            _keyboardFocusHighlight->setVisible(false);
        }
    });

    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    connect(this, &Application::applicationStateChanged, this, &Application::activeChanged);
    qCDebug(interfaceapp, "Startup time: %4.2f seconds.", (double)startupTimer.elapsed() / 1000.0);

    _idleTimer = new QTimer(this);
    connect(_idleTimer, &QTimer::timeout, [=] {
        idle(usecTimestampNow());
    });
    connect(this, &Application::beforeAboutToQuit, [=] {
        disconnect(_idleTimer);
    });
    // Setting the interval to zero forces this to get called whenever there are no messages
    // in the queue, which can be pretty damn frequent.  Hence the idle function has a bunch
    // of logic to abort early if it's being called too often.
    _idleTimer->start(0);
}


void Application::checkChangeCursor() {
    QMutexLocker locker(&_changeCursorLock);
    if (_cursorNeedsChanging) {
#ifdef Q_OS_MAC
        auto cursorTarget = _window; // OSX doesn't seem to provide for hiding the cursor only on the GL widget
#else
        // On windows and linux, hiding the top level cursor also means it's invisible when hovering over the 
        // window menu, which is a pain, so only hide it for the GL surface
        auto cursorTarget = _glWidget;
#endif
        cursorTarget->setCursor(_desiredCursor);

        _cursorNeedsChanging = false;
    }
}

void Application::showCursor(const QCursor& cursor) {
    QMutexLocker locker(&_changeCursorLock);
    _desiredCursor = cursor;
    _cursorNeedsChanging = true;
}

void Application::updateHeartbeat() {
    static_cast<DeadlockWatchdogThread*>(_deadlockWatchdogThread)->updateHeartbeat();
}

void Application::aboutToQuit() {
    emit beforeAboutToQuit();

    foreach(auto inputPlugin, PluginManager::getInstance()->getInputPlugins()) {
        QString name = inputPlugin->getName();
        QAction* action = Menu::getInstance()->getActionForOption(name);
        if (action->isChecked()) {
            inputPlugin->deactivate();
        }
    }

    getActiveDisplayPlugin()->deactivate();

    _aboutToQuit = true;

    cleanupBeforeQuit();
}

void Application::cleanupBeforeQuit() {
    // Stop third party processes so that they're not left running in the event of a subsequent shutdown crash.
#ifdef HAVE_DDE
    DependencyManager::get<DdeFaceTracker>()->setEnabled(false);
#endif
#ifdef HAVE_IVIEWHMD
    DependencyManager::get<EyeTracker>()->setEnabled(false, true);
#endif
    AnimDebugDraw::getInstance().shutdown();

    // FIXME: once we move to shared pointer for the INputDevice we shoud remove this naked delete:
    _applicationStateDevice.reset();

    if (_keyboardFocusHighlightID > 0) {
        getOverlays().deleteOverlay(_keyboardFocusHighlightID);
        _keyboardFocusHighlightID = -1;
    }
    _keyboardFocusHighlight = nullptr;

    getEntities()->clear(); // this will allow entity scripts to properly shutdown

    auto nodeList = DependencyManager::get<NodeList>();

    // send the domain a disconnect packet, force stoppage of domain-server check-ins
    nodeList->getDomainHandler().disconnect();
    nodeList->setIsShuttingDown(true);

    // tell the packet receiver we're shutting down, so it can drop packets
    nodeList->getPacketReceiver().setShouldDropPackets(true);

    getEntities()->shutdown(); // tell the entities system we're shutting down, so it will stop running scripts
    DependencyManager::get<ScriptEngines>()->saveScripts();
    DependencyManager::get<ScriptEngines>()->shutdownScripting(); // stop all currently running global scripts
    DependencyManager::destroy<ScriptEngines>();

    // first stop all timers directly or by invokeMethod
    // depending on what thread they run in
    locationUpdateTimer.stop();
    balanceUpdateTimer.stop();
    identityPacketTimer.stop();
    pingTimer.stop();
    QMetaObject::invokeMethod(&_settingsTimer, "stop", Qt::BlockingQueuedConnection);

    // save state
    _settingsThread.quit();
    saveSettings();
    _window->saveGeometry();

    // stop the AudioClient
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(),
                              "stop", Qt::BlockingQueuedConnection);

    // destroy the AudioClient so it and its thread have a chance to go down safely
    DependencyManager::destroy<AudioClient>();

    // destroy the AudioInjectorManager so it and its thread have a chance to go down safely
    // this will also stop any ongoing network injectors
    DependencyManager::destroy<AudioInjectorManager>();

    // Destroy third party processes after scripts have finished using them.
#ifdef HAVE_DDE
    DependencyManager::destroy<DdeFaceTracker>();
#endif
#ifdef HAVE_IVIEWHMD
    DependencyManager::destroy<EyeTracker>();
#endif

    DependencyManager::destroy<OffscreenUi>();
}

Application::~Application() {
    EntityTreePointer tree = getEntities()->getTree();
    tree->setSimulation(NULL);

    _octreeProcessor.terminate();
    _entityEditSender.terminate();

    _physicsEngine->setCharacterController(NULL);

    ModelEntityItem::cleanupLoadedAnimations();

    // remove avatars from physics engine
    DependencyManager::get<AvatarManager>()->clearOtherAvatars();
    VectorOfMotionStates motionStates;
    DependencyManager::get<AvatarManager>()->getObjectsToRemoveFromPhysics(motionStates);
    _physicsEngine->removeObjects(motionStates);

    DependencyManager::destroy<OffscreenUi>();
    DependencyManager::destroy<AvatarManager>();
    DependencyManager::destroy<AnimationCache>();
    DependencyManager::destroy<FramebufferCache>();
    DependencyManager::destroy<TextureCache>();
    DependencyManager::destroy<ModelCache>();
    DependencyManager::destroy<GeometryCache>();
    DependencyManager::destroy<ScriptCache>();
    DependencyManager::destroy<SoundCache>();

    ResourceManager::cleanup();

    QThread* nodeThread = DependencyManager::get<NodeList>()->thread();

    // remove the NodeList from the DependencyManager
    DependencyManager::destroy<NodeList>();

    // ask the node thread to quit and wait until it is done
    nodeThread->quit();
    nodeThread->wait();

    Leapmotion::destroy();

#if 0
    ConnexionClient::getInstance().destroy();
#endif
    // The window takes ownership of the menu, so this has the side effect of destroying it.
    _window->setMenuBar(nullptr);
    
    _window->deleteLater();

    qInstallMessageHandler(NULL); // NOTE: Do this as late as possible so we continue to get our log messages
}

void Application::initializeGL() {
    qCDebug(interfaceapp) << "Created Display Window.";

    // initialize glut for shape drawing; Qt apparently initializes it on OS X
    if (_isGLInitialized) {
        return;
    } else {
        _isGLInitialized = true;
    }

    // Where the gpuContext is initialized and where the TRUE Backend is created and assigned
    gpu::Context::init<gpu::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();

    initDisplay();
    qCDebug(interfaceapp, "Initialized Display.");

    // Set up the render engine
    render::CullFunctor cullFunctor = LODManager::shouldRender;
    _renderEngine->addJob<RenderShadowTask>("RenderShadowTask", cullFunctor);
    _renderEngine->addJob<RenderDeferredTask>("RenderDeferredTask", cullFunctor);
    _renderEngine->load();
    _renderEngine->registerScene(_main3DScene);
    // TODO: Load a cached config file

    // The UI can't be created until the primary OpenGL
    // context is created, because it needs to share
    // texture resources
    // Needs to happen AFTER the render engine initialization to access its configuration
    initializeUi();
    qCDebug(interfaceapp, "Initialized Offscreen UI.");
    _offscreenContext->makeCurrent();

    // call Menu getInstance static method to set up the menu
    // Needs to happen AFTER the QML UI initialization
    _window->setMenuBar(Menu::getInstance());

    init();
    qCDebug(interfaceapp, "init() complete.");

    // create thread for parsing of octree data independent of the main network and rendering threads
    _octreeProcessor.initialize(_enableProcessOctreeThread);
    connect(&_octreeProcessor, &OctreePacketProcessor::packetVersionMismatch, this, &Application::notifyPacketVersionMismatch);
    _entityEditSender.initialize(_enableProcessOctreeThread);

    _idleLoopStdev.reset();

    // update before the first render
    update(1.0f / _fps);

    InfoView::show(INFO_HELP_PATH, true);
}
extern void setupPreferences();
void Application::initializeUi() {
    AddressBarDialog::registerType();
    ErrorDialog::registerType();
    LoginDialog::registerType();
    Tooltip::registerType();
    UpdateDialog::registerType();
    qmlRegisterType<Preference>("Hifi", 1, 0, "Preference");


    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->create(_offscreenContext->getContext());
    offscreenUi->setProxyWindow(_window->windowHandle());
    offscreenUi->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
    // OffscreenUi is a subclass of OffscreenQmlSurface specifically designed to
    // support the window management and scripting proxies for VR use
    offscreenUi->createDesktop(QString("hifi/Desktop.qml"));

    // FIXME either expose so that dialogs can set this themselves or
    // do better detection in the offscreen UI of what has focus
    offscreenUi->setNavigationFocused(false);

    auto rootContext = offscreenUi->getRootContext();
    auto engine = rootContext->engine();
    connect(engine, &QQmlEngine::quit, [] {
        qApp->quit();
    });

    setupPreferences();

    // For some reason there is already an "Application" object in the QML context,
    // though I can't find it. Hence, "ApplicationInterface"
    rootContext->setContextProperty("SnapshotUploader", new SnapshotUploader());
    rootContext->setContextProperty("ApplicationInterface", this);
    rootContext->setContextProperty("AnimationCache", DependencyManager::get<AnimationCache>().data());
    rootContext->setContextProperty("Audio", &AudioScriptingInterface::getInstance());
    rootContext->setContextProperty("Controller", DependencyManager::get<controller::ScriptingInterface>().data());
    rootContext->setContextProperty("Entities", DependencyManager::get<EntityScriptingInterface>().data());
    rootContext->setContextProperty("MyAvatar", getMyAvatar());
    rootContext->setContextProperty("Messages", DependencyManager::get<MessagesClient>().data());
    rootContext->setContextProperty("Recording", DependencyManager::get<RecordingScriptingInterface>().data());
    rootContext->setContextProperty("Preferences", DependencyManager::get<Preferences>().data());

    rootContext->setContextProperty("TREE_SCALE", TREE_SCALE);
    rootContext->setContextProperty("Quat", new Quat());
    rootContext->setContextProperty("Vec3", new Vec3());
    rootContext->setContextProperty("Uuid", new ScriptUUID());
    rootContext->setContextProperty("Assets", new AssetMappingsScriptingInterface());

    rootContext->setContextProperty("AvatarList", DependencyManager::get<AvatarManager>().data());

    rootContext->setContextProperty("Camera", &_myCamera);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    rootContext->setContextProperty("SpeechRecognizer", DependencyManager::get<SpeechRecognizer>().data());
#endif

    rootContext->setContextProperty("Overlays", &_overlays);
    rootContext->setContextProperty("Window", DependencyManager::get<WindowScriptingInterface>().data());
    rootContext->setContextProperty("Menu", MenuScriptingInterface::getInstance());
    rootContext->setContextProperty("Stats", Stats::getInstance());
    rootContext->setContextProperty("Settings", SettingsScriptingInterface::getInstance());
    rootContext->setContextProperty("ScriptDiscoveryService", DependencyManager::get<ScriptEngines>().data());
    rootContext->setContextProperty("AudioDevice", AudioDeviceScriptingInterface::getInstance());
    rootContext->setContextProperty("AnimationCache", DependencyManager::get<AnimationCache>().data());
    rootContext->setContextProperty("SoundCache", DependencyManager::get<SoundCache>().data());
    rootContext->setContextProperty("Account", AccountScriptingInterface::getInstance());
    rootContext->setContextProperty("DialogsManager", _dialogsManagerScriptingInterface);
    rootContext->setContextProperty("GlobalServices", GlobalServicesScriptingInterface::getInstance());
    rootContext->setContextProperty("FaceTracker", DependencyManager::get<DdeFaceTracker>().data());
    rootContext->setContextProperty("AvatarManager", DependencyManager::get<AvatarManager>().data());
    rootContext->setContextProperty("UndoStack", &_undoStackScriptingInterface);
    rootContext->setContextProperty("LODManager", DependencyManager::get<LODManager>().data());
    rootContext->setContextProperty("Paths", DependencyManager::get<PathUtils>().data());
    rootContext->setContextProperty("HMD", DependencyManager::get<HMDScriptingInterface>().data());
    rootContext->setContextProperty("Scene", DependencyManager::get<SceneScriptingInterface>().data());
    rootContext->setContextProperty("Render", _renderEngine->getConfiguration().get());
    rootContext->setContextProperty("Reticle", getApplicationCompositor().getReticleInterface());

    rootContext->setContextProperty("ApplicationCompositor", &getApplicationCompositor());

    _glWidget->installEventFilter(offscreenUi.data());
    offscreenUi->setMouseTranslator([=](const QPointF& pt) {
        QPointF result = pt;
        auto displayPlugin = getActiveDisplayPlugin();
        if (displayPlugin->isHmd()) {
            getApplicationCompositor().handleRealMouseMoveEvent(false);
            auto resultVec = getApplicationCompositor().getReticlePosition();
            result = QPointF(resultVec.x, resultVec.y);
        }
        return result.toPoint();
    });
    offscreenUi->resume();
    connect(_window, &MainWindow::windowGeometryChanged, [this](const QRect& r){
        static qreal oldDevicePixelRatio = 0;
        qreal devicePixelRatio = getActiveDisplayPlugin()->devicePixelRatio();
        if (devicePixelRatio != oldDevicePixelRatio) {
            oldDevicePixelRatio = devicePixelRatio;
            qDebug() << "Device pixel ratio changed, triggering GL resize";
            resizeGL();
        }
    });

    // This will set up the input plugins UI
    _activeInputPlugins.clear();
    foreach(auto inputPlugin, PluginManager::getInstance()->getInputPlugins()) {
        QString name = inputPlugin->getName();
        if (name == KeyboardMouseDevice::NAME) {
            _keyboardMouseDevice = std::dynamic_pointer_cast<KeyboardMouseDevice>(inputPlugin);
        }
    }
    _window->setMenuBar(new Menu());
    updateInputModes();

    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    connect(compositorHelper.data(), &CompositorHelper::allowMouseCaptureChanged, [=] {
        if (isHMDMode()) {
            showCursor(compositorHelper->getAllowMouseCapture() ? Qt::BlankCursor : Qt::ArrowCursor);
        }
    });
}

void Application::paintGL() {

    updateHeartbeat();

    // Some plugins process message events, potentially leading to
    // re-entering a paint event.  don't allow further processing if this
    // happens
    if (_inPaint) {
        return;
    }
    _inPaint = true;
    Finally clearFlagLambda([this] { _inPaint = false; });

    // paintGL uses a queued connection, so we can get messages from the queue even after we've quit
    // and the plugins have shutdown
    if (_aboutToQuit) {
        return;
    }
    _frameCount++;

    // update fps moving average
    uint64_t now = usecTimestampNow();
    static uint64_t lastPaintBegin{ now };
    uint64_t diff = now - lastPaintBegin;
    float instantaneousFps = 0.0f;
    if (diff != 0) {
        instantaneousFps = (float)USECS_PER_SECOND / (float)diff;
        _framesPerSecond.updateAverage(_lastInstantaneousFps);
    }

    lastPaintBegin = now;

    // update fps once a second
    if (now - _lastFramesPerSecondUpdate > USECS_PER_SECOND) {
        _fps = _framesPerSecond.getAverage();
        _lastFramesPerSecondUpdate = now;
    }

    PROFILE_RANGE_EX(__FUNCTION__, 0xff0000ff, (uint64_t)_frameCount);
    PerformanceTimer perfTimer("paintGL");

    if (nullptr == _displayPlugin) {
        return;
    }

    auto displayPlugin = getActiveDisplayPlugin();
    // FIXME not needed anymore?
    _offscreenContext->makeCurrent();

    displayPlugin->updateHeadPose(_frameCount);

    // update the avatar with a fresh HMD pose
    getMyAvatar()->updateFromHMDSensorMatrix(getHMDSensorPose());

    auto lodManager = DependencyManager::get<LODManager>();


    RenderArgs renderArgs(_gpuContext, getEntities(), getViewFrustum(), lodManager->getOctreeSizeScale(),
                          lodManager->getBoundaryLevelAdjust(), RenderArgs::DEFAULT_RENDER_MODE,
                          RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);

    PerformanceWarning::setSuppressShortTimings(Menu::getInstance()->isOptionChecked(MenuOption::SuppressShortTimings));
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::paintGL()");
    resizeGL();

    // Before anything else, let's sync up the gpuContext with the true glcontext used in case anything happened
    {
        PerformanceTimer perfTimer("syncCache");
        renderArgs._context->syncCache();
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::MiniMirror)) {
        PerformanceTimer perfTimer("Mirror");
        auto primaryFbo = DependencyManager::get<FramebufferCache>()->getPrimaryFramebuffer();

        renderArgs._renderMode = RenderArgs::MIRROR_RENDER_MODE;
        renderArgs._blitFramebuffer = DependencyManager::get<FramebufferCache>()->getSelfieFramebuffer();

        auto inputs = AvatarInputs::getInstance();
        _mirrorViewRect.moveTo(inputs->x(), inputs->y());

        renderRearViewMirror(&renderArgs, _mirrorViewRect);

        renderArgs._blitFramebuffer.reset();
        renderArgs._renderMode = RenderArgs::DEFAULT_RENDER_MODE;
    }

    {
        PerformanceTimer perfTimer("renderOverlay");
        // NOTE: There is no batch associated with this renderArgs
        // the ApplicationOverlay class assumes it's viewport is setup to be the device size
        QSize size = getDeviceSize();
        renderArgs._viewport = glm::ivec4(0, 0, size.width(), size.height());
        _applicationOverlay.renderOverlay(&renderArgs);
        auto overlayTexture = _applicationOverlay.acquireOverlay();
        if (overlayTexture) {
            displayPlugin->submitOverlayTexture(overlayTexture);
        }
    }

    glm::vec3 boomOffset;
    {
        PerformanceTimer perfTimer("CameraUpdates");

        auto myAvatar = getMyAvatar();
        boomOffset = myAvatar->getScale() * myAvatar->getBoomLength() * -IDENTITY_FRONT;

        if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON || _myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
            Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, myAvatar->getBoomLength() <= MyAvatar::ZOOM_MIN);
            Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, !(myAvatar->getBoomLength() <= MyAvatar::ZOOM_MIN));
            cameraMenuChanged();
        }

        // The render mode is default or mirror if the camera is in mirror mode, assigned further below
        renderArgs._renderMode = RenderArgs::DEFAULT_RENDER_MODE;

        // Always use the default eye position, not the actual head eye position.
        // Using the latter will cause the camera to wobble with idle animations,
        // or with changes from the face tracker
        if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON) {
            if (isHMDMode()) {
                mat4 camMat = myAvatar->getSensorToWorldMatrix() * myAvatar->getHMDSensorMatrix();
                _myCamera.setPosition(extractTranslation(camMat));
                _myCamera.setRotation(glm::quat_cast(camMat));
            } else {
                _myCamera.setPosition(myAvatar->getDefaultEyePosition());
                _myCamera.setRotation(myAvatar->getHead()->getCameraOrientation());
            }
        } else if (_myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
            if (isHMDMode()) {
                auto hmdWorldMat = myAvatar->getSensorToWorldMatrix() * myAvatar->getHMDSensorMatrix();
                _myCamera.setRotation(glm::normalize(glm::quat_cast(hmdWorldMat)));
                _myCamera.setPosition(extractTranslation(hmdWorldMat) +
                    myAvatar->getOrientation() * boomOffset);
            } else {
                _myCamera.setRotation(myAvatar->getHead()->getOrientation());
                if (Menu::getInstance()->isOptionChecked(MenuOption::CenterPlayerInView)) {
                    _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                        + _myCamera.getRotation() * boomOffset);
                } else {
                    _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                        + myAvatar->getOrientation() * boomOffset);
                }
            }
        } else if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
            if (isHMDMode()) {
                auto mirrorBodyOrientation = myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f));

                glm::quat hmdRotation = extractRotation(myAvatar->getHMDSensorMatrix());
                // Mirror HMD yaw and roll
                glm::vec3 mirrorHmdEulers = glm::eulerAngles(hmdRotation);
                mirrorHmdEulers.y = -mirrorHmdEulers.y;
                mirrorHmdEulers.z = -mirrorHmdEulers.z;
                glm::quat mirrorHmdRotation = glm::quat(mirrorHmdEulers);

                glm::quat worldMirrorRotation = mirrorBodyOrientation * mirrorHmdRotation;

                _myCamera.setRotation(worldMirrorRotation);

                glm::vec3 hmdOffset = extractTranslation(myAvatar->getHMDSensorMatrix());
                // Mirror HMD lateral offsets
                hmdOffset.x = -hmdOffset.x;

                _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                    + glm::vec3(0, _raiseMirror * myAvatar->getUniformScale(), 0)
                   + mirrorBodyOrientation * glm::vec3(0.0f, 0.0f, 1.0f) * MIRROR_FULLSCREEN_DISTANCE * _scaleMirror
                   + mirrorBodyOrientation * hmdOffset);
            } else {
                _myCamera.setRotation(myAvatar->getWorldAlignedOrientation()
                    * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f)));
                _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                    + glm::vec3(0, _raiseMirror * myAvatar->getUniformScale(), 0)
                    + (myAvatar->getOrientation() * glm::quat(glm::vec3(0.0f, _rotateMirror, 0.0f))) *
                    glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_FULLSCREEN_DISTANCE * _scaleMirror);
            }
            renderArgs._renderMode = RenderArgs::MIRROR_RENDER_MODE;
        } else if (_myCamera.getMode() == CAMERA_MODE_ENTITY) {
            EntityItemPointer cameraEntity = _myCamera.getCameraEntityPointer();
            if (cameraEntity != nullptr) {
                if (isHMDMode()) {
                    glm::quat hmdRotation = extractRotation(myAvatar->getHMDSensorMatrix());
                    _myCamera.setRotation(cameraEntity->getRotation() * hmdRotation);
                    glm::vec3 hmdOffset = extractTranslation(myAvatar->getHMDSensorMatrix());
                    _myCamera.setPosition(cameraEntity->getPosition() + (hmdRotation * hmdOffset));
                } else {
                    _myCamera.setRotation(cameraEntity->getRotation());
                    _myCamera.setPosition(cameraEntity->getPosition());
                }
            }
        }
        // Update camera position
        if (!isHMDMode()) {
            _myCamera.update(1.0f / _fps);
        }
    }

    getApplicationCompositor().setFrameInfo(_frameCount, _myCamera.getTransform());

    // Primary rendering pass
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    const QSize size = framebufferCache->getFrameBufferSize();

    // Final framebuffer that will be handled to the display-plugin
    auto finalFramebuffer = framebufferCache->getFramebuffer();

    {
        PROFILE_RANGE(__FUNCTION__ "/mainRender");
        PerformanceTimer perfTimer("mainRender");
        renderArgs._boomOffset = boomOffset;
        // Viewport is assigned to the size of the framebuffer
        renderArgs._viewport = ivec4(0, 0, size.width(), size.height());
        if (displayPlugin->isStereo()) {
            // Stereo modes will typically have a larger projection matrix overall,
            // so we ask for the 'mono' projection matrix, which for stereo and HMD
            // plugins will imply the combined projection for both eyes.
            //
            // This is properly implemented for the Oculus plugins, but for OpenVR
            // and Stereo displays I'm not sure how to get / calculate it, so we're
            // just relying on the left FOV in each case and hoping that the
            // overall culling margin of error doesn't cause popping in the
            // right eye.  There are FIXMEs in the relevant plugins
            _myCamera.setProjection(displayPlugin->getCullingProjection(_myCamera.getProjection()));
            renderArgs._context->enableStereo(true);
            mat4 eyeOffsets[2];
            mat4 eyeProjections[2];
            auto baseProjection = renderArgs._viewFrustum->getProjection();
            auto hmdInterface = DependencyManager::get<HMDScriptingInterface>();
            float IPDScale = hmdInterface->getIPDScale();
            mat4 headPose = displayPlugin->getHeadPose();

            // FIXME we probably don't need to set the projection matrix every frame,
            // only when the display plugin changes (or in non-HMD modes when the user
            // changes the FOV manually, which right now I don't think they can.
            for_each_eye([&](Eye eye) {
                // For providing the stereo eye views, the HMD head pose has already been
                // applied to the avatar, so we need to get the difference between the head
                // pose applied to the avatar and the per eye pose, and use THAT as
                // the per-eye stereo matrix adjustment.
                mat4 eyeToHead = displayPlugin->getEyeToHeadTransform(eye);
                // Grab the translation
                vec3 eyeOffset = glm::vec3(eyeToHead[3]);
                // Apply IPD scaling
                mat4 eyeOffsetTransform = glm::translate(mat4(), eyeOffset * -1.0f * IPDScale);
                eyeOffsets[eye] = eyeOffsetTransform;

                // Tell the plugin what pose we're using to render.  In this case we're just using the
                // unmodified head pose because the only plugin that cares (the Oculus plugin) uses it
                // for rotational timewarp.  If we move to support positonal timewarp, we need to
                // ensure this contains the full pose composed with the eye offsets.
                displayPlugin->setEyeRenderPose(_frameCount, eye, headPose * glm::inverse(eyeOffsetTransform));

                eyeProjections[eye] = displayPlugin->getEyeProjection(eye, baseProjection);
            });
            renderArgs._context->setStereoProjections(eyeProjections);
            renderArgs._context->setStereoViews(eyeOffsets);
        }
        renderArgs._blitFramebuffer = finalFramebuffer;
        displaySide(&renderArgs, _myCamera);

        renderArgs._blitFramebuffer.reset();
        renderArgs._context->enableStereo(false);
    }

    // deliver final composited scene to the display plugin
    {
        PROFILE_RANGE(__FUNCTION__ "/pluginOutput");
        PerformanceTimer perfTimer("pluginOutput");

        auto finalTexture = finalFramebuffer->getRenderBuffer(0);
        Q_ASSERT(!_lockedFramebufferMap.contains(finalTexture));
        _lockedFramebufferMap[finalTexture] = finalFramebuffer;

        Q_ASSERT(isCurrentContext(_offscreenContext->getContext()));
        {
            PROFILE_RANGE(__FUNCTION__ "/pluginSubmitScene");
            PerformanceTimer perfTimer("pluginSubmitScene");
            displayPlugin->submitSceneTexture(_frameCount, finalTexture);
        }
        Q_ASSERT(isCurrentContext(_offscreenContext->getContext()));
    }

    {
        Stats::getInstance()->setRenderDetails(renderArgs._details);
        // Reset the gpu::Context Stages
        // Back to the default framebuffer;
        gpu::doInBatch(renderArgs._context, [&](gpu::Batch& batch) {
            batch.resetStages();
        });
    }

    _lastInstantaneousFps = instantaneousFps;
}

void Application::runTests() {
    runTimingTests();
    runUnitTests();
}

void Application::audioMuteToggled() {
    QAction* muteAction = Menu::getInstance()->getActionForOption(MenuOption::MuteAudio);
    Q_CHECK_PTR(muteAction);
    muteAction->setChecked(DependencyManager::get<AudioClient>()->isMuted());
}

void Application::faceTrackerMuteToggled() {

    QAction* muteAction = Menu::getInstance()->getActionForOption(MenuOption::MuteFaceTracking);
    Q_CHECK_PTR(muteAction);
    bool isMuted = getSelectedFaceTracker()->isMuted();
    muteAction->setChecked(isMuted);
    getSelectedFaceTracker()->setEnabled(!isMuted);
    Menu::getInstance()->getActionForOption(MenuOption::CalibrateCamera)->setEnabled(!isMuted);
}

void Application::setFieldOfView(float fov) {
    if (fov != _fieldOfView.get()) {
        _fieldOfView.set(fov);
        resizeGL();
    }
}

void Application::aboutApp() {
    InfoView::show(INFO_HELP_PATH);
}

void Application::showHelp() {
    InfoView::show(INFO_EDIT_ENTITIES_PATH);
}

void Application::resizeEvent(QResizeEvent* event) {
    resizeGL();
}

void Application::resizeGL() {
    PROFILE_RANGE(__FUNCTION__);
    if (nullptr == _displayPlugin) {
        return;
    }

    auto displayPlugin = getActiveDisplayPlugin();
    // Set the desired FBO texture size. If it hasn't changed, this does nothing.
    // Otherwise, it must rebuild the FBOs
    uvec2 framebufferSize = displayPlugin->getRecommendedRenderSize();
    uvec2 renderSize = uvec2(vec2(framebufferSize) * getRenderResolutionScale());
    if (_renderResolution != renderSize) {
        _renderResolution = renderSize;
        DependencyManager::get<FramebufferCache>()->setFrameBufferSize(fromGlm(renderSize));
    }

    // FIXME the aspect ratio for stereo displays is incorrect based on this.
    float aspectRatio = displayPlugin->getRecommendedAspectRatio();
    _myCamera.setProjection(glm::perspective(glm::radians(_fieldOfView.get()), aspectRatio,
                                             DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
    // Possible change in aspect ratio
    loadViewFrustum(_myCamera, _viewFrustum);

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto uiSize = displayPlugin->getRecommendedUiSize();
    // Bit of a hack since there's no device pixel ratio change event I can find.
    static qreal lastDevicePixelRatio = 0;
    qreal devicePixelRatio = _window->devicePixelRatio();
    if (offscreenUi->size() != fromGlm(uiSize) || devicePixelRatio != lastDevicePixelRatio) {
        offscreenUi->resize(fromGlm(uiSize));
        _offscreenContext->makeCurrent();
        lastDevicePixelRatio = devicePixelRatio;
    }
}

bool Application::importSVOFromURL(const QString& urlString) {
    emit svoImportRequested(urlString);
    return true;
}

bool Application::event(QEvent* event) {

    if (!Menu::getInstance()) {
        return false;
    }

    if ((int)event->type() == (int)Lambda) {
        ((LambdaEvent*)event)->call();
        return true;
    }

    if ((int)event->type() == (int)Paint) {
        paintGL();
        return true;
    }

    if (!_keyboardFocusedItem.isInvalidID()) {
        switch (event->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease: {
                auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
                auto entity = entityScriptingInterface->getEntityTree()->findEntityByID(_keyboardFocusedItem);
                RenderableWebEntityItem* webEntity = dynamic_cast<RenderableWebEntityItem*>(entity.get());
                if (webEntity && webEntity->getEventHandler()) {
                    event->setAccepted(false);
                    QCoreApplication::sendEvent(webEntity->getEventHandler(), event);
                    if (event->isAccepted()) {
                        _lastAcceptedKeyPress = usecTimestampNow();
                        return true;
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    switch (event->type()) {
        case QEvent::MouseMove:
            mouseMoveEvent((QMouseEvent*)event);
            return true;
        case QEvent::MouseButtonPress:
            mousePressEvent((QMouseEvent*)event);
            return true;
        case QEvent::MouseButtonDblClick:
            mouseDoublePressEvent((QMouseEvent*)event);
            return true;
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent((QMouseEvent*)event);
            return true;
        case QEvent::KeyPress:
            keyPressEvent((QKeyEvent*)event);
            return true;
        case QEvent::KeyRelease:
            keyReleaseEvent((QKeyEvent*)event);
            return true;
        case QEvent::FocusOut:
            focusOutEvent((QFocusEvent*)event);
            return true;
        case QEvent::TouchBegin:
            touchBeginEvent(static_cast<QTouchEvent*>(event));
            event->accept();
            return true;
        case QEvent::TouchEnd:
            touchEndEvent(static_cast<QTouchEvent*>(event));
            return true;
        case QEvent::TouchUpdate:
            touchUpdateEvent(static_cast<QTouchEvent*>(event));
            return true;
        case QEvent::Wheel:
            wheelEvent(static_cast<QWheelEvent*>(event));
            return true;
        case QEvent::Drop:
            dropEvent(static_cast<QDropEvent*>(event));
            return true;
        default:
            break;
    }

    // handle custom URL
    if (event->type() == QEvent::FileOpen) {

        QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event);

        QUrl url = fileEvent->url();

        if (!url.isEmpty()) {
            QString urlString = url.toString();
            if (canAcceptURL(urlString)) {
                return acceptURL(urlString);
            }
        }
        return false;
    }

    if (HFActionEvent::types().contains(event->type())) {
        _controllerScriptingInterface->handleMetaEvent(static_cast<HFMetaEvent*>(event));
    }

    return QApplication::event(event);
}

bool Application::eventFilter(QObject* object, QEvent* event) {

    if (event->type() == QEvent::Leave) {
        getApplicationCompositor().handleLeaveEvent();
    }

    if (event->type() == QEvent::ShortcutOverride) {
        if (DependencyManager::get<OffscreenUi>()->shouldSwallowShortcut(event)) {
            event->accept();
            return true;
        }

        // Filter out captured keys before they're used for shortcut actions.
        if (_controllerScriptingInterface->isKeyCaptured(static_cast<QKeyEvent*>(event))) {
            event->accept();
            return true;
        }
    }

    return false;
}

static bool _altPressed{ false };

void Application::keyPressEvent(QKeyEvent* event) {
    _altPressed = event->key() == Qt::Key_Alt;
    _keysPressed.insert(event->key());

    _controllerScriptingInterface->emitKeyPressEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isKeyCaptured(event)) {
        return;
    }

    if (hasFocus()) {
        if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
            _keyboardMouseDevice->keyPressEvent(event);
        }

        bool isShifted = event->modifiers().testFlag(Qt::ShiftModifier);
        bool isMeta = event->modifiers().testFlag(Qt::ControlModifier);
        bool isOption = event->modifiers().testFlag(Qt::AltModifier);
        switch (event->key()) {
            case Qt::Key_Enter:
            case Qt::Key_Return:
                if (isOption) {
                    if (_window->isFullScreen()) {
                        _pluginContainer->unsetFullscreen();
                    } else {
                        _pluginContainer->setFullscreen(nullptr);
                    }
                } else {
                    Menu::getInstance()->triggerOption(MenuOption::AddressBar);
                }
                break;

            case Qt::Key_1:
            case Qt::Key_2:
            case Qt::Key_3:
            case Qt::Key_4:
            case Qt::Key_5:
            case Qt::Key_6:
            case Qt::Key_7:
                if (isMeta || isOption) {
                    unsigned int index = static_cast<unsigned int>(event->key() - Qt::Key_1);
                    auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();
                    if (index < displayPlugins.size()) {
                        auto targetPlugin = displayPlugins.at(index);
                        QString targetName = targetPlugin->getName();
                        auto menu = Menu::getInstance();
                        QAction* action = menu->getActionForOption(targetName);
                        if (action && !action->isChecked()) {
                            action->trigger();
                        }
                    }
                }
                break;

            case Qt::Key_X:
                if (isShifted && isMeta) {
                    auto offscreenUi = DependencyManager::get<OffscreenUi>();
                    offscreenUi->getRootContext()->engine()->clearComponentCache();
                    //OffscreenUi::information("Debugging", "Component cache cleared");
                    // placeholder for dialogs being converted to QML.
                }
                break;

            case Qt::Key_B:
                if (isMeta) {
                    auto offscreenUi = DependencyManager::get<OffscreenUi>();
                    offscreenUi->load("Browser.qml");
                }
                break;

            case Qt::Key_L:
                if (isShifted && isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::Log);
                } else if (isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::AddressBar);
                } else if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::LodTools);
                }
                break;

            case Qt::Key_F: {
                _physicsEngine->dumpNextStats();
                break;
            }

            case Qt::Key_Asterisk:
                Menu::getInstance()->triggerOption(MenuOption::Stars);
                break;

            case Qt::Key_S:
                if (isShifted && isMeta && !isOption) {
                    Menu::getInstance()->triggerOption(MenuOption::SuppressShortTimings);
                } else if (isOption && !isShifted && !isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::ScriptEditor);
                } else if (!isOption && !isShifted && isMeta) {
                    takeSnapshot();
                }
                break;

            case Qt::Key_Apostrophe: {
                if (isMeta) {
                    auto cursor = Cursor::Manager::instance().getCursor();
                    auto curIcon = cursor->getIcon();
                    if (curIcon == Cursor::Icon::DEFAULT) {
                        cursor->setIcon(Cursor::Icon::LINK);
                    } else {
                        cursor->setIcon(Cursor::Icon::DEFAULT);
                    }
                } else {
                    resetSensors(true);
                }
                break;
            }

            case Qt::Key_Backslash:
                Menu::getInstance()->triggerOption(MenuOption::Chat);
                break;

            case Qt::Key_Up:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    if (!isShifted) {
                        _scaleMirror *= 0.95f;
                    } else {
                        _raiseMirror += 0.05f;
                    }
                }
                break;

            case Qt::Key_Down:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    if (!isShifted) {
                        _scaleMirror *= 1.05f;
                    } else {
                        _raiseMirror -= 0.05f;
                    }
                }
                break;

            case Qt::Key_Left:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    _rotateMirror += PI / 20.0f;
                }
                break;

            case Qt::Key_Right:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    _rotateMirror -= PI / 20.0f;
                }
                break;

#if 0
            case Qt::Key_I:
                if (isShifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0.002f, 0, 0)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, 0.001, 0));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_K:
                if (isShifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(-0.002f, 0, 0)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, -0.001, 0));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_J:
                if (isShifted) {
                    _viewFrustum.setFocalLength(_viewFrustum.getFocalLength() - 0.1f);
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(-0.001, 0, 0));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_M:
                if (isShifted) {
                    _viewFrustum.setFocalLength(_viewFrustum.getFocalLength() + 0.1f);
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0.001, 0, 0));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_U:
                if (isShifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0, 0, -0.002f)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, 0, -0.001));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_Y:
                if (isShifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0, 0, 0.002f)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, 0, 0.001));
                }
                updateProjectionMatrix();
                break;
#endif

            case Qt::Key_H:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::MiniMirror);
                } else {
                    bool isMirrorChecked = Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror);
                    Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, !isMirrorChecked);
                    if (isMirrorChecked) {
                        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
                    }
                    cameraMenuChanged();
                }
                break;
            case Qt::Key_P: {
                bool isFirstPersonChecked = Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson);
                Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, !isFirstPersonChecked);
                Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, isFirstPersonChecked);
                cameraMenuChanged();
                break;
            }

            case Qt::Key_Slash:
                Menu::getInstance()->triggerOption(MenuOption::Stats);
                break;

            case Qt::Key_Plus: {
                if (isMeta && event->modifiers().testFlag(Qt::KeypadModifier)) {
                    auto& cursorManager = Cursor::Manager::instance();
                    cursorManager.setScale(cursorManager.getScale() * 1.1f);
                } else {
                    getMyAvatar()->increaseSize();
                }
                break;
            }

            case Qt::Key_Minus: {
                if (isMeta && event->modifiers().testFlag(Qt::KeypadModifier)) {
                    auto& cursorManager = Cursor::Manager::instance();
                    cursorManager.setScale(cursorManager.getScale() / 1.1f);
                } else {
                    getMyAvatar()->decreaseSize();
                }
                break;
            }

            case Qt::Key_Equal:
                getMyAvatar()->resetSize();
                break;
            case Qt::Key_Space: {
                if (!event->isAutoRepeat()) {
                    // FIXME -- I don't think we've tested the HFActionEvent in a while... this looks possibly dubious
                    // this starts an HFActionEvent
                    HFActionEvent startActionEvent(HFActionEvent::startType(),
                                                   computePickRay(getMouse().x, getMouse().y));
                    sendEvent(this, &startActionEvent);
                }

                break;
            }
            case Qt::Key_Escape: {
                getActiveDisplayPlugin()->abandonCalibration();
                if (!event->isAutoRepeat()) {
                    // this starts the HFCancelEvent
                    HFBackEvent startBackEvent(HFBackEvent::startType());
                    sendEvent(this, &startBackEvent);
                }

                break;
            }

            default:
                event->ignore();
                break;
        }
    }
}



void Application::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Alt && _altPressed && hasFocus()) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        auto reticlePosition = getApplicationCompositor().getReticlePosition();
        offscreenUi->toggleMenu(_glWidget->mapFromGlobal(QPoint(reticlePosition.x, reticlePosition.y)));
    }

    _keysPressed.remove(event->key());

    _controllerScriptingInterface->emitKeyReleaseEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isKeyCaptured(event)) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->keyReleaseEvent(event);
    }

    switch (event->key()) {
        case Qt::Key_Space: {
            if (!event->isAutoRepeat()) {
                // FIXME -- I don't think we've tested the HFActionEvent in a while... this looks possibly dubious
                // this ends the HFActionEvent
                HFActionEvent endActionEvent(HFActionEvent::endType(),
                                             computePickRay(getMouse().x, getMouse().y));
                sendEvent(this, &endActionEvent);
            }
            break;
        }
        case Qt::Key_Escape: {
            if (!event->isAutoRepeat()) {
                // this ends the HFCancelEvent
                HFBackEvent endBackEvent(HFBackEvent::endType());
                sendEvent(this, &endBackEvent);
            }
            break;
        }
        default:
            event->ignore();
            break;
    }
}

void Application::focusOutEvent(QFocusEvent* event) {
    auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
    foreach(auto inputPlugin, inputPlugins) {
        QString name = inputPlugin->getName();
        QAction* action = Menu::getInstance()->getActionForOption(name);
        if (action && action->isChecked()) {
            inputPlugin->pluginFocusOutEvent();
        }
    }

// FIXME spacemouse code still needs cleanup
#if 0
    //SpacemouseDevice::getInstance().focusOutEvent();
    //SpacemouseManager::getInstance().getDevice()->focusOutEvent();
    SpacemouseManager::getInstance().ManagerFocusOutEvent();
#endif

    // synthesize events for keys currently pressed, since we may not get their release events
    foreach (int key, _keysPressed) {
        QKeyEvent event(QEvent::KeyRelease, key, Qt::NoModifier);
        keyReleaseEvent(&event);
    }
    _keysPressed.clear();
}

void Application::maybeToggleMenuVisible(QMouseEvent* event) {
#ifndef Q_OS_MAC
    // If in full screen, and our main windows menu bar is hidden, and we're close to the top of the QMainWindow
    // then show the menubar.
    if (_window->isFullScreen()) {
        QMenuBar* menuBar = _window->menuBar();
        if (menuBar) {
            static const int MENU_TOGGLE_AREA = 10;
            if (!menuBar->isVisible()) {
                if (event->pos().y() <= MENU_TOGGLE_AREA) {
                    menuBar->setVisible(true);
                }
            }  else {
                if (event->pos().y() > MENU_TOGGLE_AREA) {
                    menuBar->setVisible(false);
                }
            }
        }
    }
#endif
}

void Application::mouseMoveEvent(QMouseEvent* event) {
    PROFILE_RANGE(__FUNCTION__);

    if (_aboutToQuit) {
        return;
    }

    maybeToggleMenuVisible(event);

    auto& compositor = getApplicationCompositor();
    // if this is a real mouse event, and we're in HMD mode, then we should use it to move the 
    // compositor reticle
    // handleRealMouseMoveEvent() will return true, if we shouldn't process the event further
    if (!compositor.fakeEventActive() && compositor.handleRealMouseMoveEvent()) {
        return; // bail
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto eventPosition = compositor.getMouseEventPosition(event);
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(eventPosition, _glWidget);
    auto button = event->button();
    auto buttons = event->buttons();
    // Determine if the ReticleClick Action is 1 and if so, fake include the LeftMouseButton
    if (_reticleClickPressed) {
        if (button == Qt::NoButton) {
            button = Qt::LeftButton;
        }
        buttons |= Qt::LeftButton;
    }

    QMouseEvent mappedEvent(event->type(),
        transformedPos,
        event->screenPos(), button,
        buttons, event->modifiers());

    getEntities()->mouseMoveEvent(&mappedEvent);
    _controllerScriptingInterface->emitMouseMoveEvent(&mappedEvent); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isMouseCaptured()) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->mouseMoveEvent(event);
    }

}

void Application::mousePressEvent(QMouseEvent* event) {
    // Inhibit the menu if the user is using alt-mouse dragging
    _altPressed = false;

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    // If we get a mouse press event it means it wasn't consumed by the offscreen UI,
    // hence, we should defocus all of the offscreen UI windows, in order to allow
    // keyboard shortcuts not to be swallowed by them.  In particular, WebEngineViews
    // will consume all keyboard events.
    offscreenUi->unfocusWindows();

    auto eventPosition = getApplicationCompositor().getMouseEventPosition(event);
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(eventPosition, _glWidget);
    QMouseEvent mappedEvent(event->type(),
        transformedPos,
        event->screenPos(), event->button(),
        event->buttons(), event->modifiers());

    if (!_aboutToQuit) {
        getEntities()->mousePressEvent(&mappedEvent);
    }

    _controllerScriptingInterface->emitMousePressEvent(&mappedEvent); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isMouseCaptured()) {
        return;
    }


    if (hasFocus()) {
        if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
            _keyboardMouseDevice->mousePressEvent(event);
        }

        if (event->button() == Qt::LeftButton) {
            // nobody handled this - make it an action event on the _window object
            HFActionEvent actionEvent(HFActionEvent::startType(),
                computePickRay(mappedEvent.x(), mappedEvent.y()));
            sendEvent(this, &actionEvent);

        }
    }
}

void Application::mouseDoublePressEvent(QMouseEvent* event) {
    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isMouseCaptured()) {
        return;
    }

    _controllerScriptingInterface->emitMouseDoublePressEvent(event);
}

void Application::mouseReleaseEvent(QMouseEvent* event) {

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto eventPosition = getApplicationCompositor().getMouseEventPosition(event);
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(eventPosition, _glWidget);
    QMouseEvent mappedEvent(event->type(),
        transformedPos,
        event->screenPos(), event->button(),
        event->buttons(), event->modifiers());

    if (!_aboutToQuit) {
        getEntities()->mouseReleaseEvent(&mappedEvent);
    }

    _controllerScriptingInterface->emitMouseReleaseEvent(&mappedEvent); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isMouseCaptured()) {
        return;
    }

    if (hasFocus()) {
        if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
            _keyboardMouseDevice->mouseReleaseEvent(event);
        }

        if (event->button() == Qt::LeftButton) {
            // fire an action end event
            HFActionEvent actionEvent(HFActionEvent::endType(),
                computePickRay(mappedEvent.x(), mappedEvent.y()));
            sendEvent(this, &actionEvent);
        }
    }
}

void Application::touchUpdateEvent(QTouchEvent* event) {
    _altPressed = false;

    if (event->type() == QEvent::TouchUpdate) {
        TouchEvent thisEvent(*event, _lastTouchEvent);
        _controllerScriptingInterface->emitTouchUpdateEvent(thisEvent); // send events to any registered scripts
        _lastTouchEvent = thisEvent;
    }

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isTouchCaptured()) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->touchUpdateEvent(event);
    }
}

void Application::touchBeginEvent(QTouchEvent* event) {
    _altPressed = false;
    TouchEvent thisEvent(*event); // on touch begin, we don't compare to last event
    _controllerScriptingInterface->emitTouchBeginEvent(thisEvent); // send events to any registered scripts

    _lastTouchEvent = thisEvent; // and we reset our last event to this event before we call our update
    touchUpdateEvent(event);

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isTouchCaptured()) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->touchBeginEvent(event);
    }

}

void Application::touchEndEvent(QTouchEvent* event) {
    _altPressed = false;
    TouchEvent thisEvent(*event, _lastTouchEvent);
    _controllerScriptingInterface->emitTouchEndEvent(thisEvent); // send events to any registered scripts
    _lastTouchEvent = thisEvent;

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isTouchCaptured()) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->touchEndEvent(event);
    }

    // put any application specific touch behavior below here..
}

void Application::wheelEvent(QWheelEvent* event) {
    _altPressed = false;
    _controllerScriptingInterface->emitWheelEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isWheelCaptured()) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->wheelEvent(event);
    }
}

void Application::dropEvent(QDropEvent *event) {
    const QMimeData* mimeData = event->mimeData();
    for (auto& url : mimeData->urls()) {
        QString urlString = url.toString();
        if (acceptURL(urlString, true)) {
            event->acceptProposedAction();
        }
    }
}

void Application::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

bool Application::acceptSnapshot(const QString& urlString) {
    QUrl url(urlString);
    QString snapshotPath = url.toLocalFile();

    SnapshotMetaData* snapshotData = Snapshot::parseSnapshotData(snapshotPath);
    if (snapshotData) {
        if (!snapshotData->getURL().toString().isEmpty()) {
            DependencyManager::get<AddressManager>()->handleLookupString(snapshotData->getURL().toString());
        }
    } else {
        OffscreenUi::warning("", "No location details were found in the file\n" +
                             snapshotPath + "\nTry dragging in an authentic Hifi snapshot.");
    }
    return true;
}

static uint32_t _renderedFrameIndex { INVALID_FRAME };

void Application::idle(uint64_t now) {

    updateHeartbeat();

    if (_aboutToQuit || _inPaint) {
        return; // bail early, nothing to do here.
    }

    checkChangeCursor();

    Stats::getInstance()->updateStats();
    AvatarInputs::getInstance()->update();

    // These tasks need to be done on our first idle, because we don't want the showing of
    // overlay subwindows to do a showDesktop() until after the first time through
    static bool firstIdle = true;
    if (firstIdle) {
        firstIdle = false;
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        connect(offscreenUi.data(), &OffscreenUi::showDesktop, this, &Application::showDesktop);
        _overlayConductor.setEnabled(Menu::getInstance()->isOptionChecked(MenuOption::Overlays));
    }

    auto displayPlugin = getActiveDisplayPlugin();
    // depending on whether we're throttling or not.
    // Once rendering is off on another thread we should be able to have Application::idle run at start(0) in
    // perpetuity and not expect events to get backed up.
    bool isThrottled = displayPlugin->isThrottled();
    //  Only run simulation code if more than the targetFramePeriod have passed since last time we ran
    // This attempts to lock the simulation at 60 updates per second, regardless of framerate
    float timeSinceLastUpdateUs = (float)_lastTimeUpdated.nsecsElapsed() / NSECS_PER_USEC;
    float secondsSinceLastUpdate = timeSinceLastUpdateUs / USECS_PER_SECOND;

    if (isThrottled && (timeSinceLastUpdateUs / USECS_PER_MSEC) < THROTTLED_SIM_FRAME_PERIOD_MS) {
        // Throttling both rendering and idle
        return; // bail early, we're throttled and not enough time has elapsed
    }

    auto presentCount = displayPlugin->presentCount();
    if (presentCount < _renderedFrameIndex) {
        _renderedFrameIndex = INVALID_FRAME;
    }

    // Don't saturate the main thread with rendering and simulation,
    // unless display plugin has increased by at least one frame
    if (_renderedFrameIndex == INVALID_FRAME || presentCount > _renderedFrameIndex) {
        // Record what present frame we're on
        _renderedFrameIndex = presentCount;

        // request a paint, get to it as soon as possible: high priority
        postEvent(this, new QEvent(static_cast<QEvent::Type>(Paint)), Qt::HighEventPriority);
    } else {
        // there's no use in simulating or rendering faster then the present rate.
        return;
    }

    PROFILE_RANGE(__FUNCTION__);

    // We're going to execute idle processing, so restart the last idle timer
    _lastTimeUpdated.start();

    {
        static uint64_t lastIdleStart{ now };
        uint64_t idleStartToStartDuration = now - lastIdleStart;
        if (idleStartToStartDuration != 0) {
            _simsPerSecond.updateAverage((float)USECS_PER_SECOND / (float)idleStartToStartDuration);
        }
        lastIdleStart = now;
    }

    PerformanceTimer perfTimer("idle");
    // Drop focus from _keyboardFocusedItem if no keyboard messages for 30 seconds
    if (!_keyboardFocusedItem.isInvalidID()) {
        const quint64 LOSE_FOCUS_AFTER_ELAPSED_TIME = 30 * USECS_PER_SECOND; // if idle for 30 seconds, drop focus
        quint64 elapsedSinceAcceptedKeyPress = usecTimestampNow() - _lastAcceptedKeyPress;
        if (elapsedSinceAcceptedKeyPress > LOSE_FOCUS_AFTER_ELAPSED_TIME) {
            _keyboardFocusedItem = UNKNOWN_ENTITY_ID;
        }
    }

    // Normally we check PipelineWarnings, but since idle will often take more than 10ms we only show these idle timing
    // details if we're in ExtraDebugging mode. However, the ::update() and its subcomponents will show their timing
    // details normally.
    bool showWarnings = getLogger()->extraDebugging();
    PerformanceWarning warn(showWarnings, "idle()");

    {
        PerformanceTimer perfTimer("update");
        PerformanceWarning warn(showWarnings, "Application::idle()... update()");
        static const float BIGGEST_DELTA_TIME_SECS = 0.25f;
        update(glm::clamp(secondsSinceLastUpdate, 0.0f, BIGGEST_DELTA_TIME_SECS));
    }
    {
        PerformanceTimer perfTimer("pluginIdle");
        PerformanceWarning warn(showWarnings, "Application::idle()... pluginIdle()");
        getActiveDisplayPlugin()->idle();
        auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
        foreach(auto inputPlugin, inputPlugins) {
            QString name = inputPlugin->getName();
            QAction* action = Menu::getInstance()->getActionForOption(name);
            if (action && action->isChecked()) {
                inputPlugin->idle();
            }
        }
    }
    {
        PerformanceTimer perfTimer("rest");
        PerformanceWarning warn(showWarnings, "Application::idle()... rest of it");
        _idleLoopStdev.addValue(secondsSinceLastUpdate);

        //  Record standard deviation and reset counter if needed
        const int STDEV_SAMPLES = 500;
        if (_idleLoopStdev.getSamples() > STDEV_SAMPLES) {
            _idleLoopMeasuredJitter = _idleLoopStdev.getStDev();
            _idleLoopStdev.reset();
        }
    }

    _overlayConductor.update(secondsSinceLastUpdate);

    // check for any requested background downloads.
    emit checkBackgroundDownloads();
}

float Application::getAverageSimsPerSecond() {
    uint64_t now = usecTimestampNow();

    if (now - _lastSimsPerSecondUpdate > USECS_PER_SECOND) {
        _simsPerSecondReport = _simsPerSecond.getAverage();
        _lastSimsPerSecondUpdate = now;
    }
    return _simsPerSecondReport;
}
void Application::setAvatarSimrateSample(float sample) {
    _avatarSimsPerSecond.updateAverage(sample);
}
float Application::getAvatarSimrate() {
    uint64_t now = usecTimestampNow();

    if (now - _lastAvatarSimsPerSecondUpdate > USECS_PER_SECOND) {
        _avatarSimsPerSecondReport = _avatarSimsPerSecond.getAverage();
        _lastAvatarSimsPerSecondUpdate = now;
    }
    return _avatarSimsPerSecondReport;
}

void Application::setLowVelocityFilter(bool lowVelocityFilter) {
    controller::InputDevice::setLowVelocityFilter(lowVelocityFilter);
}

ivec2 Application::getMouse() {
    auto reticlePosition = getApplicationCompositor().getReticlePosition();

    // in the HMD, the reticlePosition is the mouse position
    if (isHMDMode()) {
        return reticlePosition;
    }

    // in desktop mode, we need to map from global to widget space
    return toGlm(_glWidget->mapFromGlobal(QPoint(reticlePosition.x, reticlePosition.y)));
}

FaceTracker* Application::getActiveFaceTracker() {
    auto faceshift = DependencyManager::get<Faceshift>();
    auto dde = DependencyManager::get<DdeFaceTracker>();

    return (dde->isActive() ? static_cast<FaceTracker*>(dde.data()) :
            (faceshift->isActive() ? static_cast<FaceTracker*>(faceshift.data()) : NULL));
}

FaceTracker* Application::getSelectedFaceTracker() {
    FaceTracker* faceTracker = NULL;
#ifdef HAVE_FACESHIFT
    if (Menu::getInstance()->isOptionChecked(MenuOption::Faceshift)) {
        faceTracker = DependencyManager::get<Faceshift>().data();
    }
#endif
#ifdef HAVE_DDE
    if (Menu::getInstance()->isOptionChecked(MenuOption::UseCamera)) {
        faceTracker = DependencyManager::get<DdeFaceTracker>().data();
    }
#endif
    return faceTracker;
}

void Application::setActiveFaceTracker() {
#if defined(HAVE_FACESHIFT) || defined(HAVE_DDE)
    bool isMuted = Menu::getInstance()->isOptionChecked(MenuOption::MuteFaceTracking);
#endif
#ifdef HAVE_FACESHIFT
    auto faceshiftTracker = DependencyManager::get<Faceshift>();
    faceshiftTracker->setIsMuted(isMuted);
    faceshiftTracker->setEnabled(Menu::getInstance()->isOptionChecked(MenuOption::Faceshift) && !isMuted);
#endif
#ifdef HAVE_DDE
    bool isUsingDDE = Menu::getInstance()->isOptionChecked(MenuOption::UseCamera);
    Menu::getInstance()->getActionForOption(MenuOption::BinaryEyelidControl)->setVisible(isUsingDDE);
    Menu::getInstance()->getActionForOption(MenuOption::CoupleEyelids)->setVisible(isUsingDDE);
    Menu::getInstance()->getActionForOption(MenuOption::UseAudioForMouth)->setVisible(isUsingDDE);
    Menu::getInstance()->getActionForOption(MenuOption::VelocityFilter)->setVisible(isUsingDDE);
    Menu::getInstance()->getActionForOption(MenuOption::CalibrateCamera)->setVisible(isUsingDDE);
    auto ddeTracker = DependencyManager::get<DdeFaceTracker>();
    ddeTracker->setIsMuted(isMuted);
    ddeTracker->setEnabled(isUsingDDE && !isMuted);
#endif
}

#ifdef HAVE_IVIEWHMD
void Application::setActiveEyeTracker() {
    auto eyeTracker = DependencyManager::get<EyeTracker>();
    if (!eyeTracker->isInitialized()) {
        return;
    }

    bool isEyeTracking = Menu::getInstance()->isOptionChecked(MenuOption::SMIEyeTracking);
    bool isSimulating = Menu::getInstance()->isOptionChecked(MenuOption::SimulateEyeTracking);
    eyeTracker->setEnabled(isEyeTracking, isSimulating);

    Menu::getInstance()->getActionForOption(MenuOption::OnePointCalibration)->setEnabled(isEyeTracking && !isSimulating);
    Menu::getInstance()->getActionForOption(MenuOption::ThreePointCalibration)->setEnabled(isEyeTracking && !isSimulating);
    Menu::getInstance()->getActionForOption(MenuOption::FivePointCalibration)->setEnabled(isEyeTracking && !isSimulating);
}

void Application::calibrateEyeTracker1Point() {
    DependencyManager::get<EyeTracker>()->calibrate(1);
}

void Application::calibrateEyeTracker3Points() {
    DependencyManager::get<EyeTracker>()->calibrate(3);
}

void Application::calibrateEyeTracker5Points() {
    DependencyManager::get<EyeTracker>()->calibrate(5);
}
#endif

bool Application::exportEntities(const QString& filename, const QVector<EntityItemID>& entityIDs) {
    QVector<EntityItemPointer> entities;

    auto entityTree = getEntities()->getTree();
    auto exportTree = std::make_shared<EntityTree>();
    exportTree->createRootElement();

    glm::vec3 root(TREE_SCALE, TREE_SCALE, TREE_SCALE);
    for (auto entityID : entityIDs) {
        auto entityItem = entityTree->findEntityByEntityItemID(entityID);
        if (!entityItem) {
            continue;
        }

        auto properties = entityItem->getProperties();
        auto position = properties.getPosition();

        root.x = glm::min(root.x, position.x);
        root.y = glm::min(root.y, position.y);
        root.z = glm::min(root.z, position.z);

        entities << entityItem;
    }

    if (entities.size() == 0) {
        return false;
    }

    for (auto entityItem : entities) {
        auto properties = entityItem->getProperties();

        properties.setPosition(properties.getPosition() - root);
        exportTree->addEntity(entityItem->getEntityItemID(), properties);
    }

    // remap IDs on export so that we aren't publishing the IDs of entities in our domain
    exportTree->remapIDs();

    exportTree->writeToJSONFile(filename.toLocal8Bit().constData());

    // restore the main window's active state
    _window->activateWindow();
    return true;
}

bool Application::exportEntities(const QString& filename, float x, float y, float z, float scale) {
    QVector<EntityItemPointer> entities;
    getEntities()->getTree()->findEntities(AACube(glm::vec3(x, y, z), scale), entities);

    if (entities.size() > 0) {
        glm::vec3 root(x, y, z);
        auto exportTree = std::make_shared<EntityTree>();
        exportTree->createRootElement();

        for (int i = 0; i < entities.size(); i++) {
            EntityItemProperties properties = entities.at(i)->getProperties();
            EntityItemID id = entities.at(i)->getEntityItemID();
            properties.setPosition(properties.getPosition() - root);
            exportTree->addEntity(id, properties);
        }

        // remap IDs on export so that we aren't publishing the IDs of entities in our domain
        exportTree->remapIDs();

        exportTree->writeToSVOFile(filename.toLocal8Bit().constData());
    } else {
        qCDebug(interfaceapp) << "No models were selected";
        return false;
    }

    // restore the main window's active state
    _window->activateWindow();
    return true;
}

void Application::loadSettings() {

    sessionRunTime.set(0); // Just clean living. We're about to saveSettings, which will update value.
    DependencyManager::get<AudioClient>()->loadSettings();
    DependencyManager::get<LODManager>()->loadSettings();

    // DONT CHECK IN
    //DependencyManager::get<LODManager>()->setAutomaticLODAdjust(false);

    Menu::getInstance()->loadSettings();
    getMyAvatar()->loadData();

    _settingsLoaded = true;
}

void Application::saveSettings() {
    sessionRunTime.set(_sessionRunTimer.elapsed() / MSECS_PER_SEC);
    DependencyManager::get<AudioClient>()->saveSettings();
    DependencyManager::get<LODManager>()->saveSettings();

    Menu::getInstance()->saveSettings();
    getMyAvatar()->saveData();
    PluginManager::getInstance()->saveSettings();
}

bool Application::importEntities(const QString& urlOrFilename) {
    _entityClipboard->eraseAllOctreeElements();

    QUrl url(urlOrFilename);

    // if the URL appears to be invalid or relative, then it is probably a local file
    if (!url.isValid() || url.isRelative()) {
        url = QUrl::fromLocalFile(urlOrFilename);
    }

    bool success = _entityClipboard->readFromURL(url.toString());
    if (success) {
        _entityClipboard->remapIDs();
        _entityClipboard->reaverageOctreeElements();
    }
    return success;
}

QVector<EntityItemID> Application::pasteEntities(float x, float y, float z) {
    return _entityClipboard->sendEntities(&_entityEditSender, getEntities()->getTree(), x, y, z);
}

void Application::initDisplay() {
}

void Application::init() {
    // Make sure Login state is up to date
    DependencyManager::get<DialogsManager>()->toggleLoginDialog();

    DependencyManager::get<DeferredLightingEffect>()->init();

    DependencyManager::get<AvatarManager>()->init();
    _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);

    _mirrorCamera.setMode(CAMERA_MODE_MIRROR);

    _timerStart.start();
    _lastTimeUpdated.start();

    // when --url in command line, teleport to location
    const QString HIFI_URL_COMMAND_LINE_KEY = "--url";
    int urlIndex = arguments().indexOf(HIFI_URL_COMMAND_LINE_KEY);
    QString addressLookupString;
    if (urlIndex != -1) {
        addressLookupString = arguments().value(urlIndex + 1);
    }

    DependencyManager::get<AddressManager>()->loadSettings(addressLookupString);

    qCDebug(interfaceapp) << "Loaded settings";

    Leapmotion::init();

    // fire off an immediate domain-server check in now that settings are loaded
    DependencyManager::get<NodeList>()->sendDomainServerCheckIn();

    getEntities()->init();
    getEntities()->setViewFrustum(getViewFrustum());

    ObjectMotionState::setShapeManager(&_shapeManager);
    _physicsEngine->init();

    EntityTreePointer tree = getEntities()->getTree();
    _entitySimulation.init(tree, _physicsEngine, &_entityEditSender);
    tree->setSimulation(&_entitySimulation);

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    // connect the _entityCollisionSystem to our EntityTreeRenderer since that's what handles running entity scripts
    connect(&_entitySimulation, &EntitySimulation::entityCollisionWithEntity,
            getEntities(), &EntityTreeRenderer::entityCollisionWithEntity);

    // connect the _entities (EntityTreeRenderer) to our script engine's EntityScriptingInterface for firing
    // of events related clicking, hovering over, and entering entities
    getEntities()->connectSignalsToSlots(entityScriptingInterface.data());

    _entityClipboardRenderer.init();
    _entityClipboardRenderer.setViewFrustum(getViewFrustum());
    _entityClipboardRenderer.setTree(_entityClipboard);

    // Make sure any new sounds are loaded as soon as know about them.
    connect(tree.get(), &EntityTree::newCollisionSoundURL, DependencyManager::get<SoundCache>().data(), &SoundCache::getSound);
    connect(getMyAvatar(), &MyAvatar::newCollisionSoundURL, DependencyManager::get<SoundCache>().data(), &SoundCache::getSound);
}

void Application::updateLOD() {
    PerformanceTimer perfTimer("LOD");
    // adjust it unless we were asked to disable this feature, or if we're currently in throttleRendering mode
    if (!isThrottleRendering()) {
        DependencyManager::get<LODManager>()->autoAdjustLOD(_fps);
    } else {
        DependencyManager::get<LODManager>()->resetLODAdjust();
    }
}

// Called during Application::update immediately before AvatarManager::updateMyAvatar, updating my data that is then sent to everyone.
// (Maybe this code should be moved there?)
// The principal result is to call updateLookAtTargetAvatar() and then setLookAtPosition().
// Note that it is called BEFORE we update position or joints based on sensors, etc.
void Application::updateMyAvatarLookAtPosition() {
    PerformanceTimer perfTimer("lookAt");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMyAvatarLookAtPosition()");

    auto myAvatar = getMyAvatar();
    myAvatar->updateLookAtTargetAvatar();
    FaceTracker* faceTracker = getActiveFaceTracker();
    auto eyeTracker = DependencyManager::get<EyeTracker>();

    bool isLookingAtSomeone = false;
    bool isHMD = qApp->isHMDMode();
    glm::vec3 lookAtSpot;
    if (eyeTracker->isTracking() && (isHMD || eyeTracker->isSimulating())) {
        //  Look at the point that the user is looking at.
        glm::vec3 lookAtPosition = eyeTracker->getLookAtPosition();
        if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
            lookAtPosition.x = -lookAtPosition.x;
        }
        if (isHMD) {
            glm::mat4 headPose = getActiveDisplayPlugin()->getHeadPose();
            glm::quat hmdRotation = glm::quat_cast(headPose);
            lookAtSpot = _myCamera.getPosition() + myAvatar->getOrientation() * (hmdRotation * lookAtPosition);
        } else {
            lookAtSpot = myAvatar->getHead()->getEyePosition()
                + (myAvatar->getHead()->getFinalOrientationInWorldFrame() * lookAtPosition);
        }
    } else {
        AvatarSharedPointer lookingAt = myAvatar->getLookAtTargetAvatar().lock();
        if (lookingAt && myAvatar != lookingAt.get()) {
            //  If I am looking at someone else, look directly at one of their eyes
            isLookingAtSomeone = true;
            auto lookingAtHead = static_pointer_cast<Avatar>(lookingAt)->getHead();

            const float MAXIMUM_FACE_ANGLE = 65.0f * RADIANS_PER_DEGREE;
            glm::vec3 lookingAtFaceOrientation = lookingAtHead->getFinalOrientationInWorldFrame() * IDENTITY_FRONT;
            glm::vec3 fromLookingAtToMe = glm::normalize(myAvatar->getHead()->getEyePosition()
                - lookingAtHead->getEyePosition());
            float faceAngle = glm::angle(lookingAtFaceOrientation, fromLookingAtToMe);

            if (faceAngle < MAXIMUM_FACE_ANGLE) {
                // Randomly look back and forth between look targets
                eyeContactTarget target = Menu::getInstance()->isOptionChecked(MenuOption::FixGaze) ?
                LEFT_EYE : myAvatar->getEyeContactTarget();
                switch (target) {
                    case LEFT_EYE:
                        lookAtSpot = lookingAtHead->getLeftEyePosition();
                        break;
                    case RIGHT_EYE:
                        lookAtSpot = lookingAtHead->getRightEyePosition();
                        break;
                    case MOUTH:
                        lookAtSpot = lookingAtHead->getMouthPosition();
                        break;
                }
            } else {
                // Just look at their head (mid point between eyes)
                lookAtSpot = lookingAtHead->getEyePosition();
            }
        } else {
            //  I am not looking at anyone else, so just look forward
            if (isHMD) {
                glm::mat4 headPose = myAvatar->getHMDSensorMatrix();
                glm::quat headRotation = glm::quat_cast(headPose);
                lookAtSpot = myAvatar->getPosition() +
                    myAvatar->getOrientation() * (headRotation * glm::vec3(0.0f, 0.0f, -TREE_SCALE));
            } else {
                lookAtSpot = myAvatar->getHead()->getEyePosition() +
                    (myAvatar->getHead()->getFinalOrientationInWorldFrame() * glm::vec3(0.0f, 0.0f, -TREE_SCALE));
            }
        }

        // Deflect the eyes a bit to match the detected gaze from the face tracker if active.
        if (faceTracker && !faceTracker->isMuted()) {
            float eyePitch = faceTracker->getEstimatedEyePitch();
            float eyeYaw = faceTracker->getEstimatedEyeYaw();
            const float GAZE_DEFLECTION_REDUCTION_DURING_EYE_CONTACT = 0.1f;
            glm::vec3 origin = myAvatar->getHead()->getEyePosition();
            float deflection = faceTracker->getEyeDeflection();
            if (isLookingAtSomeone) {
                deflection *= GAZE_DEFLECTION_REDUCTION_DURING_EYE_CONTACT;
            }
            lookAtSpot = origin + _myCamera.getRotation() * glm::quat(glm::radians(glm::vec3(
                eyePitch * deflection, eyeYaw * deflection, 0.0f))) *
                glm::inverse(_myCamera.getRotation()) * (lookAtSpot - origin);
        }
    }

    myAvatar->getHead()->setLookAtPosition(lookAtSpot);
}

void Application::updateThreads(float deltaTime) {
    PerformanceTimer perfTimer("updateThreads");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateThreads()");

    // parse voxel packets
    if (!_enableProcessOctreeThread) {
        _octreeProcessor.threadRoutine();
        _entityEditSender.threadRoutine();
    }
}

void Application::toggleOverlays() {
    auto newOverlaysVisible = !_overlayConductor.getEnabled();
    Menu::getInstance()->setIsOptionChecked(MenuOption::Overlays, newOverlaysVisible);
    _overlayConductor.setEnabled(newOverlaysVisible);
}

void Application::setOverlaysVisible(bool visible) {
    _overlayConductor.setEnabled(visible);
}

void Application::cycleCamera() {
    auto menu = Menu::getInstance();
    if (menu->isOptionChecked(MenuOption::FullscreenMirror)) {

        menu->setIsOptionChecked(MenuOption::FullscreenMirror, false);
        menu->setIsOptionChecked(MenuOption::FirstPerson, true);

    } else if (menu->isOptionChecked(MenuOption::FirstPerson)) {

        menu->setIsOptionChecked(MenuOption::FirstPerson, false);
        menu->setIsOptionChecked(MenuOption::ThirdPerson, true);

    } else if (menu->isOptionChecked(MenuOption::ThirdPerson)) {

        menu->setIsOptionChecked(MenuOption::ThirdPerson, false);
        menu->setIsOptionChecked(MenuOption::FullscreenMirror, true);

    } else if (menu->isOptionChecked(MenuOption::IndependentMode) || menu->isOptionChecked(MenuOption::CameraEntityMode)) {
        // do nothing if in independent or camera entity modes
        return;
    }
    cameraMenuChanged(); // handle the menu change
}

void Application::cameraMenuChanged() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        if (_myCamera.getMode() != CAMERA_MODE_MIRROR) {
            _myCamera.setMode(CAMERA_MODE_MIRROR);
        }
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson)) {
        if (_myCamera.getMode() != CAMERA_MODE_FIRST_PERSON) {
            _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
            getMyAvatar()->setBoomLength(MyAvatar::ZOOM_MIN);
        }
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::ThirdPerson)) {
        if (_myCamera.getMode() != CAMERA_MODE_THIRD_PERSON) {
            _myCamera.setMode(CAMERA_MODE_THIRD_PERSON);
            if (getMyAvatar()->getBoomLength() == MyAvatar::ZOOM_MIN) {
                getMyAvatar()->setBoomLength(MyAvatar::ZOOM_DEFAULT);
            }
        }
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::IndependentMode)) {
        if (_myCamera.getMode() != CAMERA_MODE_INDEPENDENT) {
            _myCamera.setMode(CAMERA_MODE_INDEPENDENT);
        }
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::CameraEntityMode)) {
        if (_myCamera.getMode() != CAMERA_MODE_ENTITY) {
            _myCamera.setMode(CAMERA_MODE_ENTITY);
        }
    }
}

void Application::reloadResourceCaches() {
    // Clear entities out of view frustum
    _viewFrustum.setPosition(glm::vec3(0.0f, 0.0f, TREE_SCALE));
    _viewFrustum.setOrientation(glm::quat());
    queryOctree(NodeType::EntityServer, PacketType::EntityQuery, _entityServerJurisdictions);

    DependencyManager::get<AssetClient>()->clearCache();

    DependencyManager::get<AnimationCache>()->refreshAll();
    DependencyManager::get<ModelCache>()->refreshAll();
    DependencyManager::get<SoundCache>()->refreshAll();
    DependencyManager::get<TextureCache>()->refreshAll();

    DependencyManager::get<NodeList>()->reset();  // Force redownload of .fst models

    getMyAvatar()->resetFullAvatarURL();
}

void Application::rotationModeChanged() {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::CenterPlayerInView)) {
        getMyAvatar()->setHeadPitch(0);
    }
}

void Application::updateDialogs(float deltaTime) {
    PerformanceTimer perfTimer("updateDialogs");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateDialogs()");
    auto dialogsManager = DependencyManager::get<DialogsManager>();

    // Update audio stats dialog, if any
    AudioStatsDialog* audioStatsDialog = dialogsManager->getAudioStatsDialog();
    if(audioStatsDialog) {
        audioStatsDialog->update();
    }

    // Update bandwidth dialog, if any
    BandwidthDialog* bandwidthDialog = dialogsManager->getBandwidthDialog();
    if (bandwidthDialog) {
        bandwidthDialog->update();
    }

    QPointer<OctreeStatsDialog> octreeStatsDialog = dialogsManager->getOctreeStatsDialog();
    if (octreeStatsDialog) {
        octreeStatsDialog->update();
    }
}

void Application::update(float deltaTime) {

    PROFILE_RANGE_EX(__FUNCTION__, 0xffff0000, (uint64_t)_frameCount + 1);

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::update()");

    updateLOD();

    {
        PerformanceTimer perfTimer("devices");
        DeviceTracker::updateAll();

        FaceTracker* tracker = getSelectedFaceTracker();
        if (tracker && Menu::getInstance()->isOptionChecked(MenuOption::MuteFaceTracking) != tracker->isMuted()) {
            tracker->toggleMute();
        }

        tracker = getActiveFaceTracker();
        if (tracker && !tracker->isMuted()) {
            tracker->update(deltaTime);

            // Auto-mute microphone after losing face tracking?
            if (tracker->isTracking()) {
                _lastFaceTrackerUpdate = usecTimestampNow();
            } else {
                const quint64 MUTE_MICROPHONE_AFTER_USECS = 5000000;  //5 secs
                Menu* menu = Menu::getInstance();
                if (menu->isOptionChecked(MenuOption::AutoMuteAudio) && !menu->isOptionChecked(MenuOption::MuteAudio)) {
                    if (_lastFaceTrackerUpdate > 0
                        && ((usecTimestampNow() - _lastFaceTrackerUpdate) > MUTE_MICROPHONE_AFTER_USECS)) {
                        menu->triggerOption(MenuOption::MuteAudio);
                        _lastFaceTrackerUpdate = 0;
                    }
                } else {
                    _lastFaceTrackerUpdate = 0;
                }
            }
        } else {
            _lastFaceTrackerUpdate = 0;
        }

    }

    auto myAvatar = getMyAvatar();
    auto userInputMapper = DependencyManager::get<UserInputMapper>();

    controller::InputCalibrationData calibrationData = {
        myAvatar->getSensorToWorldMatrix(),
        createMatFromQuatAndPos(myAvatar->getOrientation(), myAvatar->getPosition()),
        myAvatar->getHMDSensorMatrix()
    };

    InputPluginPointer keyboardMousePlugin;
    bool jointsCaptured = false;
    for (auto inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->getName() == KeyboardMouseDevice::NAME) {
            keyboardMousePlugin = inputPlugin;
        } else if (inputPlugin->isActive()) {
            inputPlugin->pluginUpdate(deltaTime, calibrationData, jointsCaptured);
            if (inputPlugin->isJointController()) {
                jointsCaptured = true;
            }
        }
    }

    userInputMapper->update(deltaTime);

    if (keyboardMousePlugin && keyboardMousePlugin->isActive()) {
        keyboardMousePlugin->pluginUpdate(deltaTime, calibrationData, jointsCaptured);
    }

    _controllerScriptingInterface->updateInputControllers();

    // Transfer the user inputs to the driveKeys
    // FIXME can we drop drive keys and just have the avatar read the action states directly?
    myAvatar->clearDriveKeys();
    if (_myCamera.getMode() != CAMERA_MODE_INDEPENDENT) {
        if (!_controllerScriptingInterface->areActionsCaptured()) {
            myAvatar->setDriveKeys(TRANSLATE_Z, -1.0f * userInputMapper->getActionState(controller::Action::TRANSLATE_Z));
            myAvatar->setDriveKeys(TRANSLATE_Y, userInputMapper->getActionState(controller::Action::TRANSLATE_Y));
            myAvatar->setDriveKeys(TRANSLATE_X, userInputMapper->getActionState(controller::Action::TRANSLATE_X));
            if (deltaTime > FLT_EPSILON) {
                myAvatar->setDriveKeys(PITCH, -1.0f * userInputMapper->getActionState(controller::Action::PITCH));
                myAvatar->setDriveKeys(YAW, -1.0f * userInputMapper->getActionState(controller::Action::YAW));
                myAvatar->setDriveKeys(STEP_YAW, -1.0f * userInputMapper->getActionState(controller::Action::STEP_YAW));
            }
        }
        myAvatar->setDriveKeys(ZOOM, userInputMapper->getActionState(controller::Action::TRANSLATE_CAMERA_Z));
    }

    controller::Pose leftHandPose = userInputMapper->getPoseState(controller::Action::LEFT_HAND);
    controller::Pose rightHandPose = userInputMapper->getPoseState(controller::Action::RIGHT_HAND);
    auto myAvatarMatrix = createMatFromQuatAndPos(myAvatar->getOrientation(), myAvatar->getPosition());
    auto worldToSensorMatrix = glm::inverse(myAvatar->getSensorToWorldMatrix());
    auto avatarToSensorMatrix = worldToSensorMatrix * myAvatarMatrix;
    myAvatar->setHandControllerPosesInSensorFrame(leftHandPose.transform(avatarToSensorMatrix), rightHandPose.transform(avatarToSensorMatrix));

    updateThreads(deltaTime); // If running non-threaded, then give the threads some time to process...
    updateDialogs(deltaTime); // update various stats dialogs if present

    QSharedPointer<AvatarManager> avatarManager = DependencyManager::get<AvatarManager>();

    if (_physicsEnabled) {
        PROFILE_RANGE_EX("Physics", 0xffff0000, (uint64_t)getActiveDisplayPlugin()->presentCount());

        PerformanceTimer perfTimer("physics");

        {
            PROFILE_RANGE_EX("UpdateStats", 0xffffff00, (uint64_t)getActiveDisplayPlugin()->presentCount());

            PerformanceTimer perfTimer("updateStates)");
            static VectorOfMotionStates motionStates;
            _entitySimulation.getObjectsToRemoveFromPhysics(motionStates);
            _physicsEngine->removeObjects(motionStates);
            _entitySimulation.deleteObjectsRemovedFromPhysics();

            getEntities()->getTree()->withReadLock([&] {
                _entitySimulation.getObjectsToAddToPhysics(motionStates);
                _physicsEngine->addObjects(motionStates);

            });
            getEntities()->getTree()->withReadLock([&] {
                _entitySimulation.getObjectsToChange(motionStates);
                VectorOfMotionStates stillNeedChange = _physicsEngine->changeObjects(motionStates);
                _entitySimulation.setObjectsToChange(stillNeedChange);
            });

            _entitySimulation.applyActionChanges();

             avatarManager->getObjectsToRemoveFromPhysics(motionStates);
            _physicsEngine->removeObjects(motionStates);
            avatarManager->getObjectsToAddToPhysics(motionStates);
            _physicsEngine->addObjects(motionStates);
            avatarManager->getObjectsToChange(motionStates);
            _physicsEngine->changeObjects(motionStates);

            myAvatar->prepareForPhysicsSimulation();
            _physicsEngine->forEachAction([&](EntityActionPointer action) {
                action->prepareForPhysicsSimulation();
            });
        }
        {
            PROFILE_RANGE_EX("StepSimulation", 0xffff8000, (uint64_t)getActiveDisplayPlugin()->presentCount());
            PerformanceTimer perfTimer("stepSimulation");
            getEntities()->getTree()->withWriteLock([&] {
                _physicsEngine->stepSimulation();
            });
        }
        {
            PROFILE_RANGE_EX("HarvestChanges", 0xffffff00, (uint64_t)getActiveDisplayPlugin()->presentCount());
            PerformanceTimer perfTimer("havestChanges");
            if (_physicsEngine->hasOutgoingChanges()) {
                getEntities()->getTree()->withWriteLock([&] {
                    const VectorOfMotionStates& outgoingChanges = _physicsEngine->getOutgoingChanges();
                    _entitySimulation.handleOutgoingChanges(outgoingChanges, Physics::getSessionUUID());
                    avatarManager->handleOutgoingChanges(outgoingChanges);
                });

                auto collisionEvents = _physicsEngine->getCollisionEvents();
                avatarManager->handleCollisionEvents(collisionEvents);

                _physicsEngine->dumpStatsIfNecessary();

                if (!_aboutToQuit) {
                    PerformanceTimer perfTimer("entities");
                    // Collision events (and their scripts) must not be handled when we're locked, above. (That would risk
                    // deadlock.)
                    _entitySimulation.handleCollisionEvents(collisionEvents);
                    // NOTE: the getEntities()->update() call below will wait for lock
                    // and will simulate entity motion (the EntityTree has been given an EntitySimulation).
                    getEntities()->update(); // update the models...
                }

                myAvatar->harvestResultsFromPhysicsSimulation(deltaTime);
            }
        }
    }

    // AvatarManager update
    {
        PerformanceTimer perfTimer("AvatarManger");

        qApp->setAvatarSimrateSample(1.0f / deltaTime);

        {
            PROFILE_RANGE_EX("OtherAvatars", 0xffff00ff, (uint64_t)getActiveDisplayPlugin()->presentCount());
            avatarManager->updateOtherAvatars(deltaTime);
        }

        qApp->updateMyAvatarLookAtPosition();

        // update sensorToWorldMatrix for camera and hand controllers
        myAvatar->updateSensorToWorldMatrix();

        {
            PROFILE_RANGE_EX("MyAvatar", 0xffff00ff, (uint64_t)getActiveDisplayPlugin()->presentCount());
            avatarManager->updateMyAvatar(deltaTime);
        }
    }

    {
        PROFILE_RANGE_EX("Overlays", 0xffff0000, (uint64_t)getActiveDisplayPlugin()->presentCount());
        PerformanceTimer perfTimer("overlays");
        _overlays.update(deltaTime);
    }

    // Update _viewFrustum with latest camera and view frustum data...
    // NOTE: we get this from the view frustum, to make it simpler, since the
    // loadViewFrumstum() method will get the correct details from the camera
    // We could optimize this to not actually load the viewFrustum, since we don't
    // actually need to calculate the view frustum planes to send these details
    // to the server.
    {
        PerformanceTimer perfTimer("loadViewFrustum");
        loadViewFrustum(_myCamera, _viewFrustum);
    }

    quint64 now = usecTimestampNow();

    // Update my voxel servers with my current voxel query...
    {
        PROFILE_RANGE_EX("QueryOctree", 0xffff0000, (uint64_t)getActiveDisplayPlugin()->presentCount());
        PerformanceTimer perfTimer("queryOctree");
        quint64 sinceLastQuery = now - _lastQueriedTime;
        const quint64 TOO_LONG_SINCE_LAST_QUERY = 3 * USECS_PER_SECOND;
        bool queryIsDue = sinceLastQuery > TOO_LONG_SINCE_LAST_QUERY;
        bool viewIsDifferentEnough = !_lastQueriedViewFrustum.isVerySimilar(_viewFrustum);

        // if it's been a while since our last query or the view has significantly changed then send a query, otherwise suppress it
        if (queryIsDue || viewIsDifferentEnough) {
            _lastQueriedTime = now;

            if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
                queryOctree(NodeType::EntityServer, PacketType::EntityQuery, _entityServerJurisdictions);
            }
            _lastQueriedViewFrustum = _viewFrustum;
        }
    }

    // sent nack packets containing missing sequence numbers of received packets from nodes
    {
        quint64 sinceLastNack = now - _lastNackTime;
        const quint64 TOO_LONG_SINCE_LAST_NACK = 1 * USECS_PER_SECOND;
        if (sinceLastNack > TOO_LONG_SINCE_LAST_NACK) {
            _lastNackTime = now;
            sendNackPackets();
        }
    }

    // send packet containing downstream audio stats to the AudioMixer
    {
        quint64 sinceLastNack = now - _lastSendDownstreamAudioStats;
        if (sinceLastNack > TOO_LONG_SINCE_LAST_SEND_DOWNSTREAM_AUDIO_STATS) {
            _lastSendDownstreamAudioStats = now;

            QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "sendDownstreamAudioStatsPacket", Qt::QueuedConnection);
        }
    }
}


int Application::sendNackPackets() {

    if (Menu::getInstance()->isOptionChecked(MenuOption::DisableNackPackets)) {
        return 0;
    }

    // iterates through all nodes in NodeList
    auto nodeList = DependencyManager::get<NodeList>();

    int packetsSent = 0;

    nodeList->eachNode([&](const SharedNodePointer& node){

        if (node->getActiveSocket() && node->getType() == NodeType::EntityServer) {

            auto nackPacketList = NLPacketList::create(PacketType::OctreeDataNack);

            QUuid nodeUUID = node->getUUID();

            // if there are octree packets from this node that are waiting to be processed,
            // don't send a NACK since the missing packets may be among those waiting packets.
            if (_octreeProcessor.hasPacketsToProcessFrom(nodeUUID)) {
                return;
            }

            QSet<OCTREE_PACKET_SEQUENCE> missingSequenceNumbers;
            _octreeServerSceneStats.withReadLock([&] {
                // retreive octree scene stats of this node
                if (_octreeServerSceneStats.find(nodeUUID) == _octreeServerSceneStats.end()) {
                    return;
                }
                // get sequence number stats of node, prune its missing set, and make a copy of the missing set
                SequenceNumberStats& sequenceNumberStats = _octreeServerSceneStats[nodeUUID].getIncomingOctreeSequenceNumberStats();
                sequenceNumberStats.pruneMissingSet();
                missingSequenceNumbers = sequenceNumberStats.getMissingSet();
            });

            // construct nack packet(s) for this node
            foreach(const OCTREE_PACKET_SEQUENCE& missingNumber, missingSequenceNumbers) {
                nackPacketList->writePrimitive(missingNumber);
            }

            if (nackPacketList->getNumPackets()) {
                packetsSent += (int)nackPacketList->getNumPackets();

                // send the packet list
                nodeList->sendPacketList(std::move(nackPacketList), *node);
            }
        }
    });


    return packetsSent;
}

void Application::queryOctree(NodeType_t serverType, PacketType packetType, NodeToJurisdictionMap& jurisdictions) {

    if (!_settingsLoaded) {
        return; // bail early if settings are not loaded
    }

    //qCDebug(interfaceapp) << ">>> inside... queryOctree()... _viewFrustum.getFieldOfView()=" << _viewFrustum.getFieldOfView();
    bool wantExtraDebugging = getLogger()->extraDebugging();

    _octreeQuery.setCameraPosition(_viewFrustum.getPosition());
    _octreeQuery.setCameraOrientation(_viewFrustum.getOrientation());
    _octreeQuery.setCameraFov(_viewFrustum.getFieldOfView());
    _octreeQuery.setCameraAspectRatio(_viewFrustum.getAspectRatio());
    _octreeQuery.setCameraNearClip(_viewFrustum.getNearClip());
    _octreeQuery.setCameraFarClip(_viewFrustum.getFarClip());
    _octreeQuery.setCameraEyeOffsetPosition(glm::vec3());
    _octreeQuery.setCameraCenterRadius(_viewFrustum.getCenterRadius());
    auto lodManager = DependencyManager::get<LODManager>();
    _octreeQuery.setOctreeSizeScale(lodManager->getOctreeSizeScale());
    _octreeQuery.setBoundaryLevelAdjust(lodManager->getBoundaryLevelAdjust());

    // Iterate all of the nodes, and get a count of how many octree servers we have...
    int totalServers = 0;
    int inViewServers = 0;
    int unknownJurisdictionServers = 0;

    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->eachNode([&](const SharedNodePointer& node) {
        // only send to the NodeTypes that are serverType
        if (node->getActiveSocket() && node->getType() == serverType) {
            totalServers++;

            // get the server bounds for this server
            QUuid nodeUUID = node->getUUID();

            // if we haven't heard from this voxel server, go ahead and send it a query, so we
            // can get the jurisdiction...
            if (jurisdictions.find(nodeUUID) == jurisdictions.end()) {
                unknownJurisdictionServers++;
            } else {
                const JurisdictionMap& map = (jurisdictions)[nodeUUID];

                unsigned char* rootCode = map.getRootOctalCode();

                if (rootCode) {
                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode, rootDetails);
                    AACube serverBounds(glm::vec3(rootDetails.x * TREE_SCALE,
                                                  rootDetails.y * TREE_SCALE,
                                                  rootDetails.z * TREE_SCALE) - glm::vec3(HALF_TREE_SCALE),
                                        rootDetails.s * TREE_SCALE);
                    if (_viewFrustum.cubeIntersectsKeyhole(serverBounds)) {
                        inViewServers++;
                    }
                }
            }
        }
    });

    if (wantExtraDebugging) {
        qCDebug(interfaceapp, "Servers: total %d, in view %d, unknown jurisdiction %d",
            totalServers, inViewServers, unknownJurisdictionServers);
    }

    int perServerPPS = 0;
    const int SMALL_BUDGET = 10;
    int perUnknownServer = SMALL_BUDGET;
    int totalPPS = getMaxOctreePacketsPerSecond();

    // determine PPS based on number of servers
    if (inViewServers >= 1) {
        // set our preferred PPS to be exactly evenly divided among all of the voxel servers... and allocate 1 PPS
        // for each unknown jurisdiction server
        perServerPPS = (totalPPS / inViewServers) - (unknownJurisdictionServers * perUnknownServer);
    } else {
        if (unknownJurisdictionServers > 0) {
            perUnknownServer = (totalPPS / unknownJurisdictionServers);
        }
    }

    if (wantExtraDebugging) {
        qCDebug(interfaceapp, "perServerPPS: %d perUnknownServer: %d", perServerPPS, perUnknownServer);
    }

    auto queryPacket = NLPacket::create(packetType);

    nodeList->eachNode([&](const SharedNodePointer& node){
        // only send to the NodeTypes that are serverType
        if (node->getActiveSocket() && node->getType() == serverType) {

            // get the server bounds for this server
            QUuid nodeUUID = node->getUUID();

            bool inView = false;
            bool unknownView = false;

            // if we haven't heard from this voxel server, go ahead and send it a query, so we
            // can get the jurisdiction...
            if (jurisdictions.find(nodeUUID) == jurisdictions.end()) {
                unknownView = true; // assume it's in view
                if (wantExtraDebugging) {
                    qCDebug(interfaceapp) << "no known jurisdiction for node " << *node << ", assume it's visible.";
                }
            } else {
                const JurisdictionMap& map = (jurisdictions)[nodeUUID];

                unsigned char* rootCode = map.getRootOctalCode();

                if (rootCode) {
                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode, rootDetails);
                    AACube serverBounds(glm::vec3(rootDetails.x * TREE_SCALE,
                                                  rootDetails.y * TREE_SCALE,
                                                  rootDetails.z * TREE_SCALE) - glm::vec3(HALF_TREE_SCALE),
                                        rootDetails.s * TREE_SCALE);


                    inView = _viewFrustum.cubeIntersectsKeyhole(serverBounds);
                } else {
                    if (wantExtraDebugging) {
                        qCDebug(interfaceapp) << "Jurisdiction without RootCode for node " << *node << ". That's unusual!";
                    }
                }
            }

            if (inView) {
                _octreeQuery.setMaxQueryPacketsPerSecond(perServerPPS);
            } else if (unknownView) {
                if (wantExtraDebugging) {
                    qCDebug(interfaceapp) << "no known jurisdiction for node " << *node << ", give it budget of "
                                            << perUnknownServer << " to send us jurisdiction.";
                }

                // set the query's position/orientation to be degenerate in a manner that will get the scene quickly
                // If there's only one server, then don't do this, and just let the normal voxel query pass through
                // as expected... this way, we will actually get a valid scene if there is one to be seen
                if (totalServers > 1) {
                    _octreeQuery.setCameraPosition(glm::vec3(-0.1,-0.1,-0.1));
                    const glm::quat OFF_IN_NEGATIVE_SPACE = glm::quat(-0.5, 0, -0.5, 1.0);
                    _octreeQuery.setCameraOrientation(OFF_IN_NEGATIVE_SPACE);
                    _octreeQuery.setCameraNearClip(0.1f);
                    _octreeQuery.setCameraFarClip(0.1f);
                    if (wantExtraDebugging) {
                        qCDebug(interfaceapp) << "Using 'minimal' camera position for node" << *node;
                    }
                } else {
                    if (wantExtraDebugging) {
                        qCDebug(interfaceapp) << "Using regular camera position for node" << *node;
                    }
                }
                _octreeQuery.setMaxQueryPacketsPerSecond(perUnknownServer);
            } else {
                _octreeQuery.setMaxQueryPacketsPerSecond(0);
            }

            // encode the query data
            int packetSize = _octreeQuery.getBroadcastData(reinterpret_cast<unsigned char*>(queryPacket->getPayload()));
            queryPacket->setPayloadSize(packetSize);

            // make sure we still have an active socket
            nodeList->sendUnreliablePacket(*queryPacket, *node);
        }
    });
}


bool Application::isHMDMode() const {
    return getActiveDisplayPlugin()->isHmd();
}
float Application::getTargetFrameRate() { return getActiveDisplayPlugin()->getTargetFrameRate(); }

QRect Application::getDesirableApplicationGeometry() {
    QRect applicationGeometry = getWindow()->geometry();

    // If our parent window is on the HMD, then don't use its geometry, instead use
    // the "main screen" geometry.
    HMDToolsDialog* hmdTools = DependencyManager::get<DialogsManager>()->getHMDToolsDialog();
    if (hmdTools && hmdTools->hasHMDScreen()) {
        QScreen* hmdScreen = hmdTools->getHMDScreen();
        QWindow* appWindow = getWindow()->windowHandle();
        QScreen* appScreen = appWindow->screen();

        // if our app's screen is the hmd screen, we don't want to place the
        // running scripts widget on it. So we need to pick a better screen.
        // we will use the screen for the HMDTools since it's a guarenteed
        // better screen.
        if (appScreen == hmdScreen) {
            QScreen* betterScreen = hmdTools->windowHandle()->screen();
            applicationGeometry = betterScreen->geometry();
        }
    }
    return applicationGeometry;
}

/////////////////////////////////////////////////////////////////////////////////////
// loadViewFrustum()
//
// Description: this will load the view frustum bounds for EITHER the head
//                 or the "myCamera".
//
void Application::loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum) {
    PROFILE_RANGE(__FUNCTION__);
    // We will use these below, from either the camera or head vectors calculated above
    viewFrustum.setProjection(camera.getProjection());

    // Set the viewFrustum up with the correct position and orientation of the camera
    viewFrustum.setPosition(camera.getPosition());
    viewFrustum.setOrientation(camera.getRotation());

    // Ask the ViewFrustum class to calculate our corners
    viewFrustum.calculate();
}

glm::vec3 Application::getSunDirection() {
    // Sun direction is in fact just the location of the sun relative to the origin
    auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
    return skyStage->getSunLight()->getDirection();
}

// FIXME, preprocessor guard this check to occur only in DEBUG builds
static QThread * activeRenderingThread = nullptr;

PickRay Application::computePickRay(float x, float y) const {
    vec2 pickPoint { x, y };
    PickRay result;
    if (isHMDMode()) {
        getApplicationCompositor().computeHmdPickRay(pickPoint, result.origin, result.direction);
    } else {
        pickPoint /= getCanvasSize();
        getViewFrustum()->computePickRay(pickPoint.x, pickPoint.y, result.origin, result.direction);
    }
    return result;
}

MyAvatar* Application::getMyAvatar() const {
    return DependencyManager::get<AvatarManager>()->getMyAvatar();
}

glm::vec3 Application::getAvatarPosition() const {
    return getMyAvatar()->getPosition();
}

ViewFrustum* Application::getViewFrustum() {
#ifdef DEBUG
    if (QThread::currentThread() == activeRenderingThread) {
        // FIXME, figure out a better way to do this
        //qWarning() << "Calling Application::getViewFrustum() from the active rendering thread, did you mean Application::getDisplayViewFrustum()?";
    }
#endif
    return &_viewFrustum;
}

const ViewFrustum* Application::getViewFrustum() const {
#ifdef DEBUG
    if (QThread::currentThread() == activeRenderingThread) {
        // FIXME, figure out a better way to do this
        //qWarning() << "Calling Application::getViewFrustum() from the active rendering thread, did you mean Application::getDisplayViewFrustum()?";
    }
#endif
    return &_viewFrustum;
}

ViewFrustum* Application::getDisplayViewFrustum() {
#ifdef DEBUG
    if (QThread::currentThread() != activeRenderingThread) {
        // FIXME, figure out a better way to do this
        // qWarning() << "Calling Application::getDisplayViewFrustum() from outside the active rendering thread or outside rendering, did you mean Application::getViewFrustum()?";
    }
#endif
    return &_displayViewFrustum;
}

const ViewFrustum* Application::getDisplayViewFrustum() const {
#ifdef DEBUG
    if (QThread::currentThread() != activeRenderingThread) {
        // FIXME, figure out a better way to do this
        // qWarning() << "Calling Application::getDisplayViewFrustum() from outside the active rendering thread or outside rendering, did you mean Application::getViewFrustum()?";
    }
#endif
    return &_displayViewFrustum;
}

// WorldBox Render Data & rendering functions

class WorldBoxRenderData {
public:
    typedef render::Payload<WorldBoxRenderData> Payload;
    typedef Payload::DataPointer Pointer;

    int _val = 0;
    static render::ItemID _item; // unique WorldBoxRenderData
};

render::ItemID WorldBoxRenderData::_item { render::Item::INVALID_ITEM_ID };

namespace render {
    template <> const ItemKey payloadGetKey(const WorldBoxRenderData::Pointer& stuff) { return ItemKey::Builder::opaqueShape(); }
    template <> const Item::Bound payloadGetBound(const WorldBoxRenderData::Pointer& stuff) { return Item::Bound(); }
    template <> void payloadRender(const WorldBoxRenderData::Pointer& stuff, RenderArgs* args) {
        if (args->_renderMode != RenderArgs::MIRROR_RENDER_MODE && Menu::getInstance()->isOptionChecked(MenuOption::WorldAxes)) {
            PerformanceTimer perfTimer("worldBox");

            auto& batch = *args->_batch;
            DependencyManager::get<GeometryCache>()->bindSimpleProgram(batch);
            renderWorldBox(batch);
        }
    }
}

// Background Render Data & rendering functions
class BackgroundRenderData {
public:
    typedef render::Payload<BackgroundRenderData> Payload;
    typedef Payload::DataPointer Pointer;

    Stars _stars;

    static render::ItemID _item; // unique WorldBoxRenderData
};

render::ItemID BackgroundRenderData::_item = 0;

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
                    skybox->render(batch, *(args->_viewFrustum));
                    break;
                }
            }

            // Fall through: if no skybox is available, render the SKY_DOME
            case model::SunSkyStage::SKY_DOME:  {
                if (Menu::getInstance()->isOptionChecked(MenuOption::Stars)) {
                    PerformanceTimer perfTimer("stars");
                    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                        "Application::payloadRender<BackgroundRenderData>() ... My god, it's full of stars...");
                    // should be the first rendering pass - w/o depth buffer / lighting

                    static const float alpha = 1.0f;
                    background->_stars.render(args, alpha);
                }
            }
                break;

            case model::SunSkyStage::NO_BACKGROUND:
            default:
                // this line intentionally left blank
                break;
        }
    }
}


void Application::displaySide(RenderArgs* renderArgs, Camera& theCamera, bool selfAvatarOnly) {

    // FIXME: This preRender call is temporary until we create a separate render::scene for the mirror rendering.
    // Then we can move this logic into the Avatar::simulate call.
    auto myAvatar = getMyAvatar();
    myAvatar->preRender(renderArgs);

    // Update animation debug draw renderer
    AnimDebugDraw::getInstance().update();

    activeRenderingThread = QThread::currentThread();
    PROFILE_RANGE(__FUNCTION__);
    PerformanceTimer perfTimer("display");
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "Application::displaySide()");

    // load the view frustum
    loadViewFrustum(theCamera, _displayViewFrustum);

    // TODO fix shadows and make them use the GPU library

    // The pending changes collecting the changes here
    render::PendingChanges pendingChanges;

    // FIXME: Move this out of here!, Background / skybox should be driven by the enityt content just like the other entities
    // Background rendering decision
    if (!render::Item::isValidID(BackgroundRenderData::_item)) {
        auto backgroundRenderData = make_shared<BackgroundRenderData>();
        auto backgroundRenderPayload = make_shared<BackgroundRenderData::Payload>(backgroundRenderData);
        BackgroundRenderData::_item = _main3DScene->allocateID();
        pendingChanges.resetItem(BackgroundRenderData::_item, backgroundRenderPayload);
    }

    // Assuming nothing get's rendered through that
    if (!selfAvatarOnly) {
        if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
            // render models...
            PerformanceTimer perfTimer("entities");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... entities...");

            RenderArgs::DebugFlags renderDebugFlags = RenderArgs::RENDER_DEBUG_NONE;

            if (Menu::getInstance()->isOptionChecked(MenuOption::PhysicsShowHulls)) {
                renderDebugFlags = (RenderArgs::DebugFlags) (renderDebugFlags | (int)RenderArgs::RENDER_DEBUG_HULLS);
            }
            renderArgs->_debugFlags = renderDebugFlags;
            //ViveControllerManager::getInstance().updateRendering(renderArgs, _main3DScene, pendingChanges);
        }
    }

    // FIXME: Move this out of here!, WorldBox should be driven by the entity content just like the other entities
    // Make sure the WorldBox is in the scene
    if (!render::Item::isValidID(WorldBoxRenderData::_item)) {
        auto worldBoxRenderData = make_shared<WorldBoxRenderData>();
        auto worldBoxRenderPayload = make_shared<WorldBoxRenderData::Payload>(worldBoxRenderData);

        WorldBoxRenderData::_item = _main3DScene->allocateID();

        pendingChanges.resetItem(WorldBoxRenderData::_item, worldBoxRenderPayload);
    } else {
        pendingChanges.updateItem<WorldBoxRenderData>(WorldBoxRenderData::_item,
            [](WorldBoxRenderData& payload) {
            payload._val++;
        });
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

    // For now every frame pass the renderContext
    {
        PerformanceTimer perfTimer("EngineRun");

        renderArgs->_viewFrustum = getDisplayViewFrustum();
        _renderEngine->getRenderContext()->args = renderArgs;

        // Before the deferred pass, let's try to use the render engine
        _renderEngine->run();
    }

    activeRenderingThread = nullptr;
}

void Application::renderRearViewMirror(RenderArgs* renderArgs, const QRect& region) {
    auto originalViewport = renderArgs->_viewport;
    // Grab current viewport to reset it at the end

    float aspect = (float)region.width() / region.height();
    float fov = MIRROR_FIELD_OF_VIEW;

    auto myAvatar = getMyAvatar();

    // bool eyeRelativeCamera = false;
    if (!AvatarInputs::getInstance()->mirrorZoomed()) {
        _mirrorCamera.setPosition(myAvatar->getChestPosition() +
                                  myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_REARVIEW_BODY_DISTANCE * myAvatar->getScale());

    } else { // HEAD zoom level
        // FIXME note that the positioing of the camera relative to the avatar can suffer limited
        // precision as the user's position moves further away from the origin.  Thus at
        // /1e7,1e7,1e7 (well outside the buildable volume) the mirror camera veers and sways
        // wildly as you rotate your avatar because the floating point values are becoming
        // larger, squeezing out the available digits of precision you have available at the
        // human scale for camera positioning.

        // Previously there was a hack to correct this using the mechanism of repositioning
        // the avatar at the origin of the world for the purposes of rendering the mirror,
        // but it resulted in failing to render the avatar's head model in the mirror view
        // when in first person mode.  Presumably this was because of some missed culling logic
        // that was not accounted for in the hack.

        // This was removed in commit 71e59cfa88c6563749594e25494102fe01db38e9 but could be further
        // investigated in order to adapt the technique while fixing the head rendering issue,
        // but the complexity of the hack suggests that a better approach
        _mirrorCamera.setPosition(myAvatar->getDefaultEyePosition() +
                                    myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_REARVIEW_DISTANCE * myAvatar->getScale());
    }
    _mirrorCamera.setProjection(glm::perspective(glm::radians(fov), aspect, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
    _mirrorCamera.setRotation(myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI, 0.0f)));


    // set the bounds of rear mirror view
    // the region is in device independent coordinates; must convert to device
    float ratio = (float)QApplication::desktop()->windowHandle()->devicePixelRatio() * getRenderResolutionScale();
    int width = region.width() * ratio;
    int height = region.height() * ratio;
    gpu::Vec4i viewport = gpu::Vec4i(0, 0, width, height);
    renderArgs->_viewport = viewport;

    // render rear mirror view
    displaySide(renderArgs, _mirrorCamera, true);

    renderArgs->_viewport =  originalViewport;
}

void Application::resetSensors(bool andReload) {
    DependencyManager::get<Faceshift>()->reset();
    DependencyManager::get<DdeFaceTracker>()->reset();
    DependencyManager::get<EyeTracker>()->reset();
    getActiveDisplayPlugin()->resetSensors();
    getMyAvatar()->reset(andReload);
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "reset", Qt::QueuedConnection);
}

void Application::updateWindowTitle(){

    QString buildVersion = " (build " + applicationVersion() + ")";
    auto nodeList = DependencyManager::get<NodeList>();

    QString connectionStatus = nodeList->getDomainHandler().isConnected() ? "" : " (NOT CONNECTED) ";
    QString username = AccountManager::getInstance().getAccountInfo().getUsername();
    QString currentPlaceName = DependencyManager::get<AddressManager>()->getHost();

    if (currentPlaceName.isEmpty()) {
        currentPlaceName = nodeList->getDomainHandler().getHostname();
    }

    QString title = QString() + (!username.isEmpty() ? username + " @ " : QString())
        + currentPlaceName + connectionStatus + buildVersion;

#ifndef WIN32
    // crashes with vs2013/win32
    qCDebug(interfaceapp, "Application title set to: %s", title.toStdString().c_str());
#endif
    _window->setWindowTitle(title);
}

void Application::clearDomainOctreeDetails() {
    qCDebug(interfaceapp) << "Clearing domain octree details...";
    // reset the environment so that we don't erroneously end up with multiple

    _physicsEnabled = false;

    // reset our node to stats and node to jurisdiction maps... since these must be changing...
    _entityServerJurisdictions.withWriteLock([&] {
        _entityServerJurisdictions.clear();
    });

    _octreeServerSceneStats.withWriteLock([&] {
        _octreeServerSceneStats.clear();
    });

    // reset the model renderer
    getEntities()->clear();

    auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
    skyStage->setBackgroundMode(model::SunSkyStage::SKY_DOME);

}

void Application::domainChanged(const QString& domainHostname) {
    updateWindowTitle();
    clearDomainOctreeDetails();
    // disable physics until we have enough information about our new location to not cause craziness.
    _physicsEnabled = false;
}


void Application::resettingDomain() {
    _notifiedPacketVersionMismatchThisDomain = false;
}

void Application::nodeAdded(SharedNodePointer node) {
    if (node->getType() == NodeType::AvatarMixer) {
        // new avatar mixer, send off our identity packet right away
        getMyAvatar()->sendIdentityPacket();
    }
}

void Application::nodeKilled(SharedNodePointer node) {

    // These are here because connecting NodeList::nodeKilled to OctreePacketProcessor::nodeKilled doesn't work:
    // OctreePacketProcessor::nodeKilled is not being called when NodeList::nodeKilled is emitted.
    // This may have to do with GenericThread::threadRoutine() blocking the QThread event loop

    _octreeProcessor.nodeKilled(node);

    _entityEditSender.nodeKilled(node);

    if (node->getType() == NodeType::AudioMixer) {
        QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "audioMixerKilled");
    }

    if (node->getType() == NodeType::EntityServer) {

        QUuid nodeUUID = node->getUUID();
        // see if this is the first we've heard of this node...
        _entityServerJurisdictions.withReadLock([&] {
            if (_entityServerJurisdictions.find(nodeUUID) == _entityServerJurisdictions.end()) {
                return;
            }

            unsigned char* rootCode = _entityServerJurisdictions[nodeUUID].getRootOctalCode();
            VoxelPositionSize rootDetails;
            voxelDetailsForCode(rootCode, rootDetails);

            qCDebug(interfaceapp, "model server going away...... v[%f, %f, %f, %f]",
                (double)rootDetails.x, (double)rootDetails.y, (double)rootDetails.z, (double)rootDetails.s);

        });

        // If the model server is going away, remove it from our jurisdiction map so we don't send voxels to a dead server
        _entityServerJurisdictions.withWriteLock([&] {
            _entityServerJurisdictions.erase(_entityServerJurisdictions.find(nodeUUID));
        });

        // also clean up scene stats for that server
        _octreeServerSceneStats.withWriteLock([&] {
            if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
                _octreeServerSceneStats.erase(nodeUUID);
            }
        });
    } else if (node->getType() == NodeType::AvatarMixer) {
        // our avatar mixer has gone away - clear the hash of avatars
        DependencyManager::get<AvatarManager>()->clearOtherAvatars();
    }
}
void Application::trackIncomingOctreePacket(ReceivedMessage& message, SharedNodePointer sendingNode, bool wasStatsPacket) {
    // Attempt to identify the sender from its address.
    if (sendingNode) {
        const QUuid& nodeUUID = sendingNode->getUUID();

        // now that we know the node ID, let's add these stats to the stats for that node...
        _octreeServerSceneStats.withWriteLock([&] {
            if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
                OctreeSceneStats& stats = _octreeServerSceneStats[nodeUUID];
                stats.trackIncomingOctreePacket(message, wasStatsPacket, sendingNode->getClockSkewUsec());
            }
        });
    }
}

bool Application::nearbyEntitiesAreReadyForPhysics() {
    // this is used to avoid the following scenario:
    // A table has some items sitting on top of it.  The items are at rest, meaning they aren't active in bullet.
    // Someone logs in close to the table.  They receive information about the items on the table before they
    // receive information about the table.  The items are very close to the avatar's capsule, so they become
    // activated in bullet.  This causes them to fall to the floor, because the table's shape isn't yet in bullet.
    EntityTreePointer entityTree = getEntities()->getTree();
    if (!entityTree) {
        return false;
    }

    QVector<EntityItemPointer> entities;
    entityTree->withReadLock([&] {
        AABox box(getMyAvatar()->getPosition() - glm::vec3(PHYSICS_READY_RANGE), glm::vec3(2 * PHYSICS_READY_RANGE));
        entityTree->findEntities(box, entities);
    });

    foreach (EntityItemPointer entity, entities) {
        if (entity->shouldBePhysical() && !entity->isReadyToComputeShape()) {
            static QString repeatedMessage =
                LogHandler::getInstance().addRepeatedMessageRegex("Physics disabled until entity loads: .*");
            qCDebug(interfaceapp) << "Physics disabled until entity loads: " << entity->getID() << entity->getName();
            return false;
        }
    }
    return true;
}

int Application::processOctreeStats(ReceivedMessage& message, SharedNodePointer sendingNode) {
    // But, also identify the sender, and keep track of the contained jurisdiction root for this server

    // parse the incoming stats datas stick it in a temporary object for now, while we
    // determine which server it belongs to
    int statsMessageLength = 0;

    const QUuid& nodeUUID = sendingNode->getUUID();

    // now that we know the node ID, let's add these stats to the stats for that node...
    _octreeServerSceneStats.withWriteLock([&] {
        OctreeSceneStats& octreeStats = _octreeServerSceneStats[nodeUUID];
        statsMessageLength = octreeStats.unpackFromPacket(message);

        // see if this is the first we've heard of this node...
        NodeToJurisdictionMap* jurisdiction = NULL;
        QString serverType;
        if (sendingNode->getType() == NodeType::EntityServer) {
            jurisdiction = &_entityServerJurisdictions;
            serverType = "Entity";
        }

        jurisdiction->withReadLock([&] {
            if (jurisdiction->find(nodeUUID) != jurisdiction->end()) {
                return;
            }

            VoxelPositionSize rootDetails;
            voxelDetailsForCode(octreeStats.getJurisdictionRoot(), rootDetails);

            qCDebug(interfaceapp, "stats from new %s server... [%f, %f, %f, %f]",
                qPrintable(serverType),
                (double)rootDetails.x, (double)rootDetails.y, (double)rootDetails.z, (double)rootDetails.s);
        });
        // store jurisdiction details for later use
        // This is bit of fiddling is because JurisdictionMap assumes it is the owner of the values used to construct it
        // but OctreeSceneStats thinks it's just returning a reference to its contents. So we need to make a copy of the
        // details from the OctreeSceneStats to construct the JurisdictionMap
        JurisdictionMap jurisdictionMap;
        jurisdictionMap.copyContents(octreeStats.getJurisdictionRoot(), octreeStats.getJurisdictionEndNodes());
        jurisdiction->withWriteLock([&] {
            (*jurisdiction)[nodeUUID] = jurisdictionMap;
        });
    });

    if (!_physicsEnabled) {
        if (nearbyEntitiesAreReadyForPhysics()) {
            // These stats packets are sent in between full sends of a scene.
            // We keep physics disabled until we've recieved a full scene and everything near the avatar in that
            // scene is ready to compute its collision shape.
            _physicsEnabled = true;
            getMyAvatar()->updateMotionBehaviorFromMenu();
        } else {
            auto characterController = getMyAvatar()->getCharacterController();
            if (characterController) {
                // if we have a character controller, disable it here so the avatar doesn't get stuck due to
                // a non-loading collision hull.
                characterController->setEnabled(false);
            }
        }
    }

    return statsMessageLength;
}

void Application::packetSent(quint64 length) {
}

void Application::registerScriptEngineWithApplicationServices(ScriptEngine* scriptEngine) {
    // setup the packet senders and jurisdiction listeners of the script engine's scripting interfaces so
    // we can use the same ones from the application.
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    entityScriptingInterface->setPacketSender(&_entityEditSender);
    entityScriptingInterface->setEntityTree(getEntities()->getTree());

    // AvatarManager has some custom types
    AvatarManager::registerMetaTypes(scriptEngine);

    // hook our avatar and avatar hash map object into this script engine
    scriptEngine->registerGlobalObject("MyAvatar", getMyAvatar());
    qScriptRegisterMetaType(scriptEngine, audioListenModeToScriptValue, audioListenModeFromScriptValue);

    scriptEngine->registerGlobalObject("AvatarList", DependencyManager::get<AvatarManager>().data());

    scriptEngine->registerGlobalObject("Camera", &_myCamera);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    scriptEngine->registerGlobalObject("SpeechRecognizer", DependencyManager::get<SpeechRecognizer>().data());
#endif

    ClipboardScriptingInterface* clipboardScriptable = new ClipboardScriptingInterface();
    scriptEngine->registerGlobalObject("Clipboard", clipboardScriptable);
    connect(scriptEngine, &ScriptEngine::finished, clipboardScriptable, &ClipboardScriptingInterface::deleteLater);

    scriptEngine->registerGlobalObject("Overlays", &_overlays);
    qScriptRegisterMetaType(scriptEngine, OverlayPropertyResultToScriptValue, OverlayPropertyResultFromScriptValue);
    qScriptRegisterMetaType(scriptEngine, RayToOverlayIntersectionResultToScriptValue,
                            RayToOverlayIntersectionResultFromScriptValue);

    scriptEngine->registerGlobalObject("Desktop", DependencyManager::get<DesktopScriptingInterface>().data());

    scriptEngine->registerGlobalObject("Window", DependencyManager::get<WindowScriptingInterface>().data());
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                        LocationScriptingInterface::locationSetter, "Window");
    // register `location` on the global object.
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                                       LocationScriptingInterface::locationSetter);

    scriptEngine->registerFunction("WebWindow", WebWindowClass::constructor, 1);
    scriptEngine->registerFunction("OverlayWebWindow", QmlWebWindowClass::constructor);
    scriptEngine->registerFunction("OverlayWindow", QmlWindowClass::constructor);

    scriptEngine->registerGlobalObject("Menu", MenuScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("Stats", Stats::getInstance());
    scriptEngine->registerGlobalObject("Settings", SettingsScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("AudioDevice", AudioDeviceScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("AnimationCache", DependencyManager::get<AnimationCache>().data());
    scriptEngine->registerGlobalObject("SoundCache", DependencyManager::get<SoundCache>().data());
    scriptEngine->registerGlobalObject("Account", AccountScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("DialogsManager", _dialogsManagerScriptingInterface);

    scriptEngine->registerGlobalObject("GlobalServices", GlobalServicesScriptingInterface::getInstance());
    qScriptRegisterMetaType(scriptEngine, DownloadInfoResultToScriptValue, DownloadInfoResultFromScriptValue);

    scriptEngine->registerGlobalObject("FaceTracker", DependencyManager::get<DdeFaceTracker>().data());

    scriptEngine->registerGlobalObject("AvatarManager", DependencyManager::get<AvatarManager>().data());

    scriptEngine->registerGlobalObject("UndoStack", &_undoStackScriptingInterface);

    scriptEngine->registerGlobalObject("LODManager", DependencyManager::get<LODManager>().data());

    scriptEngine->registerGlobalObject("Paths", DependencyManager::get<PathUtils>().data());

    scriptEngine->registerGlobalObject("HMD", DependencyManager::get<HMDScriptingInterface>().data());
    scriptEngine->registerFunction("HMD", "getHUDLookAtPosition2D", HMDScriptingInterface::getHUDLookAtPosition2D, 0);
    scriptEngine->registerFunction("HMD", "getHUDLookAtPosition3D", HMDScriptingInterface::getHUDLookAtPosition3D, 0);

    scriptEngine->registerGlobalObject("Scene", DependencyManager::get<SceneScriptingInterface>().data());
    scriptEngine->registerGlobalObject("Render", _renderEngine->getConfiguration().get());

    scriptEngine->registerGlobalObject("ScriptDiscoveryService", DependencyManager::get<ScriptEngines>().data());
    scriptEngine->registerGlobalObject("Reticle", getApplicationCompositor().getReticleInterface());
}

bool Application::canAcceptURL(const QString& urlString) const {
    QUrl url(urlString);
    if (urlString.startsWith(HIFI_URL_SCHEME)) {
        return true;
    }
    QHashIterator<QString, AcceptURLMethod> i(_acceptedExtensions);
    QString lowerPath = url.path().toLower();
    while (i.hasNext()) {
        i.next();
        if (lowerPath.endsWith(i.key(), Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool Application::acceptURL(const QString& urlString, bool defaultUpload) {
    if (urlString.startsWith(HIFI_URL_SCHEME)) {
        // this is a hifi URL - have the AddressManager handle it
        QMetaObject::invokeMethod(DependencyManager::get<AddressManager>().data(), "handleLookupString",
                                  Qt::AutoConnection, Q_ARG(const QString&, urlString));
        return true;
    }

    QUrl url(urlString);
    QHashIterator<QString, AcceptURLMethod> i(_acceptedExtensions);
    QString lowerPath = url.path().toLower();
    while (i.hasNext()) {
        i.next();
        if (lowerPath.endsWith(i.key(), Qt::CaseInsensitive)) {
            AcceptURLMethod method = i.value();
            return (this->*method)(urlString);
        }
    }

    if (defaultUpload) {
        toggleAssetServerWidget(urlString);
    }
    return defaultUpload;
}

void Application::setSessionUUID(const QUuid& sessionUUID) {
    // HACK: until we swap the library dependency order between physics and entities
    // we cache the sessionID in two distinct places for physics.
    Physics::setSessionUUID(sessionUUID); // TODO: remove this one
    _physicsEngine->setSessionUUID(sessionUUID);
}

bool Application::askToSetAvatarUrl(const QString& url) {
    QUrl realUrl(url);
    if (realUrl.isLocalFile()) {
        OffscreenUi::warning("", "You can not use local files for avatar components.");
        return false;
    }

    // Download the FST file, to attempt to determine its model type
    QVariantHash fstMapping = FSTReader::downloadMapping(url);

    FSTReader::ModelType modelType = FSTReader::predictModelType(fstMapping);

    QString modelName = fstMapping["name"].toString();
    bool ok = false;
    switch (modelType) {

        case FSTReader::HEAD_AND_BODY_MODEL:
             ok = QMessageBox::Ok == OffscreenUi::question("Set Avatar",
                               "Would you like to use '" + modelName + "' for your avatar?",
                               QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
        break;

        default:
            OffscreenUi::warning("", modelName + "Does not support a head and body as required.");
        break;
    }

    if (ok) {
        getMyAvatar()->useFullAvatarURL(url, modelName);
        emit fullAvatarURLChanged(url, modelName);
    } else {
        qCDebug(interfaceapp) << "Declined to use the avatar: " << url;
    }

    return true;
}


bool Application::askToLoadScript(const QString& scriptFilenameOrURL) {
    QMessageBox::StandardButton reply;
    QString message = "Would you like to run this script:\n" + scriptFilenameOrURL;
    reply = OffscreenUi::question(getWindow(), "Run Script", message, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        qCDebug(interfaceapp) << "Chose to run the script: " << scriptFilenameOrURL;
        DependencyManager::get<ScriptEngines>()->loadScript(scriptFilenameOrURL);
    } else {
        qCDebug(interfaceapp) << "Declined to run the script: " << scriptFilenameOrURL;
    }
    return true;
}

bool Application::askToWearAvatarAttachmentUrl(const QString& url) {

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest networkRequest = QNetworkRequest(url);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    QNetworkReply* reply = networkAccessManager.get(networkRequest);
    int requestNumber = ++_avatarAttachmentRequest;
    connect(reply, &QNetworkReply::finished, [this, reply, url, requestNumber]() {

        if (requestNumber != _avatarAttachmentRequest) {
            // this request has been superseded by another more recent request
            reply->deleteLater();
            return;
        }

        QNetworkReply::NetworkError networkError = reply->error();
        if (networkError == QNetworkReply::NoError) {
            // download success
            QByteArray contents = reply->readAll();

            QJsonParseError jsonError;
            auto doc = QJsonDocument::fromJson(contents, &jsonError);
            if (jsonError.error == QJsonParseError::NoError) {

                auto jsonObject = doc.object();

                // retrieve optional name field from JSON
                QString name = tr("Unnamed Attachment");
                auto nameValue = jsonObject.value("name");
                if (nameValue.isString()) {
                    name = nameValue.toString();
                }

                // display confirmation dialog
                if (displayAvatarAttachmentConfirmationDialog(name)) {

                    // add attachment to avatar
                    auto myAvatar = getMyAvatar();
                    assert(myAvatar);
                    auto attachmentDataVec = myAvatar->getAttachmentData();
                    AttachmentData attachmentData;
                    attachmentData.fromJson(jsonObject);
                    attachmentDataVec.push_back(attachmentData);
                    myAvatar->setAttachmentData(attachmentDataVec);

                } else {
                    qCDebug(interfaceapp) << "User declined to wear the avatar attachment: " << url;
                }

            } else {
                // json parse error
                auto avatarAttachmentParseErrorString = tr("Error parsing attachment JSON from url: \"%1\"");
                displayAvatarAttachmentWarning(avatarAttachmentParseErrorString.arg(url));
            }
        } else {
            // download failure
            auto avatarAttachmentDownloadErrorString = tr("Error downloading attachment JSON from url: \"%1\"");
            displayAvatarAttachmentWarning(avatarAttachmentDownloadErrorString.arg(url));
        }
        reply->deleteLater();
    });
    return true;
}

void Application::displayAvatarAttachmentWarning(const QString& message) const {
    auto avatarAttachmentWarningTitle = tr("Avatar Attachment Failure");
    OffscreenUi::warning(avatarAttachmentWarningTitle, message);
}

bool Application::displayAvatarAttachmentConfirmationDialog(const QString& name) const {
    auto avatarAttachmentConfirmationTitle = tr("Avatar Attachment Confirmation");
    auto avatarAttachmentConfirmationMessage = tr("Would you like to wear '%1' on your avatar?").arg(name);
    auto reply = OffscreenUi::question(avatarAttachmentConfirmationTitle,
                                       avatarAttachmentConfirmationMessage,
                                       QMessageBox::Ok | QMessageBox::Cancel);
    if (QMessageBox::Ok == reply) {
        return true;
    } else {
        return false;
    }
}

void Application::toggleRunningScriptsWidget() {
    static const QUrl url("hifi/dialogs/RunningScripts.qml");
    DependencyManager::get<OffscreenUi>()->show(url, "RunningScripts");
    //if (_runningScriptsWidget->isVisible()) {
    //    if (_runningScriptsWidget->hasFocus()) {
    //        _runningScriptsWidget->hide();
    //    } else {
    //        _runningScriptsWidget->raise();
    //        setActiveWindow(_runningScriptsWidget);
    //        _runningScriptsWidget->setFocus();
    //    }
    //} else {
    //    _runningScriptsWidget->show();
    //    _runningScriptsWidget->setFocus();
    //}
}

void Application::toggleAssetServerWidget(QString filePath) {
    if (!DependencyManager::get<NodeList>()->getThisNodeCanRez()) {
        return;
    }

    static const QUrl url { "AssetServer.qml" };

    auto startUpload = [=](QQmlContext* context, QObject* newObject){
        if (!filePath.isEmpty()) {
            emit uploadRequest(filePath);
        }
    };
    DependencyManager::get<OffscreenUi>()->show(url, "AssetServer", startUpload);
    startUpload(nullptr, nullptr);
}

void Application::packageModel() {
    ModelPackager::package();
}

void Application::openUrl(const QUrl& url) {
    if (!url.isEmpty()) {
        if (url.scheme() == HIFI_URL_SCHEME) {
            DependencyManager::get<AddressManager>()->handleLookupString(url.toString());
        } else {
            // address manager did not handle - ask QDesktopServices to handle
            QDesktopServices::openUrl(url);
        }
    }
}

void Application::loadDialog() {
    auto scriptEngines = DependencyManager::get<ScriptEngines>();
    QString fileNameString = OffscreenUi::getOpenFileName(
        _glWidget, tr("Open Script"), getPreviousScriptLocation(), tr("JavaScript Files (*.js)"));
    if (!fileNameString.isEmpty() && QFile(fileNameString).exists()) {
        setPreviousScriptLocation(QFileInfo(fileNameString).absolutePath());
        DependencyManager::get<ScriptEngines>()->loadScript(fileNameString, true, false, false, true);  // Don't load from cache
    }
}

QString Application::getPreviousScriptLocation() {
    QString result = _previousScriptLocation.get();
    return result;
}

void Application::setPreviousScriptLocation(const QString& location) {
    _previousScriptLocation.set(location);
}

void Application::loadScriptURLDialog() {
    auto newScript = OffscreenUi::getText(nullptr, "Open and Run Script", "Script URL");
    if (!newScript.isEmpty()) {
        DependencyManager::get<ScriptEngines>()->loadScript(newScript);
    }
}

void Application::toggleLogDialog() {
    if (! _logDialog) {
        _logDialog = new LogDialog(_glWidget, getLogger());
    }

    if (_logDialog->isVisible()) {
        _logDialog->hide();
    } else {
        _logDialog->show();
    }
}

void Application::takeSnapshot() {
    QMediaPlayer* player = new QMediaPlayer();
    QFileInfo inf = QFileInfo(PathUtils::resourcesPath() + "sounds/snap.wav");
    player->setMedia(QUrl::fromLocalFile(inf.absoluteFilePath()));
    player->play();

    QString fileName = Snapshot::saveSnapshot(getActiveDisplayPlugin()->getScreenshot());

    AccountManager& accountManager = AccountManager::getInstance();
    if (!accountManager.isLoggedIn()) {
        return;
    }

    DependencyManager::get<OffscreenUi>()->load("hifi/dialogs/SnapshotShareDialog.qml", [=](QQmlContext*, QObject* dialog) {
        dialog->setProperty("source", QUrl::fromLocalFile(fileName));
        connect(dialog, SIGNAL(uploadSnapshot(const QString& snapshot)), this, SLOT(uploadSnapshot(const QString& snapshot)));
    });
}

float Application::getRenderResolutionScale() const {
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionOne)) {
        return 1.0f;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionTwoThird)) {
        return 0.666f;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionHalf)) {
        return 0.5f;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionThird)) {
        return 0.333f;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionQuarter)) {
        return 0.25f;
    } else {
        return 1.0f;
    }
}

void Application::notifyPacketVersionMismatch() {
    if (!_notifiedPacketVersionMismatchThisDomain) {
        _notifiedPacketVersionMismatchThisDomain = true;

        QString message = "The location you are visiting is running an incompatible server version.\n";
        message += "Content may not display properly.";

        OffscreenUi::warning("", message);
    }
}

void Application::checkSkeleton() {
    if (getMyAvatar()->getSkeletonModel()->isActive() && !getMyAvatar()->getSkeletonModel()->hasSkeleton()) {
        qCDebug(interfaceapp) << "MyAvatar model has no skeleton";

        QString message = "Your selected avatar body has no skeleton.\n\nThe default body will be loaded...";
        OffscreenUi::warning("", message);

        getMyAvatar()->useFullAvatarURL(AvatarData::defaultFullAvatarModelUrl(), DEFAULT_FULL_AVATAR_MODEL_NAME);
    } else {
        _physicsEngine->setCharacterController(getMyAvatar()->getCharacterController());
    }
}

void Application::activeChanged(Qt::ApplicationState state) {
    switch (state) {
        case Qt::ApplicationActive:
            _isForeground = true;
            break;

        case Qt::ApplicationSuspended:
        case Qt::ApplicationHidden:
        case Qt::ApplicationInactive:
        default:
            _isForeground = false;
            break;
    }
}

void Application::postLambdaEvent(std::function<void()> f) {
    if (this->thread() == QThread::currentThread()) {
        f();
    } else {
        QCoreApplication::postEvent(this, new LambdaEvent(f));
    }
}

void Application::initPlugins() {
}

void Application::shutdownPlugins() {
}

glm::uvec2 Application::getCanvasSize() const {
    return glm::uvec2(_glWidget->width(), _glWidget->height());
}

QRect Application::getRenderingGeometry() const {
    auto geometry = _glWidget->geometry(); 
    auto topLeft = geometry.topLeft();
    auto topLeftScreen = _glWidget->mapToGlobal(topLeft);
    geometry.moveTopLeft(topLeftScreen);
    return geometry;
}

glm::uvec2 Application::getUiSize() const {
    return getActiveDisplayPlugin()->getRecommendedUiSize();
}

QSize Application::getDeviceSize() const {
    return fromGlm(getActiveDisplayPlugin()->getRecommendedRenderSize());
}

bool Application::isThrottleRendering() const {
    return getActiveDisplayPlugin()->isThrottled();
}

bool Application::hasFocus() const {
    return getActiveDisplayPlugin()->hasFocus();
}

glm::vec2 Application::getViewportDimensions() const {
    return toGlm(getDeviceSize());
}

void Application::setMaxOctreePacketsPerSecond(int maxOctreePPS) {
    if (maxOctreePPS != _maxOctreePPS) {
        _maxOctreePPS = maxOctreePPS;
        maxOctreePacketsPerSecond.set(_maxOctreePPS);
    }
}

int Application::getMaxOctreePacketsPerSecond() {
    return _maxOctreePPS;
}

qreal Application::getDevicePixelRatio() {
    return (_window && _window->windowHandle()) ? _window->windowHandle()->devicePixelRatio() : 1.0;
}

DisplayPlugin* Application::getActiveDisplayPlugin() {
    DisplayPlugin* result = nullptr;
    if (QThread::currentThread() == thread()) {
        if (nullptr == _displayPlugin) {
            updateDisplayMode();
            Q_ASSERT(_displayPlugin);
        }
        result = _displayPlugin.get();
    } else {
        std::unique_lock<std::mutex> lock(_displayPluginLock);
        result = _displayPlugin.get();
    }
    return result;
}

const DisplayPlugin* Application::getActiveDisplayPlugin() const {
    return ((Application*)this)->getActiveDisplayPlugin();
}

static void addDisplayPluginToMenu(DisplayPluginPointer displayPlugin, bool active = false) {
    auto menu = Menu::getInstance();
    QString name = displayPlugin->getName();
    auto grouping = displayPlugin->getGrouping();
    QString groupingMenu { "" };
    Q_ASSERT(!menu->menuItemExists(MenuOption::OutputMenu, name));

    // assign the meny grouping based on plugin grouping
    switch (grouping) {
        case Plugin::ADVANCED:
            groupingMenu = "Advanced";
            break;
        case Plugin::DEVELOPER:
            groupingMenu = "Developer";
            break;
        default:
            groupingMenu = "Standard";
            break;
    }

    static QActionGroup* displayPluginGroup = nullptr;
    if (!displayPluginGroup) {
        displayPluginGroup = new QActionGroup(menu);
        displayPluginGroup->setExclusive(true);
    }
    auto parent = menu->getMenu(MenuOption::OutputMenu);
    auto action = menu->addActionToQMenuAndActionHash(parent,
        name, 0, qApp,
        SLOT(updateDisplayMode()),
        QAction::NoRole, Menu::UNSPECIFIED_POSITION, groupingMenu);

    action->setCheckable(true);
    action->setChecked(active);
    displayPluginGroup->addAction(action);
    Q_ASSERT(menu->menuItemExists(MenuOption::OutputMenu, name));
}

void Application::updateDisplayMode() {
    // Unsafe to call this method from anything but the main thread
    if (QThread::currentThread() != thread()) {
        qFatal("Attempted to switch display plugins from a non-main thread");
    }

    // Some plugins *cough* Oculus *cough* process message events from inside their
    // display function, and we don't want to change the display plugin underneath
    // the paintGL call, so we need to guard against that
    // The current oculus runtime doesn't do this anymore
    if (_inPaint) {
        qFatal("Attempted to switch display plugins while in painting");
    }

    auto menu = Menu::getInstance();
    auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();

    static std::once_flag once;
    std::call_once(once, [&] {
        bool first = true;

        // first sort the plugins into groupings: standard, advanced, developer
        DisplayPluginList standard;
        DisplayPluginList advanced;
        DisplayPluginList developer;
        foreach(auto displayPlugin, displayPlugins) {
            auto grouping = displayPlugin->getGrouping();
            switch (grouping) {
                case Plugin::ADVANCED:
                    advanced.push_back(displayPlugin);
                    break;
                case Plugin::DEVELOPER:
                    developer.push_back(displayPlugin);
                    break;
                default:
                    standard.push_back(displayPlugin);
                    break;
            }
        }

        // concactonate the groupings into a single list in the order: standard, advanced, developer
        standard.insert(std::end(standard), std::begin(advanced), std::end(advanced));
        standard.insert(std::end(standard), std::begin(developer), std::end(developer));

        foreach(auto displayPlugin, standard) {
            addDisplayPluginToMenu(displayPlugin, first);
            QObject::connect(displayPlugin.get(), &DisplayPlugin::recommendedFramebufferSizeChanged, [this](const QSize & size) {
                resizeGL();
            });
            first = false;
        }

        // after all plugins have been added to the menu, add a seperator to the menu
        auto menu = Menu::getInstance();
        auto parent = menu->getMenu(MenuOption::OutputMenu);
        parent->addSeparator();
    });


    // Default to the first item on the list, in case none of the menu items match
    DisplayPluginPointer newDisplayPlugin = displayPlugins.at(0);
    foreach(DisplayPluginPointer displayPlugin, PluginManager::getInstance()->getDisplayPlugins()) {
        QString name = displayPlugin->getName();
        QAction* action = menu->getActionForOption(name);
        if (action->isChecked()) {
            newDisplayPlugin = displayPlugin;
            break;
        }
    }

    if (newDisplayPlugin == _displayPlugin) {
        return;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    // Make the switch atomic from the perspective of other threads
    {
        std::unique_lock<std::mutex> lock(_displayPluginLock);

        if (_displayPlugin) {
            _displayPlugin->deactivate();
        }

        // FIXME probably excessive and useless context switching
        _offscreenContext->makeCurrent();
        newDisplayPlugin->activate();
        _offscreenContext->makeCurrent();
        offscreenUi->resize(fromGlm(newDisplayPlugin->getRecommendedUiSize()));
        _offscreenContext->makeCurrent();
        getApplicationCompositor().setDisplayPlugin(newDisplayPlugin);
        _displayPlugin = newDisplayPlugin;
    }


    emit activeDisplayPluginChanged();

    // reset the avatar, to set head and hand palms back to a resonable default pose.
    getMyAvatar()->reset(false);

    Q_ASSERT_X(_displayPlugin, "Application::updateDisplayMode", "could not find an activated display plugin");
}

static void addInputPluginToMenu(InputPluginPointer inputPlugin, bool active = false) {
    auto menu = Menu::getInstance();
    QString name = inputPlugin->getName();
    Q_ASSERT(!menu->menuItemExists(MenuOption::InputMenu, name));

    static QActionGroup* inputPluginGroup = nullptr;
    if (!inputPluginGroup) {
        inputPluginGroup = new QActionGroup(menu);
    }
    auto parent = menu->getMenu(MenuOption::InputMenu);
    auto action = menu->addCheckableActionToQMenuAndActionHash(parent,
        name, 0, active, qApp,
        SLOT(updateInputModes()));
    inputPluginGroup->addAction(action);
    inputPluginGroup->setExclusive(false);
    Q_ASSERT(menu->menuItemExists(MenuOption::InputMenu, name));
}


void Application::updateInputModes() {
    auto menu = Menu::getInstance();
    auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
    static std::once_flag once;
    std::call_once(once, [&] {
        bool first = true;
        foreach(auto inputPlugin, inputPlugins) {
            addInputPluginToMenu(inputPlugin, first);
            first = false;
        }
    });
    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    InputPluginList newInputPlugins;
    InputPluginList removedInputPlugins;
    foreach(auto inputPlugin, inputPlugins) {
        QString name = inputPlugin->getName();
        QAction* action = menu->getActionForOption(name);

        auto it = std::find(std::begin(_activeInputPlugins), std::end(_activeInputPlugins), inputPlugin);
        if (action->isChecked() && it == std::end(_activeInputPlugins)) {
            _activeInputPlugins.push_back(inputPlugin);
            newInputPlugins.push_back(inputPlugin);
        } else if (!action->isChecked() && it != std::end(_activeInputPlugins)) {
            _activeInputPlugins.erase(it);
            removedInputPlugins.push_back(inputPlugin);
        }
    }

    // A plugin was checked
    if (newInputPlugins.size() > 0) {
        foreach(auto newInputPlugin, newInputPlugins) {
            newInputPlugin->activate();
            //newInputPlugin->installEventFilter(qApp);
            //newInputPlugin->installEventFilter(offscreenUi.data());
        }
    }
    if (removedInputPlugins.size() > 0) { // A plugin was unchecked
        foreach(auto removedInputPlugin, removedInputPlugins) {
            removedInputPlugin->deactivate();
            //removedInputPlugin->removeEventFilter(qApp);
            //removedInputPlugin->removeEventFilter(offscreenUi.data());
        }
    }

    //if (newInputPlugins.size() > 0 || removedInputPlugins.size() > 0) {
    //    if (!_currentInputPluginActions.isEmpty()) {
    //        auto menu = Menu::getInstance();
    //        foreach(auto itemInfo, _currentInputPluginActions) {
    //            menu->removeMenuItem(itemInfo.first, itemInfo.second);
    //        }
    //        _currentInputPluginActions.clear();
    //    }
    //}
}

mat4 Application::getEyeProjection(int eye) const {
    if (isHMDMode()) {
        return getActiveDisplayPlugin()->getEyeProjection((Eye)eye, _viewFrustum.getProjection());
    }

    return _viewFrustum.getProjection();
}

mat4 Application::getEyeOffset(int eye) const {
    // FIXME invert?
    return getActiveDisplayPlugin()->getEyeToHeadTransform((Eye)eye);
}

mat4 Application::getHMDSensorPose() const {
    if (isHMDMode()) {
        return getActiveDisplayPlugin()->getHeadPose();
    }
    return mat4();
}

void Application::crashApplication() {
    qCDebug(interfaceapp) << "Intentionally crashed Interface";
    QObject* object = nullptr;
    bool value = object->isWindowType();
    Q_UNUSED(value);
}

void Application::deadlockApplication() {
    qCDebug(interfaceapp) << "Intentionally deadlocked Interface";
    // Using a loop that will *technically* eventually exit (in ~600 billion years)
    // to avoid compiler warnings about a loop that will never exit
    for (uint64_t i = 1; i != 0; ++i) {
        QThread::sleep(1);
    }
}

void Application::setActiveDisplayPlugin(const QString& pluginName) {
    auto menu = Menu::getInstance();
    foreach(DisplayPluginPointer displayPlugin, PluginManager::getInstance()->getDisplayPlugins()) {
        QString name = displayPlugin->getName();
        QAction* action = menu->getActionForOption(name);
        if (pluginName == name) {
            action->setChecked(true);
        }
    }
    updateDisplayMode();
}

void Application::handleLocalServerConnection() {
    auto server = qobject_cast<QLocalServer*>(sender());

    qDebug() << "Got connection on local server from additional instance - waiting for parameters";

    auto socket = server->nextPendingConnection();

    connect(socket, &QLocalSocket::readyRead, this, &Application::readArgumentsFromLocalSocket);

    qApp->getWindow()->raise();
    qApp->getWindow()->activateWindow();
}

void Application::readArgumentsFromLocalSocket() {
    auto socket = qobject_cast<QLocalSocket*>(sender());

    auto message = socket->readAll();
    socket->deleteLater();

    qDebug() << "Read from connection: " << message;

    // If we received a message, try to open it as a URL
    if (message.length() > 0) {
        qApp->openUrl(QString::fromUtf8(message));
    }
}

void Application::showDesktop() {
    if (!_overlayConductor.getEnabled()) {
        _overlayConductor.setEnabled(true);
    }
}

CompositorHelper& Application::getApplicationCompositor() const {
    return *DependencyManager::get<CompositorHelper>();
}
