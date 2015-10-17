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

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QAbstractNativeEventFilter>
#include <QActionGroup>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QImage>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMediaPlayer>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QNetworkDiskCache>
#include <QObject>
#include <QScreen>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>
#include <QWindow>

#include <AccountManager.h>
#include <AddressManager.h>
#include <ApplicationVersion.h>
#include <AssetClient.h>
#include <AssetUpload.h>
#include <AutoUpdater.h>
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
#include <input-plugins/Joystick.h> // this should probably be removed
#include <input-plugins/UserInputMapper.h>
#include <LogHandler.h>
#include <MainWindow.h>
#include <MessageDialog.h>
#include <ModelEntityItem.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <ObjectMotionState.h>
#include <OctalCode.h>
#include <OctreeSceneStats.h>
#include <OffscreenGlCanvas.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <PhysicsEngine.h>
#include <plugins/PluginContainer.h>
#include <plugins/PluginManager.h>
#include <RenderableWebEntityItem.h>
#include <RenderDeferredTask.h>
#include <ResourceCache.h>
#include <SceneScriptingInterface.h>
#include <ScriptCache.h>
#include <SoundCache.h>
#include <TextureCache.h>
#include <Tooltip.h>
#include <udt/PacketHeaders.h>
#include <UserActivityLogger.h>
#include <UUID.h>
#include <VrMenu.h>

#include "AnimDebugDraw.h"
#include "AudioClient.h"
#include "audio/AudioScope.h"
#include "avatar/AvatarManager.h"
#include "CrashHandler.h"
#include "devices/3DConnexionClient.h"
#include "devices/DdeFaceTracker.h"
#include "devices/EyeTracker.h"
#include "devices/Faceshift.h"
#include "devices/Leapmotion.h"
#include "devices/MIDIManager.h"
#include "devices/RealSense.h"
#include "DiscoverabilityManager.h"
#include "GLCanvas.h"
#include "InterfaceActionFactory.h"
#include "InterfaceLogging.h"
#include "LODManager.h"
#include "Menu.h"
#include "ModelPackager.h"
#include "PluginContainerProxy.h"
#include "scripting/AccountScriptingInterface.h"
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
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif
#include "Stars.h"
#include "ui/AddressBarDialog.h"
#include "ui/AvatarInputs.h"
#include "ui/AssetUploadDialogFactory.h"
#include "ui/DataWebDialog.h"
#include "ui/DialogsManager.h"
#include "ui/LoginDialog.h"
#include "ui/overlays/Cube3DOverlay.h"
#include "ui/Snapshot.h"
#include "ui/StandAloneJSConsole.h"
#include "ui/Stats.h"
#include "ui/UpdateDialog.h"
#include "Util.h"

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
static QTimer billboardPacketTimer;
static QTimer checkFPStimer;
static QTimer idleTimer;

static const QString SNAPSHOT_EXTENSION  = ".jpg";
static const QString SVO_EXTENSION  = ".svo";
static const QString SVO_JSON_EXTENSION  = ".svo.json";
static const QString JS_EXTENSION  = ".js";
static const QString FST_EXTENSION  = ".fst";
static const QString FBX_EXTENSION  = ".fbx";
static const QString OBJ_EXTENSION  = ".obj";

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

static const unsigned int TARGET_SIM_FRAMERATE = 60;
static const unsigned int THROTTLED_SIM_FRAMERATE = 15;
static const int TARGET_SIM_FRAME_PERIOD_MS = MSECS_PER_SECOND / TARGET_SIM_FRAMERATE;
static const int THROTTLED_SIM_FRAME_PERIOD_MS = MSECS_PER_SECOND / THROTTLED_SIM_FRAMERATE;

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
    { FST_EXTENSION, &Application::askToSetAvatarUrl }
};

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
    Lambda = QEvent::User + 1
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
    QCoreApplication::setApplicationVersion(BUILD_VERSION);

    Setting::preInit();

    CrashHandler::checkForAndHandleCrash();
    CrashHandler::writeRunningMarkerFiler();
    qAddPostRoutine(CrashHandler::deleteRunningMarkerFile);

    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    DependencyManager::registerInheritance<AvatarHashMap, AvatarManager>();
    DependencyManager::registerInheritance<EntityActionFactoryInterface, InterfaceActionFactory>();

    Setting::init();

    // Set dependencies
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
    DependencyManager::set<EntityScriptingInterface>();
    DependencyManager::set<WindowScriptingInterface>();
    DependencyManager::set<HMDScriptingInterface>();
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    DependencyManager::set<SpeechRecognizer>();
#endif
    DependencyManager::set<DiscoverabilityManager>();
    DependencyManager::set<SceneScriptingInterface>();
    DependencyManager::set<OffscreenUi>();
    DependencyManager::set<AutoUpdater>();
    DependencyManager::set<PathUtils>();
    DependencyManager::set<InterfaceActionFactory>();
    DependencyManager::set<AssetClient>();
    DependencyManager::set<UserInputMapper>();

    return true;
}

// FIXME move to header, or better yet, design some kind of UI manager
// to take care of highlighting keyboard focused items, rather than
// continuing to overburden Application.cpp
Cube3DOverlay* _keyboardFocusHighlight{ nullptr };
int _keyboardFocusHighlightID{ -1 };
PluginContainer* _pluginContainer;

Application::Application(int& argc, char** argv, QElapsedTimer& startupTimer) :
        QApplication(argc, argv),
        _dependencyManagerIsSetup(setupEssentials(argc, argv)),
        _window(new MainWindow(desktop())),
        _toolWindow(NULL),
        _undoStackScriptingInterface(&_undoStack),
        _frameCount(0),
        _fps(60.0f),
        _physicsEngine(new PhysicsEngine(Vectors::ZERO)),
        _entities(true, this, this),
        _entityClipboardRenderer(false, this, this),
        _entityClipboard(new EntityTree()),
        _lastQueriedTime(usecTimestampNow()),
        _mirrorViewRect(QRect(MIRROR_VIEW_LEFT_PADDING, MIRROR_VIEW_TOP_PADDING, MIRROR_VIEW_WIDTH, MIRROR_VIEW_HEIGHT)),
        _firstRun("firstRun", true),
        _previousScriptLocation("LastScriptLocation", DESKTOP_LOCATION),
        _scriptsLocationHandle("scriptsLocation", DESKTOP_LOCATION),
        _fieldOfView("fieldOfView", DEFAULT_FIELD_OF_VIEW_DEGREES),
        _scaleMirror(1.0f),
        _rotateMirror(0.0f),
        _raiseMirror(0.0f),
        _lastMouseMoveWasSimulated(false),
        _enableProcessOctreeThread(true),
        _runningScriptsWidget(NULL),
        _runningScriptsWidgetWasVisible(false),
        _lastNackTime(usecTimestampNow()),
        _lastSendDownstreamAudioStats(usecTimestampNow()),
        _aboutToQuit(false),
        _notifiedPacketVersionMismatchThisDomain(false),
        _maxOctreePPS(maxOctreePacketsPerSecond.get()),
        _lastFaceTrackerUpdate(0)
{
    thread()->setObjectName("Main Thread");
    
    setInstance(this);
    
    // to work around the Qt constant wireless scanning, set the env for polling interval very high
    const QByteArray EXTREME_BEARER_POLL_TIMEOUT = QString::number(INT_MAX).toLocal8Bit();
    qputenv("QT_BEARER_POLL_TIMEOUT", EXTREME_BEARER_POLL_TIMEOUT);
    
    _entityClipboard->createRootElement();

    _pluginContainer = new PluginContainerProxy();
    Plugin::setContainer(_pluginContainer);
#ifdef Q_OS_WIN
    installNativeEventFilter(&MyNativeEventFilter::getInstance());
#endif

    _logger = new FileLogger(this);  // After setting organization name in order to get correct directory

    qInstallMessageHandler(messageHandler);

    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "styles/Inconsolata.otf");
    _window->setWindowTitle("Interface");

    Model::setAbstractViewStateInterface(this); // The model class will sometimes need to know view state details from us

    auto nodeList = DependencyManager::get<NodeList>();

    qCDebug(interfaceapp) << "[VERSION] Build sequence:" << qPrintable(applicationVersion());

    _bookmarks = new Bookmarks();  // Before setting up the menu

    _runningScriptsWidget = new RunningScriptsWidget(_window);
    _renderEngine->addTask(make_shared<RenderDeferredTask>());
    _renderEngine->registerScene(_main3DScene);

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

    // Setup AssetClient
    auto assetClient = DependencyManager::get<AssetClient>();
    QThread* assetThread = new QThread;
    assetThread->setObjectName("Asset Thread");
    assetClient->moveToThread(assetThread);
    connect(assetThread, &QThread::started, assetClient.data(), &AssetClient::init);
    assetThread->start();

    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    connect(&domainHandler, SIGNAL(hostnameChanged(const QString&)), SLOT(domainChanged(const QString&)));
    connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(connectedToDomain(const QString&)));
    connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(clearDomainOctreeDetails()));
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &Application::domainSettingsReceived);
    connect(&domainHandler, &DomainHandler::hostnameChanged,
            DependencyManager::get<AddressManager>().data(), &AddressManager::storeCurrentAddress);

    // update our location every 5 seconds in the metaverse server, assuming that we are authenticated with one
    const qint64 DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS = 5 * 1000;

    auto discoverabilityManager = DependencyManager::get<DiscoverabilityManager>();
    connect(&locationUpdateTimer, &QTimer::timeout, discoverabilityManager.data(), &DiscoverabilityManager::updateLocation);
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

    const qint64 BALANCE_UPDATE_INTERVAL_MSECS = 5 * 1000;

    connect(&balanceUpdateTimer, &QTimer::timeout, &accountManager, &AccountManager::updateBalance);
    balanceUpdateTimer.start(BALANCE_UPDATE_INTERVAL_MSECS);

    connect(&accountManager, &AccountManager::balanceChanged, this, &Application::updateWindowTitle);

    auto dialogsManager = DependencyManager::get<DialogsManager>();
    connect(&accountManager, &AccountManager::authRequired, dialogsManager.data(), &DialogsManager::showLoginDialog);
    connect(&accountManager, &AccountManager::usernameChanged, this, &Application::updateWindowTitle);

    // set the account manager's root URL and trigger a login request if we don't have the access token
    accountManager.setAuthURL(NetworkingConstants::METAVERSE_SERVER_URL);
    UserActivityLogger::getInstance().launch(applicationVersion());

    // once the event loop has started, check and signal for an access token
    QMetaObject::invokeMethod(&accountManager, "checkAndSignalForAccessToken", Qt::QueuedConnection);

    auto addressManager = DependencyManager::get<AddressManager>();

    // use our MyAvatar position and quat for address manager path
    addressManager->setPositionGetter([this]{ return getMyAvatar()->getPosition(); });
    addressManager->setOrientationGetter([this]{ return getMyAvatar()->getOrientation(); });

    connect(addressManager.data(), &AddressManager::hostChanged, this, &Application::updateWindowTitle);
    connect(this, &QCoreApplication::aboutToQuit, addressManager.data(), &AddressManager::storeCurrentAddress);

    #ifdef _WIN32
    WSADATA WsaData;
    int wsaresult = WSAStartup(MAKEWORD(2,2), &WsaData);
    #endif

    // tell the NodeList instance who to tell the domain server we care about
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
                                                 << NodeType::EntityServer << NodeType::AssetServer);

    // connect to the packet sent signal of the _entityEditSender
    connect(&_entityEditSender, &EntityEditPacketSender::packetSent, this, &Application::packetSent);

    // send the identity packet for our avatar each second to our avatar mixer
    connect(&identityPacketTimer, &QTimer::timeout, getMyAvatar(), &MyAvatar::sendIdentityPacket);
    identityPacketTimer.start(AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS);

    // send the billboard packet for our avatar every few seconds
    connect(&billboardPacketTimer, &QTimer::timeout, getMyAvatar(), &MyAvatar::sendBillboardPacket);
    billboardPacketTimer.start(AVATAR_BILLBOARD_PACKET_SEND_INTERVAL_MSECS);

    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkDiskCache* cache = new QNetworkDiskCache();
    cache->setMaximumCacheSize(MAXIMUM_CACHE_SIZE);
    cache->setCacheDirectory(!cachePath.isEmpty() ? cachePath : "interfaceCache");
    networkAccessManager.setCache(cache);

    ResourceCache::setRequestLimit(3);

    _glWidget = new GLCanvas();
    _window->setCentralWidget(_glWidget);

    _window->restoreGeometry();
    _window->setVisible(true);

    _glWidget->setFocusPolicy(Qt::StrongFocus);
    _glWidget->setFocus();
#ifdef Q_OS_MAC
    // OSX doesn't seem to provide for hiding the cursor only on the GL widget
    _window->setCursor(Qt::BlankCursor);
#else
    // On windows and linux, hiding the top level cursor also means it's invisible
    // when hovering over the window menu, which is a pain, so only hide it for
    // the GL surface
    _glWidget->setCursor(Qt::BlankCursor);
#endif

    // enable mouse tracking; otherwise, we only get drag events
    _glWidget->setMouseTracking(true);

    _offscreenContext = new OffscreenGlCanvas();
    _offscreenContext->create(_glWidget->context()->contextHandle());
    _offscreenContext->makeCurrent();
    initializeGL();


    _toolWindow = new ToolWindow();
    _toolWindow->setWindowFlags((_toolWindow->windowFlags() | Qt::WindowStaysOnTopHint) & ~Qt::WindowMinimizeButtonHint);
    _toolWindow->setWindowTitle("Tools");

    _offscreenContext->makeCurrent();

    // Tell our entity edit sender about our known jurisdictions
    _entityEditSender.setServerJurisdictions(&_entityServerJurisdictions);

    // For now we're going to set the PPS for outbound packets to be super high, this is
    // probably not the right long term solution. But for now, we're going to do this to
    // allow you to move an entity around in your hand
    _entityEditSender.setPacketsPerSecond(3000); // super high!!

    _overlays.init(); // do this before scripts load

    _runningScriptsWidget->setRunningScripts(getRunningScripts());

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(saveScripts()));
    connect(this, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

    // hook up bandwidth estimator
    QSharedPointer<BandwidthRecorder> bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();
    connect(nodeList.data(), &LimitedNodeList::dataSent,
            bandwidthRecorder.data(), &BandwidthRecorder::updateOutboundData);
    connect(&nodeList->getPacketReceiver(), &PacketReceiver::dataReceived,
            bandwidthRecorder.data(), &BandwidthRecorder::updateInboundData);

    connect(&getMyAvatar()->getSkeletonModel(), &SkeletonModel::skeletonLoaded,
            this, &Application::checkSkeleton, Qt::QueuedConnection);

    // Setup the userInputMapper with the actions
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    connect(userInputMapper.data(), &UserInputMapper::actionEvent, &_controllerScriptingInterface, &AbstractControllerScriptingInterface::actionEvent);
    connect(userInputMapper.data(), &UserInputMapper::actionEvent, [this](int action, float state) {
        if (state) {
            switch (action) {
            case UserInputMapper::Action::TOGGLE_MUTE:
                DependencyManager::get<AudioClient>()->toggleMute();
                break;
            }
        }
    });

    // Setup the keyboardMouseDevice and the user input mapper with the default bindings
    _keyboardMouseDevice->registerToUserInputMapper(*userInputMapper);
    _keyboardMouseDevice->assignDefaultInputMapping(*userInputMapper);

    // check first run...
    if (_firstRun.get()) {
        qCDebug(interfaceapp) << "This is a first run...";
        // clear the scripts, and set out script to our default scripts
        clearScriptsBeforeRunning();
        loadScript(DEFAULT_SCRIPTS_JS_URL);

        _firstRun.set(false);
    } else {
        // do this as late as possible so that all required subsystems are initialized
        loadScripts();
    }

    loadSettings();
    int SAVE_SETTINGS_INTERVAL = 10 * MSECS_PER_SECOND; // Let's save every seconds for now
    connect(&_settingsTimer, &QTimer::timeout, this, &Application::saveSettings);
    connect(&_settingsThread, SIGNAL(started()), &_settingsTimer, SLOT(start()));
    connect(&_settingsThread, SIGNAL(finished()), &_settingsTimer, SLOT(stop()));
    _settingsTimer.moveToThread(&_settingsThread);
    _settingsTimer.setSingleShot(false);
    _settingsTimer.setInterval(SAVE_SETTINGS_INTERVAL);
    _settingsThread.start();

    if (Menu::getInstance()->isOptionChecked(MenuOption::IndependentMode)) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
        cameraMenuChanged();
    }

    // set the local loopback interface for local sounds from audio scripts
    AudioScriptingInterface::getInstance().setLocalAudioInterface(audioIO.data());

#ifdef HAVE_RTMIDI
    // setup the MIDIManager
    MIDIManager& midiManagerInstance = MIDIManager::getInstance();
    midiManagerInstance.openDefaultPort();
#endif

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

    _oldHandMouseX[0] = -1;
    _oldHandMouseY[0] = -1;
    _oldHandMouseX[1] = -1;
    _oldHandMouseY[1] = -1;
    _oldHandLeftClick[0] = false;
    _oldHandRightClick[0] = false;
    _oldHandLeftClick[1] = false;
    _oldHandRightClick[1] = false;
    
    auto applicationUpdater = DependencyManager::get<AutoUpdater>();
    connect(applicationUpdater.data(), &AutoUpdater::newVersionIsAvailable, dialogsManager.data(), &DialogsManager::showUpdateDialog);
    applicationUpdater->checkForUpdate();

    // Now that menu is initalized we can sync myAvatar with it's state.
    getMyAvatar()->updateMotionBehaviorFromMenu();

    // the 3Dconnexion device wants to be initiliazed after a window is displayed.
    ConnexionClient::getInstance().init();

    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::DomainConnectionDenied, this, "handleDomainConnectionDeniedPacket");

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
        [=](const RayToEntityIntersectionResult& entityItemID, const QMouseEvent* event, unsigned int deviceId) {
        _keyboardFocusedItem = UNKNOWN_ENTITY_ID;
        if (_keyboardFocusHighlight) {
            _keyboardFocusHighlight->setVisible(false);
        }
    });

    connect(this, &Application::applicationStateChanged, this, &Application::activeChanged);
    
    qCDebug(interfaceapp, "Startup time: %4.2f seconds.", (double)startupTimer.elapsed() / 1000.0);
}

void Application::aboutToQuit() {
    emit beforeAboutToQuit();
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

    if (_keyboardFocusHighlightID > 0) {
        getOverlays().deleteOverlay(_keyboardFocusHighlightID);
        _keyboardFocusHighlightID = -1;
    }
    _keyboardFocusHighlight = nullptr;

    _entities.clear(); // this will allow entity scripts to properly shutdown
    
    // tell the packet receiver we're shutting down, so it can drop packets
    DependencyManager::get<NodeList>()->getPacketReceiver().setShouldDropPackets(true);
    
    _entities.shutdown(); // tell the entities system we're shutting down, so it will stop running scripts
    ScriptEngine::stopAllScripts(this); // stop all currently running global scripts

    // first stop all timers directly or by invokeMethod
    // depending on what thread they run in
    _avatarUpdate->terminate();
    locationUpdateTimer.stop();
    balanceUpdateTimer.stop();
    identityPacketTimer.stop();
    billboardPacketTimer.stop();
    checkFPStimer.stop();
    idleTimer.stop();
    QMetaObject::invokeMethod(&_settingsTimer, "stop", Qt::BlockingQueuedConnection);

    // save state
    _settingsThread.quit();
    saveSettings();
    _window->saveGeometry();

    // let the avatar mixer know we're out
    MyAvatar::sendKillAvatar();

    // stop the AudioClient
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(),
                              "stop", Qt::BlockingQueuedConnection);

    // destroy the AudioClient so it and its thread have a chance to go down safely
    DependencyManager::destroy<AudioClient>();

    // Destroy third party processes after scripts have finished using them.
#ifdef HAVE_DDE
    DependencyManager::destroy<DdeFaceTracker>();
#endif
#ifdef HAVE_IVIEWHMD
    DependencyManager::destroy<EyeTracker>();
#endif
}

void Application::emptyLocalCache() {
    if (auto cache = NetworkAccessManager::getInstance().cache()) {
        qDebug() << "DiskCacheEditor::clear(): Clearing disk cache.";
        cache->clear();
    }
}

Application::~Application() {
    EntityTreePointer tree = _entities.getTree();
    tree->setSimulation(NULL);

    _octreeProcessor.terminate();
    _entityEditSender.terminate();

    Menu::getInstance()->deleteLater();

    _physicsEngine->setCharacterController(NULL);

    ModelEntityItem::cleanupLoadedAnimations();

    foreach(auto inputPlugin, PluginManager::getInstance()->getInputPlugins()) {
        QString name = inputPlugin->getName();
        QAction* action = Menu::getInstance()->getActionForOption(name);
        if (action->isChecked()) {
            inputPlugin->deactivate();
        }
    }

    // remove avatars from physics engine
    DependencyManager::get<AvatarManager>()->clearOtherAvatars();
    VectorOfMotionStates motionStates;
    DependencyManager::get<AvatarManager>()->getObjectsToDelete(motionStates);
    _physicsEngine->deleteObjects(motionStates);

    DependencyManager::destroy<OffscreenUi>();
    DependencyManager::destroy<AvatarManager>();
    DependencyManager::destroy<AnimationCache>();
    DependencyManager::destroy<FramebufferCache>();
    DependencyManager::destroy<TextureCache>();
    DependencyManager::destroy<ModelCache>();
    DependencyManager::destroy<GeometryCache>();
    DependencyManager::destroy<ScriptCache>();
    DependencyManager::destroy<SoundCache>();
    
    // cleanup the AssetClient thread
    QThread* assetThread = DependencyManager::get<AssetClient>()->thread();
    DependencyManager::destroy<AssetClient>();
    assetThread->quit();
    assetThread->wait();

    QThread* nodeThread = DependencyManager::get<NodeList>()->thread();
    
    // remove the NodeList from the DependencyManager
    DependencyManager::destroy<NodeList>();

    // ask the node thread to quit and wait until it is done
    nodeThread->quit();
    nodeThread->wait();
    
    Leapmotion::destroy();
    RealSense::destroy();
    ConnexionClient::getInstance().destroy();

    qInstallMessageHandler(NULL); // NOTE: Do this as late as possible so we continue to get our log messages
}

void Application::initializeGL() {
    qCDebug(interfaceapp) << "Created Display Window.";

    // initialize glut for shape drawing; Qt apparently initializes it on OS X
    #ifndef __APPLE__
    static bool isInitialized = false;
    if (isInitialized) {
        return;
    } else {
        isInitialized = true;
    }
    #endif
    // Where the gpuContext is initialized and where the TRUE Backend is created and assigned
    gpu::Context::init<gpu::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();

    initDisplay();
    qCDebug(interfaceapp, "Initialized Display.");

    // The UI can't be created until the primary OpenGL
    // context is created, because it needs to share
    // texture resources
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

    // call our timer function every second
    connect(&checkFPStimer, &QTimer::timeout, this, &Application::checkFPS);
    checkFPStimer.start(1000);

    // call our idle function whenever we can
    connect(&idleTimer, &QTimer::timeout, this, &Application::idle);
    idleTimer.start(TARGET_SIM_FRAME_PERIOD_MS);
    _idleLoopStdev.reset();

    // update before the first render
    update(1.0f / _fps);

    InfoView::show(INFO_HELP_PATH, true);
}

void Application::initializeUi() {
    AddressBarDialog::registerType();
    ErrorDialog::registerType();
    LoginDialog::registerType();
    MessageDialog::registerType();
    VrMenu::registerType();
    Tooltip::registerType();
    UpdateDialog::registerType();

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->create(_offscreenContext->getContext());
    offscreenUi->setProxyWindow(_window->windowHandle());
    offscreenUi->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
    offscreenUi->load("Root.qml");
    offscreenUi->load("RootMenu.qml");
    _glWidget->installEventFilter(offscreenUi.data());
    VrMenu::load();
    VrMenu::executeQueuedLambdas();
    offscreenUi->setMouseTranslator([=](const QPointF& pt) {
        QPointF result = pt;
        auto displayPlugin = getActiveDisplayPlugin();
        if (displayPlugin->isHmd()) {
            auto resultVec = _compositor.screenToOverlay(toGlm(pt));
            result = QPointF(resultVec.x, resultVec.y);
        }
        return result;
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
            _keyboardMouseDevice = static_cast<KeyboardMouseDevice*>(inputPlugin.data()); // TODO: this seems super hacky
        }
    }
    updateInputModes();
}

void Application::paintGL() {
    PROFILE_RANGE(__FUNCTION__);
    PerformanceTimer perfTimer("paintGL");

    if (nullptr == _displayPlugin) {
        return;
    }

    // Some plugins process message events, potentially leading to 
    // re-entering a paint event.  don't allow further processing if this 
    // happens
    if (_inPaint) {
        return;
    }
    _inPaint = true;
    Finally clearFlagLambda([this] { _inPaint = false; });

    auto displayPlugin = getActiveDisplayPlugin();
    displayPlugin->preRender();
    _offscreenContext->makeCurrent();

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

    if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
        PerformanceTimer perfTimer("Mirror");
        auto primaryFbo = DependencyManager::get<FramebufferCache>()->getPrimaryFramebufferDepthColor();
        
        renderArgs._renderMode = RenderArgs::MIRROR_RENDER_MODE;
        renderRearViewMirror(&renderArgs, _mirrorViewRect);
        renderArgs._renderMode = RenderArgs::DEFAULT_RENDER_MODE;
        
        {
            float ratio = ((float)QApplication::desktop()->windowHandle()->devicePixelRatio() * getRenderResolutionScale());
            // Flip the src and destination rect horizontally to do the mirror
            auto mirrorRect = glm::ivec4(0, 0, _mirrorViewRect.width() * ratio, _mirrorViewRect.height() * ratio);
            auto mirrorRectDest = glm::ivec4(mirrorRect.z, mirrorRect.y, mirrorRect.x, mirrorRect.w);
            
            auto selfieFbo = DependencyManager::get<FramebufferCache>()->getSelfieFramebuffer();
            gpu::doInBatch(renderArgs._context, [=](gpu::Batch& batch) {
                batch.setFramebuffer(selfieFbo);
                batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
                batch.blit(primaryFbo, mirrorRect, selfieFbo, mirrorRectDest);
                batch.setFramebuffer(nullptr);
            });
        }
    }

    {
        PerformanceTimer perfTimer("renderOverlay");
        // NOTE: There is no batch associated with this renderArgs
        // the ApplicationOverlay class assumes it's viewport is setup to be the device size
        QSize size = getDeviceSize();
        renderArgs._viewport = glm::ivec4(0, 0, size.width(), size.height());
        _applicationOverlay.renderOverlay(&renderArgs);
    }

    {
        PerformanceTimer perfTimer("CameraUpdates");
        
        auto myAvatar = getMyAvatar();
        
        myAvatar->startCapture();
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
                auto worldBoomOffset = myAvatar->getOrientation() * (myAvatar->getScale() * myAvatar->getBoomLength() * glm::vec3(0.0f, 0.0f, 1.0f));
                _myCamera.setPosition(extractTranslation(hmdWorldMat) + worldBoomOffset);
            } else {
                _myCamera.setRotation(myAvatar->getHead()->getOrientation());
                if (Menu::getInstance()->isOptionChecked(MenuOption::CenterPlayerInView)) {
                    _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                        + _myCamera.getRotation()
                        * (myAvatar->getScale() * myAvatar->getBoomLength() * glm::vec3(0.0f, 0.0f, 1.0f)));
                } else {
                    _myCamera.setPosition(myAvatar->getDefaultEyePosition()
                        + myAvatar->getOrientation() 
                        * (myAvatar->getScale() * myAvatar->getBoomLength() * glm::vec3(0.0f, 0.0f, 1.0f)));
                }
            }
        } else if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
            if (isHMDMode()) {
                glm::quat hmdRotation = extractRotation(myAvatar->getHMDSensorMatrix());
                _myCamera.setRotation(myAvatar->getWorldAlignedOrientation() 
                    * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f)) * hmdRotation);
                glm::vec3 hmdOffset = extractTranslation(myAvatar->getHMDSensorMatrix());
                _myCamera.setPosition(myAvatar->getDefaultEyePosition() 
                    + glm::vec3(0, _raiseMirror * myAvatar->getScale(), 0) 
                    + (myAvatar->getOrientation() * glm::quat(glm::vec3(0.0f, _rotateMirror, 0.0f))) *
                    glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_FULLSCREEN_DISTANCE * _scaleMirror 
                    + (myAvatar->getOrientation() * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f))) * hmdOffset);
            } else {
                _myCamera.setRotation(myAvatar->getWorldAlignedOrientation() 
                    * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f)));
                _myCamera.setPosition(myAvatar->getDefaultEyePosition() 
                    + glm::vec3(0, _raiseMirror * myAvatar->getScale(), 0) 
                    + (myAvatar->getOrientation() * glm::quat(glm::vec3(0.0f, _rotateMirror, 0.0f))) *
                    glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_FULLSCREEN_DISTANCE * _scaleMirror);
            }
            renderArgs._renderMode = RenderArgs::MIRROR_RENDER_MODE;
        }
        // Update camera position 
        if (!isHMDMode()) {
            _myCamera.update(1.0f / _fps);
        }
        myAvatar->endCapture();
    }

    // Primary rendering pass
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    const QSize size = framebufferCache->getFrameBufferSize();
    {
        PROFILE_RANGE(__FUNCTION__ "/mainRender");
        PerformanceTimer perfTimer("mainRender");
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
            _myCamera.setProjection(displayPlugin->getProjection(Mono, _myCamera.getProjection()));
            renderArgs._context->enableStereo(true);
            mat4 eyeOffsets[2];
            mat4 eyeProjections[2];
            auto baseProjection = renderArgs._viewFrustum->getProjection();
            auto hmdInterface = DependencyManager::get<HMDScriptingInterface>();
            float IPDScale = hmdInterface->getIPDScale();
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
                mat4 headPose = displayPlugin->getHeadPose();
                displayPlugin->setEyeRenderPose(eye, headPose);

                eyeProjections[eye] = displayPlugin->getProjection(eye, baseProjection);
            });
            renderArgs._context->setStereoProjections(eyeProjections);
            renderArgs._context->setStereoViews(eyeOffsets);
        }
        displaySide(&renderArgs, _myCamera);
        renderArgs._context->enableStereo(false);
        gpu::doInBatch(renderArgs._context, [](gpu::Batch& batch) {
            batch.setFramebuffer(nullptr);
        });
    }

    // Overlay Composition, needs to occur after screen space effects have completed
    {
        PROFILE_RANGE(__FUNCTION__ "/compositor");
        PerformanceTimer perfTimer("compositor");
        auto primaryFbo = framebufferCache->getPrimaryFramebuffer();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(primaryFbo));
        if (displayPlugin->isStereo()) {
            QRect currentViewport(QPoint(0, 0), QSize(size.width() / 2, size.height()));
            glClear(GL_DEPTH_BUFFER_BIT);
            for_each_eye([&](Eye eye) {
                renderArgs._viewport = toGlm(currentViewport);
                if (displayPlugin->isHmd()) {
                    _compositor.displayOverlayTextureHmd(&renderArgs, eye);
                } else {
                    _compositor.displayOverlayTexture(&renderArgs);
                }
            }, [&] {
                currentViewport.moveLeft(currentViewport.width());
            });
        } else {
            glViewport(0, 0, size.width(), size.height());
            _compositor.displayOverlayTexture(&renderArgs);
        }
    }

    // deliver final composited scene to the display plugin
    {
        PROFILE_RANGE(__FUNCTION__ "/pluginOutput");
        PerformanceTimer perfTimer("pluginOutput");
        auto primaryFbo = framebufferCache->getPrimaryFramebuffer();
        GLuint finalTexture = gpu::GLBackend::getTextureID(primaryFbo->getRenderBuffer(0));
        // Ensure the rendering context commands are completed when rendering 
        GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        // Ensure the sync object is flushed to the driver thread before releasing the context
        // CRITICAL for the mac driver apparently.
        glFlush();
        _offscreenContext->doneCurrent();

        // Switches to the display plugin context
        displayPlugin->preDisplay();
        // Ensure all operations from the previous context are complete before we try to read the fbo
        glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
        glDeleteSync(sync);

        {
            PROFILE_RANGE(__FUNCTION__ "/pluginDisplay");
            PerformanceTimer perfTimer("pluginDisplay");
            displayPlugin->display(finalTexture, toGlm(size));
        }

        {
            PROFILE_RANGE(__FUNCTION__ "/bufferSwap");
            PerformanceTimer perfTimer("bufferSwap");
            displayPlugin->finishFrame();
        }
    }

    {
        PerformanceTimer perfTimer("makeCurrent");
        _offscreenContext->makeCurrent();
        _frameCount++;
        Stats::getInstance()->setRenderDetails(renderArgs._details);

        // Reset the gpu::Context Stages
        // Back to the default framebuffer;
        gpu::doInBatch(renderArgs._context, [=](gpu::Batch& batch) {
            batch.resetStages();
        });
    }
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

void Application::showEditEntitiesHelp() {
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
    QUrl url(urlString);
    emit svoImportRequested(url.url());
    return true; // assume it's accepted
}

bool Application::event(QEvent* event) {
    if ((int)event->type() == (int)Lambda) {
        ((LambdaEvent*)event)->call();
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
        _controllerScriptingInterface.handleMetaEvent(static_cast<HFMetaEvent*>(event));
    }

    return QApplication::event(event);
}

bool Application::eventFilter(QObject* object, QEvent* event) {
    if (event->type() == QEvent::ShortcutOverride) {
        if (DependencyManager::get<OffscreenUi>()->shouldSwallowShortcut(event)) {
            event->accept();
            return true;
        }

        // Filter out captured keys before they're used for shortcut actions.
        if (_controllerScriptingInterface.isKeyCaptured(static_cast<QKeyEvent*>(event))) {
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

    _controllerScriptingInterface.emitKeyPressEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isKeyCaptured(event)) {
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

            case Qt::Key_A:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Atmosphere);
                }
                break;

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
                    Menu::getInstance()->triggerOption(MenuOption::Mirror);
                } else {
                    Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, !Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror));
                    if (!Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
                        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
                    }
                    cameraMenuChanged();
                }
                break;
            case Qt::Key_P:
                 Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, !Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson));
                 Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, !Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson));
                 cameraMenuChanged();
                 break;
            case Qt::Key_O:
                _overlayConductor.setEnabled(!_overlayConductor.getEnabled());
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
            case Qt::Key_Space: {
                if (!event->isAutoRepeat()) {
                    // this starts an HFActionEvent
                    HFActionEvent startActionEvent(HFActionEvent::startType(),
                                                   computePickRay(getTrueMouse().x, getTrueMouse().y));
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
        if (getActiveDisplayPlugin()->isStereo()) {
            VrMenu::toggle();
        }
    }

    _keysPressed.remove(event->key());

    _controllerScriptingInterface.emitKeyReleaseEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isKeyCaptured(event)) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->keyReleaseEvent(event);
    }

    switch (event->key()) {
        case Qt::Key_Space: {
            if (!event->isAutoRepeat()) {
                // this ends the HFActionEvent
                HFActionEvent endActionEvent(HFActionEvent::endType(),
                                             computePickRay(getTrueMouse().x, getTrueMouse().y));
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
    ConnexionData::getInstance().focusOutEvent();

    // synthesize events for keys currently pressed, since we may not get their release events
    foreach (int key, _keysPressed) {
        QKeyEvent event(QEvent::KeyRelease, key, Qt::NoModifier);
        keyReleaseEvent(&event);
    }
    _keysPressed.clear();
}

void Application::mouseMoveEvent(QMouseEvent* event, unsigned int deviceID) {
    PROFILE_RANGE(__FUNCTION__);
    // Used by application overlay to determine how to draw cursor(s)
    _lastMouseMoveWasSimulated = deviceID > 0;

    if (_aboutToQuit) {
        return;
    }

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
            } else {
                if (event->pos().y() > MENU_TOGGLE_AREA) {
                    menuBar->setVisible(false);
                }
            }
        }
    }
#endif

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(event->localPos(), _glWidget);
    QMouseEvent mappedEvent(event->type(),
        transformedPos,
        event->screenPos(), event->button(),
        event->buttons(), event->modifiers());


    _entities.mouseMoveEvent(&mappedEvent, deviceID);
    _controllerScriptingInterface.emitMouseMoveEvent(&mappedEvent, deviceID); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }

    if (deviceID == 0 && Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->mouseMoveEvent(event, deviceID);
    }

}

void Application::mousePressEvent(QMouseEvent* event, unsigned int deviceID) {
    // Inhibit the menu if the user is using alt-mouse dragging
    _altPressed = false;

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(event->localPos(), _glWidget);
    QMouseEvent mappedEvent(event->type(),
        transformedPos,
        event->screenPos(), event->button(),
        event->buttons(), event->modifiers());

    if (!_aboutToQuit) {
        _entities.mousePressEvent(&mappedEvent, deviceID);
    }

    _controllerScriptingInterface.emitMousePressEvent(&mappedEvent); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }


    if (hasFocus()) {
        if (deviceID == 0 && Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
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

void Application::mouseDoublePressEvent(QMouseEvent* event, unsigned int deviceID) {
    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }

    _controllerScriptingInterface.emitMouseDoublePressEvent(event);
}

void Application::mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID) {

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    QPointF transformedPos = offscreenUi->mapToVirtualScreen(event->localPos(), _glWidget);
    QMouseEvent mappedEvent(event->type(),
        transformedPos,
        event->screenPos(), event->button(),
        event->buttons(), event->modifiers());

    if (!_aboutToQuit) {
        _entities.mouseReleaseEvent(&mappedEvent, deviceID);
    }

    _controllerScriptingInterface.emitMouseReleaseEvent(&mappedEvent); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }

    if (hasFocus()) {
        if (deviceID == 0 && Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
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
        _controllerScriptingInterface.emitTouchUpdateEvent(thisEvent); // send events to any registered scripts
        _lastTouchEvent = thisEvent;
    }

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isTouchCaptured()) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->touchUpdateEvent(event);
    }
}

void Application::touchBeginEvent(QTouchEvent* event) {
    _altPressed = false;
    TouchEvent thisEvent(*event); // on touch begin, we don't compare to last event
    _controllerScriptingInterface.emitTouchBeginEvent(thisEvent); // send events to any registered scripts

    _lastTouchEvent = thisEvent; // and we reset our last event to this event before we call our update
    touchUpdateEvent(event);

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isTouchCaptured()) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->touchBeginEvent(event);
    }

}

void Application::touchEndEvent(QTouchEvent* event) {
    _altPressed = false;
    TouchEvent thisEvent(*event, _lastTouchEvent);
    _controllerScriptingInterface.emitTouchEndEvent(thisEvent); // send events to any registered scripts
    _lastTouchEvent = thisEvent;

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isTouchCaptured()) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(KeyboardMouseDevice::NAME)) {
        _keyboardMouseDevice->touchEndEvent(event);
    }

    // put any application specific touch behavior below here..
}

void Application::wheelEvent(QWheelEvent* event) {
    _altPressed = false;
    _controllerScriptingInterface.emitWheelEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isWheelCaptured()) {
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
        QMessageBox msgBox;
        msgBox.setText("No location details were found in the file "
                        + snapshotPath + ", try dragging in an authentic Hifi snapshot.");

        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
    return true;
}

//  Every second, check the frame rates and other stuff
void Application::checkFPS() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        DependencyManager::get<NodeList>()->sendPingPackets();
    }

    float diffTime = (float)_timerStart.nsecsElapsed() / 1000000000.0f;

    _fps = (float)_frameCount / diffTime;
    _frameCount = 0;
    _timerStart.start();
}

void Application::idle() {
    if (_aboutToQuit) {
        return; // bail early, nothing to do here.
    }

    // depending on whether we're throttling or not.
    // Once rendering is off on another thread we should be able to have Application::idle run at start(0) in
    // perpetuity and not expect events to get backed up.
    bool isThrottled = getActiveDisplayPlugin()->isThrottled();
    //  Only run simulation code if more than the targetFramePeriod have passed since last time we ran
    // This attempts to lock the simulation at 60 updates per second, regardless of framerate
    float timeSinceLastUpdateUs = (float)_lastTimeUpdated.nsecsElapsed() / NSECS_PER_USEC;
    float secondsSinceLastUpdate = timeSinceLastUpdateUs / USECS_PER_SECOND;

    if (isThrottled && (timeSinceLastUpdateUs / USECS_PER_MSEC) < THROTTLED_SIM_FRAME_PERIOD_MS) {
        return; // bail early, we're throttled and not enough time has elapsed
    }

    _lastTimeUpdated.start();


    {
        PROFILE_RANGE(__FUNCTION__);
        uint64_t now = usecTimestampNow();
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
    InputDevice::setLowVelocityFilter(lowVelocityFilter);
}

ivec2 Application::getMouse() const {
    if (isHMDMode()) {
        return _compositor.screenToOverlay(getTrueMouse());
    }
    return getTrueMouse();
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

    auto entityTree = _entities.getTree();
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

    exportTree->writeToJSONFile(filename.toLocal8Bit().constData());

    // restore the main window's active state
    _window->activateWindow();
    return true;
}

bool Application::exportEntities(const QString& filename, float x, float y, float z, float scale) {
    QVector<EntityItemPointer> entities;
    _entities.getTree()->findEntities(AACube(glm::vec3(x, y, z), scale), entities);

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

    DependencyManager::get<AudioClient>()->loadSettings();
    DependencyManager::get<LODManager>()->loadSettings();

    // DONT CHECK IN
    //DependencyManager::get<LODManager>()->setAutomaticLODAdjust(false);

    Menu::getInstance()->loadSettings();
    getMyAvatar()->loadData();
}

void Application::saveSettings() {
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
        _entityClipboard->reaverageOctreeElements();
    }
    return success;
}

QVector<EntityItemID> Application::pasteEntities(float x, float y, float z) {
    return _entityClipboard->sendEntities(&_entityEditSender, _entities.getTree(), x, y, z);
}

void Application::initDisplay() {
}

void Application::init() {
    // Make sure Login state is up to date
    DependencyManager::get<DialogsManager>()->toggleLoginDialog();

    _environment.init();

    DependencyManager::get<DeferredLightingEffect>()->init(this);

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
    RealSense::init();

    // fire off an immediate domain-server check in now that settings are loaded
    DependencyManager::get<NodeList>()->sendDomainServerCheckIn();

    _entities.init();
    _entities.setViewFrustum(getViewFrustum());

    ObjectMotionState::setShapeManager(&_shapeManager);
    _physicsEngine->init();

    EntityTreePointer tree = _entities.getTree();
    _entitySimulation.init(tree, _physicsEngine, &_entityEditSender);
    tree->setSimulation(&_entitySimulation);

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    // connect the _entityCollisionSystem to our EntityTreeRenderer since that's what handles running entity scripts
    connect(&_entitySimulation, &EntitySimulation::entityCollisionWithEntity,
            &_entities, &EntityTreeRenderer::entityCollisionWithEntity);

    // connect the _entities (EntityTreeRenderer) to our script engine's EntityScriptingInterface for firing
    // of events related clicking, hovering over, and entering entities
    _entities.connectSignalsToSlots(entityScriptingInterface.data());

    _entityClipboardRenderer.init();
    _entityClipboardRenderer.setViewFrustum(getViewFrustum());
    _entityClipboardRenderer.setTree(_entityClipboard);

    // Make sure any new sounds are loaded as soon as know about them.
    connect(tree.get(), &EntityTree::newCollisionSoundURL, DependencyManager::get<SoundCache>().data(), &SoundCache::getSound);
    connect(getMyAvatar(), &MyAvatar::newCollisionSoundURL, DependencyManager::get<SoundCache>().data(), &SoundCache::getSound);

    setAvatarUpdateThreading();
}

void Application::setAvatarUpdateThreading() {
    setAvatarUpdateThreading(Menu::getInstance()->isOptionChecked(MenuOption::EnableAvatarUpdateThreading));
}
void Application::setRawAvatarUpdateThreading() {
    setRawAvatarUpdateThreading(Menu::getInstance()->isOptionChecked(MenuOption::EnableAvatarUpdateThreading));
}
void Application::setRawAvatarUpdateThreading(bool isThreaded) {
    if (_avatarUpdate) {
        if (_avatarUpdate->isThreaded() == isThreaded) {
            return;
        }
        _avatarUpdate->terminate();
    }
    _avatarUpdate = new AvatarUpdate();
    _avatarUpdate->initialize(isThreaded);
}
void Application::setAvatarUpdateThreading(bool isThreaded) {
    if (_avatarUpdate && (_avatarUpdate->isThreaded() == isThreaded)) {
        return;
    }
    
    auto myAvatar = getMyAvatar();
    bool isRigEnabled = myAvatar->getEnableRigAnimations();
    bool isGraphEnabled = myAvatar->getEnableAnimGraph();
    if (_avatarUpdate) {
        _avatarUpdate->terminate(); // Must be before we shutdown anim graph.
    }
    myAvatar->setEnableRigAnimations(false);
    myAvatar->setEnableAnimGraph(false);
    _avatarUpdate = new AvatarUpdate();
    _avatarUpdate->initialize(isThreaded);
    myAvatar->setEnableRigAnimations(isRigEnabled);
    myAvatar->setEnableAnimGraph(isGraphEnabled);
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
    bool isHMD = _avatarUpdate->isHMDMode();
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
                glm::mat4 headPose = _avatarUpdate->getHeadPose() ;
                glm::quat headRotation = glm::quat_cast(headPose);
                lookAtSpot = _myCamera.getPosition() +
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
    }
}

void Application::reloadResourceCaches() {
    // Clear entities out of view frustum
    _viewFrustum.setPosition(glm::vec3(0.0f, 0.0f, TREE_SCALE));
    _viewFrustum.setOrientation(glm::quat());
    queryOctree(NodeType::EntityServer, PacketType::EntityQuery, _entityServerJurisdictions);

    emptyLocalCache();

    DependencyManager::get<AnimationCache>()->refreshAll();
    DependencyManager::get<ModelCache>()->refreshAll();
    DependencyManager::get<SoundCache>()->refreshAll();
    DependencyManager::get<TextureCache>()->refreshAll();

    DependencyManager::get<NodeList>()->reset();  // Force redownload of .fst models
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
    userInputMapper->setSensorToWorldMat(myAvatar->getSensorToWorldMatrix());
    userInputMapper->update(deltaTime);

    // This needs to go after userInputMapper->update() because of the keyboard
    bool jointsCaptured = false;
    auto inputPlugins = PluginManager::getInstance()->getInputPlugins();
    foreach(auto inputPlugin, inputPlugins) {
        QString name = inputPlugin->getName();
        QAction* action = Menu::getInstance()->getActionForOption(name);
        if (action && action->isChecked()) {
            inputPlugin->pluginUpdate(deltaTime, jointsCaptured);
            if (inputPlugin->isJointController()) {
                jointsCaptured = true;
            }
        }
    }

    // Dispatch input events
    _controllerScriptingInterface.updateInputControllers();

    // Transfer the user inputs to the driveKeys
    myAvatar->clearDriveKeys();
    if (_myCamera.getMode() != CAMERA_MODE_INDEPENDENT) {
        if (!_controllerScriptingInterface.areActionsCaptured()) {
            myAvatar->setDriveKeys(FWD, userInputMapper->getActionState(UserInputMapper::LONGITUDINAL_FORWARD));
            myAvatar->setDriveKeys(BACK, userInputMapper->getActionState(UserInputMapper::LONGITUDINAL_BACKWARD));
            myAvatar->setDriveKeys(UP, userInputMapper->getActionState(UserInputMapper::VERTICAL_UP));
            myAvatar->setDriveKeys(DOWN, userInputMapper->getActionState(UserInputMapper::VERTICAL_DOWN));
            myAvatar->setDriveKeys(LEFT, userInputMapper->getActionState(UserInputMapper::LATERAL_LEFT));
            myAvatar->setDriveKeys(RIGHT, userInputMapper->getActionState(UserInputMapper::LATERAL_RIGHT));
            if (deltaTime > FLT_EPSILON) {
                // For rotations what we really want are meausures of "angles per second" (in order to prevent 
                // fps-dependent spin rates) so we need to scale the units of the controller contribution.
                // (TODO?: maybe we should similarly scale ALL action state info, or change the expected behavior 
                // controllers to provide a delta_per_second value rather than a raw delta.)
                const float EXPECTED_FRAME_RATE = 60.0f;
                float timeFactor = EXPECTED_FRAME_RATE * deltaTime;
                myAvatar->setDriveKeys(ROT_UP, userInputMapper->getActionState(UserInputMapper::PITCH_UP) / timeFactor);
                myAvatar->setDriveKeys(ROT_DOWN, userInputMapper->getActionState(UserInputMapper::PITCH_DOWN) / timeFactor);
                myAvatar->setDriveKeys(ROT_LEFT, userInputMapper->getActionState(UserInputMapper::YAW_LEFT) / timeFactor);
                myAvatar->setDriveKeys(ROT_RIGHT, userInputMapper->getActionState(UserInputMapper::YAW_RIGHT) / timeFactor);
            }
        }
        myAvatar->setDriveKeys(BOOM_IN, userInputMapper->getActionState(UserInputMapper::BOOM_IN));
        myAvatar->setDriveKeys(BOOM_OUT, userInputMapper->getActionState(UserInputMapper::BOOM_OUT));
    }
    UserInputMapper::PoseValue leftHand = userInputMapper->getPoseState(UserInputMapper::LEFT_HAND);
    UserInputMapper::PoseValue rightHand = userInputMapper->getPoseState(UserInputMapper::RIGHT_HAND);
    Hand* hand = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHand();
    setPalmData(hand, leftHand, deltaTime, LEFT_HAND_INDEX, userInputMapper->getActionState(UserInputMapper::LEFT_HAND_CLICK));
    setPalmData(hand, rightHand, deltaTime, RIGHT_HAND_INDEX, userInputMapper->getActionState(UserInputMapper::RIGHT_HAND_CLICK));
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableHandMouseInput)) {
        emulateMouse(hand, userInputMapper->getActionState(UserInputMapper::LEFT_HAND_CLICK),
            userInputMapper->getActionState(UserInputMapper::SHIFT), LEFT_HAND_INDEX);
        emulateMouse(hand, userInputMapper->getActionState(UserInputMapper::RIGHT_HAND_CLICK),
            userInputMapper->getActionState(UserInputMapper::SHIFT), RIGHT_HAND_INDEX);
    }

    updateThreads(deltaTime); // If running non-threaded, then give the threads some time to process...
    updateDialogs(deltaTime); // update various stats dialogs if present

    _avatarUpdate->synchronousProcess();

    {
        PerformanceTimer perfTimer("physics");
        myAvatar->relayDriveKeysToCharacterController();

        static VectorOfMotionStates motionStates;
        _entitySimulation.getObjectsToDelete(motionStates);
        _physicsEngine->deleteObjects(motionStates);

        _entities.getTree()->withWriteLock([&] {
            _entitySimulation.getObjectsToAdd(motionStates);
            _physicsEngine->addObjects(motionStates);

        });
        _entities.getTree()->withWriteLock([&] {
            _entitySimulation.getObjectsToChange(motionStates);
            VectorOfMotionStates stillNeedChange = _physicsEngine->changeObjects(motionStates);
            _entitySimulation.setObjectsToChange(stillNeedChange);
        });

        _entitySimulation.applyActionChanges();

        AvatarManager* avatarManager = DependencyManager::get<AvatarManager>().data();
        avatarManager->getObjectsToDelete(motionStates);
        _physicsEngine->deleteObjects(motionStates);
        avatarManager->getObjectsToAdd(motionStates);
        _physicsEngine->addObjects(motionStates);
        avatarManager->getObjectsToChange(motionStates);
        _physicsEngine->changeObjects(motionStates);

        _entities.getTree()->withWriteLock([&] {
            _physicsEngine->stepSimulation();
        });
        
        if (_physicsEngine->hasOutgoingChanges()) {
            _entities.getTree()->withWriteLock([&] {
                _entitySimulation.handleOutgoingChanges(_physicsEngine->getOutgoingChanges(), _physicsEngine->getSessionID());
                avatarManager->handleOutgoingChanges(_physicsEngine->getOutgoingChanges());
            });

            auto collisionEvents = _physicsEngine->getCollisionEvents();
            avatarManager->handleCollisionEvents(collisionEvents);

            _physicsEngine->dumpStatsIfNecessary();

            if (!_aboutToQuit) {
                PerformanceTimer perfTimer("entities");
                // Collision events (and their scripts) must not be handled when we're locked, above. (That would risk
                // deadlock.)
                _entitySimulation.handleCollisionEvents(collisionEvents);
                // NOTE: the _entities.update() call below will wait for lock
                // and will simulate entity motion (the EntityTree has been given an EntitySimulation).
                _entities.update(); // update the models...
            }
        }
    }

    {
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

    // Update animation debug draw renderer
    {
        AnimDebugDraw::getInstance().update();
    }

    quint64 now = usecTimestampNow();

    // Update my voxel servers with my current voxel query...
    {
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

    // update sensorToWorldMatrix for rendering camera.
    myAvatar->updateSensorToWorldMatrix();
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
                packetsSent += nackPacketList->getNumPackets();
                
                // send the packet list
                nodeList->sendPacketList(std::move(nackPacketList), *node);
            }
        }
    });


    return packetsSent;
}

void Application::queryOctree(NodeType_t serverType, PacketType packetType, NodeToJurisdictionMap& jurisdictions) {

    //qCDebug(interfaceapp) << ">>> inside... queryOctree()... _viewFrustum.getFieldOfView()=" << _viewFrustum.getFieldOfView();
    bool wantExtraDebugging = getLogger()->extraDebugging();

    // These will be the same for all servers, so we can set them up once and then reuse for each server we send to.
    _octreeQuery.setWantLowResMoving(true);
    _octreeQuery.setWantColor(true);
    _octreeQuery.setWantDelta(true);
    _octreeQuery.setWantOcclusionCulling(false);
    _octreeQuery.setWantCompression(true);

    _octreeQuery.setCameraPosition(_viewFrustum.getPosition());
    _octreeQuery.setCameraOrientation(_viewFrustum.getOrientation());
    _octreeQuery.setCameraFov(_viewFrustum.getFieldOfView());
    _octreeQuery.setCameraAspectRatio(_viewFrustum.getAspectRatio());
    _octreeQuery.setCameraNearClip(_viewFrustum.getNearClip());
    _octreeQuery.setCameraFarClip(_viewFrustum.getFarClip());
    _octreeQuery.setCameraEyeOffsetPosition(glm::vec3());
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
                    ViewFrustum::location serverFrustumLocation = _viewFrustum.cubeInFrustum(serverBounds);

                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
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


                    ViewFrustum::location serverFrustumLocation = _viewFrustum.cubeInFrustum(serverBounds);
                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
                        inView = true;
                    } else {
                        inView = false;
                    }
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

float Application::getSizeScale() const {
    return DependencyManager::get<LODManager>()->getOctreeSizeScale();
}

int Application::getBoundaryLevelAdjust() const {
    return DependencyManager::get<LODManager>()->getBoundaryLevelAdjust();
}

PickRay Application::computePickRay(float x, float y) const {
    vec2 pickPoint{ x, y };
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

const glm::vec3& Application::getAvatarPosition() const {
    return getMyAvatar()->getPosition();
}

QImage Application::renderAvatarBillboard(RenderArgs* renderArgs) {

    const int BILLBOARD_SIZE = 64;

    // Need to make sure the gl context is current here
    _offscreenContext->makeCurrent();

    renderArgs->_renderMode = RenderArgs::DEFAULT_RENDER_MODE;
    renderRearViewMirror(renderArgs, QRect(0, 0, BILLBOARD_SIZE, BILLBOARD_SIZE), true);

    auto primaryFbo = DependencyManager::get<FramebufferCache>()->getPrimaryFramebufferDepthColor();
    QImage image(BILLBOARD_SIZE, BILLBOARD_SIZE, QImage::Format_ARGB32);
    renderArgs->_context->downloadFramebuffer(primaryFbo, glm::ivec4(0, 0, BILLBOARD_SIZE, BILLBOARD_SIZE), image);

    return image;
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

render::ItemID WorldBoxRenderData::_item = 0;

namespace render {
    template <> const ItemKey payloadGetKey(const WorldBoxRenderData::Pointer& stuff) { return ItemKey::Builder::opaqueShape(); }
    template <> const Item::Bound payloadGetBound(const WorldBoxRenderData::Pointer& stuff) { return Item::Bound(); }
    template <> void payloadRender(const WorldBoxRenderData::Pointer& stuff, RenderArgs* args) {
        if (args->_renderMode != RenderArgs::MIRROR_RENDER_MODE && Menu::getInstance()->isOptionChecked(MenuOption::WorldAxes)) {
            PerformanceTimer perfTimer("worldBox");

            auto& batch = *args->_batch;
            DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch);
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
    Environment* _environment;

    BackgroundRenderData(Environment* environment) : _environment(environment) {
    }

    static render::ItemID _item; // unique WorldBoxRenderData
};

render::ItemID BackgroundRenderData::_item = 0;

namespace render {
    template <> const ItemKey payloadGetKey(const BackgroundRenderData::Pointer& stuff) { return ItemKey::Builder::background(); }
    template <> const Item::Bound payloadGetBound(const BackgroundRenderData::Pointer& stuff) { return Item::Bound(); }
    template <> void payloadRender(const BackgroundRenderData::Pointer& background, RenderArgs* args) {

        Q_ASSERT(args->_batch);
        gpu::Batch& batch = *args->_batch;

        // Background rendering decision
        auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
        auto skybox = model::SkyboxPointer();
        if (skyStage->getBackgroundMode() == model::SunSkyStage::NO_BACKGROUND) {
        } else if (skyStage->getBackgroundMode() == model::SunSkyStage::SKY_DOME) {
           if (/*!selfAvatarOnly &&*/ Menu::getInstance()->isOptionChecked(MenuOption::Stars)) {
                PerformanceTimer perfTimer("stars");
                PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                    "Application::payloadRender<BackgroundRenderData>() ... stars...");
                // should be the first rendering pass - w/o depth buffer / lighting

                // compute starfield alpha based on distance from atmosphere
                float alpha = 1.0f;
                bool hasStars = true;

                if (Menu::getInstance()->isOptionChecked(MenuOption::Atmosphere)) {
                    // TODO: handle this correctly for zones
                    const EnvironmentData& closestData = background->_environment->getClosestData(args->_viewFrustum->getPosition()); // was theCamera instead of  _viewFrustum

                    if (closestData.getHasStars()) {
                        const float APPROXIMATE_DISTANCE_FROM_HORIZON = 0.1f;
                        const float DOUBLE_APPROXIMATE_DISTANCE_FROM_HORIZON = 0.2f;

                        glm::vec3 sunDirection = (args->_viewFrustum->getPosition()/*getAvatarPosition()*/ - closestData.getSunLocation())
                                                        / closestData.getAtmosphereOuterRadius();
                        float height = glm::distance(args->_viewFrustum->getPosition()/*theCamera.getPosition()*/, closestData.getAtmosphereCenter());
                        if (height < closestData.getAtmosphereInnerRadius()) {
                            // If we're inside the atmosphere, then determine if our keyLight is below the horizon
                            alpha = 0.0f;

                            if (sunDirection.y > -APPROXIMATE_DISTANCE_FROM_HORIZON) {
                                float directionY = glm::clamp(sunDirection.y,
                                                    -APPROXIMATE_DISTANCE_FROM_HORIZON, APPROXIMATE_DISTANCE_FROM_HORIZON)
                                                    + APPROXIMATE_DISTANCE_FROM_HORIZON;
                                alpha = (directionY / DOUBLE_APPROXIMATE_DISTANCE_FROM_HORIZON);
                            }


                        } else if (height < closestData.getAtmosphereOuterRadius()) {
                            alpha = (height - closestData.getAtmosphereInnerRadius()) /
                                (closestData.getAtmosphereOuterRadius() - closestData.getAtmosphereInnerRadius());

                            if (sunDirection.y > -APPROXIMATE_DISTANCE_FROM_HORIZON) {
                                float directionY = glm::clamp(sunDirection.y,
                                                    -APPROXIMATE_DISTANCE_FROM_HORIZON, APPROXIMATE_DISTANCE_FROM_HORIZON)
                                                    + APPROXIMATE_DISTANCE_FROM_HORIZON;
                                alpha = (directionY / DOUBLE_APPROXIMATE_DISTANCE_FROM_HORIZON);
                            }
                        }
                    } else {
                        hasStars = false;
                    }
                }

                // finally render the starfield
                if (hasStars) {
                    background->_stars.render(args, alpha);
                }

                // draw the sky dome
                if (/*!selfAvatarOnly &&*/ Menu::getInstance()->isOptionChecked(MenuOption::Atmosphere)) {
                    PerformanceTimer perfTimer("atmosphere");
                    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                        "Application::displaySide() ... atmosphere...");

                    background->_environment->renderAtmospheres(batch, *(args->_viewFrustum));
                }

            }
        } else if (skyStage->getBackgroundMode() == model::SunSkyStage::SKY_BOX) {
            PerformanceTimer perfTimer("skybox");

            skybox = skyStage->getSkybox();
            if (skybox) {
                skybox->render(batch, *(qApp->getDisplayViewFrustum()));
            }
        }
    }
}


void Application::displaySide(RenderArgs* renderArgs, Camera& theCamera, bool selfAvatarOnly, bool billboard) {

    // FIXME: This preRender call is temporary until we create a separate render::scene for the mirror rendering.
    // Then we can move this logic into the Avatar::simulate call.
    auto myAvatar = getMyAvatar();
    myAvatar->startRender();
    myAvatar->preRender(renderArgs);
    myAvatar->endRender();


    activeRenderingThread = QThread::currentThread();
    PROFILE_RANGE(__FUNCTION__);
    PerformanceTimer perfTimer("display");
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "Application::displaySide()");

    // load the view frustum
    loadViewFrustum(theCamera, _displayViewFrustum);

    // TODO fix shadows and make them use the GPU library

    // The pending changes collecting the changes here
    render::PendingChanges pendingChanges;

    // Background rendering decision
    if (BackgroundRenderData::_item == 0) {
        auto backgroundRenderData = make_shared<BackgroundRenderData>(&_environment);
        auto backgroundRenderPayload = make_shared<BackgroundRenderData::Payload>(backgroundRenderData);
        BackgroundRenderData::_item = _main3DScene->allocateID();
        pendingChanges.resetItem(WorldBoxRenderData::_item, backgroundRenderPayload);
    } else {

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
            if (Menu::getInstance()->isOptionChecked(MenuOption::PhysicsShowOwned)) {
                renderDebugFlags =
                    (RenderArgs::DebugFlags) (renderDebugFlags | (int)RenderArgs::RENDER_DEBUG_SIMULATION_OWNERSHIP);
            }
            renderArgs->_debugFlags = renderDebugFlags;
            //ViveControllerManager::getInstance().updateRendering(renderArgs, _main3DScene, pendingChanges);
        }
    }

    // Make sure the WorldBox is in the scene
    if (WorldBoxRenderData::_item == 0) {
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

    if (!billboard) {
        DependencyManager::get<DeferredLightingEffect>()->setAmbientLightMode(getRenderAmbientLight());
        auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
        DependencyManager::get<DeferredLightingEffect>()->setGlobalLight(skyStage->getSunLight()->getDirection(), skyStage->getSunLight()->getColor(), skyStage->getSunLight()->getIntensity(), skyStage->getSunLight()->getAmbientIntensity());
        DependencyManager::get<DeferredLightingEffect>()->setGlobalAtmosphere(skyStage->getAtmosphere());

        auto skybox = model::SkyboxPointer();
        if (skyStage->getBackgroundMode() == model::SunSkyStage::SKY_BOX) {
            skybox = skyStage->getSkybox();
        }
        DependencyManager::get<DeferredLightingEffect>()->setGlobalSkybox(skybox);
    }

    {
        PerformanceTimer perfTimer("SceneProcessPendingChanges");
        _main3DScene->enqueuePendingChanges(pendingChanges);

        _main3DScene->processPendingChangesQueue();
    }

    // For now every frame pass the renderContext
    {
        PerformanceTimer perfTimer("EngineRun");
        render::RenderContext renderContext;

        auto sceneInterface = DependencyManager::get<SceneScriptingInterface>();

        renderContext._cullOpaque = sceneInterface->doEngineCullOpaque();
        renderContext._sortOpaque = sceneInterface->doEngineSortOpaque();
        renderContext._renderOpaque = sceneInterface->doEngineRenderOpaque();
        renderContext._cullTransparent = sceneInterface->doEngineCullTransparent();
        renderContext._sortTransparent = sceneInterface->doEngineSortTransparent();
        renderContext._renderTransparent = sceneInterface->doEngineRenderTransparent();

        renderContext._maxDrawnOpaqueItems = sceneInterface->getEngineMaxDrawnOpaqueItems();
        renderContext._maxDrawnTransparentItems = sceneInterface->getEngineMaxDrawnTransparentItems();
        renderContext._maxDrawnOverlay3DItems = sceneInterface->getEngineMaxDrawnOverlay3DItems();

        renderContext._drawItemStatus = sceneInterface->doEngineDisplayItemStatus();
        renderContext._drawHitEffect = sceneInterface->doEngineDisplayHitEffect();

        renderContext._occlusionStatus = Menu::getInstance()->isOptionChecked(MenuOption::DebugAmbientOcclusion);
        renderContext._fxaaStatus = Menu::getInstance()->isOptionChecked(MenuOption::Antialiasing);

        renderArgs->_shouldRender = LODManager::shouldRender;

        renderContext.args = renderArgs;
        renderArgs->_viewFrustum = getDisplayViewFrustum();
        _renderEngine->setRenderContext(renderContext);

        // Before the deferred pass, let's try to use the render engine
        myAvatar->startRenderRun();
        _renderEngine->run();
        myAvatar->endRenderRun();

        auto engineRC = _renderEngine->getRenderContext();
        sceneInterface->setEngineFeedOpaqueItems(engineRC->_numFeedOpaqueItems);
        sceneInterface->setEngineDrawnOpaqueItems(engineRC->_numDrawnOpaqueItems);

        sceneInterface->setEngineFeedTransparentItems(engineRC->_numFeedTransparentItems);
        sceneInterface->setEngineDrawnTransparentItems(engineRC->_numDrawnTransparentItems);

        sceneInterface->setEngineFeedOverlay3DItems(engineRC->_numFeedOverlay3DItems);
        sceneInterface->setEngineDrawnOverlay3DItems(engineRC->_numDrawnOverlay3DItems);
    }

    activeRenderingThread = nullptr;
}

void Application::renderRearViewMirror(RenderArgs* renderArgs, const QRect& region, bool billboard) {
    auto originalViewport = renderArgs->_viewport;
    // Grab current viewport to reset it at the end

    float aspect = (float)region.width() / region.height();
    float fov = MIRROR_FIELD_OF_VIEW;

    auto myAvatar = getMyAvatar();
    
    // bool eyeRelativeCamera = false;
    if (billboard) {
        fov = BILLBOARD_FIELD_OF_VIEW;  // degees
        _mirrorCamera.setPosition(myAvatar->getPosition() +
                                  myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * BILLBOARD_DISTANCE * myAvatar->getScale());

    } else if (!AvatarInputs::getInstance()->mirrorZoomed()) {
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
    gpu::Vec4i viewport;
    if (billboard) {
        viewport = gpu::Vec4i(0, 0, region.width(), region.height());
    } else {
        // if not rendering the billboard, the region is in device independent coordinates; must convert to device
        float ratio = (float)QApplication::desktop()->windowHandle()->devicePixelRatio() * getRenderResolutionScale();
        int width = region.width() * ratio;
        int height = region.height() * ratio;
        viewport = gpu::Vec4i(0, 0, width, height);
    }
    renderArgs->_viewport = viewport;

    // render rear mirror view
    displaySide(renderArgs, _mirrorCamera, true, billboard);

    renderArgs->_viewport =  originalViewport;
}

void Application::resetSensors(bool andReload) {
    DependencyManager::get<Faceshift>()->reset();
    DependencyManager::get<DdeFaceTracker>()->reset();
    DependencyManager::get<EyeTracker>()->reset();

    getActiveDisplayPlugin()->resetSensors();

    QScreen* currentScreen = _window->windowHandle()->screen();
    QWindow* mainWindow = _window->windowHandle();
    QPoint windowCenter = mainWindow->geometry().center();
    _glWidget->cursor().setPos(currentScreen, windowCenter);

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

    // reset our node to stats and node to jurisdiction maps... since these must be changing...
    _entityServerJurisdictions.withWriteLock([&] {
        _entityServerJurisdictions.clear();
    });

    _octreeServerSceneStats.withWriteLock([&] {
        _octreeServerSceneStats.clear();
    });

    // reset the model renderer
    _entities.clear();
}

void Application::domainChanged(const QString& domainHostname) {
    updateWindowTitle();
    clearDomainOctreeDetails();
    _domainConnectionRefusals.clear();
}

void Application::handleDomainConnectionDeniedPacket(QSharedPointer<NLPacket> packet) {
    // Read deny reason from packet
    quint16 reasonSize;
    packet->readPrimitive(&reasonSize);
    QString reason = QString::fromUtf8(packet->getPayload() + packet->pos(), reasonSize);
    packet->seek(packet->pos() + reasonSize);

    // output to the log so the user knows they got a denied connection request
    // and check and signal for an access token so that we can make sure they are logged in
    qCDebug(interfaceapp) << "The domain-server denied a connection request: " << reason;
    qCDebug(interfaceapp) << "You may need to re-log to generate a keypair so you can provide a username signature.";

    if (!_domainConnectionRefusals.contains(reason)) {
        _domainConnectionRefusals.append(reason);
        emit domainConnectionRefused(reason);
    }

    AccountManager::getInstance().checkAndSignalForAccessToken();
}

void Application::connectedToDomain(const QString& hostname) {
    AccountManager& accountManager = AccountManager::getInstance();
    const QUuid& domainID = DependencyManager::get<NodeList>()->getDomainHandler().getUUID();

    if (accountManager.isLoggedIn() && !domainID.isNull()) {
        _notifiedPacketVersionMismatchThisDomain = false;
    }
}

void Application::nodeAdded(SharedNodePointer node) {
    if (node->getType() == NodeType::AvatarMixer) {
        // new avatar mixer, send off our identity packet right away
        getMyAvatar()->sendIdentityPacket();
    } else if (node->getType() == NodeType::AssetServer) {
        // the addition of an asset-server always re-enables the upload to asset server menu option
        Menu::getInstance()->getActionForOption(MenuOption::UploadAsset)->setEnabled(true);
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
    } else if (node->getType() == NodeType::AssetServer
               && !DependencyManager::get<NodeList>()->soloNodeOfType(NodeType::AssetServer)) {
        // this was our last asset server - disable the menu option to upload an asset
        Menu::getInstance()->getActionForOption(MenuOption::UploadAsset)->setEnabled(false);
    }
}
 
void Application::trackIncomingOctreePacket(NLPacket& packet, SharedNodePointer sendingNode, bool wasStatsPacket) {

    // Attempt to identify the sender from its address.
    if (sendingNode) {
        const QUuid& nodeUUID = sendingNode->getUUID();

        // now that we know the node ID, let's add these stats to the stats for that node...
        _octreeServerSceneStats.withWriteLock([&] {
            if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
                OctreeSceneStats& stats = _octreeServerSceneStats[nodeUUID];
                stats.trackIncomingOctreePacket(packet, wasStatsPacket, sendingNode->getClockSkewUsec());
            }
        });
    }
}

int Application::processOctreeStats(NLPacket& packet, SharedNodePointer sendingNode) {
    // But, also identify the sender, and keep track of the contained jurisdiction root for this server

    // parse the incoming stats datas stick it in a temporary object for now, while we
    // determine which server it belongs to
    int statsMessageLength = 0;

    const QUuid& nodeUUID = sendingNode->getUUID();
    
    // now that we know the node ID, let's add these stats to the stats for that node...
    _octreeServerSceneStats.withWriteLock([&] {
        OctreeSceneStats& octreeStats = _octreeServerSceneStats[nodeUUID];
        statsMessageLength = octreeStats.unpackFromPacket(packet);

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



    return statsMessageLength;
}

void Application::packetSent(quint64 length) {
}

const QString SETTINGS_KEY = "Settings";

void Application::loadScripts() {
    // loads all saved scripts
    Settings settings;
    int size = settings.beginReadArray(SETTINGS_KEY);
    for (int i = 0; i < size; ++i){
        settings.setArrayIndex(i);
        QString string = settings.value("script").toString();
        if (!string.isEmpty()) {
            loadScript(string);
        }
    }
    settings.endArray();
}

void Application::clearScriptsBeforeRunning() {
    // clears all scripts from the settingsSettings settings;
    Settings settings;
    settings.beginWriteArray(SETTINGS_KEY);
    settings.remove("");
}

void Application::saveScripts() {
    // Saves all currently running user-loaded scripts
    Settings settings;
    settings.beginWriteArray(SETTINGS_KEY);
    settings.remove("");

    QStringList runningScripts = getRunningScripts();
    int i = 0;
    for (auto it = runningScripts.begin(); it != runningScripts.end(); ++it) {
        if (getScriptEngine(*it)->isUserLoaded()) {
            settings.setArrayIndex(i);
            settings.setValue("script", *it);
            ++i;
        }
    }
    settings.endArray();
}

QScriptValue joystickToScriptValue(QScriptEngine *engine, Joystick* const &in) {
    return engine->newQObject(in);
}

void joystickFromScriptValue(const QScriptValue &object, Joystick* &out) {
    out = qobject_cast<Joystick*>(object.toQObject());
}

void Application::registerScriptEngineWithApplicationServices(ScriptEngine* scriptEngine) {
    // setup the packet senders and jurisdiction listeners of the script engine's scripting interfaces so
    // we can use the same ones from the application.
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    entityScriptingInterface->setPacketSender(&_entityEditSender);
    entityScriptingInterface->setEntityTree(_entities.getTree());

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
    connect(scriptEngine, SIGNAL(finished(const QString&)), clipboardScriptable, SLOT(deleteLater()));

    connect(scriptEngine, SIGNAL(finished(const QString&)), this, SLOT(scriptFinished(const QString&)));

    connect(scriptEngine, SIGNAL(loadScript(const QString&, bool)), this, SLOT(loadScript(const QString&, bool)));
    connect(scriptEngine, SIGNAL(reloadScript(const QString&, bool)), this, SLOT(reloadScript(const QString&, bool)));

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

    qScriptRegisterMetaType(scriptEngine, joystickToScriptValue, joystickFromScriptValue);

    scriptEngine->registerGlobalObject("UndoStack", &_undoStackScriptingInterface);

    scriptEngine->registerGlobalObject("LODManager", DependencyManager::get<LODManager>().data());

    scriptEngine->registerGlobalObject("Paths", DependencyManager::get<PathUtils>().data());

    scriptEngine->registerGlobalObject("HMD", DependencyManager::get<HMDScriptingInterface>().data());
    scriptEngine->registerFunction("HMD", "getHUDLookAtPosition2D", HMDScriptingInterface::getHUDLookAtPosition2D, 0);
    scriptEngine->registerFunction("HMD", "getHUDLookAtPosition3D", HMDScriptingInterface::getHUDLookAtPosition3D, 0);

    scriptEngine->registerGlobalObject("Scene", DependencyManager::get<SceneScriptingInterface>().data());

    scriptEngine->registerGlobalObject("ScriptDiscoveryService", this->getRunningScriptsWidget());

#ifdef HAVE_RTMIDI
    scriptEngine->registerGlobalObject("MIDI", &MIDIManager::getInstance());
#endif
}

bool Application::canAcceptURL(const QString& urlString) {
    QUrl url(urlString);
    if (urlString.startsWith(HIFI_URL_SCHEME)) {
        return true;
    }
    QHashIterator<QString, AcceptURLMethod> i(_acceptedExtensions);
    QString lowerPath = url.path().toLower();
    while (i.hasNext()) {
        i.next();
        if (lowerPath.endsWith(i.key())) {
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
        if (lowerPath.endsWith(i.key())) {
            AcceptURLMethod method = i.value();
            return (this->*method)(urlString);
        }
    }
    
    return defaultUpload && askToUploadAsset(urlString);
}

void Application::setSessionUUID(const QUuid& sessionUUID) {
    _physicsEngine->setSessionUUID(sessionUUID);
}

bool Application::askToSetAvatarUrl(const QString& url) {
    QUrl realUrl(url);
    if (realUrl.isLocalFile()) {
        QString message = "You can not use local files for avatar components.";

        QMessageBox msgBox;
        msgBox.setText(message);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return false;
    }

    // Download the FST file, to attempt to determine its model type
    QVariantHash fstMapping = FSTReader::downloadMapping(url);

    FSTReader::ModelType modelType = FSTReader::predictModelType(fstMapping);

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("Set Avatar");
    QPushButton* bodyAndHeadButton = NULL;

    QString modelName = fstMapping["name"].toString();
    QString message;
    QString typeInfo;
    switch (modelType) {

        case FSTReader::HEAD_AND_BODY_MODEL:
            message = QString("Would you like to use '") + modelName + QString("' for your avatar?");
            bodyAndHeadButton = msgBox.addButton(tr("Yes"), QMessageBox::ActionRole);
        break;

        default:
            message = QString(modelName + QString("Does not support a head and body as required."));
        break;
    }

    msgBox.setText(message);
    msgBox.addButton(QMessageBox::Cancel);

    msgBox.exec();

    if (msgBox.clickedButton() == bodyAndHeadButton) {
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
    reply = QMessageBox::question(getWindow(), "Run Script", message, QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        qCDebug(interfaceapp) << "Chose to run the script: " << scriptFilenameOrURL;
        loadScript(scriptFilenameOrURL);
    } else {
        qCDebug(interfaceapp) << "Declined to run the script: " << scriptFilenameOrURL;
    }
    return true;
}

bool Application::askToUploadAsset(const QString& filename) {
    if (!DependencyManager::get<NodeList>()->getThisNodeCanRez()) {
        QMessageBox::warning(_window, "Failed Upload",
                             QString("You don't have upload rights on that domain.\n\n"));
        return false;
    }
    
    QUrl url { filename };
    if (auto upload = DependencyManager::get<AssetClient>()->createUpload(url.toLocalFile())) {
        
        QMessageBox messageBox;
        messageBox.setWindowTitle("Asset upload");
        messageBox.setText("You are about to upload the following file to the asset server:\n" +
                           url.toDisplayString());
        messageBox.setInformativeText("Do you want to continue?");
        messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        messageBox.setDefaultButton(QMessageBox::Ok);
        
        // Option to drop model in world for models
        if (filename.endsWith(FBX_EXTENSION) || filename.endsWith(OBJ_EXTENSION)) {
            auto checkBox = new QCheckBox(&messageBox);
            checkBox->setText("Add to scene");
            messageBox.setCheckBox(checkBox);
        }
        
        if (messageBox.exec() != QMessageBox::Ok) {
            upload->deleteLater();
            return false;
        }
        
        // connect to the finished signal so we know when the AssetUpload is done
        if (messageBox.checkBox() && (messageBox.checkBox()->checkState() == Qt::Checked)) {
            // Custom behavior for models
            QObject::connect(upload, &AssetUpload::finished, this, &Application::modelUploadFinished);
        } else {
            QObject::connect(upload, &AssetUpload::finished,
                             &AssetUploadDialogFactory::getInstance(),
                             &AssetUploadDialogFactory::handleUploadFinished);
        }
        
        // start the upload now
        upload->start();
        return true;
    }
    
    // display a message box with the error
    QMessageBox::warning(_window, "Failed Upload", QString("Failed to upload %1.\n\n").arg(filename));
    return false;
}

void Application::modelUploadFinished(AssetUpload* upload, const QString& hash) {
    auto filename = QFileInfo(upload->getFilename()).fileName();
    
    if ((upload->getError() == AssetUpload::NoError) &&
        (filename.endsWith(FBX_EXTENSION) || filename.endsWith(OBJ_EXTENSION))) {
        
        auto entities = DependencyManager::get<EntityScriptingInterface>();
        
        EntityItemProperties properties;
        properties.setType(EntityTypes::Model);
        properties.setModelURL(QString("%1:%2.%3").arg(URL_SCHEME_ATP).arg(hash).arg(upload->getExtension()));
        properties.setPosition(_myCamera.getPosition() + _myCamera.getOrientation() * Vectors::FRONT * 2.0f);
        properties.setName(QUrl(upload->getFilename()).fileName());
        
        entities->addEntity(properties);
        
        upload->deleteLater();
    } else {
        AssetUploadDialogFactory::getInstance().handleUploadFinished(upload, hash);
    }
}

ScriptEngine* Application::loadScript(const QString& scriptFilename, bool isUserLoaded,
                                      bool loadScriptFromEditor, bool activateMainWindow, bool reload) {

    if (isAboutToQuit()) {
        return NULL;
    }

    QUrl scriptUrl(scriptFilename);
    const QString& scriptURLString = scriptUrl.toString();
    if (_scriptEnginesHash.contains(scriptURLString) && loadScriptFromEditor
        && !_scriptEnginesHash[scriptURLString]->isFinished()) {

        return _scriptEnginesHash[scriptURLString];
    }

    ScriptEngine* scriptEngine = new ScriptEngine(NO_SCRIPT, "", &_controllerScriptingInterface);
    scriptEngine->setUserLoaded(isUserLoaded);

    if (scriptFilename.isNull()) {
        // This appears to be the script engine used by the script widget's evaluation window before the file has been saved...

        // this had better be the script editor (we should de-couple so somebody who thinks they are loading a script
        // doesn't just get an empty script engine)

        // we can complete setup now since there isn't a script we have to load
        registerScriptEngineWithApplicationServices(scriptEngine);
        scriptEngine->runInThread();
    } else {
        // connect to the appropriate signals of this script engine
        connect(scriptEngine, &ScriptEngine::scriptLoaded, this, &Application::handleScriptEngineLoaded);
        connect(scriptEngine, &ScriptEngine::errorLoadingScript, this, &Application::handleScriptLoadError);

        // get the script engine object to load the script at the designated script URL
        scriptEngine->loadURL(scriptUrl, reload);
    }

    // restore the main window's active state
    if (activateMainWindow && !loadScriptFromEditor) {
        _window->activateWindow();
    }

    return scriptEngine;
}

void Application::reloadScript(const QString& scriptName, bool isUserLoaded) {
    loadScript(scriptName, isUserLoaded, false, false, true);
}

// FIXME - change to new version of ScriptCache loading notification
void Application::handleScriptEngineLoaded(const QString& scriptFilename) {
    ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(sender());

    _scriptEnginesHash.insertMulti(scriptFilename, scriptEngine);
    _runningScriptsWidget->setRunningScripts(getRunningScripts());
    UserActivityLogger::getInstance().loadedScript(scriptFilename);

    // register our application services and set it off on its own thread
    registerScriptEngineWithApplicationServices(scriptEngine);
    scriptEngine->runInThread();
}

// FIXME - change to new version of ScriptCache loading notification
void Application::handleScriptLoadError(const QString& scriptFilename) {
    qCDebug(interfaceapp) << "Application::loadScript(), script failed to load...";
    QMessageBox::warning(getWindow(), "Error Loading Script", scriptFilename + " failed to load.");
}

void Application::scriptFinished(const QString& scriptName) {
    const QString& scriptURLString = QUrl(scriptName).toString();
    QHash<QString, ScriptEngine*>::iterator it = _scriptEnginesHash.find(scriptURLString);
    if (it != _scriptEnginesHash.end()) {
        _scriptEnginesHash.erase(it);
        _runningScriptsWidget->scriptStopped(scriptName);
        _runningScriptsWidget->setRunningScripts(getRunningScripts());
    }
}

void Application::stopAllScripts(bool restart) {
    if (restart) {
        // Delete all running scripts from cache so that they are re-downloaded when they are restarted
        auto scriptCache = DependencyManager::get<ScriptCache>();
        for (QHash<QString, ScriptEngine*>::const_iterator it = _scriptEnginesHash.constBegin();
            it != _scriptEnginesHash.constEnd(); it++) {
            if (!it.value()->isFinished()) {
                scriptCache->deleteScript(it.key());
            }
        }
    }

    // Stop and possibly restart all currently running scripts
    for (QHash<QString, ScriptEngine*>::const_iterator it = _scriptEnginesHash.constBegin();
            it != _scriptEnginesHash.constEnd(); it++) {
        if (it.value()->isFinished()) {
            continue;
        }
        if (restart && it.value()->isUserLoaded()) {
            connect(it.value(), SIGNAL(finished(const QString&)), SLOT(reloadScript(const QString&)));
        }
        it.value()->stop();
        qCDebug(interfaceapp) << "stopping script..." << it.key();
    }
    // HACK: ATM scripts cannot set/get their animation priorities, so we clear priorities
    // whenever a script stops in case it happened to have been setting joint rotations.
    // TODO: expose animation priorities and provide a layered animation control system.
    getMyAvatar()->clearJointAnimationPriorities();
    getMyAvatar()->clearScriptableSettings();
}

bool Application::stopScript(const QString& scriptHash, bool restart) {
    bool stoppedScript = false;
    if (_scriptEnginesHash.contains(scriptHash)) {
        ScriptEngine* scriptEngine = _scriptEnginesHash[scriptHash];
        if (restart) {
            auto scriptCache = DependencyManager::get<ScriptCache>();
            scriptCache->deleteScript(QUrl(scriptHash));
            connect(scriptEngine, SIGNAL(finished(const QString&)), SLOT(reloadScript(const QString&)));
        }
        scriptEngine->stop();
        stoppedScript = true;
        qCDebug(interfaceapp) << "stopping script..." << scriptHash;
        // HACK: ATM scripts cannot set/get their animation priorities, so we clear priorities
        // whenever a script stops in case it happened to have been setting joint rotations.
        // TODO: expose animation priorities and provide a layered animation control system.
        getMyAvatar()->clearJointAnimationPriorities();
    }
    if (_scriptEnginesHash.empty()) {
        getMyAvatar()->clearScriptableSettings();
    }
    return stoppedScript;
}

void Application::reloadAllScripts() {
    DependencyManager::get<ScriptCache>()->clearCache();
    getEntities()->reloadEntityScripts();
    stopAllScripts(true);
}

void Application::reloadOneScript(const QString& scriptName) {
    stopScript(scriptName, true);
}

void Application::loadDefaultScripts() {
    if (!_scriptEnginesHash.contains(DEFAULT_SCRIPTS_JS_URL)) {
        loadScript(DEFAULT_SCRIPTS_JS_URL);
    }
}

void Application::toggleRunningScriptsWidget() {
    if (_runningScriptsWidget->isVisible()) {
        if (_runningScriptsWidget->hasFocus()) {
            _runningScriptsWidget->hide();
        } else {
            _runningScriptsWidget->raise();
            setActiveWindow(_runningScriptsWidget);
            _runningScriptsWidget->setFocus();
        }
    } else {
        _runningScriptsWidget->show();
        _runningScriptsWidget->setFocus();
    }
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

void Application::domainSettingsReceived(const QJsonObject& domainSettingsObject) {
    // from the domain-handler, figure out the satoshi cost per voxel and per meter cubed
    const QString VOXEL_SETTINGS_KEY = "voxels";
    const QString PER_VOXEL_COST_KEY = "per-voxel-credits";
    const QString PER_METER_CUBED_COST_KEY = "per-meter-cubed-credits";
    const QString VOXEL_WALLET_UUID = "voxel-wallet";

    const QJsonObject& voxelObject = domainSettingsObject[VOXEL_SETTINGS_KEY].toObject();

    qint64 satoshisPerVoxel = 0;
    qint64 satoshisPerMeterCubed = 0;
    QUuid voxelWalletUUID;

    if (!domainSettingsObject.isEmpty()) {
        float perVoxelCredits = (float) voxelObject[PER_VOXEL_COST_KEY].toDouble();
        float perMeterCubedCredits = (float) voxelObject[PER_METER_CUBED_COST_KEY].toDouble();

        satoshisPerVoxel = (qint64) floorf(perVoxelCredits * SATOSHIS_PER_CREDIT);
        satoshisPerMeterCubed = (qint64) floorf(perMeterCubedCredits * SATOSHIS_PER_CREDIT);

        voxelWalletUUID = QUuid(voxelObject[VOXEL_WALLET_UUID].toString());
    }

    qCDebug(interfaceapp) << "Octree edits costs are" << satoshisPerVoxel << "per octree cell and" << satoshisPerMeterCubed << "per meter cubed";
    qCDebug(interfaceapp) << "Destination wallet UUID for edit payments is" << voxelWalletUUID;
}

QString Application::getPreviousScriptLocation() {
    return _previousScriptLocation.get();
}

void Application::setPreviousScriptLocation(const QString& previousScriptLocation) {
    _previousScriptLocation.set(previousScriptLocation);
}

void Application::loadDialog() {

    QString fileNameString = QFileDialog::getOpenFileName(_glWidget,
                                                          tr("Open Script"),
                                                          getPreviousScriptLocation(),
                                                          tr("JavaScript Files (*.js)"));
    if (!fileNameString.isEmpty()) {
        setPreviousScriptLocation(fileNameString);
        loadScript(fileNameString);
    }
}

void Application::loadScriptURLDialog() {
    QInputDialog scriptURLDialog(getWindow());
    scriptURLDialog.setWindowTitle("Open and Run Script URL");
    scriptURLDialog.setLabelText("Script:");
    scriptURLDialog.setWindowFlags(Qt::Sheet);
    const float DIALOG_RATIO_OF_WINDOW = 0.30f;
    scriptURLDialog.resize(scriptURLDialog.parentWidget()->size().width() * DIALOG_RATIO_OF_WINDOW,
                        scriptURLDialog.size().height());

    int dialogReturn = scriptURLDialog.exec();
    QString newScript;
    if (dialogReturn == QDialog::Accepted) {
        if (scriptURLDialog.textValue().size() > 0) {
            // the user input a new hostname, use that
            newScript = scriptURLDialog.textValue();
        }
        loadScript(newScript);
    }
}

QString Application::getScriptsLocation() {
    return _scriptsLocationHandle.get();
}

void Application::setScriptsLocation(const QString& scriptsLocation) {
    _scriptsLocationHandle.set(scriptsLocation);
    emit scriptLocationChanged(scriptsLocation);
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

    QString fileName = Snapshot::saveSnapshot(_glWidget->grabFrameBuffer());

    AccountManager& accountManager = AccountManager::getInstance();
    if (!accountManager.isLoggedIn()) {
        return;
    }

    if (!_snapshotShareDialog) {
        _snapshotShareDialog = new SnapshotShareDialog(fileName, _glWidget);
    }
    _snapshotShareDialog->show();
    
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

int Application::getRenderAmbientLight() const {
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLightGlobal)) {
        return -1;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight0)) {
        return 0;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight1)) {
        return 1;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight2)) {
        return 2;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight3)) {
        return 3;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight4)) {
        return 4;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight5)) {
        return 5;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight6)) {
        return 6;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight7)) {
        return 7;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight8)) {
        return 8;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight9)) {
        return 9;
    } else {
        return -1;
    }
}

void Application::notifyPacketVersionMismatch() {
    if (!_notifiedPacketVersionMismatchThisDomain) {
        _notifiedPacketVersionMismatchThisDomain = true;

        QString message = "The location you are visiting is running an incompatible server version.\n";
        message += "Content may not display properly.";

        QMessageBox msgBox;
        msgBox.setText(message);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }
}

void Application::checkSkeleton() {
    if (getMyAvatar()->getSkeletonModel().isActive() && !getMyAvatar()->getSkeletonModel().hasSkeleton()) {
        qCDebug(interfaceapp) << "MyAvatar model has no skeleton";

        QString message = "Your selected avatar body has no skeleton.\n\nThe default body will be loaded...";
        QMessageBox msgBox;
        msgBox.setText(message);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();

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
void Application::showFriendsWindow() {
    const QString FRIENDS_WINDOW_OBJECT_NAME = "FriendsWindow";
    const QString FRIENDS_WINDOW_TITLE = "Add/Remove Friends";
    const QString FRIENDS_WINDOW_URL = "https://metaverse.highfidelity.com/user/friends";
    const int FRIENDS_WINDOW_WIDTH = 290;
    const int FRIENDS_WINDOW_HEIGHT = 500;
    auto webWindowClass = _window->findChildren<WebWindowClass>(FRIENDS_WINDOW_OBJECT_NAME);
    if (webWindowClass.empty()) {
        auto friendsWindow = new WebWindowClass(FRIENDS_WINDOW_TITLE, FRIENDS_WINDOW_URL, FRIENDS_WINDOW_WIDTH,
                                                FRIENDS_WINDOW_HEIGHT, false);
        friendsWindow->setParent(_window);
        friendsWindow->setObjectName(FRIENDS_WINDOW_OBJECT_NAME);
        connect(friendsWindow, &WebWindowClass::closed, &WebWindowClass::deleteLater);
        friendsWindow->setVisible(true);
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

glm::uvec2 Application::getUiSize() const {
    return getActiveDisplayPlugin()->getRecommendedUiSize();
}

QSize Application::getDeviceSize() const {
    return fromGlm(getActiveDisplayPlugin()->getRecommendedRenderSize());
}

PickRay Application::computePickRay() const {
    return computePickRay(getTrueMouse().x, getTrueMouse().y);
}

bool Application::isThrottleRendering() const {
    return getActiveDisplayPlugin()->isThrottled();
}

ivec2 Application::getTrueMouse() const {
    return toGlm(_glWidget->mapFromGlobal(QCursor::pos()));
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
    if (nullptr == _displayPlugin) {
        updateDisplayMode();
        Q_ASSERT(_displayPlugin);
    }
    return _displayPlugin.data();
}

const DisplayPlugin* Application::getActiveDisplayPlugin() const {
    return ((Application*)this)->getActiveDisplayPlugin();
}

bool _activatingDisplayPlugin{ false };
QVector<QPair<QString, QString>> _currentDisplayPluginActions;
QVector<QPair<QString, QString>> _currentInputPluginActions;

static void addDisplayPluginToMenu(DisplayPluginPointer displayPlugin, bool active = false) {
    auto menu = Menu::getInstance();
    QString name = displayPlugin->getName();
    Q_ASSERT(!menu->menuItemExists(MenuOption::OutputMenu, name));

    static QActionGroup* displayPluginGroup = nullptr;
    if (!displayPluginGroup) {
        displayPluginGroup = new QActionGroup(menu);
        displayPluginGroup->setExclusive(true);
    }
    auto parent = menu->getMenu(MenuOption::OutputMenu);
    auto action = menu->addActionToQMenuAndActionHash(parent,
        name, 0, qApp,
        SLOT(updateDisplayMode()));
    action->setCheckable(true);
    action->setChecked(active);
    displayPluginGroup->addAction(action);
    Q_ASSERT(menu->menuItemExists(MenuOption::OutputMenu, name));
}

void Application::updateDisplayMode() {
    auto menu = Menu::getInstance();
    auto displayPlugins = PluginManager::getInstance()->getDisplayPlugins();

    static std::once_flag once;
    std::call_once(once, [&] {
        bool first = true;
        foreach(auto displayPlugin, displayPlugins) {
            addDisplayPluginToMenu(displayPlugin, first);
            QObject::connect(displayPlugin.data(), &DisplayPlugin::requestRender, [this] {
                paintGL();
            });
            QObject::connect(displayPlugin.data(), &DisplayPlugin::recommendedFramebufferSizeChanged, [this](const QSize & size) {
                resizeGL();
            });

            first = false;
        }
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

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    DisplayPluginPointer oldDisplayPlugin = _displayPlugin;
    if (newDisplayPlugin == oldDisplayPlugin) {
        return;
    }

    // Some plugins *cough* Oculus *cough* process message events from inside their 
    // display function, and we don't want to change the display plugin underneath 
    // the paintGL call, so we need to guard against that
    if (_inPaint) {
        qDebug() << "Deferring plugin switch until out of painting";
        // Have the old plugin stop requesting renders
        oldDisplayPlugin->stop();
        QTimer* timer = new QTimer();
        timer->singleShot(500, [this, timer] {
            timer->deleteLater();
            updateDisplayMode();
        });
        return;
    }

    if (!_currentDisplayPluginActions.isEmpty()) {
        auto menu = Menu::getInstance();
        foreach(auto itemInfo, _currentDisplayPluginActions) {
            menu->removeMenuItem(itemInfo.first, itemInfo.second);
        }
        _currentDisplayPluginActions.clear();
    }

    if (newDisplayPlugin) {
        _offscreenContext->makeCurrent();
        _activatingDisplayPlugin = true;
        newDisplayPlugin->activate();
        _activatingDisplayPlugin = false;
        _offscreenContext->makeCurrent();
        offscreenUi->resize(fromGlm(newDisplayPlugin->getRecommendedUiSize()));
        _offscreenContext->makeCurrent();
    }

    oldDisplayPlugin = _displayPlugin;
    _displayPlugin = newDisplayPlugin;
        
    // If the displayPlugin is a screen based HMD, then it will want the HMDTools displayed
    // Direct Mode HMDs (like windows Oculus) will be isHmd() but will have a screen of -1
    bool newPluginWantsHMDTools = newDisplayPlugin ?
                                    (newDisplayPlugin->isHmd() && (newDisplayPlugin->getHmdScreen() >= 0)) : false;
    bool oldPluginWantedHMDTools = oldDisplayPlugin ? 
                                    (oldDisplayPlugin->isHmd() && (oldDisplayPlugin->getHmdScreen() >= 0)) : false;
                                        
    // Only show the hmd tools after the correct plugin has
    // been activated so that it's UI is setup correctly
    if (newPluginWantsHMDTools) {
        _pluginContainer->showDisplayPluginsTools();
    }

    if (oldDisplayPlugin) {
        oldDisplayPlugin->deactivate();
        _offscreenContext->makeCurrent();
            
        // if the old plugin was HMD and the new plugin is not HMD, then hide our hmdtools
        if (oldPluginWantedHMDTools && !newPluginWantsHMDTools) {
            DependencyManager::get<DialogsManager>()->hmdTools(false);
        }
    }
    emit activeDisplayPluginChanged();
    resetSensors();
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
        if (action->isChecked() && !_activeInputPlugins.contains(inputPlugin)) {
            _activeInputPlugins.append(inputPlugin);
            newInputPlugins.append(inputPlugin);
        } else if (!action->isChecked() && _activeInputPlugins.contains(inputPlugin)) {
            _activeInputPlugins.removeOne(inputPlugin);
            removedInputPlugins.append(inputPlugin);
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
        return getActiveDisplayPlugin()->getProjection((Eye)eye, _viewFrustum.getProjection());
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

void Application::setPalmData(Hand* hand, UserInputMapper::PoseValue pose, float deltaTime, int index, float triggerValue) {
    PalmData* palm;
    bool foundHand = false;
    for (size_t j = 0; j < hand->getNumPalms(); j++) {
        if (hand->getPalms()[j].getSixenseID() == index) {
            palm = &(hand->getPalms()[j]);
            foundHand = true;
            break;
        }
    }
    if (!foundHand) {
        PalmData newPalm(hand);
        hand->getPalms().push_back(newPalm);
        palm = &(hand->getPalms()[hand->getNumPalms() - 1]);
        palm->setSixenseID(index);
    }
    
    palm->setActive(pose.isValid());

    // transform from sensor space, to world space, to avatar model space.
    glm::mat4 poseMat = createMatFromQuatAndPos(pose.getRotation(), pose.getTranslation());
    glm::mat4 sensorToWorldMat = getMyAvatar()->getSensorToWorldMatrix();
    glm::mat4 modelMat = createMatFromQuatAndPos(getMyAvatar()->getOrientation(), getMyAvatar()->getPosition());
    glm::mat4 objectPose = glm::inverse(modelMat) * sensorToWorldMat * poseMat;

    glm::vec3 position = extractTranslation(objectPose);
    glm::quat rotation = glm::quat_cast(objectPose);

    //  Compute current velocity from position change
    glm::vec3 rawVelocity;
    if (deltaTime > 0.0f) {
        rawVelocity = (position - palm->getRawPosition()) / deltaTime;
    } else {
        rawVelocity = glm::vec3(0.0f);
    }
    palm->setRawVelocity(rawVelocity);   //  meters/sec
    
    //  Angular Velocity of Palm
    glm::quat deltaRotation = rotation * glm::inverse(palm->getRawRotation());
    glm::vec3 angularVelocity(0.0f);
    float rotationAngle = glm::angle(deltaRotation);
    if ((rotationAngle > EPSILON) && (deltaTime > 0.0f)) {
        angularVelocity = glm::normalize(glm::axis(deltaRotation));
        angularVelocity *= (rotationAngle / deltaTime);
        palm->setRawAngularVelocity(angularVelocity);
    } else {
        palm->setRawAngularVelocity(glm::vec3(0.0f));
    }

    if (InputDevice::getLowVelocityFilter()) {
        //  Use a velocity sensitive filter to damp small motions and preserve large ones with
        //  no latency.
        float velocityFilter = glm::clamp(1.0f - glm::length(rawVelocity), 0.0f, 1.0f);
        position = palm->getRawPosition() * velocityFilter + position * (1.0f - velocityFilter);
        rotation = safeMix(palm->getRawRotation(), rotation, 1.0f - velocityFilter);
    }
    palm->setRawPosition(position);
    palm->setRawRotation(rotation);

    // Store the one fingertip in the palm structure so we can track velocity
    const float FINGER_LENGTH = 0.3f;   //  meters
    const glm::vec3 FINGER_VECTOR(0.0f, FINGER_LENGTH, 0.0f);
    const glm::vec3 newTipPosition = position + rotation * FINGER_VECTOR;
    glm::vec3 oldTipPosition = palm->getTipRawPosition();
    if (deltaTime > 0.0f) {
        palm->setTipVelocity((newTipPosition - oldTipPosition) / deltaTime);
    } else {
        palm->setTipVelocity(glm::vec3(0.0f));
    }
    palm->setTipPosition(newTipPosition);
    palm->setTrigger(triggerValue);
}

void Application::emulateMouse(Hand* hand, float click, float shift, int index) {
    // Locate the palm, if it exists and is active
    PalmData* palm;
    bool foundHand = false;
    for (size_t j = 0; j < hand->getNumPalms(); j++) {
        if (hand->getPalms()[j].getSixenseID() == index) {
            palm = &(hand->getPalms()[j]);
            foundHand = true;
            break;
        }
    }
    if (!foundHand || !palm->isActive()) {
        return;
    }

    // Process the mouse events
    QPoint pos;

    unsigned int deviceID = index == 0 ? CONTROLLER_0_EVENT : CONTROLLER_1_EVENT;

    if (isHMDMode()) {
        pos = getApplicationCompositor().getPalmClickLocation(palm);
    }
    else {
        // Get directon relative to avatar orientation
        glm::vec3 direction = glm::inverse(getMyAvatar()->getOrientation()) * palm->getFingerDirection();

        // Get the angles, scaled between (-0.5,0.5)
        float xAngle = (atan2f(direction.z, direction.x) + (float)M_PI_2);
        float yAngle = 0.5f - ((atan2f(direction.z, direction.y) + (float)M_PI_2));
        auto canvasSize = getCanvasSize();
        // Get the pixel range over which the xAngle and yAngle are scaled
        float cursorRange = canvasSize.x * InputDevice::getCursorPixelRangeMult();

        pos.setX(canvasSize.x / 2.0f + cursorRange * xAngle);
        pos.setY(canvasSize.y / 2.0f + cursorRange * yAngle);

    }
    
    //If we are off screen then we should stop processing, and if a trigger or bumper is pressed,
    //we should unpress them.
    if (pos.x() == INT_MAX) {
        if (_oldHandLeftClick[index]) {
            QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, Qt::LeftButton, Qt::LeftButton, 0);

            mouseReleaseEvent(&mouseEvent, deviceID);

            _oldHandLeftClick[index] = false;
        }
        if (_oldHandRightClick[index]) {
            QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, Qt::RightButton, Qt::RightButton, 0);

            mouseReleaseEvent(&mouseEvent, deviceID);

            _oldHandRightClick[index] = false;
        }
        return;
    }

    //If position has changed, emit a mouse move to the application
    if (pos.x() != _oldHandMouseX[index] || pos.y() != _oldHandMouseY[index]) {
        QMouseEvent mouseEvent(QEvent::MouseMove, pos, Qt::NoButton, Qt::NoButton, 0);

        // Only send the mouse event if the opposite left button isnt held down.
        // Is this check necessary?
        if (!_oldHandLeftClick[(int)(!index)]) {
            mouseMoveEvent(&mouseEvent, deviceID);
        }
    }
    _oldHandMouseX[index] = pos.x();
    _oldHandMouseY[index] = pos.y();

    //We need separate coordinates for clicks, since we need to check if
    //a magnification window was clicked on
    int clickX = pos.x();
    int clickY = pos.y();
    //Set pos to the new click location, which may be the same if no magnification window is open
    pos.setX(clickX);
    pos.setY(clickY);

    // Right click
    if (shift == 1.0f && click == 1.0f) {
        if (!_oldHandRightClick[index]) {
            _oldHandRightClick[index] = true;

            QMouseEvent mouseEvent(QEvent::MouseButtonPress, pos, Qt::RightButton, Qt::RightButton, 0);

            mousePressEvent(&mouseEvent, deviceID);
        }
    } else if (_oldHandRightClick[index]) {
        QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, Qt::RightButton, Qt::RightButton, 0);

        mouseReleaseEvent(&mouseEvent, deviceID);

        _oldHandRightClick[index] = false;
    }

    // Left click
    if (shift != 1.0f && click == 1.0f) {
        if (!_oldHandLeftClick[index]) {
            _oldHandLeftClick[index] = true;

            QMouseEvent mouseEvent(QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, 0);

            mousePressEvent(&mouseEvent, deviceID);
        }
    } else if (_oldHandLeftClick[index]) {
        QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, Qt::LeftButton, Qt::LeftButton, 0);

        mouseReleaseEvent(&mouseEvent, deviceID);

        _oldHandLeftClick[index] = false;
    }
}

void Application::crashApplication() {
    qCDebug(interfaceapp) << "Intentionally crashed Interface";
    QObject* object = nullptr;
    bool value = object->isWindowType();
    Q_UNUSED(value);
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
