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

#include <sstream>

#include <stdlib.h>
#include <cmath>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QActionGroup>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QCheckBox>
#include <QImage>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QNetworkDiskCache>
#include <QOpenGLFramebufferObject>
#include <QObject>
#include <QWheelEvent>
#include <QScreen>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>
#include <QWindow>
#include <QtDebug>
#include <QFileDialog>
#include <QDesktopServices>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QMediaPlayer>
#include <QMimeData>
#include <QMessageBox>

#include <AddressManager.h>
#include <AccountManager.h>
#include <AmbientOcclusionEffect.h>
#include <AudioInjector.h>
#include <DeferredLightingEffect.h>
#include <DependencyManager.h>
#include <EntityScriptingInterface.h>
#include <GlowEffect.h>
#include <HFActionEvent.h>
#include <HFBackEvent.h>
#include <LogHandler.h>
#include <MainWindow.h>
#include <NetworkAccessManager.h>
#include <OctalCode.h>
#include <OctreeSceneStats.h>
#include <PacketHeaders.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <PhysicsEngine.h>
#include <ProgramObject.h>
#include <ResourceCache.h>
#include <Settings.h>
#include <SoundCache.h>
#include <TextRenderer.h>
#include <UserActivityLogger.h>
#include <UUID.h>

#include "Application.h"
#include "Audio.h"
#include "InterfaceVersion.h"
#include "LODManager.h"
#include "Menu.h"
#include "ModelUploader.h"
#include "Util.h"

#include "audio/AudioToolBox.h"
#include "audio/AudioIOStatsRenderer.h"
#include "audio/AudioScope.h"

#include "devices/DdeFaceTracker.h"
#include "devices/Faceshift.h"
#include "devices/Leapmotion.h"
#include "devices/RealSense.h"
#include "devices/MIDIManager.h"
#include "devices/OculusManager.h"
#include "devices/TV3DManager.h"
#include "devices/Visage.h"

#include "gpu/Batch.h"
#include "gpu/GLBackend.h"

#include "scripting/AccountScriptingInterface.h"
#include "scripting/AudioDeviceScriptingInterface.h"
#include "scripting/ClipboardScriptingInterface.h"
#include "scripting/JoystickScriptingInterface.h"
#include "scripting/GlobalServicesScriptingInterface.h"
#include "scripting/LocationScriptingInterface.h"
#include "scripting/MenuScriptingInterface.h"
#include "scripting/SettingsScriptingInterface.h"
#include "scripting/WindowScriptingInterface.h"
#include "scripting/WebWindowClass.h"

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif

#include "ui/DataWebDialog.h"
#include "ui/DialogsManager.h"
#include "ui/InfoView.h"
#include "ui/LoginDialog.h"
#include "ui/Snapshot.h"
#include "ui/StandAloneJSConsole.h"
#include "ui/Stats.h"



using namespace std;

//  Starfield information
static unsigned STARFIELD_NUM_STARS = 50000;
static unsigned STARFIELD_SEED = 1;

static const int BANDWIDTH_METER_CLICK_MAX_DRAG_LENGTH = 6; // farther dragged clicks are ignored


const qint64 MAXIMUM_CACHE_SIZE = 10737418240;  // 10GB

static QTimer* idleTimer = NULL;

const QString CHECK_VERSION_URL = "https://highfidelity.io/latestVersion.xml";
const QString SKIP_FILENAME = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/hifi.skipversion";

const QString DEFAULT_SCRIPTS_JS_URL = "http://s3.amazonaws.com/hifi-public/scripts/defaultScripts.js";

namespace SettingHandles {
    const SettingHandle<bool> firstRun("firstRun", true);
    const SettingHandle<QString> lastScriptLocation("LastScriptLocation");
    const SettingHandle<QString> scriptsLocation("scriptsLocation");
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType) type, context, message);
    
    if (!logMessage.isEmpty()) {
        Application::getInstance()->getLogger()->addMessage(qPrintable(logMessage + "\n"));
    }
}

bool setupEssentials(int& argc, char** argv) {
    unsigned int listenPort = 0; // bind to an ephemeral port by default
    const char** constArgv = const_cast<const char**>(argv);
    const char* portStr = getCmdOption(argc, constArgv, "--listenPort");
    if (portStr) {
        listenPort = atoi(portStr);
    }
    
    // read the ApplicationInfo.ini file for Name/Version/Domain information
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings applicationInfo(PathUtils::resourcesPath() + "info/ApplicationInfo.ini", QSettings::IniFormat);
    // set the associated application properties
    applicationInfo.beginGroup("INFO");
    QApplication::setApplicationName(applicationInfo.value("name").toString());
    QApplication::setApplicationVersion(BUILD_VERSION);
    QApplication::setOrganizationName(applicationInfo.value("organizationName").toString());
    QApplication::setOrganizationDomain(applicationInfo.value("organizationDomain").toString());
    
    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    
    // Set dependencies
    auto glCanvas = DependencyManager::set<GLCanvas>();
    auto addressManager = DependencyManager::set<AddressManager>();
    auto nodeList = DependencyManager::set<NodeList>(NodeType::Agent, listenPort);
    auto geometryCache = DependencyManager::set<GeometryCache>();
    auto glowEffect = DependencyManager::set<GlowEffect>();
    auto faceshift = DependencyManager::set<Faceshift>();
    auto audio = DependencyManager::set<Audio>();
    auto audioScope = DependencyManager::set<AudioScope>();
    auto audioIOStatsRenderer = DependencyManager::set<AudioIOStatsRenderer>();
    auto deferredLightingEffect = DependencyManager::set<DeferredLightingEffect>();
    auto ambientOcclusionEffect = DependencyManager::set<AmbientOcclusionEffect>();
    auto textureCache = DependencyManager::set<TextureCache>();
    auto animationCache = DependencyManager::set<AnimationCache>();
    auto visage = DependencyManager::set<Visage>();
    auto ddeFaceTracker = DependencyManager::set<DdeFaceTracker>();
    auto modelBlender = DependencyManager::set<ModelBlender>();
    auto audioToolBox = DependencyManager::set<AudioToolBox>();
    auto lodManager = DependencyManager::set<LODManager>();
    auto jsConsole = DependencyManager::set<StandAloneJSConsole>();
    auto dialogsManager = DependencyManager::set<DialogsManager>();
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    auto speechRecognizer = DependencyManager::set<SpeechRecognizer>();
#endif
    
    return true;
}


Application::Application(int& argc, char** argv, QElapsedTimer &startup_time) :
        QApplication(argc, argv),
        _dependencyManagerIsSetup(setupEssentials(argc, argv)),
        _window(new MainWindow(desktop())),
        _toolWindow(NULL),
        _nodeThread(new QThread(this)),
        _datagramProcessor(),
        _undoStack(),
        _undoStackScriptingInterface(&_undoStack),
        _frameCount(0),
        _fps(60.0f),
        _justStarted(true),
        _physicsEngine(glm::vec3(0.0f)),
        _entities(true, this, this),
        _entityClipboardRenderer(false, this, this),
        _entityClipboard(),
        _viewFrustum(),
        _lastQueriedViewFrustum(),
        _lastQueriedTime(usecTimestampNow()),
        _mirrorViewRect(QRect(MIRROR_VIEW_LEFT_PADDING, MIRROR_VIEW_TOP_PADDING, MIRROR_VIEW_WIDTH, MIRROR_VIEW_HEIGHT)),
        _viewTransform(),
        _scaleMirror(1.0f),
        _rotateMirror(0.0f),
        _raiseMirror(0.0f),
        _lastMouseMove(usecTimestampNow()),
        _lastMouseMoveWasSimulated(false),
        _touchAvgX(0.0f),
        _touchAvgY(0.0f),
        _isTouchPressed(false),
        _mousePressed(false),
        _enableProcessOctreeThread(true),
        _octreeProcessor(),
        _packetsPerSecond(0),
        _bytesPerSecond(0),
        _nodeBoundsDisplay(this),
        _previousScriptLocation(),
        _applicationOverlay(),
        _runningScriptsWidget(NULL),
        _runningScriptsWidgetWasVisible(false),
        _trayIcon(new QSystemTrayIcon(_window)),
        _lastNackTime(usecTimestampNow()),
        _lastSendDownstreamAudioStats(usecTimestampNow()),
        _isVSyncOn(true),
        _aboutToQuit(false)
{
    _logger = new FileLogger(this);  // After setting organization name in order to get correct directory
    qInstallMessageHandler(messageHandler);
    
    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "styles/Inconsolata.otf");
    _window->setWindowTitle("Interface");
    
    Model::setAbstractViewStateInterface(this); // The model class will sometimes need to know view state details from us
    
    auto glCanvas = DependencyManager::get<GLCanvas>();
    auto nodeList = DependencyManager::get<NodeList>();

    _myAvatar = _avatarManager.getMyAvatar();

    _applicationStartupTime = startup_time;

    qDebug() << "[VERSION] Build sequence: " << qPrintable(applicationVersion());

    _bookmarks = new Bookmarks();  // Before setting up the menu

    // call Menu getInstance static method to set up the menu
    _window->setMenuBar(Menu::getInstance());

    _runningScriptsWidget = new RunningScriptsWidget(_window);
    
    // start the nodeThread so its event loop is running
    _nodeThread->start();

    // make sure the node thread is given highest priority
    _nodeThread->setPriority(QThread::TimeCriticalPriority);
    
    // put the NodeList and datagram processing on the node thread
    nodeList->moveToThread(_nodeThread);
    _datagramProcessor.moveToThread(_nodeThread);

    // connect the DataProcessor processDatagrams slot to the QUDPSocket readyRead() signal
    connect(&nodeList->getNodeSocket(), SIGNAL(readyRead()), &_datagramProcessor, SLOT(processDatagrams()));

    // put the audio processing on a separate thread
    QThread* audioThread = new QThread(this);
    
    auto audioIO = DependencyManager::get<Audio>();
    audioIO->moveToThread(audioThread);
    connect(audioThread, &QThread::started, audioIO.data(), &Audio::start);
    connect(audioIO.data(), SIGNAL(muteToggled()), this, SLOT(audioMuteToggled()));

    audioThread->start();
    
    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    connect(&domainHandler, SIGNAL(hostnameChanged(const QString&)), SLOT(domainChanged(const QString&)));
    connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(connectedToDomain(const QString&)));
    connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(clearDomainOctreeDetails()));
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &Application::domainSettingsReceived);
    connect(&domainHandler, &DomainHandler::hostnameChanged,
            DependencyManager::get<AddressManager>().data(), &AddressManager::storeCurrentAddress);

    // update our location every 5 seconds in the data-server, assuming that we are authenticated with one
    const qint64 DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS = 5 * 1000;

    QTimer* locationUpdateTimer = new QTimer(this);
    connect(locationUpdateTimer, &QTimer::timeout, this, &Application::updateLocationInServer);
    locationUpdateTimer->start(DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS);

    connect(nodeList.data(), &NodeList::nodeAdded, this, &Application::nodeAdded);
    connect(nodeList.data(), &NodeList::nodeKilled, this, &Application::nodeKilled);
    connect(nodeList.data(), SIGNAL(nodeKilled(SharedNodePointer)), SLOT(nodeKilled(SharedNodePointer)));
    connect(nodeList.data(), &NodeList::uuidChanged, _myAvatar, &MyAvatar::setSessionUUID);
    connect(nodeList.data(), &NodeList::limitOfSilentDomainCheckInsReached, nodeList.data(), &NodeList::reset);

    // connect to appropriate slots on AccountManager
    AccountManager& accountManager = AccountManager::getInstance();

    const qint64 BALANCE_UPDATE_INTERVAL_MSECS = 5 * 1000;

    QTimer* balanceUpdateTimer = new QTimer(this);
    connect(balanceUpdateTimer, &QTimer::timeout, &accountManager, &AccountManager::updateBalance);
    balanceUpdateTimer->start(BALANCE_UPDATE_INTERVAL_MSECS);

    connect(&accountManager, &AccountManager::balanceChanged, this, &Application::updateWindowTitle);

    auto dialogsManager = DependencyManager::get<DialogsManager>();
    connect(&accountManager, &AccountManager::authRequired, dialogsManager.data(), &DialogsManager::showLoginDialog);
    connect(&accountManager, &AccountManager::usernameChanged, this, &Application::updateWindowTitle);
    
    // once we have a profile in account manager make sure we generate a new keypair
    connect(&accountManager, &AccountManager::profileChanged, &accountManager, &AccountManager::generateNewKeypair);

    // set the account manager's root URL and trigger a login request if we don't have the access token
    accountManager.setAuthURL(DEFAULT_NODE_AUTH_URL);
    UserActivityLogger::getInstance().launch(applicationVersion());

    // once the event loop has started, check and signal for an access token
    QMetaObject::invokeMethod(&accountManager, "checkAndSignalForAccessToken", Qt::QueuedConnection);
    
    auto addressManager = DependencyManager::get<AddressManager>();
    
    // use our MyAvatar position and quat for address manager path
    addressManager->setPositionGetter(getPositionForPath);
    addressManager->setOrientationGetter(getOrientationForPath);
    
    connect(addressManager.data(), &AddressManager::rootPlaceNameChanged, this, &Application::updateWindowTitle);

    #ifdef _WIN32
    WSADATA WsaData;
    int wsaresult = WSAStartup(MAKEWORD(2,2), &WsaData);
    #endif

    // tell the NodeList instance who to tell the domain server we care about
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
                                                 << NodeType::EntityServer
                                                 << NodeType::MetavoxelServer);

    // connect to the packet sent signal of the _entityEditSender
    connect(&_entityEditSender, &EntityEditPacketSender::packetSent, this, &Application::packetSent);

    // move the silentNodeTimer to the _nodeThread
    QTimer* silentNodeTimer = new QTimer();
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList.data(), SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_MSECS);
    silentNodeTimer->moveToThread(_nodeThread);

    // send the identity packet for our avatar each second to our avatar mixer
    QTimer* identityPacketTimer = new QTimer();
    connect(identityPacketTimer, &QTimer::timeout, _myAvatar, &MyAvatar::sendIdentityPacket);
    identityPacketTimer->start(AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS);

    // send the billboard packet for our avatar every few seconds
    QTimer* billboardPacketTimer = new QTimer();
    connect(billboardPacketTimer, &QTimer::timeout, _myAvatar, &MyAvatar::sendBillboardPacket);
    billboardPacketTimer->start(AVATAR_BILLBOARD_PACKET_SEND_INTERVAL_MSECS);

    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkDiskCache* cache = new QNetworkDiskCache();
    cache->setMaximumCacheSize(MAXIMUM_CACHE_SIZE);
    cache->setCacheDirectory(!cachePath.isEmpty() ? cachePath : "interfaceCache");
    networkAccessManager.setCache(cache);

    ResourceCache::setRequestLimit(3);

    _window->setCentralWidget(glCanvas.data());

    _window->restoreGeometry();

    _window->setVisible(true);
    glCanvas->setFocusPolicy(Qt::StrongFocus);
    glCanvas->setFocus();

    // enable mouse tracking; otherwise, we only get drag events
    glCanvas->setMouseTracking(true);

    _toolWindow = new ToolWindow();
    _toolWindow->setWindowFlags(_toolWindow->windowFlags() | Qt::WindowStaysOnTopHint);
    _toolWindow->setWindowTitle("Tools");

    // initialization continues in initializeGL when OpenGL context is ready

    // Tell our entity edit sender about our known jurisdictions
    _entityEditSender.setServerJurisdictions(&_entityServerJurisdictions);

    // For now we're going to set the PPS for outbound packets to be super high, this is
    // probably not the right long term solution. But for now, we're going to do this to
    // allow you to move an entity around in your hand
    _entityEditSender.setPacketsPerSecond(3000); // super high!!

    checkVersion();

    _overlays.init(glCanvas.data()); // do this before scripts load

    _runningScriptsWidget->setRunningScripts(getRunningScripts());
    connect(_runningScriptsWidget, &RunningScriptsWidget::stopScriptName, this, &Application::stopScript);

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(saveScripts()));
    connect(this, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

    // check first run...
    bool firstRun = SettingHandles::firstRun.get();
    if (firstRun) {
        qDebug() << "This is a first run...";
        // clear the scripts, and set out script to our default scripts
        clearScriptsBeforeRunning();
        loadScript(DEFAULT_SCRIPTS_JS_URL);

        SettingHandles::firstRun.set(false);
    } else {
        // do this as late as possible so that all required subsystems are initialized
        loadScripts();

        _previousScriptLocation = SettingHandles::lastScriptLocation.get();
    }
    
    loadSettings();
    int SAVE_SETTINGS_INTERVAL = 10 * MSECS_PER_SECOND; // Let's save every seconds for now
    connect(&_settingsTimer, &QTimer::timeout, this, &Application::saveSettings);
    connect(&_settingsThread, SIGNAL(started), &_settingsTimer, SLOT(start));
    connect(&_settingsThread, &QThread::finished, &_settingsTimer, &QTimer::deleteLater);
    _settingsTimer.moveToThread(&_settingsThread);
    _settingsTimer.setSingleShot(false);
    _settingsTimer.setInterval(SAVE_SETTINGS_INTERVAL);
    _settingsThread.start();
    
    _trayIcon->show();
    
    // set the local loopback interface for local sounds from audio scripts
    AudioScriptingInterface::getInstance().setLocalAudioInterface(audioIO.data());
    
#ifdef HAVE_RTMIDI
    // setup the MIDIManager
    MIDIManager& midiManagerInstance = MIDIManager::getInstance();
    midiManagerInstance.openDefaultPort();
#endif

    this->installEventFilter(this);
}

void Application::aboutToQuit() {
    _aboutToQuit = true;
    setFullscreen(false); // if you exit while in full screen, you'll get bad behavior when you restart.
}

Application::~Application() {
    saveSettings();
    
    _entities.getTree()->setSimulation(NULL);
    qInstallMessageHandler(NULL);
    
    _window->saveGeometry();
    
    int DELAY_TIME = 1000;
    UserActivityLogger::getInstance().close(DELAY_TIME);
    
    // make sure we don't call the idle timer any more
    delete idleTimer;
    
    // let the avatar mixer know we're out
    MyAvatar::sendKillAvatar();

    // ask the datagram processing thread to quit and wait until it is done
    _nodeThread->quit();
    _nodeThread->wait();
    
    // kill any audio injectors that are still around
    AudioScriptingInterface::getInstance().stopAllInjectors();
    
    auto audioIO = DependencyManager::get<Audio>();

    // stop the audio process
    QMetaObject::invokeMethod(audioIO.data(), "stop", Qt::BlockingQueuedConnection);
    
    // ask the audio thread to quit and wait until it is done
    audioIO->thread()->quit();
    audioIO->thread()->wait();
    
    _octreeProcessor.terminate();
    _entityEditSender.terminate();

    Menu::getInstance()->deleteLater();

    _myAvatar = NULL;
    
    DependencyManager::destroy<GLCanvas>();
}

void Application::initializeGL() {
    qDebug( "Created Display Window.");

    // initialize glut for shape drawing; Qt apparently initializes it on OS X
    #ifndef __APPLE__
    static bool isInitialized = false;
    if (isInitialized) {
        return;
    } else {
        isInitialized = true;
    }
    #endif

    #ifdef WIN32
    GLenum err = glewInit();
    if (GLEW_OK != err) {
      /* Problem: glewInit failed, something is seriously wrong. */
      qDebug("Error: %s\n", glewGetErrorString(err));
    }
    qDebug("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    if (wglewGetExtension("WGL_EXT_swap_control")) {
        int swapInterval = wglGetSwapIntervalEXT();
        qDebug("V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    }
    #endif

#if defined(Q_OS_LINUX)
    // TODO: Write the correct  code for Linux...
    /* if (wglewGetExtension("WGL_EXT_swap_control")) {
        int swapInterval = wglGetSwapIntervalEXT();
        qDebug("V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    }*/
#endif

    initDisplay();
    qDebug( "Initialized Display.");

    init();
    qDebug( "init() complete.");

    // create thread for parsing of octee data independent of the main network and rendering threads
    _octreeProcessor.initialize(_enableProcessOctreeThread);
    _entityEditSender.initialize(_enableProcessOctreeThread);

    // call our timer function every second
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(timer()));
    timer->start(1000);

    // call our idle function whenever we can
    idleTimer = new QTimer(this);
    connect(idleTimer, SIGNAL(timeout()), SLOT(idle()));
    idleTimer->start(0);
    _idleLoopStdev.reset();

    if (_justStarted) {
        float startupTime = (float)_applicationStartupTime.elapsed() / 1000.0;
        _justStarted = false;
        qDebug("Startup time: %4.2f seconds.", startupTime);
    }

    // update before the first render
    update(1.0f / _fps);

    InfoView::showFirstTime(INFO_HELP_PATH);
}

void Application::paintGL() {
    PROFILE_RANGE(__FUNCTION__);
    PerformanceTimer perfTimer("paintGL");

    PerformanceWarning::setSuppressShortTimings(Menu::getInstance()->isOptionChecked(MenuOption::SuppressShortTimings));
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::paintGL()");

    // Set the desired FBO texture size. If it hasn't changed, this does nothing.
    // Otherwise, it must rebuild the FBOs
    if (OculusManager::isConnected()) {
        DependencyManager::get<TextureCache>()->setFrameBufferSize(OculusManager::getRenderTargetSize());
    } else {
        QSize fbSize = DependencyManager::get<GLCanvas>()->getDeviceSize() * getRenderResolutionScale();
        DependencyManager::get<TextureCache>()->setFrameBufferSize(fbSize);
    }

    glEnable(GL_LINE_SMOOTH);

    if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON) {
        if (!OculusManager::isConnected()) {
            //  If there isn't an HMD, match exactly to avatar's head
            _myCamera.setPosition(_myAvatar->getHead()->getEyePosition());
            _myCamera.setRotation(_myAvatar->getHead()->getCameraOrientation());
        } else {
            //  For an HMD, set the base position and orientation to that of the avatar body
            _myCamera.setPosition(_myAvatar->getDefaultEyePosition());
            _myCamera.setRotation(_myAvatar->getWorldAlignedOrientation());
        }

    } else if (_myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
        static const float THIRD_PERSON_CAMERA_DISTANCE = 1.5f;
        _myCamera.setPosition(_myAvatar->getDefaultEyePosition() +
            _myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, 1.0f) * THIRD_PERSON_CAMERA_DISTANCE * _myAvatar->getScale());
        if (OculusManager::isConnected()) {
            _myCamera.setRotation(_myAvatar->getWorldAlignedOrientation());
        } else {
            _myCamera.setRotation(_myAvatar->getHead()->getOrientation());
        }

    } else if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        _myCamera.setRotation(_myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f)));
        _myCamera.setPosition(_myAvatar->getDefaultEyePosition() +
                              glm::vec3(0, _raiseMirror * _myAvatar->getScale(), 0) +
                              (_myAvatar->getOrientation() * glm::quat(glm::vec3(0.0f, _rotateMirror, 0.0f))) *
                               glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_FULLSCREEN_DISTANCE * _scaleMirror);
    }

    // Update camera position
    if (!OculusManager::isConnected()) {
        _myCamera.update(1.0f / _fps);
    }

    if (getShadowsEnabled()) {
        updateShadowMap();
    }

    if (OculusManager::isConnected()) {
        //Clear the color buffer to ensure that there isnt any residual color
        //Left over from when OR was not connected.
        glClear(GL_COLOR_BUFFER_BIT);
        
        //When in mirror mode, use camera rotation. Otherwise, use body rotation
        if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
            OculusManager::display(_myCamera.getRotation(), _myCamera.getPosition(), _myCamera);
        } else {
            OculusManager::display(_myAvatar->getWorldAlignedOrientation(), _myAvatar->getDefaultEyePosition(), _myCamera);
        }
        _myCamera.update(1.0f / _fps);

    } else if (TV3DManager::isConnected()) {
       
        TV3DManager::display(_myCamera);

    } else {
        DependencyManager::get<GlowEffect>()->prepare();

        // Viewport is assigned to the size of the framebuffer
        QSize size = DependencyManager::get<TextureCache>()->getPrimaryFramebufferObject()->size();
        glViewport(0, 0, size.width(), size.height());

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        displaySide(_myCamera);
        glPopMatrix();

        if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
            _rearMirrorTools->render(true);
        
        } else if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
            renderRearViewMirror(_mirrorViewRect);       
        }

        DependencyManager::get<GlowEffect>()->render();

        {
            PerformanceTimer perfTimer("renderOverlay");
            // PrioVR will only work if renderOverlay is called, calibration is connected to Application::renderingOverlay() 
            _applicationOverlay.renderOverlay(true);
            if (Menu::getInstance()->isOptionChecked(MenuOption::UserInterface)) {
                _applicationOverlay.displayOverlayTexture();
            }
        }
    }

    _frameCount++;
}

void Application::runTests() {
    runTimingTests();
}

void Application::audioMuteToggled() {
    QAction* muteAction = Menu::getInstance()->getActionForOption(MenuOption::MuteAudio);
    Q_CHECK_PTR(muteAction);
    muteAction->setChecked(DependencyManager::get<Audio>()->isMuted());
}

void Application::aboutApp() {
    InfoView::forcedShow(INFO_HELP_PATH);
}

void Application::showEditEntitiesHelp() {
    InfoView::forcedShow(INFO_EDIT_ENTITIES_PATH);
}

void Application::resetCamerasOnResizeGL(Camera& camera, int width, int height) {
    if (OculusManager::isConnected()) {
        OculusManager::configureCamera(camera, width, height);
    } else if (TV3DManager::isConnected()) {
        TV3DManager::configureCamera(camera, width, height);
    } else {
        camera.setAspectRatio((float)width / height);
        camera.setFieldOfView(_viewFrustum.getFieldOfView());
    }
}

void Application::resizeGL(int width, int height) {
    resetCamerasOnResizeGL(_myCamera, width, height);

    glViewport(0, 0, width, height); // shouldn't this account for the menu???

    updateProjectionMatrix();
    glLoadIdentity();

    // update Stats width
    // let's set horizontal offset to give stats some margin to mirror
    int horizontalOffset = MIRROR_VIEW_WIDTH + MIRROR_VIEW_LEFT_PADDING * 2;
    Stats::getInstance()->resetWidth(width, horizontalOffset);
}

void Application::updateProjectionMatrix() {
    updateProjectionMatrix(_myCamera);
}

void Application::updateProjectionMatrix(Camera& camera, bool updateViewFrustum) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;

    // Tell our viewFrustum about this change, using the application camera
    if (updateViewFrustum) {
        loadViewFrustum(camera, _viewFrustum);
        _viewFrustum.computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    } else {
        ViewFrustum tempViewFrustum;
        loadViewFrustum(camera, tempViewFrustum);
        tempViewFrustum.computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    }
    glFrustum(left, right, bottom, top, nearVal, farVal);

    // save matrix
    glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat*)&_projectionMatrix);

    glMatrixMode(GL_MODELVIEW);
}

void Application::controlledBroadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes) {
    foreach(NodeType_t type, destinationNodeTypes) {

        // Perform the broadcast for one type
        int nReceivingNodes = DependencyManager::get<NodeList>()->broadcastToNodes(packet, NodeSet() << type);

        // Feed number of bytes to corresponding channel of the bandwidth meter, if any (done otherwise)
        BandwidthMeter::ChannelIndex channel;
        switch (type) {
            case NodeType::Agent:
            case NodeType::AvatarMixer:
                channel = BandwidthMeter::AVATARS;
                break;
            case NodeType::EntityServer:
                channel = BandwidthMeter::OCTREE;
                break;
            default:
                continue;
        }
        _bandwidthMeter.outputStream(channel).updateValue(nReceivingNodes * packet.size());
    }
}

bool Application::event(QEvent* event) {

    // handle custom URL
    if (event->type() == QEvent::FileOpen) {
        
        QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event);
        
        if (!fileEvent->url().isEmpty()) {
            DependencyManager::get<AddressManager>()->handleLookupString(fileEvent->url().toString());
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
        // Filter out captured keys before they're used for shortcut actions.
        if (_controllerScriptingInterface.isKeyCaptured(static_cast<QKeyEvent*>(event))) {
            event->accept();
            return true;
        }
    }

    return false;
}

void Application::keyPressEvent(QKeyEvent* event) {

    _keysPressed.insert(event->key());

    _controllerScriptingInterface.emitKeyPressEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isKeyCaptured(event)) {
        return;
    }

    if (activeWindow() == _window) {
        bool isShifted = event->modifiers().testFlag(Qt::ShiftModifier);
        bool isMeta = event->modifiers().testFlag(Qt::ControlModifier);
        bool isOption = event->modifiers().testFlag(Qt::AltModifier);
        switch (event->key()) {
                break;
            case Qt::Key_L:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::LodTools);
                } else if (isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::Log);
                }
                break;

            case Qt::Key_E:
            case Qt::Key_PageUp:
               if (!_myAvatar->getDriveKeys(UP)) {
                    _myAvatar->jump();
                }
                _myAvatar->setDriveKeys(UP, 1.0f);
                break;

            case Qt::Key_Asterisk:
                Menu::getInstance()->triggerOption(MenuOption::Stars);
                break;

            case Qt::Key_C:
            case Qt::Key_PageDown:
                _myAvatar->setDriveKeys(DOWN, 1.0f);
                break;

            case Qt::Key_W:
                if (isOption && !isShifted && !isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::Wireframe);
                } else {
                    _myAvatar->setDriveKeys(FWD, 1.0f);
                }
                break;

            case Qt::Key_S:
                if (isShifted && isMeta && !isOption) {
                    Menu::getInstance()->triggerOption(MenuOption::SuppressShortTimings);
                } else if (isOption && !isShifted && !isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::ScriptEditor);
                } else if (!isOption && !isShifted && isMeta) {
                    takeSnapshot();
                } else {
                    _myAvatar->setDriveKeys(BACK, 1.0f);
                }
                break;

            case Qt::Key_Apostrophe:
                resetSensors();
                break;

            case Qt::Key_G:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::ObeyEnvironmentalGravity);
                }
                break;

            case Qt::Key_A:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Atmosphere);
                } else {
                    _myAvatar->setDriveKeys(ROT_LEFT, 1.0f);
                }
                break;

            case Qt::Key_D:
                if (!isMeta) {
                    _myAvatar->setDriveKeys(ROT_RIGHT, 1.0f);
                }
                break;

            case Qt::Key_Return:
            case Qt::Key_Enter:
                Menu::getInstance()->triggerOption(MenuOption::AddressBar);
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
                } else {
                    _myAvatar->setDriveKeys(isShifted ? UP : FWD, 1.0f);
                }
                break;

            case Qt::Key_Down:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    if (!isShifted) {
                        _scaleMirror *= 1.05f;
                    } else {
                        _raiseMirror -= 0.05f;
                    }
                } else {
                    _myAvatar->setDriveKeys(isShifted ? DOWN : BACK, 1.0f);
                }
                break;

            case Qt::Key_Left:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    _rotateMirror += PI / 20.0f;
                } else {
                    _myAvatar->setDriveKeys(isShifted ? LEFT : ROT_LEFT, 1.0f);
                }
                break;

            case Qt::Key_Right:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    _rotateMirror -= PI / 20.0f;
                } else {
                    _myAvatar->setDriveKeys(isShifted ? RIGHT : ROT_RIGHT, 1.0f);
                }
                break;

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
                    if (TV3DManager::isConnected()) {
                        auto glCanvas = DependencyManager::get<GLCanvas>();
                        TV3DManager::configureCamera(_myCamera, glCanvas->getDeviceWidth(), glCanvas->getDeviceHeight());
                    }
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(-0.001, 0, 0));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_M:
                if (isShifted) {
                    _viewFrustum.setFocalLength(_viewFrustum.getFocalLength() + 0.1f);
                    if (TV3DManager::isConnected()) {
                        auto glCanvas = DependencyManager::get<GLCanvas>();
                        TV3DManager::configureCamera(_myCamera, glCanvas->getDeviceWidth(), glCanvas->getDeviceHeight());
                    }

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
            case Qt::Key_H:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Mirror);
                } else {
                    Menu::getInstance()->triggerOption(MenuOption::FullscreenMirror);
                }
                break;
            case Qt::Key_Slash:
                Menu::getInstance()->triggerOption(MenuOption::UserInterface);
                break;
            case Qt::Key_P:
                 Menu::getInstance()->triggerOption(MenuOption::FirstPerson);
                 break;
            case Qt::Key_Percent:
                Menu::getInstance()->triggerOption(MenuOption::Stats);
                break;
            case Qt::Key_Plus:
                _myAvatar->increaseSize();
                break;
            case Qt::Key_Minus:
                _myAvatar->decreaseSize();
                break;
            case Qt::Key_Equal:
                _myAvatar->resetSize();
                break;
            case Qt::Key_Space: {
                if (!event->isAutoRepeat()) {
                    // this starts an HFActionEvent
                    HFActionEvent startActionEvent(HFActionEvent::startType(),
                                                   _myCamera.computePickRay(getTrueMouseX(),
                                                                            getTrueMouseY()));
                    sendEvent(this, &startActionEvent);
                }
                
                break;
            }
            case Qt::Key_Escape: {
                OculusManager::abandonCalibration();
                
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

    _keysPressed.remove(event->key());

    _controllerScriptingInterface.emitKeyReleaseEvent(event); // send events to any registered scripts
    
    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isKeyCaptured(event)) {
        return;
    }

    switch (event->key()) {
        case Qt::Key_E:
        case Qt::Key_PageUp:
            _myAvatar->setDriveKeys(UP, 0.0f);
            break;

        case Qt::Key_C:
        case Qt::Key_PageDown:
            _myAvatar->setDriveKeys(DOWN, 0.0f);
            break;

        case Qt::Key_W:
            _myAvatar->setDriveKeys(FWD, 0.0f);
            break;

        case Qt::Key_S:
            _myAvatar->setDriveKeys(BACK, 0.0f);
            break;

        case Qt::Key_A:
            _myAvatar->setDriveKeys(ROT_LEFT, 0.0f);
            break;

        case Qt::Key_D:
            _myAvatar->setDriveKeys(ROT_RIGHT, 0.0f);
            break;

        case Qt::Key_Up:
            _myAvatar->setDriveKeys(FWD, 0.0f);
            _myAvatar->setDriveKeys(UP, 0.0f);
            break;

        case Qt::Key_Down:
            _myAvatar->setDriveKeys(BACK, 0.0f);
            _myAvatar->setDriveKeys(DOWN, 0.0f);
            break;

        case Qt::Key_Left:
            _myAvatar->setDriveKeys(LEFT, 0.0f);
            _myAvatar->setDriveKeys(ROT_LEFT, 0.0f);
            break;

        case Qt::Key_Right:
            _myAvatar->setDriveKeys(RIGHT, 0.0f);
            _myAvatar->setDriveKeys(ROT_RIGHT, 0.0f);
            break;
        case Qt::Key_Control:
        case Qt::Key_Shift:
        case Qt::Key_Meta:
        case Qt::Key_Alt:
            _myAvatar->clearDriveKeys();
            break;
        case Qt::Key_Space: {
            if (!event->isAutoRepeat()) {
                // this ends the HFActionEvent
                HFActionEvent endActionEvent(HFActionEvent::endType(),
                                             _myCamera.computePickRay(getTrueMouseX(),
                                                                      getTrueMouseY()));
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
    // synthesize events for keys currently pressed, since we may not get their release events
    foreach (int key, _keysPressed) {
        QKeyEvent event(QEvent::KeyRelease, key, Qt::NoModifier);
        keyReleaseEvent(&event);
    }
    _keysPressed.clear();
}

void Application::mouseMoveEvent(QMouseEvent* event, unsigned int deviceID) {
    // Used by application overlay to determine how to draw cursor(s)
    _lastMouseMoveWasSimulated = deviceID > 0;
    if (!_lastMouseMoveWasSimulated) {
        _lastMouseMove = usecTimestampNow();
    }
    
    if (_aboutToQuit) {
        return;
    }
    
    _entities.mouseMoveEvent(event, deviceID);
    
    _controllerScriptingInterface.emitMouseMoveEvent(event, deviceID); // send events to any registered scripts
    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }
    
}

void Application::mousePressEvent(QMouseEvent* event, unsigned int deviceID) {
    if (!_aboutToQuit) {
        _entities.mousePressEvent(event, deviceID);
    }

    _controllerScriptingInterface.emitMousePressEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }


    if (activeWindow() == _window) {
        if (event->button() == Qt::LeftButton) {
            _mouseDragStartedX = getTrueMouseX();
            _mouseDragStartedY = getTrueMouseY();
            _mousePressed = true;
            
            if (mouseOnScreen()) {
                if (DependencyManager::get<AudioToolBox>()->mousePressEvent(getMouseX(), getMouseY())) {
                    // stop propagation
                    return;
                }
                
                if (_rearMirrorTools->mousePressEvent(getMouseX(), getMouseY())) {
                    // stop propagation
                    return;
                }
            }
            
            // nobody handled this - make it an action event on the _window object
            HFActionEvent actionEvent(HFActionEvent::startType(),
                                      _myCamera.computePickRay(event->x(), event->y()));
            sendEvent(this, &actionEvent);

        } else if (event->button() == Qt::RightButton) {
            // right click items here
        }
    }
}

void Application::mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID) {

    if (!_aboutToQuit) {
        _entities.mouseReleaseEvent(event, deviceID);
    }

    _controllerScriptingInterface.emitMouseReleaseEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }

    if (activeWindow() == _window) {
        if (event->button() == Qt::LeftButton) {
            _mousePressed = false;
            
            if (Menu::getInstance()->isOptionChecked(MenuOption::Stats) && mouseOnScreen()) {
                // let's set horizontal offset to give stats some margin to mirror
                int horizontalOffset = MIRROR_VIEW_WIDTH;
                Stats::getInstance()->checkClick(getMouseX(), getMouseY(),
                                                 getMouseDragStartedX(), getMouseDragStartedY(), horizontalOffset);
                checkBandwidthMeterClick();
            }
            
            // fire an action end event
            HFActionEvent actionEvent(HFActionEvent::endType(),
                                      _myCamera.computePickRay(event->x(), event->y()));
            sendEvent(this, &actionEvent);
        }
    }
}

void Application::touchUpdateEvent(QTouchEvent* event) {
    TouchEvent thisEvent(*event, _lastTouchEvent);
    _controllerScriptingInterface.emitTouchUpdateEvent(thisEvent); // send events to any registered scripts
    _lastTouchEvent = thisEvent;

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isTouchCaptured()) {
        return;
    }

    bool validTouch = false;
    if (activeWindow() == _window) {
        const QList<QTouchEvent::TouchPoint>& tPoints = event->touchPoints();
        _touchAvgX = 0.0f;
        _touchAvgY = 0.0f;
        int numTouches = tPoints.count();
        if (numTouches > 1) {
            for (int i = 0; i < numTouches; ++i) {
                _touchAvgX += tPoints[i].pos().x();
                _touchAvgY += tPoints[i].pos().y();
            }
            _touchAvgX /= (float)(numTouches);
            _touchAvgY /= (float)(numTouches);
            validTouch = true;
        }
    }
    if (!_isTouchPressed) {
        _touchDragStartedAvgX = _touchAvgX;
        _touchDragStartedAvgY = _touchAvgY;
    }
    _isTouchPressed = validTouch;
}

void Application::touchBeginEvent(QTouchEvent* event) {
    TouchEvent thisEvent(*event); // on touch begin, we don't compare to last event
    _controllerScriptingInterface.emitTouchBeginEvent(thisEvent); // send events to any registered scripts

    _lastTouchEvent = thisEvent; // and we reset our last event to this event before we call our update
    touchUpdateEvent(event);

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isTouchCaptured()) {
        return;
    }

}

void Application::touchEndEvent(QTouchEvent* event) {
    TouchEvent thisEvent(*event, _lastTouchEvent);
    _controllerScriptingInterface.emitTouchEndEvent(thisEvent); // send events to any registered scripts
    _lastTouchEvent = thisEvent;

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isTouchCaptured()) {
        return;
    }
    // put any application specific touch behavior below here..
    _touchDragStartedAvgX = _touchAvgX;
    _touchDragStartedAvgY = _touchAvgY;
    _isTouchPressed = false;

}

void Application::wheelEvent(QWheelEvent* event) {

    _controllerScriptingInterface.emitWheelEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isWheelCaptured()) {
        return;
    }
}

void Application::dropEvent(QDropEvent *event) {
    QString snapshotPath;
    const QMimeData *mimeData = event->mimeData();
    foreach (QUrl url, mimeData->urls()) {
        if (url.url().toLower().endsWith(SNAPSHOT_EXTENSION)) {
            snapshotPath = url.toLocalFile();
            break;
        }
    }

    SnapshotMetaData* snapshotData = Snapshot::parseSnapshotData(snapshotPath);
    if (snapshotData) {
        if (!snapshotData->getDomain().isEmpty()) {
            DependencyManager::get<NodeList>()->getDomainHandler().setHostnameAndPort(snapshotData->getDomain());
        }

        _myAvatar->setPosition(snapshotData->getLocation());
        _myAvatar->setOrientation(snapshotData->getOrientation());
    } else {
        QMessageBox msgBox;
        msgBox.setText("No location details were found in this JPG, try dragging in an authentic Hifi snapshot.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
}

void Application::sendPingPackets() {
    QByteArray pingPacket = DependencyManager::get<NodeList>()->constructPingPacket();
    controlledBroadcastToNodes(pingPacket, NodeSet()
                               << NodeType::EntityServer
                               << NodeType::AudioMixer << NodeType::AvatarMixer
                               << NodeType::MetavoxelServer);
}

//  Every second, check the frame rates and other stuff
void Application::timer() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        sendPingPackets();
    }

    float diffTime = (float)_timerStart.nsecsElapsed() / 1000000000.0f;

    _fps = (float)_frameCount / diffTime;

    _packetsPerSecond = (float) _datagramProcessor.getPacketCount() / diffTime;
    _bytesPerSecond = (float) _datagramProcessor.getByteCount() / diffTime;
    _frameCount = 0;

    _datagramProcessor.resetCounters();

    _timerStart.start();

    // ask the node list to check in with the domain server
    DependencyManager::get<NodeList>()->sendDomainServerCheckIn();
}

void Application::idle() {
    PerformanceTimer perfTimer("idle");

    // Normally we check PipelineWarnings, but since idle will often take more than 10ms we only show these idle timing
    // details if we're in ExtraDebugging mode. However, the ::update() and it's subcomponents will show their timing
    // details normally.
    bool showWarnings = getLogger()->extraDebugging();
    PerformanceWarning warn(showWarnings, "idle()");

    //  Only run simulation code if more than the targetFramePeriod have passed since last time we ran
    double targetFramePeriod = 0.0;
    unsigned int targetFramerate = getRenderTargetFramerate();
    if (targetFramerate > 0) {
        targetFramePeriod = 1000.0 / targetFramerate;
    }
    double timeSinceLastUpdate = (double)_lastTimeUpdated.nsecsElapsed() / 1000000.0;
    if (timeSinceLastUpdate > targetFramePeriod) {
        _lastTimeUpdated.start();
        {
            PerformanceTimer perfTimer("update");
            PerformanceWarning warn(showWarnings, "Application::idle()... update()");
            const float BIGGEST_DELTA_TIME_SECS = 0.25f;
            update(glm::clamp((float)timeSinceLastUpdate / 1000.0f, 0.0f, BIGGEST_DELTA_TIME_SECS));
        }
        {
            PerformanceTimer perfTimer("updateGL");
            PerformanceWarning warn(showWarnings, "Application::idle()... updateGL()");
            DependencyManager::get<GLCanvas>()->updateGL();
        }
        {
            PerformanceTimer perfTimer("rest");
            PerformanceWarning warn(showWarnings, "Application::idle()... rest of it");
            _idleLoopStdev.addValue(timeSinceLastUpdate);

            //  Record standard deviation and reset counter if needed
            const int STDEV_SAMPLES = 500;
            if (_idleLoopStdev.getSamples() > STDEV_SAMPLES) {
                _idleLoopMeasuredJitter = _idleLoopStdev.getStDev();
                _idleLoopStdev.reset();
            }

            // After finishing all of the above work, restart the idle timer, allowing 2ms to process events.
            idleTimer->start(2);
        }
    }
}

void Application::checkBandwidthMeterClick() {
    // ... to be called upon button release
    auto glCanvas = DependencyManager::get<GLCanvas>();
    if (Menu::getInstance()->isOptionChecked(MenuOption::Bandwidth) &&
        Menu::getInstance()->isOptionChecked(MenuOption::Stats) &&
        Menu::getInstance()->isOptionChecked(MenuOption::UserInterface) &&
        glm::compMax(glm::abs(glm::ivec2(getMouseX() - getMouseDragStartedX(),
                                         getMouseY() - getMouseDragStartedY())))
        <= BANDWIDTH_METER_CLICK_MAX_DRAG_LENGTH
        && _bandwidthMeter.isWithinArea(getMouseX(), getMouseY(), glCanvas->width(), glCanvas->height())) {

        // The bandwidth meter is visible, the click didn't get dragged too far and
        // we actually hit the bandwidth meter
        DependencyManager::get<DialogsManager>()->bandwidthDetails();
    }
}

void Application::setFullscreen(bool fullscreen) {
    if (Menu::getInstance()->isOptionChecked(MenuOption::Fullscreen) != fullscreen) {
        Menu::getInstance()->getActionForOption(MenuOption::Fullscreen)->setChecked(fullscreen);
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode)) {
        if (fullscreen) {
            // Menu show() after hide() doesn't work with Rift VR display so set height instead.
            _window->menuBar()->setMaximumHeight(0);
        } else {
            _window->menuBar()->setMaximumHeight(QWIDGETSIZE_MAX);
        }
    }
    _window->setWindowState(fullscreen ? (_window->windowState() | Qt::WindowFullScreen) :
        (_window->windowState() & ~Qt::WindowFullScreen));
    if (!_aboutToQuit) {
        _window->show();
    }
}

void Application::setEnable3DTVMode(bool enable3DTVMode) {
    auto glCanvas = DependencyManager::get<GLCanvas>();
    resizeGL(glCanvas->getDeviceWidth(), glCanvas->getDeviceHeight());
}

void Application::setEnableVRMode(bool enableVRMode) {
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode) != enableVRMode) {
        Menu::getInstance()->getActionForOption(MenuOption::EnableVRMode)->setChecked(enableVRMode);
    }

    if (enableVRMode) {
        if (!OculusManager::isConnected()) {
            // attempt to reconnect the Oculus manager - it's possible this was a workaround
            // for the sixense crash
            OculusManager::disconnect();
            OculusManager::connect();
        }
        OculusManager::recalibrate();
    } else {
        OculusManager::abandonCalibration();
        
        _mirrorCamera.setHmdPosition(glm::vec3());
        _mirrorCamera.setHmdRotation(glm::quat());
        _myCamera.setHmdPosition(glm::vec3());
        _myCamera.setHmdRotation(glm::quat());
    }
    
    auto glCanvas = DependencyManager::get<GLCanvas>();
    resizeGL(glCanvas->getDeviceWidth(), glCanvas->getDeviceHeight());
}

void Application::setLowVelocityFilter(bool lowVelocityFilter) {
    SixenseManager::getInstance().setLowVelocityFilter(lowVelocityFilter);
}

bool Application::mouseOnScreen() const {
    if (OculusManager::isConnected()) {
        auto glCanvas = DependencyManager::get<GLCanvas>();
        return getMouseX() >= 0 && getMouseX() <= glCanvas->getDeviceWidth() &&
               getMouseY() >= 0 && getMouseY() <= glCanvas->getDeviceHeight();
    }
    return true;
}

int Application::getMouseX() const {
    if (OculusManager::isConnected()) {
        glm::vec2 pos = _applicationOverlay.screenToOverlay(glm::vec2(getTrueMouseX(), getTrueMouseY()));
        return pos.x;
    }
    return getTrueMouseX();
}

int Application::getMouseY() const {
    if (OculusManager::isConnected()) {
        glm::vec2 pos = _applicationOverlay.screenToOverlay(glm::vec2(getTrueMouseX(), getTrueMouseY()));
        return pos.y;
    }
    return getTrueMouseY();
}

int Application::getMouseDragStartedX() const {
    if (OculusManager::isConnected()) {
        glm::vec2 pos = _applicationOverlay.screenToOverlay(glm::vec2(getTrueMouseDragStartedX(),
                                                                      getTrueMouseDragStartedY()));
        return pos.x;
    }
    return getTrueMouseDragStartedX();
}

int Application::getMouseDragStartedY() const {
    if (OculusManager::isConnected()) {
        glm::vec2 pos = _applicationOverlay.screenToOverlay(glm::vec2(getTrueMouseDragStartedX(),
                                                                      getTrueMouseDragStartedY()));
        return pos.y;
    }
    return getTrueMouseDragStartedY();
}

FaceTracker* Application::getActiveFaceTracker() {
    auto faceshift = DependencyManager::get<Faceshift>();
    auto visage = DependencyManager::get<Visage>();
    auto dde = DependencyManager::get<DdeFaceTracker>();
    
    return (dde->isActive() ? static_cast<FaceTracker*>(dde.data()) :
            (faceshift->isActive() ? static_cast<FaceTracker*>(faceshift.data()) :
             (visage->isActive() ? static_cast<FaceTracker*>(visage.data()) : NULL)));
}

bool Application::exportEntities(const QString& filename, float x, float y, float z, float scale) {
    QVector<EntityItem*> entities;
    _entities.getTree()->findEntities(AACube(glm::vec3(x / (float)TREE_SCALE, 
                                y / (float)TREE_SCALE, z / (float)TREE_SCALE), scale / (float)TREE_SCALE), entities);

    if (entities.size() > 0) {
        glm::vec3 root(x, y, z);
        EntityTree exportTree;

        for (int i = 0; i < entities.size(); i++) {
            EntityItemProperties properties = entities.at(i)->getProperties();
            EntityItemID id = entities.at(i)->getEntityItemID();
            properties.setPosition(properties.getPosition() - root);
            exportTree.addEntity(id, properties);
        }
        exportTree.writeToSVOFile(filename.toLocal8Bit().constData());
    } else {
        qDebug() << "No models were selected";
        return false;
    }

    // restore the main window's active state
    _window->activateWindow();
    return true;
}

void Application::loadSettings() {
    DependencyManager::get<Audio>()->loadSettings();
    DependencyManager::get<LODManager>()->loadSettings();
    Menu::getInstance()->loadSettings();
    _myAvatar->loadData();
}

void Application::saveSettings() {
    DependencyManager::get<Audio>()->saveSettings();
    DependencyManager::get<LODManager>()->saveSettings();
    Menu::getInstance()->saveSettings();
    _myAvatar->saveData();
}

bool Application::importEntities(const QString& filename) {
    _entityClipboard.eraseAllOctreeElements();
    bool success = _entityClipboard.readFromSVOFile(filename.toLocal8Bit().constData());
    if (success) {
        _entityClipboard.reaverageOctreeElements();
    }
    return success;
}

void Application::pasteEntities(float x, float y, float z) {
    _entityClipboard.sendEntities(&_entityEditSender, _entities.getTree(), x, y, z);
}

void Application::initDisplay() {
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
}

void Application::init() {
    _environment.init();

    DependencyManager::get<DeferredLightingEffect>()->init(this);
    DependencyManager::get<AmbientOcclusionEffect>()->init(this);

    // TODO: move _myAvatar out of Application. Move relevant code to MyAvataar or AvatarManager
    _avatarManager.init();
    _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);

    _mirrorCamera.setMode(CAMERA_MODE_MIRROR);

    OculusManager::connect();
    if (OculusManager::isConnected()) {
        QMetaObject::invokeMethod(Menu::getInstance()->getActionForOption(MenuOption::Fullscreen),
                                  "trigger",
                                  Qt::QueuedConnection);
    }

    TV3DManager::connect();
    if (TV3DManager::isConnected()) {
        QMetaObject::invokeMethod(Menu::getInstance()->getActionForOption(MenuOption::Fullscreen),
                                  "trigger",
                                  Qt::QueuedConnection);
    }

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
    
    qDebug() << "Loaded settings";
    
#ifdef __APPLE__
    if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseEnabled)) {
        // on OS X we only setup sixense if the user wants it on - this allows running without the hid_init crash
        // if hydra support is temporarily not required
        SixenseManager::getInstance().toggleSixense(true);
    }
#else
    // setup sixense
    SixenseManager::getInstance().toggleSixense(true);
#endif

    // initialize our face trackers after loading the menu settings
    DependencyManager::get<Faceshift>()->init();
    DependencyManager::get<Visage>()->init();

    Leapmotion::init();
    RealSense::init();

    // fire off an immediate domain-server check in now that settings are loaded
    DependencyManager::get<NodeList>()->sendDomainServerCheckIn();

    _entities.init();
    _entities.setViewFrustum(getViewFrustum());

    EntityTree* tree = _entities.getTree();
    _physicsEngine.setEntityTree(tree);
    tree->setSimulation(&_physicsEngine);
    _physicsEngine.init(&_entityEditSender);

    connect(&_physicsEngine, &EntitySimulation::entityCollisionWithEntity,
            ScriptEngine::getEntityScriptingInterface(), &EntityScriptingInterface::entityCollisionWithEntity);

    // connect the _entityCollisionSystem to our EntityTreeRenderer since that's what handles running entity scripts
    connect(&_physicsEngine, &EntitySimulation::entityCollisionWithEntity,
            &_entities, &EntityTreeRenderer::entityCollisionWithEntity);

    // connect the _entities (EntityTreeRenderer) to our script engine's EntityScriptingInterface for firing
    // of events related clicking, hovering over, and entering entities
    _entities.connectSignalsToSlots(ScriptEngine::getEntityScriptingInterface());

    _entityClipboardRenderer.init();
    _entityClipboardRenderer.setViewFrustum(getViewFrustum());
    _entityClipboardRenderer.setTree(&_entityClipboard);

    _metavoxels.init();

    auto glCanvas = DependencyManager::get<GLCanvas>();
    _rearMirrorTools = new RearMirrorTools(glCanvas.data(), _mirrorViewRect);

    connect(_rearMirrorTools, SIGNAL(closeView()), SLOT(closeMirrorView()));
    connect(_rearMirrorTools, SIGNAL(restoreView()), SLOT(restoreMirrorView()));
    connect(_rearMirrorTools, SIGNAL(shrinkView()), SLOT(shrinkMirrorView()));
    connect(_rearMirrorTools, SIGNAL(resetView()), SLOT(resetSensors()));

    // make sure our texture cache knows about window size changes
    DependencyManager::get<TextureCache>()->associateWithWidget(glCanvas.data());

    // initialize the GlowEffect with our widget
    DependencyManager::get<GlowEffect>()->init(glCanvas.data(),
                                               Menu::getInstance()->isOptionChecked(MenuOption::EnableGlowEffect));
}

void Application::closeMirrorView() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
        Menu::getInstance()->triggerOption(MenuOption::Mirror);
    }
}

void Application::restoreMirrorView() {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        Menu::getInstance()->triggerOption(MenuOption::FullscreenMirror);
    }
}

void Application::shrinkMirrorView() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        Menu::getInstance()->triggerOption(MenuOption::FullscreenMirror);
    }
}

const float HEAD_SPHERE_RADIUS = 0.1f;

bool Application::isLookingAtMyAvatar(Avatar* avatar) {
    glm::vec3 theirLookAt = avatar->getHead()->getLookAtPosition();
    glm::vec3 myEyePosition = _myAvatar->getHead()->getEyePosition();
    if (pointInSphere(theirLookAt, myEyePosition, HEAD_SPHERE_RADIUS * _myAvatar->getScale())) {
        return true;
    }
    return false;
}

void Application::updateLOD() {
    PerformanceTimer perfTimer("LOD");
    // adjust it unless we were asked to disable this feature, or if we're currently in throttleRendering mode
    if (!Menu::getInstance()->isOptionChecked(MenuOption::DisableAutoAdjustLOD) && !isThrottleRendering()) {
        DependencyManager::get<LODManager>()->autoAdjustLOD(_fps);
    } else {
        DependencyManager::get<LODManager>()->resetLODAdjust();
    }
}

void Application::updateMouseRay() {
    PerformanceTimer perfTimer("mouseRay");

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMouseRay()");

    // make sure the frustum is up-to-date
    loadViewFrustum(_myCamera, _viewFrustum);

    PickRay pickRay = _myCamera.computePickRay(getTrueMouseX(), getTrueMouseY());
    _mouseRayOrigin = pickRay.origin;
    _mouseRayDirection = pickRay.direction;
    
    // adjust for mirroring
    if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        glm::vec3 mouseRayOffset = _mouseRayOrigin - _viewFrustum.getPosition();
        _mouseRayOrigin -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), mouseRayOffset) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), mouseRayOffset));
        _mouseRayDirection -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), _mouseRayDirection) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), _mouseRayDirection));
    }
}

void Application::updateFaceshift() {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateFaceshift()");
    auto faceshift = DependencyManager::get<Faceshift>();
    //  Update faceshift
    faceshift->update();

    //  Copy angular velocity if measured by faceshift, to the head
    if (faceshift->isActive()) {
        _myAvatar->getHead()->setAngularVelocity(faceshift->getHeadAngularVelocity());
    }
}

void Application::updateVisage() {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateVisage()");

    //  Update Visage
    DependencyManager::get<Visage>()->update();
}

void Application::updateDDE() {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateDDE()");
    
    //  Update Cara
    DependencyManager::get<DdeFaceTracker>()->update();
}

void Application::updateMyAvatarLookAtPosition() {
    PerformanceTimer perfTimer("lookAt");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMyAvatarLookAtPosition()");

    _myAvatar->updateLookAtTargetAvatar();
    FaceTracker* tracker = getActiveFaceTracker();

    bool isLookingAtSomeone = false;
    glm::vec3 lookAtSpot;
    if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        //  When I am in mirror mode, just look right at the camera (myself)
        if (!OculusManager::isConnected()) {
            lookAtSpot = _myCamera.getPosition();
        } else {
            if (_myAvatar->isLookingAtLeftEye()) {
                lookAtSpot = OculusManager::getLeftEyePosition();
            } else {
                lookAtSpot = OculusManager::getRightEyePosition();
            }
        }
 
    } else {
        AvatarSharedPointer lookingAt = _myAvatar->getLookAtTargetAvatar().toStrongRef();
        if (lookingAt && _myAvatar != lookingAt.data()) {
            
            isLookingAtSomeone = true;
            //  If I am looking at someone else, look directly at one of their eyes
            if (tracker) {
                //  If a face tracker is active, look at the eye for the side my gaze is biased toward
                if (tracker->getEstimatedEyeYaw() > _myAvatar->getHead()->getFinalYaw()) {
                    // Look at their right eye
                    lookAtSpot = static_cast<Avatar*>(lookingAt.data())->getHead()->getRightEyePosition();
                } else {
                    // Look at their left eye
                    lookAtSpot = static_cast<Avatar*>(lookingAt.data())->getHead()->getLeftEyePosition();
                }
            } else {
                //  Need to add randomly looking back and forth between left and right eye for case with no tracker
                if (_myAvatar->isLookingAtLeftEye()) {
                    lookAtSpot = static_cast<Avatar*>(lookingAt.data())->getHead()->getLeftEyePosition();
                } else {
                    lookAtSpot = static_cast<Avatar*>(lookingAt.data())->getHead()->getRightEyePosition();
                }
            }
        } else {
            //  I am not looking at anyone else, so just look forward
            lookAtSpot = _myAvatar->getHead()->getEyePosition() +
                (_myAvatar->getHead()->getFinalOrientationInWorldFrame() * glm::vec3(0.0f, 0.0f, -TREE_SCALE));
        }
    }
    //
    //  Deflect the eyes a bit to match the detected Gaze from 3D camera if active
    //
    if (tracker) {
        float eyePitch = tracker->getEstimatedEyePitch();
        float eyeYaw = tracker->getEstimatedEyeYaw();
        const float GAZE_DEFLECTION_REDUCTION_DURING_EYE_CONTACT = 0.1f;
        // deflect using Faceshift gaze data
        glm::vec3 origin = _myAvatar->getHead()->getEyePosition();
        float pitchSign = (_myCamera.getMode() == CAMERA_MODE_MIRROR) ? -1.0f : 1.0f;
        float deflection = DependencyManager::get<Faceshift>()->getEyeDeflection();
        if (isLookingAtSomeone) {
            deflection *= GAZE_DEFLECTION_REDUCTION_DURING_EYE_CONTACT;
        }
        lookAtSpot = origin + _myCamera.getRotation() * glm::quat(glm::radians(glm::vec3(
            eyePitch * pitchSign * deflection, eyeYaw * deflection, 0.0f))) *
                glm::inverse(_myCamera.getRotation()) * (lookAtSpot - origin);
    }

    _myAvatar->getHead()->setLookAtPosition(lookAtSpot);
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

void Application::updateMetavoxels(float deltaTime) {
    PerformanceTimer perfTimer("updateMetavoxels");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMetavoxels()");

    if (Menu::getInstance()->isOptionChecked(MenuOption::Metavoxels)) {
        _metavoxels.simulate(deltaTime);
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
        }
    } else {
        if (_myCamera.getMode() != CAMERA_MODE_THIRD_PERSON) {
            _myCamera.setMode(CAMERA_MODE_THIRD_PERSON);
        }
    }
}

void Application::updateCamera(float deltaTime) {
    PerformanceTimer perfTimer("updateCamera");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateCamera()");

    if (!OculusManager::isConnected() && !TV3DManager::isConnected() &&
            Menu::getInstance()->isOptionChecked(MenuOption::OffAxisProjection)) {
        FaceTracker* tracker = getActiveFaceTracker();
        if (tracker) {
            const float EYE_OFFSET_SCALE = 0.025f;
            glm::vec3 position = tracker->getHeadTranslation() * EYE_OFFSET_SCALE;
            float xSign = (_myCamera.getMode() == CAMERA_MODE_MIRROR) ? 1.0f : -1.0f;
            _myCamera.setEyeOffsetPosition(glm::vec3(position.x * xSign, position.y, -position.z));
            updateProjectionMatrix();
        }
    }
}

void Application::updateDialogs(float deltaTime) {
    PerformanceTimer perfTimer("updateDialogs");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateDialogs()");
    auto dialogsManager = DependencyManager::get<DialogsManager>();
    
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

void Application::updateCursor(float deltaTime) {
    PerformanceTimer perfTimer("updateCursor");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateCursor()");

    static QPoint lastMousePos = QPoint();
    _lastMouseMove = (lastMousePos == QCursor::pos()) ? _lastMouseMove : usecTimestampNow();
    bool hideMouse = false;
    bool underMouse = QGuiApplication::topLevelAt(QCursor::pos()) ==
                      Application::getInstance()->getWindow()->windowHandle();
    
    static const int HIDE_CURSOR_TIMEOUT = 3 * USECS_PER_SECOND; // 3 second
    int elapsed = usecTimestampNow() - _lastMouseMove;
    if ((elapsed > HIDE_CURSOR_TIMEOUT)  ||
        (OculusManager::isConnected() && Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode))) {
        hideMouse = underMouse;
    }
    
    setCursorVisible(!hideMouse);
    lastMousePos = QCursor::pos();
}

void Application::setCursorVisible(bool visible) {
    if (visible) {
        if (overrideCursor() != NULL) {
            restoreOverrideCursor();
        }
    } else {
        if (overrideCursor() != NULL) {
            changeOverrideCursor(Qt::BlankCursor);
        } else {
            setOverrideCursor(Qt::BlankCursor);
        }
    }
}

void Application::update(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::update()");

    updateLOD();
    updateMouseRay(); // check what's under the mouse and update the mouse voxel
    {
        PerformanceTimer perfTimer("devices");
        DeviceTracker::updateAll();
        updateFaceshift();
        updateVisage();
        SixenseManager::getInstance().update(deltaTime);
        JoystickScriptingInterface::getInstance().update();
        _prioVR.update(deltaTime);

    }
    
    // Dispatch input events
    _controllerScriptingInterface.updateInputControllers();

    updateThreads(deltaTime); // If running non-threaded, then give the threads some time to process...
    
    _avatarManager.updateOtherAvatars(deltaTime); //loop through all the other avatars and simulate them...

    updateMetavoxels(deltaTime); // update metavoxels
    updateCamera(deltaTime); // handle various camera tweaks like off axis projection
    updateDialogs(deltaTime); // update various stats dialogs if present
    updateCursor(deltaTime); // Handle cursor updates

    {
        PerformanceTimer perfTimer("physics");
        _physicsEngine.stepSimulation();
    }

    if (!_aboutToQuit) {
        PerformanceTimer perfTimer("entities");
        // NOTE: the _entities.update() call below will wait for lock 
        // and will simulate entity motion (the EntityTree has been given an EntitySimulation).  
        _entities.update(); // update the models...
    }

    {
        PerformanceTimer perfTimer("overlays");
        _overlays.update(deltaTime);
    }
    
    {
        PerformanceTimer perfTimer("myAvatar");
        updateMyAvatarLookAtPosition();
        updateMyAvatar(deltaTime); // Sample hardware, update view frustum if needed, and send avatar data to mixer/nodes
    }

    {
        PerformanceTimer perfTimer("emitSimulating");
        // let external parties know we're updating
        emit simulating(deltaTime);
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
        PerformanceTimer perfTimer("queryOctree");
        quint64 sinceLastQuery = now - _lastQueriedTime;
        const quint64 TOO_LONG_SINCE_LAST_QUERY = 3 * USECS_PER_SECOND;
        bool queryIsDue = sinceLastQuery > TOO_LONG_SINCE_LAST_QUERY;
        bool viewIsDifferentEnough = !_lastQueriedViewFrustum.isVerySimilar(_viewFrustum);

        // if it's been a while since our last query or the view has significantly changed then send a query, otherwise suppress it
        if (queryIsDue || viewIsDifferentEnough) {
            _lastQueriedTime = now;

            if (Menu::getInstance()->isOptionChecked(MenuOption::Entities)) {
                queryOctree(NodeType::EntityServer, PacketTypeEntityQuery, _entityServerJurisdictions);
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

            QMetaObject::invokeMethod(DependencyManager::get<Audio>().data(), "sendDownstreamAudioStatsPacket", Qt::QueuedConnection);
        }
    }
}

void Application::updateMyAvatar(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMyAvatar()");

    _myAvatar->update(deltaTime);

    quint64 now = usecTimestampNow();
    quint64 dt = now - _lastSendAvatarDataTime;

    if (dt > MIN_TIME_BETWEEN_MY_AVATAR_DATA_SENDS) {
        // send head/hand data to the avatar mixer and voxel server
        PerformanceTimer perfTimer("send");
        QByteArray packet = byteArrayWithPopulatedHeader(PacketTypeAvatarData);
        packet.append(_myAvatar->toByteArray());
        controlledBroadcastToNodes(packet, NodeSet() << NodeType::AvatarMixer);

        _lastSendAvatarDataTime = now;
    }
}

int Application::sendNackPackets() {

    if (Menu::getInstance()->isOptionChecked(MenuOption::DisableNackPackets)) {
        return 0;
    }

    int packetsSent = 0;
    char packet[MAX_PACKET_SIZE];

    // iterates thru all nodes in NodeList
    auto nodeList = DependencyManager::get<NodeList>();
    
    nodeList->eachNode([&](const SharedNodePointer& node){
        
        if (node->getActiveSocket() && node->getType() == NodeType::EntityServer) {
            
            QUuid nodeUUID = node->getUUID();
            
            // if there are octree packets from this node that are waiting to be processed,
            // don't send a NACK since the missing packets may be among those waiting packets.
            if (_octreeProcessor.hasPacketsToProcessFrom(nodeUUID)) {
                return;
            }
            
            _octreeSceneStatsLock.lockForRead();
            
            // retreive octree scene stats of this node
            if (_octreeServerSceneStats.find(nodeUUID) == _octreeServerSceneStats.end()) {
                _octreeSceneStatsLock.unlock();
                return;
            }
            
            // get sequence number stats of node, prune its missing set, and make a copy of the missing set
            SequenceNumberStats& sequenceNumberStats = _octreeServerSceneStats[nodeUUID].getIncomingOctreeSequenceNumberStats();
            sequenceNumberStats.pruneMissingSet();
            const QSet<OCTREE_PACKET_SEQUENCE> missingSequenceNumbers = sequenceNumberStats.getMissingSet();
            
            _octreeSceneStatsLock.unlock();
            
            // construct nack packet(s) for this node
            int numSequenceNumbersAvailable = missingSequenceNumbers.size();
            QSet<OCTREE_PACKET_SEQUENCE>::const_iterator missingSequenceNumbersIterator = missingSequenceNumbers.constBegin();
            while (numSequenceNumbersAvailable > 0) {
                
                char* dataAt = packet;
                int bytesRemaining = MAX_PACKET_SIZE;
                
                // pack header
                int numBytesPacketHeader = populatePacketHeader(packet, PacketTypeOctreeDataNack);
                dataAt += numBytesPacketHeader;
                bytesRemaining -= numBytesPacketHeader;
                
                // calculate and pack the number of sequence numbers
                int numSequenceNumbersRoomFor = (bytesRemaining - sizeof(uint16_t)) / sizeof(OCTREE_PACKET_SEQUENCE);
                uint16_t numSequenceNumbers = min(numSequenceNumbersAvailable, numSequenceNumbersRoomFor);
                uint16_t* numSequenceNumbersAt = (uint16_t*)dataAt;
                *numSequenceNumbersAt = numSequenceNumbers;
                dataAt += sizeof(uint16_t);
                
                // pack sequence numbers
                for (int i = 0; i < numSequenceNumbers; i++) {
                    OCTREE_PACKET_SEQUENCE* sequenceNumberAt = (OCTREE_PACKET_SEQUENCE*)dataAt;
                    *sequenceNumberAt = *missingSequenceNumbersIterator;
                    dataAt += sizeof(OCTREE_PACKET_SEQUENCE);
                    
                    missingSequenceNumbersIterator++;
                }
                numSequenceNumbersAvailable -= numSequenceNumbers;
                
                // send it
                nodeList->writeUnverifiedDatagram(packet, dataAt - packet, node);
                packetsSent++;
            }
        }
    });

    return packetsSent;
}

void Application::queryOctree(NodeType_t serverType, PacketType packetType, NodeToJurisdictionMap& jurisdictions) {

    //qDebug() << ">>> inside... queryOctree()... _viewFrustum.getFieldOfView()=" << _viewFrustum.getFieldOfView();
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
    _octreeQuery.setCameraEyeOffsetPosition(_viewFrustum.getEyeOffsetPosition());
    auto lodManager = DependencyManager::get<LODManager>();
    _octreeQuery.setOctreeSizeScale(lodManager->getOctreeSizeScale());
    _octreeQuery.setBoundaryLevelAdjust(lodManager->getBoundaryLevelAdjust());

    unsigned char queryPacket[MAX_PACKET_SIZE];

    // Iterate all of the nodes, and get a count of how many voxel servers we have...
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
                    AACube serverBounds(glm::vec3(rootDetails.x, rootDetails.y, rootDetails.z), rootDetails.s);
                    serverBounds.scale(TREE_SCALE);

                    ViewFrustum::location serverFrustumLocation = _viewFrustum.cubeInFrustum(serverBounds);

                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
                        inViewServers++;
                    }
                }
            }
        }
    });

    if (wantExtraDebugging) {
        qDebug("Servers: total %d, in view %d, unknown jurisdiction %d",
            totalServers, inViewServers, unknownJurisdictionServers);
    }

    int perServerPPS = 0;
    const int SMALL_BUDGET = 10;
    int perUnknownServer = SMALL_BUDGET;
    int totalPPS = _octreeQuery.getMaxOctreePacketsPerSecond();

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
        qDebug("perServerPPS: %d perUnknownServer: %d", perServerPPS, perUnknownServer);
    }
    
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
                    qDebug() << "no known jurisdiction for node " << *node << ", assume it's visible.";
                }
            } else {
                const JurisdictionMap& map = (jurisdictions)[nodeUUID];
                
                unsigned char* rootCode = map.getRootOctalCode();
                
                if (rootCode) {
                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode, rootDetails);
                    AACube serverBounds(glm::vec3(rootDetails.x, rootDetails.y, rootDetails.z), rootDetails.s);
                    serverBounds.scale(TREE_SCALE);
                    
                    ViewFrustum::location serverFrustumLocation = _viewFrustum.cubeInFrustum(serverBounds);
                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
                        inView = true;
                    } else {
                        inView = false;
                    }
                } else {
                    if (wantExtraDebugging) {
                        qDebug() << "Jurisdiction without RootCode for node " << *node << ". That's unusual!";
                    }
                }
            }
            
            if (inView) {
                _octreeQuery.setMaxOctreePacketsPerSecond(perServerPPS);
            } else if (unknownView) {
                if (wantExtraDebugging) {
                    qDebug() << "no known jurisdiction for node " << *node << ", give it budget of "
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
                        qDebug() << "Using 'minimal' camera position for node" << *node;
                    }
                } else {
                    if (wantExtraDebugging) {
                        qDebug() << "Using regular camera position for node" << *node;
                    }
                }
                _octreeQuery.setMaxOctreePacketsPerSecond(perUnknownServer);
            } else {
                _octreeQuery.setMaxOctreePacketsPerSecond(0);
            }
            // set up the packet for sending...
            unsigned char* endOfQueryPacket = queryPacket;
            
            // insert packet type/version and node UUID
            endOfQueryPacket += populatePacketHeader(reinterpret_cast<char*>(endOfQueryPacket), packetType);
            
            // encode the query data...
            endOfQueryPacket += _octreeQuery.getBroadcastData(endOfQueryPacket);
            
            int packetLength = endOfQueryPacket - queryPacket;
            
            // make sure we still have an active socket
            nodeList->writeUnverifiedDatagram(reinterpret_cast<const char*>(queryPacket), packetLength, node);
            
            // Feed number of bytes to corresponding channel of the bandwidth meter
            _bandwidthMeter.outputStream(BandwidthMeter::OCTREE).updateValue(packetLength);
        }
    });
}

bool Application::isHMDMode() const {
    if (OculusManager::isConnected()) {
        return true;
    } else {
        return false;
    }
}

QRect Application::getDesirableApplicationGeometry() {
    QRect applicationGeometry = getWindow()->geometry();
    
    // If our parent window is on the HMD, then don't use it's geometry, instead use
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
    // We will use these below, from either the camera or head vectors calculated above
    glm::vec3 position(camera.getPosition());
    float fov         = camera.getFieldOfView();    // degrees
    float nearClip    = camera.getNearClip();
    float farClip     = camera.getFarClip();
    float aspectRatio = camera.getAspectRatio();

    glm::quat rotation = camera.getRotation();

    // Set the viewFrustum up with the correct position and orientation of the camera
    viewFrustum.setPosition(position);
    viewFrustum.setOrientation(rotation);

    // Also make sure it's got the correct lens details from the camera
    viewFrustum.setAspectRatio(aspectRatio);
    viewFrustum.setFieldOfView(fov);    // degrees
    viewFrustum.setNearClip(nearClip);
    viewFrustum.setFarClip(farClip);
    viewFrustum.setEyeOffsetPosition(camera.getEyeOffsetPosition());
    viewFrustum.setEyeOffsetOrientation(camera.getEyeOffsetOrientation());

    // Ask the ViewFrustum class to calculate our corners
    viewFrustum.calculate();
}

glm::vec3 Application::getSunDirection() {
    // Sun direction is in fact just the location of the sun relative to the origin
    return glm::normalize(_environment.getClosestData(_myCamera.getPosition()).getSunLocation(_myCamera.getPosition()));
}

void Application::updateShadowMap() {
    PerformanceTimer perfTimer("shadowMap");
    QOpenGLFramebufferObject* fbo = DependencyManager::get<TextureCache>()->getShadowFramebufferObject();
    fbo->bind();
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    glm::vec3 lightDirection = -getSunDirection();
    glm::quat rotation = rotationBetween(IDENTITY_FRONT, lightDirection);
    glm::quat inverseRotation = glm::inverse(rotation);
    
    const float SHADOW_MATRIX_DISTANCES[] = { 0.0f, 2.0f, 6.0f, 14.0f, 30.0f };
    const glm::vec2 MAP_COORDS[] = { glm::vec2(0.0f, 0.0f), glm::vec2(0.5f, 0.0f),
        glm::vec2(0.0f, 0.5f), glm::vec2(0.5f, 0.5f) };
    
    float frustumScale = 1.0f / (_viewFrustum.getFarClip() - _viewFrustum.getNearClip());
    loadViewFrustum(_myCamera, _viewFrustum);
    
    int matrixCount = 1;
    int targetSize = fbo->width();
    float targetScale = 1.0f;
    if (Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows)) {
        matrixCount = CASCADED_SHADOW_MATRIX_COUNT;
        targetSize = fbo->width() / 2;
        targetScale = 0.5f;
    }
    for (int i = 0; i < matrixCount; i++) {
        const glm::vec2& coord = MAP_COORDS[i];
        glViewport(coord.s * fbo->width(), coord.t * fbo->height(), targetSize, targetSize);

        // if simple shadow then since the resolution is twice as much as with cascaded, cover 2 regions with the map, not just one
        int regionIncrement = (matrixCount == 1 ? 2 : 1);
        float nearScale = SHADOW_MATRIX_DISTANCES[i] * frustumScale;
        float farScale = SHADOW_MATRIX_DISTANCES[i + regionIncrement] * frustumScale;
        glm::vec3 points[] = {
            glm::mix(_viewFrustum.getNearTopLeft(), _viewFrustum.getFarTopLeft(), nearScale),
            glm::mix(_viewFrustum.getNearTopRight(), _viewFrustum.getFarTopRight(), nearScale),
            glm::mix(_viewFrustum.getNearBottomLeft(), _viewFrustum.getFarBottomLeft(), nearScale),
            glm::mix(_viewFrustum.getNearBottomRight(), _viewFrustum.getFarBottomRight(), nearScale),
            glm::mix(_viewFrustum.getNearTopLeft(), _viewFrustum.getFarTopLeft(), farScale),
            glm::mix(_viewFrustum.getNearTopRight(), _viewFrustum.getFarTopRight(), farScale),
            glm::mix(_viewFrustum.getNearBottomLeft(), _viewFrustum.getFarBottomLeft(), farScale),
            glm::mix(_viewFrustum.getNearBottomRight(), _viewFrustum.getFarBottomRight(), farScale) };
        glm::vec3 center;
        for (size_t j = 0; j < sizeof(points) / sizeof(points[0]); j++) {
            center += points[j];
        }
        center /= (float)(sizeof(points) / sizeof(points[0]));
        float radius = 0.0f;
        for (size_t j = 0; j < sizeof(points) / sizeof(points[0]); j++) {
            radius = qMax(radius, glm::distance(points[j], center));
        }
        if (i < 3) {
            const float RADIUS_SCALE = 0.5f;
            _shadowDistances[i] = -glm::distance(_viewFrustum.getPosition(), center) - radius * RADIUS_SCALE;
        }
        center = inverseRotation * center;
        
        // to reduce texture "shimmer," move in texel increments
        float texelSize = (2.0f * radius) / targetSize;
        center = glm::vec3(roundf(center.x / texelSize) * texelSize, roundf(center.y / texelSize) * texelSize,
            roundf(center.z / texelSize) * texelSize);
        
        glm::vec3 minima(center.x - radius, center.y - radius, center.z - radius);
        glm::vec3 maxima(center.x + radius, center.y + radius, center.z + radius);

        // stretch out our extents in z so that we get all of the avatars
        minima.z -= _viewFrustum.getFarClip() * 0.5f;
        maxima.z += _viewFrustum.getFarClip() * 0.5f;

        // save the combined matrix for rendering
        _shadowMatrices[i] = glm::transpose(glm::translate(glm::vec3(coord, 0.0f)) *
            glm::scale(glm::vec3(targetScale, targetScale, 1.0f)) *
            glm::translate(glm::vec3(0.5f, 0.5f, 0.5f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)) *
            glm::ortho(minima.x, maxima.x, minima.y, maxima.y, -maxima.z, -minima.z) * glm::mat4_cast(inverseRotation));

        // update the shadow view frustum
        _shadowViewFrustum.setPosition(rotation * ((minima + maxima) * 0.5f));
        _shadowViewFrustum.setOrientation(rotation);
        _shadowViewFrustum.setOrthographic(true);
        _shadowViewFrustum.setWidth(maxima.x - minima.x);
        _shadowViewFrustum.setHeight(maxima.y - minima.y);
        _shadowViewFrustum.setNearClip(minima.z);
        _shadowViewFrustum.setFarClip(maxima.z);
        _shadowViewFrustum.setEyeOffsetPosition(glm::vec3());
        _shadowViewFrustum.setEyeOffsetOrientation(glm::quat());
        _shadowViewFrustum.calculate();

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(minima.x, maxima.x, minima.y, maxima.y, -maxima.z, -minima.z);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glm::vec3 axis = glm::axis(inverseRotation);
        glRotatef(glm::degrees(glm::angle(inverseRotation)), axis.x, axis.y, axis.z);

        // store view matrix without translation, which we'll use for precision-sensitive objects
        updateUntranslatedViewMatrix();
 
        // Equivalent to what is happening with _untranslatedViewMatrix and the _viewMatrixTranslation
        // the viewTransofmr object is updatded with the correct values and saved,
        // this is what is used for rendering the Entities and avatars
        Transform viewTransform;
        viewTransform.setRotation(rotation);
        setViewTransform(viewTransform);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.1f, 4.0f); // magic numbers courtesy http://www.eecs.berkeley.edu/~ravir/6160/papers/shadowmaps.ppt

        {
            PerformanceTimer perfTimer("avatarManager");
            _avatarManager.renderAvatars(Avatar::SHADOW_RENDER_MODE);
        }

        {
            PerformanceTimer perfTimer("entities");
            _entities.render(RenderArgs::SHADOW_RENDER_MODE);
        }

        // render JS/scriptable overlays
        {
            PerformanceTimer perfTimer("3dOverlays");
            _overlays.renderWorld(false, RenderArgs::SHADOW_RENDER_MODE);
        }

        {
            PerformanceTimer perfTimer("3dOverlaysFront");
            _overlays.renderWorld(true, RenderArgs::SHADOW_RENDER_MODE);
        }

        glDisable(GL_POLYGON_OFFSET_FILL);

        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glMatrixMode(GL_MODELVIEW);
    }
    
    fbo->release();
    
    auto glCanvas = DependencyManager::get<GLCanvas>();
    glViewport(0, 0, glCanvas->getDeviceWidth(), glCanvas->getDeviceHeight());
}

const GLfloat WORLD_AMBIENT_COLOR[] = { 0.525f, 0.525f, 0.6f };
const GLfloat WORLD_DIFFUSE_COLOR[] = { 0.6f, 0.525f, 0.525f };
const GLfloat WORLD_SPECULAR_COLOR[] = { 0.94f, 0.94f, 0.737f, 1.0f };

void Application::setupWorldLight() {

    //  Setup 3D lights (after the camera transform, so that they are positioned in world space)
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glm::vec3 sunDirection = getSunDirection();
    GLfloat light_position0[] = { sunDirection.x, sunDirection.y, sunDirection.z, 0.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, WORLD_AMBIENT_COLOR);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, WORLD_DIFFUSE_COLOR);
    glLightfv(GL_LIGHT0, GL_SPECULAR, WORLD_SPECULAR_COLOR);
    glMaterialfv(GL_FRONT, GL_SPECULAR, WORLD_SPECULAR_COLOR);
    glMateriali(GL_FRONT, GL_SHININESS, 96);
}

bool Application::shouldRenderMesh(float largestDimension, float distanceToCamera) {
    return DependencyManager::get<LODManager>()->shouldRenderMesh(largestDimension, distanceToCamera);
}

float Application::getSizeScale() const { 
    return DependencyManager::get<LODManager>()->getOctreeSizeScale();
}

int Application::getBoundaryLevelAdjust() const { 
    return DependencyManager::get<LODManager>()->getBoundaryLevelAdjust();
}

PickRay Application::computePickRay(float x, float y) {
    return getCamera()->computePickRay(x, y);
}

QImage Application::renderAvatarBillboard() {
    DependencyManager::get<TextureCache>()->getPrimaryFramebufferObject()->bind();

    // the "glow" here causes an alpha of one
    Glower glower;

    const int BILLBOARD_SIZE = 64;
    renderRearViewMirror(QRect(0, DependencyManager::get<GLCanvas>()->getDeviceHeight() - BILLBOARD_SIZE,
                               BILLBOARD_SIZE, BILLBOARD_SIZE),
                         true);

    QImage image(BILLBOARD_SIZE, BILLBOARD_SIZE, QImage::Format_ARGB32);
    glReadPixels(0, 0, BILLBOARD_SIZE, BILLBOARD_SIZE, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());

    DependencyManager::get<TextureCache>()->getPrimaryFramebufferObject()->release();

    return image;
}

void Application::displaySide(Camera& theCamera, bool selfAvatarOnly, RenderArgs::RenderSide renderSide) {
    PROFILE_RANGE(__FUNCTION__);
    PerformanceTimer perfTimer("display");
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "Application::displaySide()");
    // transform by eye offset

    // load the view frustum
    loadViewFrustum(theCamera, _displayViewFrustum);

    // flip x if in mirror mode (also requires reversing winding order for backface culling)
    if (theCamera.getMode() == CAMERA_MODE_MIRROR) {
        glScalef(-1.0f, 1.0f, 1.0f);
        glFrontFace(GL_CW);

    } else {
        glFrontFace(GL_CCW);
    }

    glm::vec3 eyeOffsetPos = theCamera.getEyeOffsetPosition();
    glm::quat eyeOffsetOrient = theCamera.getEyeOffsetOrientation();
    glm::vec3 eyeOffsetAxis = glm::axis(eyeOffsetOrient);
    glRotatef(-glm::degrees(glm::angle(eyeOffsetOrient)), eyeOffsetAxis.x, eyeOffsetAxis.y, eyeOffsetAxis.z);
    glTranslatef(-eyeOffsetPos.x, -eyeOffsetPos.y, -eyeOffsetPos.z);

    // transform view according to theCamera
    // could be myCamera (if in normal mode)
    // or could be viewFrustumOffsetCamera if in offset mode

    glm::quat rotation = theCamera.getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(-glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

    // store view matrix without translation, which we'll use for precision-sensitive objects
    updateUntranslatedViewMatrix(-theCamera.getPosition());

    // Equivalent to what is happening with _untranslatedViewMatrix and the _viewMatrixTranslation
    // the viewTransofmr object is updatded with the correct values and saved,
    // this is what is used for rendering the Entities and avatars
    Transform viewTransform;
    viewTransform.setTranslation(theCamera.getPosition());
    viewTransform.setRotation(rotation);
    viewTransform.postTranslate(eyeOffsetPos);
    viewTransform.postRotate(eyeOffsetOrient);
    if (theCamera.getMode() == CAMERA_MODE_MIRROR) {
         viewTransform.setScale(Transform::Vec3(-1.0f, 1.0f, 1.0f));
    }
    if (renderSide != RenderArgs::MONO) {
        glm::mat4 invView = glm::inverse(_untranslatedViewMatrix);
        
        viewTransform.evalFromRawMatrix(invView);
        viewTransform.preTranslate(_viewMatrixTranslation);
    }
    
    setViewTransform(viewTransform);

    glTranslatef(_viewMatrixTranslation.x, _viewMatrixTranslation.y, _viewMatrixTranslation.z);

    //  Setup 3D lights (after the camera transform, so that they are positioned in world space)
    {
        PerformanceTimer perfTimer("lights");
        setupWorldLight();
    }

    // setup shadow matrices (again, after the camera transform)
    int shadowMatrixCount = 0;
    if (Menu::getInstance()->isOptionChecked(MenuOption::SimpleShadows)) {
        shadowMatrixCount = 1;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows)) {
        shadowMatrixCount = CASCADED_SHADOW_MATRIX_COUNT;
    }
    for (int i = shadowMatrixCount - 1; i >= 0; i--) {
        glActiveTexture(GL_TEXTURE0 + i);
        glTexGenfv(GL_S, GL_EYE_PLANE, (const GLfloat*)&_shadowMatrices[i][0]);
        glTexGenfv(GL_T, GL_EYE_PLANE, (const GLfloat*)&_shadowMatrices[i][1]);
        glTexGenfv(GL_R, GL_EYE_PLANE, (const GLfloat*)&_shadowMatrices[i][2]);
    }

    if (!selfAvatarOnly && Menu::getInstance()->isOptionChecked(MenuOption::Stars)) {
        PerformanceTimer perfTimer("stars");
        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
            "Application::displaySide() ... stars...");
        if (!_stars.isStarsLoaded()) {
            _stars.generate(STARFIELD_NUM_STARS, STARFIELD_SEED);
        }
        // should be the first rendering pass - w/o depth buffer / lighting

        // compute starfield alpha based on distance from atmosphere
        float alpha = 1.0f;
        if (Menu::getInstance()->isOptionChecked(MenuOption::Atmosphere)) {
            const EnvironmentData& closestData = _environment.getClosestData(theCamera.getPosition());
            float height = glm::distance(theCamera.getPosition(),
                closestData.getAtmosphereCenter(theCamera.getPosition()));
            if (height < closestData.getAtmosphereInnerRadius()) {
                alpha = 0.0f;

            } else if (height < closestData.getAtmosphereOuterRadius()) {
                alpha = (height - closestData.getAtmosphereInnerRadius()) /
                    (closestData.getAtmosphereOuterRadius() - closestData.getAtmosphereInnerRadius());
            }
        }

        // finally render the starfield
        _stars.render(theCamera.getFieldOfView(), theCamera.getAspectRatio(), theCamera.getNearClip(), alpha);
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::Wireframe)) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // draw the sky dome
    if (!selfAvatarOnly && Menu::getInstance()->isOptionChecked(MenuOption::Atmosphere)) {
        PerformanceTimer perfTimer("atmosphere");
        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
            "Application::displaySide() ... atmosphere...");
        _environment.renderAtmospheres(theCamera);
    }
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    
    DependencyManager::get<DeferredLightingEffect>()->prepare();

    if (!selfAvatarOnly) {
        // draw a red sphere
        float originSphereRadius = 0.05f;
        DependencyManager::get<GeometryCache>()->renderSphere(originSphereRadius, 15, 15, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        
        // also, metavoxels
        if (Menu::getInstance()->isOptionChecked(MenuOption::Metavoxels)) {
            PerformanceTimer perfTimer("metavoxels");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... metavoxels...");
            _metavoxels.render();
        }

        // render models...
        if (Menu::getInstance()->isOptionChecked(MenuOption::Entities)) {
            PerformanceTimer perfTimer("entities");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... entities...");
            _entities.render(RenderArgs::DEFAULT_RENDER_MODE, renderSide);
        }

        // render JS/scriptable overlays
        {
            PerformanceTimer perfTimer("3dOverlays");
            _overlays.renderWorld(false);
        }

        // render the ambient occlusion effect if enabled
        if (Menu::getInstance()->isOptionChecked(MenuOption::AmbientOcclusion)) {
            PerformanceTimer perfTimer("ambientOcclusion");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... AmbientOcclusion...");
            DependencyManager::get<AmbientOcclusionEffect>()->render();
        }
    }



    bool mirrorMode = (theCamera.getMode() == CAMERA_MODE_MIRROR);
    {
        PerformanceTimer perfTimer("avatars");
        _avatarManager.renderAvatars(mirrorMode ? Avatar::MIRROR_RENDER_MODE : Avatar::NORMAL_RENDER_MODE,
            false, selfAvatarOnly);   
    }


    {
        DependencyManager::get<DeferredLightingEffect>()->setAmbientLightMode(getRenderAmbientLight());

        PROFILE_RANGE("DeferredLighting"); 
        PerformanceTimer perfTimer("lighting");
        DependencyManager::get<DeferredLightingEffect>()->render();
    }

    {
        PerformanceTimer perfTimer("avatarsPostLighting");
        _avatarManager.renderAvatars(mirrorMode ? Avatar::MIRROR_RENDER_MODE : Avatar::NORMAL_RENDER_MODE,
            true, selfAvatarOnly);   
    }
    
    //Render the sixense lasers
    if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseLasers)) {
        _myAvatar->renderLaserPointers();
    }

    if (!selfAvatarOnly) {
        _nodeBoundsDisplay.draw();
    
        //  Render the world box
        if (theCamera.getMode() != CAMERA_MODE_MIRROR && Menu::getInstance()->isOptionChecked(MenuOption::Stats) && 
                Menu::getInstance()->isOptionChecked(MenuOption::UserInterface)) {
            PerformanceTimer perfTimer("worldBox");
            renderWorldBox();
        }

        // render octree fades if they exist
        if (_octreeFades.size() > 0) {
            PerformanceTimer perfTimer("octreeFades");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... octree fades...");
            _octreeFadesLock.lockForWrite();
            for(std::vector<OctreeFade>::iterator fade = _octreeFades.begin(); fade != _octreeFades.end();) {
                fade->render();
                if(fade->isDone()) {
                    fade = _octreeFades.erase(fade);
                } else {
                    ++fade;
                }
            }
            _octreeFadesLock.unlock();
        }

        // give external parties a change to hook in
        {
            PerformanceTimer perfTimer("inWorldInterface");
            emit renderingInWorldInterface();
        }
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::Wireframe)) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Render 3D overlays that should be drawn in front
    {
        PerformanceTimer perfTimer("3dOverlaysFront");
        glClear(GL_DEPTH_BUFFER_BIT);
        _overlays.renderWorld(true);
    }
}

void Application::updateUntranslatedViewMatrix(const glm::vec3& viewMatrixTranslation) {
    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&_untranslatedViewMatrix);
    _viewMatrixTranslation = viewMatrixTranslation;
}

void Application::setViewTransform(const Transform& view) {
    _viewTransform = view;
}

void Application::loadTranslatedViewMatrix(const glm::vec3& translation) {
    glLoadMatrixf((const GLfloat*)&_untranslatedViewMatrix);
    glTranslatef(translation.x + _viewMatrixTranslation.x, translation.y + _viewMatrixTranslation.y,
        translation.z + _viewMatrixTranslation.z);
}

void Application::getModelViewMatrix(glm::dmat4* modelViewMatrix) {
    (*modelViewMatrix) =_untranslatedViewMatrix;
    (*modelViewMatrix)[3] = _untranslatedViewMatrix * glm::vec4(_viewMatrixTranslation, 1);
}

void Application::getProjectionMatrix(glm::dmat4* projectionMatrix) {
    *projectionMatrix = _projectionMatrix;
}

void Application::computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
    float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const {

    // allow 3DTV/Oculus to override parameters from camera
    _displayViewFrustum.computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    if (OculusManager::isConnected()) {
        OculusManager::overrideOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    
    } else if (TV3DManager::isConnected()) {
        TV3DManager::overrideOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);    
    }
}

bool Application::getShadowsEnabled() {
    Menu* menubar = Menu::getInstance();
    return menubar->isOptionChecked(MenuOption::SimpleShadows) ||
           menubar->isOptionChecked(MenuOption::CascadedShadows);
}

bool Application::getCascadeShadowsEnabled() { 
    return Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows); 
}

glm::vec2 Application::getScaledScreenPoint(glm::vec2 projectedPoint) {
    auto glCanvas = DependencyManager::get<GLCanvas>();
    float horizontalScale = glCanvas->getDeviceWidth() / 2.0f;
    float verticalScale   = glCanvas->getDeviceHeight() / 2.0f;

    // -1,-1 is 0,windowHeight
    // 1,1 is windowWidth,0

    // -1,1                    1,1
    // +-----------------------+
    // |           |           |
    // |           |           |
    // | -1,0      |           |
    // |-----------+-----------|
    // |          0,0          |
    // |           |           |
    // |           |           |
    // |           |           |
    // +-----------------------+
    // -1,-1                   1,-1

    glm::vec2 screenPoint((projectedPoint.x + 1.0) * horizontalScale,
        ((projectedPoint.y + 1.0) * -verticalScale) + glCanvas->getDeviceHeight());

    return screenPoint;
}

void Application::renderRearViewMirror(const QRect& region, bool billboard) {
    // Grab current viewport to reset it at the end
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    bool eyeRelativeCamera = false;
    if (billboard) {
        _mirrorCamera.setFieldOfView(BILLBOARD_FIELD_OF_VIEW);  // degees
        _mirrorCamera.setPosition(_myAvatar->getPosition() +
                                  _myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * BILLBOARD_DISTANCE * _myAvatar->getScale());

    } else if (SettingHandles::rearViewZoomLevel.get() == BODY) {
        _mirrorCamera.setFieldOfView(MIRROR_FIELD_OF_VIEW);     // degrees
        _mirrorCamera.setPosition(_myAvatar->getChestPosition() +
                                  _myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_REARVIEW_BODY_DISTANCE * _myAvatar->getScale());

    } else { // HEAD zoom level
        _mirrorCamera.setFieldOfView(MIRROR_FIELD_OF_VIEW);     // degrees
        if (_myAvatar->getSkeletonModel().isActive() && _myAvatar->getHead()->getFaceModel().isActive()) {
            // as a hack until we have a better way of dealing with coordinate precision issues, reposition the
            // face/body so that the average eye position lies at the origin
            eyeRelativeCamera = true;
            _mirrorCamera.setPosition(_myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_REARVIEW_DISTANCE * _myAvatar->getScale());

        } else {
            _mirrorCamera.setPosition(_myAvatar->getHead()->getEyePosition() +
                                      _myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_REARVIEW_DISTANCE * _myAvatar->getScale());
        }
    }
    _mirrorCamera.setAspectRatio((float)region.width() / region.height());

    _mirrorCamera.setRotation(_myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI, 0.0f)));
    _mirrorCamera.update(1.0f/_fps);

    // set the bounds of rear mirror view
    if (billboard) {
        QSize size = DependencyManager::get<TextureCache>()->getFrameBufferSize();
        glViewport(region.x(), size.height() - region.y() - region.height(), region.width(), region.height());
        glScissor(region.x(), size.height() - region.y() - region.height(), region.width(), region.height());    
    } else {
        // if not rendering the billboard, the region is in device independent coordinates; must convert to device
        QSize size = DependencyManager::get<TextureCache>()->getFrameBufferSize();
        float ratio = QApplication::desktop()->windowHandle()->devicePixelRatio() * getRenderResolutionScale();
        int x = region.x() * ratio, y = region.y() * ratio, width = region.width() * ratio, height = region.height() * ratio;
        glViewport(x, size.height() - y - height, width, height);
        glScissor(x, size.height() - y - height, width, height);
    }
    bool updateViewFrustum = false;
    updateProjectionMatrix(_mirrorCamera, updateViewFrustum);
    glEnable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render rear mirror view
    glPushMatrix();
    if (eyeRelativeCamera) {
        // save absolute translations
        glm::vec3 absoluteSkeletonTranslation = _myAvatar->getSkeletonModel().getTranslation();
        glm::vec3 absoluteFaceTranslation = _myAvatar->getHead()->getFaceModel().getTranslation();

        // get the neck position so we can translate the face relative to it
        glm::vec3 neckPosition;
        _myAvatar->getSkeletonModel().setTranslation(glm::vec3());
        _myAvatar->getSkeletonModel().getNeckPosition(neckPosition);

        // get the eye position relative to the body
        glm::vec3 eyePosition = _myAvatar->getHead()->getEyePosition();
        float eyeHeight = eyePosition.y - _myAvatar->getPosition().y;

        // set the translation of the face relative to the neck position
        _myAvatar->getHead()->getFaceModel().setTranslation(neckPosition - glm::vec3(0, eyeHeight, 0));

        // translate the neck relative to the face
        _myAvatar->getSkeletonModel().setTranslation(_myAvatar->getHead()->getFaceModel().getTranslation() -
            neckPosition);

        // update the attachments to match
        QVector<glm::vec3> absoluteAttachmentTranslations;
        glm::vec3 delta = _myAvatar->getSkeletonModel().getTranslation() - absoluteSkeletonTranslation;
        foreach (Model* attachment, _myAvatar->getAttachmentModels()) {
            absoluteAttachmentTranslations.append(attachment->getTranslation());
            attachment->setTranslation(attachment->getTranslation() + delta);
        }

        // and lo, even the shadow matrices
        glm::mat4 savedShadowMatrices[CASCADED_SHADOW_MATRIX_COUNT];
        for (int i = 0; i < CASCADED_SHADOW_MATRIX_COUNT; i++) {
            savedShadowMatrices[i] = _shadowMatrices[i];
            _shadowMatrices[i] = glm::transpose(glm::transpose(_shadowMatrices[i]) * glm::translate(-delta));
        }

        displaySide(_mirrorCamera, true);

        // restore absolute translations
        _myAvatar->getSkeletonModel().setTranslation(absoluteSkeletonTranslation);
        _myAvatar->getHead()->getFaceModel().setTranslation(absoluteFaceTranslation);
        for (int i = 0; i < absoluteAttachmentTranslations.size(); i++) {
            _myAvatar->getAttachmentModels().at(i)->setTranslation(absoluteAttachmentTranslations.at(i));
        }
        
        // restore the shadow matrices
        for (int i = 0; i < CASCADED_SHADOW_MATRIX_COUNT; i++) {
            _shadowMatrices[i] = savedShadowMatrices[i];
        }
    } else {
        displaySide(_mirrorCamera, true);
    }
    glPopMatrix();

    if (!billboard) {
        _rearMirrorTools->render(false);
    }

    // reset Viewport and projection matrix
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_SCISSOR_TEST);
    updateProjectionMatrix(_myCamera, updateViewFrustum);
}

void Application::resetSensors() {
    DependencyManager::get<Faceshift>()->reset();
    DependencyManager::get<Visage>()->reset();
    DependencyManager::get<DdeFaceTracker>()->reset();

    OculusManager::reset();

    _prioVR.reset();
    //_leapmotion.reset();

    QScreen* currentScreen = _window->windowHandle()->screen();
    QWindow* mainWindow = _window->windowHandle();
    QPoint windowCenter = mainWindow->geometry().center();
    DependencyManager::get<GLCanvas>()->cursor().setPos(currentScreen, windowCenter);
    
    _myAvatar->reset();

    QMetaObject::invokeMethod(DependencyManager::get<Audio>().data(), "reset", Qt::QueuedConnection);
}

static void setShortcutsEnabled(QWidget* widget, bool enabled) {
    foreach (QAction* action, widget->actions()) {
        QKeySequence shortcut = action->shortcut();
        if (!shortcut.isEmpty() && (shortcut[0] & (Qt::CTRL | Qt::ALT | Qt::META)) == 0) {
            // it's a shortcut that may coincide with a "regular" key, so switch its context
            action->setShortcutContext(enabled ? Qt::WindowShortcut : Qt::WidgetShortcut);
        }
    }
    foreach (QObject* child, widget->children()) {
        if (child->isWidgetType()) {
            setShortcutsEnabled(static_cast<QWidget*>(child), enabled);
        }
    }
}

void Application::setMenuShortcutsEnabled(bool enabled) {
    setShortcutsEnabled(_window->menuBar(), enabled);
}

void Application::updateWindowTitle(){

    QString buildVersion = " (build " + applicationVersion() + ")";
    auto nodeList = DependencyManager::get<NodeList>();

    QString connectionStatus = nodeList->getDomainHandler().isConnected() ? "" : " (NOT CONNECTED) ";
    QString username = AccountManager::getInstance().getAccountInfo().getUsername();
    QString currentPlaceName = DependencyManager::get<AddressManager>()->getRootPlaceName();
    
    if (currentPlaceName.isEmpty()) {
        currentPlaceName = nodeList->getDomainHandler().getHostname();
    }
    
    QString title = QString() + (!username.isEmpty() ? username + " @ " : QString())
        + currentPlaceName + connectionStatus + buildVersion;

#ifndef WIN32
    // crashes with vs2013/win32
    qDebug("Application title set to: %s", title.toStdString().c_str());
#endif
    _window->setWindowTitle(title);
}

void Application::updateLocationInServer() {

    AccountManager& accountManager = AccountManager::getInstance();
    auto addressManager = DependencyManager::get<AddressManager>();
    DomainHandler& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
    
    if (accountManager.isLoggedIn() && domainHandler.isConnected()
        && (!addressManager->getRootPlaceID().isNull() || !domainHandler.getUUID().isNull())) {

        // construct a QJsonObject given the user's current address information
        QJsonObject rootObject;

        QJsonObject locationObject;
        
        QString pathString = addressManager->currentPath();
       
        const QString LOCATION_KEY_IN_ROOT = "location";
        
        const QString PATH_KEY_IN_LOCATION = "path";
        locationObject.insert(PATH_KEY_IN_LOCATION, pathString);
        
        if (!addressManager->getRootPlaceID().isNull()) {
            const QString PLACE_ID_KEY_IN_LOCATION = "place_id";
            locationObject.insert(PLACE_ID_KEY_IN_LOCATION,
                                  uuidStringWithoutCurlyBraces(addressManager->getRootPlaceID()));
            
        } else {
            const QString DOMAIN_ID_KEY_IN_LOCATION = "domain_id";
            locationObject.insert(DOMAIN_ID_KEY_IN_LOCATION,
                                  uuidStringWithoutCurlyBraces(domainHandler.getUUID()));
        }

        rootObject.insert(LOCATION_KEY_IN_ROOT, locationObject);

        accountManager.authenticatedRequest("/api/v1/user/location", QNetworkAccessManager::PutOperation,
                                            JSONCallbackParameters(), QJsonDocument(rootObject).toJson());
     }
}

void Application::clearDomainOctreeDetails() {
    qDebug() << "Clearing domain octree details...";
    // reset the environment so that we don't erroneously end up with multiple
    _environment.resetToDefault();

    // reset our node to stats and node to jurisdiction maps... since these must be changing...
    _entityServerJurisdictions.lockForWrite();
    _entityServerJurisdictions.clear();
    _entityServerJurisdictions.unlock();

    _octreeSceneStatsLock.lockForWrite();
    _octreeServerSceneStats.clear();
    _octreeSceneStatsLock.unlock();

    // reset the model renderer
    _entities.clear();

}

void Application::domainChanged(const QString& domainHostname) {
    updateWindowTitle();
    clearDomainOctreeDetails();
}

void Application::connectedToDomain(const QString& hostname) {
    AccountManager& accountManager = AccountManager::getInstance();
    const QUuid& domainID = DependencyManager::get<NodeList>()->getDomainHandler().getUUID();
    
    if (accountManager.isLoggedIn() && !domainID.isNull()) {
        // update our data-server with the domain-server we're logged in with

        QString domainPutJsonString = "{\"location\":{\"domain_id\":\"" + uuidStringWithoutCurlyBraces(domainID) + "\"}}";

        accountManager.authenticatedRequest("/api/v1/user/location", QNetworkAccessManager::PutOperation,
                                            JSONCallbackParameters(), domainPutJsonString.toUtf8());
    }
}

void Application::nodeAdded(SharedNodePointer node) {
    if (node->getType() == NodeType::AvatarMixer) {
        // new avatar mixer, send off our identity packet right away
        _myAvatar->sendIdentityPacket();
    }
}

void Application::nodeKilled(SharedNodePointer node) {

    // These are here because connecting NodeList::nodeKilled to OctreePacketProcessor::nodeKilled doesn't work:
    // OctreePacketProcessor::nodeKilled is not being called when NodeList::nodeKilled is emitted.
    // This may have to do with GenericThread::threadRoutine() blocking the QThread event loop

    _octreeProcessor.nodeKilled(node);

    _entityEditSender.nodeKilled(node);

    if (node->getType() == NodeType::AudioMixer) {
        QMetaObject::invokeMethod(DependencyManager::get<Audio>().data(), "audioMixerKilled");
    }

    if (node->getType() == NodeType::EntityServer) {

        QUuid nodeUUID = node->getUUID();
        // see if this is the first we've heard of this node...
        _entityServerJurisdictions.lockForRead();
        if (_entityServerJurisdictions.find(nodeUUID) != _entityServerJurisdictions.end()) {
            unsigned char* rootCode = _entityServerJurisdictions[nodeUUID].getRootOctalCode();
            VoxelPositionSize rootDetails;
            voxelDetailsForCode(rootCode, rootDetails);
            _entityServerJurisdictions.unlock();

            qDebug("model server going away...... v[%f, %f, %f, %f]",
                rootDetails.x, rootDetails.y, rootDetails.z, rootDetails.s);

            // Add the jurisditionDetails object to the list of "fade outs"
            if (!Menu::getInstance()->isOptionChecked(MenuOption::DontFadeOnOctreeServerChanges)) {
                OctreeFade fade(OctreeFade::FADE_OUT, NODE_KILLED_RED, NODE_KILLED_GREEN, NODE_KILLED_BLUE);
                fade.voxelDetails = rootDetails;
                const float slightly_smaller = 0.99f;
                fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
                _octreeFadesLock.lockForWrite();
                _octreeFades.push_back(fade);
                _octreeFadesLock.unlock();
            }

            // If the model server is going away, remove it from our jurisdiction map so we don't send voxels to a dead server
            _entityServerJurisdictions.lockForWrite();
            _entityServerJurisdictions.erase(_entityServerJurisdictions.find(nodeUUID));
        }
        _entityServerJurisdictions.unlock();

        // also clean up scene stats for that server
        _octreeSceneStatsLock.lockForWrite();
        if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
            _octreeServerSceneStats.erase(nodeUUID);
        }
        _octreeSceneStatsLock.unlock();

    } else if (node->getType() == NodeType::AvatarMixer) {
        // our avatar mixer has gone away - clear the hash of avatars
        _avatarManager.clearOtherAvatars();
    }
}

void Application::trackIncomingOctreePacket(const QByteArray& packet, const SharedNodePointer& sendingNode, bool wasStatsPacket) {

    // Attempt to identify the sender from it's address.
    if (sendingNode) {
        QUuid nodeUUID = sendingNode->getUUID();

        // now that we know the node ID, let's add these stats to the stats for that node...
        _octreeSceneStatsLock.lockForWrite();
        if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
            OctreeSceneStats& stats = _octreeServerSceneStats[nodeUUID];
            stats.trackIncomingOctreePacket(packet, wasStatsPacket, sendingNode->getClockSkewUsec());
        }
        _octreeSceneStatsLock.unlock();
    }
}

int Application::parseOctreeStats(const QByteArray& packet, const SharedNodePointer& sendingNode) {
    // But, also identify the sender, and keep track of the contained jurisdiction root for this server

    // parse the incoming stats datas stick it in a temporary object for now, while we
    // determine which server it belongs to
    OctreeSceneStats temp;
    int statsMessageLength = temp.unpackFromMessage(reinterpret_cast<const unsigned char*>(packet.data()), packet.size());

    // quick fix for crash... why would voxelServer be NULL?
    if (sendingNode) {
        QUuid nodeUUID = sendingNode->getUUID();

        // now that we know the node ID, let's add these stats to the stats for that node...
        _octreeSceneStatsLock.lockForWrite();
        if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
            _octreeServerSceneStats[nodeUUID].unpackFromMessage(reinterpret_cast<const unsigned char*>(packet.data()),
                                                                packet.size());
        } else {
            _octreeServerSceneStats[nodeUUID] = temp;
        }
        _octreeSceneStatsLock.unlock();

        VoxelPositionSize rootDetails;
        voxelDetailsForCode(temp.getJurisdictionRoot(), rootDetails);

        // see if this is the first we've heard of this node...
        NodeToJurisdictionMap* jurisdiction = NULL;
        QString serverType;
        if (sendingNode->getType() == NodeType::EntityServer) {
            jurisdiction = &_entityServerJurisdictions;
            serverType = "Entity";
        }

        jurisdiction->lockForRead();
        if (jurisdiction->find(nodeUUID) == jurisdiction->end()) {
            jurisdiction->unlock();

            qDebug("stats from new %s server... [%f, %f, %f, %f]",
                qPrintable(serverType), rootDetails.x, rootDetails.y, rootDetails.z, rootDetails.s);

            // Add the jurisditionDetails object to the list of "fade outs"
            if (!Menu::getInstance()->isOptionChecked(MenuOption::DontFadeOnOctreeServerChanges)) {
                OctreeFade fade(OctreeFade::FADE_OUT, NODE_ADDED_RED, NODE_ADDED_GREEN, NODE_ADDED_BLUE);
                fade.voxelDetails = rootDetails;
                const float slightly_smaller = 0.99f;
                fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
                _octreeFadesLock.lockForWrite();
                _octreeFades.push_back(fade);
                _octreeFadesLock.unlock();
            }
        } else {
            jurisdiction->unlock();
        }
        // store jurisdiction details for later use
        // This is bit of fiddling is because JurisdictionMap assumes it is the owner of the values used to construct it
        // but OctreeSceneStats thinks it's just returning a reference to it's contents. So we need to make a copy of the
        // details from the OctreeSceneStats to construct the JurisdictionMap
        JurisdictionMap jurisdictionMap;
        jurisdictionMap.copyContents(temp.getJurisdictionRoot(), temp.getJurisdictionEndNodes());
        jurisdiction->lockForWrite();
        (*jurisdiction)[nodeUUID] = jurisdictionMap;
        jurisdiction->unlock();
    }
    return statsMessageLength;
}

void Application::packetSent(quint64 length) {
    _bandwidthMeter.outputStream(BandwidthMeter::OCTREE).updateValue(length);
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
    scriptEngine->getEntityScriptingInterface()->setPacketSender(&_entityEditSender);
    scriptEngine->getEntityScriptingInterface()->setEntityTree(_entities.getTree());

    // AvatarManager has some custom types
    AvatarManager::registerMetaTypes(scriptEngine);

    // hook our avatar and avatar hash map object into this script engine
    scriptEngine->setAvatarData(_myAvatar, "MyAvatar"); // leave it as a MyAvatar class to expose thrust features
    scriptEngine->setAvatarHashMap(&_avatarManager, "AvatarList");

    scriptEngine->registerGlobalObject("Camera", &_myCamera);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    scriptEngine->registerGlobalObject("SpeechRecognizer", DependencyManager::get<SpeechRecognizer>().data());
#endif

    ClipboardScriptingInterface* clipboardScriptable = new ClipboardScriptingInterface();
    scriptEngine->registerGlobalObject("Clipboard", clipboardScriptable);
    connect(scriptEngine, SIGNAL(finished(const QString&)), clipboardScriptable, SLOT(deleteLater()));

    connect(scriptEngine, SIGNAL(finished(const QString&)), this, SLOT(scriptFinished(const QString&)));

    connect(scriptEngine, SIGNAL(loadScript(const QString&, bool)), this, SLOT(loadScript(const QString&, bool)));

    scriptEngine->registerGlobalObject("Overlays", &_overlays);
    qScriptRegisterMetaType(scriptEngine, OverlayPropertyResultToScriptValue, OverlayPropertyResultFromScriptValue);
    qScriptRegisterMetaType(scriptEngine, RayToOverlayIntersectionResultToScriptValue, 
                            RayToOverlayIntersectionResultFromScriptValue);

    QScriptValue windowValue = scriptEngine->registerGlobalObject("Window", WindowScriptingInterface::getInstance());
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                                       LocationScriptingInterface::locationSetter, windowValue);
    // register `location` on the global object.
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                                       LocationScriptingInterface::locationSetter);

    scriptEngine->registerFunction("WebWindow", WebWindowClass::constructor, 1);
    
    scriptEngine->registerGlobalObject("Menu", MenuScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("Settings", SettingsScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("AudioDevice", AudioDeviceScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("AnimationCache", DependencyManager::get<AnimationCache>().data());
    scriptEngine->registerGlobalObject("SoundCache", &SoundCache::getInstance());
    scriptEngine->registerGlobalObject("Account", AccountScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("Metavoxels", &_metavoxels);

    scriptEngine->registerGlobalObject("GlobalServices", GlobalServicesScriptingInterface::getInstance());
    qScriptRegisterMetaType(scriptEngine, DownloadInfoResultToScriptValue, DownloadInfoResultFromScriptValue);

    scriptEngine->registerGlobalObject("AvatarManager", &_avatarManager);
    
    scriptEngine->registerGlobalObject("Joysticks", &JoystickScriptingInterface::getInstance());
    qScriptRegisterMetaType(scriptEngine, joystickToScriptValue, joystickFromScriptValue);

    scriptEngine->registerGlobalObject("UndoStack", &_undoStackScriptingInterface);

#ifdef HAVE_RTMIDI
    scriptEngine->registerGlobalObject("MIDI", &MIDIManager::getInstance());
#endif

    QThread* workerThread = new QThread(this);

    // when the worker thread is started, call our engine's run..
    connect(workerThread, &QThread::started, scriptEngine, &ScriptEngine::run);

    // when the thread is terminated, add both scriptEngine and thread to the deleteLater queue
    connect(scriptEngine, SIGNAL(finished(const QString&)), scriptEngine, SLOT(deleteLater()));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));

    // when the application is about to quit, stop our script engine so it unwinds properly
    connect(this, SIGNAL(aboutToQuit()), scriptEngine, SLOT(stop()));

    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &NodeList::nodeKilled, scriptEngine, &ScriptEngine::nodeKilled);

    scriptEngine->moveToThread(workerThread);

    // Starts an event loop, and emits workerThread->started()
    workerThread->start();
}

ScriptEngine* Application::loadScript(const QString& scriptFilename, bool isUserLoaded,
    bool loadScriptFromEditor, bool activateMainWindow) {
    QUrl scriptUrl(scriptFilename);
    const QString& scriptURLString = scriptUrl.toString();
    if (_scriptEnginesHash.contains(scriptURLString) && loadScriptFromEditor
        && !_scriptEnginesHash[scriptURLString]->isFinished()) {

        return _scriptEnginesHash[scriptURLString];
    }

    ScriptEngine* scriptEngine = new ScriptEngine(NO_SCRIPT, "", &_controllerScriptingInterface);
    scriptEngine->setUserLoaded(isUserLoaded);
    
    if (scriptFilename.isNull()) {
        // this had better be the script editor (we should de-couple so somebody who thinks they are loading a script
        // doesn't just get an empty script engine)
        
        // we can complete setup now since there isn't a script we have to load
        registerScriptEngineWithApplicationServices(scriptEngine);
    } else {
        // connect to the appropriate signals of this script engine
        connect(scriptEngine, &ScriptEngine::scriptLoaded, this, &Application::handleScriptEngineLoaded);
        connect(scriptEngine, &ScriptEngine::errorLoadingScript, this, &Application::handleScriptLoadError);
        
        // get the script engine object to load the script at the designated script URL
        scriptEngine->loadURL(scriptUrl);
    }

    // restore the main window's active state
    if (activateMainWindow && !loadScriptFromEditor) {
        _window->activateWindow();
    }

    return scriptEngine;
}

void Application::handleScriptEngineLoaded(const QString& scriptFilename) {
    ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(sender());
    
    _scriptEnginesHash.insertMulti(scriptFilename, scriptEngine);
    _runningScriptsWidget->setRunningScripts(getRunningScripts());
    UserActivityLogger::getInstance().loadedScript(scriptFilename);
    
    // register our application services and set it off on its own thread
    registerScriptEngineWithApplicationServices(scriptEngine);
}

void Application::handleScriptLoadError(const QString& scriptFilename) {
    qDebug() << "Application::loadScript(), script failed to load...";
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
    // stops all current running scripts
    for (QHash<QString, ScriptEngine*>::const_iterator it = _scriptEnginesHash.constBegin();
            it != _scriptEnginesHash.constEnd(); it++) {
        if (restart && it.value()->isUserLoaded()) {
            connect(it.value(), SIGNAL(finished(const QString&)), SLOT(loadScript(const QString&)));
        }
        it.value()->stop();
        qDebug() << "stopping script..." << it.key();
    }
    // HACK: ATM scripts cannot set/get their animation priorities, so we clear priorities
    // whenever a script stops in case it happened to have been setting joint rotations.
    // TODO: expose animation priorities and provide a layered animation control system.
    _myAvatar->clearScriptableSettings();
}

void Application::stopScript(const QString &scriptName) {
    const QString& scriptURLString = QUrl(scriptName).toString();
    if (_scriptEnginesHash.contains(scriptURLString)) {
        _scriptEnginesHash.value(scriptURLString)->stop();
        qDebug() << "stopping script..." << scriptName;
        // HACK: ATM scripts cannot set/get their animation priorities, so we clear priorities
        // whenever a script stops in case it happened to have been setting joint rotations.
        // TODO: expose animation priorities and provide a layered animation control system.
        _myAvatar->clearJointAnimationPriorities();
    }
    if (_scriptEnginesHash.empty()) {
        _myAvatar->clearScriptableSettings();
    }
}

void Application::reloadAllScripts() {
    stopAllScripts(true);
}

void Application::loadDefaultScripts() {
    if (!_scriptEnginesHash.contains(DEFAULT_SCRIPTS_JS_URL)) {
        loadScript(DEFAULT_SCRIPTS_JS_URL);
    }
}

void Application::manageRunningScriptsWidgetVisibility(bool shown) {
    if (_runningScriptsWidgetWasVisible && shown) {
        _runningScriptsWidget->show();
    } else if (_runningScriptsWidgetWasVisible && !shown) {
        _runningScriptsWidget->hide();
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

void Application::uploadHead() {
    ModelUploader::uploadHead();
}

void Application::uploadSkeleton() {
    ModelUploader::uploadSkeleton();
}

void Application::uploadAttachment() {
    ModelUploader::uploadAttachment();
}

void Application::uploadEntity() {
    ModelUploader::uploadEntity();
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

void Application::updateMyAvatarTransform() {
    const float SIMULATION_OFFSET_QUANTIZATION = 16.0f; // meters
    glm::vec3 avatarPosition = _myAvatar->getPosition();
    glm::vec3 physicsWorldOffset = _physicsEngine.getOriginOffset();
    if (glm::distance(avatarPosition, physicsWorldOffset) > SIMULATION_OFFSET_QUANTIZATION) {
        glm::vec3 newOriginOffset = avatarPosition;
        int halfExtent = (int)HALF_SIMULATION_EXTENT;
        for (int i = 0; i < 3; ++i) {
            newOriginOffset[i] = (float)(glm::max(halfExtent, 
                    ((int)(avatarPosition[i] / SIMULATION_OFFSET_QUANTIZATION)) * (int)SIMULATION_OFFSET_QUANTIZATION));
        }
        // TODO: Andrew to replace this with method that actually moves existing object positions in PhysicsEngine
        _physicsEngine.setOriginOffset(newOriginOffset);
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
    
    qDebug() << "Voxel costs are" << satoshisPerVoxel << "per voxel and" << satoshisPerMeterCubed << "per meter cubed";
    qDebug() << "Destination wallet UUID for voxel payments is" << voxelWalletUUID;
}

QString Application::getPreviousScriptLocation() {
    QString suggestedName;
    if (_previousScriptLocation.isEmpty()) {
        QString desktopLocation = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
// Temporary fix to Qt bug: http://stackoverflow.com/questions/16194475
#ifdef __APPLE__
        suggestedName = desktopLocation.append("/script.js");
#endif
    } else {
        suggestedName = _previousScriptLocation;
    }
    return suggestedName;
}

void Application::setPreviousScriptLocation(const QString& previousScriptLocation) {
    _previousScriptLocation = previousScriptLocation;
    SettingHandles::lastScriptLocation.set(_previousScriptLocation);
}

void Application::loadDialog() {

    QString fileNameString = QFileDialog::getOpenFileName(DependencyManager::get<GLCanvas>().data(),
                                                          tr("Open Script"),
                                                          getPreviousScriptLocation(),
                                                          tr("JavaScript Files (*.js)"));
    if (!fileNameString.isEmpty()) {
        setPreviousScriptLocation(fileNameString);
        loadScript(fileNameString);
    }
}

void Application::loadScriptURLDialog() {
    QInputDialog scriptURLDialog(Application::getInstance()->getWindow());
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

QString Application::getScriptsLocation() const {
    return SettingHandles::scriptsLocation.get();
}

void Application::setScriptsLocation(const QString& scriptsLocation) {
    SettingHandles::scriptsLocation.set(scriptsLocation);
    emit scriptLocationChanged(scriptsLocation);
}

void Application::toggleLogDialog() {
    if (! _logDialog) {
        _logDialog = new LogDialog(DependencyManager::get<GLCanvas>().data(), getLogger());
    }

    if (_logDialog->isVisible()) {
        _logDialog->hide();
    } else {
        _logDialog->show();
    }
}

void Application::initAvatarAndViewFrustum() {
    updateMyAvatar(0.0f);
}

void Application::checkVersion() {
    QNetworkRequest latestVersionRequest((QUrl(CHECK_VERSION_URL)));
    latestVersionRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    QNetworkReply* reply = NetworkAccessManager::getInstance().get(latestVersionRequest);
    connect(reply, SIGNAL(finished()), SLOT(parseVersionXml()));
}

void Application::parseVersionXml() {

    #ifdef Q_OS_WIN32
    QString operatingSystem("win");
    #endif

    #ifdef Q_OS_MAC
    QString operatingSystem("mac");
    #endif

    #ifdef Q_OS_LINUX
    QString operatingSystem("ubuntu");
    #endif

    QString latestVersion;
    QUrl downloadUrl;
    QString releaseNotes("Unavailable");
    QNetworkReply* sender = qobject_cast<QNetworkReply*>(QObject::sender());

    QXmlStreamReader xml(sender);

    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == operatingSystem) {
            while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == operatingSystem)) {
                if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name().toString() == "version") {
                    xml.readNext();
                    latestVersion = xml.text().toString();
                }
                if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name().toString() == "url") {
                    xml.readNext();
                    downloadUrl = QUrl(xml.text().toString());
                }
                xml.readNext();
            }
        }
        xml.readNext();
    }

    if (!shouldSkipVersion(latestVersion) && applicationVersion() != latestVersion) {
        new UpdateDialog(DependencyManager::get<GLCanvas>().data(), releaseNotes, latestVersion, downloadUrl);
    }
    sender->deleteLater();
}

bool Application::shouldSkipVersion(QString latestVersion) {
    QFile skipFile(SKIP_FILENAME);
    skipFile.open(QIODevice::ReadWrite);
    QString skipVersion(skipFile.readAll());
    return (skipVersion == latestVersion || applicationVersion() == "dev");
}

void Application::skipVersion(QString latestVersion) {
    QFile skipFile(SKIP_FILENAME);
    skipFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    skipFile.seek(0);
    skipFile.write(latestVersion.toStdString().c_str());
}

void Application::takeSnapshot() {
    QMediaPlayer* player = new QMediaPlayer();
    QFileInfo inf = QFileInfo(PathUtils::resourcesPath() + "sounds/snap.wav");
    player->setMedia(QUrl::fromLocalFile(inf.absoluteFilePath()));
    player->play();

    QString fileName = Snapshot::saveSnapshot();

    AccountManager& accountManager = AccountManager::getInstance();
    if (!accountManager.isLoggedIn()) {
        return;
    }

    if (!_snapshotShareDialog) {
        _snapshotShareDialog = new SnapshotShareDialog(fileName, DependencyManager::get<GLCanvas>().data());
    }
    _snapshotShareDialog->show();
}

void Application::setVSyncEnabled() {
    bool vsyncOn = Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerateVSyncOn);
#if defined(Q_OS_WIN)
    if (wglewGetExtension("WGL_EXT_swap_control")) {
        wglSwapIntervalEXT(vsyncOn);
        int swapInterval = wglGetSwapIntervalEXT();
        qDebug("V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    } else {
        qDebug("V-Sync is FORCED ON on this system\n");
    }
#elif defined(Q_OS_LINUX)
    // TODO: write the poper code for linux
    /*
    if (glQueryExtension.... ("GLX_EXT_swap_control")) {
        glxSwapIntervalEXT(vsyncOn);
        int swapInterval = xglGetSwapIntervalEXT();
        _isVSyncOn = swapInterval;
        qDebug("V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    } else {
    qDebug("V-Sync is FORCED ON on this system\n");
    }
    */
#else
    qDebug("V-Sync is FORCED ON on this system\n");
#endif
    vsyncOn = true; // Turns off unused variable warning
}

bool Application::isVSyncOn() const {
#if defined(Q_OS_WIN)
    if (wglewGetExtension("WGL_EXT_swap_control")) {
        int swapInterval = wglGetSwapIntervalEXT();
        return (swapInterval > 0);
    }
#elif defined(Q_OS_LINUX)
    // TODO: write the poper code for linux
    /*
    if (glQueryExtension.... ("GLX_EXT_swap_control")) {
        int swapInterval = xglGetSwapIntervalEXT();
        return (swapInterval > 0);
    } else {
        return true;
    }
    */    
#endif
    return true;
}

bool Application::isVSyncEditable() const {
#if defined(Q_OS_WIN)
    if (wglewGetExtension("WGL_EXT_swap_control")) {
        return true;
    }
#elif defined(Q_OS_LINUX)
    // TODO: write the poper code for linux
    /*
    if (glQueryExtension.... ("GLX_EXT_swap_control")) {
        return true;
    }
    */
#endif
    return false;
}

unsigned int Application::getRenderTargetFramerate() const {
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerateUnlimited)) {
        return 0;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerate60)) {
        return 60;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerate50)) {
        return 50;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerate40)) {
        return 40;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerate30)) {
        return 30;
    }
    return 0;
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
