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

#include <chrono>
#include <thread>

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <gl/Config.h>

#include <QtCore/QResource>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QCommandLineParser>
#include <QtCore/QMimeData>
#include <QtCore/QThreadPool>
#include <QtCore/QFileSelector>
#include <QtConcurrent/QtConcurrentRun>

#include <QtGui/QClipboard>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtGui/QDesktopServices>

#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QLocalServer>

#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickWindow>

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMessageBox>

#include <QtMultimedia/QMediaPlayer>

#include <QFontDatabase>
#include <QProcessEnvironment>
#include <QTemporaryDir>

#include <gl/QOpenGLContextWrapper.h>

#include <shared/FileUtils.h>
#include <shared/QtHelpers.h>
#include <shared/GlobalAppProperties.h>
#include <StatTracker.h>
#include <Trace.h>
#include <ResourceScriptingInterface.h>
#include <AccountManager.h>
#include <AddressManager.h>
#include <AnimDebugDraw.h>
#include <BuildInfo.h>
#include <AnimationCacheScriptingInterface.h>
#include <AssetClient.h>
#include <AssetUpload.h>
#include <AutoUpdater.h>
#include <Midi.h>
#include <AudioInjectorManager.h>
#include <AvatarBookmarks.h>
#include <CursorManager.h>
#include <VirtualPadManager.h>
#include <DebugDraw.h>
#include <DeferredLightingEffect.h>
#include <EntityScriptClient.h>
#include <EntityScriptServerLogClient.h>
#include <EntityScriptingInterface.h>
#include "ui/overlays/ContextOverlayInterface.h"
#include <ErrorDialog.h>
#include <FileScriptingInterface.h>
#include <Finally.h>
#include <FingerprintUtils.h>
#include <FramebufferCache.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <gpu/gl/GLBackend.h>
#include <InfoView.h>
#include <input-plugins/InputPlugin.h>
#include <controllers/UserInputMapper.h>
#include <controllers/InputRecorder.h>
#include <controllers/ScriptingInterface.h>
#include <controllers/StateController.h>
#include <UserActivityLoggerScriptingInterface.h>
#include <LogHandler.h>
#include "LocationBookmarks.h"
#include <LocationScriptingInterface.h>
#include <MainWindow.h>
#include <MappingRequest.h>
#include <MessagesClient.h>
#include <model-networking/ModelCacheScriptingInterface.h>
#include <model-networking/TextureCacheScriptingInterface.h>
#include <ModelEntityItem.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <ObjectMotionState.h>
#include <OctalCode.h>
#include <OctreeSceneStats.h>
#include <OffscreenUi.h>
#include <gl/OffscreenGLCanvas.h>
#include <ui/OffscreenQmlSurfaceCache.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <PhysicsEngine.h>
#include <PhysicsHelpers.h>
#include <plugins/CodecPlugin.h>
#include <plugins/PluginManager.h>
#include <plugins/PluginUtils.h>
#include <plugins/SteamClientPlugin.h>
#include <plugins/InputConfiguration.h>
#include <RecordingScriptingInterface.h>
#include <UpdateSceneTask.h>
#include <RenderViewTask.h>
#include <SecondaryCamera.h>
#include <ResourceCache.h>
#include <ResourceRequest.h>
#include <SandboxUtils.h>
#include <SceneScriptingInterface.h>
#include <ScriptEngines.h>
#include <ScriptCache.h>
#include <ShapeEntityItem.h>
#include <SoundCacheScriptingInterface.h>
#include <ui/TabletScriptingInterface.h>
#include <ui/ToolbarScriptingInterface.h>
#include <InteractiveWindow.h>
#include <Tooltip.h>
#include <udt/PacketHeaders.h>
#include <UserActivityLogger.h>
#include <UsersScriptingInterface.h>
#include <recording/ClipCache.h>
#include <recording/Deck.h>
#include <recording/Recorder.h>
#include <shared/StringHelpers.h>
#include <QmlWebWindowClass.h>
#include <QmlFragmentClass.h>
#include <Preferences.h>
#include <display-plugins/CompositorHelper.h>
#include <display-plugins/hmd/HmdDisplayPlugin.h>
#include <trackers/EyeTracker.h>
#include <avatars-renderer/ScriptAvatar.h>
#include <RenderableEntityItem.h>
#include <procedural/ProceduralSkybox.h>

#include "AudioClient.h"
#include "audio/AudioScope.h"
#include "avatar/AvatarManager.h"
#include "avatar/MyHead.h"
#include "CrashRecoveryHandler.h"
#include "CrashHandler.h"
#include "devices/DdeFaceTracker.h"
#include "DiscoverabilityManager.h"
#include "GLCanvas.h"
#include "InterfaceDynamicFactory.h"
#include "InterfaceLogging.h"
#include "LODManager.h"
#include "ModelPackager.h"
#include "scripting/Audio.h"
#include "networking/CloseEventSender.h"
#include "scripting/TestScriptingInterface.h"
#include "scripting/AssetMappingsScriptingInterface.h"
#include "scripting/ClipboardScriptingInterface.h"
#include "scripting/DesktopScriptingInterface.h"
#include "scripting/AccountServicesScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "scripting/MenuScriptingInterface.h"
#include "graphics-scripting/GraphicsScriptingInterface.h"
#include "scripting/SettingsScriptingInterface.h"
#include "scripting/WindowScriptingInterface.h"
#include "scripting/ControllerScriptingInterface.h"
#include "scripting/RatesScriptingInterface.h"
#include "scripting/SelectionScriptingInterface.h"
#include "scripting/WalletScriptingInterface.h"
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif
#include "ui/ResourceImageItem.h"
#include "ui/AddressBarDialog.h"
#include "ui/AvatarInputs.h"
#include "ui/DialogsManager.h"
#include "ui/LoginDialog.h"
#include "ui/overlays/Cube3DOverlay.h"
#include "ui/overlays/Web3DOverlay.h"
#include "ui/Snapshot.h"
#include "ui/SnapshotAnimated.h"
#include "ui/StandAloneJSConsole.h"
#include "ui/Stats.h"
#include "ui/AnimStats.h"
#include "ui/UpdateDialog.h"
#include "ui/overlays/Overlays.h"
#include "ui/DomainConnectionModel.h"
#include "Util.h"
#include "InterfaceParentFinder.h"
#include "ui/OctreeStatsProvider.h"

#include <GPUIdent.h>
#include <gl/GLHelpers.h>
#include <src/scripting/GooglePolyScriptingInterface.h>
#include <EntityScriptClient.h>
#include <ModelScriptingInterface.h>

#include <PickManager.h>
#include <PointerManager.h>
#include <raypick/RayPickScriptingInterface.h>
#include <raypick/LaserPointerScriptingInterface.h>
#include <raypick/PickScriptingInterface.h>
#include <raypick/PointerScriptingInterface.h>
#include <raypick/MouseRayPick.h>

#include <FadeEffect.h>

#include "commerce/Ledger.h"
#include "commerce/Wallet.h"
#include "commerce/QmlCommerce.h"

#include "webbrowser/WebBrowserSuggestionsEngine.h"
#include <DesktopPreviewProvider.h>

#include "AboutUtil.h"

#if defined(Q_OS_WIN)
#include <VersionHelpers.h>

#ifdef DEBUG_EVENT_QUEUE
// This is a HACK that uses private headers included with the qt source distrubution.
// To use this feature you need to add these directores to your include path:
// E:/Qt/5.10.1/Src/qtbase/include/QtCore/5.10.1/QtCore
// E:/Qt/5.10.1/Src/qtbase/include/QtCore/5.10.1
#define QT_BOOTSTRAPPED
#include <private/qthread_p.h>
#include <private/qobject_p.h>
#undef QT_BOOTSTRAPPED
#endif

// On Windows PC, NVidia Optimus laptop, we want to enable NVIDIA GPU
// FIXME seems to be broken.
extern "C" {
 _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

#if defined(Q_OS_ANDROID)
#include <android/log.h>
#include "AndroidHelper.h"
#endif

enum ApplicationEvent {
    // Execute a lambda function
    Lambda = QEvent::User + 1,
    // Trigger the next render
    Render,
    // Trigger the next idle
    Idle,
};

class RenderEventHandler : public QObject {
    using Parent = QObject;
    Q_OBJECT
public:
    RenderEventHandler() {
        // Transfer to a new thread
        moveToNewNamedThread(this, "RenderThread", [this](QThread* renderThread) {
            hifi::qt::addBlockingForbiddenThread("Render", renderThread);
            qApp->_lastTimeRendered.start();
        }, std::bind(&RenderEventHandler::initialize, this), QThread::HighestPriority);
    }

private:
    void initialize() {
        setObjectName("Render");
        PROFILE_SET_THREAD_NAME("Render");
        setCrashAnnotation("render_thread_id", std::to_string((size_t)QThread::currentThreadId()));
    }

    void render() {
        if (qApp->shouldPaint()) {
            qApp->paintGL();
        }
    }

    bool event(QEvent* event) override {
        switch ((int)event->type()) {
            case ApplicationEvent::Render:
                render();
                qApp->_pendingRenderEvent.store(false);
                return true;

            default:
                break;
        }
        return Parent::event(event);
    }
};


Q_LOGGING_CATEGORY(trace_app_input_mouse, "trace.app.input.mouse")

using namespace std;

static QTimer locationUpdateTimer;
static QTimer identityPacketTimer;
static QTimer pingTimer;

#if defined(Q_OS_ANDROID)
static bool DISABLE_WATCHDOG = true;
#else
static const QString DISABLE_WATCHDOG_FLAG{ "HIFI_DISABLE_WATCHDOG" };
static bool DISABLE_WATCHDOG = nsightActive() || QProcessEnvironment::systemEnvironment().contains(DISABLE_WATCHDOG_FLAG);
#endif

#if defined(USE_GLES)
static bool DISABLE_DEFERRED = true;
#else
static const QString RENDER_FORWARD{ "HIFI_RENDER_FORWARD" };
static bool DISABLE_DEFERRED = QProcessEnvironment::systemEnvironment().contains(RENDER_FORWARD);
#endif

#if !defined(Q_OS_ANDROID)
static const int MAX_CONCURRENT_RESOURCE_DOWNLOADS = 16;
#else
static const int MAX_CONCURRENT_RESOURCE_DOWNLOADS = 4;
#endif

// For processing on QThreadPool, we target a number of threads after reserving some
// based on how many are being consumed by the application and the display plugin.  However,
// we will never drop below the 'min' value
static const int MIN_PROCESSING_THREAD_POOL_SIZE = 1;

static const QString SNAPSHOT_EXTENSION = ".jpg";
static const QString JPG_EXTENSION = ".jpg";
static const QString PNG_EXTENSION = ".png";
static const QString SVO_EXTENSION = ".svo";
static const QString SVO_JSON_EXTENSION = ".svo.json";
static const QString JSON_GZ_EXTENSION = ".json.gz";
static const QString JSON_EXTENSION = ".json";
static const QString JS_EXTENSION = ".js";
static const QString FST_EXTENSION = ".fst";
static const QString FBX_EXTENSION = ".fbx";
static const QString OBJ_EXTENSION = ".obj";
static const QString AVA_JSON_EXTENSION = ".ava.json";
static const QString WEB_VIEW_TAG = "noDownload=true";
static const QString ZIP_EXTENSION = ".zip";
static const QString CONTENT_ZIP_EXTENSION = ".content.zip";

static const float MIRROR_FULLSCREEN_DISTANCE = 0.789f;

static const quint64 TOO_LONG_SINCE_LAST_SEND_DOWNSTREAM_AUDIO_STATS = 1 * USECS_PER_SECOND;

static const QString INFO_EDIT_ENTITIES_PATH = "html/edit-commands.html";
static const QString INFO_HELP_PATH = "html/tabletHelp.html";

static const unsigned int THROTTLED_SIM_FRAMERATE = 15;
static const int THROTTLED_SIM_FRAME_PERIOD_MS = MSECS_PER_SECOND / THROTTLED_SIM_FRAMERATE;

static const uint32_t INVALID_FRAME = UINT32_MAX;

static const float PHYSICS_READY_RANGE = 3.0f; // how far from avatar to check for entities that aren't ready for simulation
static const float INITIAL_QUERY_RADIUS = 10.0f;  // priority radius for entities before physics enabled

static const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

Setting::Handle<int> maxOctreePacketsPerSecond("maxOctreePPS", DEFAULT_MAX_OCTREE_PPS);

static const QString MARKETPLACE_CDN_HOSTNAME = "mpassets.highfidelity.com";
static const int INTERVAL_TO_CHECK_HMD_WORN_STATUS = 500; // milliseconds
static const QString DESKTOP_DISPLAY_PLUGIN_NAME = "Desktop";
static const QString ACTIVE_DISPLAY_PLUGIN_SETTING_NAME = "activeDisplayPlugin";
static const QString SYSTEM_TABLET = "com.highfidelity.interface.tablet.system";
static const QString AUTO_LOGOUT_SETTING_NAME = "wallet/autoLogout";

const std::vector<std::pair<QString, Application::AcceptURLMethod>> Application::_acceptedExtensions {
    { SVO_EXTENSION, &Application::importSVOFromURL },
    { SVO_JSON_EXTENSION, &Application::importSVOFromURL },
    { AVA_JSON_EXTENSION, &Application::askToWearAvatarAttachmentUrl },
    { JSON_EXTENSION, &Application::importJSONFromURL },
    { JS_EXTENSION, &Application::askToLoadScript },
    { FST_EXTENSION, &Application::askToSetAvatarUrl },
    { JSON_GZ_EXTENSION, &Application::askToReplaceDomainContent },
    { CONTENT_ZIP_EXTENSION, &Application::askToReplaceDomainContent },
    { ZIP_EXTENSION, &Application::importFromZIP },
    { JPG_EXTENSION, &Application::importImage },
    { PNG_EXTENSION, &Application::importImage }
};

class DeadlockWatchdogThread : public QThread {
public:
    static const unsigned long HEARTBEAT_UPDATE_INTERVAL_SECS = 1;
    static const unsigned long MAX_HEARTBEAT_AGE_USECS = 120 * USECS_PER_SECOND; // 2 mins with no checkin probably a deadlock
    static const int WARNING_ELAPSED_HEARTBEAT = 500 * USECS_PER_MSEC; // warn if elapsed heartbeat average is large
    static const int HEARTBEAT_SAMPLES = 100000; // ~5 seconds worth of samples

    // Set the heartbeat on launch
    DeadlockWatchdogThread() {
        setObjectName("Deadlock Watchdog");
        // Give the heartbeat an initial value
        _heartbeat = usecTimestampNow();
        _paused = false;
        connect(qApp, &QCoreApplication::aboutToQuit, [this] {
            _quit = true;
        });
    }

    static void updateHeartbeat() {
        auto now = usecTimestampNow();
        auto elapsed = now - _heartbeat;
        _movingAverage.addSample(elapsed);
        _heartbeat = now;
    }

    static void deadlockDetectionCrash() {
        uint32_t* crashTrigger = nullptr;
        *crashTrigger = 0xDEAD10CC;
    }

    static void withPause(const std::function<void()>& lambda) {
        pause();
        lambda();
        resume();
    }
    static void pause() {
        _paused = true;
    }

    static void resume() {
        // Update the heartbeat BEFORE resuming the checks
        updateHeartbeat();
        _paused = false;
    }

    void run() override {
        while (!_quit) {
            QThread::sleep(HEARTBEAT_UPDATE_INTERVAL_SECS);
            // Don't do heartbeat detection under nsight
            if (_paused) {
                continue;
            }
            uint64_t lastHeartbeat = _heartbeat; // sample atomic _heartbeat, because we could context switch away and have it updated on us
            uint64_t now = usecTimestampNow();
            auto lastHeartbeatAge = (now > lastHeartbeat) ? now - lastHeartbeat : 0;
            auto elapsedMovingAverage = _movingAverage.getAverage();

            if (elapsedMovingAverage > _maxElapsedAverage) {
                qCDebug(interfaceapp_deadlock) << "DEADLOCK WATCHDOG WARNING:"
                    << "lastHeartbeatAge:" << lastHeartbeatAge
                    << "elapsedMovingAverage:" << elapsedMovingAverage
                    << "maxElapsed:" << _maxElapsed
                    << "PREVIOUS maxElapsedAverage:" << _maxElapsedAverage
                    << "NEW maxElapsedAverage:" << elapsedMovingAverage << "** NEW MAX ELAPSED AVERAGE **"
                    << "samples:" << _movingAverage.getSamples();
                _maxElapsedAverage = elapsedMovingAverage;
            }
            if (lastHeartbeatAge > _maxElapsed) {
                qCDebug(interfaceapp_deadlock) << "DEADLOCK WATCHDOG WARNING:"
                    << "lastHeartbeatAge:" << lastHeartbeatAge
                    << "elapsedMovingAverage:" << elapsedMovingAverage
                    << "PREVIOUS maxElapsed:" << _maxElapsed
                    << "NEW maxElapsed:" << lastHeartbeatAge << "** NEW MAX ELAPSED **"
                    << "maxElapsedAverage:" << _maxElapsedAverage
                    << "samples:" << _movingAverage.getSamples();
                _maxElapsed = lastHeartbeatAge;
            }
            if (elapsedMovingAverage > WARNING_ELAPSED_HEARTBEAT) {
                qCDebug(interfaceapp_deadlock) << "DEADLOCK WATCHDOG WARNING:"
                    << "lastHeartbeatAge:" << lastHeartbeatAge
                    << "elapsedMovingAverage:" << elapsedMovingAverage << "** OVER EXPECTED VALUE **"
                    << "maxElapsed:" << _maxElapsed
                    << "maxElapsedAverage:" << _maxElapsedAverage
                    << "samples:" << _movingAverage.getSamples();
            }

            if (lastHeartbeatAge > MAX_HEARTBEAT_AGE_USECS) {
                qCDebug(interfaceapp_deadlock) << "DEADLOCK DETECTED -- "
                         << "lastHeartbeatAge:" << lastHeartbeatAge
                         << "[ lastHeartbeat :" << lastHeartbeat
                         << "now:" << now << " ]"
                         << "elapsedMovingAverage:" << elapsedMovingAverage
                         << "maxElapsed:" << _maxElapsed
                         << "maxElapsedAverage:" << _maxElapsedAverage
                         << "samples:" << _movingAverage.getSamples();

                // Don't actually crash in debug builds, in case this apparent deadlock is simply from
                // the developer actively debugging code
                #ifdef NDEBUG
                deadlockDetectionCrash();
                #endif
            }
        }
    }

    static std::atomic<bool> _paused;
    static std::atomic<uint64_t> _heartbeat;
    static std::atomic<uint64_t> _maxElapsed;
    static std::atomic<int> _maxElapsedAverage;
    static ThreadSafeMovingAverage<int, HEARTBEAT_SAMPLES> _movingAverage;

    bool _quit { false };
};

std::atomic<bool> DeadlockWatchdogThread::_paused;
std::atomic<uint64_t> DeadlockWatchdogThread::_heartbeat;
std::atomic<uint64_t> DeadlockWatchdogThread::_maxElapsed;
std::atomic<int> DeadlockWatchdogThread::_maxElapsedAverage;
ThreadSafeMovingAverage<int, DeadlockWatchdogThread::HEARTBEAT_SAMPLES> DeadlockWatchdogThread::_movingAverage;

bool isDomainURL(QUrl url) {
    if (!url.isValid()) {
        return false;
    }
    if (url.scheme() == URL_SCHEME_HIFI) {
        return true;
    }
    if (url.scheme() != URL_SCHEME_FILE) {
        // TODO -- once Octree::readFromURL no-longer takes over the main event-loop, serverless-domain urls can
        // be loaded over http(s)
        // && url.scheme() != URL_SCHEME_HTTP &&
        // url.scheme() != URL_SCHEME_HTTPS
        return false;
    }
    if (url.path().endsWith(".json", Qt::CaseInsensitive) ||
        url.path().endsWith(".json.gz", Qt::CaseInsensitive)) {
        return true;
    }
    return false;
}

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
                if (isDomainURL(url)) {
                    DependencyManager::get<AddressManager>()->handleLookupString(url.toString());
                    return true;
                }
            }

            if (message->message == WM_DEVICECHANGE) {
                const float MIN_DELTA_SECONDS = 2.0f; // de-bounce signal
                static float lastTriggerTime = 0.0f;
                const float deltaSeconds = secTimestampNow() - lastTriggerTime;
                lastTriggerTime = secTimestampNow();
                if (deltaSeconds > MIN_DELTA_SECONDS) {
                    Midi::USBchanged();                // re-scan the MIDI bus
                }
            }
        }
        return false;
    }
};
#endif

class LambdaEvent : public QEvent {
    std::function<void()> _fun;
public:
    LambdaEvent(const std::function<void()> & fun) :
    QEvent(static_cast<QEvent::Type>(ApplicationEvent::Lambda)), _fun(fun) {
    }
    LambdaEvent(std::function<void()> && fun) :
    QEvent(static_cast<QEvent::Type>(ApplicationEvent::Lambda)), _fun(fun) {
    }
    void call() const { _fun(); }
};

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType) type, context, message);

    if (!logMessage.isEmpty()) {
#ifdef Q_OS_ANDROID
        const char * local=logMessage.toStdString().c_str();
        switch (type) {
            case QtDebugMsg:
                __android_log_write(ANDROID_LOG_DEBUG,"Interface",local);
                break;
            case QtInfoMsg:
                __android_log_write(ANDROID_LOG_INFO,"Interface",local);
                break;
            case QtWarningMsg:
                __android_log_write(ANDROID_LOG_WARN,"Interface",local);
                break;
            case QtCriticalMsg:
                __android_log_write(ANDROID_LOG_ERROR,"Interface",local);
                break;
            case QtFatalMsg:
            default:
                __android_log_write(ANDROID_LOG_FATAL,"Interface",local);
                abort();
        }
#endif
        qApp->getLogger()->addMessage(qPrintable(logMessage));
    }
}


class ApplicationMeshProvider : public scriptable::ModelProviderFactory  {
public:
    virtual scriptable::ModelProviderPointer lookupModelProvider(const QUuid& uuid) override {
        bool success;
        if (auto nestable = DependencyManager::get<SpatialParentFinder>()->find(uuid, success).lock()) {
            auto type = nestable->getNestableType();
#ifdef SCRIPTABLE_MESH_DEBUG
            qCDebug(interfaceapp) << "ApplicationMeshProvider::lookupModelProvider" << uuid << SpatiallyNestable::nestableTypeToString(type);
#endif
            switch (type) {
            case NestableType::Entity:
                return getEntityModelProvider(static_cast<EntityItemID>(uuid));
            case NestableType::Overlay:
                return getOverlayModelProvider(static_cast<OverlayID>(uuid));
            case NestableType::Avatar:
                return getAvatarModelProvider(uuid);
            }
        }
        return nullptr;
    }

private:
    scriptable::ModelProviderPointer getEntityModelProvider(EntityItemID entityID) {
        scriptable::ModelProviderPointer provider;
        auto entityTreeRenderer = qApp->getEntities();
        auto entityTree = entityTreeRenderer->getTree();
        if (auto entity = entityTree->findEntityByID(entityID)) {
            if (auto renderer = entityTreeRenderer->renderableForEntityId(entityID)) {
                provider = std::dynamic_pointer_cast<scriptable::ModelProvider>(renderer);
                provider->modelProviderType = NestableType::Entity;
            } else {
                qCWarning(interfaceapp) << "no renderer for entity ID" << entityID.toString();
            }
        }
        return provider;
    }

    scriptable::ModelProviderPointer getOverlayModelProvider(OverlayID overlayID) {
        scriptable::ModelProviderPointer provider;
        auto &overlays = qApp->getOverlays();
        if (auto overlay = overlays.getOverlay(overlayID)) {
            if (auto base3d = std::dynamic_pointer_cast<Base3DOverlay>(overlay)) {
                provider = std::dynamic_pointer_cast<scriptable::ModelProvider>(base3d);
                provider->modelProviderType = NestableType::Overlay;
            } else {
                qCWarning(interfaceapp) << "no renderer for overlay ID" << overlayID.toString();
            }
        } else {
            qCWarning(interfaceapp) << "overlay not found" << overlayID.toString();
        }
        return provider;
    }

    scriptable::ModelProviderPointer getAvatarModelProvider(QUuid sessionUUID) {
        scriptable::ModelProviderPointer provider;
        auto avatarManager = DependencyManager::get<AvatarManager>();
        if (auto avatar = avatarManager->getAvatarBySessionID(sessionUUID)) {
            provider = std::dynamic_pointer_cast<scriptable::ModelProvider>(avatar);
            provider->modelProviderType = NestableType::Avatar;
        }
        return provider;
    }
};

/**jsdoc
 * <p>The <code>Controller.Hardware.Application</code> object has properties representing Interface's state. The property
 * values are integer IDs, uniquely identifying each output. <em>Read-only.</em> These can be mapped to actions or functions or
 * <code>Controller.Standard</code> items in a {@link RouteObject} mapping (e.g., using the {@link RouteObject#when} method).
 * Each data value is either <code>1.0</code> for "true" or <code>0.0</code> for "false".</p>
 * <table>
 *   <thead>
 *     <tr><th>Property</th><th>Type</th><th>Data</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>CameraFirstPerson</code></td><td>number</td><td>number</td><td>The camera is in first-person mode.
 *       </td></tr>
 *     <tr><td><code>CameraThirdPerson</code></td><td>number</td><td>number</td><td>The camera is in third-person mode.
 *       </td></tr>
 *     <tr><td><code>CameraFSM</code></td><td>number</td><td>number</td><td>The camera is in full screen mirror mode.</td></tr>
 *     <tr><td><code>CameraIndependent</code></td><td>number</td><td>number</td><td>The camera is in independent mode.</td></tr>
 *     <tr><td><code>CameraEntity</code></td><td>number</td><td>number</td><td>The camera is in entity mode.</td></tr>
 *     <tr><td><code>InHMD</code></td><td>number</td><td>number</td><td>The user is in HMD mode.</td></tr>
 *     <tr><td><code>AdvancedMovement</code></td><td>number</td><td>number</td><td>Advanced movement controls are enabled.
 *       </td></tr>
 *     <tr><td><code>SnapTurn</code></td><td>number</td><td>number</td><td>Snap turn is enabled.</td></tr>
 *     <tr><td><code>Grounded</code></td><td>number</td><td>number</td><td>The user's avatar is on the ground.</td></tr>
 *     <tr><td><code>NavigationFocused</code></td><td>number</td><td>number</td><td><em>Not used.</em></td></tr>
 *   </tbody>
 * </table>
 * @typedef {object} Controller.Hardware-Application
 */

static const QString STATE_IN_HMD = "InHMD";
static const QString STATE_CAMERA_FULL_SCREEN_MIRROR = "CameraFSM";
static const QString STATE_CAMERA_FIRST_PERSON = "CameraFirstPerson";
static const QString STATE_CAMERA_THIRD_PERSON = "CameraThirdPerson";
static const QString STATE_CAMERA_ENTITY = "CameraEntity";
static const QString STATE_CAMERA_INDEPENDENT = "CameraIndependent";
static const QString STATE_SNAP_TURN = "SnapTurn";
static const QString STATE_ADVANCED_MOVEMENT_CONTROLS = "AdvancedMovement";
static const QString STATE_GROUNDED = "Grounded";
static const QString STATE_NAV_FOCUSED = "NavigationFocused";
static const QString STATE_PLATFORM_WINDOWS = "PlatformWindows";
static const QString STATE_PLATFORM_MAC = "PlatformMac";
static const QString STATE_PLATFORM_ANDROID = "PlatformAndroid";

// Statically provided display and input plugins
extern DisplayPluginList getDisplayPlugins();
extern InputPluginList getInputPlugins();
extern void saveInputPluginSettings(const InputPluginList& plugins);

// Parameters used for running tests from teh command line
const QString TEST_SCRIPT_COMMAND{ "--testScript" };
const QString TEST_QUIT_WHEN_FINISHED_OPTION{ "quitWhenFinished" };
const QString TEST_RESULTS_LOCATION_COMMAND{ "--testResultsLocation" };

bool setupEssentials(int& argc, char** argv, bool runningMarkerExisted) {
    const char** constArgv = const_cast<const char**>(argv);

    // HRS: I could not figure out how to move these any earlier in startup, so when using this option, be sure to also supply
    // --allowMultipleInstances
    auto reportAndQuit = [&](const char* commandSwitch, std::function<void(FILE* fp)> report) {
        const char* reportfile = getCmdOption(argc, constArgv, commandSwitch);
        // Reports to the specified file, because stdout is set up to be captured for logging.
        if (reportfile) {
            FILE* fp = fopen(reportfile, "w");
            if (fp) {
                report(fp);
                fclose(fp);
                if (!runningMarkerExisted) { // don't leave ours around
                    RunningMarker runingMarker(RUNNING_MARKER_FILENAME);
                    runingMarker.deleteRunningMarkerFile(); // happens in deleter, but making the side-effect explicit.
                }
                _exit(0);
            }
        }
    };
    reportAndQuit("--protocolVersion", [&](FILE* fp) {
        auto version = protocolVersionsSignatureBase64();
        fputs(version.toLatin1().data(), fp);
    });
    reportAndQuit("--version", [&](FILE* fp) {
        fputs(BuildInfo::VERSION.toLatin1().data(), fp);
    });

    const char* portStr = getCmdOption(argc, constArgv, "--listenPort");
    const int listenPort = portStr ? atoi(portStr) : INVALID_PORT;

    static const auto SUPPRESS_SETTINGS_RESET = "--suppress-settings-reset";
    bool suppressPrompt = cmdOptionExists(argc, const_cast<const char**>(argv), SUPPRESS_SETTINGS_RESET);

    // Ignore any previous crashes if running from command line with a test script.
    bool inTestMode { false };
    for (int i = 0; i < argc; ++i) {
        QString parameter(argv[i]);
        if (parameter == TEST_SCRIPT_COMMAND) {
            inTestMode = true;
            break;
        }
    }

    bool previousSessionCrashed { false };
    if (!inTestMode) {
        previousSessionCrashed = CrashRecoveryHandler::checkForResetSettings(runningMarkerExisted, suppressPrompt);
    }

    // get dir to use for cache
    static const auto CACHE_SWITCH = "--cache";
    QString cacheDir = getCmdOption(argc, const_cast<const char**>(argv), CACHE_SWITCH);
    if (!cacheDir.isEmpty()) {
        qApp->setProperty(hifi::properties::APP_LOCAL_DATA_PATH, cacheDir);
    }

    {
        const QString resourcesBinaryFile = PathUtils::getRccPath();
        if (!QFile::exists(resourcesBinaryFile)) {
            throw std::runtime_error("Unable to find primary resources");
        }
        if (!QResource::registerResource(resourcesBinaryFile)) {
            throw std::runtime_error("Unable to load primary resources");
        }
    }

    // Tell the plugin manager about our statically linked plugins
    DependencyManager::set<PluginManager>();
    auto pluginManager = PluginManager::getInstance();
    pluginManager->setInputPluginProvider([] { return getInputPlugins(); });
    pluginManager->setDisplayPluginProvider([] { return getDisplayPlugins(); });
    pluginManager->setInputPluginSettingsPersister([](const InputPluginList& plugins) { saveInputPluginSettings(plugins); });
    if (auto steamClient = pluginManager->getSteamClientPlugin()) {
        steamClient->init();
    }

    PROFILE_SET_THREAD_NAME("Main Thread");

#if defined(Q_OS_WIN)
    // Select appropriate audio DLL
    QString audioDLLPath = QCoreApplication::applicationDirPath();
    if (IsWindows8OrGreater()) {
        audioDLLPath += "/audioWin8";
    } else {
        audioDLLPath += "/audioWin7";
    }
    QCoreApplication::addLibraryPath(audioDLLPath);
#endif

    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    DependencyManager::registerInheritance<AvatarHashMap, AvatarManager>();
    DependencyManager::registerInheritance<EntityDynamicFactoryInterface, InterfaceDynamicFactory>();
    DependencyManager::registerInheritance<SpatialParentFinder, InterfaceParentFinder>();

    // Set dependencies
    DependencyManager::set<PickManager>();
    DependencyManager::set<PointerManager>();
    DependencyManager::set<LaserPointerScriptingInterface>();
    DependencyManager::set<RayPickScriptingInterface>();
    DependencyManager::set<PointerScriptingInterface>();
    DependencyManager::set<PickScriptingInterface>();
    DependencyManager::set<Cursor::Manager>();
    DependencyManager::set<VirtualPad::Manager>();
    DependencyManager::set<DesktopPreviewProvider>();
#if defined(Q_OS_ANDROID)
    DependencyManager::set<AccountManager>(); // use the default user agent getter
#else
    DependencyManager::set<AccountManager>(std::bind(&Application::getUserAgent, qApp));
#endif
    DependencyManager::set<StatTracker>();
    DependencyManager::set<ScriptEngines>(ScriptEngine::CLIENT_SCRIPT);
    DependencyManager::set<ScriptInitializerMixin, NativeScriptInitializers>();
    DependencyManager::set<Preferences>();
    DependencyManager::set<recording::Deck>();
    DependencyManager::set<recording::Recorder>();
    DependencyManager::set<AddressManager>();
    DependencyManager::set<NodeList>(NodeType::Agent, listenPort);
    DependencyManager::set<recording::ClipCache>();
    DependencyManager::set<GeometryCache>();
    DependencyManager::set<ModelCache>();
    DependencyManager::set<ModelCacheScriptingInterface>();
    DependencyManager::set<ScriptCache>();
    DependencyManager::set<SoundCache>();
    DependencyManager::set<SoundCacheScriptingInterface>();
    DependencyManager::set<DdeFaceTracker>();
    DependencyManager::set<EyeTracker>();
    DependencyManager::set<AudioClient>();
    DependencyManager::set<AudioScope>();
    DependencyManager::set<DeferredLightingEffect>();
    DependencyManager::set<TextureCache>();
    DependencyManager::set<TextureCacheScriptingInterface>();
    DependencyManager::set<FramebufferCache>();
    DependencyManager::set<AnimationCache>();
    DependencyManager::set<AnimationCacheScriptingInterface>();
    DependencyManager::set<ModelBlender>();
    DependencyManager::set<UsersScriptingInterface>();
    DependencyManager::set<AvatarManager>();
    DependencyManager::set<LODManager>();
    DependencyManager::set<StandAloneJSConsole>();
    DependencyManager::set<DialogsManager>();
    DependencyManager::set<BandwidthRecorder>();
    DependencyManager::set<ResourceCacheSharedItems>();
    DependencyManager::set<DesktopScriptingInterface>();
    DependencyManager::set<EntityScriptingInterface>(true);
    DependencyManager::set<GraphicsScriptingInterface>();
    DependencyManager::registerInheritance<scriptable::ModelProviderFactory, ApplicationMeshProvider>();
    DependencyManager::set<ApplicationMeshProvider>();
    DependencyManager::set<RecordingScriptingInterface>();
    DependencyManager::set<WindowScriptingInterface>();
    DependencyManager::set<HMDScriptingInterface>();
    DependencyManager::set<ResourceScriptingInterface>();
    DependencyManager::set<TabletScriptingInterface>();
    DependencyManager::set<InputConfiguration>();
    DependencyManager::set<ToolbarScriptingInterface>();
    DependencyManager::set<UserActivityLoggerScriptingInterface>();
    DependencyManager::set<AssetMappingsScriptingInterface>();
    DependencyManager::set<DomainConnectionModel>();

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    DependencyManager::set<SpeechRecognizer>();
#endif
    DependencyManager::set<DiscoverabilityManager>();
    DependencyManager::set<SceneScriptingInterface>();
    DependencyManager::set<OffscreenUi>();
    DependencyManager::set<Midi>();
    DependencyManager::set<PathUtils>();
    DependencyManager::set<InterfaceDynamicFactory>();
    DependencyManager::set<AudioInjectorManager>();
    DependencyManager::set<MessagesClient>();
    controller::StateController::setStateVariables({ { STATE_IN_HMD, STATE_CAMERA_FULL_SCREEN_MIRROR,
                    STATE_CAMERA_FIRST_PERSON, STATE_CAMERA_THIRD_PERSON, STATE_CAMERA_ENTITY, STATE_CAMERA_INDEPENDENT,
                    STATE_SNAP_TURN, STATE_ADVANCED_MOVEMENT_CONTROLS, STATE_GROUNDED, STATE_NAV_FOCUSED,
                    STATE_PLATFORM_WINDOWS, STATE_PLATFORM_MAC, STATE_PLATFORM_ANDROID } });
    DependencyManager::set<UserInputMapper>();
    DependencyManager::set<controller::ScriptingInterface, ControllerScriptingInterface>();
    DependencyManager::set<InterfaceParentFinder>();
    DependencyManager::set<EntityTreeRenderer>(true, qApp, qApp);
    DependencyManager::set<CompositorHelper>();
    DependencyManager::set<OffscreenQmlSurfaceCache>();
    DependencyManager::set<EntityScriptClient>();
    DependencyManager::set<EntityScriptServerLogClient>();
    DependencyManager::set<GooglePolyScriptingInterface>();
    DependencyManager::set<OctreeStatsProvider>(nullptr, qApp->getOcteeSceneStats());
    DependencyManager::set<AvatarBookmarks>();
    DependencyManager::set<LocationBookmarks>();
    DependencyManager::set<Snapshot>();
    DependencyManager::set<CloseEventSender>();
    DependencyManager::set<ResourceManager>();
    DependencyManager::set<SelectionScriptingInterface>();
    DependencyManager::set<Ledger>();
    DependencyManager::set<Wallet>();
    DependencyManager::set<WalletScriptingInterface>();

    DependencyManager::set<FadeEffect>();

    return previousSessionCrashed;
}

// FIXME move to header, or better yet, design some kind of UI manager
// to take care of highlighting keyboard focused items, rather than
// continuing to overburden Application.cpp
std::shared_ptr<Cube3DOverlay> _keyboardFocusHighlight{ nullptr };
OverlayID _keyboardFocusHighlightID{ UNKNOWN_OVERLAY_ID };


OffscreenGLCanvas* _qmlShareContext { nullptr };

// FIXME hack access to the internal share context for the Chromium helper
// Normally we'd want to use QWebEngine::initialize(), but we can't because
// our primary context is a QGLWidget, which can't easily be initialized to share
// from a QOpenGLContext.
//
// So instead we create a new offscreen context to share with the QGLWidget,
// and manually set THAT to be the shared context for the Chromium helper
#if !defined(DISABLE_QML)
OffscreenGLCanvas* _chromiumShareContext { nullptr };
Q_GUI_EXPORT void qt_gl_set_global_share_context(QOpenGLContext *context);
#endif

Setting::Handle<int> sessionRunTime{ "sessionRunTime", 0 };

const float DEFAULT_HMD_TABLET_SCALE_PERCENT = 70.0f;
const float DEFAULT_DESKTOP_TABLET_SCALE_PERCENT = 75.0f;
const bool DEFAULT_DESKTOP_TABLET_BECOMES_TOOLBAR = true;
const bool DEFAULT_HMD_TABLET_BECOMES_TOOLBAR = false;
const bool DEFAULT_PREFER_STYLUS_OVER_LASER = false;
const bool DEFAULT_PREFER_AVATAR_FINGER_OVER_STYLUS = false;
const QString DEFAULT_CURSOR_NAME = "DEFAULT";

Application::Application(int& argc, char** argv, QElapsedTimer& startupTimer, bool runningMarkerExisted) :
    QApplication(argc, argv),
    _window(new MainWindow(desktop())),
    _sessionRunTimer(startupTimer),
    _previousSessionCrashed(setupEssentials(argc, argv, runningMarkerExisted)),
    _undoStackScriptingInterface(&_undoStack),
    _entitySimulation(new PhysicalEntitySimulation()),
    _physicsEngine(new PhysicsEngine(Vectors::ZERO)),
    _entityClipboard(new EntityTree()),
    _previousScriptLocation("LastScriptLocation", DESKTOP_LOCATION),
    _fieldOfView("fieldOfView", DEFAULT_FIELD_OF_VIEW_DEGREES),
    _hmdTabletScale("hmdTabletScale", DEFAULT_HMD_TABLET_SCALE_PERCENT),
    _desktopTabletScale("desktopTabletScale", DEFAULT_DESKTOP_TABLET_SCALE_PERCENT),
    _desktopTabletBecomesToolbarSetting("desktopTabletBecomesToolbar", DEFAULT_DESKTOP_TABLET_BECOMES_TOOLBAR),
    _hmdTabletBecomesToolbarSetting("hmdTabletBecomesToolbar", DEFAULT_HMD_TABLET_BECOMES_TOOLBAR),
    _preferStylusOverLaserSetting("preferStylusOverLaser", DEFAULT_PREFER_STYLUS_OVER_LASER),
    _preferAvatarFingerOverStylusSetting("preferAvatarFingerOverStylus", DEFAULT_PREFER_AVATAR_FINGER_OVER_STYLUS),
    _constrainToolbarPosition("toolbar/constrainToolbarToCenterX", true),
    _preferredCursor("preferredCursor", DEFAULT_CURSOR_NAME),
    _scaleMirror(1.0f),
    _mirrorYawOffset(0.0f),
    _raiseMirror(0.0f),
    _enableProcessOctreeThread(true),
    _lastNackTime(usecTimestampNow()),
    _lastSendDownstreamAudioStats(usecTimestampNow()),
    _aboutToQuit(false),
    _notifiedPacketVersionMismatchThisDomain(false),
    _maxOctreePPS(maxOctreePacketsPerSecond.get()),
    _lastFaceTrackerUpdate(0),
    _snapshotSound(nullptr),
    _sampleSound(nullptr)

{

    auto steamClient = PluginManager::getInstance()->getSteamClientPlugin();
    setProperty(hifi::properties::STEAM, (steamClient && steamClient->isRunning()));
    setProperty(hifi::properties::CRASHED, _previousSessionCrashed);
    {
        const QStringList args = arguments();

        for (int i = 0; i < args.size() - 1; ++i) {
            if (args.at(i) == TEST_SCRIPT_COMMAND && (i + 1) < args.size()) {
                QString testScriptPath = args.at(i + 1);

                // If the URL scheme is http(s) or ftp, then use as is, else - treat it as a local file
                // This is done so as not break previous command line scripts
                if (testScriptPath.left(URL_SCHEME_HTTP.length()) == URL_SCHEME_HTTP ||
                    testScriptPath.left(URL_SCHEME_FTP.length()) == URL_SCHEME_FTP) {

                    setProperty(hifi::properties::TEST, QUrl::fromUserInput(testScriptPath));
                } else if (QFileInfo(testScriptPath).exists()) {
                    setProperty(hifi::properties::TEST, QUrl::fromLocalFile(testScriptPath));
                }

                // quite when finished parameter must directly follow the test script
                if ((i + 2) < args.size() && args.at(i + 2) == TEST_QUIT_WHEN_FINISHED_OPTION) {
                    quitWhenFinished = true;
                }
            } else if (args.at(i) == TEST_RESULTS_LOCATION_COMMAND) {
                // Set test snapshot location only if it is a writeable directory
                QString path(args.at(i + 1));

                QFileInfo fileInfo(path);
                if (fileInfo.isDir() && fileInfo.isWritable()) {
                    TestScriptingInterface::getInstance()->setTestResultsLocation(path);
                }
            }
        }
    }

    // make sure the debug draw singleton is initialized on the main thread.
    DebugDraw::getInstance().removeMarker("");

    PluginContainer* pluginContainer = dynamic_cast<PluginContainer*>(this); // set the container for any plugins that care
    PluginManager::getInstance()->setContainer(pluginContainer);

    QThreadPool::globalInstance()->setMaxThreadCount(MIN_PROCESSING_THREAD_POOL_SIZE);
    thread()->setPriority(QThread::HighPriority);
    thread()->setObjectName("Main Thread");

    setInstance(this);

    auto controllerScriptingInterface = DependencyManager::get<controller::ScriptingInterface>().data();
    _controllerScriptingInterface = dynamic_cast<ControllerScriptingInterface*>(controllerScriptingInterface);

    _entityClipboard->createRootElement();

#ifdef Q_OS_WIN
    installNativeEventFilter(&MyNativeEventFilter::getInstance());
#endif

    _logger = new FileLogger(this);
    qInstallMessageHandler(messageHandler);

    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "styles/Inconsolata.otf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/fontawesome-webfont.ttf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/hifi-glyphs.ttf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/AnonymousPro-Regular.ttf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/FiraSans-Regular.ttf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/FiraSans-SemiBold.ttf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/Raleway-Light.ttf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/Raleway-Regular.ttf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/Raleway-Bold.ttf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/Raleway-SemiBold.ttf");
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "fonts/Cairo-SemiBold.ttf");
    _window->setWindowTitle("High Fidelity Interface");

    Model::setAbstractViewStateInterface(this); // The model class will sometimes need to know view state details from us

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->startThread();

    // move the AddressManager to the NodeList thread so that domain resets due to domain changes always occur
    // before we tell MyAvatar to go to a new location in the new domain
    auto addressManager = DependencyManager::get<AddressManager>();
    addressManager->moveToThread(nodeList->thread());

    const char** constArgv = const_cast<const char**>(argv);
    if (cmdOptionExists(argc, constArgv, "--disableWatchdog")) {
        DISABLE_WATCHDOG = true;
    }
    // Set up a watchdog thread to intentionally crash the application on deadlocks
    if (!DISABLE_WATCHDOG) {
        (new DeadlockWatchdogThread())->start();
    }

    // Set File Logger Session UUID
    auto avatarManager = DependencyManager::get<AvatarManager>();
    auto myAvatar = avatarManager ? avatarManager->getMyAvatar() : nullptr;
    if (avatarManager) {
        workload::SpacePointer space = getEntities()->getWorkloadSpace();
        avatarManager->setSpace(space);
    }
    auto accountManager = DependencyManager::get<AccountManager>();

    _logger->setSessionID(accountManager->getSessionID());

    setCrashAnnotation("metaverse_session_id", accountManager->getSessionID().toString().toStdString());
    setCrashAnnotation("main_thread_id", std::to_string((size_t)QThread::currentThreadId()));

    if (steamClient) {
        qCDebug(interfaceapp) << "[VERSION] SteamVR buildID:" << steamClient->getSteamVRBuildID();
    }
    setCrashAnnotation("steam", property(hifi::properties::STEAM).toBool() ? "1" : "0");

    qCDebug(interfaceapp) << "[VERSION] Build sequence:" << qPrintable(applicationVersion());
    qCDebug(interfaceapp) << "[VERSION] MODIFIED_ORGANIZATION:" << BuildInfo::MODIFIED_ORGANIZATION;
    qCDebug(interfaceapp) << "[VERSION] VERSION:" << BuildInfo::VERSION;
    qCDebug(interfaceapp) << "[VERSION] BUILD_TYPE_STRING:" << BuildInfo::BUILD_TYPE_STRING;
    qCDebug(interfaceapp) << "[VERSION] BUILD_GLOBAL_SERVICES:" << BuildInfo::BUILD_GLOBAL_SERVICES;
#if USE_STABLE_GLOBAL_SERVICES
    qCDebug(interfaceapp) << "[VERSION] We will use STABLE global services.";
#else
    qCDebug(interfaceapp) << "[VERSION] We will use DEVELOPMENT global services.";
#endif

    // set the OCULUS_STORE property so the oculus plugin can know if we ran from the Oculus Store
    static const QString OCULUS_STORE_ARG = "--oculus-store";
    setProperty(hifi::properties::OCULUS_STORE, arguments().indexOf(OCULUS_STORE_ARG) != -1);

    updateHeartbeat();

    // setup a timer for domain-server check ins
    QTimer* domainCheckInTimer = new QTimer(this);
    QWeakPointer<NodeList> nodeListWeak = nodeList;
    connect(domainCheckInTimer, &QTimer::timeout, [this, nodeListWeak] {
        auto nodeList = nodeListWeak.lock();
        if (!isServerlessMode() && nodeList) {
            nodeList->sendDomainServerCheckIn();
        }
    });
    domainCheckInTimer->start(DOMAIN_SERVER_CHECK_IN_MSECS);
    connect(this, &QCoreApplication::aboutToQuit, [domainCheckInTimer] {
        domainCheckInTimer->stop();
        domainCheckInTimer->deleteLater();
    });

    {
        auto audioIO = DependencyManager::get<AudioClient>().data();
        audioIO->setPositionGetter([] {
            auto avatarManager = DependencyManager::get<AvatarManager>();
            auto myAvatar = avatarManager ? avatarManager->getMyAvatar() : nullptr;

            return myAvatar ? myAvatar->getPositionForAudio() : Vectors::ZERO;
        });
        audioIO->setOrientationGetter([] {
            auto avatarManager = DependencyManager::get<AvatarManager>();
            auto myAvatar = avatarManager ? avatarManager->getMyAvatar() : nullptr;

            return myAvatar ? myAvatar->getOrientationForAudio() : Quaternions::IDENTITY;
        });

        recording::Frame::registerFrameHandler(AudioConstants::getAudioFrameName(), [&audioIO](recording::Frame::ConstPointer frame) {
            audioIO->handleRecordedAudioInput(frame->data);
        });

        connect(audioIO, &AudioClient::inputReceived, [](const QByteArray& audio) {
            static auto recorder = DependencyManager::get<recording::Recorder>();
            if (recorder->isRecording()) {
                static const recording::FrameType AUDIO_FRAME_TYPE = recording::Frame::registerFrameType(AudioConstants::getAudioFrameName());
                recorder->recordFrame(AUDIO_FRAME_TYPE, audio);
            }
        });
        audioIO->startThread();
    }

    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    // Setup MessagesClient
    DependencyManager::get<MessagesClient>()->startThread();

    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    connect(&domainHandler, SIGNAL(domainURLChanged(QUrl)), SLOT(domainURLChanged(QUrl)));
    connect(&domainHandler, SIGNAL(redirectToErrorDomainURL(QUrl)), SLOT(goToErrorDomainURL(QUrl)));
    connect(&domainHandler, &DomainHandler::domainURLChanged, [](QUrl domainURL){
        setCrashAnnotation("domain", domainURL.toString().toStdString());
    });
    connect(&domainHandler, SIGNAL(resetting()), SLOT(resettingDomain()));
    connect(&domainHandler, SIGNAL(connectedToDomain(QUrl)), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(updateWindowTitle()));
    connect(&domainHandler, &DomainHandler::disconnectedFromDomain, this, [this]() {
        getOverlays().deleteOverlay(getTabletScreenID());
        getOverlays().deleteOverlay(getTabletHomeButtonID());
        getOverlays().deleteOverlay(getTabletFrameID());
    });
    connect(&domainHandler, &DomainHandler::domainConnectionRefused, this, &Application::domainConnectionRefused);

    nodeList->getDomainHandler().setErrorDomainURL(QUrl(REDIRECT_HIFI_ADDRESS));

    // We could clear ATP assets only when changing domains, but it's possible that the domain you are connected
    // to has gone down and switched to a new content set, so when you reconnect the cached ATP assets will no longer be valid.
    connect(&domainHandler, &DomainHandler::disconnectedFromDomain, DependencyManager::get<ScriptCache>().data(), &ScriptCache::clearATPScriptsFromCache);

    // update our location every 5 seconds in the metaverse server, assuming that we are authenticated with one
    const qint64 DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS = 5 * MSECS_PER_SECOND;

    auto discoverabilityManager = DependencyManager::get<DiscoverabilityManager>();
    connect(&locationUpdateTimer, &QTimer::timeout, discoverabilityManager.data(), &DiscoverabilityManager::updateLocation);
    connect(&locationUpdateTimer, &QTimer::timeout,
        DependencyManager::get<AddressManager>().data(), &AddressManager::storeCurrentAddress);
    locationUpdateTimer.start(DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS);

    // if we get a domain change, immediately attempt update location in metaverse server
    connect(&nodeList->getDomainHandler(), &DomainHandler::connectedToDomain,
        discoverabilityManager.data(), &DiscoverabilityManager::updateLocation);

    // send a location update immediately
    discoverabilityManager->updateLocation();

    connect(nodeList.data(), &NodeList::nodeAdded, this, &Application::nodeAdded);
    connect(nodeList.data(), &NodeList::nodeKilled, this, &Application::nodeKilled);
    connect(nodeList.data(), &NodeList::nodeActivated, this, &Application::nodeActivated);
    connect(nodeList.data(), &NodeList::uuidChanged, myAvatar.get(), &MyAvatar::setSessionUUID);
    connect(nodeList.data(), &NodeList::uuidChanged, this, &Application::setSessionUUID);
    connect(nodeList.data(), &NodeList::packetVersionMismatch, this, &Application::notifyPacketVersionMismatch);

    // you might think we could just do this in NodeList but we only want this connection for Interface
    connect(&nodeList->getDomainHandler(), SIGNAL(limitOfSilentDomainCheckInsReached()),
            nodeList.data(), SLOT(reset()));

    auto dialogsManager = DependencyManager::get<DialogsManager>();
#if defined(Q_OS_ANDROID)
    connect(accountManager.data(), &AccountManager::authRequired, this, []() {
        AndroidHelper::instance().showLoginDialog();
    });
#else
    connect(accountManager.data(), &AccountManager::authRequired, dialogsManager.data(), &DialogsManager::showLoginDialog);
#endif
    connect(accountManager.data(), &AccountManager::usernameChanged, this, &Application::updateWindowTitle);

    // set the account manager's root URL and trigger a login request if we don't have the access token
    accountManager->setIsAgent(true);
    accountManager->setAuthURL(NetworkingConstants::METAVERSE_SERVER_URL());

    // use our MyAvatar position and quat for address manager path
    addressManager->setPositionGetter([this]{ return getMyAvatar()->getWorldFeetPosition(); });
    addressManager->setOrientationGetter([this]{ return getMyAvatar()->getWorldOrientation(); });

    connect(addressManager.data(), &AddressManager::hostChanged, this, &Application::updateWindowTitle);
    connect(this, &QCoreApplication::aboutToQuit, addressManager.data(), &AddressManager::storeCurrentAddress);

    connect(this, &Application::activeDisplayPluginChanged, this, &Application::updateThreadPoolCount);
    connect(this, &Application::activeDisplayPluginChanged, this, [](){
        qApp->setProperty(hifi::properties::HMD, qApp->isHMDMode());
        auto displayPlugin = qApp->getActiveDisplayPlugin();
        setCrashAnnotation("display_plugin", displayPlugin->getName().toStdString());
        setCrashAnnotation("hmd", displayPlugin->isHmd() ? "1" : "0");
    });
    connect(this, &Application::activeDisplayPluginChanged, this, &Application::updateSystemTabletMode);

    // Save avatar location immediately after a teleport.
    connect(myAvatar.get(), &MyAvatar::positionGoneTo,
        DependencyManager::get<AddressManager>().data(), &AddressManager::storeCurrentAddress);

    connect(myAvatar.get(), &MyAvatar::skeletonModelURLChanged, [](){
        QUrl avatarURL = qApp->getMyAvatar()->getSkeletonModelURL();
        setCrashAnnotation("avatar", avatarURL.toString().toStdString());
    });


    // Inititalize sample before registering
    _sampleSound = DependencyManager::get<SoundCache>()->getSound(PathUtils::resourcesUrl("sounds/sample.wav"));

    {
        auto scriptEngines = DependencyManager::get<ScriptEngines>().data();
        scriptEngines->registerScriptInitializer([this](ScriptEnginePointer engine) {
            registerScriptEngineWithApplicationServices(engine);
        });

        connect(scriptEngines, &ScriptEngines::scriptCountChanged, this, [this] {
            auto scriptEngines = DependencyManager::get<ScriptEngines>();
            if (scriptEngines->getRunningScripts().isEmpty()) {
                getMyAvatar()->clearScriptableSettings();
            }
        }, Qt::QueuedConnection);

        connect(scriptEngines, &ScriptEngines::scriptsReloading, this, [this] {
            getEntities()->reloadEntityScripts();
            loadAvatarScripts(getMyAvatar()->getScriptUrls());
        }, Qt::QueuedConnection);

        connect(scriptEngines, &ScriptEngines::scriptLoadError,
            this, [](const QString& filename, const QString& error) {
            OffscreenUi::asyncWarning(nullptr, "Error Loading Script", filename + " failed to load.");
        }, Qt::QueuedConnection);
    }

#ifdef _WIN32
    WSADATA WsaData;
    int wsaresult = WSAStartup(MAKEWORD(2, 2), &WsaData);
#endif

    // tell the NodeList instance who to tell the domain server we care about
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
        << NodeType::EntityServer << NodeType::AssetServer << NodeType::MessagesMixer << NodeType::EntityScriptServer);

    // connect to the packet sent signal of the _entityEditSender
    connect(&_entityEditSender, &EntityEditPacketSender::packetSent, this, &Application::packetSent);
    connect(&_entityEditSender, &EntityEditPacketSender::addingEntityWithCertificate, this, &Application::addingEntityWithCertificate);

    QString concurrentDownloadsStr = getCmdOption(argc, constArgv, "--concurrent-downloads");
    bool success;
    int concurrentDownloads = concurrentDownloadsStr.toInt(&success);
    if (!success) {
        concurrentDownloads = MAX_CONCURRENT_RESOURCE_DOWNLOADS;
    }
    ResourceCache::setRequestLimit(concurrentDownloads);

    // perhaps override the avatar url.  Since we will test later for validity
    // we don't need to do so here.
    QString avatarURL = getCmdOption(argc, constArgv, "--avatarURL");
    _avatarOverrideUrl = QUrl::fromUserInput(avatarURL);

    // If someone specifies both --avatarURL and --replaceAvatarURL,
    // the replaceAvatarURL wins.  So only set the _overrideUrl if this
    // does have a non-empty string.
    QString replaceURL = getCmdOption(argc, constArgv, "--replaceAvatarURL");
    if (!replaceURL.isEmpty()) {
        _avatarOverrideUrl = QUrl::fromUserInput(replaceURL);
        _saveAvatarOverrideUrl = true;
    }

    _glWidget = new GLCanvas();
    getApplicationCompositor().setRenderingWidget(_glWidget);
    _window->setCentralWidget(_glWidget);

    _window->restoreGeometry();
    _window->setVisible(true);

    _glWidget->setFocusPolicy(Qt::StrongFocus);
    _glWidget->setFocus();

    if (cmdOptionExists(argc, constArgv, "--system-cursor")) {
        _preferredCursor.set(Cursor::Manager::getIconName(Cursor::Icon::SYSTEM));
    }
    showCursor(Cursor::Manager::lookupIcon(_preferredCursor.get()));

    // enable mouse tracking; otherwise, we only get drag events
    _glWidget->setMouseTracking(true);
    // Make sure the window is set to the correct size by processing the pending events
    QCoreApplication::processEvents();
    _glWidget->createContext();

    // Create the main thread context, the GPU backend
    initializeGL();
    qCDebug(interfaceapp, "Initialized GL");

    // Initialize the display plugin architecture
    initializeDisplayPlugins();
    qCDebug(interfaceapp, "Initialized Display");

    // An audio device changed signal received before the display plugins are set up will cause a crash,
    // so we defer the setup of the `scripting::Audio` class until this point
    {
        auto audioScriptingInterface = DependencyManager::set<AudioScriptingInterface, scripting::Audio>();
        auto audioIO = DependencyManager::get<AudioClient>().data();
        connect(audioIO, &AudioClient::mutedByMixer, audioScriptingInterface.data(), &AudioScriptingInterface::mutedByMixer);
        connect(audioIO, &AudioClient::receivedFirstPacket, audioScriptingInterface.data(), &AudioScriptingInterface::receivedFirstPacket);
        connect(audioIO, &AudioClient::disconnected, audioScriptingInterface.data(), &AudioScriptingInterface::disconnected);
        connect(audioIO, &AudioClient::muteEnvironmentRequested, [](glm::vec3 position, float radius) {
            auto audioClient = DependencyManager::get<AudioClient>();
            auto audioScriptingInterface = DependencyManager::get<AudioScriptingInterface>();
            auto myAvatarPosition = DependencyManager::get<AvatarManager>()->getMyAvatar()->getWorldPosition();
            float distance = glm::distance(myAvatarPosition, position);

            if (distance < radius) {
                audioClient->setMuted(true);
                audioScriptingInterface->environmentMuted();
            }
        });
        connect(this, &Application::activeDisplayPluginChanged,
            reinterpret_cast<scripting::Audio*>(audioScriptingInterface.data()), &scripting::Audio::onContextChanged);
    }

    // Create the rendering engine.  This can be slow on some machines due to lots of
    // GPU pipeline creation.
    initializeRenderEngine();
    qCDebug(interfaceapp, "Initialized Render Engine.");

    // Overlays need to exist before we set the ContextOverlayInterface dependency
    _overlays.init(); // do this before scripts load
    DependencyManager::set<ContextOverlayInterface>();

    // Initialize the user interface and menu system
    // Needs to happen AFTER the render engine initialization to access its configuration
    initializeUi();

    init();
    qCDebug(interfaceapp, "init() complete.");

    // create thread for parsing of octree data independent of the main network and rendering threads
    _octreeProcessor.initialize(_enableProcessOctreeThread);
    connect(&_octreeProcessor, &OctreePacketProcessor::packetVersionMismatch, this, &Application::notifyPacketVersionMismatch);
    _entityEditSender.initialize(_enableProcessOctreeThread);

    _idleLoopStdev.reset();

    // update before the first render
    update(0);

    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    constexpr auto INSTALLER_INI_NAME = "installer.ini";
    auto iniPath = QDir(applicationDirPath()).filePath(INSTALLER_INI_NAME);
    QFile installerFile { iniPath };
    std::unordered_map<QString, QString> installerKeyValues;
    if (installerFile.open(QIODevice::ReadOnly)) {
        while (!installerFile.atEnd()) {
            auto line = installerFile.readLine();
            if (!line.isEmpty()) {
                auto index = line.indexOf("=");
                if (index >= 0) {
                    installerKeyValues[line.mid(0, index).trimmed()] = line.mid(index + 1).trimmed();
                }
            }
        }
    }

    // In practice we shouldn't run across installs that don't have a known installer type.
    // Client or Client+Server installs should always have the installer.ini next to their
    // respective interface.exe, and Steam installs will be detected as such. If a user were
    // to delete the installer.ini, though, and as an example, we won't know the context of the
    // original install.
    constexpr auto INSTALLER_KEY_TYPE = "type";
    constexpr auto INSTALLER_KEY_CAMPAIGN = "campaign";
    constexpr auto INSTALLER_TYPE_UNKNOWN = "unknown";
    constexpr auto INSTALLER_TYPE_STEAM = "steam";

    auto typeIt = installerKeyValues.find(INSTALLER_KEY_TYPE);
    QString installerType = INSTALLER_TYPE_UNKNOWN;
    if (typeIt == installerKeyValues.end()) {
        if (property(hifi::properties::STEAM).toBool()) {
            installerType = INSTALLER_TYPE_STEAM;
        }
    } else {
        installerType = typeIt->second;
    }

    auto campaignIt = installerKeyValues.find(INSTALLER_KEY_CAMPAIGN);
    QString installerCampaign = campaignIt != installerKeyValues.end() ? campaignIt->second : "";

    qDebug() << "Detected installer type:" << installerType;
    qDebug() << "Detected installer campaign:" << installerCampaign;

    // add firstRun flag from settings to launch event
    Setting::Handle<bool> firstRun { Settings::firstRun, true };

    auto& userActivityLogger = UserActivityLogger::getInstance();
    if (userActivityLogger.isEnabled()) {
        // sessionRunTime will be reset soon by loadSettings. Grab it now to get previous session value.
        // The value will be 0 if the user blew away settings this session, which is both a feature and a bug.
        static const QString TESTER = "HIFI_TESTER";
        auto gpuIdent = GPUIdent::getInstance();
        auto glContextData = getGLContextData();
        QJsonObject properties = {
            { "version", applicationVersion() },
            { "tester", QProcessEnvironment::systemEnvironment().contains(TESTER) },
            { "installer_campaign", installerCampaign },
            { "installer_type", installerType },
            { "build_type", BuildInfo::BUILD_TYPE_STRING },
            { "previousSessionCrashed", _previousSessionCrashed },
            { "previousSessionRuntime", sessionRunTime.get() },
            { "cpu_architecture", QSysInfo::currentCpuArchitecture() },
            { "kernel_type", QSysInfo::kernelType() },
            { "kernel_version", QSysInfo::kernelVersion() },
            { "os_type", QSysInfo::productType() },
            { "os_version", QSysInfo::productVersion() },
            { "gpu_name", gpuIdent->getName() },
            { "gpu_driver", gpuIdent->getDriver() },
            { "gpu_memory", static_cast<qint64>(gpuIdent->getMemory()) },
            { "gl_version_int", glVersionToInteger(glContextData.value("version").toString()) },
            { "gl_version", glContextData["version"] },
            { "gl_vender", glContextData["vendor"] },
            { "gl_sl_version", glContextData["sl_version"] },
            { "gl_renderer", glContextData["renderer"] },
            { "ideal_thread_count", QThread::idealThreadCount() }
        };
        auto macVersion = QSysInfo::macVersion();
        if (macVersion != QSysInfo::MV_None) {
            properties["os_osx_version"] = QSysInfo::macVersion();
        }
        auto windowsVersion = QSysInfo::windowsVersion();
        if (windowsVersion != QSysInfo::WV_None) {
            properties["os_win_version"] = QSysInfo::windowsVersion();
        }

        ProcessorInfo procInfo;
        if (getProcessorInfo(procInfo)) {
            properties["processor_core_count"] = procInfo.numProcessorCores;
            properties["logical_processor_count"] = procInfo.numLogicalProcessors;
            properties["processor_l1_cache_count"] = procInfo.numProcessorCachesL1;
            properties["processor_l2_cache_count"] = procInfo.numProcessorCachesL2;
            properties["processor_l3_cache_count"] = procInfo.numProcessorCachesL3;
        }

        properties["first_run"] = firstRun.get();

        // add the user's machine ID to the launch event
        QString machineFingerPrint = uuidStringWithoutCurlyBraces(FingerprintUtils::getMachineFingerprint());
        properties["machine_fingerprint"] = machineFingerPrint;

        userActivityLogger.logAction("launch", properties);
    }

    _entityEditSender.setMyAvatar(myAvatar.get());

    // The entity octree will have to know about MyAvatar for the parentJointName import
    getEntities()->getTree()->setMyAvatar(myAvatar);
    _entityClipboard->setMyAvatar(myAvatar);

    // For now we're going to set the PPS for outbound packets to be super high, this is
    // probably not the right long term solution. But for now, we're going to do this to
    // allow you to move an entity around in your hand
    _entityEditSender.setPacketsPerSecond(3000); // super high!!

    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));

    // hook up bandwidth estimator
    QSharedPointer<BandwidthRecorder> bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();
    connect(nodeList.data(), &LimitedNodeList::dataSent,
        bandwidthRecorder.data(), &BandwidthRecorder::updateOutboundData);
    connect(nodeList.data(), &LimitedNodeList::dataReceived,
        bandwidthRecorder.data(), &BandwidthRecorder::updateInboundData);

    // FIXME -- I'm a little concerned about this.
    connect(myAvatar->getSkeletonModel().get(), &SkeletonModel::skeletonLoaded,
        this, &Application::checkSkeleton, Qt::QueuedConnection);

    // Setup the userInputMapper with the actions
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    connect(userInputMapper.data(), &UserInputMapper::actionEvent, [this](int action, float state) {
        using namespace controller;
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
        {
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

            auto window = tabletScriptingInterface->getTabletWindow();
            if (navAxis && window) {
                if (lastKey != Qt::Key_unknown) {
                    QKeyEvent event(QEvent::KeyRelease, lastKey, Qt::NoModifier);
                    sendEvent(window, &event);
                    lastKey = Qt::Key_unknown;
                }

                if (key != Qt::Key_unknown) {
                    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
                    sendEvent(window, &event);
                    tabletScriptingInterface->processEvent(&event);
                    lastKey = key;
                }
            } else if (key != Qt::Key_unknown && window) {
                if (state) {
                    QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
                    sendEvent(window, &event);
                    tabletScriptingInterface->processEvent(&event);
                } else {
                    QKeyEvent event(QEvent::KeyRelease, key, Qt::NoModifier);
                    sendEvent(window, &event);
                }
                return;
            }
        }

        if (action == controller::toInt(controller::Action::RETICLE_CLICK)) {
            auto reticlePos = getApplicationCompositor().getReticlePosition();
            QPoint localPos(reticlePos.x, reticlePos.y); // both hmd and desktop already handle this in our coordinates.
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
                auto audioClient = DependencyManager::get<AudioClient>();
                audioClient->setMuted(!audioClient->isMuted());
            } else if (action == controller::toInt(controller::Action::CYCLE_CAMERA)) {
                cycleCamera();
            } else if (action == controller::toInt(controller::Action::CONTEXT_MENU) && !isInterstitialMode()) {
                toggleTabletUI();
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

    _applicationStateDevice = userInputMapper->getStateDevice();

    _applicationStateDevice->setInputVariant(STATE_IN_HMD, []() -> float {
        return qApp->isHMDMode() ? 1 : 0;
    });
    _applicationStateDevice->setInputVariant(STATE_CAMERA_FULL_SCREEN_MIRROR, []() -> float {
        return qApp->getCamera().getMode() == CAMERA_MODE_MIRROR ? 1 : 0;
    });
    _applicationStateDevice->setInputVariant(STATE_CAMERA_FIRST_PERSON, []() -> float {
        return qApp->getCamera().getMode() == CAMERA_MODE_FIRST_PERSON ? 1 : 0;
    });
    _applicationStateDevice->setInputVariant(STATE_CAMERA_THIRD_PERSON, []() -> float {
        return qApp->getCamera().getMode() == CAMERA_MODE_THIRD_PERSON ? 1 : 0;
    });
    _applicationStateDevice->setInputVariant(STATE_CAMERA_ENTITY, []() -> float {
        return qApp->getCamera().getMode() == CAMERA_MODE_ENTITY ? 1 : 0;
    });
    _applicationStateDevice->setInputVariant(STATE_CAMERA_INDEPENDENT, []() -> float {
        return qApp->getCamera().getMode() == CAMERA_MODE_INDEPENDENT ? 1 : 0;
    });
    _applicationStateDevice->setInputVariant(STATE_SNAP_TURN, []() -> float {
        return qApp->getMyAvatar()->getSnapTurn() ? 1 : 0;
    });
    _applicationStateDevice->setInputVariant(STATE_ADVANCED_MOVEMENT_CONTROLS, []() -> float {
        return qApp->getMyAvatar()->useAdvancedMovementControls() ? 1 : 0;
    });

    _applicationStateDevice->setInputVariant(STATE_GROUNDED, []() -> float {
        return qApp->getMyAvatar()->getCharacterController()->onGround() ? 1 : 0;
    });
    _applicationStateDevice->setInputVariant(STATE_NAV_FOCUSED, []() -> float {
        return DependencyManager::get<OffscreenUi>()->navigationFocused() ? 1 : 0;
    });
    _applicationStateDevice->setInputVariant(STATE_PLATFORM_WINDOWS, []() -> float {
#if defined(Q_OS_WIN) 
        return 1;
#else
        return 0;
#endif
    });
    _applicationStateDevice->setInputVariant(STATE_PLATFORM_MAC, []() -> float {
#if defined(Q_OS_MAC) 
        return 1;
#else
        return 0;
#endif
    });
    _applicationStateDevice->setInputVariant(STATE_PLATFORM_ANDROID, []() -> float {
#if defined(Q_OS_ANDROID) 
        return 1;
#else
        return 0;
#endif
    });

    // Setup the _keyboardMouseDevice, _touchscreenDevice, _touchscreenVirtualPadDevice and the user input mapper with the default bindings
    userInputMapper->registerDevice(_keyboardMouseDevice->getInputDevice());
    // if the _touchscreenDevice is not supported it will not be registered
    if (_touchscreenDevice) {
        userInputMapper->registerDevice(_touchscreenDevice->getInputDevice());
    }
    if (_touchscreenVirtualPadDevice) {
        userInputMapper->registerDevice(_touchscreenVirtualPadDevice->getInputDevice());
    }

    {
        auto scriptEngines = DependencyManager::get<ScriptEngines>().data();
        // this will force the model the look at the correct directory (weird order of operations issue)
        scriptEngines->reloadLocalFiles();

        // do this as late as possible so that all required subsystems are initialized
        // If we've overridden the default scripts location, just load default scripts
        // otherwise, load 'em all

        // we just want to see if --scripts was set, we've already parsed it and done
        // the change in PathUtils.  Rather than pass that in the constructor, lets just
        // look (this could be debated)
        QString scriptsSwitch = QString("--").append(SCRIPTS_SWITCH);
        QDir defaultScriptsLocation(getCmdOption(argc, constArgv, scriptsSwitch.toStdString().c_str()));
        if (!defaultScriptsLocation.exists()) {
            scriptEngines->loadDefaultScripts();
            scriptEngines->defaultScriptsLocationOverridden(true);
        } else {
            scriptEngines->loadScripts();
        }
    }

    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    loadSettings();

    updateVerboseLogging();

    // Now that we've loaded the menu and thus switched to the previous display plugin
    // we can unlock the desktop repositioning code, since all the positions will be
    // relative to the desktop size for this plugin
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->getDesktop()->setProperty("repositionLocked", false);

    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    QTimer* settingsTimer = new QTimer();
    moveToNewNamedThread(settingsTimer, "Settings Thread", [this, settingsTimer]{
        connect(qApp, &Application::beforeAboutToQuit, [this, settingsTimer]{
            // Disconnect the signal from the save settings
            QObject::disconnect(settingsTimer, &QTimer::timeout, this, &Application::saveSettings);
            // Stop the settings timer
            settingsTimer->stop();
            // Delete it (this will trigger the thread destruction
            settingsTimer->deleteLater();
            // Mark the settings thread as finished, so we know we can safely save in the main application
            // shutdown code
            _settingsGuard.trigger();
        });

        int SAVE_SETTINGS_INTERVAL = 10 * MSECS_PER_SECOND; // Let's save every seconds for now
        settingsTimer->setSingleShot(false);
        settingsTimer->setInterval(SAVE_SETTINGS_INTERVAL); // 10s, Qt::CoarseTimer acceptable
        QObject::connect(settingsTimer, &QTimer::timeout, this, &Application::saveSettings);
        settingsTimer->start();
    }, QThread::LowestPriority);

    if (Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson)) {
        getMyAvatar()->setBoomLength(MyAvatar::ZOOM_MIN);  // So that camera doesn't auto-switch to third person.
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::IndependentMode)) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
        cameraMenuChanged();
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::CameraEntityMode)) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
        cameraMenuChanged();
    }

    {
        auto audioIO = DependencyManager::get<AudioClient>().data();
        // set the local loopback interface for local sounds
        AudioInjector::setLocalAudioInterface(audioIO);
        auto audioScriptingInterface = DependencyManager::get<AudioScriptingInterface>();
        audioScriptingInterface->setLocalAudioInterface(audioIO);
        connect(audioIO, &AudioClient::noiseGateOpened, audioScriptingInterface.data(), &AudioScriptingInterface::noiseGateOpened);
        connect(audioIO, &AudioClient::noiseGateClosed, audioScriptingInterface.data(), &AudioScriptingInterface::noiseGateClosed);
        connect(audioIO, &AudioClient::inputReceived, audioScriptingInterface.data(), &AudioScriptingInterface::inputReceived);
    }

    this->installEventFilter(this);

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

    // If launched from Steam, let it handle updates
    const QString HIFI_NO_UPDATER_COMMAND_LINE_KEY = "--no-updater";
    bool noUpdater = arguments().indexOf(HIFI_NO_UPDATER_COMMAND_LINE_KEY) != -1;
    bool buildCanUpdate = BuildInfo::BUILD_TYPE == BuildInfo::BuildType::Stable
        || BuildInfo::BUILD_TYPE == BuildInfo::BuildType::Master;
    if (!noUpdater && buildCanUpdate) {
        constexpr auto INSTALLER_TYPE_CLIENT_ONLY = "client_only";

        auto applicationUpdater = DependencyManager::set<AutoUpdater>();

        AutoUpdater::InstallerType type = installerType == INSTALLER_TYPE_CLIENT_ONLY
            ? AutoUpdater::InstallerType::CLIENT_ONLY : AutoUpdater::InstallerType::FULL;

        applicationUpdater->setInstallerType(type);
        applicationUpdater->setInstallerCampaign(installerCampaign);
        connect(applicationUpdater.data(), &AutoUpdater::newVersionIsAvailable, dialogsManager.data(), &DialogsManager::showUpdateDialog);
        applicationUpdater->checkForUpdate();
    }

    Menu::getInstance()->setIsOptionChecked(MenuOption::ActionMotorControl, true);

// FIXME spacemouse code still needs cleanup
#if 0
    // the 3Dconnexion device wants to be initialized after a window is displayed.
    SpacemouseManager::getInstance().init();
#endif

    // If the user clicks an an entity, we will check that it's an unlocked web entity, and if so, set the focus to it
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    connect(entityScriptingInterface.data(), &EntityScriptingInterface::mousePressOnEntity,
            [this](const EntityItemID& entityItemID, const PointerEvent& event) {
        if (event.shouldFocus()) {
            if (getEntities()->wantsKeyboardFocus(entityItemID)) {
                setKeyboardFocusOverlay(UNKNOWN_OVERLAY_ID);
                setKeyboardFocusEntity(entityItemID);
            } else {
                setKeyboardFocusEntity(UNKNOWN_ENTITY_ID);
            }
        }
    });

    connect(entityScriptingInterface.data(), &EntityScriptingInterface::deletingEntity, [this](const EntityItemID& entityItemID) {
        if (entityItemID == _keyboardFocusedEntity.get()) {
            setKeyboardFocusEntity(UNKNOWN_ENTITY_ID);
        }
    });

    EntityTree::setAddMaterialToEntityOperator([this](const QUuid& entityID, graphics::MaterialLayer material, const std::string& parentMaterialName) {
        if (_aboutToQuit) {
            return false;
        }

        // try to find the renderable
        auto renderable = getEntities()->renderableForEntityId(entityID);
        if (renderable) {
            renderable->addMaterial(material, parentMaterialName);
        }

        // even if we don't find it, try to find the entity
        auto entity = getEntities()->getEntity(entityID);
        if (entity) {
            entity->addMaterial(material, parentMaterialName);
            return true;
        }
        return false;
    });
    EntityTree::setRemoveMaterialFromEntityOperator([this](const QUuid& entityID, graphics::MaterialPointer material, const std::string& parentMaterialName) {
        if (_aboutToQuit) {
            return false;
        }

        // try to find the renderable
        auto renderable = getEntities()->renderableForEntityId(entityID);
        if (renderable) {
            renderable->removeMaterial(material, parentMaterialName);
        }

        // even if we don't find it, try to find the entity
        auto entity = getEntities()->getEntity(entityID);
        if (entity) {
            entity->removeMaterial(material, parentMaterialName);
            return true;
        }
        return false;
    });

    EntityTree::setAddMaterialToAvatarOperator([](const QUuid& avatarID, graphics::MaterialLayer material, const std::string& parentMaterialName) {
        auto avatarManager = DependencyManager::get<AvatarManager>();
        auto avatar = avatarManager->getAvatarBySessionID(avatarID);
        if (avatar) {
            avatar->addMaterial(material, parentMaterialName);
            return true;
        }
        return false;
    });
    EntityTree::setRemoveMaterialFromAvatarOperator([](const QUuid& avatarID, graphics::MaterialPointer material, const std::string& parentMaterialName) {
        auto avatarManager = DependencyManager::get<AvatarManager>();
        auto avatar = avatarManager->getAvatarBySessionID(avatarID);
        if (avatar) {
            avatar->removeMaterial(material, parentMaterialName);
            return true;
        }
        return false;
    });

    EntityTree::setAddMaterialToOverlayOperator([this](const QUuid& overlayID, graphics::MaterialLayer material, const std::string& parentMaterialName) {
        auto overlay = _overlays.getOverlay(overlayID);
        if (overlay) {
            overlay->addMaterial(material, parentMaterialName);
            return true;
        }
        return false;
    });
    EntityTree::setRemoveMaterialFromOverlayOperator([this](const QUuid& overlayID, graphics::MaterialPointer material, const std::string& parentMaterialName) {
        auto overlay = _overlays.getOverlay(overlayID);
        if (overlay) {
            overlay->removeMaterial(material, parentMaterialName);
            return true;
        }
        return false;
    });

    // Keyboard focus handling for Web overlays.
    auto overlays = &(qApp->getOverlays());
    connect(overlays, &Overlays::overlayDeleted, [this](const OverlayID& overlayID) {
        if (overlayID == _keyboardFocusedOverlay.get()) {
            setKeyboardFocusOverlay(UNKNOWN_OVERLAY_ID);
        }
    });

    connect(this, &Application::aboutToQuit, [this]() {
        setKeyboardFocusOverlay(UNKNOWN_OVERLAY_ID);
        setKeyboardFocusEntity(UNKNOWN_ENTITY_ID);
    });

    // Add periodic checks to send user activity data
    static int CHECK_NEARBY_AVATARS_INTERVAL_MS = 10000;
    static int NEARBY_AVATAR_RADIUS_METERS = 10;

    // setup the stats interval depending on if the 1s faster hearbeat was requested
    static const QString FAST_STATS_ARG = "--fast-heartbeat";
    static int SEND_STATS_INTERVAL_MS = arguments().indexOf(FAST_STATS_ARG) != -1 ? 1000 : 10000;

    static glm::vec3 lastAvatarPosition = myAvatar->getWorldPosition();
    static glm::mat4 lastHMDHeadPose = getHMDSensorPose();
    static controller::Pose lastLeftHandPose = myAvatar->getLeftHandPose();
    static controller::Pose lastRightHandPose = myAvatar->getRightHandPose();

    // Periodically send fps as a user activity event
    QTimer* sendStatsTimer = new QTimer(this);
    sendStatsTimer->setInterval(SEND_STATS_INTERVAL_MS);  // 10s, Qt::CoarseTimer acceptable
    connect(sendStatsTimer, &QTimer::timeout, this, [this]() {

        QJsonObject properties = {};
        MemoryInfo memInfo;
        if (getMemoryInfo(memInfo)) {
            properties["system_memory_total"] = static_cast<qint64>(memInfo.totalMemoryBytes);
            properties["system_memory_used"] = static_cast<qint64>(memInfo.usedMemoryBytes);
            properties["process_memory_used"] = static_cast<qint64>(memInfo.processUsedMemoryBytes);
        }

        // content location and build info - useful for filtering stats
        auto addressManager = DependencyManager::get<AddressManager>();
        auto currentDomain = addressManager->currentShareableAddress(true).toString(); // domain only
        auto currentPath = addressManager->currentPath(true); // with orientation
        properties["current_domain"] = currentDomain;
        properties["current_path"] = currentPath;
        properties["build_version"] = BuildInfo::VERSION;

        auto displayPlugin = qApp->getActiveDisplayPlugin();

        properties["render_rate"] = _renderLoopCounter.rate();
        properties["target_render_rate"] = getTargetRenderFrameRate();
        properties["present_rate"] = displayPlugin->presentRate();
        properties["new_frame_present_rate"] = displayPlugin->newFramePresentRate();
        properties["dropped_frame_rate"] = displayPlugin->droppedFrameRate();
        properties["stutter_rate"] = displayPlugin->stutterRate();
        properties["game_rate"] = getGameLoopRate();
        properties["has_async_reprojection"] = displayPlugin->hasAsyncReprojection();
        properties["hardware_stats"] = displayPlugin->getHardwareStats();

        // deadlock watchdog related stats
        properties["deadlock_watchdog_maxElapsed"] = (int)DeadlockWatchdogThread::_maxElapsed;
        properties["deadlock_watchdog_maxElapsedAverage"] = (int)DeadlockWatchdogThread::_maxElapsedAverage;

        auto bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();
        properties["packet_rate_in"] = bandwidthRecorder->getCachedTotalAverageInputPacketsPerSecond();
        properties["packet_rate_out"] = bandwidthRecorder->getCachedTotalAverageOutputPacketsPerSecond();
        properties["kbps_in"] = bandwidthRecorder->getCachedTotalAverageInputKilobitsPerSecond();
        properties["kbps_out"] = bandwidthRecorder->getCachedTotalAverageOutputKilobitsPerSecond();

        properties["atp_in_kbps"] = bandwidthRecorder->getAverageInputKilobitsPerSecond(NodeType::AssetServer);

        auto nodeList = DependencyManager::get<NodeList>();
        SharedNodePointer entityServerNode = nodeList->soloNodeOfType(NodeType::EntityServer);
        SharedNodePointer audioMixerNode = nodeList->soloNodeOfType(NodeType::AudioMixer);
        SharedNodePointer avatarMixerNode = nodeList->soloNodeOfType(NodeType::AvatarMixer);
        SharedNodePointer assetServerNode = nodeList->soloNodeOfType(NodeType::AssetServer);
        SharedNodePointer messagesMixerNode = nodeList->soloNodeOfType(NodeType::MessagesMixer);
        properties["entity_ping"] = entityServerNode ? entityServerNode->getPingMs() : -1;
        properties["audio_ping"] = audioMixerNode ? audioMixerNode->getPingMs() : -1;
        properties["avatar_ping"] = avatarMixerNode ? avatarMixerNode->getPingMs() : -1;
        properties["asset_ping"] = assetServerNode ? assetServerNode->getPingMs() : -1;
        properties["messages_ping"] = messagesMixerNode ? messagesMixerNode->getPingMs() : -1;

        auto loadingRequests = ResourceCache::getLoadingRequests();

        QJsonArray loadingRequestsStats;
        for (const auto& request : loadingRequests) {
            QJsonObject requestStats;
            requestStats["filename"] = request->getURL().fileName();
            requestStats["received"] = request->getBytesReceived();
            requestStats["total"] = request->getBytesTotal();
            requestStats["attempts"] = (int)request->getDownloadAttempts();
            loadingRequestsStats.append(requestStats);
        }

        properties["active_downloads"] = loadingRequests.size();
        properties["pending_downloads"] = ResourceCache::getPendingRequestCount();
        properties["active_downloads_details"] = loadingRequestsStats;

        auto statTracker = DependencyManager::get<StatTracker>();

        properties["processing_resources"] = statTracker->getStat("Processing").toInt();
        properties["pending_processing_resources"] = statTracker->getStat("PendingProcessing").toInt();

        QJsonObject startedRequests;
        startedRequests["atp"] = statTracker->getStat(STAT_ATP_REQUEST_STARTED).toInt();
        startedRequests["http"] = statTracker->getStat(STAT_HTTP_REQUEST_STARTED).toInt();
        startedRequests["file"] = statTracker->getStat(STAT_FILE_REQUEST_STARTED).toInt();
        startedRequests["total"] = startedRequests["atp"].toInt() + startedRequests["http"].toInt()
            + startedRequests["file"].toInt();
        properties["started_requests"] = startedRequests;

        QJsonObject successfulRequests;
        successfulRequests["atp"] = statTracker->getStat(STAT_ATP_REQUEST_SUCCESS).toInt();
        successfulRequests["http"] = statTracker->getStat(STAT_HTTP_REQUEST_SUCCESS).toInt();
        successfulRequests["file"] = statTracker->getStat(STAT_FILE_REQUEST_SUCCESS).toInt();
        successfulRequests["total"] = successfulRequests["atp"].toInt() + successfulRequests["http"].toInt()
            + successfulRequests["file"].toInt();
        properties["successful_requests"] = successfulRequests;

        QJsonObject failedRequests;
        failedRequests["atp"] = statTracker->getStat(STAT_ATP_REQUEST_FAILED).toInt();
        failedRequests["http"] = statTracker->getStat(STAT_HTTP_REQUEST_FAILED).toInt();
        failedRequests["file"] = statTracker->getStat(STAT_FILE_REQUEST_FAILED).toInt();
        failedRequests["total"] = failedRequests["atp"].toInt() + failedRequests["http"].toInt()
            + failedRequests["file"].toInt();
        properties["failed_requests"] = failedRequests;

        QJsonObject cacheRequests;
        cacheRequests["atp"] = statTracker->getStat(STAT_ATP_REQUEST_CACHE).toInt();
        cacheRequests["http"] = statTracker->getStat(STAT_HTTP_REQUEST_CACHE).toInt();
        cacheRequests["total"] = cacheRequests["atp"].toInt() + cacheRequests["http"].toInt();
        properties["cache_requests"] = cacheRequests;

        QJsonObject atpMappingRequests;
        atpMappingRequests["started"] = statTracker->getStat(STAT_ATP_MAPPING_REQUEST_STARTED).toInt();
        atpMappingRequests["failed"] = statTracker->getStat(STAT_ATP_MAPPING_REQUEST_FAILED).toInt();
        atpMappingRequests["successful"] = statTracker->getStat(STAT_ATP_MAPPING_REQUEST_SUCCESS).toInt();
        properties["atp_mapping_requests"] = atpMappingRequests;

        properties["throttled"] = _displayPlugin ? _displayPlugin->isThrottled() : false;

        QJsonObject bytesDownloaded;
        auto atpBytes = statTracker->getStat(STAT_ATP_RESOURCE_TOTAL_BYTES).toLongLong();
        auto httpBytes = statTracker->getStat(STAT_HTTP_RESOURCE_TOTAL_BYTES).toLongLong();
        auto fileBytes = statTracker->getStat(STAT_FILE_RESOURCE_TOTAL_BYTES).toLongLong();
        bytesDownloaded["atp"] = atpBytes;
        bytesDownloaded["http"] = httpBytes;
        bytesDownloaded["file"] = fileBytes;
        bytesDownloaded["total"] = atpBytes + httpBytes + fileBytes;
        properties["bytes_downloaded"] = bytesDownloaded;

        auto myAvatar = getMyAvatar();
        glm::vec3 avatarPosition = myAvatar->getWorldPosition();
        properties["avatar_has_moved"] = lastAvatarPosition != avatarPosition;
        lastAvatarPosition = avatarPosition;

        auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
        auto entityActivityTracking = entityScriptingInterface->getActivityTracking();
        entityScriptingInterface->resetActivityTracking();
        properties["added_entity_cnt"] = entityActivityTracking.addedEntityCount;
        properties["deleted_entity_cnt"] = entityActivityTracking.deletedEntityCount;
        properties["edited_entity_cnt"] = entityActivityTracking.editedEntityCount;

        NodeToOctreeSceneStats* octreeServerSceneStats = getOcteeSceneStats();
        unsigned long totalServerOctreeElements = 0;
        for (NodeToOctreeSceneStatsIterator i = octreeServerSceneStats->begin(); i != octreeServerSceneStats->end(); i++) {
            totalServerOctreeElements += i->second.getTotalElements();
        }

        properties["local_octree_elements"] = (qint64) OctreeElement::getInternalNodeCount();
        properties["server_octree_elements"] = (qint64) totalServerOctreeElements;

        properties["active_display_plugin"] = getActiveDisplayPlugin()->getName();
        properties["using_hmd"] = isHMDMode();

        _autoSwitchDisplayModeSupportedHMDPlugin = nullptr;
        foreach(DisplayPluginPointer displayPlugin, PluginManager::getInstance()->getDisplayPlugins()) {
            if (displayPlugin->isHmd() &&
                displayPlugin->getSupportsAutoSwitch()) {
                _autoSwitchDisplayModeSupportedHMDPlugin = displayPlugin;
                _autoSwitchDisplayModeSupportedHMDPluginName =
                    _autoSwitchDisplayModeSupportedHMDPlugin->getName();
                _previousHMDWornStatus =
                    _autoSwitchDisplayModeSupportedHMDPlugin->isDisplayVisible();
                break;
            }
        }

        if (_autoSwitchDisplayModeSupportedHMDPlugin) {
            if (getActiveDisplayPlugin() != _autoSwitchDisplayModeSupportedHMDPlugin &&
                !_autoSwitchDisplayModeSupportedHMDPlugin->isSessionActive()) {
                    startHMDStandBySession();
            }
            // Poll periodically to check whether the user has worn HMD or not. Switch Display mode accordingly.
            // If the user wears HMD then switch to VR mode. If the user removes HMD then switch to Desktop mode.
            QTimer* autoSwitchDisplayModeTimer = new QTimer(this);
            connect(autoSwitchDisplayModeTimer, SIGNAL(timeout()), this, SLOT(switchDisplayMode()));
            autoSwitchDisplayModeTimer->start(INTERVAL_TO_CHECK_HMD_WORN_STATUS);
        }

        auto glInfo = getGLContextData();
        properties["gl_info"] = glInfo;
        properties["gpu_used_memory"] = (int)BYTES_TO_MB(gpu::Context::getUsedGPUMemSize());
        properties["gpu_free_memory"] = (int)BYTES_TO_MB(gpu::Context::getFreeGPUMemSize());
        properties["gpu_frame_time"] = (float)(qApp->getGPUContext()->getFrameTimerGPUAverage());
        properties["batch_frame_time"] = (float)(qApp->getGPUContext()->getFrameTimerBatchAverage());
        properties["ideal_thread_count"] = QThread::idealThreadCount();

        auto hmdHeadPose = getHMDSensorPose();
        properties["hmd_head_pose_changed"] = isHMDMode() && (hmdHeadPose != lastHMDHeadPose);
        lastHMDHeadPose = hmdHeadPose;

        auto leftHandPose = myAvatar->getLeftHandPose();
        auto rightHandPose = myAvatar->getRightHandPose();
        // controller::Pose considers two poses to be different if either are invalid. In our case, we actually
        // want to consider the pose to be unchanged if it was invalid and still is invalid, so we check that first.
        properties["hand_pose_changed"] =
            ((leftHandPose.valid || lastLeftHandPose.valid) && (leftHandPose != lastLeftHandPose))
            || ((rightHandPose.valid || lastRightHandPose.valid) && (rightHandPose != lastRightHandPose));
        lastLeftHandPose = leftHandPose;
        lastRightHandPose = rightHandPose;
        properties["avatar_identity_requests_sent"] = DependencyManager::get<AvatarManager>()->getIdentityRequestsSent();

        UserActivityLogger::getInstance().logAction("stats", properties);
    });
    sendStatsTimer->start();

    // Periodically check for count of nearby avatars
    static int lastCountOfNearbyAvatars = -1;
    QTimer* checkNearbyAvatarsTimer = new QTimer(this);
    checkNearbyAvatarsTimer->setInterval(CHECK_NEARBY_AVATARS_INTERVAL_MS); // 10 seconds, Qt::CoarseTimer ok
    connect(checkNearbyAvatarsTimer, &QTimer::timeout, this, []() {
        auto avatarManager = DependencyManager::get<AvatarManager>();
        int nearbyAvatars = avatarManager->numberOfAvatarsInRange(avatarManager->getMyAvatar()->getWorldPosition(),
                                                                  NEARBY_AVATAR_RADIUS_METERS) - 1;
        if (nearbyAvatars != lastCountOfNearbyAvatars) {
            lastCountOfNearbyAvatars = nearbyAvatars;
            UserActivityLogger::getInstance().logAction("nearby_avatars", { { "count", nearbyAvatars } });
        }
    });
    checkNearbyAvatarsTimer->start();

    // Track user activity event when we receive a mute packet
    auto onMutedByMixer = []() {
        UserActivityLogger::getInstance().logAction("received_mute_packet");
    };
    connect(DependencyManager::get<AudioClient>().data(), &AudioClient::mutedByMixer, this, onMutedByMixer);

    // Track when the address bar is opened
    auto onAddressBarShown = [this]() {
        // Record time
        UserActivityLogger::getInstance().logAction("opened_address_bar", { { "uptime_ms", _sessionRunTimer.elapsed() } });
    };
    connect(DependencyManager::get<DialogsManager>().data(), &DialogsManager::addressBarShown, this, onAddressBarShown);

    // Make sure we don't time out during slow operations at startup
    updateHeartbeat();

    OctreeEditPacketSender* packetSender = entityScriptingInterface->getPacketSender();
    EntityEditPacketSender* entityPacketSender = static_cast<EntityEditPacketSender*>(packetSender);
    entityPacketSender->setMyAvatar(myAvatar.get());

    connect(this, &Application::applicationStateChanged, this, &Application::activeChanged);
    connect(_window, SIGNAL(windowMinimizedChanged(bool)), this, SLOT(windowMinimizedChanged(bool)));
    qCDebug(interfaceapp, "Startup time: %4.2f seconds.", (double)startupTimer.elapsed() / 1000.0);

    EntityTreeRenderer::setEntitiesShouldFadeFunction([this]() {
        SharedNodePointer entityServerNode = DependencyManager::get<NodeList>()->soloNodeOfType(NodeType::EntityServer);
        return entityServerNode && !isPhysicsEnabled();
    });

    _snapshotSound = DependencyManager::get<SoundCache>()->getSound(PathUtils::resourcesUrl("sounds/snapshot/snap.wav"));

    QVariant testProperty = property(hifi::properties::TEST);
    qDebug() << testProperty;
    if (testProperty.isValid()) {
        const auto testScript = property(hifi::properties::TEST).toUrl();

        // Set last parameter to exit interface when the test script finishes, if so requested
        DependencyManager::get<ScriptEngines>()->loadScript(testScript, false, false, false, false, quitWhenFinished);

        // This is done so we don't get a "connection time-out" message when we haven't passed in a URL.
        if (arguments().contains("--url")) {
            auto reply = SandboxUtils::getStatus();
            connect(reply, &QNetworkReply::finished, this, [this, reply] {
                handleSandboxStatus(reply);
            });
        }
    } else {
        PROFILE_RANGE(render, "GetSandboxStatus");
        auto reply = SandboxUtils::getStatus();
        connect(reply, &QNetworkReply::finished, this, [this, reply] {
            handleSandboxStatus(reply);
        });
    }

    // Monitor model assets (e.g., from Clara.io) added to the world that may need resizing.
    static const int ADD_ASSET_TO_WORLD_TIMER_INTERVAL_MS = 1000;
    _addAssetToWorldResizeTimer.setInterval(ADD_ASSET_TO_WORLD_TIMER_INTERVAL_MS); // 1s, Qt::CoarseTimer acceptable
    connect(&_addAssetToWorldResizeTimer, &QTimer::timeout, this, &Application::addAssetToWorldCheckModelSize);

    // Auto-update and close adding asset to world info message box.
    static const int ADD_ASSET_TO_WORLD_INFO_TIMEOUT_MS = 5000;
    _addAssetToWorldInfoTimer.setInterval(ADD_ASSET_TO_WORLD_INFO_TIMEOUT_MS); // 5s, Qt::CoarseTimer acceptable
    _addAssetToWorldInfoTimer.setSingleShot(true);
    connect(&_addAssetToWorldInfoTimer, &QTimer::timeout, this, &Application::addAssetToWorldInfoTimeout);
    static const int ADD_ASSET_TO_WORLD_ERROR_TIMEOUT_MS = 8000;
    _addAssetToWorldErrorTimer.setInterval(ADD_ASSET_TO_WORLD_ERROR_TIMEOUT_MS); // 8s, Qt::CoarseTimer acceptable
    _addAssetToWorldErrorTimer.setSingleShot(true);
    connect(&_addAssetToWorldErrorTimer, &QTimer::timeout, this, &Application::addAssetToWorldErrorTimeout);

    connect(this, &QCoreApplication::aboutToQuit, this, &Application::addAssetToWorldMessageClose);
    connect(&domainHandler, &DomainHandler::domainURLChanged, this, &Application::addAssetToWorldMessageClose);
    connect(&domainHandler, &DomainHandler::redirectToErrorDomainURL, this, &Application::addAssetToWorldMessageClose);

    updateSystemTabletMode();

    connect(&_myCamera, &Camera::modeUpdated, this, &Application::cameraModeChanged);

    DependencyManager::get<PickManager>()->setShouldPickHUDOperator([]() { return DependencyManager::get<HMDScriptingInterface>()->isHMDMode(); });
    DependencyManager::get<PickManager>()->setCalculatePos2DFromHUDOperator([this](const glm::vec3& intersection) {
        const glm::vec2 MARGIN(25.0f);
        glm::vec2 maxPos = _controllerScriptingInterface->getViewportDimensions() - MARGIN;
        glm::vec2 pos2D = DependencyManager::get<HMDScriptingInterface>()->overlayFromWorldPoint(intersection);
        return glm::max(MARGIN, glm::min(pos2D, maxPos));
    });

    // Setup the mouse ray pick and related operators
    DependencyManager::get<EntityTreeRenderer>()->setMouseRayPickID(DependencyManager::get<PickManager>()->addPick(PickQuery::Ray, std::make_shared<MouseRayPick>(
        PickFilter(PickScriptingInterface::PICK_ENTITIES() | PickScriptingInterface::PICK_INCLUDE_NONCOLLIDABLE()), 0.0f, true)));
    DependencyManager::get<EntityTreeRenderer>()->setMouseRayPickResultOperator([](unsigned int rayPickID) {
        RayToEntityIntersectionResult entityResult;
        entityResult.intersects = false;
        auto pickResult = DependencyManager::get<PickManager>()->getPrevPickResultTyped<RayPickResult>(rayPickID);
        if (pickResult) {
            entityResult.intersects = pickResult->type != IntersectionType::NONE;
            if (entityResult.intersects) {
                entityResult.intersection = pickResult->intersection;
                entityResult.distance = pickResult->distance;
                entityResult.surfaceNormal = pickResult->surfaceNormal;
                entityResult.entityID = pickResult->objectID;
                entityResult.extraInfo = pickResult->extraInfo;
            }
        }
        return entityResult;
    });
    DependencyManager::get<EntityTreeRenderer>()->setSetPrecisionPickingOperator([](unsigned int rayPickID, bool value) {
        DependencyManager::get<PickManager>()->setPrecisionPicking(rayPickID, value);
    });

    // Preload Tablet sounds
    DependencyManager::get<TabletScriptingInterface>()->preloadSounds();

    _pendingIdleEvent = false;
    _pendingRenderEvent = false;

    qCDebug(interfaceapp) << "Metaverse session ID is" << uuidStringWithoutCurlyBraces(accountManager->getSessionID());

#if defined(Q_OS_ANDROID)
    connect(&AndroidHelper::instance(), &AndroidHelper::beforeEnterBackground, this, &Application::beforeEnterBackground);
    connect(&AndroidHelper::instance(), &AndroidHelper::enterBackground, this, &Application::enterBackground);
    connect(&AndroidHelper::instance(), &AndroidHelper::enterForeground, this, &Application::enterForeground);
    AndroidHelper::instance().notifyLoadComplete();
#else
    static int CHECK_LOGIN_TIMER = 3000;
    QTimer* checkLoginTimer = new QTimer(this);
    checkLoginTimer->setInterval(CHECK_LOGIN_TIMER);
    checkLoginTimer->setSingleShot(true);
    connect(checkLoginTimer, &QTimer::timeout, this, [this]() {
        auto accountManager = DependencyManager::get<AccountManager>();
        auto dialogsManager = DependencyManager::get<DialogsManager>();
        if (!accountManager->isLoggedIn()) {
            Setting::Handle<bool>{"loginDialogPoppedUp", false}.set(true);
            dialogsManager->showLoginDialog();
            QJsonObject loginData = {};
            loginData["action"] = "login dialog shown";
            UserActivityLogger::getInstance().logAction("encourageLoginDialog", loginData);
        }
    });
    Setting::Handle<bool>{"loginDialogPoppedUp", false}.set(false);
    checkLoginTimer->start();
#endif
}

void Application::updateVerboseLogging() {
    auto menu = Menu::getInstance();
    if (!menu) {
        return;
    }
    bool enable = menu->isOptionChecked(MenuOption::VerboseLogging);

    QString rules =
        "hifi.*.debug=%1\n"
        "hifi.*.info=%1\n"
        "hifi.audio-stream.debug=false\n"
        "hifi.audio-stream.info=false";
    rules = rules.arg(enable ? "true" : "false");
    QLoggingCategory::setFilterRules(rules);
}

void Application::domainConnectionRefused(const QString& reasonMessage, int reasonCodeInt, const QString& extraInfo) {
    DomainHandler::ConnectionRefusedReason reasonCode = static_cast<DomainHandler::ConnectionRefusedReason>(reasonCodeInt);

    if (reasonCode == DomainHandler::ConnectionRefusedReason::TooManyUsers && !extraInfo.isEmpty()) {
        DependencyManager::get<AddressManager>()->handleLookupString(extraInfo);
        return;
    }

    switch (reasonCode) {
        case DomainHandler::ConnectionRefusedReason::ProtocolMismatch:
        case DomainHandler::ConnectionRefusedReason::TooManyUsers:
        case DomainHandler::ConnectionRefusedReason::Unknown: {
            QString message = "Unable to connect to the location you are visiting.\n";
            message += reasonMessage;
            OffscreenUi::asyncWarning("", message);
            getMyAvatar()->setWorldVelocity(glm::vec3(0.0f));
            break;
        }
        default:
            // nothing to do.
            break;
    }
}

QString Application::getUserAgent() {
    if (QThread::currentThread() != thread()) {
        QString userAgent;

        BLOCKING_INVOKE_METHOD(this, "getUserAgent", Q_RETURN_ARG(QString, userAgent));

        return userAgent;
    }

    QString userAgent = "Mozilla/5.0 (HighFidelityInterface/" + BuildInfo::VERSION + "; "
        + QSysInfo::productType() + " " + QSysInfo::productVersion() + ")";

    auto formatPluginName = [](QString name) -> QString { return name.trimmed().replace(" ", "-");  };

    // For each plugin, add to userAgent
    auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();
    for (auto& dp : displayPlugins) {
        if (dp->isActive() && dp->isHmd()) {
            userAgent += " " + formatPluginName(dp->getName());
        }
    }
    auto inputPlugins= PluginManager::getInstance()->getInputPlugins();
    for (auto& ip : inputPlugins) {
        if (ip->isActive()) {
            userAgent += " " + formatPluginName(ip->getName());
        }
    }
    // for codecs, we include all of them, even if not active
    auto codecPlugins = PluginManager::getInstance()->getCodecPlugins();
    for (auto& cp : codecPlugins) {
        userAgent += " " + formatPluginName(cp->getName());
    }

    return userAgent;
}

void Application::toggleTabletUI(bool shouldOpen) const {
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto hmd = DependencyManager::get<HMDScriptingInterface>();
    if (!(shouldOpen && hmd->getShouldShowTablet())) {
        auto HMD = DependencyManager::get<HMDScriptingInterface>();
        HMD->toggleShouldShowTablet();
    }
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

void Application::showCursor(const Cursor::Icon& cursor) {
    QMutexLocker locker(&_changeCursorLock);

    auto managedCursor = Cursor::Manager::instance().getCursor();
    auto curIcon = managedCursor->getIcon();
    if (curIcon != cursor) {
        managedCursor->setIcon(cursor);
        curIcon = cursor;
    }
    _desiredCursor = cursor == Cursor::Icon::SYSTEM ? Qt::ArrowCursor : Qt::BlankCursor;
    _cursorNeedsChanging = true;
}

void Application::updateHeartbeat() const {
    DeadlockWatchdogThread::updateHeartbeat();
}

void Application::onAboutToQuit() {
    emit beforeAboutToQuit();

    foreach(auto inputPlugin, PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->isActive()) {
            inputPlugin->deactivate();
        }
    }

    // The active display plugin needs to be loaded before the menu system is active,
    // so its persisted explicitly here
    Setting::Handle<QString>{ ACTIVE_DISPLAY_PLUGIN_SETTING_NAME }.set(getActiveDisplayPlugin()->getName());

    Setting::Handle<bool>{"loginDialogPoppedUp", false}.set(false);

    getActiveDisplayPlugin()->deactivate();
    if (_autoSwitchDisplayModeSupportedHMDPlugin
        && _autoSwitchDisplayModeSupportedHMDPlugin->isSessionActive()) {
        _autoSwitchDisplayModeSupportedHMDPlugin->endSession();
    }
    // use the CloseEventSender via a QThread to send an event that says the user asked for the app to close
    DependencyManager::get<CloseEventSender>()->startThread();

    // Hide Running Scripts dialog so that it gets destroyed in an orderly manner; prevents warnings at shutdown.
    DependencyManager::get<OffscreenUi>()->hide("RunningScripts");

    _aboutToQuit = true;

    cleanupBeforeQuit();
}

void Application::cleanupBeforeQuit() {
    // add a logline indicating if QTWEBENGINE_REMOTE_DEBUGGING is set or not
    QString webengineRemoteDebugging = QProcessEnvironment::systemEnvironment().value("QTWEBENGINE_REMOTE_DEBUGGING", "false");
    qCDebug(interfaceapp) << "QTWEBENGINE_REMOTE_DEBUGGING =" << webengineRemoteDebugging;

    if (tracing::enabled()) {
        auto tracer = DependencyManager::get<tracing::Tracer>();
        tracer->stopTracing();
        auto outputFile = property(hifi::properties::TRACING).toString();
        tracer->serialize(outputFile);
    }

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

    {
        if (_keyboardFocusHighlightID != UNKNOWN_OVERLAY_ID) {
            getOverlays().deleteOverlay(_keyboardFocusHighlightID);
            _keyboardFocusHighlightID = UNKNOWN_OVERLAY_ID;
        }
        _keyboardFocusHighlight = nullptr;
    }

    {
        auto nodeList = DependencyManager::get<NodeList>();

        // send the domain a disconnect packet, force stoppage of domain-server check-ins
        nodeList->getDomainHandler().disconnect();
        nodeList->setIsShuttingDown(true);

        // tell the packet receiver we're shutting down, so it can drop packets
        nodeList->getPacketReceiver().setShouldDropPackets(true);
    }

    getEntities()->shutdown(); // tell the entities system we're shutting down, so it will stop running scripts

    // Clear any queued processing (I/O, FBX/OBJ/Texture parsing)
    QThreadPool::globalInstance()->clear();

    DependencyManager::destroy<RecordingScriptingInterface>();

    // FIXME: Something is still holding on to the ScriptEnginePointers contained in ScriptEngines, and they hold backpointers to ScriptEngines,
    // so this doesn't shut down properly
    DependencyManager::get<ScriptEngines>()->shutdownScripting(); // stop all currently running global scripts
    // These classes hold ScriptEnginePointers, so they must be destroyed before ScriptEngines
    // Must be done after shutdownScripting in case any scripts try to access these things
    {
        DependencyManager::destroy<StandAloneJSConsole>();
        EntityTreePointer tree = getEntities()->getTree();
        tree->setSimulation(nullptr);
        DependencyManager::destroy<EntityTreeRenderer>();
    }
    DependencyManager::destroy<ScriptEngines>();

    bool autoLogout = Setting::Handle<bool>(AUTO_LOGOUT_SETTING_NAME, false).get();
    if (autoLogout) {
        DependencyManager::get<AccountManager>()->removeAccountFromFile();
    }

    _displayPlugin.reset();
    PluginManager::getInstance()->shutdown();

    // Cleanup all overlays after the scripts, as scripts might add more
    _overlays.cleanupAllOverlays();
    // The cleanup process enqueues the transactions but does not process them.  Calling this here will force the actual
    // removal of the items.
    // See https://highfidelity.fogbugz.com/f/cases/5328
    _main3DScene->enqueueFrame(); // flush all the transactions
    _main3DScene->processTransactionQueue(); // process and apply deletions

    // first stop all timers directly or by invokeMethod
    // depending on what thread they run in
    locationUpdateTimer.stop();
    identityPacketTimer.stop();
    pingTimer.stop();

    // Wait for the settings thread to shut down, and save the settings one last time when it's safe
    if (_settingsGuard.wait()) {
        // save state
        saveSettings();
    }

    _window->saveGeometry();
    _gpuContext->shutdown();

    // Destroy third party processes after scripts have finished using them.
#ifdef HAVE_DDE
    DependencyManager::destroy<DdeFaceTracker>();
#endif
#ifdef HAVE_IVIEWHMD
    DependencyManager::destroy<EyeTracker>();
#endif

    DependencyManager::destroy<ContextOverlayInterface>(); // Must be destroyed before TabletScriptingInterface

    // stop QML
    DependencyManager::destroy<TabletScriptingInterface>();
    DependencyManager::destroy<ToolbarScriptingInterface>();
    DependencyManager::destroy<OffscreenUi>();

    DependencyManager::destroy<OffscreenQmlSurfaceCache>();

    if (_snapshotSoundInjector != nullptr) {
        _snapshotSoundInjector->stop();
    }

    // destroy Audio so it and its threads have a chance to go down safely
    // this must happen after QML, as there are unexplained audio crashes originating in qtwebengine
    DependencyManager::destroy<AudioClient>();
    DependencyManager::destroy<AudioInjectorManager>();
    DependencyManager::destroy<AudioScriptingInterface>();

    // The PointerManager must be destroyed before the PickManager because when a Pointer is deleted,
    // it accesses the PickManager to delete its associated Pick
    DependencyManager::destroy<PointerManager>();
    DependencyManager::destroy<PickManager>();

    qCDebug(interfaceapp) << "Application::cleanupBeforeQuit() complete";
}

Application::~Application() {
    // remove avatars from physics engine
    auto avatarManager = DependencyManager::get<AvatarManager>();
    avatarManager->clearOtherAvatars();

    PhysicsEngine::Transaction transaction;
    avatarManager->buildPhysicsTransaction(transaction);
    _physicsEngine->processTransaction(transaction);
    avatarManager->handleProcessedPhysicsTransaction(transaction);

    avatarManager->deleteAllAvatars();

    _physicsEngine->setCharacterController(nullptr);

    // the _shapeManager should have zero references
    _shapeManager.collectGarbage();
    assert(_shapeManager.getNumShapes() == 0);

    // shutdown render engine
    _main3DScene = nullptr;
    _renderEngine = nullptr;

    _gameWorkload.shutdown();

    DependencyManager::destroy<Preferences>();

    _entityClipboard->eraseAllOctreeElements();
    _entityClipboard.reset();

    _octreeProcessor.terminate();
    _entityEditSender.terminate();

    if (auto steamClient = PluginManager::getInstance()->getSteamClientPlugin()) {
        steamClient->shutdown();
    }
    DependencyManager::destroy<PluginManager>();

    DependencyManager::destroy<CompositorHelper>(); // must be destroyed before the FramebufferCache

    DependencyManager::destroy<SoundCacheScriptingInterface>();

    DependencyManager::destroy<AvatarManager>();
    DependencyManager::destroy<AnimationCacheScriptingInterface>();
    DependencyManager::destroy<AnimationCache>();
    DependencyManager::destroy<FramebufferCache>();
    DependencyManager::destroy<TextureCacheScriptingInterface>();
    DependencyManager::destroy<TextureCache>();
    DependencyManager::destroy<ModelCacheScriptingInterface>();
    DependencyManager::destroy<ModelCache>();
    DependencyManager::destroy<ScriptCache>();
    DependencyManager::destroy<SoundCacheScriptingInterface>();
    DependencyManager::destroy<SoundCache>();
    DependencyManager::destroy<OctreeStatsProvider>();
    DependencyManager::destroy<GeometryCache>();

    DependencyManager::get<ResourceManager>()->cleanup();

    // remove the NodeList from the DependencyManager
    DependencyManager::destroy<NodeList>();

#if 0
    ConnexionClient::getInstance().destroy();
#endif
    // The window takes ownership of the menu, so this has the side effect of destroying it.
    _window->setMenuBar(nullptr);

    _window->deleteLater();

    // make sure that the quit event has finished sending before we take the application down
    auto closeEventSender = DependencyManager::get<CloseEventSender>();
    while (!closeEventSender->hasFinishedQuitEvent() && !closeEventSender->hasTimedOutQuitEvent()) {
        // sleep a little so we're not spinning at 100%
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // quit the thread used by the closure event sender
    closeEventSender->thread()->quit();

    // Can't log to file passed this point, FileLogger about to be deleted
    qInstallMessageHandler(LogHandler::verboseMessageHandler);

    _renderEventHandler->deleteLater();
}

void Application::initializeGL() {
    qCDebug(interfaceapp) << "Created Display Window.";

#ifdef DISABLE_QML
    setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);
#endif

    // initialize glut for shape drawing; Qt apparently initializes it on OS X
    if (_isGLInitialized) {
        return;
    } else {
        _isGLInitialized = true;
    }

    if (!_glWidget->makeCurrent()) {
        qCWarning(interfaceapp, "Unable to make window context current");
    }

#if !defined(DISABLE_QML)
    // Build a shared canvas / context for the Chromium processes
    {
        // Disable signed distance field font rendering on ATI/AMD GPUs, due to
        // https://highfidelity.manuscript.com/f/cases/13677/Text-showing-up-white-on-Marketplace-app
        std::string vendor{ (const char*)glGetString(GL_VENDOR) };
        if ((vendor.find("AMD") != std::string::npos) || (vendor.find("ATI") != std::string::npos)) {
            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", QByteArray("--disable-distance-field-text"));
        }

        // Chromium rendering uses some GL functions that prevent nSight from capturing
        // frames, so we only create the shared context if nsight is NOT active.
        if (!nsightActive()) {
            _chromiumShareContext = new OffscreenGLCanvas();
            _chromiumShareContext->setObjectName("ChromiumShareContext");
            _chromiumShareContext->create(_glWidget->qglContext());
            if (!_chromiumShareContext->makeCurrent()) {
                qCWarning(interfaceapp, "Unable to make chromium shared context current");
            }
            qt_gl_set_global_share_context(_chromiumShareContext->getContext());
            _chromiumShareContext->doneCurrent();
            // Restore the GL widget context
            if (!_glWidget->makeCurrent()) {
                qCWarning(interfaceapp, "Unable to make window context current");
            }
        } else {
            qCWarning(interfaceapp) << "nSight detected, disabling chrome rendering";
        }
    }
#endif

    // Build a shared canvas / context for the QML rendering
    {
        _qmlShareContext = new OffscreenGLCanvas();
        _qmlShareContext->setObjectName("QmlShareContext");
        _qmlShareContext->create(_glWidget->qglContext());
        if (!_qmlShareContext->makeCurrent()) {
            qCWarning(interfaceapp, "Unable to make QML shared context current");
        }
        OffscreenQmlSurface::setSharedContext(_qmlShareContext->getContext());
        _qmlShareContext->doneCurrent();
        if (!_glWidget->makeCurrent()) {
            qCWarning(interfaceapp, "Unable to make window context current");
        }
    }

    _renderEventHandler = new RenderEventHandler();

    // Build an offscreen GL context for the main thread.
    _glWidget->makeCurrent();
    glClearColor(0.2f, 0.2f, 0.2f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    _glWidget->swapBuffers();


    // Create the GPU backend

    // Requires the window context, because that's what's used in the actual rendering
    // and the GPU backend will make things like the VAO which cannot be shared across
    // contexts
    _glWidget->makeCurrent();
    gpu::Context::init<gpu::gl::GLBackend>();
    _glWidget->makeCurrent();
    _gpuContext = std::make_shared<gpu::Context>();

    DependencyManager::get<TextureCache>()->setGPUContext(_gpuContext);
}

static const QString SPLASH_SKYBOX{ "{\"ProceduralEntity\":{ \"version\":2, \"shaderUrl\":\"qrc:///shaders/splashSkybox.frag\" } }" };

void Application::initializeDisplayPlugins() {
    auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();
    Setting::Handle<QString> activeDisplayPluginSetting{ ACTIVE_DISPLAY_PLUGIN_SETTING_NAME, displayPlugins.at(0)->getName() };
    auto lastActiveDisplayPluginName = activeDisplayPluginSetting.get();

    auto defaultDisplayPlugin = displayPlugins.at(0);
    // Once time initialization code
    DisplayPluginPointer targetDisplayPlugin;
    foreach(auto displayPlugin, displayPlugins) {
        displayPlugin->setContext(_gpuContext);
        if (displayPlugin->getName() == lastActiveDisplayPluginName) {
            targetDisplayPlugin = displayPlugin;
        }
        QObject::connect(displayPlugin.get(), &DisplayPlugin::recommendedFramebufferSizeChanged,
            [this](const QSize& size) { resizeGL(); });
        QObject::connect(displayPlugin.get(), &DisplayPlugin::resetSensorsRequested, this, &Application::requestReset);
        if (displayPlugin->isHmd()) {
            QObject::connect(dynamic_cast<HmdDisplayPlugin*>(displayPlugin.get()), &HmdDisplayPlugin::hmdMountedChanged,
                DependencyManager::get<HMDScriptingInterface>().data(), &HMDScriptingInterface::mountedChanged);
        }
    }

    // The default display plugin needs to be activated first, otherwise the display plugin thread
    // may be launched by an external plugin, which is bad
    setDisplayPlugin(defaultDisplayPlugin);

    // Now set the desired plugin if it's not the same as the default plugin
    if (targetDisplayPlugin && (targetDisplayPlugin != defaultDisplayPlugin)) {
        setDisplayPlugin(targetDisplayPlugin);
    }

    // Submit a default frame to render until the engine starts up
    updateRenderArgs(0.0f);

#define ENABLE_SPLASH_FRAME 0
#if ENABLE_SPLASH_FRAME
    {
        QMutexLocker viewLocker(&_renderArgsMutex);

        if (_appRenderArgs._isStereo) {
            _gpuContext->enableStereo(true);
            _gpuContext->setStereoProjections(_appRenderArgs._eyeProjections);
            _gpuContext->setStereoViews(_appRenderArgs._eyeOffsets);
        }

        // Frame resources
        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        gpu::FramebufferPointer finalFramebuffer = framebufferCache->getFramebuffer();
        std::shared_ptr<ProceduralSkybox> procedural = std::make_shared<ProceduralSkybox>();
        procedural->parse(SPLASH_SKYBOX);

        _gpuContext->beginFrame(_appRenderArgs._view, _appRenderArgs._headPose);
        gpu::doInBatch("splashFrame", _gpuContext, [&](gpu::Batch& batch) {
            batch.resetStages();
            batch.enableStereo(false);
            batch.setFramebuffer(finalFramebuffer);
            batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, { 0, 0, 0, 1 });
            batch.enableSkybox(true);
            batch.enableStereo(_appRenderArgs._isStereo);
            batch.setViewportTransform({ 0, 0, finalFramebuffer->getSize() });
            procedural->render(batch, _appRenderArgs._renderArgs.getViewFrustum());
        });
        auto frame = _gpuContext->endFrame();
        frame->frameIndex = 0;
        frame->framebuffer = finalFramebuffer;
        frame->pose = _appRenderArgs._headPose;
        frame->framebufferRecycler = [framebufferCache, procedural](const gpu::FramebufferPointer& framebuffer) {
            framebufferCache->releaseFramebuffer(framebuffer);
        };
        _displayPlugin->submitFrame(frame);
    }
#endif
}

void Application::initializeRenderEngine() {
    // FIXME: on low end systems os the shaders take up to 1 minute to compile, so we pause the deadlock watchdog thread.
    DeadlockWatchdogThread::withPause([&] {
        // Set up the render engine
        render::CullFunctor cullFunctor = LODManager::shouldRender;
        _renderEngine->addJob<UpdateSceneTask>("UpdateScene");
#ifndef Q_OS_ANDROID
        _renderEngine->addJob<SecondaryCameraRenderTask>("SecondaryCameraJob", cullFunctor, !DISABLE_DEFERRED);
#endif
        _renderEngine->addJob<RenderViewTask>("RenderMainView", cullFunctor, !DISABLE_DEFERRED, render::ItemKey::TAG_BITS_0, render::ItemKey::TAG_BITS_0);
        _renderEngine->load();
        _renderEngine->registerScene(_main3DScene);

        // Now that OpenGL is initialized, we are sure we have a valid context and can create the various pipeline shaders with success.
        DependencyManager::get<GeometryCache>()->initializeShapePipelines();
    });
}

extern void setupPreferences();
static void addDisplayPluginToMenu(const DisplayPluginPointer& displayPlugin, int index, bool active = false);

void Application::initializeUi() {
    AddressBarDialog::registerType();
    ErrorDialog::registerType();
    LoginDialog::registerType();
    Tooltip::registerType();
    UpdateDialog::registerType();
    QmlContextCallback callback = [](QQmlContext* context) {
        context->setContextProperty("Commerce", new QmlCommerce());
    };
    OffscreenQmlSurface::addWhitelistContextHandler({
        QUrl{ "hifi/commerce/checkout/Checkout.qml" },
        QUrl{ "hifi/commerce/common/CommerceLightbox.qml" },
        QUrl{ "hifi/commerce/common/EmulatedMarketplaceHeader.qml" },
        QUrl{ "hifi/commerce/common/FirstUseTutorial.qml" },
        QUrl{ "hifi/commerce/common/SortableListModel.qml" },
        QUrl{ "hifi/commerce/common/sendAsset/SendAsset.qml" },
        QUrl{ "hifi/commerce/inspectionCertificate/InspectionCertificate.qml" },
        QUrl{ "hifi/commerce/purchases/PurchasedItem.qml" },
        QUrl{ "hifi/commerce/purchases/Purchases.qml" },
        QUrl{ "hifi/commerce/wallet/Help.qml" },
        QUrl{ "hifi/commerce/wallet/NeedsLogIn.qml" },
        QUrl{ "hifi/commerce/wallet/PassphraseChange.qml" },
        QUrl{ "hifi/commerce/wallet/PassphraseModal.qml" },
        QUrl{ "hifi/commerce/wallet/PassphraseSelection.qml" },
        QUrl{ "hifi/commerce/wallet/Security.qml" },
        QUrl{ "hifi/commerce/wallet/SecurityImageChange.qml" },
        QUrl{ "hifi/commerce/wallet/SecurityImageModel.qml" },
        QUrl{ "hifi/commerce/wallet/SecurityImageSelection.qml" },
        QUrl{ "hifi/commerce/wallet/Wallet.qml" },
        QUrl{ "hifi/commerce/wallet/WalletHome.qml" },
        QUrl{ "hifi/commerce/wallet/WalletSetup.qml" },
    }, callback);
    qmlRegisterType<ResourceImageItem>("Hifi", 1, 0, "ResourceImageItem");
    qmlRegisterType<Preference>("Hifi", 1, 0, "Preference");
    qmlRegisterType<WebBrowserSuggestionsEngine>("HifiWeb", 1, 0, "WebBrowserSuggestionsEngine");

    {
        auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
        tabletScriptingInterface->getTablet(SYSTEM_TABLET);
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    connect(offscreenUi.data(), &hifi::qml::OffscreenSurface::rootContextCreated,
        this, &Application::onDesktopRootContextCreated);
    connect(offscreenUi.data(), &hifi::qml::OffscreenSurface::rootItemCreated,
        this, &Application::onDesktopRootItemCreated);

    offscreenUi->setProxyWindow(_window->windowHandle());
    // OffscreenUi is a subclass of OffscreenQmlSurface specifically designed to
    // support the window management and scripting proxies for VR use
    DeadlockWatchdogThread::withPause([&] {
        offscreenUi->createDesktop(PathUtils::qmlUrl("hifi/Desktop.qml"));
    });
    // FIXME either expose so that dialogs can set this themselves or
    // do better detection in the offscreen UI of what has focus
    offscreenUi->setNavigationFocused(false);

    setupPreferences();

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
        resizeGL();
        if (_touchscreenVirtualPadDevice) {
            _touchscreenVirtualPadDevice->resize();
        }
    });

    // This will set up the input plugins UI
    _activeInputPlugins.clear();
    foreach(auto inputPlugin, PluginManager::getInstance()->getInputPlugins()) {
        if (KeyboardMouseDevice::NAME == inputPlugin->getName()) {
            _keyboardMouseDevice = std::dynamic_pointer_cast<KeyboardMouseDevice>(inputPlugin);
        }
        if (TouchscreenDevice::NAME == inputPlugin->getName()) {
            _touchscreenDevice = std::dynamic_pointer_cast<TouchscreenDevice>(inputPlugin);
        }
        if (TouchscreenVirtualPadDevice::NAME == inputPlugin->getName()) {
            _touchscreenVirtualPadDevice = std::dynamic_pointer_cast<TouchscreenVirtualPadDevice>(inputPlugin);
#if defined(Q_OS_ANDROID)
            auto& virtualPadManager = VirtualPad::Manager::instance();
            connect(&virtualPadManager, &VirtualPad::Manager::hapticFeedbackRequested,
                    this, [](int duration) {
                        AndroidHelper::instance().performHapticFeedback(duration);
                    });
#endif
        }
    }

    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    connect(compositorHelper.data(), &CompositorHelper::allowMouseCaptureChanged, this, [=] {
        if (isHMDMode()) {
            auto compositorHelper = DependencyManager::get<CompositorHelper>(); // don't capture outer smartpointer
            showCursor(compositorHelper->getAllowMouseCapture() ?
                       Cursor::Manager::lookupIcon(_preferredCursor.get()) :
                       Cursor::Icon::SYSTEM);
        }
    });

    // Pre-create a couple of Web3D overlays to speed up tablet UI
    auto offscreenSurfaceCache = DependencyManager::get<OffscreenQmlSurfaceCache>();
    offscreenSurfaceCache->setOnRootContextCreated([&](const QString& rootObject, QQmlContext* surfaceContext) {
        if (rootObject == TabletScriptingInterface::QML) {
            // in Qt 5.10.0 there is already an "Audio" object in the QML context
            // though I failed to find it (from QtMultimedia??). So..  let it be "AudioScriptingInterface"
            surfaceContext->setContextProperty("AudioScriptingInterface", DependencyManager::get<AudioScriptingInterface>().data());
            surfaceContext->setContextProperty("Account", AccountServicesScriptingInterface::getInstance()); // DEPRECATED - TO BE REMOVED
        }
    });

    offscreenSurfaceCache->reserve(TabletScriptingInterface::QML, 1);
    offscreenSurfaceCache->reserve(Web3DOverlay::QML, 2);

    flushMenuUpdates();

    // Now that the menu is instantiated, ensure the display plugin menu is properly updated
    {
        auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();
        // first sort the plugins into groupings: standard, advanced, developer
        std::stable_sort(displayPlugins.begin(), displayPlugins.end(),
            [](const DisplayPluginPointer& a, const DisplayPluginPointer& b)->bool { return a->getGrouping() < b->getGrouping(); });

        int dpIndex = 1;
        // concatenate the groupings into a single list in the order: standard, advanced, developer
        for(const auto& displayPlugin : displayPlugins) {
            addDisplayPluginToMenu(displayPlugin, dpIndex, _displayPlugin == displayPlugin);
            dpIndex++;
        }

        // after all plugins have been added to the menu, add a separator to the menu
        auto parent = getPrimaryMenu()->getMenu(MenuOption::OutputMenu);
        parent->addSeparator();
    }

    // The display plugins are created before the menu now, so we need to do this here to hide the menu bar
    // now that it exists
    if (_window && _window->isFullScreen()) {
        setFullscreen(nullptr, true);
    }


    setIsInterstitialMode(true);
}


void Application::onDesktopRootContextCreated(QQmlContext* surfaceContext) {
    auto engine = surfaceContext->engine();
    // in Qt 5.10.0 there is already an "Audio" object in the QML context
    // though I failed to find it (from QtMultimedia??). So..  let it be "AudioScriptingInterface"
    surfaceContext->setContextProperty("AudioScriptingInterface", DependencyManager::get<AudioScriptingInterface>().data());

    surfaceContext->setContextProperty("AudioStats", DependencyManager::get<AudioClient>()->getStats().data());
    surfaceContext->setContextProperty("AudioScope", DependencyManager::get<AudioScope>().data());

    surfaceContext->setContextProperty("Controller", DependencyManager::get<controller::ScriptingInterface>().data());
    surfaceContext->setContextProperty("Entities", DependencyManager::get<EntityScriptingInterface>().data());
    _fileDownload = new FileScriptingInterface(engine);
    surfaceContext->setContextProperty("File", _fileDownload);
    connect(_fileDownload, &FileScriptingInterface::unzipResult, this, &Application::handleUnzip);
    surfaceContext->setContextProperty("MyAvatar", getMyAvatar().get());
    surfaceContext->setContextProperty("Messages", DependencyManager::get<MessagesClient>().data());
    surfaceContext->setContextProperty("Recording", DependencyManager::get<RecordingScriptingInterface>().data());
    surfaceContext->setContextProperty("Preferences", DependencyManager::get<Preferences>().data());
    surfaceContext->setContextProperty("AddressManager", DependencyManager::get<AddressManager>().data());
    surfaceContext->setContextProperty("FrameTimings", &_frameTimingsScriptingInterface);
    surfaceContext->setContextProperty("Rates", new RatesScriptingInterface(this));

    surfaceContext->setContextProperty("TREE_SCALE", TREE_SCALE);
    // FIXME Quat and Vec3 won't work with QJSEngine used by QML
    surfaceContext->setContextProperty("Quat", new Quat());
    surfaceContext->setContextProperty("Vec3", new Vec3());
    surfaceContext->setContextProperty("Uuid", new ScriptUUID());
    surfaceContext->setContextProperty("Assets", DependencyManager::get<AssetMappingsScriptingInterface>().data());

    surfaceContext->setContextProperty("AvatarList", DependencyManager::get<AvatarManager>().data());
    surfaceContext->setContextProperty("Users", DependencyManager::get<UsersScriptingInterface>().data());

    surfaceContext->setContextProperty("UserActivityLogger", DependencyManager::get<UserActivityLoggerScriptingInterface>().data());

    surfaceContext->setContextProperty("Camera", &_myCamera);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    surfaceContext->setContextProperty("SpeechRecognizer", DependencyManager::get<SpeechRecognizer>().data());
#endif

    surfaceContext->setContextProperty("Overlays", &_overlays);
    surfaceContext->setContextProperty("Window", DependencyManager::get<WindowScriptingInterface>().data());
    surfaceContext->setContextProperty("Desktop", DependencyManager::get<DesktopScriptingInterface>().data());
    surfaceContext->setContextProperty("MenuInterface", MenuScriptingInterface::getInstance());
    surfaceContext->setContextProperty("Settings", SettingsScriptingInterface::getInstance());
    surfaceContext->setContextProperty("ScriptDiscoveryService", DependencyManager::get<ScriptEngines>().data());
    surfaceContext->setContextProperty("AvatarBookmarks", DependencyManager::get<AvatarBookmarks>().data());
    surfaceContext->setContextProperty("LocationBookmarks", DependencyManager::get<LocationBookmarks>().data());

    // Caches
    surfaceContext->setContextProperty("AnimationCache", DependencyManager::get<AnimationCacheScriptingInterface>().data());
    surfaceContext->setContextProperty("TextureCache", DependencyManager::get<TextureCacheScriptingInterface>().data());
    surfaceContext->setContextProperty("ModelCache", DependencyManager::get<ModelCacheScriptingInterface>().data());
    surfaceContext->setContextProperty("SoundCache", DependencyManager::get<SoundCacheScriptingInterface>().data());

    surfaceContext->setContextProperty("InputConfiguration", DependencyManager::get<InputConfiguration>().data());

    surfaceContext->setContextProperty("Account", AccountServicesScriptingInterface::getInstance()); // DEPRECATED - TO BE REMOVED
    surfaceContext->setContextProperty("GlobalServices", AccountServicesScriptingInterface::getInstance()); // DEPRECATED - TO BE REMOVED
    surfaceContext->setContextProperty("AccountServices", AccountServicesScriptingInterface::getInstance());

    surfaceContext->setContextProperty("DialogsManager", _dialogsManagerScriptingInterface);
    surfaceContext->setContextProperty("FaceTracker", DependencyManager::get<DdeFaceTracker>().data());
    surfaceContext->setContextProperty("AvatarManager", DependencyManager::get<AvatarManager>().data());
    surfaceContext->setContextProperty("UndoStack", &_undoStackScriptingInterface);
    surfaceContext->setContextProperty("LODManager", DependencyManager::get<LODManager>().data());
    surfaceContext->setContextProperty("HMD", DependencyManager::get<HMDScriptingInterface>().data());
    surfaceContext->setContextProperty("Scene", DependencyManager::get<SceneScriptingInterface>().data());
    surfaceContext->setContextProperty("Render", _renderEngine->getConfiguration().get());
    surfaceContext->setContextProperty("Workload", _gameWorkload._engine->getConfiguration().get());
    surfaceContext->setContextProperty("Reticle", getApplicationCompositor().getReticleInterface());
    surfaceContext->setContextProperty("Snapshot", DependencyManager::get<Snapshot>().data());

    surfaceContext->setContextProperty("ApplicationCompositor", &getApplicationCompositor());

    surfaceContext->setContextProperty("AvatarInputs", AvatarInputs::getInstance());
    surfaceContext->setContextProperty("Selection", DependencyManager::get<SelectionScriptingInterface>().data());
    surfaceContext->setContextProperty("ContextOverlay", DependencyManager::get<ContextOverlayInterface>().data());
    surfaceContext->setContextProperty("Wallet", DependencyManager::get<WalletScriptingInterface>().data());
    surfaceContext->setContextProperty("HiFiAbout", AboutUtil::getInstance());

    if (auto steamClient = PluginManager::getInstance()->getSteamClientPlugin()) {
        surfaceContext->setContextProperty("Steam", new SteamScriptingInterface(engine, steamClient.get()));
    }

    _window->setMenuBar(new Menu());
}

void Application::onDesktopRootItemCreated(QQuickItem* rootItem) {
    Stats::show();
    AnimStats::show();
    auto surfaceContext = DependencyManager::get<OffscreenUi>()->getSurfaceContext();
    surfaceContext->setContextProperty("Stats", Stats::getInstance());
    surfaceContext->setContextProperty("AnimStats", AnimStats::getInstance());

#if !defined(Q_OS_ANDROID)
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto qml = PathUtils::qmlUrl("AvatarInputsBar.qml");
    offscreenUi->show(qml, "AvatarInputsBar");
#endif
}

void Application::updateCamera(RenderArgs& renderArgs, float deltaTime) {
    PROFILE_RANGE(render, __FUNCTION__);
    PerformanceTimer perfTimer("updateCamera");

    glm::vec3 boomOffset;
    auto myAvatar = getMyAvatar();
    boomOffset = myAvatar->getModelScale() * myAvatar->getBoomLength() * -IDENTITY_FORWARD;

    // The render mode is default or mirror if the camera is in mirror mode, assigned further below
    renderArgs._renderMode = RenderArgs::DEFAULT_RENDER_MODE;

    // Always use the default eye position, not the actual head eye position.
    // Using the latter will cause the camera to wobble with idle animations,
    // or with changes from the face tracker
    if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON) {
        _thirdPersonHMDCameraBoomValid= false;
        if (isHMDMode()) {
            mat4 camMat = myAvatar->getSensorToWorldMatrix() * myAvatar->getHMDSensorMatrix();
            _myCamera.setPosition(extractTranslation(camMat));
            _myCamera.setOrientation(glmExtractRotation(camMat));
        }
        else {
            _myCamera.setPosition(myAvatar->getDefaultEyePosition());
            _myCamera.setOrientation(myAvatar->getMyHead()->getHeadOrientation());
        }
    }
    else if (_myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
        if (isHMDMode()) {

            if (!_thirdPersonHMDCameraBoomValid) {
                const glm::vec3 CAMERA_OFFSET = glm::vec3(0.0f, 0.0f, 0.7f);
                _thirdPersonHMDCameraBoom = cancelOutRollAndPitch(myAvatar->getHMDSensorOrientation()) * CAMERA_OFFSET;
                _thirdPersonHMDCameraBoomValid = true;
            }

            glm::mat4 thirdPersonCameraSensorToWorldMatrix = myAvatar->getSensorToWorldMatrix();

            const glm::vec3 cameraPos = myAvatar->getHMDSensorPosition() + _thirdPersonHMDCameraBoom * myAvatar->getBoomLength();
            glm::mat4 sensorCameraMat = createMatFromQuatAndPos(myAvatar->getHMDSensorOrientation(), cameraPos);
            glm::mat4 worldCameraMat = thirdPersonCameraSensorToWorldMatrix * sensorCameraMat;

            _myCamera.setOrientation(glm::normalize(glmExtractRotation(worldCameraMat)));
            _myCamera.setPosition(extractTranslation(worldCameraMat));
        }
        else {
            _thirdPersonHMDCameraBoomValid = false;

            _myCamera.setOrientation(myAvatar->getHead()->getOrientation());
            if (isOptionChecked(MenuOption::CenterPlayerInView)) {
                _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                    + _myCamera.getOrientation() * boomOffset);
            }
            else {
                _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                    + myAvatar->getWorldOrientation() * boomOffset);
            }
        }
    }
    else if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        _thirdPersonHMDCameraBoomValid= false;

        if (isHMDMode()) {
            auto mirrorBodyOrientation = myAvatar->getWorldOrientation() * glm::quat(glm::vec3(0.0f, PI + _mirrorYawOffset, 0.0f));

            glm::quat hmdRotation = extractRotation(myAvatar->getHMDSensorMatrix());
            // Mirror HMD yaw and roll
            glm::vec3 mirrorHmdEulers = glm::eulerAngles(hmdRotation);
            mirrorHmdEulers.y = -mirrorHmdEulers.y;
            mirrorHmdEulers.z = -mirrorHmdEulers.z;
            glm::quat mirrorHmdRotation = glm::quat(mirrorHmdEulers);

            glm::quat worldMirrorRotation = mirrorBodyOrientation * mirrorHmdRotation;

            _myCamera.setOrientation(worldMirrorRotation);

            glm::vec3 hmdOffset = extractTranslation(myAvatar->getHMDSensorMatrix());
            // Mirror HMD lateral offsets
            hmdOffset.x = -hmdOffset.x;

            _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                + glm::vec3(0, _raiseMirror * myAvatar->getModelScale(), 0)
                + mirrorBodyOrientation * glm::vec3(0.0f, 0.0f, 1.0f) * MIRROR_FULLSCREEN_DISTANCE * _scaleMirror
                + mirrorBodyOrientation * hmdOffset);
        }
        else {
            auto userInputMapper = DependencyManager::get<UserInputMapper>();
            const float YAW_SPEED = TWO_PI / 5.0f;
            float deltaYaw = userInputMapper->getActionState(controller::Action::YAW) * YAW_SPEED * deltaTime;
            _mirrorYawOffset += deltaYaw;
            _myCamera.setOrientation(myAvatar->getWorldOrientation() * glm::quat(glm::vec3(0.0f, PI + _mirrorYawOffset, 0.0f)));
            _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                + glm::vec3(0, _raiseMirror * myAvatar->getModelScale(), 0)
                + (myAvatar->getWorldOrientation() * glm::quat(glm::vec3(0.0f, _mirrorYawOffset, 0.0f))) *
                glm::vec3(0.0f, 0.0f, -1.0f) * myAvatar->getBoomLength() * _scaleMirror);
        }
        renderArgs._renderMode = RenderArgs::MIRROR_RENDER_MODE;
    }
    else if (_myCamera.getMode() == CAMERA_MODE_ENTITY) {
        _thirdPersonHMDCameraBoomValid= false;
        EntityItemPointer cameraEntity = _myCamera.getCameraEntityPointer();
        if (cameraEntity != nullptr) {
            if (isHMDMode()) {
                glm::quat hmdRotation = extractRotation(myAvatar->getHMDSensorMatrix());
                _myCamera.setOrientation(cameraEntity->getWorldOrientation() * hmdRotation);
                glm::vec3 hmdOffset = extractTranslation(myAvatar->getHMDSensorMatrix());
                _myCamera.setPosition(cameraEntity->getWorldPosition() + (hmdRotation * hmdOffset));
            }
            else {
                _myCamera.setOrientation(cameraEntity->getWorldOrientation());
                _myCamera.setPosition(cameraEntity->getWorldPosition());
            }
        }
    }
    // Update camera position
    if (!isHMDMode()) {
        _myCamera.update();
    }

    renderArgs._cameraMode = (int8_t)_myCamera.getMode();
}

void Application::runTests() {
    runTimingTests();
    runUnitTests();
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

void Application::setHMDTabletScale(float hmdTabletScale) {
    _hmdTabletScale.set(hmdTabletScale);
}

void Application::setDesktopTabletScale(float desktopTabletScale) {
    _desktopTabletScale.set(desktopTabletScale);
}

void Application::setDesktopTabletBecomesToolbarSetting(bool value) {
    _desktopTabletBecomesToolbarSetting.set(value);
    updateSystemTabletMode();
}

void Application::setHmdTabletBecomesToolbarSetting(bool value) {
    _hmdTabletBecomesToolbarSetting.set(value);
    updateSystemTabletMode();
}

void Application::setPreferStylusOverLaser(bool value) {
    _preferStylusOverLaserSetting.set(value);
}

void Application::setPreferAvatarFingerOverStylus(bool value) {
    _preferAvatarFingerOverStylusSetting.set(value);
}

void Application::setPreferredCursor(const QString& cursorName) {
    qCDebug(interfaceapp) << "setPreferredCursor" << cursorName;
    _preferredCursor.set(cursorName.isEmpty() ? DEFAULT_CURSOR_NAME : cursorName);
    showCursor(Cursor::Manager::lookupIcon(_preferredCursor.get()));
}

void Application::setSettingConstrainToolbarPosition(bool setting) {
    _constrainToolbarPosition.set(setting);
    DependencyManager::get<OffscreenUi>()->setConstrainToolbarToCenterX(setting);
}

void Application::showHelp() {
    static const QString HAND_CONTROLLER_NAME_VIVE = "vive";
    static const QString HAND_CONTROLLER_NAME_OCULUS_TOUCH = "oculus";
    static const QString HAND_CONTROLLER_NAME_WINDOWS_MR = "windowsMR";

    static const QString TAB_KEYBOARD_MOUSE = "kbm";
    static const QString TAB_GAMEPAD = "gamepad";
    static const QString TAB_HAND_CONTROLLERS = "handControllers";

    QString handControllerName = HAND_CONTROLLER_NAME_VIVE;
    QString defaultTab = TAB_KEYBOARD_MOUSE;

    if (PluginUtils::isViveControllerAvailable()) {
        defaultTab = TAB_HAND_CONTROLLERS;
        handControllerName = HAND_CONTROLLER_NAME_VIVE;
    } else if (PluginUtils::isOculusTouchControllerAvailable()) {
        defaultTab = TAB_HAND_CONTROLLERS;
        handControllerName = HAND_CONTROLLER_NAME_OCULUS_TOUCH;
    } else if (qApp->getActiveDisplayPlugin()->getName() == "WindowMS") {
        defaultTab = TAB_HAND_CONTROLLERS;
        handControllerName = HAND_CONTROLLER_NAME_WINDOWS_MR;
    } else if (PluginUtils::isXboxControllerAvailable()) {
        defaultTab = TAB_GAMEPAD;
    }
    // TODO need some way to detect windowsMR to load controls reference default tab in Help > Controls Reference menu.

    QUrlQuery queryString;
    queryString.addQueryItem("handControllerName", handControllerName);
    queryString.addQueryItem("defaultTab", defaultTab);
    TabletProxy* tablet = dynamic_cast<TabletProxy*>(DependencyManager::get<TabletScriptingInterface>()->getTablet(SYSTEM_TABLET));
    tablet->gotoWebScreen(PathUtils::resourcesUrl() + INFO_HELP_PATH + "?" + queryString.toString());
    DependencyManager::get<HMDScriptingInterface>()->openTablet();
    //InfoView::show(INFO_HELP_PATH, false, queryString.toString());
}

void Application::resizeEvent(QResizeEvent* event) {
    resizeGL();
}

void Application::resizeGL() {
    PROFILE_RANGE(render, __FUNCTION__);
    if (nullptr == _displayPlugin) {
        return;
    }

    auto displayPlugin = getActiveDisplayPlugin();
    // Set the desired FBO texture size. If it hasn't changed, this does nothing.
    // Otherwise, it must rebuild the FBOs
    uvec2 framebufferSize = displayPlugin->getRecommendedRenderSize();
    uvec2 renderSize = uvec2(framebufferSize);
    if (_renderResolution != renderSize) {
        _renderResolution = renderSize;
        DependencyManager::get<FramebufferCache>()->setFrameBufferSize(fromGlm(renderSize));
    }

    auto renderResolutionScale = getRenderResolutionScale();
    if (displayPlugin->getRenderResolutionScale() != renderResolutionScale) {
        auto renderConfig = _renderEngine->getConfiguration();
        assert(renderConfig);
        auto mainView = renderConfig->getConfig("RenderMainView.RenderDeferredTask");
        assert(mainView);
        mainView->setProperty("resolutionScale", renderResolutionScale);
        displayPlugin->setRenderResolutionScale(renderResolutionScale);
    }

    // FIXME the aspect ratio for stereo displays is incorrect based on this.
    float aspectRatio = displayPlugin->getRecommendedAspectRatio();
    _myCamera.setProjection(glm::perspective(glm::radians(_fieldOfView.get()), aspectRatio,
                                             DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
    // Possible change in aspect ratio
    {
        QMutexLocker viewLocker(&_viewMutex);
        _myCamera.loadViewFrustum(_viewFrustum);
    }

    DependencyManager::get<OffscreenUi>()->resize(fromGlm(displayPlugin->getRecommendedUiSize()));
}

void Application::handleSandboxStatus(QNetworkReply* reply) {
    PROFILE_RANGE(render, __FUNCTION__);

    bool sandboxIsRunning = SandboxUtils::readStatus(reply->readAll());

    enum HandControllerType {
        Vive,
        Oculus
    };
    static const std::map<HandControllerType, int> MIN_CONTENT_VERSION = {
        { Vive, 1 },
        { Oculus, 27 }
    };

    // Get sandbox content set version
    auto acDirPath = PathUtils::getAppDataPath() + "../../" + BuildInfo::MODIFIED_ORGANIZATION + "/assignment-client/";
    auto contentVersionPath = acDirPath + "content-version.txt";
    qCDebug(interfaceapp) << "Checking " << contentVersionPath << " for content version";
    int contentVersion = 0;
    QFile contentVersionFile(contentVersionPath);
    if (contentVersionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString line = contentVersionFile.readAll();
        contentVersion = line.toInt(); // returns 0 if conversion fails
    }

    // Get controller availability
    bool hasHandControllers = false;
    if (PluginUtils::isViveControllerAvailable() || PluginUtils::isOculusTouchControllerAvailable()) {
        hasHandControllers = true;
    }

    // Check HMD use (may be technically available without being in use)
    bool hasHMD = PluginUtils::isHMDAvailable();
    bool isUsingHMD = _displayPlugin->isHmd();
    bool isUsingHMDAndHandControllers = hasHMD && hasHandControllers && isUsingHMD;

    Setting::Handle<bool> firstRun{ Settings::firstRun, true };

    qCDebug(interfaceapp) << "HMD:" << hasHMD << ", Hand Controllers: " << hasHandControllers << ", Using HMD: " << isUsingHMDAndHandControllers;

    // when --url in command line, teleport to location
    const QString HIFI_URL_COMMAND_LINE_KEY = "--url";
    int urlIndex = arguments().indexOf(HIFI_URL_COMMAND_LINE_KEY);
    QString addressLookupString;
    if (urlIndex != -1) {
        addressLookupString = arguments().value(urlIndex + 1);
    }

    static const QString SENT_TO_PREVIOUS_LOCATION = "previous_location";
    static const QString SENT_TO_ENTRY = "entry";

    QString sentTo;

    // If this is a first run we short-circuit the address passed in
    if (firstRun.get()) {
#if !defined(Q_OS_ANDROID)
        DependencyManager::get<AddressManager>()->goToEntry();
        sentTo = SENT_TO_ENTRY;
#endif
        firstRun.set(false);

    } else {
#if !defined(Q_OS_ANDROID)
        qCDebug(interfaceapp) << "Not first run... going to" << qPrintable(addressLookupString.isEmpty() ? QString("previous location") : addressLookupString);
        DependencyManager::get<AddressManager>()->loadSettings(addressLookupString);
        sentTo = SENT_TO_PREVIOUS_LOCATION;
#endif
    }

    UserActivityLogger::getInstance().logAction("startup_sent_to", {
        { "sent_to", sentTo },
        { "sandbox_is_running", sandboxIsRunning },
        { "has_hmd", hasHMD },
        { "has_hand_controllers", hasHandControllers },
        { "is_using_hmd", isUsingHMD },
        { "is_using_hmd_and_hand_controllers", isUsingHMDAndHandControllers },
        { "content_version", contentVersion }
    });

    _connectionMonitor.init();
}

bool Application::importJSONFromURL(const QString& urlString) {
    // we only load files that terminate in just .json (not .svo.json and not .ava.json)
    QUrl jsonURL { urlString };

    emit svoImportRequested(urlString);
    return true;
}

bool Application::importSVOFromURL(const QString& urlString) {
    emit svoImportRequested(urlString);
    return true;
}

bool Application::importFromZIP(const QString& filePath) {
    qDebug() << "A zip file has been dropped in: " << filePath;
    QUrl empty;
    // handle Blocks download from Marketplace
    if (filePath.contains("poly.google.com/downloads")) {
        addAssetToWorldFromURL(filePath);
    } else {
        qApp->getFileDownloadInterface()->runUnzip(filePath, empty, true, true, false);
    }
    return true;
}

bool Application::isServerlessMode() const {
    auto tree = getEntities()->getTree();
    if (tree) {
        return tree->isServerlessMode();
    }
    return false;
}

void Application::setIsInterstitialMode(bool interstitialMode) {
    Settings settings;
    bool enableInterstitial = settings.value("enableIntersitialMode", false).toBool();
    if (_interstitialMode != interstitialMode && enableInterstitial) {
        _interstitialMode = interstitialMode;

        DependencyManager::get<AudioClient>()->setAudioPaused(_interstitialMode);
        DependencyManager::get<AvatarManager>()->setMyAvatarDataPacketsPaused(_interstitialMode);
    }
}

void Application::setIsServerlessMode(bool serverlessDomain) {
    auto tree = getEntities()->getTree();
    if (tree) {
        tree->setIsServerlessMode(serverlessDomain);
    }
}

void Application::loadServerlessDomain(QUrl domainURL, bool errorDomain) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "loadServerlessDomain", Q_ARG(QUrl, domainURL), Q_ARG(bool, errorDomain));
        return;
    }

    if (domainURL.isEmpty()) {
        return;
    }

    QUuid serverlessSessionID = QUuid::createUuid();
    getMyAvatar()->setSessionUUID(serverlessSessionID);
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->setSessionUUID(serverlessSessionID);

    // there is no domain-server to tell us our permissions, so enable all
    NodePermissions permissions;
    permissions.setAll(true);
    nodeList->setPermissions(permissions);

    // we can't import directly into the main tree because we would need to lock it, and
    // Octree::readFromURL calls loop.exec which can run code which will also attempt to lock the tree.
    EntityTreePointer tmpTree(new EntityTree());
    tmpTree->setIsServerlessMode(true);
    tmpTree->createRootElement();
    auto myAvatar = getMyAvatar();
    tmpTree->setMyAvatar(myAvatar);
    bool success = tmpTree->readFromURL(domainURL.toString());
    if (success) {
        tmpTree->reaverageOctreeElements();
        tmpTree->sendEntities(&_entityEditSender, getEntities()->getTree(), 0, 0, 0);
    }

    std::map<QString, QString> namedPaths = tmpTree->getNamedPaths();
    if (errorDomain) {
        nodeList->getDomainHandler().loadedErrorDomain(namedPaths);
    } else {
        nodeList->getDomainHandler().connectedToServerless(namedPaths);
    }

    _fullSceneReceivedCounter++;
}

bool Application::importImage(const QString& urlString) {
    qCDebug(interfaceapp) << "An image file has been dropped in";
    QString filepath(urlString);
    filepath.remove("file:///");
    addAssetToWorld(filepath, "", false, false);
    return true;
}

// thread-safe
void Application::onPresent(quint32 frameCount) {
    bool expected = false;
    if (_pendingIdleEvent.compare_exchange_strong(expected, true)) {
        postEvent(this, new QEvent((QEvent::Type)ApplicationEvent::Idle), Qt::HighEventPriority);
    }
    expected = false;
    if (_renderEventHandler && !isAboutToQuit() && _pendingRenderEvent.compare_exchange_strong(expected, true)) {
        postEvent(_renderEventHandler, new QEvent((QEvent::Type)ApplicationEvent::Render));
    }
}

static inline bool isKeyEvent(QEvent::Type type) {
    return type == QEvent::KeyPress || type == QEvent::KeyRelease;
}

bool Application::handleKeyEventForFocusedEntityOrOverlay(QEvent* event) {
    if (!_keyboardFocusedEntity.get().isInvalidID()) {
        switch (event->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
                {
                    auto eventHandler = getEntities()->getEventHandler(_keyboardFocusedEntity.get());
                    if (eventHandler) {
                        event->setAccepted(false);
                        QCoreApplication::sendEvent(eventHandler, event);
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

    if (_keyboardFocusedOverlay.get() != UNKNOWN_OVERLAY_ID) {
        switch (event->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease: {
                    // Only Web overlays can have focus.
                    auto overlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlays().getOverlay(_keyboardFocusedOverlay.get()));
                    if (overlay && overlay->getEventHandler()) {
                        event->setAccepted(false);
                        QCoreApplication::sendEvent(overlay->getEventHandler(), event);
                        if (event->isAccepted()) {
                            _lastAcceptedKeyPress = usecTimestampNow();
                            return true;
                        }
                    }
                }
                break;

            default:
                break;
        }
    }

    return false;
}

bool Application::handleFileOpenEvent(QFileOpenEvent* fileEvent) {
    QUrl url = fileEvent->url();
    if (!url.isEmpty()) {
        QString urlString = url.toString();
        if (canAcceptURL(urlString)) {
            return acceptURL(urlString);
        }
    }
    return false;
}

#ifdef DEBUG_EVENT_QUEUE
static int getEventQueueSize(QThread* thread) {
    auto threadData = QThreadData::get2(thread);
    QMutexLocker locker(&threadData->postEventList.mutex);
    return threadData->postEventList.size();
}

static void dumpEventQueue(QThread* thread) {
    auto threadData = QThreadData::get2(thread);
    QMutexLocker locker(&threadData->postEventList.mutex);
    qDebug() << "Event list, size =" << threadData->postEventList.size();
    for (auto& postEvent : threadData->postEventList) {
        QEvent::Type type = (postEvent.event ? postEvent.event->type() : QEvent::None);
        qDebug() << "    " << type;
    }
}
#endif // DEBUG_EVENT_QUEUE

bool Application::event(QEvent* event) {

    if (_aboutToQuit) {
        return false;
    }

    if (!Menu::getInstance()) {
        return false;
    }

    // Allow focused Entities and Overlays to handle keyboard input
    if (isKeyEvent(event->type()) && handleKeyEventForFocusedEntityOrOverlay(event)) {
        return true;
    }

    int type = event->type();
    switch (type) {
        case ApplicationEvent::Lambda:
            static_cast<LambdaEvent*>(event)->call();
            return true;

        // Explicit idle keeps the idle running at a lower interval, but without any rendering
        // see (windowMinimizedChanged)
        case ApplicationEvent::Idle:
            idle();

#ifdef DEBUG_EVENT_QUEUE
            {
                int count = getEventQueueSize(QThread::currentThread());
                if (count > 400) {
                    dumpEventQueue(QThread::currentThread());
                }
            }
#endif // DEBUG_EVENT_QUEUE

            _pendingIdleEvent.store(false);

            return true;

        case QEvent::MouseMove:
            mouseMoveEvent(static_cast<QMouseEvent*>(event));
            return true;
        case QEvent::MouseButtonPress:
            mousePressEvent(static_cast<QMouseEvent*>(event));
            return true;
        case QEvent::MouseButtonDblClick:
            mouseDoublePressEvent(static_cast<QMouseEvent*>(event));
            return true;
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent(static_cast<QMouseEvent*>(event));
            return true;
        case QEvent::KeyPress:
            keyPressEvent(static_cast<QKeyEvent*>(event));
            return true;
        case QEvent::KeyRelease:
            keyReleaseEvent(static_cast<QKeyEvent*>(event));
            return true;
        case QEvent::FocusOut:
            focusOutEvent(static_cast<QFocusEvent*>(event));
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
        case QEvent::Gesture:
            touchGestureEvent((QGestureEvent*)event);
            return true;
        case QEvent::Wheel:
            wheelEvent(static_cast<QWheelEvent*>(event));
            return true;
        case QEvent::Drop:
            dropEvent(static_cast<QDropEvent*>(event));
            return true;

        case QEvent::FileOpen:
            if (handleFileOpenEvent(static_cast<QFileOpenEvent*>(event))) {
                return true;
            }
            break;

        default:
            break;
    }

    return QApplication::event(event);
}

bool Application::eventFilter(QObject* object, QEvent* event) {

    if (_aboutToQuit && event->type() != QEvent::DeferredDelete && event->type() != QEvent::Destroy) {
        return true;
    }

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

    if (!event->isAutoRepeat()) {
        _keysPressed.insert(event->key(), *event);
    }

    _controllerScriptingInterface->emitKeyPressEvent(event); // send events to any registered scripts
    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isKeyCaptured(event) || isInterstitialMode()) {
        return;
    }

    if (hasFocus()) {
        if (_keyboardMouseDevice->isActive()) {
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
                        unsetFullscreen();
                    } else {
                        setFullscreen(nullptr);
                    }
                }
                break;

            case Qt::Key_1: {
                Menu* menu = Menu::getInstance();
                menu->triggerOption(MenuOption::FirstPerson);
                break;
            }
            case Qt::Key_2: {
                Menu* menu = Menu::getInstance();
                menu->triggerOption(MenuOption::FullscreenMirror);
                break;
            }
            case Qt::Key_3: {
                Menu* menu = Menu::getInstance();
                menu->triggerOption(MenuOption::ThirdPerson);
                break;
            }
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
                    offscreenUi->togglePinned();
                    //offscreenUi->getSurfaceContext()->engine()->clearComponentCache();
                    //OffscreenUi::information("Debugging", "Component cache cleared");
                    // placeholder for dialogs being converted to QML.
                }
                break;

            case Qt::Key_Y:
                if (isShifted && isMeta) {
                    getActiveDisplayPlugin()->cycleDebugOutput();
                }
                break;

            case Qt::Key_B:
                if (isMeta) {
                    auto offscreenUi = DependencyManager::get<OffscreenUi>();
                    offscreenUi->load("Browser.qml");
                } else if (isOption) {
                    controller::InputRecorder* inputRecorder = controller::InputRecorder::getInstance();
                    inputRecorder->stopPlayback();
                }
                break;

            case Qt::Key_L:
                if (isShifted && isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::Log);
                } else if (isMeta) {
                    auto dialogsManager = DependencyManager::get<DialogsManager>();
                    dialogsManager->toggleAddressBar();
                } else if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::LodTools);
                }
                break;

            case Qt::Key_R:
                if (isMeta && !event->isAutoRepeat()) {
                    DependencyManager::get<ScriptEngines>()->reloadAllScripts();
                    DependencyManager::get<OffscreenUi>()->clearCache();
                }
                break;

            case Qt::Key_Asterisk:
                Menu::getInstance()->triggerOption(MenuOption::DefaultSkybox);
                break;

            case Qt::Key_M:
                if (isMeta) {
                    auto audioClient = DependencyManager::get<AudioClient>();
                    audioClient->setMuted(!audioClient->isMuted());
                }
                break;

            case Qt::Key_N:
                if (!isOption && !isShifted && isMeta) {
                    DependencyManager::get<NodeList>()->toggleIgnoreRadius();
                }
                break;

            case Qt::Key_S:
                if (isShifted && isMeta && !isOption) {
                    Menu::getInstance()->triggerOption(MenuOption::SuppressShortTimings);
                }
                break;

            case Qt::Key_P: {
                AudioInjectorOptions options;
                options.localOnly = true;
                options.stereo = true;
                Setting::Handle<bool> notificationSounds{ MenuOption::NotificationSounds, true};
                Setting::Handle<bool> notificationSoundSnapshot{ MenuOption::NotificationSoundsSnapshot, true};
                if (notificationSounds.get() && notificationSoundSnapshot.get()) {
                    if (_snapshotSoundInjector) {
                        _snapshotSoundInjector->setOptions(options);
                        _snapshotSoundInjector->restart();
                    } else {
                        QByteArray samples = _snapshotSound->getByteArray();
                        _snapshotSoundInjector = AudioInjector::playSound(samples, options);
                    }
                }
                takeSnapshot(true);
                break;
            }

            case Qt::Key_Apostrophe: {
                if (isMeta) {
                    auto cursor = Cursor::Manager::instance().getCursor();
                    auto curIcon = cursor->getIcon();
                    if (curIcon == Cursor::Icon::DEFAULT) {
                        showCursor(Cursor::Icon::RETICLE);
                    } else if (curIcon == Cursor::Icon::RETICLE) {
                        showCursor(Cursor::Icon::SYSTEM);
                    } else if (curIcon == Cursor::Icon::SYSTEM) {
                        showCursor(Cursor::Icon::LINK);
                    } else {
                        showCursor(Cursor::Icon::DEFAULT);
                    }
                } else if (!event->isAutoRepeat()){
                    resetSensors(true);
                }
                break;
            }

            case Qt::Key_Backslash:
                Menu::getInstance()->triggerOption(MenuOption::Chat);
                break;

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
            case Qt::Key_Escape: {
                getActiveDisplayPlugin()->abandonCalibration();
                break;
            }

            default:
                event->ignore();
                break;
        }
    }
}

void Application::keyReleaseEvent(QKeyEvent* event) {
    if (!event->isAutoRepeat()) {
        _keysPressed.remove(event->key());
    }

#if defined(Q_OS_ANDROID)
    if (event->key() == Qt::Key_Back) {
        event->accept();
        AndroidHelper::instance().requestActivity("Home", false);
    }
#endif
    _controllerScriptingInterface->emitKeyReleaseEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isKeyCaptured(event)) {
        return;
    }

    if (_keyboardMouseDevice->isActive()) {
        _keyboardMouseDevice->keyReleaseEvent(event);
    }
}

void Application::focusOutEvent(QFocusEvent* event) {
    auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
    foreach(auto inputPlugin, inputPlugins) {
        if (inputPlugin->isActive()) {
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
    // Because our key event handlers may manipulate _keysPressed, lets swap the keys pressed into a local copy,
    // clearing the existing list.
    QHash<int, QKeyEvent> keysPressed;
    std::swap(keysPressed, _keysPressed);
    for (auto& ev : keysPressed) {
        QKeyEvent synthesizedEvent { QKeyEvent::KeyRelease, ev.key(), Qt::NoModifier, ev.text() };
        keyReleaseEvent(&synthesizedEvent);
    }
}

void Application::maybeToggleMenuVisible(QMouseEvent* event) const {
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
    PROFILE_RANGE(app_input_mouse, __FUNCTION__);

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
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(eventPosition);
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

    if (compositor.getReticleVisible() || !isHMDMode() || !compositor.getReticleOverDesktop() ||
        getOverlays().getOverlayAtPoint(glm::vec2(transformedPos.x(), transformedPos.y())) != UNKNOWN_OVERLAY_ID) {
        getOverlays().mouseMoveEvent(&mappedEvent);
        getEntities()->mouseMoveEvent(&mappedEvent);
    }

    _controllerScriptingInterface->emitMouseMoveEvent(&mappedEvent); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isMouseCaptured()) {
        return;
    }

    if (_keyboardMouseDevice->isActive()) {
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
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(eventPosition);
    QMouseEvent mappedEvent(event->type(),
        transformedPos,
        event->screenPos(), event->button(),
        event->buttons(), event->modifiers());

    getOverlays().mousePressEvent(&mappedEvent);
    if (!_controllerScriptingInterface->areEntityClicksCaptured()) {
        getEntities()->mousePressEvent(&mappedEvent);
    }

    _controllerScriptingInterface->emitMousePressEvent(&mappedEvent); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isMouseCaptured()) {
        return;
    }

#if defined(Q_OS_MAC)
    // Fix for OSX right click dragging on window when coming from a native window
    bool isFocussed = hasFocus();
    if (!isFocussed && event->button() == Qt::MouseButton::RightButton) {
        setFocus();
        isFocussed = true;
    }

    if (isFocussed) {
#else
    if (hasFocus()) {
#endif
        if (_keyboardMouseDevice->isActive()) {
            _keyboardMouseDevice->mousePressEvent(event);
        }
    }
}

void Application::mouseDoublePressEvent(QMouseEvent* event) {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto eventPosition = getApplicationCompositor().getMouseEventPosition(event);
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(eventPosition);
    QMouseEvent mappedEvent(event->type(),
        transformedPos,
        event->screenPos(), event->button(),
        event->buttons(), event->modifiers());

    getOverlays().mouseDoublePressEvent(&mappedEvent);
    if (!_controllerScriptingInterface->areEntityClicksCaptured()) {
        getEntities()->mouseDoublePressEvent(&mappedEvent);
    }

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isMouseCaptured()) {
        return;
    }

    _controllerScriptingInterface->emitMouseDoublePressEvent(event);
}

void Application::mouseReleaseEvent(QMouseEvent* event) {

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto eventPosition = getApplicationCompositor().getMouseEventPosition(event);
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(eventPosition);
    QMouseEvent mappedEvent(event->type(),
        transformedPos,
        event->screenPos(), event->button(),
        event->buttons(), event->modifiers());

    getOverlays().mouseReleaseEvent(&mappedEvent);
    getEntities()->mouseReleaseEvent(&mappedEvent);

    _controllerScriptingInterface->emitMouseReleaseEvent(&mappedEvent); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isMouseCaptured()) {
        return;
    }

    if (hasFocus()) {
        if (_keyboardMouseDevice->isActive()) {
            _keyboardMouseDevice->mouseReleaseEvent(event);
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

    if (_keyboardMouseDevice->isActive()) {
        _keyboardMouseDevice->touchUpdateEvent(event);
    }
    if (_touchscreenDevice && _touchscreenDevice->isActive()) {
        _touchscreenDevice->touchUpdateEvent(event);
    }
    if (_touchscreenVirtualPadDevice && _touchscreenVirtualPadDevice->isActive()) {
        _touchscreenVirtualPadDevice->touchUpdateEvent(event);
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

    if (_keyboardMouseDevice->isActive()) {
        _keyboardMouseDevice->touchBeginEvent(event);
    }
    if (_touchscreenDevice && _touchscreenDevice->isActive()) {
        _touchscreenDevice->touchBeginEvent(event);
    }
    if (_touchscreenVirtualPadDevice && _touchscreenVirtualPadDevice->isActive()) {
        _touchscreenVirtualPadDevice->touchBeginEvent(event);
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

    if (_keyboardMouseDevice->isActive()) {
        _keyboardMouseDevice->touchEndEvent(event);
    }
    if (_touchscreenDevice && _touchscreenDevice->isActive()) {
        _touchscreenDevice->touchEndEvent(event);
    }
    if (_touchscreenVirtualPadDevice && _touchscreenVirtualPadDevice->isActive()) {
        _touchscreenVirtualPadDevice->touchEndEvent(event);
    }
    // put any application specific touch behavior below here..
}

void Application::touchGestureEvent(QGestureEvent* event) {
    if (_touchscreenDevice && _touchscreenDevice->isActive()) {
        _touchscreenDevice->touchGestureEvent(event);
    }
    if (_touchscreenVirtualPadDevice && _touchscreenVirtualPadDevice->isActive()) {
        _touchscreenVirtualPadDevice->touchGestureEvent(event);
    }
}

void Application::wheelEvent(QWheelEvent* event) const {
    _altPressed = false;
    _controllerScriptingInterface->emitWheelEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface->isWheelCaptured()) {
        return;
    }

    if (_keyboardMouseDevice->isActive()) {
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

// This is currently not used, but could be invoked if the user wants to go to the place embedded in an
// Interface-taken snapshot. (It was developed for drag and drop, before we had asset-server loading or in-world browsers.)
bool Application::acceptSnapshot(const QString& urlString) {
    QUrl url(urlString);
    QString snapshotPath = url.toLocalFile();

    SnapshotMetaData* snapshotData = DependencyManager::get<Snapshot>()->parseSnapshotData(snapshotPath);
    if (snapshotData) {
        if (!snapshotData->getURL().toString().isEmpty()) {
            DependencyManager::get<AddressManager>()->handleLookupString(snapshotData->getURL().toString());
        }
    } else {
        OffscreenUi::asyncWarning("", "No location details were found in the file\n" +
                             snapshotPath + "\nTry dragging in an authentic Hifi snapshot.");
    }
    return true;
}

static uint32_t _renderedFrameIndex { INVALID_FRAME };

bool Application::shouldPaint() const {
    if (_aboutToQuit || _window->isMinimized()) {
        return false;
    }

    auto displayPlugin = getActiveDisplayPlugin();

#ifdef DEBUG_PAINT_DELAY
    static uint64_t paintDelaySamples{ 0 };
    static uint64_t paintDelayUsecs{ 0 };

    paintDelayUsecs += displayPlugin->getPaintDelayUsecs();

    static const int PAINT_DELAY_THROTTLE = 1000;
    if (++paintDelaySamples % PAINT_DELAY_THROTTLE == 0) {
        qCDebug(interfaceapp).nospace() <<
            "Paint delay (" << paintDelaySamples << " samples): " <<
            (float)paintDelaySamples / paintDelayUsecs << "us";
    }
#endif

    // Throttle if requested
    if (displayPlugin->isThrottled() && (_lastTimeRendered.elapsed() < THROTTLED_SIM_FRAME_PERIOD_MS)) {
        return false;
    }

    // Sync up the _renderedFrameIndex
    _renderedFrameIndex = displayPlugin->presentCount();
    return true;
}

#ifdef Q_OS_WIN
#include <Windows.h>
#include <TCHAR.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "ntdll.lib")

extern "C" {
    enum SYSTEM_INFORMATION_CLASS {
        SystemBasicInformation = 0,
        SystemProcessorPerformanceInformation = 8,
    };

    struct SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
        LARGE_INTEGER IdleTime;
        LARGE_INTEGER KernelTime;
        LARGE_INTEGER UserTime;
        LARGE_INTEGER DpcTime;
        LARGE_INTEGER InterruptTime;
        ULONG InterruptCount;
    };

    struct SYSTEM_BASIC_INFORMATION {
        ULONG Reserved;
        ULONG TimerResolution;
        ULONG PageSize;
        ULONG NumberOfPhysicalPages;
        ULONG LowestPhysicalPageNumber;
        ULONG HighestPhysicalPageNumber;
        ULONG AllocationGranularity;
        ULONG_PTR MinimumUserModeAddress;
        ULONG_PTR MaximumUserModeAddress;
        ULONG_PTR ActiveProcessorsAffinityMask;
        CCHAR NumberOfProcessors;
    };

    NTSYSCALLAPI NTSTATUS NTAPI NtQuerySystemInformation(
        _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
        _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
        _In_ ULONG SystemInformationLength,
        _Out_opt_ PULONG ReturnLength
    );

}
template <typename T>
NTSTATUS NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, T& t) {
    return NtQuerySystemInformation(SystemInformationClass, &t, (ULONG)sizeof(T), nullptr);
}

template <typename T>
NTSTATUS NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, std::vector<T>& t) {
    return NtQuerySystemInformation(SystemInformationClass, t.data(), (ULONG)(sizeof(T) * t.size()), nullptr);
}


template <typename T>
void updateValueAndDelta(std::pair<T, T>& pair, T newValue) {
    auto& value = pair.first;
    auto& delta = pair.second;
    delta = (value != 0) ? newValue - value : 0;
    value = newValue;
}

struct MyCpuInfo {
    using ValueAndDelta = std::pair<LONGLONG, LONGLONG>;
    std::string name;
    ValueAndDelta kernel { 0, 0 };
    ValueAndDelta user { 0, 0 };
    ValueAndDelta idle { 0, 0 };
    float kernelUsage { 0.0f };
    float userUsage { 0.0f };

    void update(const SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION& cpuInfo) {
        updateValueAndDelta(kernel, cpuInfo.KernelTime.QuadPart);
        updateValueAndDelta(user, cpuInfo.UserTime.QuadPart);
        updateValueAndDelta(idle, cpuInfo.IdleTime.QuadPart);
        auto totalTime = kernel.second + user.second + idle.second;
        if (totalTime != 0) {
            kernelUsage = (FLOAT)kernel.second / totalTime;
            userUsage = (FLOAT)user.second / totalTime;
        } else {
            kernelUsage = userUsage = 0.0f;
        }
    }
};

void updateCpuInformation() {
    static std::once_flag once;
    static SYSTEM_BASIC_INFORMATION systemInfo {};
    static SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION cpuTotals;
    static std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> cpuInfos;
    static std::vector<MyCpuInfo> myCpuInfos;
    static MyCpuInfo myCpuTotals;
    std::call_once(once, [&] {
        NtQuerySystemInformation( SystemBasicInformation, systemInfo);
        cpuInfos.resize(systemInfo.NumberOfProcessors);
        myCpuInfos.resize(systemInfo.NumberOfProcessors);
        for (size_t i = 0; i < systemInfo.NumberOfProcessors; ++i) {
            myCpuInfos[i].name = "cpu." + std::to_string(i);
        }
        myCpuTotals.name = "cpu.total";
    });
    NtQuerySystemInformation(SystemProcessorPerformanceInformation, cpuInfos);

    // Zero the CPU totals.
    memset(&cpuTotals, 0, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
    for (size_t i = 0; i < systemInfo.NumberOfProcessors; ++i) {
        auto& cpuInfo = cpuInfos[i];
        // KernelTime includes IdleTime.
        cpuInfo.KernelTime.QuadPart -= cpuInfo.IdleTime.QuadPart;

        // Update totals
        cpuTotals.IdleTime.QuadPart += cpuInfo.IdleTime.QuadPart;
        cpuTotals.KernelTime.QuadPart += cpuInfo.KernelTime.QuadPart;
        cpuTotals.UserTime.QuadPart += cpuInfo.UserTime.QuadPart;

        // Update friendly structure
        auto& myCpuInfo = myCpuInfos[i];
        myCpuInfo.update(cpuInfo);
        PROFILE_COUNTER(app, myCpuInfo.name.c_str(), {
            { "kernel", myCpuInfo.kernelUsage },
            { "user", myCpuInfo.userUsage }
        });
    }

    myCpuTotals.update(cpuTotals);
    PROFILE_COUNTER(app, myCpuTotals.name.c_str(), {
        { "kernel", myCpuTotals.kernelUsage },
        { "user", myCpuTotals.userUsage }
    });
}


static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
static HANDLE self;
static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;

void initCpuUsage() {
    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));

    self = GetCurrentProcess();
    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
    memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));

    PdhOpenQuery(NULL, NULL, &cpuQuery);
    PdhAddCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
    PdhCollectQueryData(cpuQuery);
}

void getCpuUsage(vec3& systemAndUser) {
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));
    systemAndUser.x = (sys.QuadPart - lastSysCPU.QuadPart);
    systemAndUser.y = (user.QuadPart - lastUserCPU.QuadPart);
    systemAndUser /= (float)(now.QuadPart - lastCPU.QuadPart);
    systemAndUser /= (float)numProcessors;
    systemAndUser *= 100.0f;
    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;

    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQuery);
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    systemAndUser.z = (float)counterVal.doubleValue;
}

void setupCpuMonitorThread() {
    initCpuUsage();
    auto cpuMonitorThread = QThread::currentThread();

    QTimer* timer = new QTimer();
    timer->setInterval(50);
    QObject::connect(timer, &QTimer::timeout, [] {
        updateCpuInformation();
        vec3 kernelUserAndSystem;
        getCpuUsage(kernelUserAndSystem);
        PROFILE_COUNTER(app, "cpuProcess", { { "system", kernelUserAndSystem.x }, { "user", kernelUserAndSystem.y } });
        PROFILE_COUNTER(app, "cpuSystem", { { "system", kernelUserAndSystem.z } });
    });
    QObject::connect(cpuMonitorThread, &QThread::finished, [=] {
        timer->deleteLater();
        cpuMonitorThread->deleteLater();
    });
    timer->start();
}

#endif

void Application::idle() {
    PerformanceTimer perfTimer("idle");

    // Update the deadlock watchdog
    updateHeartbeat();

    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    // These tasks need to be done on our first idle, because we don't want the showing of
    // overlay subwindows to do a showDesktop() until after the first time through
    static bool firstIdle = true;
    if (firstIdle) {
        firstIdle = false;
        connect(offscreenUi.data(), &OffscreenUi::showDesktop, this, &Application::showDesktop);
    }

#ifdef Q_OS_WIN
    // If tracing is enabled then monitor the CPU in a separate thread
    static std::once_flag once;
    std::call_once(once, [&] {
        if (trace_app().isDebugEnabled()) {
            QThread* cpuMonitorThread = new QThread(qApp);
            cpuMonitorThread->setObjectName("cpuMonitorThread");
            QObject::connect(cpuMonitorThread, &QThread::started, [this] { setupCpuMonitorThread(); });
            QObject::connect(qApp, &QCoreApplication::aboutToQuit, cpuMonitorThread, &QThread::quit);
            cpuMonitorThread->start();
        }
    });
#endif

    auto displayPlugin = getActiveDisplayPlugin();
    if (displayPlugin) {
        auto uiSize = displayPlugin->getRecommendedUiSize();
        // Bit of a hack since there's no device pixel ratio change event I can find.
        if (offscreenUi->size() != fromGlm(uiSize)) {
            qCDebug(interfaceapp) << "Device pixel ratio changed, triggering resize to " << uiSize;
            offscreenUi->resize(fromGlm(uiSize));
        }
    }

    if (displayPlugin) {
        PROFILE_COUNTER_IF_CHANGED(app, "present", float, displayPlugin->presentRate());
    }
    PROFILE_COUNTER_IF_CHANGED(app, "renderLoopRate", float, _renderLoopCounter.rate());
    PROFILE_COUNTER_IF_CHANGED(app, "currentDownloads", int, ResourceCache::getLoadingRequests().length());
    PROFILE_COUNTER_IF_CHANGED(app, "pendingDownloads", int, ResourceCache::getPendingRequestCount());
    PROFILE_COUNTER_IF_CHANGED(app, "currentProcessing", int, DependencyManager::get<StatTracker>()->getStat("Processing").toInt());
    PROFILE_COUNTER_IF_CHANGED(app, "pendingProcessing", int, DependencyManager::get<StatTracker>()->getStat("PendingProcessing").toInt());
    auto renderConfig = _renderEngine->getConfiguration();
    PROFILE_COUNTER_IF_CHANGED(render, "gpuTime", float, (float)_gpuContext->getFrameTimerGPUAverage());
    auto opaqueRangeTimer = renderConfig->getConfig("OpaqueRangeTimer");
    auto linearDepth = renderConfig->getConfig("LinearDepth");
    auto surfaceGeometry = renderConfig->getConfig("SurfaceGeometry");
    auto renderDeferred = renderConfig->getConfig("RenderDeferred");
    auto toneAndPostRangeTimer = renderConfig->getConfig("ToneAndPostRangeTimer");

    PROFILE_COUNTER(render_detail, "gpuTimes", {
        { "OpaqueRangeTimer", opaqueRangeTimer ? opaqueRangeTimer->property("gpuRunTime") : 0 },
        { "LinearDepth", linearDepth ? linearDepth->property("gpuRunTime") : 0 },
        { "SurfaceGeometry", surfaceGeometry ? surfaceGeometry->property("gpuRunTime") : 0 },
        { "RenderDeferred", renderDeferred ? renderDeferred->property("gpuRunTime") : 0 },
        { "ToneAndPostRangeTimer", toneAndPostRangeTimer ? toneAndPostRangeTimer->property("gpuRunTime") : 0 }
    });

    PROFILE_RANGE(app, __FUNCTION__);

    if (auto steamClient = PluginManager::getInstance()->getSteamClientPlugin()) {
        steamClient->runCallbacks();
    }

    float secondsSinceLastUpdate = (float)_lastTimeUpdated.nsecsElapsed() / NSECS_PER_MSEC / MSECS_PER_SECOND;
    _lastTimeUpdated.start();

    // If the offscreen Ui has something active that is NOT the root, then assume it has keyboard focus.
    if (offscreenUi && offscreenUi->getWindow()) {
        auto activeFocusItem = offscreenUi->getWindow()->activeFocusItem();
        if (_keyboardDeviceHasFocus && activeFocusItem != offscreenUi->getRootItem()) {
            _keyboardMouseDevice->pluginFocusOutEvent();
            _keyboardDeviceHasFocus = false;
        } else if (activeFocusItem == offscreenUi->getRootItem()) {
            _keyboardDeviceHasFocus = true;
        }
    }

    checkChangeCursor();

    Stats::getInstance()->updateStats();
    AnimStats::getInstance()->updateStats();

    // Normally we check PipelineWarnings, but since idle will often take more than 10ms we only show these idle timing
    // details if we're in ExtraDebugging mode. However, the ::update() and its subcomponents will show their timing
    // details normally.
    bool showWarnings = getLogger()->extraDebugging();
    PerformanceWarning warn(showWarnings, "idle()");

    {
        _gameWorkload.updateViews(_viewFrustum, getMyAvatar()->getHeadPosition());
        _gameWorkload._engine->run();
    }
    {
        PerformanceTimer perfTimer("update");
        PerformanceWarning warn(showWarnings, "Application::idle()... update()");
        static const float BIGGEST_DELTA_TIME_SECS = 0.25f;
        update(glm::clamp(secondsSinceLastUpdate, 0.0f, BIGGEST_DELTA_TIME_SECS));
    }


    // Update focus highlight for entity or overlay.
    {
        if (!_keyboardFocusedEntity.get().isInvalidID() || _keyboardFocusedOverlay.get() != UNKNOWN_OVERLAY_ID) {
            const quint64 LOSE_FOCUS_AFTER_ELAPSED_TIME = 30 * USECS_PER_SECOND; // if idle for 30 seconds, drop focus
            quint64 elapsedSinceAcceptedKeyPress = usecTimestampNow() - _lastAcceptedKeyPress;
            if (elapsedSinceAcceptedKeyPress > LOSE_FOCUS_AFTER_ELAPSED_TIME) {
                setKeyboardFocusEntity(UNKNOWN_ENTITY_ID);
                setKeyboardFocusOverlay(UNKNOWN_OVERLAY_ID);
            } else {
                // update position of highlight overlay
                if (!_keyboardFocusedEntity.get().isInvalidID()) {
                    auto entity = getEntities()->getTree()->findEntityByID(_keyboardFocusedEntity.get());
                    if (entity && _keyboardFocusHighlight) {
                        _keyboardFocusHighlight->setWorldOrientation(entity->getWorldOrientation());
                        _keyboardFocusHighlight->setWorldPosition(entity->getWorldPosition());
                    }
                } else {
                    // Only Web overlays can have focus.
                    auto overlay =
                        std::dynamic_pointer_cast<Web3DOverlay>(getOverlays().getOverlay(_keyboardFocusedOverlay.get()));
                    if (overlay && _keyboardFocusHighlight) {
                        _keyboardFocusHighlight->setWorldOrientation(overlay->getWorldOrientation());
                        _keyboardFocusHighlight->setWorldPosition(overlay->getWorldPosition());
                    }
                }
            }
        }
    }

    {
        PerformanceTimer perfTimer("pluginIdle");
        PerformanceWarning warn(showWarnings, "Application::idle()... pluginIdle()");
        getActiveDisplayPlugin()->idle();
        auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
        foreach(auto inputPlugin, inputPlugins) {
            if (inputPlugin->isActive()) {
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

    _gameLoopCounter.increment();
}

ivec2 Application::getMouse() const {
    return getApplicationCompositor().getReticlePosition();
}

FaceTracker* Application::getActiveFaceTracker() {
    auto dde = DependencyManager::get<DdeFaceTracker>();

    return dde->isActive() ? static_cast<FaceTracker*>(dde.data()) : nullptr;
}

FaceTracker* Application::getSelectedFaceTracker() {
    FaceTracker* faceTracker = nullptr;
#ifdef HAVE_DDE
    if (Menu::getInstance()->isOptionChecked(MenuOption::UseCamera)) {
        faceTracker = DependencyManager::get<DdeFaceTracker>().data();
    }
#endif
    return faceTracker;
}

void Application::setActiveFaceTracker() const {
#ifdef HAVE_DDE
    bool isMuted = Menu::getInstance()->isOptionChecked(MenuOption::MuteFaceTracking);
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

bool Application::exportEntities(const QString& filename,
                                 const QVector<EntityItemID>& entityIDs,
                                 const glm::vec3* givenOffset) {
    QHash<EntityItemID, EntityItemPointer> entities;

    auto nodeList = DependencyManager::get<NodeList>();
    const QUuid myAvatarID = nodeList->getSessionUUID();

    auto entityTree = getEntities()->getTree();
    auto exportTree = std::make_shared<EntityTree>();
    exportTree->setMyAvatar(getMyAvatar());
    exportTree->createRootElement();
    glm::vec3 root(TREE_SCALE, TREE_SCALE, TREE_SCALE);
    bool success = true;
    entityTree->withReadLock([entityIDs, entityTree, givenOffset, myAvatarID, &root, &entities, &success, &exportTree] {
        for (auto entityID : entityIDs) { // Gather entities and properties.
            auto entityItem = entityTree->findEntityByEntityItemID(entityID);
            if (!entityItem) {
                qCWarning(interfaceapp) << "Skipping export of" << entityID << "that is not in scene.";
                continue;
            }

            if (!givenOffset) {
                EntityItemID parentID = entityItem->getParentID();
                bool parentIsAvatar = (parentID == AVATAR_SELF_ID || parentID == myAvatarID);
                if (!parentIsAvatar && (parentID.isInvalidID() ||
                                        !entityIDs.contains(parentID) ||
                                        !entityTree->findEntityByEntityItemID(parentID))) {
                    // If parent wasn't selected, we want absolute position, which isn't in properties.
                    auto position = entityItem->getWorldPosition();
                    root.x = glm::min(root.x, position.x);
                    root.y = glm::min(root.y, position.y);
                    root.z = glm::min(root.z, position.z);
                }
            }
            entities[entityID] = entityItem;
        }

        if (entities.size() == 0) {
            success = false;
            return;
        }

        if (givenOffset) {
            root = *givenOffset;
        }
        for (EntityItemPointer& entityDatum : entities) {
            auto properties = entityDatum->getProperties();
            EntityItemID parentID = properties.getParentID();
            bool parentIsAvatar = (parentID == AVATAR_SELF_ID || parentID == myAvatarID);
            if (parentIsAvatar) {
                properties.setParentID(AVATAR_SELF_ID);
            } else {
                if (parentID.isInvalidID()) {
                    properties.setPosition(properties.getPosition() - root);
                } else if (!entities.contains(parentID)) {
                    entityDatum->globalizeProperties(properties, "Parent %3 of %2 %1 is not selected for export.", -root);
                } // else valid parent -- don't offset
            }
            exportTree->addEntity(entityDatum->getEntityItemID(), properties);
        }
    });
    if (success) {
        success = exportTree->writeToJSONFile(filename.toLocal8Bit().constData());

        // restore the main window's active state
        _window->activateWindow();
    }
    return success;
}

bool Application::exportEntities(const QString& filename, float x, float y, float z, float scale) {
    glm::vec3 center(x, y, z);
    glm::vec3 minCorner = center - vec3(scale);
    float cubeSize = scale * 2;
    AACube boundingCube(minCorner, cubeSize);
    QVector<EntityItemPointer> entities;
    QVector<EntityItemID> ids;
    auto entityTree = getEntities()->getTree();
    entityTree->withReadLock([&] {
        entityTree->findEntities(boundingCube, entities);
        foreach(EntityItemPointer entity, entities) {
            ids << entity->getEntityItemID();
        }
    });
    return exportEntities(filename, ids, &center);
}

void Application::loadSettings() {

    sessionRunTime.set(0); // Just clean living. We're about to saveSettings, which will update value.
    DependencyManager::get<AudioClient>()->loadSettings();
    DependencyManager::get<LODManager>()->loadSettings();

    // DONT CHECK IN
    //DependencyManager::get<LODManager>()->setAutomaticLODAdjust(false);

    auto menu = Menu::getInstance();
    menu->loadSettings();

    // override the menu option show overlays to always be true on startup
    menu->setIsOptionChecked(MenuOption::Overlays, true);

    // If there is a preferred plugin, we probably messed it up with the menu settings, so fix it.
    auto pluginManager = PluginManager::getInstance();
    auto plugins = pluginManager->getPreferredDisplayPlugins();
    if (plugins.size() > 0) {
        for (auto plugin : plugins) {
            if (auto action = menu->getActionForOption(plugin->getName())) {
                action->setChecked(true);
                action->trigger();
                // Find and activated highest priority plugin, bail for the rest
                break;
            }
        }
    }

    Setting::Handle<bool> firstRun { Settings::firstRun, true };
    bool isFirstPerson = false;
    if (firstRun.get()) {
        // If this is our first run, and no preferred devices were set, default to
        // an HMD device if available.
        auto displayPlugins = pluginManager->getDisplayPlugins();
        for (auto& plugin : displayPlugins) {
            if (plugin->isHmd()) {
                if (auto action = menu->getActionForOption(plugin->getName())) {
                    action->setChecked(true);
                    action->trigger();
                    break;
                }
            }
        }

        isFirstPerson = (qApp->isHMDMode());

    } else {
        // if this is not the first run, the camera will be initialized differently depending on user settings

        if (qApp->isHMDMode()) {
            // if the HMD is active, use first-person camera, unless the appropriate setting is checked
            isFirstPerson = menu->isOptionChecked(MenuOption::FirstPersonHMD);
        } else {
            // if HMD is not active, only use first person if the menu option is checked
            isFirstPerson = menu->isOptionChecked(MenuOption::FirstPerson);
        }
    }

    // finish initializing the camera, based on everything we checked above. Third person camera will be used if no settings
    // dictated that we should be in first person
    Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, isFirstPerson);
    Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, !isFirstPerson);
    _myCamera.setMode((isFirstPerson) ? CAMERA_MODE_FIRST_PERSON : CAMERA_MODE_THIRD_PERSON);
    cameraMenuChanged();

    auto inputs = pluginManager->getInputPlugins();
    for (auto plugin : inputs) {
        if (!plugin->isActive()) {
            plugin->activate();
        }
    }

    getMyAvatar()->loadData();
    _settingsLoaded = true;
}

void Application::saveSettings() const {
    sessionRunTime.set(_sessionRunTimer.elapsed() / MSECS_PER_SECOND);
    DependencyManager::get<AudioClient>()->saveSettings();
    DependencyManager::get<LODManager>()->saveSettings();

    Menu::getInstance()->saveSettings();
    getMyAvatar()->saveData();
    PluginManager::getInstance()->saveSettings();
}

bool Application::importEntities(const QString& urlOrFilename) {
    bool success = false;
    _entityClipboard->withWriteLock([&] {
        _entityClipboard->eraseAllOctreeElements();

        success = _entityClipboard->readFromURL(urlOrFilename);
        if (success) {
            _entityClipboard->reaverageOctreeElements();
        }
    });
    return success;
}

QVector<EntityItemID> Application::pasteEntities(float x, float y, float z) {
    return _entityClipboard->sendEntities(&_entityEditSender, getEntities()->getTree(), x, y, z);
}

void Application::init() {
    // Make sure Login state is up to date
    DependencyManager::get<DialogsManager>()->toggleLoginDialog();
    if (!DISABLE_DEFERRED) {
        DependencyManager::get<DeferredLightingEffect>()->init();
    }
    DependencyManager::get<AvatarManager>()->init();

    _timerStart.start();
    _lastTimeUpdated.start();

    if (auto steamClient = PluginManager::getInstance()->getSteamClientPlugin()) {
        // when +connect_lobby in command line, join steam lobby
        const QString STEAM_LOBBY_COMMAND_LINE_KEY = "+connect_lobby";
        int lobbyIndex = arguments().indexOf(STEAM_LOBBY_COMMAND_LINE_KEY);
        if (lobbyIndex != -1) {
            QString lobbyId = arguments().value(lobbyIndex + 1);
            steamClient->joinLobby(lobbyId);
        }
    }


    qCDebug(interfaceapp) << "Loaded settings";

    // fire off an immediate domain-server check in now that settings are loaded
    if (!isServerlessMode()) {
        DependencyManager::get<NodeList>()->sendDomainServerCheckIn();
    }

    // This allows collision to be set up properly for shape entities supported by GeometryCache.
    // This is before entity setup to ensure that it's ready for whenever instance collision is initialized.
    ShapeEntityItem::setShapeInfoCalulator(ShapeEntityItem::ShapeInfoCalculator(&shapeInfoCalculator));

    getEntities()->init();
    getEntities()->setEntityLoadingPriorityFunction([this](const EntityItem& item) {
        auto dims = item.getScaledDimensions();
        auto maxSize = glm::compMax(dims);

        if (maxSize <= 0.0f) {
            return 0.0f;
        }

        auto distance = glm::distance(getMyAvatar()->getWorldPosition(), item.getWorldPosition());
        return atan2(maxSize, distance);
    });

    ObjectMotionState::setShapeManager(&_shapeManager);
    _physicsEngine->init();

    EntityTreePointer tree = getEntities()->getTree();
    _entitySimulation->init(tree, _physicsEngine, &_entityEditSender);
    tree->setSimulation(_entitySimulation);

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    // connect the _entityCollisionSystem to our EntityTreeRenderer since that's what handles running entity scripts
    connect(_entitySimulation.get(), &PhysicalEntitySimulation::entityCollisionWithEntity,
            getEntities().data(), &EntityTreeRenderer::entityCollisionWithEntity);

    // connect the _entities (EntityTreeRenderer) to our script engine's EntityScriptingInterface for firing
    // of events related clicking, hovering over, and entering entities
    getEntities()->connectSignalsToSlots(entityScriptingInterface.data());

    // Make sure any new sounds are loaded as soon as know about them.
    connect(tree.get(), &EntityTree::newCollisionSoundURL, this, [this](QUrl newURL, EntityItemID id) {
        getEntities()->setCollisionSound(id, DependencyManager::get<SoundCache>()->getSound(newURL));
    }, Qt::QueuedConnection);
    connect(getMyAvatar().get(), &MyAvatar::newCollisionSoundURL, this, [this](QUrl newURL) {
        if (auto avatar = getMyAvatar()) {
            auto sound = DependencyManager::get<SoundCache>()->getSound(newURL);
            avatar->setCollisionSound(sound);
        }
    }, Qt::QueuedConnection);

    _gameWorkload.startup(getEntities()->getWorkloadSpace(), _main3DScene, _entitySimulation);
    _entitySimulation->setWorkloadSpace(getEntities()->getWorkloadSpace());
}

void Application::loadAvatarScripts(const QVector<QString>& urls) {
    auto scriptEngines = DependencyManager::get<ScriptEngines>();
    auto runningScripts = scriptEngines->getRunningScripts();
    for (auto url : urls) {
        int index = runningScripts.indexOf(url);
        if (index < 0) {
            auto scriptEnginePointer = scriptEngines->loadScript(url, false);
            if (scriptEnginePointer) {
                scriptEnginePointer->setType(ScriptEngine::Type::AVATAR);
            }
        }
    }
}

void Application::unloadAvatarScripts() {
    auto scriptEngines = DependencyManager::get<ScriptEngines>();
    auto urls = scriptEngines->getRunningScripts();
    for (auto url : urls) {
        auto scriptEngine = scriptEngines->getScriptEngine(url);
        if (scriptEngine->getType() == ScriptEngine::Type::AVATAR) {
            scriptEngines->stopScript(url, false);
        }
    }
}

void Application::updateLOD(float deltaTime) const {
    PerformanceTimer perfTimer("LOD");
    // adjust it unless we were asked to disable this feature, or if we're currently in throttleRendering mode
    if (!isThrottleRendering()) {
        float presentTime = getActiveDisplayPlugin()->getAveragePresentTime();
        float engineRunTime = (float)(_renderEngine->getConfiguration().get()->getCPURunTime());
        float gpuTime = getGPUContext()->getFrameTimerGPUAverage();
        float batchTime = getGPUContext()->getFrameTimerBatchAverage();
        auto lodManager = DependencyManager::get<LODManager>();
        lodManager->setRenderTimes(presentTime, engineRunTime, batchTime, gpuTime);
        lodManager->autoAdjustLOD(deltaTime);
    } else {
        DependencyManager::get<LODManager>()->resetLODAdjust();
    }
}

void Application::pushPostUpdateLambda(void* key, const std::function<void()>& func) {
    std::unique_lock<std::mutex> guard(_postUpdateLambdasLock);
    _postUpdateLambdas[key] = func;
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
            // TODO -- this code is probably wrong, getHeadPose() returns something in sensor frame, not avatar
            glm::mat4 headPose = getActiveDisplayPlugin()->getHeadPose();
            glm::quat hmdRotation = glm::quat_cast(headPose);
            lookAtSpot = _myCamera.getPosition() + myAvatar->getWorldOrientation() * (hmdRotation * lookAtPosition);
        } else {
            lookAtSpot = myAvatar->getHead()->getEyePosition()
                + (myAvatar->getHead()->getFinalOrientationInWorldFrame() * lookAtPosition);
        }
    } else {
        AvatarSharedPointer lookingAt = myAvatar->getLookAtTargetAvatar().lock();
        bool haveLookAtCandidate = lookingAt && myAvatar.get() != lookingAt.get();
        auto avatar = static_pointer_cast<Avatar>(lookingAt);
        bool mutualLookAtSnappingEnabled = avatar && avatar->getLookAtSnappingEnabled() && myAvatar->getLookAtSnappingEnabled();
        if (haveLookAtCandidate && mutualLookAtSnappingEnabled) {
            //  If I am looking at someone else, look directly at one of their eyes
            isLookingAtSomeone = true;
            auto lookingAtHead = avatar->getHead();

            const float MAXIMUM_FACE_ANGLE = 65.0f * RADIANS_PER_DEGREE;
            glm::vec3 lookingAtFaceOrientation = lookingAtHead->getFinalOrientationInWorldFrame() * IDENTITY_FORWARD;
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
            auto headPose = myAvatar->getControllerPoseInWorldFrame(controller::Action::HEAD);
            if (headPose.isValid()) {
                lookAtSpot = transformPoint(headPose.getMatrix(), glm::vec3(0.0f, 0.0f, TREE_SCALE));
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
            lookAtSpot = origin + _myCamera.getOrientation() * glm::quat(glm::radians(glm::vec3(
                eyePitch * deflection, eyeYaw * deflection, 0.0f))) *
                glm::inverse(_myCamera.getOrientation()) * (lookAtSpot - origin);
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
    auto menu = Menu::getInstance();
    menu->setIsOptionChecked(MenuOption::Overlays, !menu->isOptionChecked(MenuOption::Overlays));
}

void Application::setOverlaysVisible(bool visible) {
    auto menu = Menu::getInstance();
    menu->setIsOptionChecked(MenuOption::Overlays, visible);
}

void Application::centerUI() {
    _overlayConductor.centerUI();
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

void Application::cameraModeChanged() {
    switch (_myCamera.getMode()) {
        case CAMERA_MODE_FIRST_PERSON:
            Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, true);
            break;
        case CAMERA_MODE_THIRD_PERSON:
            Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
            break;
        case CAMERA_MODE_MIRROR:
            Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, true);
            break;
        case CAMERA_MODE_INDEPENDENT:
            Menu::getInstance()->setIsOptionChecked(MenuOption::IndependentMode, true);
            break;
        case CAMERA_MODE_ENTITY:
            Menu::getInstance()->setIsOptionChecked(MenuOption::CameraEntityMode, true);
            break;
        default:
            break;
    }
    cameraMenuChanged();
}

void Application::changeViewAsNeeded(float boomLength) {
    // Switch between first and third person views as needed
    // This is called when the boom length has changed
    bool boomLengthGreaterThanMinimum = (boomLength > MyAvatar::ZOOM_MIN);

    if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON && boomLengthGreaterThanMinimum) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, false);
        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
        cameraMenuChanged();
    } else if (_myCamera.getMode() == CAMERA_MODE_THIRD_PERSON && !boomLengthGreaterThanMinimum) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, true);
        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, false);
        cameraMenuChanged();
    }
}

void Application::cameraMenuChanged() {
    auto menu = Menu::getInstance();
    if (menu->isOptionChecked(MenuOption::FullscreenMirror)) {
        if (!isHMDMode() && _myCamera.getMode() != CAMERA_MODE_MIRROR) {
            _mirrorYawOffset = 0.0f;
            _myCamera.setMode(CAMERA_MODE_MIRROR);
            getMyAvatar()->reset(false, false, false); // to reset any active MyAvatar::FollowHelpers
            getMyAvatar()->setBoomLength(MyAvatar::ZOOM_DEFAULT);
        }
    } else if (menu->isOptionChecked(MenuOption::FirstPerson)) {
        if (_myCamera.getMode() != CAMERA_MODE_FIRST_PERSON) {
            _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
            getMyAvatar()->setBoomLength(MyAvatar::ZOOM_MIN);
        }
    } else if (menu->isOptionChecked(MenuOption::ThirdPerson)) {
        if (_myCamera.getMode() != CAMERA_MODE_THIRD_PERSON) {
            _myCamera.setMode(CAMERA_MODE_THIRD_PERSON);
            if (getMyAvatar()->getBoomLength() == MyAvatar::ZOOM_MIN) {
                getMyAvatar()->setBoomLength(MyAvatar::ZOOM_DEFAULT);
            }
        }
    } else if (menu->isOptionChecked(MenuOption::IndependentMode)) {
        if (_myCamera.getMode() != CAMERA_MODE_INDEPENDENT) {
            _myCamera.setMode(CAMERA_MODE_INDEPENDENT);
        }
    } else if (menu->isOptionChecked(MenuOption::CameraEntityMode)) {
        if (_myCamera.getMode() != CAMERA_MODE_ENTITY) {
            _myCamera.setMode(CAMERA_MODE_ENTITY);
        }
    }
}

void Application::resetPhysicsReadyInformation() {
    // we've changed domains or cleared out caches or something.  we no longer know enough about the
    // collision information of nearby entities to make running bullet be safe.
    _fullSceneReceivedCounter = 0;
    _fullSceneCounterAtLastPhysicsCheck = 0;
    _nearbyEntitiesCountAtLastPhysicsCheck = 0;
    _nearbyEntitiesStabilityCount = 0;
    _physicsEnabled = false;
    _octreeProcessor.startEntitySequence();
}


void Application::reloadResourceCaches() {
    resetPhysicsReadyInformation();

    // Query the octree to refresh everything in view
    _queryExpiry = SteadyClock::now();
    _octreeQuery.incrementConnectionID();

    queryOctree(NodeType::EntityServer, PacketType::EntityQuery);

    DependencyManager::get<AssetClient>()->clearCache();
    DependencyManager::get<ScriptCache>()->clearCache();

    DependencyManager::get<AnimationCache>()->refreshAll();
    DependencyManager::get<ModelCache>()->refreshAll();
    DependencyManager::get<SoundCache>()->refreshAll();
    DependencyManager::get<TextureCache>()->refreshAll();

    DependencyManager::get<NodeList>()->reset();  // Force redownload of .fst models

    getMyAvatar()->resetFullAvatarURL();
}

void Application::rotationModeChanged() const {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::CenterPlayerInView)) {
        getMyAvatar()->setHeadPitch(0);
    }
}

void Application::setKeyboardFocusHighlight(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& dimensions) {
    // Create focus
    if (_keyboardFocusHighlightID == UNKNOWN_OVERLAY_ID || !getOverlays().isAddedOverlay(_keyboardFocusHighlightID)) {
        _keyboardFocusHighlight = std::make_shared<Cube3DOverlay>();
        _keyboardFocusHighlight->setAlpha(1.0f);
        _keyboardFocusHighlight->setColor({ 0xFF, 0xEF, 0x00 });
        _keyboardFocusHighlight->setIsSolid(false);
        _keyboardFocusHighlight->setPulseMin(0.5);
        _keyboardFocusHighlight->setPulseMax(1.0);
        _keyboardFocusHighlight->setColorPulse(1.0);
        _keyboardFocusHighlight->setIgnorePickIntersection(true);
        _keyboardFocusHighlight->setDrawInFront(false);
        _keyboardFocusHighlightID = getOverlays().addOverlay(_keyboardFocusHighlight);
    }

    // Position focus
    _keyboardFocusHighlight->setWorldOrientation(rotation);
    _keyboardFocusHighlight->setWorldPosition(position);
    _keyboardFocusHighlight->setDimensions(dimensions);
    _keyboardFocusHighlight->setVisible(true);
}

QUuid Application::getKeyboardFocusEntity() const {
    return _keyboardFocusedEntity.get();
}

static const float FOCUS_HIGHLIGHT_EXPANSION_FACTOR = 1.05f;

void Application::setKeyboardFocusEntity(const EntityItemID& entityItemID) {
    if (_keyboardFocusedEntity.get() != entityItemID) {
        _keyboardFocusedEntity.set(entityItemID);

        if (_keyboardFocusHighlight && _keyboardFocusedOverlay.get() == UNKNOWN_OVERLAY_ID) {
            _keyboardFocusHighlight->setVisible(false);
        }

        if (entityItemID == UNKNOWN_ENTITY_ID) {
            return;
        }

        auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
        auto properties = entityScriptingInterface->getEntityProperties(entityItemID);
        if (!properties.getLocked() && properties.getVisible()) {

            auto entities = getEntities();
            auto entityId = _keyboardFocusedEntity.get();
            if (entities->wantsKeyboardFocus(entityId)) {
                entities->setProxyWindow(entityId, _window->windowHandle());
                if (_keyboardMouseDevice->isActive()) {
                    _keyboardMouseDevice->pluginFocusOutEvent();
                }
                _lastAcceptedKeyPress = usecTimestampNow();

                auto entity = getEntities()->getEntity(entityId);
                if (entity) {
                    setKeyboardFocusHighlight(entity->getWorldPosition(), entity->getWorldOrientation(),
                        entity->getScaledDimensions() * FOCUS_HIGHLIGHT_EXPANSION_FACTOR);
                }
            }
        }
    }
}

OverlayID Application::getKeyboardFocusOverlay() {
    return _keyboardFocusedOverlay.get();
}

void Application::setKeyboardFocusOverlay(const OverlayID& overlayID) {
    if (overlayID != _keyboardFocusedOverlay.get()) {
        _keyboardFocusedOverlay.set(overlayID);

        if (_keyboardFocusHighlight && _keyboardFocusedEntity.get() == UNKNOWN_ENTITY_ID) {
            _keyboardFocusHighlight->setVisible(false);
        }

        if (overlayID == UNKNOWN_OVERLAY_ID) {
            return;
        }

        auto overlayType = getOverlays().getOverlayType(overlayID);
        auto isVisible = getOverlays().getProperty(overlayID, "visible").value.toBool();
        if (overlayType == Web3DOverlay::TYPE && isVisible) {
            auto overlay = std::dynamic_pointer_cast<Web3DOverlay>(getOverlays().getOverlay(overlayID));
            overlay->setProxyWindow(_window->windowHandle());

            if (_keyboardMouseDevice->isActive()) {
                _keyboardMouseDevice->pluginFocusOutEvent();
            }
            _lastAcceptedKeyPress = usecTimestampNow();

            if (overlay->getProperty("showKeyboardFocusHighlight").toBool()) {
                auto size = overlay->getSize() * FOCUS_HIGHLIGHT_EXPANSION_FACTOR;
                const float OVERLAY_DEPTH = 0.0105f;
                setKeyboardFocusHighlight(overlay->getWorldPosition(), overlay->getWorldOrientation(), glm::vec3(size.x, size.y, OVERLAY_DEPTH));
            } else if (_keyboardFocusHighlight) {
                _keyboardFocusHighlight->setVisible(false);
            }
        }
    }
}

void Application::updateDialogs(float deltaTime) const {
    PerformanceTimer perfTimer("updateDialogs");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateDialogs()");
    auto dialogsManager = DependencyManager::get<DialogsManager>();

    QPointer<OctreeStatsDialog> octreeStatsDialog = dialogsManager->getOctreeStatsDialog();
    if (octreeStatsDialog) {
        octreeStatsDialog->update();
    }
}

void Application::updateSecondaryCameraViewFrustum() {
    // TODO: Fix this by modeling the way the secondary camera works on how the main camera works
    // ie. Use a camera object stored in the game logic and informs the Engine on where the secondary
    // camera should be.

    // Code based on SecondaryCameraJob
    auto renderConfig = _renderEngine->getConfiguration();
    assert(renderConfig);
    auto camera = dynamic_cast<SecondaryCameraJobConfig*>(renderConfig->getConfig("SecondaryCamera"));

    if (!camera || !camera->isEnabled()) {
        return;
    }

    ViewFrustum secondaryViewFrustum;
    if (camera->mirrorProjection && !camera->attachedEntityId.isNull()) {
        auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
        auto entityProperties = entityScriptingInterface->getEntityProperties(camera->attachedEntityId);
        glm::vec3 mirrorPropertiesPosition = entityProperties.getPosition();
        glm::quat mirrorPropertiesRotation = entityProperties.getRotation();
        glm::vec3 mirrorPropertiesDimensions = entityProperties.getDimensions();
        glm::vec3 halfMirrorPropertiesDimensions = 0.5f * mirrorPropertiesDimensions;

        // setup mirror from world as inverse of world from mirror transformation using inverted x and z for mirrored image
        // TODO: we are assuming here that UP is world y-axis
        glm::mat4 worldFromMirrorRotation = glm::mat4_cast(mirrorPropertiesRotation) * glm::scale(vec3(-1.0f, 1.0f, -1.0f));
        glm::mat4 worldFromMirrorTranslation = glm::translate(mirrorPropertiesPosition);
        glm::mat4 worldFromMirror = worldFromMirrorTranslation * worldFromMirrorRotation;
        glm::mat4 mirrorFromWorld = glm::inverse(worldFromMirror);

        // get mirror camera position by reflecting main camera position's z coordinate in mirror space
        glm::vec3 mainCameraPositionWorld = getCamera().getPosition();
        glm::vec3 mainCameraPositionMirror = vec3(mirrorFromWorld * vec4(mainCameraPositionWorld, 1.0f));
        glm::vec3 mirrorCameraPositionMirror = vec3(mainCameraPositionMirror.x, mainCameraPositionMirror.y,
                                                    -mainCameraPositionMirror.z);
        glm::vec3 mirrorCameraPositionWorld = vec3(worldFromMirror * vec4(mirrorCameraPositionMirror, 1.0f));

        // set frustum position to be mirrored camera and set orientation to mirror's adjusted rotation
        glm::quat mirrorCameraOrientation = glm::quat_cast(worldFromMirrorRotation);
        secondaryViewFrustum.setPosition(mirrorCameraPositionWorld);
        secondaryViewFrustum.setOrientation(mirrorCameraOrientation);

        // build frustum using mirror space translation of mirrored camera
        float nearClip = mirrorCameraPositionMirror.z + mirrorPropertiesDimensions.z * 2.0f;
        glm::vec3 upperRight = halfMirrorPropertiesDimensions - mirrorCameraPositionMirror;
        glm::vec3 bottomLeft = -halfMirrorPropertiesDimensions - mirrorCameraPositionMirror;
        glm::mat4 frustum = glm::frustum(bottomLeft.x, upperRight.x, bottomLeft.y, upperRight.y, nearClip, camera->farClipPlaneDistance);
        secondaryViewFrustum.setProjection(frustum);
    } else {
        if (!camera->attachedEntityId.isNull()) {
            auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
            auto entityProperties = entityScriptingInterface->getEntityProperties(camera->attachedEntityId);
            secondaryViewFrustum.setPosition(entityProperties.getPosition());
            secondaryViewFrustum.setOrientation(entityProperties.getRotation());
        } else {
            secondaryViewFrustum.setPosition(camera->position);
            secondaryViewFrustum.setOrientation(camera->orientation);
        }

        float aspectRatio = (float)camera->textureWidth / (float)camera->textureHeight;
        secondaryViewFrustum.setProjection(camera->vFoV,
                                            aspectRatio,
                                            camera->nearClipPlaneDistance,
                                            camera->farClipPlaneDistance);
    }
    // Without calculating the bound planes, the secondary camera will use the same culling frustum as the main camera,
    // which is not what we want here.
    secondaryViewFrustum.calculate();

    _conicalViews.push_back(secondaryViewFrustum);
}

static bool domainLoadingInProgress = false;

void Application::update(float deltaTime) {
    PROFILE_RANGE_EX(app, __FUNCTION__, 0xffff0000, (uint64_t)_renderFrameCount + 1);

    if (_aboutToQuit) {
        return;
    }


    if (!_physicsEnabled) {
        if (!domainLoadingInProgress) {
            PROFILE_ASYNC_BEGIN(app, "Scene Loading", "");
            domainLoadingInProgress = true;
        }

        // we haven't yet enabled physics.  we wait until we think we have all the collision information
        // for nearby entities before starting bullet up.
        quint64 now = usecTimestampNow();
        if (isServerlessMode() || _octreeProcessor.isLoadSequenceComplete()) {
            // we've received a new full-scene octree stats packet, or it's been long enough to try again anyway
            _lastPhysicsCheckTime = now;
            _fullSceneCounterAtLastPhysicsCheck = _fullSceneReceivedCounter;
            _lastQueriedViews.clear();  // Force new view.

            // process octree stats packets are sent in between full sends of a scene (this isn't currently true).
            // We keep physics disabled until we've received a full scene and everything near the avatar in that
            // scene is ready to compute its collision shape.
            if (getMyAvatar()->isReadyForPhysics()) {
                _physicsEnabled = true;
                setIsInterstitialMode(false);
                getMyAvatar()->updateMotionBehaviorFromMenu();
            }
        }
    } else if (domainLoadingInProgress) {
        domainLoadingInProgress = false;
        PROFILE_ASYNC_END(app, "Scene Loading", "");
    }

    auto myAvatar = getMyAvatar();
    {
        PerformanceTimer perfTimer("devices");

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
                auto audioClient = DependencyManager::get<AudioClient>();
                if (menu->isOptionChecked(MenuOption::AutoMuteAudio) && !audioClient->isMuted()) {
                    if (_lastFaceTrackerUpdate > 0
                        && ((usecTimestampNow() - _lastFaceTrackerUpdate) > MUTE_MICROPHONE_AFTER_USECS)) {
                        audioClient->setMuted(true);
                        _lastFaceTrackerUpdate = 0;
                    }
                } else {
                    _lastFaceTrackerUpdate = 0;
                }
            }
        } else {
            _lastFaceTrackerUpdate = 0;
        }

        auto userInputMapper = DependencyManager::get<UserInputMapper>();

        controller::InputCalibrationData calibrationData = {
            myAvatar->getSensorToWorldMatrix(),
            createMatFromQuatAndPos(myAvatar->getWorldOrientation(), myAvatar->getWorldPosition()),
            myAvatar->getHMDSensorMatrix(),
            myAvatar->getCenterEyeCalibrationMat(),
            myAvatar->getHeadCalibrationMat(),
            myAvatar->getSpine2CalibrationMat(),
            myAvatar->getHipsCalibrationMat(),
            myAvatar->getLeftFootCalibrationMat(),
            myAvatar->getRightFootCalibrationMat(),
            myAvatar->getRightArmCalibrationMat(),
            myAvatar->getLeftArmCalibrationMat(),
            myAvatar->getRightHandCalibrationMat(),
            myAvatar->getLeftHandCalibrationMat()
        };

        InputPluginPointer keyboardMousePlugin;
        for (auto inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
            if (inputPlugin->getName() == KeyboardMouseDevice::NAME) {
                keyboardMousePlugin = inputPlugin;
            } else if (inputPlugin->isActive()) {
                inputPlugin->pluginUpdate(deltaTime, calibrationData);
            }
        }

        userInputMapper->setInputCalibrationData(calibrationData);
        userInputMapper->update(deltaTime);

        if (keyboardMousePlugin && keyboardMousePlugin->isActive()) {
            keyboardMousePlugin->pluginUpdate(deltaTime, calibrationData);
        }

        // Transfer the user inputs to the driveKeys
        // FIXME can we drop drive keys and just have the avatar read the action states directly?
        myAvatar->clearDriveKeys();
        if (_myCamera.getMode() != CAMERA_MODE_INDEPENDENT && !isInterstitialMode()) {
            if (!_controllerScriptingInterface->areActionsCaptured() && _myCamera.getMode() != CAMERA_MODE_MIRROR) {
                myAvatar->setDriveKey(MyAvatar::TRANSLATE_Z, -1.0f * userInputMapper->getActionState(controller::Action::TRANSLATE_Z));
                myAvatar->setDriveKey(MyAvatar::TRANSLATE_Y, userInputMapper->getActionState(controller::Action::TRANSLATE_Y));
                myAvatar->setDriveKey(MyAvatar::TRANSLATE_X, userInputMapper->getActionState(controller::Action::TRANSLATE_X));
                if (deltaTime > FLT_EPSILON) {
                    myAvatar->setDriveKey(MyAvatar::PITCH, -1.0f * userInputMapper->getActionState(controller::Action::PITCH));
                    myAvatar->setDriveKey(MyAvatar::YAW, -1.0f * userInputMapper->getActionState(controller::Action::YAW));
                    myAvatar->setDriveKey(MyAvatar::STEP_YAW, -1.0f * userInputMapper->getActionState(controller::Action::STEP_YAW));
                }
            }
            myAvatar->setDriveKey(MyAvatar::ZOOM, userInputMapper->getActionState(controller::Action::TRANSLATE_CAMERA_Z));
        }

        myAvatar->setSprintMode((bool)userInputMapper->getActionState(controller::Action::SPRINT));
        static const std::vector<controller::Action> avatarControllerActions = {
            controller::Action::LEFT_HAND,
            controller::Action::RIGHT_HAND,
            controller::Action::LEFT_FOOT,
            controller::Action::RIGHT_FOOT,
            controller::Action::HIPS,
            controller::Action::SPINE2,
            controller::Action::HEAD,
            controller::Action::LEFT_HAND_THUMB1,
            controller::Action::LEFT_HAND_THUMB2,
            controller::Action::LEFT_HAND_THUMB3,
            controller::Action::LEFT_HAND_THUMB4,
            controller::Action::LEFT_HAND_INDEX1,
            controller::Action::LEFT_HAND_INDEX2,
            controller::Action::LEFT_HAND_INDEX3,
            controller::Action::LEFT_HAND_INDEX4,
            controller::Action::LEFT_HAND_MIDDLE1,
            controller::Action::LEFT_HAND_MIDDLE2,
            controller::Action::LEFT_HAND_MIDDLE3,
            controller::Action::LEFT_HAND_MIDDLE4,
            controller::Action::LEFT_HAND_RING1,
            controller::Action::LEFT_HAND_RING2,
            controller::Action::LEFT_HAND_RING3,
            controller::Action::LEFT_HAND_RING4,
            controller::Action::LEFT_HAND_PINKY1,
            controller::Action::LEFT_HAND_PINKY2,
            controller::Action::LEFT_HAND_PINKY3,
            controller::Action::LEFT_HAND_PINKY4,
            controller::Action::RIGHT_HAND_THUMB1,
            controller::Action::RIGHT_HAND_THUMB2,
            controller::Action::RIGHT_HAND_THUMB3,
            controller::Action::RIGHT_HAND_THUMB4,
            controller::Action::RIGHT_HAND_INDEX1,
            controller::Action::RIGHT_HAND_INDEX2,
            controller::Action::RIGHT_HAND_INDEX3,
            controller::Action::RIGHT_HAND_INDEX4,
            controller::Action::RIGHT_HAND_MIDDLE1,
            controller::Action::RIGHT_HAND_MIDDLE2,
            controller::Action::RIGHT_HAND_MIDDLE3,
            controller::Action::RIGHT_HAND_MIDDLE4,
            controller::Action::RIGHT_HAND_RING1,
            controller::Action::RIGHT_HAND_RING2,
            controller::Action::RIGHT_HAND_RING3,
            controller::Action::RIGHT_HAND_RING4,
            controller::Action::RIGHT_HAND_PINKY1,
            controller::Action::RIGHT_HAND_PINKY2,
            controller::Action::RIGHT_HAND_PINKY3,
            controller::Action::RIGHT_HAND_PINKY4,
            controller::Action::LEFT_ARM,
            controller::Action::RIGHT_ARM,
            controller::Action::LEFT_SHOULDER,
            controller::Action::RIGHT_SHOULDER,
            controller::Action::LEFT_FORE_ARM,
            controller::Action::RIGHT_FORE_ARM,
            controller::Action::LEFT_LEG,
            controller::Action::RIGHT_LEG,
            controller::Action::LEFT_UP_LEG,
            controller::Action::RIGHT_UP_LEG,
            controller::Action::LEFT_TOE_BASE,
            controller::Action::RIGHT_TOE_BASE
        };

        // copy controller poses from userInputMapper to myAvatar.
        glm::mat4 myAvatarMatrix = createMatFromQuatAndPos(myAvatar->getWorldOrientation(), myAvatar->getWorldPosition());
        glm::mat4 worldToSensorMatrix = glm::inverse(myAvatar->getSensorToWorldMatrix());
        glm::mat4 avatarToSensorMatrix = worldToSensorMatrix * myAvatarMatrix;
        for (auto& action : avatarControllerActions) {
            controller::Pose pose = userInputMapper->getPoseState(action);
            myAvatar->setControllerPoseInSensorFrame(action, pose.transform(avatarToSensorMatrix));
        }
    }

    updateThreads(deltaTime); // If running non-threaded, then give the threads some time to process...
    updateDialogs(deltaTime); // update various stats dialogs if present

    QSharedPointer<AvatarManager> avatarManager = DependencyManager::get<AvatarManager>();

    {
        PROFILE_RANGE(simulation_physics, "Simulation");
        PerformanceTimer perfTimer("simulation");

        if (_physicsEnabled) {
            auto t0 = std::chrono::high_resolution_clock::now();
            auto t1 = t0;
            {
                PROFILE_RANGE(simulation_physics, "PrePhysics");
                PerformanceTimer perfTimer("prePhysics)");
                {
                    const VectorOfMotionStates& motionStates = _entitySimulation->getObjectsToRemoveFromPhysics();
                    _physicsEngine->removeObjects(motionStates);
                    _entitySimulation->deleteObjectsRemovedFromPhysics();
                }

                VectorOfMotionStates motionStates;
                getEntities()->getTree()->withReadLock([&] {
                    _entitySimulation->getObjectsToAddToPhysics(motionStates);
                    _physicsEngine->addObjects(motionStates);

                });
                getEntities()->getTree()->withReadLock([&] {
                    _entitySimulation->getObjectsToChange(motionStates);
                    VectorOfMotionStates stillNeedChange = _physicsEngine->changeObjects(motionStates);
                    _entitySimulation->setObjectsToChange(stillNeedChange);
                });

                _entitySimulation->applyDynamicChanges();

                t1 = std::chrono::high_resolution_clock::now();

                PhysicsEngine::Transaction transaction;
                avatarManager->buildPhysicsTransaction(transaction);
                _physicsEngine->processTransaction(transaction);
                avatarManager->handleProcessedPhysicsTransaction(transaction);

                myAvatar->prepareForPhysicsSimulation();
                _physicsEngine->forEachDynamic([&](EntityDynamicPointer dynamic) {
                    dynamic->prepareForPhysicsSimulation();
                });
            }
            auto t2 = std::chrono::high_resolution_clock::now();
            {
                PROFILE_RANGE(simulation_physics, "StepPhysics");
                PerformanceTimer perfTimer("stepPhysics");
                getEntities()->getTree()->withWriteLock([&] {
                    _physicsEngine->stepSimulation();
                });
            }
            auto t3 = std::chrono::high_resolution_clock::now();
            {
                if (_physicsEngine->hasOutgoingChanges()) {
                    {
                        PROFILE_RANGE(simulation_physics, "PostPhysics");
                        PerformanceTimer perfTimer("postPhysics");
                        // grab the collision events BEFORE handleChangedMotionStates() because at this point
                        // we have a better idea of which objects we own or should own.
                        auto& collisionEvents = _physicsEngine->getCollisionEvents();

                        getEntities()->getTree()->withWriteLock([&] {
                            PROFILE_RANGE(simulation_physics, "HandleChanges");
                            PerformanceTimer perfTimer("handleChanges");

                            const VectorOfMotionStates& outgoingChanges = _physicsEngine->getChangedMotionStates();
                            _entitySimulation->handleChangedMotionStates(outgoingChanges);
                            avatarManager->handleChangedMotionStates(outgoingChanges);

                            const VectorOfMotionStates& deactivations = _physicsEngine->getDeactivatedMotionStates();
                            _entitySimulation->handleDeactivatedMotionStates(deactivations);
                        });

                        // handleCollisionEvents() AFTER handleChangedMotionStates()
                        {
                            PROFILE_RANGE(simulation_physics, "CollisionEvents");
                            avatarManager->handleCollisionEvents(collisionEvents);
                            // Collision events (and their scripts) must not be handled when we're locked, above. (That would risk
                            // deadlock.)
                            _entitySimulation->handleCollisionEvents(collisionEvents);
                        }

                        {
                            PROFILE_RANGE(simulation_physics, "MyAvatar");
                            myAvatar->harvestResultsFromPhysicsSimulation(deltaTime);
                        }

                        if (PerformanceTimer::isActive() &&
                                Menu::getInstance()->isOptionChecked(MenuOption::DisplayDebugTimingDetails) &&
                                Menu::getInstance()->isOptionChecked(MenuOption::ExpandPhysicsTiming)) {
                            _physicsEngine->harvestPerformanceStats();
                        }
                        // NOTE: the PhysicsEngine stats are written to stdout NOT to Qt log framework
                        _physicsEngine->dumpStatsIfNecessary();
                    }
                    auto t4 = std::chrono::high_resolution_clock::now();

                    // NOTE: the getEntities()->update() call below will wait for lock
                    // and will provide non-physical entity motion
                    getEntities()->update(true); // update the models...

                    auto t5 = std::chrono::high_resolution_clock::now();

                    workload::Timings timings(6);
                    timings[0] = t1 - t0; // prePhysics entities
                    timings[1] = t2 - t1; // prePhysics avatars
                    timings[2] = t3 - t2; // stepPhysics
                    timings[3] = t4 - t3; // postPhysics
                    timings[4] = t5 - t4; // non-physical kinematics
                    timings[5] = workload::Timing_ns((int32_t)(NSECS_PER_SECOND * deltaTime)); // game loop duration
                    _gameWorkload.updateSimulationTimings(timings);
                }
            }
        } else {
            // update the rendering without any simulation
            getEntities()->update(false);
        }
    }

    // AvatarManager update
    {
        {
            PROFILE_RANGE(simulation, "OtherAvatars");
            PerformanceTimer perfTimer("otherAvatars");
            avatarManager->updateOtherAvatars(deltaTime);
        }

        {
            PROFILE_RANGE(simulation, "MyAvatar");
            PerformanceTimer perfTimer("MyAvatar");
            qApp->updateMyAvatarLookAtPosition();
            avatarManager->updateMyAvatar(deltaTime);
        }
    }

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::update()");

    updateLOD(deltaTime);

    // TODO: break these out into distinct perfTimers when they prove interesting
    {
        PROFILE_RANGE(app, "PickManager");
        PerformanceTimer perfTimer("pickManager");
        DependencyManager::get<PickManager>()->update();
    }

    {
        PROFILE_RANGE(app, "PointerManager");
        PerformanceTimer perfTimer("pointerManager");
        DependencyManager::get<PointerManager>()->update();
    }

    {
        PROFILE_RANGE_EX(app, "Overlays", 0xffff0000, (uint64_t)getActiveDisplayPlugin()->presentCount());
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
        QMutexLocker viewLocker(&_viewMutex);
        _myCamera.loadViewFrustum(_viewFrustum);

        _conicalViews.clear();
        _conicalViews.push_back(_viewFrustum);
        // TODO: Fix this by modeling the way the secondary camera works on how the main camera works
        // ie. Use a camera object stored in the game logic and informs the Engine on where the secondary
        // camera should be.
        updateSecondaryCameraViewFrustum();
    }

    quint64 now = usecTimestampNow();

    // Update my voxel servers with my current voxel query...
    {
        PROFILE_RANGE_EX(app, "QueryOctree", 0xffff0000, (uint64_t)getActiveDisplayPlugin()->presentCount());
        PerformanceTimer perfTimer("queryOctree");
        QMutexLocker viewLocker(&_viewMutex);

        bool viewIsDifferentEnough = false;
        if (_conicalViews.size() == _lastQueriedViews.size()) {
            for (size_t i = 0; i < _conicalViews.size(); ++i) {
                if (!_conicalViews[i].isVerySimilar(_lastQueriedViews[i])) {
                    viewIsDifferentEnough = true;
                    break;
                }
            }
        } else {
            viewIsDifferentEnough = true;
        }


        // if it's been a while since our last query or the view has significantly changed then send a query, otherwise suppress it
        static const std::chrono::seconds MIN_PERIOD_BETWEEN_QUERIES { 3 };
        auto now = SteadyClock::now();
        if (now > _queryExpiry || viewIsDifferentEnough) {
            if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
                queryOctree(NodeType::EntityServer, PacketType::EntityQuery);
            }
            queryAvatars();

            _lastQueriedViews = _conicalViews;
            _queryExpiry = now + MIN_PERIOD_BETWEEN_QUERIES;
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
        if (sinceLastNack > TOO_LONG_SINCE_LAST_SEND_DOWNSTREAM_AUDIO_STATS && !isInterstitialMode()) {
            _lastSendDownstreamAudioStats = now;

            QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "sendDownstreamAudioStatsPacket", Qt::QueuedConnection);
        }
    }

    {
        PerformanceTimer perfTimer("avatarManager/postUpdate");
        avatarManager->postUpdate(deltaTime, getMain3DScene());
    }

    {
        PROFILE_RANGE_EX(app, "PostUpdateLambdas", 0xffff0000, (uint64_t)0);
        PerformanceTimer perfTimer("postUpdateLambdas");
        std::unique_lock<std::mutex> guard(_postUpdateLambdasLock);
        for (auto& iter : _postUpdateLambdas) {
            iter.second();
        }
        _postUpdateLambdas.clear();
    }


    updateRenderArgs(deltaTime);

    // HACK
    // load the view frustum
    // FIXME: This preDisplayRender call is temporary until we create a separate render::scene for the mirror rendering.
    // Then we can move this logic into the Avatar::simulate call.
    myAvatar->preDisplaySide(&_appRenderArgs._renderArgs);


    {
        PerformanceTimer perfTimer("AnimDebugDraw");
        AnimDebugDraw::getInstance().update();
    }


    { // Game loop is done, mark the end of the frame for the scene transactions and the render loop to take over
        PerformanceTimer perfTimer("enqueueFrame");
        getMain3DScene()->enqueueFrame();
    }
}

void Application::updateRenderArgs(float deltaTime) {
    editRenderArgs([this, deltaTime](AppRenderArgs& appRenderArgs) {
        PerformanceTimer perfTimer("editRenderArgs");
        appRenderArgs._headPose = getHMDSensorPose();

        auto myAvatar = getMyAvatar();

        // update the avatar with a fresh HMD pose
        {
            PROFILE_RANGE(render, "/updateAvatar");
            myAvatar->updateFromHMDSensorMatrix(appRenderArgs._headPose);
        }

        auto lodManager = DependencyManager::get<LODManager>();

        float sensorToWorldScale = getMyAvatar()->getSensorToWorldScale();
        appRenderArgs._sensorToWorldScale = sensorToWorldScale;
        appRenderArgs._sensorToWorld = getMyAvatar()->getSensorToWorldMatrix();
        {
            PROFILE_RANGE(render, "/buildFrustrumAndArgs");
            {
                QMutexLocker viewLocker(&_viewMutex);
                // adjust near clip plane to account for sensor scaling.
                auto adjustedProjection = glm::perspective(glm::radians(_fieldOfView.get()),
                    getActiveDisplayPlugin()->getRecommendedAspectRatio(),
                    DEFAULT_NEAR_CLIP * sensorToWorldScale,
                    DEFAULT_FAR_CLIP);
                _viewFrustum.setProjection(adjustedProjection);
                _viewFrustum.calculate();
            }
            appRenderArgs._renderArgs = RenderArgs(_gpuContext, lodManager->getOctreeSizeScale(),
                lodManager->getBoundaryLevelAdjust(), lodManager->getLODAngleHalfTan(), RenderArgs::DEFAULT_RENDER_MODE,
                RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);
            appRenderArgs._renderArgs._scene = getMain3DScene();

            {
                QMutexLocker viewLocker(&_viewMutex);
                appRenderArgs._renderArgs.setViewFrustum(_viewFrustum);
            }
        }
        {
            PROFILE_RANGE(render, "/resizeGL");
            bool showWarnings = false;
            bool suppressShortTimings = false;
            auto menu = Menu::getInstance();
            if (menu) {
                suppressShortTimings = menu->isOptionChecked(MenuOption::SuppressShortTimings);
                showWarnings = menu->isOptionChecked(MenuOption::PipelineWarnings);
            }
            PerformanceWarning::setSuppressShortTimings(suppressShortTimings);
            PerformanceWarning warn(showWarnings, "Application::paintGL()");
            resizeGL();
        }

        this->updateCamera(appRenderArgs._renderArgs, deltaTime);
        appRenderArgs._eyeToWorld = _myCamera.getTransform();
        appRenderArgs._isStereo = false;

        {
            auto hmdInterface = DependencyManager::get<HMDScriptingInterface>();
            float ipdScale = hmdInterface->getIPDScale();

            // scale IPD by sensorToWorldScale, to make the world seem larger or smaller accordingly.
            ipdScale *= sensorToWorldScale;

            auto baseProjection = appRenderArgs._renderArgs.getViewFrustum().getProjection();

            if (getActiveDisplayPlugin()->isStereo()) {
                // Stereo modes will typically have a larger projection matrix overall,
                // so we ask for the 'mono' projection matrix, which for stereo and HMD
                // plugins will imply the combined projection for both eyes.
                //
                // This is properly implemented for the Oculus plugins, but for OpenVR
                // and Stereo displays I'm not sure how to get / calculate it, so we're
                // just relying on the left FOV in each case and hoping that the
                // overall culling margin of error doesn't cause popping in the
                // right eye.  There are FIXMEs in the relevant plugins
                _myCamera.setProjection(getActiveDisplayPlugin()->getCullingProjection(baseProjection));
                appRenderArgs._isStereo = true;

                auto& eyeOffsets = appRenderArgs._eyeOffsets;
                auto& eyeProjections = appRenderArgs._eyeProjections;

                // FIXME we probably don't need to set the projection matrix every frame,
                // only when the display plugin changes (or in non-HMD modes when the user
                // changes the FOV manually, which right now I don't think they can.
                for_each_eye([&](Eye eye) {
                    // For providing the stereo eye views, the HMD head pose has already been
                    // applied to the avatar, so we need to get the difference between the head
                    // pose applied to the avatar and the per eye pose, and use THAT as
                    // the per-eye stereo matrix adjustment.
                    mat4 eyeToHead = getActiveDisplayPlugin()->getEyeToHeadTransform(eye);
                    // Grab the translation
                    vec3 eyeOffset = glm::vec3(eyeToHead[3]);
                    // Apply IPD scaling
                    mat4 eyeOffsetTransform = glm::translate(mat4(), eyeOffset * -1.0f * ipdScale);
                    eyeOffsets[eye] = eyeOffsetTransform;
                    eyeProjections[eye] = getActiveDisplayPlugin()->getEyeProjection(eye, baseProjection);
                });

                // Configure the type of display / stereo
                appRenderArgs._renderArgs._displayMode = (isHMDMode() ? RenderArgs::STEREO_HMD : RenderArgs::STEREO_MONITOR);
            }
        }

        {
            QMutexLocker viewLocker(&_viewMutex);
            _myCamera.loadViewFrustum(_displayViewFrustum);
            appRenderArgs._view = glm::inverse(_displayViewFrustum.getView());
        }

        {
            QMutexLocker viewLocker(&_viewMutex);
            appRenderArgs._renderArgs.setViewFrustum(_displayViewFrustum);
        }
    });
}

void Application::queryAvatars() {
    if (!isInterstitialMode()) {
        auto avatarPacket = NLPacket::create(PacketType::AvatarQuery);
        auto destinationBuffer = reinterpret_cast<unsigned char*>(avatarPacket->getPayload());
        unsigned char* bufferStart = destinationBuffer;

        uint8_t numFrustums = (uint8_t)_conicalViews.size();
        memcpy(destinationBuffer, &numFrustums, sizeof(numFrustums));
        destinationBuffer += sizeof(numFrustums);

        for (const auto& view : _conicalViews) {
            destinationBuffer += view.serialize(destinationBuffer);
        }

        avatarPacket->setPayloadSize(destinationBuffer - bufferStart);

        DependencyManager::get<NodeList>()->broadcastToNodes(std::move(avatarPacket), NodeSet() << NodeType::AvatarMixer);
    }
}


int Application::sendNackPackets() {

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
                // retrieve octree scene stats of this node
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

void Application::queryOctree(NodeType_t serverType, PacketType packetType) {

    if (!_settingsLoaded) {
        return; // bail early if settings are not loaded
    }

    const bool isModifiedQuery = !_physicsEnabled;
    if (isModifiedQuery) {
        // Create modified view that is a simple sphere.
        ConicalViewFrustum sphericalView;
        sphericalView.setSimpleRadius(INITIAL_QUERY_RADIUS);
        _octreeQuery.setConicalViews({ sphericalView });
        _octreeQuery.setOctreeSizeScale(DEFAULT_OCTREE_SIZE_SCALE);
        static constexpr float MIN_LOD_ADJUST = -20.0f;
        _octreeQuery.setBoundaryLevelAdjust(MIN_LOD_ADJUST);
    } else {
        _octreeQuery.setConicalViews(_conicalViews);
        auto lodManager = DependencyManager::get<LODManager>();
        _octreeQuery.setOctreeSizeScale(lodManager->getOctreeSizeScale());
        _octreeQuery.setBoundaryLevelAdjust(lodManager->getBoundaryLevelAdjust());
    }
    _octreeQuery.setReportInitialCompletion(isModifiedQuery);


    auto nodeList = DependencyManager::get<NodeList>();

    auto node = nodeList->soloNodeOfType(serverType);
    if (node && node->getActiveSocket()) {
        _octreeQuery.setMaxQueryPacketsPerSecond(getMaxOctreePacketsPerSecond());

        auto queryPacket = NLPacket::create(packetType);

        // encode the query data
        auto packetData = reinterpret_cast<unsigned char*>(queryPacket->getPayload());
        int packetSize = _octreeQuery.getBroadcastData(packetData);
        queryPacket->setPayloadSize(packetSize);

        // make sure we still have an active socket
        nodeList->sendUnreliablePacket(*queryPacket, *node);
    }
}


bool Application::isHMDMode() const {
    return getActiveDisplayPlugin()->isHmd();
}

float Application::getNumCollisionObjects() const {
    return _physicsEngine ? _physicsEngine->getNumCollisionObjects() : 0;
}

float Application::getTargetRenderFrameRate() const { return getActiveDisplayPlugin()->getTargetFrameRate(); }

QRect Application::getDesirableApplicationGeometry() const {
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
        // we will use the screen for the HMDTools since it's a guaranteed
        // better screen.
        if (appScreen == hmdScreen) {
            QScreen* betterScreen = hmdTools->windowHandle()->screen();
            applicationGeometry = betterScreen->geometry();
        }
    }
    return applicationGeometry;
}

PickRay Application::computePickRay(float x, float y) const {
    vec2 pickPoint { x, y };
    PickRay result;
    if (isHMDMode()) {
        getApplicationCompositor().computeHmdPickRay(pickPoint, result.origin, result.direction);
    } else {
        pickPoint /= getCanvasSize();
        if (_myCamera.getMode() == CameraMode::CAMERA_MODE_MIRROR) {
            pickPoint.x = 1.0f - pickPoint.x;
        }
        QMutexLocker viewLocker(&_viewMutex);
        _viewFrustum.computePickRay(pickPoint.x, pickPoint.y, result.origin, result.direction);
    }
    return result;
}

std::shared_ptr<MyAvatar> Application::getMyAvatar() const {
    return DependencyManager::get<AvatarManager>()->getMyAvatar();
}

glm::vec3 Application::getAvatarPosition() const {
    return getMyAvatar()->getWorldPosition();
}

void Application::copyViewFrustum(ViewFrustum& viewOut) const {
    QMutexLocker viewLocker(&_viewMutex);
    viewOut = _viewFrustum;
}

void Application::copyDisplayViewFrustum(ViewFrustum& viewOut) const {
    QMutexLocker viewLocker(&_viewMutex);
    viewOut = _displayViewFrustum;
}

void Application::resetSensors(bool andReload) {
    DependencyManager::get<DdeFaceTracker>()->reset();
    DependencyManager::get<EyeTracker>()->reset();
    getActiveDisplayPlugin()->resetSensors();
    _overlayConductor.centerUI();
    getMyAvatar()->reset(true, andReload);
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "reset", Qt::QueuedConnection);
}

void Application::updateWindowTitle() const {

    auto nodeList = DependencyManager::get<NodeList>();
    auto accountManager = DependencyManager::get<AccountManager>();
    auto isInErrorState = nodeList->getDomainHandler().isInErrorState();

    QString buildVersion = " - "
        + (BuildInfo::BUILD_TYPE == BuildInfo::BuildType::Stable ? QString("Version") : QString("Build"))
        + " " + applicationVersion();

    QString loginStatus = accountManager->isLoggedIn() ? "" : " (NOT LOGGED IN)";

    QString connectionStatus = isInErrorState ? " (ERROR CONNECTING)" :
        nodeList->getDomainHandler().isConnected() ? "" : " (NOT CONNECTED)";
    QString username = accountManager->getAccountInfo().getUsername();

    setCrashAnnotation("username", username.toStdString());

    QString currentPlaceName;
    if (isServerlessMode()) {
        if (isInErrorState) {
            currentPlaceName = "serverless: " + nodeList->getDomainHandler().getErrorDomainURL().toString();
        } else {
            currentPlaceName = "serverless: " + DependencyManager::get<AddressManager>()->getDomainURL().toString();
        }
    } else {
        currentPlaceName = DependencyManager::get<AddressManager>()->getDomainURL().host();
        if (currentPlaceName.isEmpty()) {
            currentPlaceName = nodeList->getDomainHandler().getHostname();
        }
    }

    QString title = QString() + (!username.isEmpty() ? username + " @ " : QString())
        + currentPlaceName + connectionStatus + loginStatus + buildVersion;

#ifndef WIN32
    // crashes with vs2013/win32
    qCDebug(interfaceapp, "Application title set to: %s", title.toStdString().c_str());
#endif
    _window->setWindowTitle(title);

    // updateTitleWindow gets called whenever there's a change regarding the domain, so rather
    // than placing this within domainURLChanged, it's placed here to cover the other potential cases.
    DependencyManager::get< MessagesClient >()->sendLocalMessage("Toolbar-DomainChanged", "");
}

void Application::clearDomainOctreeDetails() {

    // if we're about to quit, we really don't need to do any of these things...
    if (_aboutToQuit) {
        return;
    }

    qCDebug(interfaceapp) << "Clearing domain octree details...";

    resetPhysicsReadyInformation();
    setIsInterstitialMode(true);

    _octreeServerSceneStats.withWriteLock([&] {
        _octreeServerSceneStats.clear();
    });

    // reset the model renderer
    getEntities()->clear();

    auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();

    skyStage->setBackgroundMode(graphics::SunSkyStage::SKY_DEFAULT);

    DependencyManager::get<AnimationCache>()->clearUnusedResources();
    DependencyManager::get<ModelCache>()->clearUnusedResources();
    DependencyManager::get<SoundCache>()->clearUnusedResources();
    DependencyManager::get<TextureCache>()->clearUnusedResources();

    getMyAvatar()->setAvatarEntityDataChanged(true);
}

void Application::domainURLChanged(QUrl domainURL) {
    // disable physics until we have enough information about our new location to not cause craziness.
    resetPhysicsReadyInformation();
    setIsServerlessMode(domainURL.scheme() != URL_SCHEME_HIFI);
    if (isServerlessMode()) {
        loadServerlessDomain(domainURL);
    }
    updateWindowTitle();
}

void Application::goToErrorDomainURL(QUrl errorDomainURL) {
    // disable physics until we have enough information about our new location to not cause craziness.
    resetPhysicsReadyInformation();
    setIsServerlessMode(errorDomainURL.scheme() != URL_SCHEME_HIFI);
    if (isServerlessMode()) {
        loadServerlessDomain(errorDomainURL, true);
    }
    updateWindowTitle();
}


void Application::resettingDomain() {
    _notifiedPacketVersionMismatchThisDomain = false;

    clearDomainOctreeDetails();
}

void Application::nodeAdded(SharedNodePointer node) const {
    // nothing to do here
}

void Application::nodeActivated(SharedNodePointer node) {
    if (node->getType() == NodeType::AssetServer) {
        // asset server just connected - check if we have the asset browser showing

        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        auto assetDialog = offscreenUi->getRootItem()->findChild<QQuickItem*>("AssetServer");

        if (assetDialog) {
            auto nodeList = DependencyManager::get<NodeList>();

            if (nodeList->getThisNodeCanWriteAssets()) {
                // call reload on the shown asset browser dialog to get the mappings (if permissions allow)
                QMetaObject::invokeMethod(assetDialog, "reload");
            } else {
                // we switched to an Asset Server that we can't modify, hide the Asset Browser
                assetDialog->setVisible(false);
            }
        }
    }

    // If we get a new EntityServer activated, reset lastQueried time
    // so we will do a proper query during update
    if (node->getType() == NodeType::EntityServer) {
        _queryExpiry = SteadyClock::now();
        _octreeQuery.incrementConnectionID();
    }

    if (node->getType() == NodeType::AudioMixer && !isInterstitialMode()) {
        DependencyManager::get<AudioClient>()->negotiateAudioFormat();
    }

    if (node->getType() == NodeType::AvatarMixer) {
        _queryExpiry = SteadyClock::now();

        // new avatar mixer, send off our identity packet on next update loop
        // Reset skeletonModelUrl if the last server modified our choice.
        // Override the avatar url (but not model name) here too.
        if (_avatarOverrideUrl.isValid()) {
            getMyAvatar()->useFullAvatarURL(_avatarOverrideUrl);
        }

        if (getMyAvatar()->getFullAvatarURLFromPreferences() != getMyAvatar()->getSkeletonModelURL()) {
            getMyAvatar()->resetFullAvatarURL();
        }
        getMyAvatar()->markIdentityDataChanged();
        getMyAvatar()->resetLastSent();

        if (!isInterstitialMode()) {
            // transmit a "sendAll" packet to the AvatarMixer we just connected to.
            getMyAvatar()->sendAvatarDataPacket(true);
        }
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
    } else if (node->getType() == NodeType::EntityServer) {
        // we lost an entity server, clear all of the domain octree details
        clearDomainOctreeDetails();
    } else if (node->getType() == NodeType::AssetServer) {
        // asset server going away - check if we have the asset browser showing

        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        auto assetDialog = offscreenUi->getRootItem()->findChild<QQuickItem*>("AssetServer");

        if (assetDialog) {
            // call reload on the shown asset browser dialog
            QMetaObject::invokeMethod(assetDialog, "clear");
        }
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

    // We don't want to use EntityTree::findEntities(AABox, ...) method because that scan will snarf parented entities
    // whose bounding boxes cannot be computed (it is too loose for our purposes here).  Instead we manufacture
    // custom filters and use the general-purpose EntityTree::findEntities(filter, ...)
    QVector<EntityItemPointer> entities;
    AABox avatarBox(getMyAvatar()->getWorldPosition() - glm::vec3(PHYSICS_READY_RANGE), glm::vec3(2 * PHYSICS_READY_RANGE));
    // create two functions that use avatarBox (entityScan and elementScan), the second calls the first
    std::function<bool (EntityItemPointer&)> entityScan = [=](EntityItemPointer& entity) {
        if (entity->shouldBePhysical()) {
            bool success = false;
            AABox entityBox = entity->getAABox(success);
            // important: bail for entities that cannot supply a valid AABox
            return success && avatarBox.touches(entityBox);
        }
        return false;
    };
    std::function<bool(const OctreeElementPointer&, void*)> elementScan = [&](const OctreeElementPointer& element, void* unused) {
        if (element->getAACube().touches(avatarBox)) {
            EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
            entityTreeElement->getEntities(entityScan, entities);
            return true;
        }
        return false;
    };

    entityTree->withReadLock([&] {
        // Pass the second function to the general-purpose EntityTree::findEntities()
        // which will traverse the tree, apply the two filter functions (to element, then to entities)
        // as it traverses.  The end result will be a list of entities that match.
        entityTree->findEntities(elementScan, entities);
    });

    uint32_t nearbyCount = entities.size();
    if (nearbyCount == _nearbyEntitiesCountAtLastPhysicsCheck) {
        _nearbyEntitiesStabilityCount++;
    } else {
        _nearbyEntitiesStabilityCount = 0;
    }
    _nearbyEntitiesCountAtLastPhysicsCheck = nearbyCount;

    const uint32_t MINIMUM_NEARBY_ENTITIES_STABILITY_COUNT = 3;
    if (_nearbyEntitiesStabilityCount >= MINIMUM_NEARBY_ENTITIES_STABILITY_COUNT) {
        // We've seen the same number of nearby entities for several stats packets in a row.  assume we've got all
        // the local entities.
        bool result = true;
        foreach (EntityItemPointer entity, entities) {
            if (entity->shouldBePhysical() && !entity->isReadyToComputeShape()) {
                HIFI_FCDEBUG(interfaceapp(), "Physics disabled until entity loads: " << entity->getID() << entity->getName());
                // don't break here because we want all the relevant entities to start their downloads
                result = false;
            }
        }
        return result;
    }
    return false;
}

int Application::processOctreeStats(ReceivedMessage& message, SharedNodePointer sendingNode) {
    // parse the incoming stats datas stick it in a temporary object for now, while we
    // determine which server it belongs to
    int statsMessageLength = 0;

    const QUuid& nodeUUID = sendingNode->getUUID();

    // now that we know the node ID, let's add these stats to the stats for that node...
    _octreeServerSceneStats.withWriteLock([&] {
        OctreeSceneStats& octreeStats = _octreeServerSceneStats[nodeUUID];
        statsMessageLength = octreeStats.unpackFromPacket(message);

        if (octreeStats.isFullScene()) {
            _fullSceneReceivedCounter++;
        }
    });

    return statsMessageLength;
}

void Application::packetSent(quint64 length) {
}

void Application::addingEntityWithCertificate(const QString& certificateID, const QString& placeName) {
    auto ledger = DependencyManager::get<Ledger>();
    ledger->updateLocation(certificateID, placeName);
}

void Application::registerScriptEngineWithApplicationServices(ScriptEnginePointer scriptEngine) {

    scriptEngine->setEmitScriptUpdatesFunction([this]() {
        SharedNodePointer entityServerNode = DependencyManager::get<NodeList>()->soloNodeOfType(NodeType::EntityServer);
        return !entityServerNode || isPhysicsEnabled();
    });

    // setup the packet sender of the script engine's scripting interfaces so
    // we can use the same ones from the application.
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    entityScriptingInterface->setPacketSender(&_entityEditSender);
    entityScriptingInterface->setEntityTree(getEntities()->getTree());

    if (property(hifi::properties::TEST).isValid()) {
        scriptEngine->registerGlobalObject("Test", TestScriptingInterface::getInstance());
    }

    scriptEngine->registerGlobalObject("Rates", new RatesScriptingInterface(this));

    // hook our avatar and avatar hash map object into this script engine
    getMyAvatar()->registerMetaTypes(scriptEngine);

    scriptEngine->registerGlobalObject("AvatarList", DependencyManager::get<AvatarManager>().data());

    scriptEngine->registerGlobalObject("Camera", &_myCamera);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    scriptEngine->registerGlobalObject("SpeechRecognizer", DependencyManager::get<SpeechRecognizer>().data());
#endif

    ClipboardScriptingInterface* clipboardScriptable = new ClipboardScriptingInterface();
    scriptEngine->registerGlobalObject("Clipboard", clipboardScriptable);
    connect(scriptEngine.data(), &ScriptEngine::finished, clipboardScriptable, &ClipboardScriptingInterface::deleteLater);

    scriptEngine->registerGlobalObject("Overlays", &_overlays);
    qScriptRegisterMetaType(scriptEngine.data(), OverlayPropertyResultToScriptValue, OverlayPropertyResultFromScriptValue);
    qScriptRegisterMetaType(scriptEngine.data(), RayToOverlayIntersectionResultToScriptValue,
                            RayToOverlayIntersectionResultFromScriptValue);

    scriptEngine->registerGlobalObject("OffscreenFlags", DependencyManager::get<OffscreenUi>()->getFlags());
    scriptEngine->registerGlobalObject("Desktop", DependencyManager::get<DesktopScriptingInterface>().data());

    qScriptRegisterMetaType(scriptEngine.data(), wrapperToScriptValue<ToolbarProxy>, wrapperFromScriptValue<ToolbarProxy>);
    qScriptRegisterMetaType(scriptEngine.data(),
                            wrapperToScriptValue<ToolbarButtonProxy>, wrapperFromScriptValue<ToolbarButtonProxy>);
    scriptEngine->registerGlobalObject("Toolbars", DependencyManager::get<ToolbarScriptingInterface>().data());

    qScriptRegisterMetaType(scriptEngine.data(), wrapperToScriptValue<TabletProxy>, wrapperFromScriptValue<TabletProxy>);
    qScriptRegisterMetaType(scriptEngine.data(),
                            wrapperToScriptValue<TabletButtonProxy>, wrapperFromScriptValue<TabletButtonProxy>);
    scriptEngine->registerGlobalObject("Tablet", DependencyManager::get<TabletScriptingInterface>().data());
    // FIXME remove these deprecated names for the tablet scripting interface
    scriptEngine->registerGlobalObject("tabletInterface", DependencyManager::get<TabletScriptingInterface>().data());

    auto toolbarScriptingInterface = DependencyManager::get<ToolbarScriptingInterface>().data();
    DependencyManager::get<TabletScriptingInterface>().data()->setToolbarScriptingInterface(toolbarScriptingInterface);

    scriptEngine->registerGlobalObject("Window", DependencyManager::get<WindowScriptingInterface>().data());
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                        LocationScriptingInterface::locationSetter, "Window");
    // register `location` on the global object.
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                                       LocationScriptingInterface::locationSetter);

    bool clientScript = scriptEngine->isClientScript();
    scriptEngine->registerFunction("OverlayWindow", clientScript ? QmlWindowClass::constructor : QmlWindowClass::restricted_constructor);
#if !defined(Q_OS_ANDROID)
    scriptEngine->registerFunction("OverlayWebWindow", clientScript ? QmlWebWindowClass::constructor : QmlWebWindowClass::restricted_constructor);
#endif
    scriptEngine->registerFunction("QmlFragment", clientScript ? QmlFragmentClass::constructor : QmlFragmentClass::restricted_constructor);

    scriptEngine->registerGlobalObject("Menu", MenuScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("DesktopPreviewProvider", DependencyManager::get<DesktopPreviewProvider>().data());
    scriptEngine->registerGlobalObject("Stats", Stats::getInstance());
    scriptEngine->registerGlobalObject("Settings", SettingsScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("Snapshot", DependencyManager::get<Snapshot>().data());
    scriptEngine->registerGlobalObject("AudioStats", DependencyManager::get<AudioClient>()->getStats().data());
    scriptEngine->registerGlobalObject("AudioScope", DependencyManager::get<AudioScope>().data());
    scriptEngine->registerGlobalObject("AvatarBookmarks", DependencyManager::get<AvatarBookmarks>().data());
    scriptEngine->registerGlobalObject("LocationBookmarks", DependencyManager::get<LocationBookmarks>().data());

    scriptEngine->registerGlobalObject("RayPick", DependencyManager::get<RayPickScriptingInterface>().data());
    scriptEngine->registerGlobalObject("LaserPointers", DependencyManager::get<LaserPointerScriptingInterface>().data());
    scriptEngine->registerGlobalObject("Picks", DependencyManager::get<PickScriptingInterface>().data());
    scriptEngine->registerGlobalObject("Pointers", DependencyManager::get<PointerScriptingInterface>().data());

    // Caches
    scriptEngine->registerGlobalObject("AnimationCache", DependencyManager::get<AnimationCacheScriptingInterface>().data());
    scriptEngine->registerGlobalObject("TextureCache", DependencyManager::get<TextureCacheScriptingInterface>().data());
    scriptEngine->registerGlobalObject("ModelCache", DependencyManager::get<ModelCacheScriptingInterface>().data());
    scriptEngine->registerGlobalObject("SoundCache", DependencyManager::get<SoundCacheScriptingInterface>().data());

    scriptEngine->registerGlobalObject("DialogsManager", _dialogsManagerScriptingInterface);

    scriptEngine->registerGlobalObject("Account", AccountServicesScriptingInterface::getInstance()); // DEPRECATED - TO BE REMOVED
    scriptEngine->registerGlobalObject("GlobalServices", AccountServicesScriptingInterface::getInstance()); // DEPRECATED - TO BE REMOVED
    scriptEngine->registerGlobalObject("AccountServices", AccountServicesScriptingInterface::getInstance());
    qScriptRegisterMetaType(scriptEngine.data(), DownloadInfoResultToScriptValue, DownloadInfoResultFromScriptValue);

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
    scriptEngine->registerGlobalObject("Workload", _gameWorkload._engine->getConfiguration().get());

    GraphicsScriptingInterface::registerMetaTypes(scriptEngine.data());
    scriptEngine->registerGlobalObject("Graphics", DependencyManager::get<GraphicsScriptingInterface>().data());

    scriptEngine->registerGlobalObject("ScriptDiscoveryService", DependencyManager::get<ScriptEngines>().data());
    scriptEngine->registerGlobalObject("Reticle", getApplicationCompositor().getReticleInterface());

    scriptEngine->registerGlobalObject("UserActivityLogger", DependencyManager::get<UserActivityLoggerScriptingInterface>().data());
    scriptEngine->registerGlobalObject("Users", DependencyManager::get<UsersScriptingInterface>().data());

    scriptEngine->registerGlobalObject("GooglePoly", DependencyManager::get<GooglePolyScriptingInterface>().data());

    if (auto steamClient = PluginManager::getInstance()->getSteamClientPlugin()) {
        scriptEngine->registerGlobalObject("Steam", new SteamScriptingInterface(scriptEngine.data(), steamClient.get()));
    }
    auto scriptingInterface = DependencyManager::get<controller::ScriptingInterface>();
    scriptEngine->registerGlobalObject("Controller", scriptingInterface.data());
    UserInputMapper::registerControllerTypes(scriptEngine.data());

    auto recordingInterface = DependencyManager::get<RecordingScriptingInterface>();
    scriptEngine->registerGlobalObject("Recording", recordingInterface.data());

    auto entityScriptServerLog = DependencyManager::get<EntityScriptServerLogClient>();
    scriptEngine->registerGlobalObject("EntityScriptServerLog", entityScriptServerLog.data());
    scriptEngine->registerGlobalObject("AvatarInputs", AvatarInputs::getInstance());
    scriptEngine->registerGlobalObject("Selection", DependencyManager::get<SelectionScriptingInterface>().data());
    scriptEngine->registerGlobalObject("ContextOverlay", DependencyManager::get<ContextOverlayInterface>().data());
    scriptEngine->registerGlobalObject("Wallet", DependencyManager::get<WalletScriptingInterface>().data());
    scriptEngine->registerGlobalObject("AddressManager", DependencyManager::get<AddressManager>().data());
    scriptEngine->registerGlobalObject("HifiAbout", AboutUtil::getInstance());

    qScriptRegisterMetaType(scriptEngine.data(), OverlayIDtoScriptValue, OverlayIDfromScriptValue);

    registerInteractiveWindowMetaType(scriptEngine.data());

    auto pickScriptingInterface = DependencyManager::get<PickScriptingInterface>();
    pickScriptingInterface->registerMetaTypes(scriptEngine.data());

    // connect this script engines printedMessage signal to the global ScriptEngines these various messages
    auto scriptEngines = DependencyManager::get<ScriptEngines>().data();
    connect(scriptEngine.data(), &ScriptEngine::printedMessage, scriptEngines, &ScriptEngines::onPrintedMessage);
    connect(scriptEngine.data(), &ScriptEngine::errorMessage, scriptEngines, &ScriptEngines::onErrorMessage);
    connect(scriptEngine.data(), &ScriptEngine::warningMessage, scriptEngines, &ScriptEngines::onWarningMessage);
    connect(scriptEngine.data(), &ScriptEngine::infoMessage, scriptEngines, &ScriptEngines::onInfoMessage);
    connect(scriptEngine.data(), &ScriptEngine::clearDebugWindow, scriptEngines, &ScriptEngines::onClearDebugWindow);

}

bool Application::canAcceptURL(const QString& urlString) const {
    QUrl url(urlString);
    if (url.query().contains(WEB_VIEW_TAG)) {
        return false;
    } else if (urlString.startsWith(URL_SCHEME_HIFI)) {
        return true;
    }
    QString lowerPath = url.path().toLower();
    for (auto& pair : _acceptedExtensions) {
        if (lowerPath.endsWith(pair.first, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool Application::acceptURL(const QString& urlString, bool defaultUpload) {
    QUrl url(urlString);

    if (url.scheme() == URL_SCHEME_HIFI) {
        // this is a hifi URL - have the AddressManager handle it
        QMetaObject::invokeMethod(DependencyManager::get<AddressManager>().data(), "handleLookupString",
                                  Qt::AutoConnection, Q_ARG(const QString&, urlString));
        return true;
    }

    QString lowerPath = url.path().toLower();
    for (auto& pair : _acceptedExtensions) {
        if (lowerPath.endsWith(pair.first, Qt::CaseInsensitive)) {
            AcceptURLMethod method = pair.second;
            return (this->*method)(urlString);
        }
    }

    if (defaultUpload && !url.fileName().isEmpty() && url.isLocalFile()) {
        showAssetServerWidget(urlString);
    }
    return defaultUpload;
}

void Application::setSessionUUID(const QUuid& sessionUUID) const {
    Physics::setSessionUUID(sessionUUID);
}

bool Application::askToSetAvatarUrl(const QString& url) {
    QUrl realUrl(url);
    if (realUrl.isLocalFile()) {
        OffscreenUi::asyncWarning("", "You can not use local files for avatar components.");
        return false;
    }

    // Download the FST file, to attempt to determine its model type
    QVariantHash fstMapping = FSTReader::downloadMapping(url);

    FSTReader::ModelType modelType = FSTReader::predictModelType(fstMapping);

    QString modelName = fstMapping["name"].toString();
    QString modelLicense = fstMapping["license"].toString();

    bool agreeToLicense = true; // assume true
    //create set avatar callback
    auto setAvatar = [=] (QString url, QString modelName) {
        ModalDialogListener* dlg = OffscreenUi::asyncQuestion("Set Avatar",
                                                              "Would you like to use '" + modelName + "' for your avatar?",
                                                              QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
        QObject::connect(dlg, &ModalDialogListener::response, this, [=] (QVariant answer) {
            QObject::disconnect(dlg, &ModalDialogListener::response, this, nullptr);

            bool ok = (QMessageBox::Ok == static_cast<QMessageBox::StandardButton>(answer.toInt()));
            if (ok) {
                getMyAvatar()->useFullAvatarURL(url, modelName);
                emit fullAvatarURLChanged(url, modelName);
            } else {
                qCDebug(interfaceapp) << "Declined to use the avatar: " << url;
            }
        });
    };

    if (!modelLicense.isEmpty()) {
        // word wrap the license text to fit in a reasonable shaped message box.
        const int MAX_CHARACTERS_PER_LINE = 90;
        modelLicense = simpleWordWrap(modelLicense, MAX_CHARACTERS_PER_LINE);

        ModalDialogListener* dlg = OffscreenUi::asyncQuestion("Avatar Usage License",
                                                              modelLicense + "\nDo you agree to these terms?",
                                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        QObject::connect(dlg, &ModalDialogListener::response, this, [=, &agreeToLicense] (QVariant answer) {
            QObject::disconnect(dlg, &ModalDialogListener::response, this, nullptr);

            agreeToLicense = (static_cast<QMessageBox::StandardButton>(answer.toInt()) == QMessageBox::Yes);
            if (agreeToLicense) {
                switch (modelType) {
                    case FSTReader::HEAD_AND_BODY_MODEL: {
                    setAvatar(url, modelName);
                    break;
                }
                default:
                    OffscreenUi::asyncWarning("", modelName + "Does not support a head and body as required.");
                    break;
                }
            } else {
                qCDebug(interfaceapp) << "Declined to agree to avatar license: " << url;
            }

            //auto offscreenUi = DependencyManager::get<OffscreenUi>();
        });
    } else {
        setAvatar(url, modelName);
    }

    return true;
}


bool Application::askToLoadScript(const QString& scriptFilenameOrURL) {
    QString shortName = scriptFilenameOrURL;

    QUrl scriptURL { scriptFilenameOrURL };

    if (scriptURL.host().endsWith(MARKETPLACE_CDN_HOSTNAME)) {
        int startIndex = shortName.lastIndexOf('/') + 1;
        int endIndex = shortName.lastIndexOf('?');
        shortName = shortName.mid(startIndex, endIndex - startIndex);
    }

#ifdef DISABLE_QML
    DependencyManager::get<ScriptEngines>()->loadScript(scriptFilenameOrURL);
#else
    QString message = "Would you like to run this script:\n" + shortName;
    ModalDialogListener* dlg = OffscreenUi::asyncQuestion(getWindow(), "Run Script", message,
                                                           QMessageBox::Yes | QMessageBox::No);

    QObject::connect(dlg, &ModalDialogListener::response, this, [=] (QVariant answer) {
        const QString& fileName = scriptFilenameOrURL;
        if (static_cast<QMessageBox::StandardButton>(answer.toInt()) == QMessageBox::Yes) {
            qCDebug(interfaceapp) << "Chose to run the script: " << fileName;
            DependencyManager::get<ScriptEngines>()->loadScript(fileName);
        } else {
            qCDebug(interfaceapp) << "Declined to run the script: " << scriptFilenameOrURL;
        }
        QObject::disconnect(dlg, &ModalDialogListener::response, this, nullptr);
    });
#endif
    return true;
}

bool Application::askToWearAvatarAttachmentUrl(const QString& url) {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest networkRequest = QNetworkRequest(url);
    networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
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

                auto avatarAttachmentConfirmationTitle = tr("Avatar Attachment Confirmation");
                auto avatarAttachmentConfirmationMessage = tr("Would you like to wear '%1' on your avatar?").arg(name);
                ModalDialogListener* dlg = OffscreenUi::asyncQuestion(avatarAttachmentConfirmationTitle,
                                           avatarAttachmentConfirmationMessage,
                                           QMessageBox::Ok | QMessageBox::Cancel);
                QObject::connect(dlg, &ModalDialogListener::response, this, [=] (QVariant answer) {
                    QObject::disconnect(dlg, &ModalDialogListener::response, this, nullptr);
                    if (static_cast<QMessageBox::StandardButton>(answer.toInt()) == QMessageBox::Yes) {
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
                });
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

void Application::replaceDomainContent(const QString& url) {
    qCDebug(interfaceapp) << "Attempting to replace domain content: " << url;
    QByteArray urlData(url.toUtf8());
    auto limitedNodeList = DependencyManager::get<NodeList>();
    const auto& domainHandler = limitedNodeList->getDomainHandler();

    auto octreeFilePacket = NLPacket::create(PacketType::DomainContentReplacementFromUrl, urlData.size(), true);
    octreeFilePacket->write(urlData);
    limitedNodeList->sendPacket(std::move(octreeFilePacket), domainHandler.getSockAddr());

    auto addressManager = DependencyManager::get<AddressManager>();
    addressManager->handleLookupString(DOMAIN_SPAWNING_POINT);
    QString newHomeAddress = addressManager->getHost() + DOMAIN_SPAWNING_POINT;
    qCDebug(interfaceapp) << "Setting new home bookmark to: " << newHomeAddress;
    DependencyManager::get<LocationBookmarks>()->setHomeLocationToAddress(newHomeAddress);
}

bool Application::askToReplaceDomainContent(const QString& url) {
    QString methodDetails;
    const int MAX_CHARACTERS_PER_LINE = 90;
    if (DependencyManager::get<NodeList>()->getThisNodeCanReplaceContent()) {
        QUrl originURL { url };
        if (originURL.host().endsWith(MARKETPLACE_CDN_HOSTNAME)) {
            // Create a confirmation dialog when this call is made
            static const QString infoText = simpleWordWrap("Your domain's content will be replaced with a new content set. "
                "If you want to save what you have now, create a backup before proceeding. For more information about backing up "
                "and restoring content, visit the documentation page at: ", MAX_CHARACTERS_PER_LINE) +
                "\nhttps://docs.highfidelity.com/create-and-explore/start-working-in-your-sandbox/restoring-sandbox-content";

            ModalDialogListener* dig = OffscreenUi::asyncQuestion("Are you sure you want to replace this domain's content set?",
                                                                  infoText, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

            QObject::connect(dig, &ModalDialogListener::response, this, [=] (QVariant answer) {
                QString details;
                if (static_cast<QMessageBox::StandardButton>(answer.toInt()) == QMessageBox::Yes) {
                    // Given confirmation, send request to domain server to replace content
                    replaceDomainContent(url);
                    details = "SuccessfulRequestToReplaceContent";
                } else {
                    details = "UserDeclinedToReplaceContent";
                }
                QJsonObject messageProperties = {
                    { "status", details },
                    { "content_set_url", url }
                };
                UserActivityLogger::getInstance().logAction("replace_domain_content", messageProperties);
                QObject::disconnect(dig, &ModalDialogListener::response, this, nullptr);
            });
        } else {
            methodDetails = "ContentSetDidNotOriginateFromMarketplace";
            QJsonObject messageProperties = {
                { "status", methodDetails },
                { "content_set_url", url }
            };
            UserActivityLogger::getInstance().logAction("replace_domain_content", messageProperties);
        }
    } else {
            methodDetails = "UserDoesNotHavePermissionToReplaceContent";
            static const QString warningMessage = simpleWordWrap("The domain owner must enable 'Replace Content' "
                "permissions for you in this domain's server settings before you can continue.", MAX_CHARACTERS_PER_LINE);
            OffscreenUi::asyncWarning("You do not have permissions to replace domain content", warningMessage,
                                 QMessageBox::Ok, QMessageBox::Ok);

            QJsonObject messageProperties = {
                { "status", methodDetails },
                { "content_set_url", url }
            };
            UserActivityLogger::getInstance().logAction("replace_domain_content", messageProperties);
    }
    return true;
}

void Application::displayAvatarAttachmentWarning(const QString& message) const {
    auto avatarAttachmentWarningTitle = tr("Avatar Attachment Failure");
    OffscreenUi::asyncWarning(avatarAttachmentWarningTitle, message);
}

void Application::showDialog(const QUrl& widgetUrl, const QUrl& tabletUrl, const QString& name) const {
    auto tablet = DependencyManager::get<TabletScriptingInterface>()->getTablet(SYSTEM_TABLET);
    auto hmd = DependencyManager::get<HMDScriptingInterface>();
    bool onTablet = false;

    if (!tablet->getToolbarMode()) {
        onTablet = tablet->pushOntoStack(tabletUrl);
        if (onTablet) {
            toggleTabletUI(true);
        }
    } else {
        DependencyManager::get<OffscreenUi>()->show(widgetUrl, name);
    }
}

void Application::showScriptLogs() {
    QUrl defaultScriptsLoc = PathUtils::defaultScriptsLocation();
    defaultScriptsLoc.setPath(defaultScriptsLoc.path() + "developer/debugging/debugWindow.js");
    DependencyManager::get<ScriptEngines>()->loadScript(defaultScriptsLoc.toString());
}

void Application::showAssetServerWidget(QString filePath) {
    if (!DependencyManager::get<NodeList>()->getThisNodeCanWriteAssets()) {
        return;
    }
    static const QUrl url { "hifi/AssetServer.qml" };

    auto startUpload = [=](QQmlContext* context, QObject* newObject){
        if (!filePath.isEmpty()) {
            emit uploadRequest(filePath);
        }
    };
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet(SYSTEM_TABLET));
    auto hmd = DependencyManager::get<HMDScriptingInterface>();
    if (tablet->getToolbarMode()) {
        DependencyManager::get<OffscreenUi>()->show(url, "AssetServer", startUpload);
    } else {
        if (!hmd->getShouldShowTablet() && !isHMDMode()) {
            DependencyManager::get<OffscreenUi>()->show(url, "AssetServer", startUpload);
        } else {
            static const QUrl url("hifi/dialogs/TabletAssetServer.qml");
            if (!tablet->isPathLoaded(url)) {
                tablet->pushOntoStack(url);
            }
        }
    }

    startUpload(nullptr, nullptr);
}

void Application::addAssetToWorldFromURL(QString url) {
    qInfo(interfaceapp) << "Download model and add to world from" << url;

    QString filename;
    if (url.contains("filename")) {
        filename = url.section("filename=", 1, 1);  // Filename is in "?filename=" parameter at end of URL.
    }
    if (url.contains("poly.google.com/downloads")) {
        filename = url.section('/', -1);
        if (url.contains("noDownload")) {
            filename.remove(".zip?noDownload=false");
        } else {
            filename.remove(".zip");
        }

    }

    if (!DependencyManager::get<NodeList>()->getThisNodeCanWriteAssets()) {
        QString errorInfo = "You do not have permissions to write to the Asset Server.";
        qWarning(interfaceapp) << "Error downloading model: " + errorInfo;
        addAssetToWorldError(filename, errorInfo);
        return;
    }

    addAssetToWorldInfo(filename, "Downloading model file " + filename + ".");

    auto request = DependencyManager::get<ResourceManager>()->createResourceRequest(nullptr, QUrl(url));
    connect(request, &ResourceRequest::finished, this, &Application::addAssetToWorldFromURLRequestFinished);
    request->send();
}

void Application::addAssetToWorldFromURLRequestFinished() {
    auto request = qobject_cast<ResourceRequest*>(sender());
    auto url = request->getUrl().toString();
    auto result = request->getResult();

    QString filename;
    bool isBlocks = false;

    if (url.contains("filename")) {
        filename = url.section("filename=", 1, 1);  // Filename is in "?filename=" parameter at end of URL.
    }
    if (url.contains("poly.google.com/downloads")) {
        filename = url.section('/', -1);
        if (url.contains("noDownload")) {
            filename.remove(".zip?noDownload=false");
        } else {
            filename.remove(".zip");
        }
        isBlocks = true;
    }

    if (result == ResourceRequest::Success) {
        qInfo(interfaceapp) << "Downloaded model from" << url;
        QTemporaryDir temporaryDir;
        temporaryDir.setAutoRemove(false);
        if (temporaryDir.isValid()) {
            QString temporaryDirPath = temporaryDir.path();
            QString downloadPath = temporaryDirPath + "/" + filename;
            qInfo(interfaceapp) << "Download path:" << downloadPath;

            QFile tempFile(downloadPath);
            if (tempFile.open(QIODevice::WriteOnly)) {
                tempFile.write(request->getData());
                addAssetToWorldInfoClear(filename);  // Remove message from list; next one added will have a different key.
                tempFile.close();
                qApp->getFileDownloadInterface()->runUnzip(downloadPath, url, true, false, isBlocks);
            } else {
                QString errorInfo = "Couldn't open temporary file for download";
                qWarning(interfaceapp) << errorInfo;
                addAssetToWorldError(filename, errorInfo);
            }
        } else {
            QString errorInfo = "Couldn't create temporary directory for download";
            qWarning(interfaceapp) << errorInfo;
            addAssetToWorldError(filename, errorInfo);
        }
    } else {
        qWarning(interfaceapp) << "Error downloading" << url << ":" << request->getResultString();
        addAssetToWorldError(filename, "Error downloading " + filename + " : " + request->getResultString());
    }

    request->deleteLater();
}


QString filenameFromPath(QString filePath) {
    return filePath.right(filePath.length() - filePath.lastIndexOf("/") - 1);
}

void Application::addAssetToWorldUnzipFailure(QString filePath) {
    QString filename = filenameFromPath(QUrl(filePath).toLocalFile());
    qWarning(interfaceapp) << "Couldn't unzip file" << filePath;
    addAssetToWorldError(filename, "Couldn't unzip file " + filename + ".");
}

void Application::addAssetToWorld(QString path, QString zipFile, bool isZip, bool isBlocks) {
    // Automatically upload and add asset to world as an alternative manual process initiated by showAssetServerWidget().
    QString mapping;
    QString filename = filenameFromPath(path);
    if (isZip || isBlocks) {
        QString assetName = zipFile.section("/", -1).remove(QRegExp("[.]zip(.*)$"));
        QString assetFolder = path.section("model_repo/", -1);
        mapping = "/" + assetName + "/" + assetFolder;
    } else {
        mapping = "/" + filename;
    }

    // Test repeated because possibly different code paths.
    if (!DependencyManager::get<NodeList>()->getThisNodeCanWriteAssets()) {
        QString errorInfo = "You do not have permissions to write to the Asset Server.";
        qWarning(interfaceapp) << "Error downloading model: " + errorInfo;
        addAssetToWorldError(filename, errorInfo);
        return;
    }

    addAssetToWorldInfo(filename, "Adding " + mapping.mid(1) + " to the Asset Server.");

    addAssetToWorldWithNewMapping(path, mapping, 0, isZip, isBlocks);
}

void Application::addAssetToWorldWithNewMapping(QString filePath, QString mapping, int copy, bool isZip, bool isBlocks) {
    auto request = DependencyManager::get<AssetClient>()->createGetMappingRequest(mapping);

    QObject::connect(request, &GetMappingRequest::finished, this, [=](GetMappingRequest* request) mutable {
        const int MAX_COPY_COUNT = 100;  // Limit number of duplicate assets; recursion guard.
        auto result = request->getError();
        if (result == GetMappingRequest::NotFound) {
            addAssetToWorldUpload(filePath, mapping, isZip, isBlocks);
        } else if (result != GetMappingRequest::NoError) {
            QString errorInfo = "Could not map asset name: "
                + mapping.left(mapping.length() - QString::number(copy).length() - 1);
            qWarning(interfaceapp) << "Error downloading model: " + errorInfo;
            addAssetToWorldError(filenameFromPath(filePath), errorInfo);
        } else if (copy < MAX_COPY_COUNT - 1) {
            if (copy > 0) {
                mapping = mapping.remove(mapping.lastIndexOf("-"), QString::number(copy).length() + 1);
            }
            copy++;
            mapping = mapping.insert(mapping.lastIndexOf("."), "-" + QString::number(copy));
            addAssetToWorldWithNewMapping(filePath, mapping, copy, isZip, isBlocks);
        } else {
            QString errorInfo = "Too many copies of asset name: "
                + mapping.left(mapping.length() - QString::number(copy).length() - 1);
            qWarning(interfaceapp) << "Error downloading model: " + errorInfo;
            addAssetToWorldError(filenameFromPath(filePath), errorInfo);
        }
        request->deleteLater();
    });

    request->start();
}

void Application::addAssetToWorldUpload(QString filePath, QString mapping, bool isZip, bool isBlocks) {
    qInfo(interfaceapp) << "Uploading" << filePath << "to Asset Server as" << mapping;
    auto upload = DependencyManager::get<AssetClient>()->createUpload(filePath);
    QObject::connect(upload, &AssetUpload::finished, this, [=](AssetUpload* upload, const QString& hash) mutable {
        if (upload->getError() != AssetUpload::NoError) {
            QString errorInfo = "Could not upload model to the Asset Server.";
            qWarning(interfaceapp) << "Error downloading model: " + errorInfo;
            addAssetToWorldError(filenameFromPath(filePath), errorInfo);
        } else {
            addAssetToWorldSetMapping(filePath, mapping, hash, isZip, isBlocks);
        }

        // Remove temporary directory created by Clara.io market place download.
        int index = filePath.lastIndexOf("/model_repo/");
        if (index > 0) {
            QString tempDir = filePath.left(index);
            qCDebug(interfaceapp) << "Removing temporary directory at: " + tempDir;
            QDir(tempDir).removeRecursively();
        }

        upload->deleteLater();
    });

    upload->start();
}

void Application::addAssetToWorldSetMapping(QString filePath, QString mapping, QString hash, bool isZip, bool isBlocks) {
    auto request = DependencyManager::get<AssetClient>()->createSetMappingRequest(mapping, hash);
    connect(request, &SetMappingRequest::finished, this, [=](SetMappingRequest* request) mutable {
        if (request->getError() != SetMappingRequest::NoError) {
            QString errorInfo = "Could not set asset mapping.";
            qWarning(interfaceapp) << "Error downloading model: " + errorInfo;
            addAssetToWorldError(filenameFromPath(filePath), errorInfo);
        } else {
            // to prevent files that aren't models or texture files from being loaded into world automatically
            if ((filePath.toLower().endsWith(OBJ_EXTENSION) || filePath.toLower().endsWith(FBX_EXTENSION)) ||
                ((filePath.toLower().endsWith(JPG_EXTENSION) || filePath.toLower().endsWith(PNG_EXTENSION)) &&
                ((!isBlocks) && (!isZip)))) {
                addAssetToWorldAddEntity(filePath, mapping);
            } else {
                qCDebug(interfaceapp) << "Zipped contents are not supported entity files";
                addAssetToWorldInfoDone(filenameFromPath(filePath));
            }
        }
        request->deleteLater();
    });

    request->start();
}

void Application::addAssetToWorldAddEntity(QString filePath, QString mapping) {
    EntityItemProperties properties;
    properties.setType(EntityTypes::Model);
    properties.setName(mapping.right(mapping.length() - 1));
    if (filePath.toLower().endsWith(PNG_EXTENSION) || filePath.toLower().endsWith(JPG_EXTENSION)) {
        QJsonObject textures {
            {"tex.picture", QString("atp:" + mapping) }
        };
        properties.setModelURL("https://hifi-content.s3.amazonaws.com/DomainContent/production/default-image-model.fbx");
        properties.setTextures(QJsonDocument(textures).toJson(QJsonDocument::Compact));
        properties.setShapeType(SHAPE_TYPE_BOX);
    } else {
        properties.setModelURL("atp:" + mapping);
        properties.setShapeType(SHAPE_TYPE_SIMPLE_COMPOUND);
    }
    properties.setCollisionless(true);  // Temporarily set so that doesn't collide with avatar.
    properties.setVisible(false);  // Temporarily set so that don't see at large unresized dimensions.
    bool grabbable = (Menu::getInstance()->isOptionChecked(MenuOption::CreateEntitiesGrabbable));
    properties.setUserData(grabbable ? GRABBABLE_USER_DATA : NOT_GRABBABLE_USER_DATA);
    glm::vec3 positionOffset = getMyAvatar()->getWorldOrientation() * (getMyAvatar()->getSensorToWorldScale() * glm::vec3(0.0f, 0.0f, -2.0f));
    properties.setPosition(getMyAvatar()->getWorldPosition() + positionOffset);
    properties.setRotation(getMyAvatar()->getWorldOrientation());
    properties.setGravity(glm::vec3(0.0f, 0.0f, 0.0f));
    auto entityID = DependencyManager::get<EntityScriptingInterface>()->addEntity(properties);

    // Note: Model dimensions are not available here; model is scaled per FBX mesh in RenderableModelEntityItem::update() later
    // on. But FBX dimensions may be in cm, so we monitor for the dimension change and rescale again if warranted.

    if (entityID == QUuid()) {
        QString errorInfo = "Could not add model " + mapping + " to world.";
        qWarning(interfaceapp) << "Could not add model to world: " + errorInfo;
        addAssetToWorldError(filenameFromPath(filePath), errorInfo);
    } else {
        // Monitor when asset is rendered in world so that can resize if necessary.
        _addAssetToWorldResizeList.insert(entityID, 0);  // List value is count of checks performed.
        if (!_addAssetToWorldResizeTimer.isActive()) {
            _addAssetToWorldResizeTimer.start();
        }

        // Close progress message box.
        addAssetToWorldInfoDone(filenameFromPath(filePath));
    }
}

void Application::addAssetToWorldCheckModelSize() {
    if (_addAssetToWorldResizeList.size() == 0) {
        return;
    }

    auto item = _addAssetToWorldResizeList.begin();
    while (item != _addAssetToWorldResizeList.end()) {
        auto entityID = item.key();

        EntityPropertyFlags propertyFlags;
        propertyFlags += PROP_NAME;
        propertyFlags += PROP_DIMENSIONS;
        auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
        auto properties = entityScriptingInterface->getEntityProperties(entityID, propertyFlags);
        auto name = properties.getName();
        auto dimensions = properties.getDimensions();

        bool doResize = false;

        const glm::vec3 DEFAULT_DIMENSIONS = glm::vec3(0.1f, 0.1f, 0.1f);
        if (dimensions != DEFAULT_DIMENSIONS) {

            // Scale model so that its maximum is exactly specific size.
            const float MAXIMUM_DIMENSION = getMyAvatar()->getSensorToWorldScale();
            auto previousDimensions = dimensions;
            auto scale = std::min(MAXIMUM_DIMENSION / dimensions.x, std::min(MAXIMUM_DIMENSION / dimensions.y,
                MAXIMUM_DIMENSION / dimensions.z));
            dimensions *= scale;
            qInfo(interfaceapp) << "Model" << name << "auto-resized from" << previousDimensions << " to " << dimensions;
            doResize = true;

            item = _addAssetToWorldResizeList.erase(item);  // Finished with this entity; advance to next.
        } else {
            // Increment count of checks done.
            _addAssetToWorldResizeList[entityID]++;

            const int CHECK_MODEL_SIZE_MAX_CHECKS = 300;
            if (_addAssetToWorldResizeList[entityID] > CHECK_MODEL_SIZE_MAX_CHECKS) {
                // Have done enough checks; model was either the default size or something's gone wrong.

                // Rescale all dimensions.
                const glm::vec3 UNIT_DIMENSIONS = glm::vec3(1.0f, 1.0f, 1.0f);
                dimensions = UNIT_DIMENSIONS;
                qInfo(interfaceapp) << "Model" << name << "auto-resize timed out; resized to " << dimensions;
                doResize = true;

                item = _addAssetToWorldResizeList.erase(item);  // Finished with this entity; advance to next.
            } else {
                // No action on this entity; advance to next.
                ++item;
            }
        }

        if (doResize) {
            EntityItemProperties properties;
            properties.setDimensions(dimensions);
            properties.setVisible(true);
            if (!name.toLower().endsWith(PNG_EXTENSION) && !name.toLower().endsWith(JPG_EXTENSION)) {
                properties.setCollisionless(false);
            }
            bool grabbable = (Menu::getInstance()->isOptionChecked(MenuOption::CreateEntitiesGrabbable));
            properties.setUserData(grabbable ? GRABBABLE_USER_DATA : NOT_GRABBABLE_USER_DATA);
            properties.setLastEdited(usecTimestampNow());
            entityScriptingInterface->editEntity(entityID, properties);
        }
    }

    // Stop timer if nothing in list to check.
    if (_addAssetToWorldResizeList.size() == 0) {
        _addAssetToWorldResizeTimer.stop();
    }
}


void Application::addAssetToWorldInfo(QString modelName, QString infoText) {
    // Displays the most recent info message, subject to being overridden by error messages.

    if (_aboutToQuit) {
        return;
    }

    /*
    Cancel info timer if running.
    If list has an entry for modelName, delete it (just one).
    Append modelName, infoText to list.
    Display infoText in message box unless an error is being displayed (i.e., error timer is running).
    Show message box if not already visible.
    */

    _addAssetToWorldInfoTimer.stop();

    addAssetToWorldInfoClear(modelName);

    _addAssetToWorldInfoKeys.append(modelName);
    _addAssetToWorldInfoMessages.append(infoText);

    if (!_addAssetToWorldErrorTimer.isActive()) {
        if (!_addAssetToWorldMessageBox) {
            _addAssetToWorldMessageBox = DependencyManager::get<OffscreenUi>()->createMessageBox(OffscreenUi::ICON_INFORMATION,
                "Downloading Model", "", QMessageBox::NoButton, QMessageBox::NoButton);
            connect(_addAssetToWorldMessageBox, SIGNAL(destroyed()), this, SLOT(onAssetToWorldMessageBoxClosed()));
        }

        _addAssetToWorldMessageBox->setProperty("text", "\n" + infoText);
        _addAssetToWorldMessageBox->setVisible(true);
    }
}

void Application::addAssetToWorldInfoClear(QString modelName) {
    // Clears modelName entry from message list without affecting message currently displayed.

    if (_aboutToQuit) {
        return;
    }

    /*
    Delete entry for modelName from list.
    */

    auto index = _addAssetToWorldInfoKeys.indexOf(modelName);
    if (index > -1) {
        _addAssetToWorldInfoKeys.removeAt(index);
        _addAssetToWorldInfoMessages.removeAt(index);
    }
}

void Application::addAssetToWorldInfoDone(QString modelName) {
    // Continues to display this message if the latest for a few seconds, then deletes it and displays the next latest.

    if (_aboutToQuit) {
        return;
    }

    /*
    Delete entry for modelName from list.
    (Re)start the info timer to update message box. ... onAddAssetToWorldInfoTimeout()
    */

    addAssetToWorldInfoClear(modelName);
    _addAssetToWorldInfoTimer.start();
}

void Application::addAssetToWorldInfoTimeout() {
    if (_aboutToQuit) {
        return;
    }

    /*
    If list not empty, display last message in list (may already be displayed ) unless an error is being displayed.
    If list empty, close the message box unless an error is being displayed.
    */

    if (!_addAssetToWorldErrorTimer.isActive() && _addAssetToWorldMessageBox) {
        if (_addAssetToWorldInfoKeys.length() > 0) {
            _addAssetToWorldMessageBox->setProperty("text", "\n" + _addAssetToWorldInfoMessages.last());
        } else {
            disconnect(_addAssetToWorldMessageBox);
            _addAssetToWorldMessageBox->setVisible(false);
            _addAssetToWorldMessageBox->deleteLater();
            _addAssetToWorldMessageBox = nullptr;
        }
    }
}

void Application::addAssetToWorldError(QString modelName, QString errorText) {
    // Displays the most recent error message for a few seconds.

    if (_aboutToQuit) {
        return;
    }

    /*
    If list has an entry for modelName, delete it.
    Display errorText in message box.
    Show message box if not already visible.
    (Re)start error timer. ... onAddAssetToWorldErrorTimeout()
    */

    addAssetToWorldInfoClear(modelName);

    if (!_addAssetToWorldMessageBox) {
        _addAssetToWorldMessageBox = DependencyManager::get<OffscreenUi>()->createMessageBox(OffscreenUi::ICON_INFORMATION,
            "Downloading Model", "", QMessageBox::NoButton, QMessageBox::NoButton);
        connect(_addAssetToWorldMessageBox, SIGNAL(destroyed()), this, SLOT(onAssetToWorldMessageBoxClosed()));
    }

    _addAssetToWorldMessageBox->setProperty("text", "\n" + errorText);
    _addAssetToWorldMessageBox->setVisible(true);

    _addAssetToWorldErrorTimer.start();
}

void Application::addAssetToWorldErrorTimeout() {
    if (_aboutToQuit) {
        return;
    }

    /*
    If list is not empty, display message from last entry.
    If list is empty, close the message box.
    */

    if (_addAssetToWorldMessageBox) {
        if (_addAssetToWorldInfoKeys.length() > 0) {
            _addAssetToWorldMessageBox->setProperty("text", "\n" + _addAssetToWorldInfoMessages.last());
        } else {
            disconnect(_addAssetToWorldMessageBox);
            _addAssetToWorldMessageBox->setVisible(false);
            _addAssetToWorldMessageBox->deleteLater();
            _addAssetToWorldMessageBox = nullptr;
        }
    }
}


void Application::addAssetToWorldMessageClose() {
    // Clear messages, e.g., if Interface is being closed or domain changes.

    /*
    Call if user manually closes message box.
    Call if domain changes.
    Call if application is shutting down.

    Stop timers.
    Close the message box if open.
    Clear lists.
    */

    _addAssetToWorldInfoTimer.stop();
    _addAssetToWorldErrorTimer.stop();

    if (_addAssetToWorldMessageBox) {
        disconnect(_addAssetToWorldMessageBox);
        _addAssetToWorldMessageBox->setVisible(false);
        _addAssetToWorldMessageBox->deleteLater();
        _addAssetToWorldMessageBox = nullptr;
    }

    _addAssetToWorldInfoKeys.clear();
    _addAssetToWorldInfoMessages.clear();
}

void Application::onAssetToWorldMessageBoxClosed() {
    if (_addAssetToWorldMessageBox) {
        // User manually closed message box; perhaps because it has become stuck, so reset all messages.
        qInfo(interfaceapp) << "User manually closed download status message box";
        disconnect(_addAssetToWorldMessageBox);
        _addAssetToWorldMessageBox = nullptr;
        addAssetToWorldMessageClose();
    }
}


void Application::handleUnzip(QString zipFile, QStringList unzipFile, bool autoAdd, bool isZip, bool isBlocks) {
    if (autoAdd) {
        if (!unzipFile.isEmpty()) {
            for (int i = 0; i < unzipFile.length(); i++) {
                if (QFileInfo(unzipFile.at(i)).isFile()) {
                    qCDebug(interfaceapp) << "Preparing file for asset server: " << unzipFile.at(i);
                    addAssetToWorld(unzipFile.at(i), zipFile, isZip, isBlocks);
                }
            }
        } else {
            addAssetToWorldUnzipFailure(zipFile);
        }
    } else {
        showAssetServerWidget(unzipFile.first());
    }
}

void Application::packageModel() {
    ModelPackager::package();
}

void Application::openUrl(const QUrl& url) const {
    if (!url.isEmpty()) {
        if (url.scheme() == URL_SCHEME_HIFI) {
            DependencyManager::get<AddressManager>()->handleLookupString(url.toString());
        } else {
            // address manager did not handle - ask QDesktopServices to handle
            QDesktopServices::openUrl(url);
        }
    }
}

void Application::loadDialog() {
    ModalDialogListener* dlg = OffscreenUi::getOpenFileNameAsync(_glWidget, tr("Open Script"),
                                                                 getPreviousScriptLocation(),
                                                                 tr("JavaScript Files (*.js)"));
    connect(dlg, &ModalDialogListener::response, this, [=] (QVariant answer) {
        disconnect(dlg, &ModalDialogListener::response, this, nullptr);
        const QString& response = answer.toString();
        if (!response.isEmpty() && QFile(response).exists()) {
            setPreviousScriptLocation(QFileInfo(response).absolutePath());
            DependencyManager::get<ScriptEngines>()->loadScript(response, true, false, false, true);  // Don't load from cache
        }
    });
}

QString Application::getPreviousScriptLocation() {
    QString result = _previousScriptLocation.get();
    return result;
}

void Application::setPreviousScriptLocation(const QString& location) {
    _previousScriptLocation.set(location);
}

void Application::loadScriptURLDialog() const {
    ModalDialogListener* dlg = OffscreenUi::getTextAsync(OffscreenUi::ICON_NONE, "Open and Run Script", "Script URL");
    connect(dlg, &ModalDialogListener::response, this, [=] (QVariant response) {
        disconnect(dlg, &ModalDialogListener::response, this, nullptr);
        const QString& newScript = response.toString();
        if (QUrl(newScript).scheme() == "atp") {
            OffscreenUi::asyncWarning("Error Loading Script", "Cannot load client script over ATP");
        } else if (!newScript.isEmpty()) {
            DependencyManager::get<ScriptEngines>()->loadScript(newScript.trimmed());
        }
    });
}

SharedSoundPointer Application::getSampleSound() const {
    return _sampleSound;
}

void Application::loadLODToolsDialog() {
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet(SYSTEM_TABLET));
    if (tablet->getToolbarMode() || (!tablet->getTabletRoot() && !isHMDMode())) {
        auto dialogsManager = DependencyManager::get<DialogsManager>();
        dialogsManager->lodTools();
    } else {
        tablet->pushOntoStack("hifi/dialogs/TabletLODTools.qml");
    }
}

void Application::loadEntityStatisticsDialog() {
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet(SYSTEM_TABLET));
    if (tablet->getToolbarMode() || (!tablet->getTabletRoot() && !isHMDMode())) {
        auto dialogsManager = DependencyManager::get<DialogsManager>();
        dialogsManager->octreeStatsDetails();
    } else {
        tablet->pushOntoStack("hifi/dialogs/TabletEntityStatistics.qml");
    }
}

void Application::loadDomainConnectionDialog() {
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    auto tablet = dynamic_cast<TabletProxy*>(tabletScriptingInterface->getTablet(SYSTEM_TABLET));
    if (tablet->getToolbarMode() || (!tablet->getTabletRoot() && !isHMDMode())) {
        auto dialogsManager = DependencyManager::get<DialogsManager>();
        dialogsManager->showDomainConnectionDialog();
    } else {
        tablet->pushOntoStack("hifi/dialogs/TabletDCDialog.qml");
    }
}

void Application::toggleLogDialog() {
    if (! _logDialog) {
        _logDialog = new LogDialog(nullptr, getLogger());
    }

    if (_logDialog->isVisible()) {
        _logDialog->hide();
    } else {
        _logDialog->show();
    }
}

void Application::toggleEntityScriptServerLogDialog() {
    if (! _entityScriptServerLogDialog) {
        _entityScriptServerLogDialog = new EntityScriptServerLogDialog(nullptr);
    }

    if (_entityScriptServerLogDialog->isVisible()) {
        _entityScriptServerLogDialog->hide();
    } else {
        _entityScriptServerLogDialog->show();
    }
}

void Application::loadAddAvatarBookmarkDialog() const {
    auto avatarBookmarks = DependencyManager::get<AvatarBookmarks>();
}

void Application::loadAvatarBrowser() const {
    auto tablet = dynamic_cast<TabletProxy*>(DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system"));
    // construct the url to the marketplace item
    QString url = NetworkingConstants::METAVERSE_SERVER_URL().toString() + "/marketplace?category=avatars";
    QString MARKETPLACES_INJECT_SCRIPT_PATH = "file:///" + qApp->applicationDirPath() + "/scripts/system/html/js/marketplacesInject.js";
    tablet->gotoWebScreen(url, MARKETPLACES_INJECT_SCRIPT_PATH);
    DependencyManager::get<HMDScriptingInterface>()->openTablet();
}

void Application::takeSnapshot(bool notify, bool includeAnimated, float aspectRatio, const QString& filename) {
    postLambdaEvent([notify, includeAnimated, aspectRatio, filename, this] {
        // Get a screenshot and save it
        QString path = DependencyManager::get<Snapshot>()->saveSnapshot(getActiveDisplayPlugin()->getScreenshot(aspectRatio), filename,
                                              TestScriptingInterface::getInstance()->getTestResultsLocation());

        // If we're not doing an animated snapshot as well...
        if (!includeAnimated) {
            if (!path.isEmpty()) {
                // Tell the dependency manager that the capture of the still snapshot has taken place.
                emit DependencyManager::get<WindowScriptingInterface>()->stillSnapshotTaken(path, notify);
            }
        } else if (!SnapshotAnimated::isAlreadyTakingSnapshotAnimated()) {
            // Get an animated GIF snapshot and save it
            SnapshotAnimated::saveSnapshotAnimated(path, aspectRatio, qApp, DependencyManager::get<WindowScriptingInterface>());
        }
    });
}

void Application::takeSecondaryCameraSnapshot(const bool& notify, const QString& filename) {
    postLambdaEvent([notify, filename, this] {
        QString snapshotPath = DependencyManager::get<Snapshot>()->saveSnapshot(getActiveDisplayPlugin()->getSecondaryCameraScreenshot(), filename,
                                                      TestScriptingInterface::getInstance()->getTestResultsLocation());

        emit DependencyManager::get<WindowScriptingInterface>()->stillSnapshotTaken(snapshotPath, notify);
    });
}

void Application::takeSecondaryCamera360Snapshot(const glm::vec3& cameraPosition, const bool& cubemapOutputFormat, const bool& notify, const QString& filename) {
    postLambdaEvent([notify, filename, cubemapOutputFormat, cameraPosition] {
        DependencyManager::get<Snapshot>()->save360Snapshot(cameraPosition, cubemapOutputFormat, notify, filename);
    });
}

void Application::shareSnapshot(const QString& path, const QUrl& href) {
    postLambdaEvent([path, href] {
        // not much to do here, everything is done in snapshot code...
        DependencyManager::get<Snapshot>()->uploadSnapshot(path, href);
    });
}

float Application::getRenderResolutionScale() const {
    auto menu = Menu::getInstance();
    if (!menu) {
        return 1.0f;
    }
    if (menu->isOptionChecked(MenuOption::RenderResolutionOne)) {
        return 1.0f;
    } else if (menu->isOptionChecked(MenuOption::RenderResolutionTwoThird)) {
        return 0.666f;
    } else if (menu->isOptionChecked(MenuOption::RenderResolutionHalf)) {
        return 0.5f;
    } else if (menu->isOptionChecked(MenuOption::RenderResolutionThird)) {
        return 0.333f;
    } else if (menu->isOptionChecked(MenuOption::RenderResolutionQuarter)) {
        return 0.25f;
    } else {
        return 1.0f;
    }
}

void Application::notifyPacketVersionMismatch() {
    if (!_notifiedPacketVersionMismatchThisDomain && !isInterstitialMode()) {
        _notifiedPacketVersionMismatchThisDomain = true;

        QString message = "The location you are visiting is running an incompatible server version.\n";
        message += "Content may not display properly.";

        OffscreenUi::asyncWarning("", message);
    }
}

void Application::checkSkeleton() const {
    if (getMyAvatar()->getSkeletonModel()->isActive() && !getMyAvatar()->getSkeletonModel()->hasSkeleton()) {
        qCDebug(interfaceapp) << "MyAvatar model has no skeleton";

        QString message = "Your selected avatar body has no skeleton.\n\nThe default body will be loaded...";
        OffscreenUi::asyncWarning("", message);

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

void Application::windowMinimizedChanged(bool minimized) {
    // initialize the _minimizedWindowTimer
    static std::once_flag once;
    std::call_once(once, [&] {
        connect(&_minimizedWindowTimer, &QTimer::timeout, this, [] {
            QCoreApplication::postEvent(QCoreApplication::instance(), new QEvent(static_cast<QEvent::Type>(Idle)), Qt::HighEventPriority);
        });
    });

    // avoid rendering to the display plugin but continue posting Idle events,
    // so that physics continues to simulate and the deadlock watchdog knows we're alive
    if (!minimized && !getActiveDisplayPlugin()->isActive()) {
        _minimizedWindowTimer.stop();
        getActiveDisplayPlugin()->activate();
    } else if (minimized && getActiveDisplayPlugin()->isActive()) {
        getActiveDisplayPlugin()->deactivate();
        _minimizedWindowTimer.start(THROTTLED_SIM_FRAME_PERIOD_MS);
    }
}

void Application::postLambdaEvent(const std::function<void()>& f) {
    if (this->thread() == QThread::currentThread()) {
        f();
    } else {
        QCoreApplication::postEvent(this, new LambdaEvent(f));
    }
}

void Application::sendLambdaEvent(const std::function<void()>& f) {
    if (this->thread() == QThread::currentThread()) {
        f();
    } else {
        LambdaEvent event(f);
        QCoreApplication::sendEvent(this, &event);
    }
}

void Application::initPlugins(const QStringList& arguments) {
    QCommandLineOption display("display", "Preferred displays", "displays");
    QCommandLineOption disableDisplays("disable-displays", "Displays to disable", "displays");
    QCommandLineOption disableInputs("disable-inputs", "Inputs to disable", "inputs");

    QCommandLineParser parser;
    parser.addOption(display);
    parser.addOption(disableDisplays);
    parser.addOption(disableInputs);
    parser.parse(arguments);

    if (parser.isSet(display)) {
        auto preferredDisplays = parser.value(display).split(',', QString::SkipEmptyParts);
        qInfo() << "Setting prefered display plugins:" << preferredDisplays;
        PluginManager::getInstance()->setPreferredDisplayPlugins(preferredDisplays);
    }

    if (parser.isSet(disableDisplays)) {
        auto disabledDisplays = parser.value(disableDisplays).split(',', QString::SkipEmptyParts);
        qInfo() << "Disabling following display plugins:"  << disabledDisplays;
        PluginManager::getInstance()->disableDisplays(disabledDisplays);
    }

    if (parser.isSet(disableInputs)) {
        auto disabledInputs = parser.value(disableInputs).split(',', QString::SkipEmptyParts);
        qInfo() << "Disabling following input plugins:" << disabledInputs;
        PluginManager::getInstance()->disableInputs(disabledInputs);
    }
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
    static const uint MIN_SIZE = 1;
    glm::uvec2 result(MIN_SIZE);
    if (_displayPlugin) {
        result = getActiveDisplayPlugin()->getRecommendedUiSize();
    }
    return result;
}

QRect Application::getRecommendedHUDRect() const {
    auto uiSize = getUiSize();
    QRect result(0, 0, uiSize.x, uiSize.y);
    if (_displayPlugin) {
        result = getActiveDisplayPlugin()->getRecommendedHUDRect();
    }
    return result;
}

glm::vec2 Application::getDeviceSize() const {
    static const int MIN_SIZE = 1;
    glm::vec2 result(MIN_SIZE);
    if (_displayPlugin) {
        result = getActiveDisplayPlugin()->getRecommendedRenderSize();
    }
    return result;
}

bool Application::isThrottleRendering() const {
    if (_displayPlugin) {
        return getActiveDisplayPlugin()->isThrottled();
    }
    return false;
}

bool Application::hasFocus() const {
    bool result = (QApplication::activeWindow() != nullptr);
#if defined(Q_OS_WIN)
    // On Windows, QWidget::activateWindow() - as called in setFocus() - makes the application's taskbar icon flash but doesn't
    // take user focus away from their current window. So also check whether the application is the user's current foreground
    // window.
    result = result && (HWND)QApplication::activeWindow()->winId() == GetForegroundWindow();
#endif
    return result;
}

void Application::setFocus() {
    // Note: Windows doesn't allow a user focus to be taken away from another application. Instead, it changes the color of and
    // flashes the taskbar icon.
    auto window = qApp->getWindow();
    window->activateWindow();
}

void Application::raise() {
    auto windowState = qApp->getWindow()->windowState();
    if (windowState & Qt::WindowMinimized) {
        if (windowState & Qt::WindowMaximized) {
            qApp->getWindow()->showMaximized();
        } else if (windowState & Qt::WindowFullScreen) {
            qApp->getWindow()->showFullScreen();
        } else {
            qApp->getWindow()->showNormal();
        }
    }
    qApp->getWindow()->raise();
}

void Application::setMaxOctreePacketsPerSecond(int maxOctreePPS) {
    if (maxOctreePPS != _maxOctreePPS) {
        _maxOctreePPS = maxOctreePPS;
        maxOctreePacketsPerSecond.set(_maxOctreePPS);
    }
}

int Application::getMaxOctreePacketsPerSecond() const {
    return _maxOctreePPS;
}

qreal Application::getDevicePixelRatio() {
    return (_window && _window->windowHandle()) ? _window->windowHandle()->devicePixelRatio() : 1.0;
}

DisplayPluginPointer Application::getActiveDisplayPlugin() const {
    if (QThread::currentThread() != thread()) {
        std::unique_lock<std::mutex> lock(_displayPluginLock);
        return _displayPlugin;
    }

    if (!_aboutToQuit && !_displayPlugin) {
        const_cast<Application*>(this)->updateDisplayMode();
        Q_ASSERT(_displayPlugin);
    }
    return _displayPlugin;
}

static const char* EXCLUSION_GROUP_KEY = "exclusionGroup";

static void addDisplayPluginToMenu(const DisplayPluginPointer& displayPlugin, int index, bool active) {
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
        name, QKeySequence(Qt::CTRL + (Qt::Key_0 + index)), qApp,
        SLOT(updateDisplayMode()),
        QAction::NoRole, Menu::UNSPECIFIED_POSITION, groupingMenu);

    action->setCheckable(true);
    action->setChecked(active);
    displayPluginGroup->addAction(action);

    action->setProperty(EXCLUSION_GROUP_KEY, QVariant::fromValue(displayPluginGroup));
    Q_ASSERT(menu->menuItemExists(MenuOption::OutputMenu, name));
}

void Application::updateDisplayMode() {
    // Unsafe to call this method from anything but the main thread
    if (QThread::currentThread() != thread()) {
        qFatal("Attempted to switch display plugins from a non-main thread");
    }

    // Once time initialization code that depends on the UI being available
    auto displayPlugins = getDisplayPlugins();

    // Default to the first item on the list, in case none of the menu items match
    DisplayPluginPointer newDisplayPlugin = displayPlugins.at(0);
    auto menu = getPrimaryMenu();
    if (menu) {
        foreach(DisplayPluginPointer displayPlugin, PluginManager::getInstance()->getDisplayPlugins()) {
            QString name = displayPlugin->getName();
            QAction* action = menu->getActionForOption(name);
            // Menu might have been removed if the display plugin lost
            if (!action) {
                continue;
            }
            if (action->isChecked()) {
                newDisplayPlugin = displayPlugin;
                break;
            }
        }
    }

    if (newDisplayPlugin == _displayPlugin) {
        return;
    }

    setDisplayPlugin(newDisplayPlugin);
}

void Application::setDisplayPlugin(DisplayPluginPointer newDisplayPlugin) {
    if (newDisplayPlugin == _displayPlugin) {
        return;
    }

    // FIXME don't have the application directly set the state of the UI,
    // instead emit a signal that the display plugin is changing and let
    // the desktop lock itself.  Reduces coupling between the UI and display
    // plugins
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto desktop = offscreenUi->getDesktop();
    auto menu = Menu::getInstance();

    // Make the switch atomic from the perspective of other threads
    {
        std::unique_lock<std::mutex> lock(_displayPluginLock);
        bool wasRepositionLocked = false;
        if (desktop) {
            // Tell the desktop to no reposition (which requires plugin info), until we have set the new plugin, below.
            wasRepositionLocked = desktop->property("repositionLocked").toBool();
            desktop->setProperty("repositionLocked", true);
        }

        if (_displayPlugin) {
            disconnect(_displayPlugin.get(), &DisplayPlugin::presented, this, &Application::onPresent);
            _displayPlugin->deactivate();
        }

        auto oldDisplayPlugin = _displayPlugin;
        bool active = newDisplayPlugin->activate();

        if (!active) {
            auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();

            // If the new plugin fails to activate, fallback to last display
            qWarning() << "Failed to activate display: " << newDisplayPlugin->getName();
            newDisplayPlugin = oldDisplayPlugin;

            if (newDisplayPlugin) {
                qWarning() << "Falling back to last display: " << newDisplayPlugin->getName();
                active = newDisplayPlugin->activate();
            }

            // If there is no last display, or
            // If the last display fails to activate, fallback to desktop
            if (!active) {
                newDisplayPlugin = displayPlugins.at(0);
                qWarning() << "Falling back to display: " << newDisplayPlugin->getName();
                active = newDisplayPlugin->activate();
            }

            if (!active) {
                qFatal("Failed to activate fallback plugin");
            }
        }

        offscreenUi->resize(fromGlm(newDisplayPlugin->getRecommendedUiSize()));
        getApplicationCompositor().setDisplayPlugin(newDisplayPlugin);
        _displayPlugin = newDisplayPlugin;
        connect(_displayPlugin.get(), &DisplayPlugin::presented, this, &Application::onPresent, Qt::DirectConnection);
        if (desktop) {
            desktop->setProperty("repositionLocked", wasRepositionLocked);
        }
    }

    bool isHmd = _displayPlugin->isHmd();
    qCDebug(interfaceapp) << "Entering into" << (isHmd ? "HMD" : "Desktop") << "Mode";

    // Only log/emit after a successful change
    UserActivityLogger::getInstance().logAction("changed_display_mode", {
        { "previous_display_mode", _displayPlugin ? _displayPlugin->getName() : "" },
        { "display_mode", newDisplayPlugin ? newDisplayPlugin->getName() : "" },
        { "hmd", isHmd }
    });
    emit activeDisplayPluginChanged();

    // reset the avatar, to set head and hand palms back to a reasonable default pose.
    getMyAvatar()->reset(false);

    // switch to first person if entering hmd and setting is checked
    if (menu) {
        QAction* action = menu->getActionForOption(newDisplayPlugin->getName());
        if (action) {
            action->setChecked(true);
        }

        if (isHmd && menu->isOptionChecked(MenuOption::FirstPersonHMD)) {
            menu->setIsOptionChecked(MenuOption::FirstPerson, true);
            cameraMenuChanged();
        }

        // Remove the mirror camera option from menu if in HMD mode
        auto mirrorAction = menu->getActionForOption(MenuOption::FullscreenMirror);
        mirrorAction->setVisible(!isHmd);
    }

    Q_ASSERT_X(_displayPlugin, "Application::updateDisplayMode", "could not find an activated display plugin");
}

void Application::switchDisplayMode() {
    if (!_autoSwitchDisplayModeSupportedHMDPlugin) {
        return;
    }
    bool currentHMDWornStatus = _autoSwitchDisplayModeSupportedHMDPlugin->isDisplayVisible();
    if (currentHMDWornStatus != _previousHMDWornStatus) {
        // Switch to respective mode as soon as currentHMDWornStatus changes
        if (currentHMDWornStatus) {
            qCDebug(interfaceapp) << "Switching from Desktop to HMD mode";
            endHMDSession();
            setActiveDisplayPlugin(_autoSwitchDisplayModeSupportedHMDPluginName);
        } else {
            qCDebug(interfaceapp) << "Switching from HMD to desktop mode";
            setActiveDisplayPlugin(DESKTOP_DISPLAY_PLUGIN_NAME);
            startHMDStandBySession();
        }
    }
    _previousHMDWornStatus = currentHMDWornStatus;
}

void Application::setShowBulletWireframe(bool value) {
    _physicsEngine->setShowBulletWireframe(value);
}

void Application::setShowBulletAABBs(bool value) {
    _physicsEngine->setShowBulletAABBs(value);
}

void Application::setShowBulletContactPoints(bool value) {
    _physicsEngine->setShowBulletContactPoints(value);
}

void Application::setShowBulletConstraints(bool value) {
    _physicsEngine->setShowBulletConstraints(value);
}

void Application::setShowBulletConstraintLimits(bool value) {
    _physicsEngine->setShowBulletConstraintLimits(value);
}

void Application::startHMDStandBySession() {
    _autoSwitchDisplayModeSupportedHMDPlugin->startStandBySession();
}

void Application::endHMDSession() {
    _autoSwitchDisplayModeSupportedHMDPlugin->endSession();
}

mat4 Application::getEyeProjection(int eye) const {
    QMutexLocker viewLocker(&_viewMutex);
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

void Application::deadlockApplication() {
    qCDebug(interfaceapp) << "Intentionally deadlocked Interface";
    // Using a loop that will *technically* eventually exit (in ~600 billion years)
    // to avoid compiler warnings about a loop that will never exit
    for (uint64_t i = 1; i != 0; ++i) {
        QThread::sleep(1);
    }
}

// cause main thread to be unresponsive for 35 seconds
void Application::unresponsiveApplication() {
    // to avoid compiler warnings about a loop that will never exit
    uint64_t start = usecTimestampNow();
    uint64_t UNRESPONSIVE_FOR_SECONDS = 35;
    uint64_t UNRESPONSIVE_FOR_USECS = UNRESPONSIVE_FOR_SECONDS * USECS_PER_SECOND;
    qCDebug(interfaceapp) << "Intentionally cause Interface to be unresponsive for " << UNRESPONSIVE_FOR_SECONDS << " seconds";
    while (usecTimestampNow() - start < UNRESPONSIVE_FOR_USECS) {
        QThread::sleep(1);
    }
}

void Application::setActiveDisplayPlugin(const QString& pluginName) {
    DisplayPluginPointer newDisplayPlugin;
    for (DisplayPluginPointer displayPlugin : PluginManager::getInstance()->getDisplayPlugins()) {
        QString name = displayPlugin->getName();
        if (pluginName == name) {
            newDisplayPlugin = displayPlugin;
            break;
        }
    }

    if (newDisplayPlugin) {
        setDisplayPlugin(newDisplayPlugin);
    }
}

void Application::handleLocalServerConnection() const {
    auto server = qobject_cast<QLocalServer*>(sender());

    qCDebug(interfaceapp) << "Got connection on local server from additional instance - waiting for parameters";

    auto socket = server->nextPendingConnection();

    connect(socket, &QLocalSocket::readyRead, this, &Application::readArgumentsFromLocalSocket);

    qApp->getWindow()->raise();
    qApp->getWindow()->activateWindow();
}

void Application::readArgumentsFromLocalSocket() const {
    auto socket = qobject_cast<QLocalSocket*>(sender());

    auto message = socket->readAll();
    socket->deleteLater();

    qCDebug(interfaceapp) << "Read from connection: " << message;

    // If we received a message, try to open it as a URL
    if (message.length() > 0) {
        qApp->openUrl(QString::fromUtf8(message));
    }
}

void Application::showDesktop() {
}

CompositorHelper& Application::getApplicationCompositor() const {
    return *DependencyManager::get<CompositorHelper>();
}


// virtual functions required for PluginContainer
ui::Menu* Application::getPrimaryMenu() {
    auto appMenu = _window->menuBar();
    auto uiMenu = dynamic_cast<ui::Menu*>(appMenu);
    return uiMenu;
}

void Application::showDisplayPluginsTools(bool show) {
    DependencyManager::get<DialogsManager>()->hmdTools(show);
}

GLWidget* Application::getPrimaryWidget() {
    return _glWidget;
}

MainWindow* Application::getPrimaryWindow() {
    return getWindow();
}

QOpenGLContext* Application::getPrimaryContext() {
    return _glWidget->qglContext();
}

bool Application::makeRenderingContextCurrent() {
    return true;
}

bool Application::isForeground() const {
    return _isForeground && !_window->isMinimized();
}

// FIXME?  perhaps two, one for the main thread and one for the offscreen UI rendering thread?
static const int UI_RESERVED_THREADS = 1;
// Windows won't let you have all the cores
static const int OS_RESERVED_THREADS = 1;

void Application::updateThreadPoolCount() const {
    auto reservedThreads = UI_RESERVED_THREADS + OS_RESERVED_THREADS + _displayPlugin->getRequiredThreadCount();
    auto availableThreads = QThread::idealThreadCount() - reservedThreads;
    auto threadPoolSize = std::max(MIN_PROCESSING_THREAD_POOL_SIZE, availableThreads);
    qCDebug(interfaceapp) << "Ideal Thread Count " << QThread::idealThreadCount();
    qCDebug(interfaceapp) << "Reserved threads " << reservedThreads;
    qCDebug(interfaceapp) << "Setting thread pool size to " << threadPoolSize;
    QThreadPool::globalInstance()->setMaxThreadCount(threadPoolSize);
}

void Application::updateSystemTabletMode() {
    if (_settingsLoaded) {
        qApp->setProperty(hifi::properties::HMD, isHMDMode());
        if (isHMDMode()) {
            DependencyManager::get<TabletScriptingInterface>()->setToolbarMode(getHmdTabletBecomesToolbarSetting());
        } else {
            DependencyManager::get<TabletScriptingInterface>()->setToolbarMode(getDesktopTabletBecomesToolbarSetting());
        }
    }
}

OverlayID Application::getTabletScreenID() const {
    auto HMD = DependencyManager::get<HMDScriptingInterface>();
    return HMD->getCurrentTabletScreenID();
}

OverlayID Application::getTabletHomeButtonID() const {
    auto HMD = DependencyManager::get<HMDScriptingInterface>();
    return HMD->getCurrentHomeButtonID();
}

QUuid Application::getTabletFrameID() const {
    auto HMD = DependencyManager::get<HMDScriptingInterface>();
    return HMD->getCurrentTabletFrameID();
}

void Application::setAvatarOverrideUrl(const QUrl& url, bool save) {
    _avatarOverrideUrl = url;
    _saveAvatarOverrideUrl = save;
}

void Application::saveNextPhysicsStats(QString filename) {
    _physicsEngine->saveNextPhysicsStats(filename);
}

void Application::copyToClipboard(const QString& text) {
    if (QThread::currentThread() != qApp->thread()) {
        QMetaObject::invokeMethod(this, "copyToClipboard");
        return;
    }

    // assume that the address is being copied because the user wants a shareable address
    QApplication::clipboard()->setText(text);
}

#if defined(Q_OS_ANDROID)
void Application::beforeEnterBackground() {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->setSendDomainServerCheckInEnabled(false);
    nodeList->reset(true);
    clearDomainOctreeDetails();
}

void Application::enterBackground() {
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(),
                              "stop", Qt::BlockingQueuedConnection);
    if (getActiveDisplayPlugin()->isActive()) {
        getActiveDisplayPlugin()->deactivate();
    }
}

void Application::enterForeground() {
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(),
                                  "start", Qt::BlockingQueuedConnection);
    if (!getActiveDisplayPlugin() || getActiveDisplayPlugin()->isActive() || !getActiveDisplayPlugin()->activate()) {
        qWarning() << "Could not re-activate display plugin";
    }
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->setSendDomainServerCheckInEnabled(true);
}
#endif

#include "Application.moc"
