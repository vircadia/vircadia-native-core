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
#include <QSettings>
#include <QShortcut>
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
#include <AudioInjector.h>
#include <EntityScriptingInterface.h>
#include <LocalVoxelsList.h>
#include <Logging.h>
#include <NetworkAccessManager.h>
#include <OctalCode.h>
#include <OctreeSceneStats.h>
#include <PacketHeaders.h>
#include <ParticlesScriptingInterface.h>
#include <PerfStat.h>
#include <ResourceCache.h>
#include <UserActivityLogger.h>
#include <UUID.h>

#include "Application.h"
#include "InterfaceVersion.h"
#include "Menu.h"
#include "ModelUploader.h"
#include "PaymentManager.h"
#include "Util.h"
#include "devices/MIDIManager.h"
#include "devices/OculusManager.h"
#include "devices/TV3DManager.h"
#include "renderer/ProgramObject.h"

#include "scripting/AccountScriptingInterface.h"
#include "scripting/AudioDeviceScriptingInterface.h"
#include "scripting/ClipboardScriptingInterface.h"
#include "scripting/LocationScriptingInterface.h"
#include "scripting/MenuScriptingInterface.h"
#include "scripting/SettingsScriptingInterface.h"
#include "scripting/WindowScriptingInterface.h"

#include "ui/InfoView.h"
#include "ui/OAuthWebViewHandler.h"
#include "ui/Snapshot.h"
#include "ui/Stats.h"
#include "ui/TextRenderer.h"

#include "devices/Leapmotion.h"

using namespace std;

//  Starfield information
static unsigned STARFIELD_NUM_STARS = 50000;
static unsigned STARFIELD_SEED = 1;

static const int BANDWIDTH_METER_CLICK_MAX_DRAG_LENGTH = 6; // farther dragged clicks are ignored

const int IDLE_SIMULATE_MSECS = 16;              //  How often should call simulate and other stuff
                                                 //  in the idle loop?  (60 FPS is default)
static QTimer* idleTimer = NULL;

const QString CHECK_VERSION_URL = "https://highfidelity.io/latestVersion.xml";
const QString SKIP_FILENAME = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/hifi.skipversion";

const QString DEFAULT_SCRIPTS_JS_URL = "http://public.highfidelity.io/scripts/defaultScripts.js";

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (message.size() > 0) {
        QString dateString = QDateTime::currentDateTime().toTimeSpec(Qt::LocalTime).toString(Qt::ISODate);
        QString formattedMessage = QString("[%1] %2\n").arg(dateString).arg(message);

        fprintf(stdout, "%s", qPrintable(formattedMessage));
        Application::getInstance()->getLogger()->addMessage(qPrintable(formattedMessage));
    }
}

QString& Application::resourcesPath() {
#ifdef Q_OS_MAC
    static QString staticResourcePath = QCoreApplication::applicationDirPath() + "/../Resources/";
#else
    static QString staticResourcePath = QCoreApplication::applicationDirPath() + "/resources/";
#endif
    return staticResourcePath;
}

Application::Application(int& argc, char** argv, QElapsedTimer &startup_time) :
        QApplication(argc, argv),
        _window(new MainWindow(desktop())),
        _glWidget(new GLCanvas()),
        _nodeThread(new QThread(this)),
        _datagramProcessor(),
        _frameCount(0),
        _fps(60.0f),
        _justStarted(true),
        _voxelImportDialog(NULL),
        _voxelImporter(),
        _importSucceded(false),
        _sharedVoxelSystem(TREE_SCALE, DEFAULT_MAX_VOXELS_PER_SYSTEM, &_clipboard),
        _entityClipboardRenderer(),
        _entityClipboard(),
        _wantToKillLocalVoxels(false),
        _viewFrustum(),
        _lastQueriedViewFrustum(),
        _lastQueriedTime(usecTimestampNow()),
        _mirrorViewRect(QRect(MIRROR_VIEW_LEFT_PADDING, MIRROR_VIEW_TOP_PADDING, MIRROR_VIEW_WIDTH, MIRROR_VIEW_HEIGHT)),
        _cameraPushback(0.0f),
        _scaleMirror(1.0f),
        _rotateMirror(0.0f),
        _raiseMirror(0.0f),
        _mouseX(0),
        _mouseY(0),
        _lastMouseMove(usecTimestampNow()),
        _lastMouseMoveWasSimulated(false),
        _mouseHidden(false),
        _seenMouseMove(false),
        _touchAvgX(0.0f),
        _touchAvgY(0.0f),
        _isTouchPressed(false),
        _mousePressed(false),
        _audio(),
        _enableProcessVoxelsThread(true),
        _octreeProcessor(),
        _voxelHideShowThread(&_voxels),
        _packetsPerSecond(0),
        _bytesPerSecond(0),
        _nodeBoundsDisplay(this),
        _previousScriptLocation(),
        _applicationOverlay(),
        _runningScriptsWidget(NULL),
        _runningScriptsWidgetWasVisible(false),
        _trayIcon(new QSystemTrayIcon(_window)),
        _lastNackTime(usecTimestampNow()),
        _lastSendDownstreamAudioStats(usecTimestampNow())
{

    // read the ApplicationInfo.ini file for Name/Version/Domain information
    QSettings applicationInfo(Application::resourcesPath() + "info/ApplicationInfo.ini", QSettings::IniFormat);

    // set the associated application properties
    applicationInfo.beginGroup("INFO");

    qDebug() << "[VERSION] Build sequence: " << qPrintable(applicationVersion());

    setApplicationName(applicationInfo.value("name").toString());
    setApplicationVersion(BUILD_VERSION);
    setOrganizationName(applicationInfo.value("organizationName").toString());
    setOrganizationDomain(applicationInfo.value("organizationDomain").toString());

    _logger = new FileLogger(this);  // After setting organization name in order to get correct directory

    QSettings::setDefaultFormat(QSettings::IniFormat);

    _myAvatar = _avatarManager.getMyAvatar();

    _applicationStartupTime = startup_time;

    QFontDatabase::addApplicationFont(Application::resourcesPath() + "styles/Inconsolata.otf");
    _window->setWindowTitle("Interface");

    qInstallMessageHandler(messageHandler);

    // call Menu getInstance static method to set up the menu
    _window->setMenuBar(Menu::getInstance());

    _runningScriptsWidget = new RunningScriptsWidget(_window);

    unsigned int listenPort = 0; // bind to an ephemeral port by default
    const char** constArgv = const_cast<const char**>(argv);
    const char* portStr = getCmdOption(argc, constArgv, "--listenPort");
    if (portStr) {
        listenPort = atoi(portStr);
    }

    // start the nodeThread so its event loop is running
    _nodeThread->start();

    // make sure the node thread is given highest priority
    _nodeThread->setPriority(QThread::TimeCriticalPriority);

    // put the NodeList and datagram processing on the node thread
    NodeList* nodeList = NodeList::createInstance(NodeType::Agent, listenPort);

    nodeList->moveToThread(_nodeThread);
    _datagramProcessor.moveToThread(_nodeThread);

    // connect the DataProcessor processDatagrams slot to the QUDPSocket readyRead() signal
    connect(&nodeList->getNodeSocket(), SIGNAL(readyRead()), &_datagramProcessor, SLOT(processDatagrams()));

    // put the audio processing on a separate thread
    QThread* audioThread = new QThread(this);

    _audio.moveToThread(audioThread);
    connect(audioThread, SIGNAL(started()), &_audio, SLOT(start()));

    audioThread->start();
    
    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    connect(&domainHandler, SIGNAL(hostnameChanged(const QString&)), SLOT(domainChanged(const QString&)));
    connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(connectedToDomain(const QString&)));
    connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(updateWindowTitle()));
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &Application::domainSettingsReceived);
    connect(&domainHandler, &DomainHandler::hostnameChanged, Menu::getInstance(), &Menu::clearLoginDialogDisplayedFlag);

    // hookup VoxelEditSender to PaymentManager so we can pay for octree edits
    const PaymentManager& paymentManager = PaymentManager::getInstance();
    connect(&_voxelEditSender, &VoxelEditPacketSender::octreePaymentRequired,
            &paymentManager, &PaymentManager::sendSignedPayment);

    // update our location every 5 seconds in the data-server, assuming that we are authenticated with one
    const qint64 DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS = 5 * 1000;

    QTimer* locationUpdateTimer = new QTimer(this);
    connect(locationUpdateTimer, &QTimer::timeout, this, &Application::updateLocationInServer);
    locationUpdateTimer->start(DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS);

    connect(nodeList, &NodeList::nodeAdded, this, &Application::nodeAdded);
    connect(nodeList, &NodeList::nodeKilled, this, &Application::nodeKilled);
    connect(nodeList, SIGNAL(nodeKilled(SharedNodePointer)), SLOT(nodeKilled(SharedNodePointer)));
    connect(nodeList, SIGNAL(nodeAdded(SharedNodePointer)), &_voxels, SLOT(nodeAdded(SharedNodePointer)));
    connect(nodeList, SIGNAL(nodeKilled(SharedNodePointer)), &_voxels, SLOT(nodeKilled(SharedNodePointer)));
    connect(nodeList, &NodeList::uuidChanged, _myAvatar, &MyAvatar::setSessionUUID);
    connect(nodeList, &NodeList::limitOfSilentDomainCheckInsReached, nodeList, &NodeList::reset);

    // connect to appropriate slots on AccountManager
    AccountManager& accountManager = AccountManager::getInstance();

    const qint64 BALANCE_UPDATE_INTERVAL_MSECS = 5 * 1000;

    QTimer* balanceUpdateTimer = new QTimer(this);
    connect(balanceUpdateTimer, &QTimer::timeout, &accountManager, &AccountManager::updateBalance);
    balanceUpdateTimer->start(BALANCE_UPDATE_INTERVAL_MSECS);

    connect(&accountManager, &AccountManager::balanceChanged, this, &Application::updateWindowTitle);

    connect(&accountManager, &AccountManager::authRequired, Menu::getInstance(), &Menu::loginForCurrentDomain);
    connect(&accountManager, &AccountManager::usernameChanged, this, &Application::updateWindowTitle);

    // set the account manager's root URL and trigger a login request if we don't have the access token
    accountManager.setAuthURL(DEFAULT_NODE_AUTH_URL);
    UserActivityLogger::getInstance().launch(applicationVersion());

    // once the event loop has started, check and signal for an access token
    QMetaObject::invokeMethod(&accountManager, "checkAndSignalForAccessToken", Qt::QueuedConnection);

    _settings = new QSettings(this);
    _numChangedSettings = 0;

    // Check to see if the user passed in a command line option for loading a local
    // Voxel File.
    _voxelsFilename = getCmdOption(argc, constArgv, "-i");

    #ifdef _WIN32
    WSADATA WsaData;
    int wsaresult = WSAStartup(MAKEWORD(2,2), &WsaData);
    #endif

    // tell the NodeList instance who to tell the domain server we care about
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
                                                 << NodeType::VoxelServer << NodeType::ParticleServer << NodeType::EntityServer
                                                 << NodeType::MetavoxelServer);

    // connect to the packet sent signal of the _voxelEditSender and the _particleEditSender
    connect(&_voxelEditSender, &VoxelEditPacketSender::packetSent, this, &Application::packetSent);
    connect(&_particleEditSender, &ParticleEditPacketSender::packetSent, this, &Application::packetSent);
    connect(&_entityEditSender, &EntityEditPacketSender::packetSent, this, &Application::packetSent);

    // move the silentNodeTimer to the _nodeThread
    QTimer* silentNodeTimer = new QTimer();
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
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

    NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkDiskCache* cache = new QNetworkDiskCache();
    cache->setCacheDirectory(!cachePath.isEmpty() ? cachePath : "interfaceCache");
    networkAccessManager.setCache(cache);

    ResourceCache::setRequestLimit(3);

    _window->setCentralWidget(_glWidget);

    restoreSizeAndPosition();

    _window->setVisible(true);
    _glWidget->setFocusPolicy(Qt::StrongFocus);
    _glWidget->setFocus();

    // enable mouse tracking; otherwise, we only get drag events
    _glWidget->setMouseTracking(true);

    // initialization continues in initializeGL when OpenGL context is ready

    // Tell our voxel edit sender about our known jurisdictions
    _voxelEditSender.setVoxelServerJurisdictions(&_voxelServerJurisdictions);
    _particleEditSender.setServerJurisdictions(&_particleServerJurisdictions);
    _entityEditSender.setServerJurisdictions(&_entityServerJurisdictions);

    Particle::setVoxelEditPacketSender(&_voxelEditSender);
    Particle::setParticleEditPacketSender(&_particleEditSender);

    // For now we're going to set the PPS for outbound packets to be super high, this is
    // probably not the right long term solution. But for now, we're going to do this to
    // allow you to move a particle around in your hand
    _particleEditSender.setPacketsPerSecond(3000); // super high!!
    _entityEditSender.setPacketsPerSecond(3000); // super high!!

    // Set the sixense filtering
    _sixenseManager.setFilter(Menu::getInstance()->isOptionChecked(MenuOption::FilterSixense));
    
    // Set hand controller velocity filtering
    _sixenseManager.setLowVelocityFilter(Menu::getInstance()->isOptionChecked(MenuOption::LowVelocityFilter));

    checkVersion();

    _overlays.init(_glWidget); // do this before scripts load

    LocalVoxelsList::getInstance()->addPersistantTree(DOMAIN_TREE_NAME, _voxels.getTree());
    LocalVoxelsList::getInstance()->addPersistantTree(CLIPBOARD_TREE_NAME, &_clipboard);

    _runningScriptsWidget->setRunningScripts(getRunningScripts());
    connect(_runningScriptsWidget, &RunningScriptsWidget::stopScriptName, this, &Application::stopScript);

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(saveScripts()));

    // check first run...
    QVariant firstRunValue = _settings->value("firstRun",QVariant(true));
    if (firstRunValue.isValid() && firstRunValue.toBool()) {
        qDebug() << "This is a first run...";
        // clear the scripts, and set out script to our default scripts
        clearScriptsBeforeRunning();
        loadScript(DEFAULT_SCRIPTS_JS_URL);

        QMutexLocker locker(&_settingsMutex);
        _settings->setValue("firstRun",QVariant(false));
    } else {
        // do this as late as possible so that all required subsystems are initialized
        loadScripts();

        QMutexLocker locker(&_settingsMutex);
        _previousScriptLocation = _settings->value("LastScriptLocation", QVariant("")).toString();
    }

    connect(_window, &MainWindow::windowGeometryChanged,
            _runningScriptsWidget, &RunningScriptsWidget::setBoundary);
    
    AddressManager& addressManager = AddressManager::getInstance();

    // connect to the domainChangeRequired signal on AddressManager
    connect(&addressManager, &AddressManager::possibleDomainChangeRequired,
            this, &Application::changeDomainHostname);
    
    // when -url in command line, teleport to location
    addressManager.handleLookupString(getCmdOption(argc, constArgv, "-url"));

    // call the OAuthWebviewHandler static getter so that its instance lives in our thread
    OAuthWebViewHandler::getInstance();
    // make sure the High Fidelity root CA is in our list of trusted certs
    OAuthWebViewHandler::addHighFidelityRootCAToSSLConfig();

    _trayIcon->show();
    
#ifdef HAVE_RTMIDI
    // setup the MIDIManager
    MIDIManager& midiManagerInstance = MIDIManager::getInstance();
    midiManagerInstance.openDefaultPort();
#endif
}

Application::~Application() {
    qInstallMessageHandler(NULL);
    
    saveSettings();
    storeSizeAndPosition();
    
    int DELAY_TIME = 1000;
    UserActivityLogger::getInstance().close(DELAY_TIME);
    
    // make sure we don't call the idle timer any more
    delete idleTimer;
    
    _sharedVoxelSystem.changeTree(new VoxelTree);
    delete _voxelImportDialog;

    // let the avatar mixer know we're out
    MyAvatar::sendKillAvatar();

    // ask the datagram processing thread to quit and wait until it is done
    _nodeThread->quit();
    _nodeThread->wait();

    // stop the audio process
    QMetaObject::invokeMethod(&_audio, "stop");

    // ask the audio thread to quit and wait until it is done
    _audio.thread()->quit();
    _audio.thread()->wait();

    _octreeProcessor.terminate();
    _voxelHideShowThread.terminate();
    _voxelEditSender.terminate();
    _particleEditSender.terminate();
    _entityEditSender.terminate();


    VoxelTreeElement::removeDeleteHook(&_voxels); // we don't need to do this processing on shutdown
    Menu::getInstance()->deleteLater();

    _myAvatar = NULL;

    delete _glWidget;
}

void Application::saveSettings() {
    Menu::getInstance()->saveSettings();
    _rearMirrorTools->saveSettings(_settings);

    if (_voxelImportDialog) {
        _voxelImportDialog->saveSettings(_settings);
    }
    _settings->sync();
    _numChangedSettings = 0;
}


void Application::restoreSizeAndPosition() {
    QRect available = desktop()->availableGeometry();

    QMutexLocker locker(&_settingsMutex);
    _settings->beginGroup("Window");

    int x = (int)loadSetting(_settings, "x", 0);
    int y = (int)loadSetting(_settings, "y", 0);
    _window->move(x, y);

    int width = (int)loadSetting(_settings, "width", available.width());
    int height = (int)loadSetting(_settings, "height", available.height());
    _window->resize(width, height);

    _settings->endGroup();
}

void Application::storeSizeAndPosition() {
    QMutexLocker locker(&_settingsMutex);
    _settings->beginGroup("Window");

    _settings->setValue("width", _window->rect().width());
    _settings->setValue("height", _window->rect().height());

    _settings->setValue("x", _window->pos().x());
    _settings->setValue("y", _window->pos().y());

    _settings->endGroup();
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
    int argc = 0;
    glutInit(&argc, 0);
    #endif

    #ifdef WIN32
    GLenum err = glewInit();
    if (GLEW_OK != err) {
      /* Problem: glewInit failed, something is seriously wrong. */
      qDebug("Error: %s\n", glewGetErrorString(err));
    }
    qDebug("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif


    // Before we render anything, let's set up our viewFrustumOffsetCamera with a sufficiently large
    // field of view and near and far clip to make it interesting.
    //viewFrustumOffsetCamera.setFieldOfView(90.0);
    _viewFrustumOffsetCamera.setNearClip(DEFAULT_NEAR_CLIP);
    _viewFrustumOffsetCamera.setFarClip(DEFAULT_FAR_CLIP);

    initDisplay();
    qDebug( "Initialized Display.");

    init();
    qDebug( "init() complete.");

    // create thread for parsing of voxel data independent of the main network and rendering threads
    _octreeProcessor.initialize(_enableProcessVoxelsThread);
    _voxelEditSender.initialize(_enableProcessVoxelsThread);
    _voxelHideShowThread.initialize(_enableProcessVoxelsThread);
    _particleEditSender.initialize(_enableProcessVoxelsThread);
    _entityEditSender.initialize(_enableProcessVoxelsThread);

    if (_enableProcessVoxelsThread) {
        qDebug("Voxel parsing thread created.");
    }

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
        const char LOGSTASH_INTERFACE_START_TIME_KEY[] = "interface-start-time";

        // ask the Logstash class to record the startup time
        Logging::stashValue(STAT_TYPE_TIMER, LOGSTASH_INTERFACE_START_TIME_KEY, startupTime);
    }

    // update before the first render
    update(1.f / _fps);

    InfoView::showFirstTime();
}

void Application::paintGL() {
    PerformanceTimer perfTimer("paintGL");

    PerformanceWarning::setSuppressShortTimings(Menu::getInstance()->isOptionChecked(MenuOption::SuppressShortTimings));
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::paintGL()");

    // Set the desired FBO texture size. If it hasn't changed, this does nothing.
    // Otherwise, it must rebuild the FBOs
    if (OculusManager::isConnected()) {
        _textureCache.setFrameBufferSize(OculusManager::getRenderTargetSize());
    } else {
        _textureCache.setFrameBufferSize(_glWidget->getDeviceSize());
    }

    glEnable(GL_LINE_SMOOTH);

    if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON) {
        _myCamera.setTightness(0.0f);  //  In first person, camera follows (untweaked) head exactly without delay
        _myCamera.setTargetPosition(_myAvatar->getHead()->getEyePosition());
        _myCamera.setTargetRotation(_myAvatar->getHead()->getCameraOrientation());

    } else if (_myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
        //Note, the camera distance is set in Camera::setMode() so we dont have to do it here.
        _myCamera.setTightness(0.0f);     //  Camera is directly connected to head without smoothing
        _myCamera.setTargetPosition(_myAvatar->getUprightHeadPosition());
        if (OculusManager::isConnected()) {
            _myCamera.setTargetRotation(_myAvatar->getWorldAlignedOrientation());
        } else {
            _myCamera.setTargetRotation(_myAvatar->getHead()->getOrientation());
        }

    } else if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        _myCamera.setTightness(0.0f);
        //Only behave like a true mirror when in the OR
        if (OculusManager::isConnected()) {
            _myCamera.setDistance(MIRROR_FULLSCREEN_DISTANCE * _scaleMirror);
            _myCamera.setTargetRotation(_myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f)));
            _myCamera.setTargetPosition(_myAvatar->getHead()->getEyePosition() + glm::vec3(0, _raiseMirror * _myAvatar->getScale(), 0));
        } else {
            _myCamera.setTightness(0.0f);
            glm::vec3 eyePosition = _myAvatar->getHead()->getEyePosition();
            float headHeight = eyePosition.y - _myAvatar->getPosition().y;
            _myCamera.setDistance(MIRROR_FULLSCREEN_DISTANCE * _scaleMirror);
            _myCamera.setTargetPosition(_myAvatar->getPosition() + glm::vec3(0, headHeight + (_raiseMirror * _myAvatar->getScale()), 0));
            _myCamera.setTargetRotation(_myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f)));
        }
    }

    // Update camera position
    _myCamera.update( 1.f/_fps );

    // Note: whichCamera is used to pick between the normal camera myCamera for our
    // main camera, vs, an alternate camera. The alternate camera we support right now
    // is the viewFrustumOffsetCamera. But theoretically, we could use this same mechanism
    // to add other cameras.
    //
    // Why have two cameras? Well, one reason is that because in the case of the renderViewFrustum()
    // code, we want to keep the state of "myCamera" intact, so we can render what the view frustum of
    // myCamera is. But we also want to do meaningful camera transforms on OpenGL for the offset camera
    Camera whichCamera = _myCamera;

    if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayFrustum)) {

        ViewFrustumOffset viewFrustumOffset = Menu::getInstance()->getViewFrustumOffset();

        // set the camera to third-person view but offset so we can see the frustum
        _viewFrustumOffsetCamera.setTargetPosition(_myCamera.getTargetPosition());
        _viewFrustumOffsetCamera.setTargetRotation(_myCamera.getTargetRotation() * glm::quat(glm::radians(glm::vec3(
            viewFrustumOffset.pitch, viewFrustumOffset.yaw, viewFrustumOffset.roll))));
        _viewFrustumOffsetCamera.setUpShift(viewFrustumOffset.up);
        _viewFrustumOffsetCamera.setDistance(viewFrustumOffset.distance);
        _viewFrustumOffsetCamera.initialize(); // force immediate snap to ideal position and orientation
        _viewFrustumOffsetCamera.update(1.f/_fps);
        whichCamera = _viewFrustumOffsetCamera;
    }

    if (Menu::getInstance()->getShadowsEnabled()) {
        updateShadowMap();
    }

    if (OculusManager::isConnected()) {
        //Clear the color buffer to ensure that there isnt any residual color
        //Left over from when OR was not connected.
        glClear(GL_COLOR_BUFFER_BIT);
        
        //When in mirror mode, use camera rotation. Otherwise, use body rotation
        if (whichCamera.getMode() == CAMERA_MODE_MIRROR) {
            OculusManager::display(whichCamera.getRotation(), whichCamera.getPosition(), whichCamera);
        } else {
            OculusManager::display(_myAvatar->getWorldAlignedOrientation(), whichCamera.getPosition(), whichCamera);
        }

    } else if (TV3DManager::isConnected()) {
       
        TV3DManager::display(whichCamera);

    } else {
        _glowEffect.prepare();

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        displaySide(whichCamera);
        glPopMatrix();

        _glowEffect.render();

        if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
            renderRearViewMirror(_mirrorViewRect);

        } else if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
            _rearMirrorTools->render(true);
        }

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

void Application::resetCamerasOnResizeGL(Camera& camera, int width, int height) {
    if (OculusManager::isConnected()) {
        OculusManager::configureCamera(camera, width, height);
    } else if (TV3DManager::isConnected()) {
        TV3DManager::configureCamera(camera, width, height);
    } else {
        camera.setAspectRatio((float)width / height);
        camera.setFieldOfView(Menu::getInstance()->getFieldOfView());
    }
}

void Application::resizeGL(int width, int height) {
    resetCamerasOnResizeGL(_viewFrustumOffsetCamera, width, height);
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
        computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);

        // If we're in Display Frustum mode, then we want to use the slightly adjust near/far clip values of the
        // _viewFrustumOffsetCamera, so that we can see more of the application content in the application's frustum
        if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayFrustum)) {
            nearVal = _viewFrustumOffsetCamera.getNearClip();
            farVal = _viewFrustumOffsetCamera.getFarClip();
        }
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
        // Intercept data to voxel server when voxels are disabled
        if (type == NodeType::VoxelServer && !Menu::getInstance()->isOptionChecked(MenuOption::Voxels)) {
            continue;
        }

        // Perform the broadcast for one type
        int nReceivingNodes = NodeList::getInstance()->broadcastToNodes(packet, NodeSet() << type);

        // Feed number of bytes to corresponding channel of the bandwidth meter, if any (done otherwise)
        BandwidthMeter::ChannelIndex channel;
        switch (type) {
            case NodeType::Agent:
            case NodeType::AvatarMixer:
                channel = BandwidthMeter::AVATARS;
                break;
            case NodeType::VoxelServer:
            case NodeType::ParticleServer:
            case NodeType::EntityServer:
                channel = BandwidthMeter::VOXELS;
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
            AddressManager::getInstance().handleLookupString(fileEvent->url().toLocalFile());
        }
        
        return false;
    }
    
    return QApplication::event(event);
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
            case Qt::Key_BracketLeft:
            case Qt::Key_BracketRight:
            case Qt::Key_BraceLeft:
            case Qt::Key_BraceRight:
            case Qt::Key_ParenLeft:
            case Qt::Key_ParenRight:
            case Qt::Key_Less:
            case Qt::Key_Greater:
            case Qt::Key_Comma:
            case Qt::Key_Period:
                Menu::getInstance()->handleViewFrustumOffsetKeyModifier(event->key());
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
                _myAvatar->setDriveKeys(UP, 1.f);
                break;

            case Qt::Key_Asterisk:
                Menu::getInstance()->triggerOption(MenuOption::Stars);
                break;

            case Qt::Key_C:
            case Qt::Key_PageDown:
                _myAvatar->setDriveKeys(DOWN, 1.f);
                break;

            case Qt::Key_W:
                _myAvatar->setDriveKeys(FWD, 1.f);
                break;

            case Qt::Key_S:
                if (isShifted && isMeta && !isOption) {
                    Menu::getInstance()->triggerOption(MenuOption::SuppressShortTimings);
                } else if (isOption && !isShifted && !isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::ScriptEditor);
                } else if (!isOption && !isShifted && isMeta) {
                    takeSnapshot();
                } else {
                    _myAvatar->setDriveKeys(BACK, 1.f);
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
                    _myAvatar->setDriveKeys(ROT_LEFT, 1.f);
                }
                break;

            case Qt::Key_D:
                if (!isMeta) {
                    _myAvatar->setDriveKeys(ROT_RIGHT, 1.f);
                }
                break;

            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::AddressBar);
                } else {
                    Menu::getInstance()->triggerOption(MenuOption::Chat);
                }
                
                break;

            case Qt::Key_Up:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    if (!isShifted) {
                        _scaleMirror *= 0.95f;
                    } else {
                        _raiseMirror += 0.05f;
                    }
                } else {
                    _myAvatar->setDriveKeys(isShifted ? UP : FWD, 1.f);
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
                    _myAvatar->setDriveKeys(isShifted ? DOWN : BACK, 1.f);
                }
                break;

            case Qt::Key_Left:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    _rotateMirror += PI / 20.f;
                } else {
                    _myAvatar->setDriveKeys(isShifted ? LEFT : ROT_LEFT, 1.f);
                }
                break;

            case Qt::Key_Right:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    _rotateMirror -= PI / 20.f;
                } else {
                    _myAvatar->setDriveKeys(isShifted ? RIGHT : ROT_RIGHT, 1.f);
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
                        TV3DManager::configureCamera(_myCamera, _glWidget->getDeviceWidth(), _glWidget->getDeviceHeight());
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
                        TV3DManager::configureCamera(_myCamera, _glWidget->getDeviceWidth(), _glWidget->getDeviceHeight());
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
            case Qt::Key_F:
                if (isShifted)  {
                    Menu::getInstance()->triggerOption(MenuOption::DisplayFrustum);
                }
                break;
            case Qt::Key_V:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Voxels);
                }
                break;
            case Qt::Key_P:
                 Menu::getInstance()->triggerOption(MenuOption::FirstPerson);
                 break;
            case Qt::Key_R:
                if (isShifted)  {
                    Menu::getInstance()->triggerOption(MenuOption::FrustumRenderMode);
                }
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
            _myAvatar->setDriveKeys(UP, 0.f);
            break;

        case Qt::Key_C:
        case Qt::Key_PageDown:
            _myAvatar->setDriveKeys(DOWN, 0.f);
            break;

        case Qt::Key_W:
            _myAvatar->setDriveKeys(FWD, 0.f);
            break;

        case Qt::Key_S:
            _myAvatar->setDriveKeys(BACK, 0.f);
            break;

        case Qt::Key_A:
            _myAvatar->setDriveKeys(ROT_LEFT, 0.f);
            break;

        case Qt::Key_D:
            _myAvatar->setDriveKeys(ROT_RIGHT, 0.f);
            break;

        case Qt::Key_Up:
            _myAvatar->setDriveKeys(FWD, 0.f);
            _myAvatar->setDriveKeys(UP, 0.f);
            break;

        case Qt::Key_Down:
            _myAvatar->setDriveKeys(BACK, 0.f);
            _myAvatar->setDriveKeys(DOWN, 0.f);
            break;

        case Qt::Key_Left:
            _myAvatar->setDriveKeys(LEFT, 0.f);
            _myAvatar->setDriveKeys(ROT_LEFT, 0.f);
            break;

        case Qt::Key_Right:
            _myAvatar->setDriveKeys(RIGHT, 0.f);
            _myAvatar->setDriveKeys(ROT_RIGHT, 0.f);
            break;
        case Qt::Key_Control:
        case Qt::Key_Shift:
        case Qt::Key_Meta:
        case Qt::Key_Alt:
            _myAvatar->clearDriveKeys();
            break;
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

    bool showMouse = true;

    // Used by application overlay to determine how to draw cursor(s)
    _lastMouseMoveWasSimulated = deviceID > 0;

    // If this mouse move event is emitted by a controller, dont show the mouse cursor
    if (_lastMouseMoveWasSimulated) {
        showMouse = false;
    }

    _controllerScriptingInterface.emitMouseMoveEvent(event, deviceID); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }

    _lastMouseMove = usecTimestampNow();

    if (_mouseHidden && showMouse && !OculusManager::isConnected() && !TV3DManager::isConnected()) {
        getGLWidget()->setCursor(Qt::ArrowCursor);
        _mouseHidden = false;
        _seenMouseMove = true;
    }

    _mouseX = event->x();
    _mouseY = event->y();
}

void Application::mousePressEvent(QMouseEvent* event, unsigned int deviceID) {
    _controllerScriptingInterface.emitMousePressEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }


    if (activeWindow() == _window) {
        if (event->button() == Qt::LeftButton) {
            _mouseX = event->x();
            _mouseY = event->y();
            _mouseDragStartedX = _mouseX;
            _mouseDragStartedY = _mouseY;
            _mousePressed = true;

            if (_audio.mousePressEvent(_mouseX, _mouseY)) {
                // stop propagation
                return;
            }

            if (_rearMirrorTools->mousePressEvent(_mouseX, _mouseY)) {
                // stop propagation
                return;
            }

        } else if (event->button() == Qt::RightButton) {
            // right click items here
        }
    }
}

void Application::mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID) {
    _controllerScriptingInterface.emitMouseReleaseEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }

    if (activeWindow() == _window) {
        if (event->button() == Qt::LeftButton) {
            _mouseX = event->x();
            _mouseY = event->y();
            _mousePressed = false;
            checkBandwidthMeterClick();
            if (Menu::getInstance()->isOptionChecked(MenuOption::Stats)) {
                // let's set horizontal offset to give stats some margin to mirror
                int horizontalOffset = MIRROR_VIEW_WIDTH;
                Stats::getInstance()->checkClick(_mouseX, _mouseY, _mouseDragStartedX, _mouseDragStartedY, horizontalOffset);
            }
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
            changeDomainHostname(snapshotData->getDomain());
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
    QByteArray pingPacket = NodeList::getInstance()->constructPingPacket();
    controlledBroadcastToNodes(pingPacket, NodeSet()
                               << NodeType::VoxelServer << NodeType::ParticleServer << NodeType::EntityServer
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
    NodeList::getInstance()->sendDomainServerCheckIn();
}

void Application::idle() {
    PerformanceTimer perfTimer("idle");

    // Normally we check PipelineWarnings, but since idle will often take more than 10ms we only show these idle timing
    // details if we're in ExtraDebugging mode. However, the ::update() and it's subcomponents will show their timing
    // details normally.
    bool showWarnings = getLogger()->extraDebugging();
    PerformanceWarning warn(showWarnings, "idle()");

    //  Only run simulation code if more than IDLE_SIMULATE_MSECS have passed since last time we ran

    double timeSinceLastUpdate = (double)_lastTimeUpdated.nsecsElapsed() / 1000000.0;
    if (timeSinceLastUpdate > IDLE_SIMULATE_MSECS) {
        _lastTimeUpdated.start();
        {
            PerformanceTimer perfTimer("update");
            PerformanceWarning warn(showWarnings, "Application::idle()... update()");
            const float BIGGEST_DELTA_TIME_SECS = 0.25f;
            update(glm::clamp((float)timeSinceLastUpdate / 1000.f, 0.f, BIGGEST_DELTA_TIME_SECS));
        }
        {
            PerformanceTimer perfTimer("updateGL");
            PerformanceWarning warn(showWarnings, "Application::idle()... updateGL()");
            _glWidget->updateGL();
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

            if (Menu::getInstance()->isOptionChecked(MenuOption::BuckyBalls)) {
                PerformanceTimer perfTimer("buckyBalls");
                _buckyBalls.simulate(timeSinceLastUpdate / 1000.f, Application::getInstance()->getAvatar()->getHandData());
            }

            // After finishing all of the above work, restart the idle timer, allowing 2ms to process events.
            idleTimer->start(2);
            
            if (_numChangedSettings > 0) {
                saveSettings();
            }
        }
    }
}

void Application::checkBandwidthMeterClick() {
    // ... to be called upon button release

    if (Menu::getInstance()->isOptionChecked(MenuOption::Bandwidth) &&
        glm::compMax(glm::abs(glm::ivec2(_mouseX - _mouseDragStartedX, _mouseY - _mouseDragStartedY)))
            <= BANDWIDTH_METER_CLICK_MAX_DRAG_LENGTH
            && _bandwidthMeter.isWithinArea(_mouseX, _mouseY, _glWidget->width(), _glWidget->height())) {

        // The bandwidth meter is visible, the click didn't get dragged too far and
        // we actually hit the bandwidth meter
        Menu::getInstance()->bandwidthDetails();
    }
}

void Application::setFullscreen(bool fullscreen) {
    _window->setWindowState(fullscreen ? (_window->windowState() | Qt::WindowFullScreen) :
        (_window->windowState() & ~Qt::WindowFullScreen));
}

void Application::setEnable3DTVMode(bool enable3DTVMode) {
    resizeGL(_glWidget->getDeviceWidth(),_glWidget->getDeviceHeight());
}

void Application::setEnableVRMode(bool enableVRMode) {
    if (enableVRMode) {
        if (!OculusManager::isConnected()) {
            // attempt to reconnect the Oculus manager - it's possible this was a workaround
            // for the sixense crash
            OculusManager::disconnect();
            OculusManager::connect();
        }
    }
    
    resizeGL(_glWidget->getDeviceWidth(), _glWidget->getDeviceHeight());
}

void Application::setRenderVoxels(bool voxelRender) {
    _voxelEditSender.setShouldSend(voxelRender);
    if (!voxelRender) {
        doKillLocalVoxels();
    }
}

void Application::setLowVelocityFilter(bool lowVelocityFilter) {
    getSixenseManager()->setLowVelocityFilter(lowVelocityFilter);
}

void Application::doKillLocalVoxels() {
    _wantToKillLocalVoxels = true;
}

void Application::removeVoxel(glm::vec3 position,
                              float scale) {
    VoxelDetail voxel;
    voxel.x = position.x / TREE_SCALE;
    voxel.y = position.y / TREE_SCALE;
    voxel.z = position.z / TREE_SCALE;
    voxel.s = scale / TREE_SCALE;
    _voxelEditSender.sendVoxelEditMessage(PacketTypeVoxelErase, voxel);

    // delete it locally to see the effect immediately (and in case no voxel server is present)
    _voxels.getTree()->deleteVoxelAt(voxel.x, voxel.y, voxel.z, voxel.s);
}


void Application::makeVoxel(glm::vec3 position,
                            float scale,
                            unsigned char red,
                            unsigned char green,
                            unsigned char blue,
                            bool isDestructive) {
    VoxelDetail voxel;
    voxel.x = position.x / TREE_SCALE;
    voxel.y = position.y / TREE_SCALE;
    voxel.z = position.z / TREE_SCALE;
    voxel.s = scale / TREE_SCALE;
    voxel.red = red;
    voxel.green = green;
    voxel.blue = blue;
    PacketType message = isDestructive ? PacketTypeVoxelSetDestructive : PacketTypeVoxelSet;
    _voxelEditSender.sendVoxelEditMessage(message, voxel);

    // create the voxel locally so it appears immediately
    _voxels.getTree()->createVoxel(voxel.x, voxel.y, voxel.z, voxel.s,
                        voxel.red, voxel.green, voxel.blue,
                        isDestructive);
   }

glm::vec3 Application::getMouseVoxelWorldCoordinates(const VoxelDetail& mouseVoxel) {
    return glm::vec3((mouseVoxel.x + mouseVoxel.s / 2.f) * TREE_SCALE, (mouseVoxel.y + mouseVoxel.s / 2.f) * TREE_SCALE,
        (mouseVoxel.z + mouseVoxel.s / 2.f) * TREE_SCALE);
}

FaceTracker* Application::getActiveFaceTracker() {
    return (_dde.isActive() ? static_cast<FaceTracker*>(&_dde) :
            (_cara.isActive() ? static_cast<FaceTracker*>(&_cara) :
             (_faceshift.isActive() ? static_cast<FaceTracker*>(&_faceshift) :
              (_faceplus.isActive() ? static_cast<FaceTracker*>(&_faceplus) :
               (_visage.isActive() ? static_cast<FaceTracker*>(&_visage) : NULL)))));
}

struct SendVoxelsOperationArgs {
    const unsigned char*  newBaseOctCode;
};

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

bool Application::sendVoxelsOperation(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    SendVoxelsOperationArgs* args = (SendVoxelsOperationArgs*)extraData;
    if (voxel->isColored()) {
        const unsigned char* nodeOctalCode = voxel->getOctalCode();
        unsigned char* codeColorBuffer = NULL;
        int codeLength  = 0;
        int bytesInCode = 0;
        int codeAndColorLength;

        // If the newBase is NULL, then don't rebase
        if (args->newBaseOctCode) {
            codeColorBuffer = rebaseOctalCode(nodeOctalCode, args->newBaseOctCode, true);
            codeLength  = numberOfThreeBitSectionsInCode(codeColorBuffer);
            bytesInCode = bytesRequiredForCodeLength(codeLength);
            codeAndColorLength = bytesInCode + SIZE_OF_COLOR_DATA;
        } else {
            codeLength  = numberOfThreeBitSectionsInCode(nodeOctalCode);
            bytesInCode = bytesRequiredForCodeLength(codeLength);
            codeAndColorLength = bytesInCode + SIZE_OF_COLOR_DATA;
            codeColorBuffer = new unsigned char[codeAndColorLength];
            memcpy(codeColorBuffer, nodeOctalCode, bytesInCode);
        }

        // copy the colors over
        codeColorBuffer[bytesInCode + RED_INDEX] = voxel->getColor()[RED_INDEX];
        codeColorBuffer[bytesInCode + GREEN_INDEX] = voxel->getColor()[GREEN_INDEX];
        codeColorBuffer[bytesInCode + BLUE_INDEX] = voxel->getColor()[BLUE_INDEX];
        getInstance()->_voxelEditSender.queueVoxelEditMessage(PacketTypeVoxelSetDestructive,
                codeColorBuffer, codeAndColorLength);

        delete[] codeColorBuffer;
    }
    return true; // keep going
}

void Application::exportVoxels(const VoxelDetail& sourceVoxel) {
    QString desktopLocation = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString suggestedName = desktopLocation.append("/voxels.svo");

    QString fileNameString = QFileDialog::getSaveFileName(_glWidget, tr("Export Voxels"), suggestedName,
                                                          tr("Sparse Voxel Octree Files (*.svo)"));
    QByteArray fileNameAscii = fileNameString.toLocal8Bit();
    const char* fileName = fileNameAscii.data();

    VoxelTreeElement* selectedNode = _voxels.getTree()->getVoxelAt(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s);
    if (selectedNode) {
        VoxelTree exportTree;
        getVoxelTree()->copySubTreeIntoNewTree(selectedNode, &exportTree, true);
        exportTree.writeToSVOFile(fileName);
    }

    // restore the main window's active state
    _window->activateWindow();
}

void Application::importVoxels() {
    _importSucceded = false;

    if (!_voxelImportDialog) {
        _voxelImportDialog = new VoxelImportDialog(_window);
        _voxelImportDialog->loadSettings(_settings);
    }

    if (!_voxelImportDialog->exec()) {
        qDebug() << "Import succeeded." << endl;
        _importSucceded = true;
    } else {
        qDebug() << "Import failed." << endl;
        if (_sharedVoxelSystem.getTree() == _voxelImporter.getVoxelTree()) {
            _sharedVoxelSystem.killLocalVoxels();
            _sharedVoxelSystem.changeTree(&_clipboard);
        }
    }

    // restore the main window's active state
    _window->activateWindow();

    emit importDone();
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

void Application::cutVoxels(const VoxelDetail& sourceVoxel) {
    copyVoxels(sourceVoxel);
    deleteVoxelAt(sourceVoxel);
}

void Application::copyVoxels(const VoxelDetail& sourceVoxel) {
    // switch to and clear the clipboard first...
    _sharedVoxelSystem.killLocalVoxels();
    if (_sharedVoxelSystem.getTree() != &_clipboard) {
        _clipboard.eraseAllOctreeElements();
        _sharedVoxelSystem.changeTree(&_clipboard);
    }

    // then copy onto it if there is something to copy
    VoxelTreeElement* selectedNode = _voxels.getTree()->getVoxelAt(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s);
    if (selectedNode) {
        getVoxelTree()->copySubTreeIntoNewTree(selectedNode, _sharedVoxelSystem.getTree(), true);
        _sharedVoxelSystem.forceRedrawEntireTree();
    }
}

void Application::pasteVoxelsToOctalCode(const unsigned char* octalCodeDestination) {
    // Recurse the clipboard tree, where everything is root relative, and send all the colored voxels to
    // the server as an set voxel message, this will also rebase the voxels to the new location
    SendVoxelsOperationArgs args;
    args.newBaseOctCode = octalCodeDestination;
    _sharedVoxelSystem.getTree()->recurseTreeWithOperation(sendVoxelsOperation, &args);

    // Switch back to clipboard if it was an import
    if (_sharedVoxelSystem.getTree() != &_clipboard) {
        _sharedVoxelSystem.killLocalVoxels();
        _sharedVoxelSystem.changeTree(&_clipboard);
    }

    _voxelEditSender.releaseQueuedMessages();
}

void Application::pasteVoxels(const VoxelDetail& sourceVoxel) {
    unsigned char* calculatedOctCode = NULL;
    VoxelTreeElement* selectedNode = _voxels.getTree()->getVoxelAt(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s);

    // we only need the selected voxel to get the newBaseOctCode, which we can actually calculate from the
    // voxel size/position details. If we don't have an actual selectedNode then use the mouseVoxel to create a
    // target octalCode for where the user is pointing.
    const unsigned char* octalCodeDestination;
    if (selectedNode) {
        octalCodeDestination = selectedNode->getOctalCode();
    } else {
        octalCodeDestination = calculatedOctCode = pointToVoxel(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s);
    }

    pasteVoxelsToOctalCode(octalCodeDestination);

    if (calculatedOctCode) {
        delete[] calculatedOctCode;
    }
}

void Application::nudgeVoxelsByVector(const VoxelDetail& sourceVoxel, const glm::vec3& nudgeVec) {
    VoxelTreeElement* nodeToNudge = _voxels.getTree()->getVoxelAt(sourceVoxel.x, sourceVoxel.y, sourceVoxel.z, sourceVoxel.s);
    if (nodeToNudge) {
        _voxels.getTree()->nudgeSubTree(nodeToNudge, nudgeVec, _voxelEditSender);
    }
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
    _sharedVoxelSystemViewFrustum.setPosition(glm::vec3(TREE_SCALE / 2.0f,
                                                        TREE_SCALE / 2.0f,
                                                        3.0f * TREE_SCALE / 2.0f));
    _sharedVoxelSystemViewFrustum.setNearClip(TREE_SCALE / 2.0f);
    _sharedVoxelSystemViewFrustum.setFarClip(3.0f * TREE_SCALE / 2.0f);
    _sharedVoxelSystemViewFrustum.setFieldOfView(90.f);
    _sharedVoxelSystemViewFrustum.setOrientation(glm::quat());
    _sharedVoxelSystemViewFrustum.calculate();
    _sharedVoxelSystem.setViewFrustum(&_sharedVoxelSystemViewFrustum);

    VoxelTreeElement::removeUpdateHook(&_sharedVoxelSystem);

    // Cleanup of the original shared tree
    _sharedVoxelSystem.init();

    _voxelImportDialog = new VoxelImportDialog(_window);

    _environment.init();

    _deferredLightingEffect.init();
    _glowEffect.init();
    _ambientOcclusionEffect.init();
    _voxelShader.init();
    _pointShader.init();

    _mouseX = _glWidget->width() / 2;
    _mouseY = _glWidget->height() / 2;
    QCursor::setPos(_mouseX, _mouseY);

    // TODO: move _myAvatar out of Application. Move relevant code to MyAvataar or AvatarManager
    _avatarManager.init();
    _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
    _myCamera.setModeShiftPeriod(1.0f);

    _mirrorCamera.setMode(CAMERA_MODE_MIRROR);
    _mirrorCamera.setModeShiftPeriod(0.0f);

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

    Menu::getInstance()->loadSettings();
    _audio.setReceivedAudioStreamSettings(Menu::getInstance()->getReceivedAudioStreamSettings());

    qDebug("Loaded settings");

    // initialize our face trackers after loading the menu settings
    _faceshift.init();
    _faceplus.init();
    _visage.init();

    Leapmotion::init();

    // fire off an immediate domain-server check in now that settings are loaded
    NodeList::getInstance()->sendDomainServerCheckIn();

    // Set up VoxelSystem after loading preferences so we can get the desired max voxel count
    _voxels.setMaxVoxels(Menu::getInstance()->getMaxVoxels());
    _voxels.setUseVoxelShader(false);
    _voxels.setVoxelsAsPoints(false);
    _voxels.setDisableFastVoxelPipeline(false);
    _voxels.init();

    _particles.init();
    _particles.setViewFrustum(getViewFrustum());

    _entities.init();
    _entities.setViewFrustum(getViewFrustum());

    _entityClipboardRenderer.init();
    _entityClipboardRenderer.setViewFrustum(getViewFrustum());
    _entityClipboardRenderer.setTree(&_entityClipboard);

    _metavoxels.init();

    _particleCollisionSystem.init(&_particleEditSender, _particles.getTree(), _voxels.getTree(), &_audio, &_avatarManager);

    // connect the _particleCollisionSystem to our script engine's ParticlesScriptingInterface
    connect(&_particleCollisionSystem, &ParticleCollisionSystem::particleCollisionWithVoxel,
            ScriptEngine::getParticlesScriptingInterface(), &ParticlesScriptingInterface::particleCollisionWithVoxel);

    connect(&_particleCollisionSystem, &ParticleCollisionSystem::particleCollisionWithParticle,
            ScriptEngine::getParticlesScriptingInterface(), &ParticlesScriptingInterface::particleCollisionWithParticle);

    _audio.init(_glWidget);

    _rearMirrorTools = new RearMirrorTools(_glWidget, _mirrorViewRect, _settings);

    connect(_rearMirrorTools, SIGNAL(closeView()), SLOT(closeMirrorView()));
    connect(_rearMirrorTools, SIGNAL(restoreView()), SLOT(restoreMirrorView()));
    connect(_rearMirrorTools, SIGNAL(shrinkView()), SLOT(shrinkMirrorView()));
    connect(_rearMirrorTools, SIGNAL(resetView()), SLOT(resetSensors()));

    // set up our audio reflector
    _audioReflector.setMyAvatar(getAvatar());
    _audioReflector.setVoxels(_voxels.getTree());
    _audioReflector.setAudio(getAudio());
    _audioReflector.setAvatarManager(&_avatarManager);

    connect(getAudio(), &Audio::processInboundAudio, &_audioReflector, &AudioReflector::processInboundAudio,Qt::DirectConnection);
    connect(getAudio(), &Audio::processLocalAudio, &_audioReflector, &AudioReflector::processLocalAudio,Qt::DirectConnection);
    connect(getAudio(), &Audio::preProcessOriginalInboundAudio, &_audioReflector,
                        &AudioReflector::preProcessOriginalInboundAudio,Qt::DirectConnection);

    // save settings when avatar changes
    connect(_myAvatar, &MyAvatar::transformChanged, this, &Application::bumpSettings);
}

void Application::closeMirrorView() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
        Menu::getInstance()->triggerOption(MenuOption::Mirror);;
    }
}

void Application::restoreMirrorView() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
        Menu::getInstance()->triggerOption(MenuOption::Mirror);;
    }

    if (!Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        Menu::getInstance()->triggerOption(MenuOption::FullscreenMirror);
    }
}

void Application::shrinkMirrorView() {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
        Menu::getInstance()->triggerOption(MenuOption::Mirror);;
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        Menu::getInstance()->triggerOption(MenuOption::FullscreenMirror);
    }
}

const float HEAD_SPHERE_RADIUS = 0.07f;

bool Application::isLookingAtMyAvatar(Avatar* avatar) {
    glm::vec3 theirLookat = avatar->getHead()->getLookAtPosition();
    glm::vec3 myHeadPosition = _myAvatar->getHead()->getPosition();

    if (pointInSphere(theirLookat, myHeadPosition, HEAD_SPHERE_RADIUS * _myAvatar->getScale())) {
        return true;
    }
    return false;
}

void Application::updateLOD() {
    PerformanceTimer perfTimer("LOD");
    // adjust it unless we were asked to disable this feature, or if we're currently in throttleRendering mode
    if (!Menu::getInstance()->isOptionChecked(MenuOption::DisableAutoAdjustLOD) && !isThrottleRendering()) {
        Menu::getInstance()->autoAdjustLOD(_fps);
    } else {
        Menu::getInstance()->resetLODAdjust();
    }
}

void Application::updateMouseRay() {
    PerformanceTimer perfTimer("mouseRay");

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMouseRay()");

    // make sure the frustum is up-to-date
    loadViewFrustum(_myCamera, _viewFrustum);

    // if the mouse pointer isn't visible, act like it's at the center of the screen
    float x = 0.5f, y = 0.5f;
    if (!_mouseHidden) {
        x = _mouseX / (float)_glWidget->width();
        y = _mouseY / (float)_glWidget->height();
    }
    _viewFrustum.computePickRay(x, y, _mouseRayOrigin, _mouseRayDirection);

    // adjust for mirroring
    if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        glm::vec3 mouseRayOffset = _mouseRayOrigin - _viewFrustum.getPosition();
        _mouseRayOrigin -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), mouseRayOffset) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), mouseRayOffset));
        _mouseRayDirection -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), _mouseRayDirection) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), _mouseRayDirection));
    }

    // tell my avatar if the mouse is being pressed...
    _myAvatar->setMousePressed(_mousePressed);

    // tell my avatar the posiion and direction of the ray projected ino the world based on the mouse position
    _myAvatar->setMouseRay(_mouseRayOrigin, _mouseRayDirection);
}

void Application::updateFaceshift() {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateFaceshift()");

    //  Update faceshift
    _faceshift.update();

    //  Copy angular velocity if measured by faceshift, to the head
    if (_faceshift.isActive()) {
        _myAvatar->getHead()->setAngularVelocity(_faceshift.getHeadAngularVelocity());
    }
}

void Application::updateVisage() {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateVisage()");

    //  Update Visage
    _visage.update();
}

void Application::updateDDE() {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateDDE()");
    
    //  Update Cara
    _dde.update();
}

void Application::updateCara() {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateCara()");
    
    //  Update Cara
    _cara.update();
    
    //  Copy angular velocity if measured by cara, to the head
    if (_cara.isActive()) {
        _myAvatar->getHead()->setAngularVelocity(_cara.getHeadAngularVelocity());
    }
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
        lookAtSpot = _myCamera.getPosition();

    } else {
        if (_myAvatar->getLookAtTargetAvatar() && _myAvatar != _myAvatar->getLookAtTargetAvatar()) {
            isLookingAtSomeone = true;
            //  If I am looking at someone else, look directly at one of their eyes
            if (tracker) {
                //  If tracker active, look at the eye for the side my gaze is biased toward
                if (tracker->getEstimatedEyeYaw() > _myAvatar->getHead()->getFinalYaw()) {
                    // Look at their right eye
                    lookAtSpot = static_cast<Avatar*>(_myAvatar->getLookAtTargetAvatar())->getHead()->getRightEyePosition();
                } else {
                    // Look at their left eye
                    lookAtSpot = static_cast<Avatar*>(_myAvatar->getLookAtTargetAvatar())->getHead()->getLeftEyePosition();
                }
            } else {
                //  Need to add randomly looking back and forth between left and right eye for case with no tracker
                lookAtSpot = static_cast<Avatar*>(_myAvatar->getLookAtTargetAvatar())->getHead()->getEyePosition();
            }
        } else {
            //  I am not looking at anyone else, so just look forward
            lookAtSpot = _myAvatar->getHead()->getEyePosition() +
                (_myAvatar->getHead()->getFinalOrientationInWorldFrame() * glm::vec3(0.f, 0.f, -TREE_SCALE));
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
        float deflection = Menu::getInstance()->getFaceshiftEyeDeflection();
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
    if (!_enableProcessVoxelsThread) {
        _octreeProcessor.threadRoutine();
        _voxelHideShowThread.threadRoutine();
        _voxelEditSender.threadRoutine();
        _particleEditSender.threadRoutine();
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
    float modeShiftPeriod = (_myCamera.getMode() == CAMERA_MODE_MIRROR) ? 0.0f : 1.0f;
    if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        if (_myCamera.getMode() != CAMERA_MODE_MIRROR) {
            _myCamera.setMode(CAMERA_MODE_MIRROR);
            _myCamera.setModeShiftPeriod(0.0f);
        }
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson)) {
        if (_myCamera.getMode() != CAMERA_MODE_FIRST_PERSON) {
            _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
            _myCamera.setModeShiftPeriod(modeShiftPeriod);
        }
    } else {
        if (_myCamera.getMode() != CAMERA_MODE_THIRD_PERSON) {
            _myCamera.setMode(CAMERA_MODE_THIRD_PERSON);
            _myCamera.setModeShiftPeriod(modeShiftPeriod);
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

    // Update bandwidth dialog, if any
    BandwidthDialog* bandwidthDialog = Menu::getInstance()->getBandwidthDialog();
    if (bandwidthDialog) {
        bandwidthDialog->update();
    }

    OctreeStatsDialog* octreeStatsDialog = Menu::getInstance()->getOctreeStatsDialog();
    if (octreeStatsDialog) {
        octreeStatsDialog->update();
    }
}

void Application::updateCursor(float deltaTime) {
    PerformanceTimer perfTimer("updateCursor");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateCursor()");

    // watch mouse position, if it hasn't moved, hide the cursor
    bool underMouse = _glWidget->underMouse();
    if (!_mouseHidden) {
        quint64 now = usecTimestampNow();
        int elapsed = now - _lastMouseMove;
        const int HIDE_CURSOR_TIMEOUT = 1 * 1000 * 1000; // 1 second
        if (elapsed > HIDE_CURSOR_TIMEOUT && (underMouse || !_seenMouseMove)) {
            getGLWidget()->setCursor(Qt::BlankCursor);
            _mouseHidden = true;
        }
    } else {
        // if the mouse is hidden, but we're not inside our window, then consider ourselves to be moving
        if (!underMouse && _seenMouseMove) {
            _lastMouseMove = usecTimestampNow();
            getGLWidget()->setCursor(Qt::ArrowCursor);
            _mouseHidden = false;
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
        _sixenseManager.update(deltaTime);
        _joystickManager.update();
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
        PerformanceTimer perfTimer("particles");
        _particles.update(); // update the particles...
        {
            PerformanceTimer perfTimer("collisions");
            _particleCollisionSystem.update(); // collide the particles...
        }
    }

    {
        PerformanceTimer perfTimer("entities");
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

            if (Menu::getInstance()->isOptionChecked(MenuOption::Voxels)) {
                queryOctree(NodeType::VoxelServer, PacketTypeVoxelQuery, _voxelServerJurisdictions);
            }
            if (Menu::getInstance()->isOptionChecked(MenuOption::Particles)) {
                queryOctree(NodeType::ParticleServer, PacketTypeParticleQuery, _particleServerJurisdictions);
            }
            if (Menu::getInstance()->isOptionChecked(MenuOption::Models)) {
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

            QMetaObject::invokeMethod(&_audio, "sendDownstreamAudioStatsPacket", Qt::QueuedConnection);
        }
    }
}

void Application::updateMyAvatar(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMyAvatar()");

    _myAvatar->update(deltaTime);

    {
        // send head/hand data to the avatar mixer and voxel server
        PerformanceTimer perfTimer("send");
        QByteArray packet = byteArrayWithPopulatedHeader(PacketTypeAvatarData);
        packet.append(_myAvatar->toByteArray());
        controlledBroadcastToNodes(packet, NodeSet() << NodeType::AvatarMixer);
    }
}

int Application::sendNackPackets() {

    if (Menu::getInstance()->isOptionChecked(MenuOption::DisableNackPackets)) {
        return 0;
    }

    int packetsSent = 0;
    char packet[MAX_PACKET_SIZE];

    // iterates thru all nodes in NodeList
    foreach(const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        
        if (node->getActiveSocket() &&
            ( node->getType() == NodeType::VoxelServer
            || node->getType() == NodeType::ParticleServer
            || node->getType() == NodeType::EntityServer)
            ) {

            QUuid nodeUUID = node->getUUID();

            // if there are octree packets from this node that are waiting to be processed,
            // don't send a NACK since the missing packets may be among those waiting packets.
            if (_octreeProcessor.hasPacketsToProcessFrom(nodeUUID)) {
                continue;
            }
            
            _octreeSceneStatsLock.lockForRead();

            // retreive octree scene stats of this node
            if (_octreeServerSceneStats.find(nodeUUID) == _octreeServerSceneStats.end()) {
                _octreeSceneStatsLock.unlock();
                continue;
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
                NodeList::getInstance()->writeUnverifiedDatagram(packet, dataAt - packet, node);
                packetsSent++;
            }
        }
    }
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
    _octreeQuery.setOctreeSizeScale(Menu::getInstance()->getVoxelSizeScale());
    _octreeQuery.setBoundaryLevelAdjust(Menu::getInstance()->getBoundaryLevelAdjust());

    unsigned char queryPacket[MAX_PACKET_SIZE];

    // Iterate all of the nodes, and get a count of how many voxel servers we have...
    int totalServers = 0;
    int inViewServers = 0;
    int unknownJurisdictionServers = 0;

    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
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
    }

    if (wantExtraDebugging) {
        qDebug("Servers: total %d, in view %d, unknown jurisdiction %d",
            totalServers, inViewServers, unknownJurisdictionServers);
    }

    int perServerPPS = 0;
    const int SMALL_BUDGET = 10;
    int perUnknownServer = SMALL_BUDGET;
    int totalPPS = Menu::getInstance()->getMaxVoxelPacketsPerSecond();

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

    NodeList* nodeList = NodeList::getInstance();

    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
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
            _bandwidthMeter.outputStream(BandwidthMeter::VOXELS).updateValue(packetLength);
        }
    }
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
    return glm::normalize(_environment.getClosestData(_myCamera.getPosition()).getSunLocation(_myCamera.getPosition()) -
        _myCamera.getPosition());
}

void Application::updateShadowMap() {
    PerformanceTimer perfTimer("shadowMap");
    QOpenGLFramebufferObject* fbo = _textureCache.getShadowFramebufferObject();
    fbo->bind();
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        float nearScale = SHADOW_MATRIX_DISTANCES[i] * frustumScale;
        float farScale = SHADOW_MATRIX_DISTANCES[i + 1] * frustumScale;
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

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.1f, 4.0f); // magic numbers courtesy http://www.eecs.berkeley.edu/~ravir/6160/papers/shadowmaps.ppt

        {
            PerformanceTimer perfTimer("avatarManager");
            _avatarManager.renderAvatars(Avatar::SHADOW_RENDER_MODE);
        }

        {
            PerformanceTimer perfTimer("particles");
            _particles.render(OctreeRenderer::SHADOW_RENDER_MODE);
        }

        {
            PerformanceTimer perfTimer("entities");
            _entities.render(OctreeRenderer::SHADOW_RENDER_MODE);
        }

        glDisable(GL_POLYGON_OFFSET_FILL);

        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glMatrixMode(GL_MODELVIEW);
    }
    
    fbo->release();

    glViewport(0, 0, _glWidget->getDeviceWidth(), _glWidget->getDeviceHeight());
}

const GLfloat WORLD_AMBIENT_COLOR[] = { 0.525f, 0.525f, 0.6f };
const GLfloat WORLD_DIFFUSE_COLOR[] = { 0.6f, 0.525f, 0.525f };
const GLfloat WORLD_SPECULAR_COLOR[] = { 0.94f, 0.94f, 0.737f, 1.0f };
const GLfloat NO_SPECULAR_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };

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

QImage Application::renderAvatarBillboard() {
    _textureCache.getPrimaryFramebufferObject()->bind();

    glDisable(GL_BLEND);

    const int BILLBOARD_SIZE = 64;
    renderRearViewMirror(QRect(0, _glWidget->getDeviceHeight() - BILLBOARD_SIZE, BILLBOARD_SIZE, BILLBOARD_SIZE), true);

    QImage image(BILLBOARD_SIZE, BILLBOARD_SIZE, QImage::Format_ARGB32);
    glReadPixels(0, 0, BILLBOARD_SIZE, BILLBOARD_SIZE, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());

    glEnable(GL_BLEND);

    _textureCache.getPrimaryFramebufferObject()->release();

    return image;
}

void Application::displaySide(Camera& whichCamera, bool selfAvatarOnly) {
    PerformanceTimer perfTimer("display");
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "Application::displaySide()");
    // transform by eye offset

    // load the view frustum
    loadViewFrustum(whichCamera, _displayViewFrustum);

    // flip x if in mirror mode (also requires reversing winding order for backface culling)
    if (whichCamera.getMode() == CAMERA_MODE_MIRROR) {
        glScalef(-1.0f, 1.0f, 1.0f);
        glFrontFace(GL_CW);

    } else {
        glFrontFace(GL_CCW);
    }

    glm::vec3 eyeOffsetPos = whichCamera.getEyeOffsetPosition();
    glm::quat eyeOffsetOrient = whichCamera.getEyeOffsetOrientation();
    glm::vec3 eyeOffsetAxis = glm::axis(eyeOffsetOrient);
    glRotatef(-glm::degrees(glm::angle(eyeOffsetOrient)), eyeOffsetAxis.x, eyeOffsetAxis.y, eyeOffsetAxis.z);
    glTranslatef(-eyeOffsetPos.x, -eyeOffsetPos.y, -eyeOffsetPos.z);

    // transform view according to whichCamera
    // could be myCamera (if in normal mode)
    // or could be viewFrustumOffsetCamera if in offset mode

    glm::quat rotation = whichCamera.getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(-glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

    // store view matrix without translation, which we'll use for precision-sensitive objects
    updateUntranslatedViewMatrix(-whichCamera.getPosition());

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
            const EnvironmentData& closestData = _environment.getClosestData(whichCamera.getPosition());
            float height = glm::distance(whichCamera.getPosition(),
                closestData.getAtmosphereCenter(whichCamera.getPosition()));
            if (height < closestData.getAtmosphereInnerRadius()) {
                alpha = 0.0f;

            } else if (height < closestData.getAtmosphereOuterRadius()) {
                alpha = (height - closestData.getAtmosphereInnerRadius()) /
                    (closestData.getAtmosphereOuterRadius() - closestData.getAtmosphereInnerRadius());
            }
        }

        // finally render the starfield
        _stars.render(whichCamera.getFieldOfView(), whichCamera.getAspectRatio(), whichCamera.getNearClip(), alpha);
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::Wireframe)) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // draw the sky dome
    if (!selfAvatarOnly && Menu::getInstance()->isOptionChecked(MenuOption::Atmosphere)) {
        PerformanceTimer perfTimer("atmosphere");
        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
            "Application::displaySide() ... atmosphere...");
        _environment.renderAtmospheres(whichCamera);
    }
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    if (!selfAvatarOnly) {
        // draw a red sphere
        float originSphereRadius = 0.05f;
        glColor3f(1,0,0);
        glPushMatrix();
            glutSolidSphere(originSphereRadius, 15, 15);
        glPopMatrix();

        // disable specular lighting for ground and voxels
        glMaterialfv(GL_FRONT, GL_SPECULAR, NO_SPECULAR_COLOR);

        // draw the audio reflector overlay
        {
            PerformanceTimer perfTimer("audio");
            _audioReflector.render();
        }
        
        _deferredLightingEffect.prepare();
        bool deferredLightingRequired = false;
        
        //  Draw voxels
        if (Menu::getInstance()->isOptionChecked(MenuOption::Voxels)) {
            PerformanceTimer perfTimer("voxels");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... voxels...");
            _voxels.render();
            deferredLightingRequired = true;
        }

        // also, metavoxels
        if (Menu::getInstance()->isOptionChecked(MenuOption::Metavoxels)) {
            PerformanceTimer perfTimer("metavoxels");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... metavoxels...");
            _metavoxels.render();
            deferredLightingRequired = true;
        }

        if (deferredLightingRequired) {
            _deferredLightingEffect.render();
        }

        if (Menu::getInstance()->isOptionChecked(MenuOption::BuckyBalls)) {
            PerformanceTimer perfTimer("buckyBalls");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... bucky balls...");
            _buckyBalls.render();
        }

        // render particles...
        if (Menu::getInstance()->isOptionChecked(MenuOption::Particles)) {
            PerformanceTimer perfTimer("particles");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... particles...");
            _particles.render();
        }

        // render models...
        if (Menu::getInstance()->isOptionChecked(MenuOption::Models)) {
            PerformanceTimer perfTimer("entities");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... entities...");
            _entities.render();
        }

        // render the ambient occlusion effect if enabled
        if (Menu::getInstance()->isOptionChecked(MenuOption::AmbientOcclusion)) {
            PerformanceTimer perfTimer("ambientOcclusion");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... AmbientOcclusion...");
            _ambientOcclusionEffect.render();
        }

        // restore default, white specular
        glMaterialfv(GL_FRONT, GL_SPECULAR, WORLD_SPECULAR_COLOR);

        _nodeBoundsDisplay.draw();

    }

    bool mirrorMode = (whichCamera.getInterpolatedMode() == CAMERA_MODE_MIRROR);
    {
        PerformanceTimer perfTimer("avatars");
        
        _avatarManager.renderAvatars(mirrorMode ? Avatar::MIRROR_RENDER_MODE : Avatar::NORMAL_RENDER_MODE, selfAvatarOnly);

        //Render the sixense lasers
        if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseLasers)) {
            _myAvatar->renderLaserPointers();
        }
    }

    if (!selfAvatarOnly) {
        //  Render the world box
        if (whichCamera.getMode() != CAMERA_MODE_MIRROR && Menu::getInstance()->isOptionChecked(MenuOption::Stats) && 
                Menu::getInstance()->isOptionChecked(MenuOption::UserInterface)) {
            PerformanceTimer perfTimer("worldBox");
            renderWorldBox();
        }

        // view frustum for debugging
        if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayFrustum) && whichCamera.getMode() != CAMERA_MODE_MIRROR) {
            PerformanceTimer perfTimer("viewFrustum");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... renderViewFrustum...");
            renderViewFrustum(_viewFrustum);
        }

        // render voxel fades if they exist
        if (_voxelFades.size() > 0) {
            PerformanceTimer perfTimer("voxelFades");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... voxel fades...");
            _voxelFadesLock.lockForWrite();
            for(std::vector<VoxelFade>::iterator fade = _voxelFades.begin(); fade != _voxelFades.end();) {
                fade->render();
                if(fade->isDone()) {
                    fade = _voxelFades.erase(fade);
                } else {
                    ++fade;
                }
            }
            _voxelFadesLock.unlock();
        }

        // give external parties a change to hook in
        {
            PerformanceTimer perfTimer("inWorldInterface");
            emit renderingInWorldInterface();
        }

        // render JS/scriptable overlays
        {
            PerformanceTimer perfTimer("3dOverlays");
            _overlays.render3D();
        }
    }
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::Wireframe)) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Application::updateUntranslatedViewMatrix(const glm::vec3& viewMatrixTranslation) {
    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&_untranslatedViewMatrix);
    _viewMatrixTranslation = viewMatrixTranslation;
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
    _viewFrustum.computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    if (OculusManager::isConnected()) {
        OculusManager::overrideOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    
    } else if (TV3DManager::isConnected()) {
        TV3DManager::overrideOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);    
    }
}

glm::vec2 Application::getScaledScreenPoint(glm::vec2 projectedPoint) {
    float horizontalScale = _glWidget->getDeviceWidth() / 2.0f;
    float verticalScale   = _glWidget->getDeviceHeight() / 2.0f;

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
        ((projectedPoint.y + 1.0) * -verticalScale) + _glWidget->getDeviceHeight());

    return screenPoint;
}

void Application::renderRearViewMirror(const QRect& region, bool billboard) {
    bool eyeRelativeCamera = false;
    if (billboard) {
        _mirrorCamera.setFieldOfView(BILLBOARD_FIELD_OF_VIEW);  // degees
        _mirrorCamera.setDistance(BILLBOARD_DISTANCE * _myAvatar->getScale());
        _mirrorCamera.setTargetPosition(_myAvatar->getPosition());

    } else if (_rearMirrorTools->getZoomLevel() == BODY) {
        _mirrorCamera.setFieldOfView(MIRROR_FIELD_OF_VIEW);     // degrees
        _mirrorCamera.setDistance(MIRROR_REARVIEW_BODY_DISTANCE * _myAvatar->getScale());
        _mirrorCamera.setTargetPosition(_myAvatar->getChestPosition());

    } else { // HEAD zoom level
        _mirrorCamera.setFieldOfView(MIRROR_FIELD_OF_VIEW);     // degrees
        _mirrorCamera.setDistance(MIRROR_REARVIEW_DISTANCE * _myAvatar->getScale());
        if (_myAvatar->getSkeletonModel().isActive() && _myAvatar->getHead()->getFaceModel().isActive()) {
            // as a hack until we have a better way of dealing with coordinate precision issues, reposition the
            // face/body so that the average eye position lies at the origin
            eyeRelativeCamera = true;
            _mirrorCamera.setTargetPosition(glm::vec3());

        } else {
            _mirrorCamera.setTargetPosition(_myAvatar->getHead()->getEyePosition());
        }
    }
    _mirrorCamera.setAspectRatio((float)region.width() / region.height());

    _mirrorCamera.setTargetRotation(_myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI, 0.0f)));
    _mirrorCamera.update(1.0f/_fps);

    // set the bounds of rear mirror view
    if (billboard) {
        glViewport(region.x(), _glWidget->getDeviceHeight() - region.y() - region.height(), region.width(), region.height());
        glScissor(region.x(), _glWidget->getDeviceHeight() - region.y() - region.height(), region.width(), region.height());    
    } else {
        // if not rendering the billboard, the region is in device independent coordinates; must convert to device
        float ratio = QApplication::desktop()->windowHandle()->devicePixelRatio();
        int x = region.x() * ratio, y = region.y() * ratio, width = region.width() * ratio, height = region.height() * ratio;
        glViewport(x, _glWidget->getDeviceHeight() - y - height, width, height);
        glScissor(x, _glWidget->getDeviceHeight() - y - height, width, height);
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
    glViewport(0, 0, _glWidget->getDeviceWidth(), _glWidget->getDeviceHeight());
    glDisable(GL_SCISSOR_TEST);
    updateProjectionMatrix(_myCamera, updateViewFrustum);
}

// renderViewFrustum()
//
// Description: this will render the view frustum bounds for EITHER the head
//                 or the "myCamera".
//
// Frustum rendering mode. For debug purposes, we allow drawing the frustum in a couple of different ways.
// We can draw it with each of these parts:
//    * Origin Direction/Up/Right vectors - these will be drawn at the point of the camera
//    * Near plane - this plane is drawn very close to the origin point.
//    * Right/Left planes - these two planes are drawn between the near and far planes.
//    * Far plane - the plane is drawn in the distance.
// Modes - the following modes, will draw the following parts.
//    * All - draws all the parts listed above
//    * Planes - draws the planes but not the origin vectors
//    * Origin Vectors - draws the origin vectors ONLY
//    * Near Plane - draws only the near plane
//    * Far Plane - draws only the far plane
void Application::renderViewFrustum(ViewFrustum& viewFrustum) {
    // Load it with the latest details!
    loadViewFrustum(_myCamera, viewFrustum);

    glm::vec3 position  = viewFrustum.getOffsetPosition();
    glm::vec3 direction = viewFrustum.getOffsetDirection();
    glm::vec3 up        = viewFrustum.getOffsetUp();
    glm::vec3 right     = viewFrustum.getOffsetRight();

    //  Get ready to draw some lines
    glDisable(GL_LIGHTING);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glLineWidth(1.0);
    glBegin(GL_LINES);

    if (Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_ALL
        || Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_VECTORS) {
        // Calculate the origin direction vectors
        glm::vec3 lookingAt      = position + (direction * 0.2f);
        glm::vec3 lookingAtUp    = position + (up * 0.2f);
        glm::vec3 lookingAtRight = position + (right * 0.2f);

        // Looking At = white
        glColor3f(1,1,1);
        glVertex3f(position.x, position.y, position.z);
        glVertex3f(lookingAt.x, lookingAt.y, lookingAt.z);

        // Looking At Up = purple
        glColor3f(1,0,1);
        glVertex3f(position.x, position.y, position.z);
        glVertex3f(lookingAtUp.x, lookingAtUp.y, lookingAtUp.z);

        // Looking At Right = cyan
        glColor3f(0,1,1);
        glVertex3f(position.x, position.y, position.z);
        glVertex3f(lookingAtRight.x, lookingAtRight.y, lookingAtRight.z);
    }

    if (Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_ALL
        || Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_PLANES
        || Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_NEAR_PLANE) {
        // Drawing the bounds of the frustum
        // viewFrustum.getNear plane - bottom edge
        glColor3f(1,0,0);
        glVertex3f(viewFrustum.getNearBottomLeft().x, viewFrustum.getNearBottomLeft().y, viewFrustum.getNearBottomLeft().z);
        glVertex3f(viewFrustum.getNearBottomRight().x, viewFrustum.getNearBottomRight().y, viewFrustum.getNearBottomRight().z);

        // viewFrustum.getNear plane - top edge
        glVertex3f(viewFrustum.getNearTopLeft().x, viewFrustum.getNearTopLeft().y, viewFrustum.getNearTopLeft().z);
        glVertex3f(viewFrustum.getNearTopRight().x, viewFrustum.getNearTopRight().y, viewFrustum.getNearTopRight().z);

        // viewFrustum.getNear plane - right edge
        glVertex3f(viewFrustum.getNearBottomRight().x, viewFrustum.getNearBottomRight().y, viewFrustum.getNearBottomRight().z);
        glVertex3f(viewFrustum.getNearTopRight().x, viewFrustum.getNearTopRight().y, viewFrustum.getNearTopRight().z);

        // viewFrustum.getNear plane - left edge
        glVertex3f(viewFrustum.getNearBottomLeft().x, viewFrustum.getNearBottomLeft().y, viewFrustum.getNearBottomLeft().z);
        glVertex3f(viewFrustum.getNearTopLeft().x, viewFrustum.getNearTopLeft().y, viewFrustum.getNearTopLeft().z);
    }

    if (Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_ALL
        || Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_PLANES
        || Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_FAR_PLANE) {
        // viewFrustum.getFar plane - bottom edge
        glColor3f(0,1,0);
        glVertex3f(viewFrustum.getFarBottomLeft().x, viewFrustum.getFarBottomLeft().y, viewFrustum.getFarBottomLeft().z);
        glVertex3f(viewFrustum.getFarBottomRight().x, viewFrustum.getFarBottomRight().y, viewFrustum.getFarBottomRight().z);

        // viewFrustum.getFar plane - top edge
        glVertex3f(viewFrustum.getFarTopLeft().x, viewFrustum.getFarTopLeft().y, viewFrustum.getFarTopLeft().z);
        glVertex3f(viewFrustum.getFarTopRight().x, viewFrustum.getFarTopRight().y, viewFrustum.getFarTopRight().z);

        // viewFrustum.getFar plane - right edge
        glVertex3f(viewFrustum.getFarBottomRight().x, viewFrustum.getFarBottomRight().y, viewFrustum.getFarBottomRight().z);
        glVertex3f(viewFrustum.getFarTopRight().x, viewFrustum.getFarTopRight().y, viewFrustum.getFarTopRight().z);

        // viewFrustum.getFar plane - left edge
        glVertex3f(viewFrustum.getFarBottomLeft().x, viewFrustum.getFarBottomLeft().y, viewFrustum.getFarBottomLeft().z);
        glVertex3f(viewFrustum.getFarTopLeft().x, viewFrustum.getFarTopLeft().y, viewFrustum.getFarTopLeft().z);
    }

    if (Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_ALL
        || Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_PLANES) {
        // RIGHT PLANE IS CYAN
        // right plane - bottom edge - viewFrustum.getNear to distant
        glColor3f(0,1,1);
        glVertex3f(viewFrustum.getNearBottomRight().x, viewFrustum.getNearBottomRight().y, viewFrustum.getNearBottomRight().z);
        glVertex3f(viewFrustum.getFarBottomRight().x, viewFrustum.getFarBottomRight().y, viewFrustum.getFarBottomRight().z);

        // right plane - top edge - viewFrustum.getNear to distant
        glVertex3f(viewFrustum.getNearTopRight().x, viewFrustum.getNearTopRight().y, viewFrustum.getNearTopRight().z);
        glVertex3f(viewFrustum.getFarTopRight().x, viewFrustum.getFarTopRight().y, viewFrustum.getFarTopRight().z);

        // LEFT PLANE IS BLUE
        // left plane - bottom edge - viewFrustum.getNear to distant
        glColor3f(0,0,1);
        glVertex3f(viewFrustum.getNearBottomLeft().x, viewFrustum.getNearBottomLeft().y, viewFrustum.getNearBottomLeft().z);
        glVertex3f(viewFrustum.getFarBottomLeft().x, viewFrustum.getFarBottomLeft().y, viewFrustum.getFarBottomLeft().z);

        // left plane - top edge - viewFrustum.getNear to distant
        glVertex3f(viewFrustum.getNearTopLeft().x, viewFrustum.getNearTopLeft().y, viewFrustum.getNearTopLeft().z);
        glVertex3f(viewFrustum.getFarTopLeft().x, viewFrustum.getFarTopLeft().y, viewFrustum.getFarTopLeft().z);

        // focal plane - bottom edge
        glColor3f(1.0f, 0.0f, 1.0f);
        float focalProportion = (viewFrustum.getFocalLength() - viewFrustum.getNearClip()) /
            (viewFrustum.getFarClip() - viewFrustum.getNearClip());
        glm::vec3 focalBottomLeft = glm::mix(viewFrustum.getNearBottomLeft(), viewFrustum.getFarBottomLeft(), focalProportion);
        glm::vec3 focalBottomRight = glm::mix(viewFrustum.getNearBottomRight(),
            viewFrustum.getFarBottomRight(), focalProportion);
        glVertex3f(focalBottomLeft.x, focalBottomLeft.y, focalBottomLeft.z);
        glVertex3f(focalBottomRight.x, focalBottomRight.y, focalBottomRight.z);

        // focal plane - top edge
        glm::vec3 focalTopLeft = glm::mix(viewFrustum.getNearTopLeft(), viewFrustum.getFarTopLeft(), focalProportion);
        glm::vec3 focalTopRight = glm::mix(viewFrustum.getNearTopRight(), viewFrustum.getFarTopRight(), focalProportion);
        glVertex3f(focalTopLeft.x, focalTopLeft.y, focalTopLeft.z);
        glVertex3f(focalTopRight.x, focalTopRight.y, focalTopRight.z);

        // focal plane - left edge
        glVertex3f(focalBottomLeft.x, focalBottomLeft.y, focalBottomLeft.z);
        glVertex3f(focalTopLeft.x, focalTopLeft.y, focalTopLeft.z);

        // focal plane - right edge
        glVertex3f(focalBottomRight.x, focalBottomRight.y, focalBottomRight.z);
        glVertex3f(focalTopRight.x, focalTopRight.y, focalTopRight.z);
    }
    glEnd();
    glEnable(GL_LIGHTING);

    if (Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_ALL
        || Menu::getInstance()->getFrustumDrawMode() == FRUSTUM_DRAW_MODE_KEYHOLE) {
        // Draw the keyhole
        float keyholeRadius = viewFrustum.getKeyholeRadius();
        if (keyholeRadius > 0.0f) {
            glPushMatrix();
            glColor4f(1, 1, 0, 1);
            glTranslatef(position.x, position.y, position.z); // where we actually want it!
            glutWireSphere(keyholeRadius, 20, 20);
            glPopMatrix();
        }
    }
}

void Application::deleteVoxels(const VoxelDetail& voxel) {
    deleteVoxelAt(voxel);
}

void Application::deleteVoxelAt(const VoxelDetail& voxel) {
    if (voxel.s != 0) {
        // sending delete to the server is sufficient, server will send new version so we see updates soon enough
        _voxelEditSender.sendVoxelEditMessage(PacketTypeVoxelErase, voxel);

        // delete it locally to see the effect immediately (and in case no voxel server is present)
        _voxels.getTree()->deleteVoxelAt(voxel.x, voxel.y, voxel.z, voxel.s);
    }
}


void Application::resetSensors() {
    _mouseX = _glWidget->width() / 2;
    _mouseY = _glWidget->height() / 2;

    _faceplus.reset();
    _faceshift.reset();
    _visage.reset();
    _dde.reset();

    OculusManager::reset();

    _prioVR.reset();
    //_leapmotion.reset();

    QCursor::setPos(_mouseX, _mouseY);
    _myAvatar->reset();

    QMetaObject::invokeMethod(&_audio, "reset", Qt::QueuedConnection);
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

void Application::uploadModel(ModelType modelType) {
    ModelUploader* uploader = new ModelUploader(modelType);
    QThread* thread = new QThread();
    thread->connect(uploader, SIGNAL(destroyed()), SLOT(quit()));
    thread->connect(thread, SIGNAL(finished()), SLOT(deleteLater()));
    uploader->connect(thread, SIGNAL(started()), SLOT(send()));

    thread->start();
}

void Application::updateWindowTitle(){

    QString buildVersion = " (build " + applicationVersion() + ")";
    NodeList* nodeList = NodeList::getInstance();

    QString connectionStatus = nodeList->getDomainHandler().isConnected() ? "" : " (NOT CONNECTED) ";
    QString username = AccountManager::getInstance().getAccountInfo().getUsername();
    QString title = QString() + (!username.isEmpty() ? username + " @ " : QString())
        + nodeList->getDomainHandler().getHostname() + connectionStatus + buildVersion;

    AccountManager& accountManager = AccountManager::getInstance();
    if (accountManager.getAccountInfo().hasBalance()) {
        float creditBalance = accountManager.getAccountInfo().getBalance() / SATOSHIS_PER_CREDIT;

        QString creditBalanceString;
        creditBalanceString.sprintf("%.8f", creditBalance);

        title += " - ₵" + creditBalanceString;
    }

#ifndef WIN32
    // crashes with vs2013/win32
    qDebug("Application title set to: %s", title.toStdString().c_str());
#endif
    _window->setWindowTitle(title);
}

void Application::updateLocationInServer() {

    AccountManager& accountManager = AccountManager::getInstance();
    const QUuid& domainUUID = NodeList::getInstance()->getDomainHandler().getUUID();
    
    if (accountManager.isLoggedIn() && !domainUUID.isNull()) {

        // construct a QJsonObject given the user's current address information
        QJsonObject rootObject;

        QJsonObject locationObject;
        
        QString pathString = AddressManager::pathForPositionAndOrientation(_myAvatar->getPosition(),
                                                                           true,
                                                                           _myAvatar->getOrientation());
       
        const QString LOCATION_KEY_IN_ROOT = "location";
        const QString PATH_KEY_IN_LOCATION = "path";
        const QString DOMAIN_ID_KEY_IN_LOCATION = "domain_id";
        
        locationObject.insert(PATH_KEY_IN_LOCATION, pathString);
        locationObject.insert(DOMAIN_ID_KEY_IN_LOCATION, domainUUID.toString());

        rootObject.insert(LOCATION_KEY_IN_ROOT, locationObject);

        accountManager.authenticatedRequest("/api/v1/users/location", QNetworkAccessManager::PutOperation,
                                            JSONCallbackParameters(), QJsonDocument(rootObject).toJson());
     }
}

void Application::changeDomainHostname(const QString &newDomainHostname) {
    NodeList* nodeList = NodeList::getInstance();
    
    if (!nodeList->getDomainHandler().isCurrentHostname(newDomainHostname)) {
        // tell the MyAvatar object to send a kill packet so that it dissapears from its old avatar mixer immediately
        _myAvatar->sendKillAvatar();
        
        // call the domain hostname change as a queued connection on the nodelist
        QMetaObject::invokeMethod(&NodeList::getInstance()->getDomainHandler(), "setHostname",
                                  Q_ARG(const QString&, newDomainHostname));
    }
}

void Application::domainChanged(const QString& domainHostname) {
    updateWindowTitle();

    // reset the environment so that we don't erroneously end up with multiple
    _environment.resetToDefault();

    // reset our node to stats and node to jurisdiction maps... since these must be changing...
    _voxelServerJurisdictions.lockForWrite();
    _voxelServerJurisdictions.clear();
    _voxelServerJurisdictions.unlock();

    _octreeServerSceneStats.clear();

    _particleServerJurisdictions.lockForWrite();
    _particleServerJurisdictions.clear();
    _particleServerJurisdictions.unlock();

    // reset the particle renderer
    _particles.clear();

    // reset the model renderer
    _entities.clear();

    // reset the voxels renderer
    _voxels.killLocalVoxels();

    // reset the auth URL for OAuth web view handler
    OAuthWebViewHandler::getInstance().clearLastAuthorizationURL();
}

void Application::connectedToDomain(const QString& hostname) {
    AccountManager& accountManager = AccountManager::getInstance();

    if (accountManager.isLoggedIn()) {
        // update our domain-server with the data-server we're logged in with

        QString domainPutJsonString = "{\"address\":{\"domain\":\"" + hostname + "\"}}";

        accountManager.authenticatedRequest("/api/v1/users/address", QNetworkAccessManager::PutOperation,
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

    _voxelEditSender.nodeKilled(node);
    _particleEditSender.nodeKilled(node);
    _entityEditSender.nodeKilled(node);

    if (node->getType() == NodeType::AudioMixer) {
        QMetaObject::invokeMethod(&_audio, "audioMixerKilled");
    }

    if (node->getType() == NodeType::VoxelServer) {
        QUuid nodeUUID = node->getUUID();
        // see if this is the first we've heard of this node...
        _voxelServerJurisdictions.lockForRead();
        if (_voxelServerJurisdictions.find(nodeUUID) != _voxelServerJurisdictions.end()) {
            unsigned char* rootCode = _voxelServerJurisdictions[nodeUUID].getRootOctalCode();
            VoxelPositionSize rootDetails;
            voxelDetailsForCode(rootCode, rootDetails);
            _voxelServerJurisdictions.unlock();

            qDebug("voxel server going away...... v[%f, %f, %f, %f]",
                rootDetails.x, rootDetails.y, rootDetails.z, rootDetails.s);

            // Add the jurisditionDetails object to the list of "fade outs"
            if (!Menu::getInstance()->isOptionChecked(MenuOption::DontFadeOnVoxelServerChanges)) {
                VoxelFade fade(VoxelFade::FADE_OUT, NODE_KILLED_RED, NODE_KILLED_GREEN, NODE_KILLED_BLUE);
                fade.voxelDetails = rootDetails;
                const float slightly_smaller = 0.99f;
                fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
                _voxelFadesLock.lockForWrite();
                _voxelFades.push_back(fade);
                _voxelFadesLock.unlock();
            }

            // If the voxel server is going away, remove it from our jurisdiction map so we don't send voxels to a dead server
            _voxelServerJurisdictions.lockForWrite();
            _voxelServerJurisdictions.erase(_voxelServerJurisdictions.find(nodeUUID));
        }
        _voxelServerJurisdictions.unlock();

        // also clean up scene stats for that server
        _octreeSceneStatsLock.lockForWrite();
        if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
            _octreeServerSceneStats.erase(nodeUUID);
        }
        _octreeSceneStatsLock.unlock();

    } else if (node->getType() == NodeType::ParticleServer) {
        QUuid nodeUUID = node->getUUID();
        // see if this is the first we've heard of this node...
        _particleServerJurisdictions.lockForRead();
        if (_particleServerJurisdictions.find(nodeUUID) != _particleServerJurisdictions.end()) {
            unsigned char* rootCode = _particleServerJurisdictions[nodeUUID].getRootOctalCode();
            VoxelPositionSize rootDetails;
            voxelDetailsForCode(rootCode, rootDetails);
            _particleServerJurisdictions.unlock();

            qDebug("particle server going away...... v[%f, %f, %f, %f]",
                rootDetails.x, rootDetails.y, rootDetails.z, rootDetails.s);

            // Add the jurisditionDetails object to the list of "fade outs"
            if (!Menu::getInstance()->isOptionChecked(MenuOption::DontFadeOnVoxelServerChanges)) {
                VoxelFade fade(VoxelFade::FADE_OUT, NODE_KILLED_RED, NODE_KILLED_GREEN, NODE_KILLED_BLUE);
                fade.voxelDetails = rootDetails;
                const float slightly_smaller = 0.99f;
                fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
                _voxelFadesLock.lockForWrite();
                _voxelFades.push_back(fade);
                _voxelFadesLock.unlock();
            }

            // If the particle server is going away, remove it from our jurisdiction map so we don't send voxels to a dead server
            _particleServerJurisdictions.lockForWrite();
            _particleServerJurisdictions.erase(_particleServerJurisdictions.find(nodeUUID));
        }
        _particleServerJurisdictions.unlock();

        // also clean up scene stats for that server
        _octreeSceneStatsLock.lockForWrite();
        if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
            _octreeServerSceneStats.erase(nodeUUID);
        }
        _octreeSceneStatsLock.unlock();

    } else if (node->getType() == NodeType::EntityServer) {

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
            if (!Menu::getInstance()->isOptionChecked(MenuOption::DontFadeOnVoxelServerChanges)) {
                VoxelFade fade(VoxelFade::FADE_OUT, NODE_KILLED_RED, NODE_KILLED_GREEN, NODE_KILLED_BLUE);
                fade.voxelDetails = rootDetails;
                const float slightly_smaller = 0.99f;
                fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
                _voxelFadesLock.lockForWrite();
                _voxelFades.push_back(fade);
                _voxelFadesLock.unlock();
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

void Application::trackIncomingVoxelPacket(const QByteArray& packet, const SharedNodePointer& sendingNode, bool wasStatsPacket) {

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
        if (sendingNode->getType() == NodeType::VoxelServer) {
            jurisdiction = &_voxelServerJurisdictions;
            serverType = "Voxel";
        } else if (sendingNode->getType() == NodeType::ParticleServer) {
            jurisdiction = &_particleServerJurisdictions;
            serverType = "Particle";
        } else {
            jurisdiction = &_entityServerJurisdictions;
            serverType = "Entity";
        }

        jurisdiction->lockForRead();
        if (jurisdiction->find(nodeUUID) == jurisdiction->end()) {
            jurisdiction->unlock();

            qDebug("stats from new %s server... [%f, %f, %f, %f]",
                qPrintable(serverType), rootDetails.x, rootDetails.y, rootDetails.z, rootDetails.s);

            // Add the jurisditionDetails object to the list of "fade outs"
            if (!Menu::getInstance()->isOptionChecked(MenuOption::DontFadeOnVoxelServerChanges)) {
                VoxelFade fade(VoxelFade::FADE_OUT, NODE_ADDED_RED, NODE_ADDED_GREEN, NODE_ADDED_BLUE);
                fade.voxelDetails = rootDetails;
                const float slightly_smaller = 0.99f;
                fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
                _voxelFadesLock.lockForWrite();
                _voxelFades.push_back(fade);
                _voxelFadesLock.unlock();
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
    _bandwidthMeter.outputStream(BandwidthMeter::VOXELS).updateValue(length);
}

void Application::loadScripts() {
    // loads all saved scripts
    int size = lockSettings()->beginReadArray("Settings");
    unlockSettings();
    for (int i = 0; i < size; ++i){
        lockSettings()->setArrayIndex(i);
        QString string = _settings->value("script").toString();
        unlockSettings();
        if (!string.isEmpty()) {
            loadScript(string);
        }
    }

    QMutexLocker locker(&_settingsMutex);
    _settings->endArray();
}

void Application::clearScriptsBeforeRunning() {
    // clears all scripts from the settings
    QMutexLocker locker(&_settingsMutex);
    _settings->remove("Settings");
}

void Application::saveScripts() {
    // Saves all currently running user-loaded scripts
    QMutexLocker locker(&_settingsMutex);
    _settings->beginWriteArray("Settings");

    QStringList runningScripts = getRunningScripts();
    int i = 0;
    for (QStringList::const_iterator it = runningScripts.begin(); it != runningScripts.end(); it += 1) {
        if (getScriptEngine(*it)->isUserLoaded()) {
            _settings->setArrayIndex(i);
            _settings->setValue("script", *it);
            i += 1;
        }
    }

    _settings->endArray();
}

ScriptEngine* Application::loadScript(const QString& scriptFilename, bool isUserLoaded,
    bool loadScriptFromEditor, bool activateMainWindow) {
    QUrl scriptUrl(scriptFilename);
    const QString& scriptURLString = scriptUrl.toString();
    if (_scriptEnginesHash.contains(scriptURLString) && loadScriptFromEditor
        && !_scriptEnginesHash[scriptURLString]->isFinished()) {

        return _scriptEnginesHash[scriptURLString];
    }

    ScriptEngine* scriptEngine;
    if (scriptFilename.isNull()) {
        scriptEngine = new ScriptEngine(NO_SCRIPT, "", &_controllerScriptingInterface);
    } else {
        // start the script on a new thread...
        scriptEngine = new ScriptEngine(scriptUrl, &_controllerScriptingInterface);

        if (!scriptEngine->hasScript()) {
            qDebug() << "Application::loadScript(), script failed to load...";
            QMessageBox::warning(getWindow(), "Error Loading Script", scriptURLString + " failed to load.");
            return NULL;
        }

        _scriptEnginesHash.insertMulti(scriptURLString, scriptEngine);
        _runningScriptsWidget->setRunningScripts(getRunningScripts());
        UserActivityLogger::getInstance().loadedScript(scriptURLString);
    }
    scriptEngine->setUserLoaded(isUserLoaded);

    // setup the packet senders and jurisdiction listeners of the script engine's scripting interfaces so
    // we can use the same ones from the application.
    scriptEngine->getVoxelsScriptingInterface()->setPacketSender(&_voxelEditSender);
    scriptEngine->getVoxelsScriptingInterface()->setVoxelTree(_voxels.getTree());
    scriptEngine->getVoxelsScriptingInterface()->setUndoStack(&_undoStack);
    scriptEngine->getParticlesScriptingInterface()->setPacketSender(&_particleEditSender);
    scriptEngine->getParticlesScriptingInterface()->setParticleTree(_particles.getTree());

    scriptEngine->getEntityScriptingInterface()->setPacketSender(&_entityEditSender);
    scriptEngine->getEntityScriptingInterface()->setEntityTree(_entities.getTree());

    // model has some custom types
    Model::registerMetaTypes(scriptEngine);

    // hook our avatar object into this script engine
    scriptEngine->setAvatarData(_myAvatar, "MyAvatar"); // leave it as a MyAvatar class to expose thrust features

    CameraScriptableObject* cameraScriptable = new CameraScriptableObject(&_myCamera, &_viewFrustum);
    scriptEngine->registerGlobalObject("Camera", cameraScriptable);
    connect(scriptEngine, SIGNAL(finished(const QString&)), cameraScriptable, SLOT(deleteLater()));

#ifdef Q_OS_MAC
    scriptEngine->registerGlobalObject("SpeechRecognizer", Menu::getInstance()->getSpeechRecognizer());
#endif

    ClipboardScriptingInterface* clipboardScriptable = new ClipboardScriptingInterface();
    scriptEngine->registerGlobalObject("Clipboard", clipboardScriptable);
    connect(scriptEngine, SIGNAL(finished(const QString&)), clipboardScriptable, SLOT(deleteLater()));

    connect(scriptEngine, SIGNAL(finished(const QString&)), this, SLOT(scriptFinished(const QString&)));

    connect(scriptEngine, SIGNAL(loadScript(const QString&, bool)), this, SLOT(loadScript(const QString&, bool)));

    scriptEngine->registerGlobalObject("Overlays", &_overlays);

    QScriptValue windowValue = scriptEngine->registerGlobalObject("Window", WindowScriptingInterface::getInstance());
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                                       LocationScriptingInterface::locationSetter, windowValue);
    // register `location` on the global object.
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                                       LocationScriptingInterface::locationSetter);
    
    scriptEngine->registerGlobalObject("Menu", MenuScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("Settings", SettingsScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("AudioDevice", AudioDeviceScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("AnimationCache", &_animationCache);
    scriptEngine->registerGlobalObject("AudioReflector", &_audioReflector);
    scriptEngine->registerGlobalObject("Account", AccountScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("Metavoxels", &_metavoxels);
    
    scriptEngine->registerGlobalObject("AvatarManager", &_avatarManager);
    
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

    NodeList* nodeList = NodeList::getInstance();
    connect(nodeList, &NodeList::nodeKilled, scriptEngine, &ScriptEngine::nodeKilled);

    scriptEngine->moveToThread(workerThread);

    // Starts an event loop, and emits workerThread->started()
    workerThread->start();

    // restore the main window's active state
    if (activateMainWindow && !loadScriptFromEditor) {
        _window->activateWindow();
    }
    bumpSettings();

    return scriptEngine;
}

void Application::scriptFinished(const QString& scriptName) {
    const QString& scriptURLString = QUrl(scriptName).toString();
    QHash<QString, ScriptEngine*>::iterator it = _scriptEnginesHash.find(scriptURLString);
    if (it != _scriptEnginesHash.end()) {
        _scriptEnginesHash.erase(it);
        _runningScriptsWidget->scriptStopped(scriptName);
        _runningScriptsWidget->setRunningScripts(getRunningScripts());
        bumpSettings();
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
    _myAvatar->clearJointAnimationPriorities();
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
    uploadModel(HEAD_MODEL);
}

void Application::uploadSkeleton() {
    uploadModel(SKELETON_MODEL);
}

void Application::uploadAttachment() {
    uploadModel(ATTACHMENT_MODEL);
}

void Application::openUrl(const QUrl& url) {
    if (url.scheme() == HIFI_URL_SCHEME) {
        AddressManager::getInstance().handleLookupString(url.toString());
    } else {
        // address manager did not handle - ask QDesktopServices to handle
        QDesktopServices::openUrl(url);
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
    
    _voxelEditSender.setSatoshisPerVoxel(satoshisPerVoxel);
    _voxelEditSender.setSatoshisPerMeterCubed(satoshisPerMeterCubed);
    _voxelEditSender.setDestinationWalletUUID(voxelWalletUUID);
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
    QMutexLocker locker(&_settingsMutex);
    _settings->setValue("LastScriptLocation", _previousScriptLocation);
    bumpSettings();
}

void Application::loadDialog() {

    QString fileNameString = QFileDialog::getOpenFileName(_glWidget, tr("Open Script"),
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

    sendFakeEnterEvent();
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

void Application::initAvatarAndViewFrustum() {
    updateMyAvatar(0.f);
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
    QObject* sender = QObject::sender();

    QXmlStreamReader xml(qobject_cast<QNetworkReply*>(sender));

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
        new UpdateDialog(_glWidget, releaseNotes, latestVersion, downloadUrl);
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

void Application::setCursorVisible(bool visible) {
    if (visible) {
        restoreOverrideCursor();
    } else {
        setOverrideCursor(Qt::BlankCursor);
    }
}

void Application::takeSnapshot() {
    QMediaPlayer* player = new QMediaPlayer();
    QFileInfo inf = QFileInfo(Application::resourcesPath() + "sounds/snap.wav");
    player->setMedia(QUrl::fromLocalFile(inf.absoluteFilePath()));
    player->play();

    QString fileName = Snapshot::saveSnapshot(_glWidget, _myAvatar);

    AccountManager& accountManager = AccountManager::getInstance();
    if (!accountManager.isLoggedIn()) {
        return;
    }

    if (!_snapshotShareDialog) {
        _snapshotShareDialog = new SnapshotShareDialog(fileName, _glWidget);
    }
    _snapshotShareDialog->show();
}
