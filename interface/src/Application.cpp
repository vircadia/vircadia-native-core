//
//  Application.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <sstream>

#include <stdlib.h>
#include <cmath>

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#else
#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QActionGroup>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QCheckBox>
#include <QImage>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QOpenGLFramebufferObject>
#include <QWheelEvent>
#include <QSettings>
#include <QShortcut>
#include <QTimer>
#include <QUrl>
#include <QtDebug>
#include <QFileDialog>
#include <QDesktopServices>

#include <NodeTypes.h>
#include <AudioInjectionManager.h>
#include <AudioInjector.h>
#include <Logging.h>
#include <OctalCode.h>
#include <PacketHeaders.h>
#include <PairingHandler.h>
#include <PerfStat.h>
#include <UUID.h>
#include <VoxelSceneStats.h>

#include "Application.h"
#include "DataServerClient.h"
#include "LogDisplay.h"
#include "Menu.h"
#include "Swatch.h"
#include "Util.h"
#include "devices/LeapManager.h"
#include "devices/OculusManager.h"
#include "renderer/ProgramObject.h"
#include "ui/TextRenderer.h"
#include "InfoView.h"

using namespace std;

//  Starfield information
static unsigned STARFIELD_NUM_STARS = 50000;
static unsigned STARFIELD_SEED = 1;

static const int BANDWIDTH_METER_CLICK_MAX_DRAG_LENGTH = 6; // farther dragged clicks are ignored 

const int IDLE_SIMULATE_MSECS = 16;              //  How often should call simulate and other stuff
                                                 //  in the idle loop?  (60 FPS is default)
static QTimer* idleTimer = NULL;

const int STARTUP_JITTER_SAMPLES = PACKET_LENGTH_SAMPLES_PER_CHANNEL / 2;
                                                 //  Startup optimistically with small jitter buffer that 
                                                 //  will start playback on the second received audio packet.

const int MIRROR_VIEW_TOP_PADDING = 5;
const int MIRROR_VIEW_LEFT_PADDING = 10;
const int MIRROR_VIEW_WIDTH = 265;
const int MIRROR_VIEW_HEIGHT = 215;
const float MAX_ZOOM_DISTANCE = 0.3f;
const float MIN_ZOOM_DISTANCE = 2.0f;

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString &message) {
    fprintf(stdout, "%s", message.toLocal8Bit().constData());
    LogDisplay::instance.addMessage(message.toLocal8Bit().constData());
}

Application::Application(int& argc, char** argv, timeval &startup_time) :
        QApplication(argc, argv),
        _window(new QMainWindow(desktop())),
        _glWidget(new GLCanvas()),
        _displayLevels(false),
        _frameCount(0),
        _fps(120.0f),
        _justStarted(true),
        _voxelImporter(_window),
        _wantToKillLocalVoxels(false),
        _audioScope(256, 200, true),
        _profile(QString()),
        _mirrorViewRect(QRect(MIRROR_VIEW_LEFT_PADDING, MIRROR_VIEW_TOP_PADDING, MIRROR_VIEW_WIDTH, MIRROR_VIEW_HEIGHT)),
        _mouseX(0),
        _mouseY(0),
        _lastMouseMove(usecTimestampNow()),
        _mouseHidden(false),
        _seenMouseMove(false),
        _touchAvgX(0.0f),
        _touchAvgY(0.0f),
        _isTouchPressed(false),
        _yawFromTouch(0.0f),
        _pitchFromTouch(0.0f),
        _mousePressed(false),
        _isHoverVoxel(false),
        _isHoverVoxelSounding(false),
        _mouseVoxelScale(1.0f / 1024.0f),
        _mouseVoxelScaleInitialized(false),
        _justEditedVoxel(false),
        _nudgeStarted(false),
        _lookingAlongX(false),
        _lookingAwayFromOrigin(true),
        _lookatTargetAvatar(NULL),
        _lookatIndicatorScale(1.0f),
        _perfStatsOn(false),
        _chatEntryOn(false),
        _oculusProgram(0),
        _oculusDistortionScale(1.25),
#ifndef _WIN32
        _audio(&_audioScope, STARTUP_JITTER_SAMPLES),
#endif
        _stopNetworkReceiveThread(false),  
        _voxelProcessor(),
        _voxelEditSender(this),
        _packetCount(0),
        _packetsPerSecond(0),
        _bytesPerSecond(0),
        _bytesCount(0),
        _recentMaxPackets(0),
        _resetRecentMaxPacketsSoon(true),
        _swatch(NULL),
        _pasteMode(false)
{
    _applicationStartupTime = startup_time;
    _window->setWindowTitle("Interface");
    
    qInstallMessageHandler(messageHandler);
    
    unsigned int listenPort = 0; // bind to an ephemeral port by default
    const char** constArgv = const_cast<const char**>(argv);
    const char* portStr = getCmdOption(argc, constArgv, "--listenPort");
    if (portStr) {
        listenPort = atoi(portStr);
    }
    
    NodeList::createInstance(NODE_TYPE_AGENT, listenPort);
    
    NodeList::getInstance()->addHook(&_voxels);
    NodeList::getInstance()->addHook(this);
    NodeList::getInstance()->addDomainListener(this);
    NodeList::getInstance()->addDomainListener(&_voxels);

    
    // network receive thread and voxel parsing thread are both controlled by the --nonblocking command line
    _enableProcessVoxelsThread = _enableNetworkThread = !cmdOptionExists(argc, constArgv, "--nonblocking");
    if (!_enableNetworkThread) {
        NodeList::getInstance()->getNodeSocket()->setBlocking(false);
    }
    
    // setup QSettings    
#ifdef Q_OS_MAC
    QString resourcesPath = QCoreApplication::applicationDirPath() + "/../Resources";
#else
    QString resourcesPath = QCoreApplication::applicationDirPath() + "/resources";
#endif
    
    // read the ApplicationInfo.ini file for Name/Version/Domain information
    QSettings applicationInfo(resourcesPath + "/info/ApplicationInfo.ini", QSettings::IniFormat);
    
    // set the associated application properties
    applicationInfo.beginGroup("INFO");
    
    setApplicationName(applicationInfo.value("name").toString());
    setApplicationVersion(applicationInfo.value("version").toString());
    setOrganizationName(applicationInfo.value("organizationName").toString());
    setOrganizationDomain(applicationInfo.value("organizationDomain").toString());
    
    _settings = new QSettings(this);
    
    // call Menu getInstance static method to set up the menu
    _window->setMenuBar(Menu::getInstance());

    // Check to see if the user passed in a command line option for loading a local
    // Voxel File.
    _voxelsFilename = getCmdOption(argc, constArgv, "-i");
    
    // the callback for our instance of NodeList is attachNewHeadToNode
    NodeList::getInstance()->linkedDataCreateCallback = &attachNewHeadToNode;
    
    #ifdef _WIN32
    WSADATA WsaData;
    int wsaresult = WSAStartup(MAKEWORD(2,2), &WsaData);
    #endif
    
    // tell the NodeList instance who to tell the domain server we care about
    const char nodeTypesOfInterest[] = {NODE_TYPE_AUDIO_MIXER, NODE_TYPE_AVATAR_MIXER, NODE_TYPE_VOXEL_SERVER};
    NodeList::getInstance()->setNodeTypesOfInterest(nodeTypesOfInterest, sizeof(nodeTypesOfInterest));

    // start the nodeList threads
    NodeList::getInstance()->startSilentNodeRemovalThread();
    
    _networkAccessManager = new QNetworkAccessManager(this);
    QNetworkDiskCache* cache = new QNetworkDiskCache(_networkAccessManager);
    cache->setCacheDirectory("interfaceCache");
    _networkAccessManager->setCache(cache);
    
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
}

Application::~Application() {
    storeSizeAndPosition();
    NodeList::getInstance()->removeHook(&_voxels);
    NodeList::getInstance()->removeHook(this);
    NodeList::getInstance()->removeDomainListener(this);

    _sharedVoxelSystem.changeTree(new VoxelTree);

    _audio.shutdown();

    delete Menu::getInstance();
    
    delete _oculusProgram;
    delete _settings;
    delete _followMode;
    delete _glWidget;
}

void Application::restoreSizeAndPosition() {
    QSettings* settings = new QSettings(this);
    QRect available = desktop()->availableGeometry();
    
    settings->beginGroup("Window");
    
    int x = (int)loadSetting(settings, "x", 0);
    int y = (int)loadSetting(settings, "y", 0);
    _window->move(x, y);
    
    int width = (int)loadSetting(settings, "width", available.width());
    int height = (int)loadSetting(settings, "height", available.height());
    _window->resize(width, height);
    
    settings->endGroup();
}

void Application::storeSizeAndPosition() {
    QSettings* settings = new QSettings(this);
    
    settings->beginGroup("Window");
    
    settings->setValue("width", _window->rect().width());
    settings->setValue("height", _window->rect().height());
    
    settings->setValue("x", _window->pos().x());
    settings->setValue("y", _window->pos().y());
    
    settings->endGroup();
}

void Application::initializeGL() {
    qDebug( "Created Display Window.\n" );
    
    // initialize glut for shape drawing; Qt apparently initializes it on OS X
    #ifndef __APPLE__
    int argc = 0;
    glutInit(&argc, 0);
    #endif
    
    // Before we render anything, let's set up our viewFrustumOffsetCamera with a sufficiently large
    // field of view and near and far clip to make it interesting.
    //viewFrustumOffsetCamera.setFieldOfView(90.0);
    _viewFrustumOffsetCamera.setNearClip(0.1);
    _viewFrustumOffsetCamera.setFarClip(500.0 * TREE_SCALE);
    
    initDisplay();
    qDebug( "Initialized Display.\n" );
    
    init();
    qDebug( "Init() complete.\n" );
    
    // Check to see if the user passed in a command line option for randomizing colors
    bool wantColorRandomizer = !arguments().contains("--NoColorRandomizer");
    
    // Check to see if the user passed in a command line option for loading a local
    // Voxel File. If so, load it now.
    if (!_voxelsFilename.isEmpty()) {
        _voxels.loadVoxelsFile(_voxelsFilename.constData(), wantColorRandomizer);
        qDebug("Local Voxel File loaded.\n");
    }
    
    // create thread for receipt of data via UDP
    if (_enableNetworkThread) {
        pthread_create(&_networkReceiveThread, NULL, networkReceive, NULL);
        qDebug("Network receive thread created.\n"); 
    }

    // create thread for parsing of voxel data independent of the main network and rendering threads
    _voxelProcessor.initialize(_enableProcessVoxelsThread);
    _voxelEditSender.initialize(_enableProcessVoxelsThread);
    if (_enableProcessVoxelsThread) {
        qDebug("Voxel parsing thread created.\n");
    }
    
    // call terminate before exiting
    connect(this, SIGNAL(aboutToQuit()), SLOT(terminate()));
    
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
        float startupTime = (usecTimestampNow() - usecTimestamp(&_applicationStartupTime)) / 1000000.0;
        _justStarted = false;
        char title[50];
        sprintf(title, "Interface: %4.2f seconds\n", startupTime);
        qDebug("%s", title);
        const char LOGSTASH_INTERFACE_START_TIME_KEY[] = "interface-start-time";
        
        // ask the Logstash class to record the startup time
        Logging::stashValue(STAT_TYPE_TIMER, LOGSTASH_INTERFACE_START_TIME_KEY, startupTime);
    }
    
    // update before the first render
    update(0.0f);
    
    InfoView::showFirstTime();
}

void Application::paintGL() {
    PerformanceWarning::setSuppressShortTimings(Menu::getInstance()->isOptionChecked(MenuOption::SuppressShortTimings));
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::paintGL()");
    
    glEnable(GL_LINE_SMOOTH);

    if (OculusManager::isConnected()) {
        _myCamera.setUpShift       (0.0f);
        _myCamera.setDistance      (0.0f);
        _myCamera.setTightness     (0.0f);     //  Camera is directly connected to head without smoothing
        _myCamera.setTargetPosition(_myAvatar.getHead().calculateAverageEyePosition());
        _myCamera.setTargetRotation(_myAvatar.getHead().getOrientation());
    
    } else if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON) {
        _myCamera.setTightness(0.0f);  //  In first person, camera follows head exactly without delay
        _myCamera.setTargetPosition(_myAvatar.getHead().calculateAverageEyePosition());
        _myCamera.setTargetRotation(_myAvatar.getHead().getCameraOrientation());
        
    } else if (_myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
        _myCamera.setTargetPosition(_myAvatar.getUprightHeadPosition());
        _myCamera.setTargetRotation(_myAvatar.getHead().getCameraOrientation());
    
    } else if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        _myCamera.setTightness(0.0f);
        _myCamera.setDistance(MAX_ZOOM_DISTANCE);
        _myCamera.setTargetPosition(_myAvatar.getHead().calculateAverageEyePosition());
        _myCamera.setTargetRotation(_myAvatar.getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PIf, 0.0f)));
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

    if (OculusManager::isConnected()) {
        displayOculus(whichCamera);
        
    } else {
        _glowEffect.prepare(); 

        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        displaySide(whichCamera);
        glPopMatrix();
        
        _glowEffect.render();
        
        if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
            
            if (_rearMirrorTools->getZoomLevel() == BODY) {
                _mirrorCamera.setDistance(MIN_ZOOM_DISTANCE);
                _mirrorCamera.setTargetPosition(_myAvatar.getChestJointPosition());
            } else { // HEAD zoom level
                _mirrorCamera.setDistance(MAX_ZOOM_DISTANCE);
                _mirrorCamera.setTargetPosition(_myAvatar.getHead().calculateAverageEyePosition());
            }
            
            _mirrorCamera.setTargetRotation(_myAvatar.getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PIf, 0.0f)));
            _mirrorCamera.update(1.0f/_fps);
            
            // set the bounds of rear mirror view
            glViewport(_mirrorViewRect.x(), _glWidget->height() - _mirrorViewRect.y() - _mirrorViewRect.height(), _mirrorViewRect.width(), _mirrorViewRect.height());
            glScissor(_mirrorViewRect.x(), _glWidget->height() - _mirrorViewRect.y() - _mirrorViewRect.height(), _mirrorViewRect.width(), _mirrorViewRect.height());
            bool updateViewFrustum = false;
            updateProjectionMatrix(_mirrorCamera, updateViewFrustum);
            glEnable(GL_SCISSOR_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            // render rear mirror view
            glPushMatrix();
            bool selfAvatarOnly = true;
            displaySide(_mirrorCamera, selfAvatarOnly);
            glPopMatrix();
            
            _rearMirrorTools->render(false);
            
            // reset Viewport and projection matrix
            glViewport(0, 0, _glWidget->width(), _glWidget->height());
            glDisable(GL_SCISSOR_TEST);
            updateProjectionMatrix(_myCamera, updateViewFrustum);
        } else if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
            _rearMirrorTools->render(true);
        }
        
        displayOverlay();
    }
    
    _frameCount++;
}

void Application::resetCamerasOnResizeGL(Camera& camera, int width, int height) {
    float aspectRatio = ((float)width/(float)height); // based on screen resize
    
    if (OculusManager::isConnected()) {
        // more magic numbers; see Oculus SDK docs, p. 32
        camera.setAspectRatio(aspectRatio *= 0.5);
        camera.setFieldOfView(2 * atan((0.0468 * _oculusDistortionScale) / 0.041) * (180 / PIf));
    } else {
        camera.setAspectRatio(aspectRatio);
        camera.setFieldOfView(Menu::getInstance()->getFieldOfView());
    }
}

void Application::resizeGL(int width, int height) {
    resetCamerasOnResizeGL(_viewFrustumOffsetCamera, width, height);
    resetCamerasOnResizeGL(_myCamera, width, height);
    
    glViewport(0, 0, width, height); // shouldn't this account for the menu???

    updateProjectionMatrix();
    glLoadIdentity();
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
    
    glMatrixMode(GL_MODELVIEW);
}

void Application::resetProfile(const QString& username) {
    // call the destructor on the old profile and construct a new one
    (&_profile)->~Profile();
    new (&_profile) Profile(username);
    updateWindowTitle();
}

void Application::controlledBroadcastToNodes(unsigned char* broadcastData, size_t dataBytes, 
                                             const char* nodeTypes, int numNodeTypes) {
    Application* self = getInstance();
    for (int i = 0; i < numNodeTypes; ++i) {

        // Intercept data to voxel server when voxels are disabled
        if (nodeTypes[i] == NODE_TYPE_VOXEL_SERVER && !Menu::getInstance()->isOptionChecked(MenuOption::Voxels)) {
            continue;
        }
        
        // Perform the broadcast for one type
        int nReceivingNodes = NodeList::getInstance()->broadcastToNodes(broadcastData, dataBytes, & nodeTypes[i], 1);

        // Feed number of bytes to corresponding channel of the bandwidth meter, if any (done otherwise)
        BandwidthMeter::ChannelIndex channel;
        switch (nodeTypes[i]) {
            case NODE_TYPE_AGENT:
            case NODE_TYPE_AVATAR_MIXER:
                channel = BandwidthMeter::AVATARS;
                break;
            case NODE_TYPE_VOXEL_SERVER:
                channel = BandwidthMeter::VOXELS;
                break;
            default:
                continue;
        }
        self->_bandwidthMeter.outputStream(channel).updateValue(nReceivingNodes * dataBytes); 
    }
}

void Application::keyPressEvent(QKeyEvent* event) {
    if (activeWindow() == _window) {
        if (_chatEntryOn) {
            if (_chatEntry.keyPressEvent(event)) {
                _myAvatar.setKeyState(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete ?
                                      DELETE_KEY_DOWN : INSERT_KEY_DOWN);
                _myAvatar.setChatMessage(string(_chatEntry.getContents().size(), SOLID_BLOCK_CHAR));
                
            } else {
                _myAvatar.setChatMessage(_chatEntry.getContents());
                _chatEntry.clear();
                _chatEntryOn = false;
                setMenuShortcutsEnabled(true);
            }
            return;
        }

        //this is for switching between modes for the leap rave glove test
        if (Menu::getInstance()->isOptionChecked(MenuOption::SimulateLeapHand)
            || Menu::getInstance()->isOptionChecked(MenuOption::TestRaveGlove)) {
            _myAvatar.getHand().setRaveGloveEffectsMode((QKeyEvent*)event);
        }

        bool isShifted = event->modifiers().testFlag(Qt::ShiftModifier);
        bool isMeta = event->modifiers().testFlag(Qt::ControlModifier);
        switch (event->key()) {
            case Qt::Key_Shift:
                if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelSelectMode)) {
                    _pasteMode = true;
                }
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
            case Qt::Key_Semicolon:
                _audio.ping();
                break;
            case Qt::Key_Apostrophe:
                _audioScope.inputPaused = !_audioScope.inputPaused;
                break; 
            case Qt::Key_L:
                if (!isShifted && !isMeta) {
                    _displayLevels = !_displayLevels;
                } else if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::LodTools);
                } else if (isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::Log);
                }
                break;
                
            case Qt::Key_E:
                if (_nudgeStarted) {
                    _nudgeGuidePosition.y += _mouseVoxel.s;
                } else {
                   if (!_myAvatar.getDriveKeys(UP)) {
                        _myAvatar.jump();
                    }
                    _myAvatar.setDriveKeys(UP, 1); 
                }
                break;

            case Qt::Key_Asterisk:
                Menu::getInstance()->triggerOption(MenuOption::Stars);
                break;
                
            case Qt::Key_C:
                if (isShifted)  {
                    Menu::getInstance()->triggerOption(MenuOption::OcclusionCulling);
                } else if (_nudgeStarted) {
                    _nudgeGuidePosition.y -= _mouseVoxel.s;
                } else {
                    _myAvatar.setDriveKeys(DOWN, 1);
                }
                break;
                
            case Qt::Key_W:
                if (_nudgeStarted) {
                    if (_lookingAlongX) {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.x += _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.x -= _mouseVoxel.s;
                        }
                    } else {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.z += _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.z -= _mouseVoxel.s;
                        }
                    }
                } else {
                    _myAvatar.setDriveKeys(FWD, 1);
                }
                break;
                
            case Qt::Key_S:
                if (isShifted && !isMeta)  {
                    _voxels.collectStatsForTreesAndVBOs();
                } else if (isShifted && isMeta)  {
                    Menu::getInstance()->triggerOption(MenuOption::SuppressShortTimings);
                } else if (_nudgeStarted) {
                    if (_lookingAlongX) {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.x -= _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.x += _mouseVoxel.s;
                        }
                    } else {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.z -= _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.z += _mouseVoxel.s;
                        }
                    }
                } else {
                    _myAvatar.setDriveKeys(BACK, 1);
                }
                break;
                
            case Qt::Key_Space:
                resetSensors();
                break;
                
            case Qt::Key_G:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Gravity);
                } else {
                    Menu::getInstance()->triggerOption(MenuOption::VoxelGetColorMode);
                }
                break;
                
            case Qt::Key_A:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Atmosphere);
                } else if (_nudgeStarted) {
                    if (_lookingAlongX) {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.z -= _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.z += _mouseVoxel.s;
                        }
                    } else {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.x += _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.x -= _mouseVoxel.s;
                        }
                    }
                } else {
                    _myAvatar.setDriveKeys(ROT_LEFT, 1);
                }
                break;
                
            case Qt::Key_D:
                if (_nudgeStarted) {
                    if (_lookingAlongX) {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.z += _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.z -= _mouseVoxel.s;
                        }
                    } else {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.x -= _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.x += _mouseVoxel.s;
                        }
                    }
                } else {
                    _myAvatar.setDriveKeys(ROT_RIGHT, 1);
                }
                break;
                
            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (_nudgeStarted) {
                    nudgeVoxels();
                } else {
                    _chatEntryOn = true;
                    _myAvatar.setKeyState(NO_KEY_DOWN);
                    _myAvatar.setChatMessage(string());
                    setMenuShortcutsEnabled(false);
                }
                break;
                
            case Qt::Key_Up:
                if (_nudgeStarted && !isShifted) {
                    if (_lookingAlongX) {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.x += _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.x -= _mouseVoxel.s;
                        }
                    } else {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.z += _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.z -= _mouseVoxel.s;
                        }
                    }
                } else if (_nudgeStarted && isShifted) {
                    _nudgeGuidePosition.y += _mouseVoxel.s;
                } else {
                    _myAvatar.setDriveKeys(isShifted ? UP : FWD, 1);
                }
                break;
                
            case Qt::Key_Down:
                if (_nudgeStarted && !isShifted) {
                    if (_lookingAlongX) {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.x -= _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.x += _mouseVoxel.s;
                        }
                    } else {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.z -= _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.z += _mouseVoxel.s;
                        }
                    }
                } else if (_nudgeStarted && isShifted) {
                    _nudgeGuidePosition.y -= _mouseVoxel.s;
                } else {
                    _myAvatar.setDriveKeys(isShifted ? DOWN : BACK, 1);
                }
                break;
                
            case Qt::Key_Left:
                if (_nudgeStarted) {
                    if (_lookingAlongX) {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.z -= _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.z += _mouseVoxel.s;
                        }
                    } else {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.x += _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.x -= _mouseVoxel.s;
                        }
                    }
                } else {
                    _myAvatar.setDriveKeys(isShifted ? LEFT : ROT_LEFT, 1);
                }
                break;
                
            case Qt::Key_Right:
                if (_nudgeStarted) {
                    if (_lookingAlongX) {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.z += _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.z -= _mouseVoxel.s;
                        }
                    } else {
                        if (_lookingAwayFromOrigin) {
                            _nudgeGuidePosition.x -= _mouseVoxel.s;
                        } else {
                            _nudgeGuidePosition.x += _mouseVoxel.s;
                        }
                    }
                } else {
                    _myAvatar.setDriveKeys(isShifted ? RIGHT : ROT_RIGHT, 1);
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
            case Qt::Key_H:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Mirror);
                } else {
                    Menu::getInstance()->triggerOption(MenuOption::FullscreenMirror);
                }
                break;
            case Qt::Key_F:
                if (isShifted)  {
                    Menu::getInstance()->triggerOption(MenuOption::DisplayFrustum);
                }
                break;
            case Qt::Key_V:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Voxels);
                } else {
                    Menu::getInstance()->triggerOption(MenuOption::VoxelAddMode);
                    _nudgeStarted = false;
                }
                break;
            case Qt::Key_P:
                 Menu::getInstance()->triggerOption(MenuOption::FirstPerson);
                 break;
            case Qt::Key_R:
                if (isShifted)  {
                    Menu::getInstance()->triggerOption(MenuOption::FrustumRenderMode);
                } else {
                    Menu::getInstance()->triggerOption(MenuOption::VoxelDeleteMode);
                    _nudgeStarted = false;
                }
                break;
            case Qt::Key_B:
                Menu::getInstance()->triggerOption(MenuOption::VoxelColorMode);
                _nudgeStarted = false;
                break;
            case Qt::Key_O:
                Menu::getInstance()->triggerOption(MenuOption::VoxelSelectMode);
                _nudgeStarted = false;
                break;
            case Qt::Key_Slash:
                Menu::getInstance()->triggerOption(MenuOption::Stats);
                break;
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
                if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelDeleteMode) ||
                    Menu::getInstance()->isOptionChecked(MenuOption::VoxelSelectMode)) {
                    deleteVoxelUnderCursor();
                }
                break;
            case Qt::Key_Plus:
                _myAvatar.increaseSize();
                break;
            case Qt::Key_Minus:
                _myAvatar.decreaseSize();
                break;

            case Qt::Key_1:
            case Qt::Key_2:
            case Qt::Key_3:
            case Qt::Key_4:
            case Qt::Key_5:
            case Qt::Key_6:
            case Qt::Key_7:
            case Qt::Key_8:
                _swatch.handleEvent(event->key(), Menu::getInstance()->isOptionChecked(MenuOption::VoxelGetColorMode));
                break;
            case Qt::Key_At:
                Menu::getInstance()->goToUser();
                break;
            default:
                event->ignore();
                break;
        }
    }
}

void Application::keyReleaseEvent(QKeyEvent* event) {
    if (activeWindow() == _window) {
        if (_chatEntryOn) {
            _myAvatar.setKeyState(NO_KEY_DOWN);
            return;
        }
        
        switch (event->key()) {
            case Qt::Key_Shift:
                _pasteMode = false;
                break;
            case Qt::Key_E:
                _myAvatar.setDriveKeys(UP, 0);
                break;
                
            case Qt::Key_C:
                _myAvatar.setDriveKeys(DOWN, 0);
                break;
                
            case Qt::Key_W:
                _myAvatar.setDriveKeys(FWD, 0);
                break;
                
            case Qt::Key_S:
                _myAvatar.setDriveKeys(BACK, 0);
                break;
                
            case Qt::Key_A:
                _myAvatar.setDriveKeys(ROT_LEFT, 0);
                break;
                
            case Qt::Key_D:
                _myAvatar.setDriveKeys(ROT_RIGHT, 0);
                break;
                
            case Qt::Key_Up:
                _myAvatar.setDriveKeys(FWD, 0);
                _myAvatar.setDriveKeys(UP, 0);
                break;
                
            case Qt::Key_Down:
                _myAvatar.setDriveKeys(BACK, 0);
                _myAvatar.setDriveKeys(DOWN, 0);
                break;
                
            case Qt::Key_Left:
                _myAvatar.setDriveKeys(LEFT, 0);
                _myAvatar.setDriveKeys(ROT_LEFT, 0);
                break;
                
            case Qt::Key_Right:
                _myAvatar.setDriveKeys(RIGHT, 0);
                _myAvatar.setDriveKeys(ROT_RIGHT, 0);
                break;
                
            default:
                event->ignore();
                break;
        }
    }
}

void Application::mouseMoveEvent(QMouseEvent* event) {
    _lastMouseMove = usecTimestampNow();
    if (_mouseHidden) {
        getGLWidget()->setCursor(Qt::ArrowCursor);
        _mouseHidden = false;
        _seenMouseMove = true;
    }

    if (activeWindow() == _window) {
        _mouseX = event->x();
        _mouseY = event->y();

        // detect drag
        glm::vec3 mouseVoxelPos(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z);
        if (!_justEditedVoxel && mouseVoxelPos != _lastMouseVoxelPos) {
            if (event->buttons().testFlag(Qt::LeftButton)) {
                maybeEditVoxelUnderCursor();
                
            } else if (event->buttons().testFlag(Qt::RightButton) && Menu::getInstance()->isVoxelModeActionChecked()) {
                deleteVoxelUnderCursor();
            }
        }

        _pieMenu.mouseMoveEvent(_mouseX, _mouseY);
    }
}

const bool MAKE_SOUND_ON_VOXEL_HOVER = false;
const bool MAKE_SOUND_ON_VOXEL_CLICK = true;
const float HOVER_VOXEL_FREQUENCY = 7040.f;
const float HOVER_VOXEL_DECAY = 0.999f;

void Application::mousePressEvent(QMouseEvent* event) {
    if (activeWindow() == _window) {
        if (event->button() == Qt::LeftButton) {
            _mouseX = event->x();
            _mouseY = event->y();
            _mouseDragStartedX = _mouseX;
            _mouseDragStartedY = _mouseY;
            _mouseVoxelDragging = _mouseVoxel;
            _mousePressed = true;

            maybeEditVoxelUnderCursor();
            
            if (_audio.mousePressEvent(_mouseX, _mouseY)) {
                // stop propagation
                return;
            }
            
            if (_rearMirrorTools->mousePressEvent(_mouseX, _mouseY)) {
                // stop propagation
                return;
            }
            
            if (!_palette.isActive() && (!_isHoverVoxel || _lookatTargetAvatar)) {
                _pieMenu.mousePressEvent(_mouseX, _mouseY);
            }

            if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelSelectMode) && _pasteMode) {
                pasteVoxels();
            }

            if (MAKE_SOUND_ON_VOXEL_CLICK && _isHoverVoxel && !_isHoverVoxelSounding) {
                _hoverVoxelOriginalColor[0] = _hoverVoxel.red;
                _hoverVoxelOriginalColor[1] = _hoverVoxel.green;
                _hoverVoxelOriginalColor[2] = _hoverVoxel.blue;
                _hoverVoxelOriginalColor[3] = 1;
                const float RED_CLICK_FREQUENCY = 1000.f;
                const float GREEN_CLICK_FREQUENCY = 1250.f;
                const float BLUE_CLICK_FREQUENCY = 1330.f;
                const float MIDDLE_A_FREQUENCY = 440.f;
                float frequency = MIDDLE_A_FREQUENCY + 
                    (_hoverVoxel.red / 255.f * RED_CLICK_FREQUENCY +
                    _hoverVoxel.green / 255.f * GREEN_CLICK_FREQUENCY +
                    _hoverVoxel.blue / 255.f * BLUE_CLICK_FREQUENCY) / 3.f;
                
                _audio.startCollisionSound(1.0, frequency, 0.0, HOVER_VOXEL_DECAY);
                _isHoverVoxelSounding = true;
                
                const float PERCENTAGE_TO_MOVE_TOWARD = 0.90f;
                glm::vec3 newTarget = getMouseVoxelWorldCoordinates(_hoverVoxel);
                glm::vec3 myPosition = _myAvatar.getPosition();
                
                // If there is not an action tool set (add, delete, color), move to this voxel
                if (!(Menu::getInstance()->isOptionChecked(MenuOption::VoxelAddMode) ||
                     Menu::getInstance()->isOptionChecked(MenuOption::VoxelDeleteMode) ||
                     Menu::getInstance()->isOptionChecked(MenuOption::VoxelColorMode))) {
                    _myAvatar.setMoveTarget(myPosition + (newTarget - myPosition) * PERCENTAGE_TO_MOVE_TOWARD);
                }
            }
            
        } else if (event->button() == Qt::RightButton && Menu::getInstance()->isVoxelModeActionChecked()) {
            deleteVoxelUnderCursor();
        }
    }
}

void Application::mouseReleaseEvent(QMouseEvent* event) {
    if (activeWindow() == _window) {
        if (event->button() == Qt::LeftButton) {
            _mouseX = event->x();
            _mouseY = event->y();
            _mousePressed = false;
            checkBandwidthMeterClick();

            _pieMenu.mouseReleaseEvent(_mouseX, _mouseY);
        }
    }
}

void Application::touchUpdateEvent(QTouchEvent* event) {
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
    touchUpdateEvent(event);
    _lastTouchAvgX = _touchAvgX;
    _lastTouchAvgY = _touchAvgY;
}

void Application::touchEndEvent(QTouchEvent* event) {
    _touchDragStartedAvgX = _touchAvgX;
    _touchDragStartedAvgY = _touchAvgY;
    _isTouchPressed = false;
}

const bool USE_MOUSEWHEEL = false; 
void Application::wheelEvent(QWheelEvent* event) {
    //  Wheel Events disabled for now because they are also activated by touch look pitch up/down.  
    if (USE_MOUSEWHEEL && (activeWindow() == _window)) {
        if (!Menu::getInstance()->isVoxelModeActionChecked()) {
            event->ignore();
            return;
        }
        if (event->delta() > 0) {
            increaseVoxelSize();
        } else {
            decreaseVoxelSize();
        }
    }
}

void Application::sendPingPackets() {

    const char nodesToPing[] = {NODE_TYPE_VOXEL_SERVER, NODE_TYPE_AUDIO_MIXER, NODE_TYPE_AVATAR_MIXER};

    uint64_t currentTime = usecTimestampNow();
    unsigned char pingPacket[numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_PING) + sizeof(currentTime)];
    int numHeaderBytes = populateTypeAndVersion(pingPacket, PACKET_TYPE_PING);
    
    memcpy(pingPacket + numHeaderBytes, &currentTime, sizeof(currentTime));
    getInstance()->controlledBroadcastToNodes(pingPacket, sizeof(pingPacket),
                                              nodesToPing, sizeof(nodesToPing));
}

void Application::sendAvatarFaceVideoMessage(int frameCount, const QByteArray& data) {
    unsigned char packet[MAX_PACKET_SIZE];
    unsigned char* packetPosition = packet;
    
    packetPosition += populateTypeAndVersion(packetPosition, PACKET_TYPE_AVATAR_FACE_VIDEO);
    
    QByteArray rfcUUID = NodeList::getInstance()->getOwnerUUID().toRfc4122();
    memcpy(packetPosition, rfcUUID.constData(), rfcUUID.size());
    packetPosition += rfcUUID.size();
    
    *(uint32_t*)packetPosition = frameCount;
    packetPosition += sizeof(uint32_t);
    
    *(uint32_t*)packetPosition = data.size();
    packetPosition += sizeof(uint32_t);
    
    uint32_t* offsetPosition = (uint32_t*)packetPosition;
    packetPosition += sizeof(uint32_t);
    
    int headerSize = packetPosition - packet;
    
    // break the data up into submessages of the maximum size (at least one, for zero-length packets)
    *offsetPosition = 0;
    do {
        int payloadSize = min(data.size() - (int)*offsetPosition, MAX_PACKET_SIZE - headerSize);
        memcpy(packetPosition, data.constData() + *offsetPosition, payloadSize);
        getInstance()->controlledBroadcastToNodes(packet, headerSize + payloadSize, &NODE_TYPE_AVATAR_MIXER, 1);
        *offsetPosition += payloadSize;
         
    } while (*offsetPosition < data.size());
}

//  Every second, check the frame rates and other stuff
void Application::timer() {
    gettimeofday(&_timerEnd, NULL);

    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        sendPingPackets();
    }
        
    _fps = (float)_frameCount / ((float)diffclock(&_timerStart, &_timerEnd) / 1000.f);
    _packetsPerSecond = (float)_packetCount / ((float)diffclock(&_timerStart, &_timerEnd) / 1000.f);
    _bytesPerSecond = (float)_bytesCount / ((float)diffclock(&_timerStart, &_timerEnd) / 1000.f);
    _frameCount = 0;
    _packetCount = 0;
    _bytesCount = 0;
    
    gettimeofday(&_timerStart, NULL);
    
    // if we haven't detected gyros, check for them now
    if (!_serialHeadSensor.isActive()) {
        _serialHeadSensor.pair();
    }
    
    // ask the node list to check in with the domain server
    NodeList::getInstance()->sendDomainServerCheckIn();
    
    // give the MyAvatar object position to the Profile so it can propagate to the data-server
    _profile.updatePosition(_myAvatar.getPosition());
}

static glm::vec3 getFaceVector(BoxFace face) {
    switch (face) {
        case MIN_X_FACE:
            return glm::vec3(-1, 0, 0);
        
        case MAX_X_FACE:
            return glm::vec3(1, 0, 0);
        
        case MIN_Y_FACE:
            return glm::vec3(0, -1, 0);
        
        case MAX_Y_FACE:
            return glm::vec3(0, 1, 0);
        
        case MIN_Z_FACE:
            return glm::vec3(0, 0, -1);
            
        case MAX_Z_FACE:
            return glm::vec3(0, 0, 1);
    }
}

void Application::idle() {
    // Normally we check PipelineWarnings, but since idle will often take more than 10ms we only show these idle timing 
    // details if we're in ExtraDebugging mode. However, the ::update() and it's subcomponents will show their timing 
    // details normally.
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::ExtraDebugging);
    PerformanceWarning warn(showWarnings, "Application::idle()");
    
    timeval check;
    gettimeofday(&check, NULL);
    
    //  Only run simulation code if more than IDLE_SIMULATE_MSECS have passed since last time we ran

    double timeSinceLastUpdate = diffclock(&_lastTimeUpdated, &check);
    if (timeSinceLastUpdate > IDLE_SIMULATE_MSECS) {
        {
            PerformanceWarning warn(showWarnings, "Application::idle()... update()");
            const float BIGGEST_DELTA_TIME_SECS = 0.25f;
            update(glm::clamp((float)timeSinceLastUpdate / 1000.f, 0.f, BIGGEST_DELTA_TIME_SECS));
        }
        {
            PerformanceWarning warn(showWarnings, "Application::idle()... updateGL()");
            _glWidget->updateGL();
        }
        {
            PerformanceWarning warn(showWarnings, "Application::idle()... rest of it");
            _lastTimeUpdated = check;
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
void Application::terminate() {
    // Close serial port
    // close(serial_fd);
    
    LeapManager::terminate();
    Menu::getInstance()->saveSettings();
    _rearMirrorTools->saveSettings(_settings);
    _settings->sync();

    if (_enableNetworkThread) {
        _stopNetworkReceiveThread = true;
        pthread_join(_networkReceiveThread, NULL); 
    }

    _voxelProcessor.terminate();
    _voxelEditSender.terminate();
}

static Avatar* processAvatarMessageHeader(unsigned char*& packetData, size_t& dataBytes) {
    // record the packet for stats-tracking
    Application::getInstance()->getBandwidthMeter()->inputStream(BandwidthMeter::AVATARS).updateValue(dataBytes);
    Node* avatarMixerNode = NodeList::getInstance()->soloNodeOfType(NODE_TYPE_AVATAR_MIXER);
    if (avatarMixerNode) {
        avatarMixerNode->recordBytesReceived(dataBytes);
    }
    
    // skip the header
    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    packetData += numBytesPacketHeader;
    dataBytes -= numBytesPacketHeader;
    
    // read the node id
    QUuid nodeUUID = QUuid::fromRfc4122(QByteArray((char*) packetData, NUM_BYTES_RFC4122_UUID));
    
    packetData += NUM_BYTES_RFC4122_UUID;
    dataBytes -= NUM_BYTES_RFC4122_UUID;
    
    // make sure the node exists
    Node* node = NodeList::getInstance()->nodeWithUUID(nodeUUID);
    if (!node || !node->getLinkedData()) {
        return NULL;
    }
    Avatar* avatar = static_cast<Avatar*>(node->getLinkedData());
    return avatar->isInitialized() ? avatar : NULL;
}

void Application::processAvatarURLsMessage(unsigned char* packetData, size_t dataBytes) {
    Avatar* avatar = processAvatarMessageHeader(packetData, dataBytes);
    if (!avatar) {
        return;
    }
    QDataStream in(QByteArray((char*)packetData, dataBytes));
    QUrl voxelURL;
    in >> voxelURL;
    
    // invoke the set URL functions on the simulate/render thread
    QMetaObject::invokeMethod(avatar->getVoxels(), "setVoxelURL", Q_ARG(QUrl, voxelURL));
    
    // use this timing to as the data-server for an updated mesh for this avatar (if we have UUID)
    DataServerClient::getValuesForKeysAndUUID(QStringList() << DataServerKey::FaceMeshURL << DataServerKey::SkeletonURL,
        avatar->getUUID());
}

void Application::processAvatarFaceVideoMessage(unsigned char* packetData, size_t dataBytes) {
    Avatar* avatar = processAvatarMessageHeader(packetData, dataBytes);
    if (!avatar) {
        return;
    }
    avatar->getHead().getVideoFace().processVideoMessage(packetData, dataBytes);
}

void Application::checkBandwidthMeterClick() {
    // ... to be called upon button release

    if (Menu::getInstance()->isOptionChecked(MenuOption::Bandwidth) &&
        glm::compMax(glm::abs(glm::ivec2(_mouseX - _mouseDragStartedX, _mouseY - _mouseDragStartedY))) <= BANDWIDTH_METER_CLICK_MAX_DRAG_LENGTH &&
        _bandwidthMeter.isWithinArea(_mouseX, _mouseY, _glWidget->width(), _glWidget->height())) {

        // The bandwidth meter is visible, the click didn't get dragged too far and
        // we actually hit the bandwidth meter
        Menu::getInstance()->bandwidthDetails();
    }
}

void Application::setFullscreen(bool fullscreen) {
    _window->setWindowState(fullscreen ? (_window->windowState() | Qt::WindowFullScreen) :
        (_window->windowState() & ~Qt::WindowFullScreen));
    updateCursor();
}

void Application::setRenderVoxels(bool voxelRender) {
    _voxelEditSender.setShouldSend(voxelRender);
    if (!voxelRender) {
        doKillLocalVoxels();
    }
}

void Application::doKillLocalVoxels() {
    _wantToKillLocalVoxels = true;
}

const glm::vec3 Application::getMouseVoxelWorldCoordinates(const VoxelDetail _mouseVoxel) {
    return glm::vec3((_mouseVoxel.x + _mouseVoxel.s / 2.f) * TREE_SCALE,
                     (_mouseVoxel.y + _mouseVoxel.s / 2.f) * TREE_SCALE,
                     (_mouseVoxel.z + _mouseVoxel.s / 2.f) * TREE_SCALE);
}

const float NUDGE_PRECISION_MIN = 1 / pow(2.0, 12.0);

void Application::decreaseVoxelSize() {
    if (_nudgeStarted) {
        if (_mouseVoxelScale >= NUDGE_PRECISION_MIN) {
            _mouseVoxelScale /= 2;
        }
    } else {
        _mouseVoxelScale /= 2;
    }
}

void Application::increaseVoxelSize() {
    if (_nudgeStarted) {
        if (_mouseVoxelScale < _nudgeVoxel.s) {
            _mouseVoxelScale *= 2;
        }
    } else {
        _mouseVoxelScale *= 2;
    }
}

const int MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE = 1500;
struct SendVoxelsOperationArgs {
    const unsigned char*  newBaseOctCode;
};

bool Application::sendVoxelsOperation(VoxelNode* node, void* extraData) {
    SendVoxelsOperationArgs* args = (SendVoxelsOperationArgs*)extraData;
    if (node->isColored()) {
        const unsigned char* nodeOctalCode = node->getOctalCode();
        
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
        codeColorBuffer[bytesInCode + RED_INDEX  ] = node->getColor()[RED_INDEX  ];
        codeColorBuffer[bytesInCode + GREEN_INDEX] = node->getColor()[GREEN_INDEX];
        codeColorBuffer[bytesInCode + BLUE_INDEX ] = node->getColor()[BLUE_INDEX ];

        getInstance()->_voxelEditSender.queueVoxelEditMessage(PACKET_TYPE_SET_VOXEL_DESTRUCTIVE, 
                codeColorBuffer, codeAndColorLength);
        
        delete[] codeColorBuffer;
    }
    return true; // keep going
}

void Application::exportVoxels() {
    QString desktopLocation = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString suggestedName = desktopLocation.append("/voxels.svo");

    QString fileNameString = QFileDialog::getSaveFileName(_glWidget, tr("Export Voxels"), suggestedName, 
                                                          tr("Sparse Voxel Octree Files (*.svo)"));
    QByteArray fileNameAscii = fileNameString.toLocal8Bit();
    const char* fileName = fileNameAscii.data();
    VoxelNode* selectedNode = _voxels.getVoxelAt(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
    if (selectedNode) {
        VoxelTree exportTree;
        _voxels.copySubTreeIntoNewTree(selectedNode, &exportTree, true);
        exportTree.writeToSVOFile(fileName);
    }

    // restore the main window's active state
    _window->activateWindow();
}

void Application::importVoxels() {
    if (_voxelImporter.exec()) {
        qDebug("[DEBUG] Import succedded.\n");
    } else {
        qDebug("[DEBUG] Import failed.\n");
    }

    // restore the main window's active state
    _window->activateWindow();
}

void Application::cutVoxels() {
    copyVoxels();
    deleteVoxelUnderCursor();
}

void Application::copyVoxels() {
    // switch to and clear the clipboard first...
    _sharedVoxelSystem.killLocalVoxels();
    if (_sharedVoxelSystem.getTree() != &_clipboard) {
        _clipboard.eraseAllVoxels();
        _sharedVoxelSystem.changeTree(&_clipboard);
    }

    // then copy onto it if there is something to copy
    VoxelNode* selectedNode = _voxels.getVoxelAt(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
    if (selectedNode) {
        _voxels.copySubTreeIntoNewTree(selectedNode, &_sharedVoxelSystem, true);
    }
}

void Application::pasteVoxels() {
    unsigned char* calculatedOctCode = NULL;
    VoxelNode* selectedNode = _voxels.getVoxelAt(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);

    // Recurse the clipboard tree, where everything is root relative, and send all the colored voxels to 
    // the server as an set voxel message, this will also rebase the voxels to the new location
    SendVoxelsOperationArgs args;

    // we only need the selected voxel to get the newBaseOctCode, which we can actually calculate from the
    // voxel size/position details. If we don't have an actual selectedNode then use the mouseVoxel to create a 
    // target octalCode for where the user is pointing.
    if (selectedNode) {
        args.newBaseOctCode = selectedNode->getOctalCode();
    } else {
        args.newBaseOctCode = calculatedOctCode = pointToVoxel(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
    }

    _sharedVoxelSystem.getTree()->recurseTreeWithOperation(sendVoxelsOperation, &args);

    if (_sharedVoxelSystem.getTree() != &_clipboard) {
        _sharedVoxelSystem.killLocalVoxels();
        _sharedVoxelSystem.changeTree(&_clipboard);
    }

    _voxelEditSender.releaseQueuedMessages();
    
    if (calculatedOctCode) {
        delete[] calculatedOctCode;
    }
}

void Application::findAxisAlignment() {
    glm::vec3 direction = _myAvatar.getMouseRayDirection();
    if (fabs(direction.z) > fabs(direction.x)) {
        _lookingAlongX = false;
        if (direction.z < 0) {
            _lookingAwayFromOrigin = false;
        } else {
            _lookingAwayFromOrigin = true;
        }
    } else {
        _lookingAlongX = true;
        if (direction.x < 0) {
            _lookingAwayFromOrigin = false;
        } else {
            _lookingAwayFromOrigin = true;
        }
    }
}

void Application::nudgeVoxels() {
    VoxelNode* selectedNode = _voxels.getVoxelAt(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
    if (!Menu::getInstance()->isOptionChecked(MenuOption::VoxelSelectMode) && selectedNode) {
        Menu::getInstance()->triggerOption(MenuOption::VoxelSelectMode);
    }
    
    if (!_nudgeStarted && selectedNode) {
        _nudgeVoxel = _mouseVoxel;
        _nudgeStarted = true;
        _nudgeGuidePosition = glm::vec3(_nudgeVoxel.x, _nudgeVoxel.y, _nudgeVoxel.z);
        findAxisAlignment();
    } else {
        // calculate nudgeVec
        glm::vec3 nudgeVec(_nudgeGuidePosition.x - _nudgeVoxel.x, _nudgeGuidePosition.y - _nudgeVoxel.y, _nudgeGuidePosition.z - _nudgeVoxel.z);

        VoxelNode* nodeToNudge = _voxels.getVoxelAt(_nudgeVoxel.x, _nudgeVoxel.y, _nudgeVoxel.z, _nudgeVoxel.s);

        if (nodeToNudge) {
            _voxels.getTree()->nudgeSubTree(nodeToNudge, nudgeVec, _voxelEditSender);
            _nudgeStarted = false;
        }
    }
}

void Application::deleteVoxels() {
    deleteVoxelUnderCursor();
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
    _sharedVoxelSystemViewFrustum.setFieldOfView(90);
    _sharedVoxelSystemViewFrustum.setOrientation(glm::quat());
    _sharedVoxelSystemViewFrustum.calculate();
    _sharedVoxelSystem.setViewFrustum(&_sharedVoxelSystemViewFrustum);

    VoxelNode::removeUpdateHook(&_sharedVoxelSystem);

    _sharedVoxelSystem.init();
    VoxelTree* tmpTree = _sharedVoxelSystem.getTree();
    _sharedVoxelSystem.changeTree(&_clipboard);
    delete tmpTree;

    _voxelImporter.init();
    
    _environment.init();

    _glowEffect.init();
    _ambientOcclusionEffect.init();
    _voxelShader.init();
    _pointShader.init();
    
    _handControl.setScreenDimensions(_glWidget->width(), _glWidget->height());

    _headMouseX = _mouseX = _glWidget->width() / 2;
    _headMouseY = _mouseY = _glWidget->height() / 2;
    QCursor::setPos(_headMouseX, _headMouseY);

    _myAvatar.init();
    _myAvatar.setPosition(START_LOCATION);
    _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
    _myCamera.setModeShiftRate(1.0f);
    _myAvatar.setDisplayingLookatVectors(false);
    
    _mirrorCamera.setMode(CAMERA_MODE_MIRROR);
    _mirrorCamera.setAspectRatio((float)MIRROR_VIEW_WIDTH / (float)MIRROR_VIEW_HEIGHT);
    _mirrorCamera.setFieldOfView(30);
    
    OculusManager::connect();
    if (OculusManager::isConnected()) {
        QMetaObject::invokeMethod(Menu::getInstance()->getActionForOption(MenuOption::Fullscreen),
                                  "trigger",
                                  Qt::QueuedConnection);
    }
    
    LeapManager::initialize();
    
    gettimeofday(&_timerStart, NULL);
    gettimeofday(&_lastTimeUpdated, NULL);

    Menu::getInstance()->loadSettings();
    if (Menu::getInstance()->getAudioJitterBufferSamples() != 0) {
        _audio.setJitterBufferSamples(Menu::getInstance()->getAudioJitterBufferSamples());
    }
    qDebug("Loaded settings.\n");
    
    if (!_profile.getUsername().isEmpty()) {
        // we have a username for this avatar, ask the data-server for the mesh URL for this avatar
        DataServerClient::getClientValueForKey(DataServerKey::FaceMeshURL);
        DataServerClient::getClientValueForKey(DataServerKey::SkeletonURL);
    }

    // Set up VoxelSystem after loading preferences so we can get the desired max voxel count    
    _voxels.setMaxVoxels(Menu::getInstance()->getMaxVoxels());
    _voxels.setUseVoxelShader(Menu::getInstance()->isOptionChecked(MenuOption::UseVoxelShader));
    _voxels.setVoxelsAsPoints(Menu::getInstance()->isOptionChecked(MenuOption::VoxelsAsPoints));
    _voxels.setDisableFastVoxelPipeline(Menu::getInstance()->isOptionChecked(MenuOption::DisableFastVoxelPipeline));
    _voxels.init();
    

    Avatar::sendAvatarURLsMessage(_myAvatar.getVoxels()->getVoxelURL());
   
    _palette.init(_glWidget->width(), _glWidget->height());
    _palette.addAction(Menu::getInstance()->getActionForOption(MenuOption::VoxelAddMode), 0, 0);
    _palette.addAction(Menu::getInstance()->getActionForOption(MenuOption::VoxelDeleteMode), 0, 1);
    _palette.addTool(&_swatch);
    _palette.addAction(Menu::getInstance()->getActionForOption(MenuOption::VoxelColorMode), 0, 2);
    _palette.addAction(Menu::getInstance()->getActionForOption(MenuOption::VoxelGetColorMode), 0, 3);
    _palette.addAction(Menu::getInstance()->getActionForOption(MenuOption::VoxelSelectMode), 0, 4);

    _pieMenu.init("./resources/images/hifi-interface-tools-v2-pie.svg",
                  _glWidget->width(),
                  _glWidget->height());

    _followMode = new QAction(this);
    connect(_followMode, SIGNAL(triggered()), this, SLOT(toggleFollowMode()));
    _pieMenu.addAction(_followMode);

    _audio.init(_glWidget);
    
    _rearMirrorTools = new RearMirrorTools(_glWidget, _mirrorViewRect, _settings);
    connect(_rearMirrorTools, SIGNAL(closeView()), SLOT(closeMirrorView()));
    connect(_rearMirrorTools, SIGNAL(restoreView()), SLOT(restoreMirrorView()));
    connect(_rearMirrorTools, SIGNAL(shrinkView()), SLOT(shrinkMirrorView()));
    connect(_rearMirrorTools, SIGNAL(resetView()), SLOT(resetSensors()));
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

const float MAX_AVATAR_EDIT_VELOCITY = 1.0f;
const float MAX_VOXEL_EDIT_DISTANCE = 50.0f;
const float HEAD_SPHERE_RADIUS = 0.07;

static QUuid DEFAULT_NODE_ID_REF;

void Application::updateLookatTargetAvatar(const glm::vec3& mouseRayOrigin, const glm::vec3& mouseRayDirection,
    glm::vec3& eyePosition) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateLookatTargetAvatar()");
    
    _lookatTargetAvatar = findLookatTargetAvatar(mouseRayOrigin, mouseRayDirection, eyePosition, DEFAULT_NODE_ID_REF);
}
        
Avatar* Application::findLookatTargetAvatar(const glm::vec3& mouseRayOrigin, const glm::vec3& mouseRayDirection,
    glm::vec3& eyePosition, QUuid& nodeUUID = DEFAULT_NODE_ID_REF) {
                                         
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
            Avatar* avatar = (Avatar *) node->getLinkedData();
            glm::vec3 headPosition = avatar->getHead().getPosition();
            float distance;
            if (rayIntersectsSphere(mouseRayOrigin, mouseRayDirection, headPosition,
                    HEAD_SPHERE_RADIUS * avatar->getHead().getScale(), distance)) {
                // rescale to compensate for head embiggening
                eyePosition = (avatar->getHead().calculateAverageEyePosition() - avatar->getHead().getScalePivot()) *
                    (avatar->getScale() / avatar->getHead().getScale()) + avatar->getHead().getScalePivot();
                
                _lookatIndicatorScale = avatar->getHead().getScale();
                _lookatOtherPosition = headPosition;
                nodeUUID = avatar->getOwningNode()->getUUID();
                return avatar;
            }
        }
    }
    return NULL;
}

bool Application::isLookingAtMyAvatar(Avatar* avatar) {
    glm::vec3 theirLookat = avatar->getHead().getLookAtPosition();
    glm::vec3 myHeadPosition = _myAvatar.getHead().getPosition();
    
    if (pointInSphere(theirLookat, myHeadPosition, HEAD_SPHERE_RADIUS * _myAvatar.getScale())) {
        return true;
    }
    return false;
}

void Application::renderLookatIndicator(glm::vec3 pointOfInterest, Camera& whichCamera) {

    const float DISTANCE_FROM_HEAD_SPHERE = 0.1f * _lookatIndicatorScale;
    const float INDICATOR_RADIUS = 0.1f * _lookatIndicatorScale;
    const float YELLOW[] = { 1.0f, 1.0f, 0.0f };
    const int NUM_SEGMENTS = 30;
    glm::vec3 haloOrigin(pointOfInterest.x, pointOfInterest.y + DISTANCE_FROM_HEAD_SPHERE, pointOfInterest.z);
    glColor3f(YELLOW[0], YELLOW[1], YELLOW[2]);
    renderCircle(haloOrigin, INDICATOR_RADIUS, IDENTITY_UP, NUM_SEGMENTS);
}

void maybeBeginFollowIndicator(bool& began) {
    if (!began) {
        Application::getInstance()->getGlowEffect()->begin();
        glLineWidth(5);
        glBegin(GL_LINES);
        began = true;
    }
}

void Application::renderFollowIndicator() {
    NodeList* nodeList = NodeList::getInstance();

    // initialize lazily so that we don't enable the glow effect unnecessarily
    bool began = false;

    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); ++node) {
        if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
            Avatar* avatar = (Avatar *) node->getLinkedData();
            Avatar* leader = NULL;

            if (!avatar->getLeaderUUID().isNull()) {
                if (avatar->getLeaderUUID() == NodeList::getInstance()->getOwnerUUID()) {
                    leader = &_myAvatar;
                } else {
                    for (NodeList::iterator it = nodeList->begin(); it != nodeList->end(); ++it) {
                        if(it->getUUID() == avatar->getLeaderUUID()
                                && it->getType() == NODE_TYPE_AGENT) {
                            leader = (Avatar*) it->getLinkedData();
                        }
                    }
                }

                if (leader != NULL) {
                    maybeBeginFollowIndicator(began);
                    glColor3f(1.f, 0.f, 0.f);
                    glVertex3f((avatar->getHead().getPosition().x + avatar->getPosition().x) / 2.f,
                               (avatar->getHead().getPosition().y + avatar->getPosition().y) / 2.f,
                               (avatar->getHead().getPosition().z + avatar->getPosition().z) / 2.f);
                    glColor3f(0.f, 1.f, 0.f);
                    glVertex3f((leader->getHead().getPosition().x + leader->getPosition().x) / 2.f,
                               (leader->getHead().getPosition().y + leader->getPosition().y) / 2.f,
                               (leader->getHead().getPosition().z + leader->getPosition().z) / 2.f);
                }
            }
        }
    }

    if (_myAvatar.getLeadingAvatar() != NULL) {
        maybeBeginFollowIndicator(began);
        glColor3f(1.f, 0.f, 0.f);
        glVertex3f((_myAvatar.getHead().getPosition().x + _myAvatar.getPosition().x) / 2.f,
                   (_myAvatar.getHead().getPosition().y + _myAvatar.getPosition().y) / 2.f,
                   (_myAvatar.getHead().getPosition().z + _myAvatar.getPosition().z) / 2.f);
        glColor3f(0.f, 1.f, 0.f);
        glVertex3f((_myAvatar.getLeadingAvatar()->getHead().getPosition().x + _myAvatar.getLeadingAvatar()->getPosition().x) / 2.f,
                   (_myAvatar.getLeadingAvatar()->getHead().getPosition().y + _myAvatar.getLeadingAvatar()->getPosition().y) / 2.f,
                   (_myAvatar.getLeadingAvatar()->getHead().getPosition().z + _myAvatar.getLeadingAvatar()->getPosition().z) / 2.f);
    }

    if (began) {
        glEnd();
        _glowEffect.end();
    }
}

void Application::updateAvatars(float deltaTime, glm::vec3 mouseRayOrigin, glm::vec3 mouseRayDirection) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateAvatars()");
    NodeList* nodeList = NodeList::getInstance();
    
    for(NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        node->lock();
        if (node->getLinkedData() != NULL) {
            Avatar *avatar = (Avatar *)node->getLinkedData();
            if (!avatar->isInitialized()) {
                avatar->init();
            }
            avatar->simulate(deltaTime, NULL);
            avatar->setMouseRay(mouseRayOrigin, mouseRayDirection);
        }
        node->unlock();
    }
}

void Application::updateMouseRay(float deltaTime, glm::vec3& mouseRayOrigin, glm::vec3& mouseRayDirection) {

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMouseRay()");

    _viewFrustum.computePickRay(_mouseX / (float)_glWidget->width(), _mouseY / (float)_glWidget->height(), 
                                mouseRayOrigin, mouseRayDirection);

    // adjust for mirroring
    if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        glm::vec3 mouseRayOffset = mouseRayOrigin - _viewFrustum.getPosition();
        mouseRayOrigin -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), mouseRayOffset) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), mouseRayOffset));
        mouseRayDirection -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), mouseRayDirection) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), mouseRayDirection));
    }

    // tell my avatar if the mouse is being pressed...
    _myAvatar.setMousePressed(_mousePressed);

    // tell my avatar the posiion and direction of the ray projected ino the world based on the mouse position        
    _myAvatar.setMouseRay(mouseRayOrigin, mouseRayDirection);
}

void Application::updateFaceshift() {

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateFaceshift()");
    
    //  Update faceshift
    _faceshift.update();
    
    //  Copy angular velocity if measured by faceshift, to the head
    if (_faceshift.isActive()) {
        _myAvatar.getHead().setAngularVelocity(_faceshift.getHeadAngularVelocity());
    }
}

void Application::updateMyAvatarLookAtPosition(glm::vec3& lookAtSpot, glm::vec3& lookAtRayOrigin, 
        glm::vec3& lookAtRayDirection) {

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMyAvatarLookAtPosition()");
    
    if (!_lookatTargetAvatar) {
        if (_isHoverVoxel) {
            //  Look at the hovered voxel
            lookAtSpot = getMouseVoxelWorldCoordinates(_hoverVoxel);
            
        } else if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
            lookAtSpot = _myCamera.getPosition();
        
        } else {
            //  Just look in direction of the mouse ray
            const float FAR_AWAY_STARE = TREE_SCALE;
            lookAtSpot = lookAtRayOrigin + lookAtRayDirection * FAR_AWAY_STARE;
        }
    }
    if (_faceshift.isActive()) {
        // deflect using Faceshift gaze data
        glm::vec3 origin = _myAvatar.getHead().calculateAverageEyePosition();
        float pitchSign = (_myCamera.getMode() == CAMERA_MODE_MIRROR) ? -1.0f : 1.0f;
        const float PITCH_SCALE = 0.25f;
        const float YAW_SCALE = 0.25f;
        lookAtSpot = origin + _myCamera.getRotation() * glm::quat(glm::radians(glm::vec3(
            _faceshift.getEstimatedEyePitch() * pitchSign * PITCH_SCALE, _faceshift.getEstimatedEyeYaw() * YAW_SCALE, 0.0f))) *
                glm::inverse(_myCamera.getRotation()) * (lookAtSpot - origin);
    }
    _myAvatar.getHead().setLookAtPosition(lookAtSpot);
}

void Application::updateHoverVoxels(float deltaTime, glm::vec3& mouseRayOrigin, glm::vec3& mouseRayDirection, 
                                    float& distance, BoxFace& face) {

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateHoverVoxels()");

    //  If we have clicked on a voxel, update it's color
    if (_isHoverVoxelSounding) {
        VoxelNode* hoveredNode = _voxels.getVoxelAt(_hoverVoxel.x, _hoverVoxel.y, _hoverVoxel.z, _hoverVoxel.s);
        if (hoveredNode) {
            float bright = _audio.getCollisionSoundMagnitude();
            nodeColor clickColor = { 255 * bright + _hoverVoxelOriginalColor[0] * (1.f - bright),
                                    _hoverVoxelOriginalColor[1] * (1.f - bright),
                                    _hoverVoxelOriginalColor[2] * (1.f - bright), 1 };
            hoveredNode->setColor(clickColor);
            if (bright < 0.01f) {
                hoveredNode->setColor(_hoverVoxelOriginalColor);
                _isHoverVoxelSounding = false;
            }
        } else {
            //  Voxel is not found, clear all
            _isHoverVoxelSounding = false;
            _isHoverVoxel = false; 
        }
    } else {
        //  Check for a new hover voxel
        glm::vec4 oldVoxel(_hoverVoxel.x, _hoverVoxel.y, _hoverVoxel.z, _hoverVoxel.s);
        // only do this work if MAKE_SOUND_ON_VOXEL_HOVER or MAKE_SOUND_ON_VOXEL_CLICK is enabled, 
        // and make sure the tree is not already busy... because otherwise you'll have to wait.
        if (!_voxels.treeIsBusy()) {
            {
                PerformanceWarning warn(showWarnings, "Application::updateHoverVoxels() _voxels.findRayIntersection()");
                _isHoverVoxel = _voxels.findRayIntersection(mouseRayOrigin, mouseRayDirection, _hoverVoxel, distance, face);
            }
            if (MAKE_SOUND_ON_VOXEL_HOVER && _isHoverVoxel && 
                    glm::vec4(_hoverVoxel.x, _hoverVoxel.y, _hoverVoxel.z, _hoverVoxel.s) != oldVoxel) {
                    
                _hoverVoxelOriginalColor[0] = _hoverVoxel.red;
                _hoverVoxelOriginalColor[1] = _hoverVoxel.green;
                _hoverVoxelOriginalColor[2] = _hoverVoxel.blue;
                _hoverVoxelOriginalColor[3] = 1;
                _audio.startCollisionSound(1.0, HOVER_VOXEL_FREQUENCY * _hoverVoxel.s * TREE_SCALE, 0.0, HOVER_VOXEL_DECAY);
                _isHoverVoxelSounding = true;
            }
        }
    }
}

void Application::updateMouseVoxels(float deltaTime, glm::vec3& mouseRayOrigin, glm::vec3& mouseRayDirection,
                                    float& distance, BoxFace& face) {

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMouseVoxels()");

    _mouseVoxel.s = 0.0f;
    bool wasInitialized = _mouseVoxelScaleInitialized;
    _mouseVoxelScaleInitialized = false;
    if (Menu::getInstance()->isVoxelModeActionChecked() &&
        (fabs(_myAvatar.getVelocity().x) +
         fabs(_myAvatar.getVelocity().y) +
         fabs(_myAvatar.getVelocity().z)) / 3 < MAX_AVATAR_EDIT_VELOCITY) {

        if (_voxels.findRayIntersection(mouseRayOrigin, mouseRayDirection, _mouseVoxel, distance, face)) {
            if (distance < MAX_VOXEL_EDIT_DISTANCE) {
                // set the voxel scale to that of the first moused-over voxel
                if (!wasInitialized) {
                    _mouseVoxelScale = _mouseVoxel.s;
                }
                _mouseVoxelScaleInitialized = true;
                
                // find the nearest voxel with the desired scale
                if (_mouseVoxelScale > _mouseVoxel.s) {
                    // choose the larger voxel that encompasses the one selected
                    _mouseVoxel.x = _mouseVoxelScale * floorf(_mouseVoxel.x / _mouseVoxelScale);
                    _mouseVoxel.y = _mouseVoxelScale * floorf(_mouseVoxel.y / _mouseVoxelScale);
                    _mouseVoxel.z = _mouseVoxelScale * floorf(_mouseVoxel.z / _mouseVoxelScale);
                    _mouseVoxel.s = _mouseVoxelScale;
                    
                } else {
                    glm::vec3 faceVector = getFaceVector(face);
                    if (_mouseVoxelScale < _mouseVoxel.s) {
                        // find the closest contained voxel
                        glm::vec3 pt = (mouseRayOrigin + mouseRayDirection * distance) / (float)TREE_SCALE -
                        faceVector * (_mouseVoxelScale * 0.5f);
                        _mouseVoxel.x = _mouseVoxelScale * floorf(pt.x / _mouseVoxelScale);
                        _mouseVoxel.y = _mouseVoxelScale * floorf(pt.y / _mouseVoxelScale);
                        _mouseVoxel.z = _mouseVoxelScale * floorf(pt.z / _mouseVoxelScale);
                        _mouseVoxel.s = _mouseVoxelScale;
                    }
                    if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelAddMode)) {
                        // use the face to determine the side on which to create a neighbor
                        _mouseVoxel.x += faceVector.x * _mouseVoxel.s;
                        _mouseVoxel.y += faceVector.y * _mouseVoxel.s;
                        _mouseVoxel.z += faceVector.z * _mouseVoxel.s;
                    }
                }
            } else {
                _mouseVoxel.s = 0.0f;
            }
        } else if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelAddMode)
                   || Menu::getInstance()->isOptionChecked(MenuOption::VoxelSelectMode)) {
            // place the voxel a fixed distance away
            float worldMouseVoxelScale = _mouseVoxelScale * TREE_SCALE;
            glm::vec3 pt = mouseRayOrigin + mouseRayDirection * (2.0f + worldMouseVoxelScale * 0.5f);
            _mouseVoxel.x = _mouseVoxelScale * floorf(pt.x / worldMouseVoxelScale);
            _mouseVoxel.y = _mouseVoxelScale * floorf(pt.y / worldMouseVoxelScale);
            _mouseVoxel.z = _mouseVoxelScale * floorf(pt.z / worldMouseVoxelScale);
            _mouseVoxel.s = _mouseVoxelScale;
        }
        
        if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelDeleteMode)) {
            // red indicates deletion
            _mouseVoxel.red = 255;
            _mouseVoxel.green = _mouseVoxel.blue = 0;
        } else if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelSelectMode)) {
            if (_nudgeStarted) {
                _mouseVoxel.red = _mouseVoxel.green = _mouseVoxel.blue = 255;
            } else {
                // yellow indicates selection
                _mouseVoxel.red = _mouseVoxel.green = 255;
                _mouseVoxel.blue = 0;
            }
        } else { // _addVoxelMode->isChecked() || _colorVoxelMode->isChecked()
            QColor paintColor = Menu::getInstance()->getActionForOption(MenuOption::VoxelPaintColor)->data().value<QColor>();
            _mouseVoxel.red = paintColor.red();
            _mouseVoxel.green = paintColor.green();
            _mouseVoxel.blue = paintColor.blue();
        }
        
        // if we just edited, use the currently selected voxel as the "last" for drag detection
        if (_justEditedVoxel) {
            _lastMouseVoxelPos = glm::vec3(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z);
            _justEditedVoxel = false;
        }
    }
}

void Application::updateHandAndTouch(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateHandAndTouch()");

    // walking triggers the handControl to stop
    if (_myAvatar.getMode() == AVATAR_MODE_WALKING) {
        _handControl.stop();
    }

    //  Update from Touch
    if (_isTouchPressed) {
        float TOUCH_YAW_SCALE = -0.25f;
        float TOUCH_PITCH_SCALE = -12.5f;
        float FIXED_TOUCH_TIMESTEP = 0.016f;
        _yawFromTouch += ((_touchAvgX - _lastTouchAvgX) * TOUCH_YAW_SCALE * FIXED_TOUCH_TIMESTEP);
        _pitchFromTouch += ((_touchAvgY - _lastTouchAvgY) * TOUCH_PITCH_SCALE * FIXED_TOUCH_TIMESTEP);
        _lastTouchAvgX = _touchAvgX;
        _lastTouchAvgY = _touchAvgY;
    }
}

void Application::updateLeap(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateLeap()");

    LeapManager::enableFakeFingers(Menu::getInstance()->isOptionChecked(MenuOption::SimulateLeapHand));
    _myAvatar.getHand().setRaveGloveActive(Menu::getInstance()->isOptionChecked(MenuOption::TestRaveGlove));
    LeapManager::nextFrame(_myAvatar);
}

void Application::updateSerialDevices(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateSerialDevices()");

    if (_serialHeadSensor.isActive()) {
        _serialHeadSensor.readData(deltaTime);
    }
}

void Application::updateThreads(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateThreads()");

    // read incoming packets from network
    if (!_enableNetworkThread) {
        networkReceive(0);
    }

    // parse voxel packets
    if (!_enableProcessVoxelsThread) {
        _voxelProcessor.threadRoutine();
        _voxelEditSender.threadRoutine();
    }
}

void Application::updateMyAvatarSimulation(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMyAvatarSimulation()");

    if (Menu::getInstance()->isOptionChecked(MenuOption::Gravity)) {
        _myAvatar.setGravity(_environment.getGravity(_myAvatar.getPosition()));
    }
    else {
        _myAvatar.setGravity(glm::vec3(0.0f, 0.0f, 0.0f));
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::TransmitterDrive) && _myTransmitter.isConnected()) {
        _myAvatar.simulate(deltaTime, &_myTransmitter);
    } else {
        _myAvatar.simulate(deltaTime, NULL);
    }
}

void Application::updateParticles(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateParticles()");

    if (Menu::getInstance()->isOptionChecked(MenuOption::ParticleCloud)) {
        _cloud.simulate(deltaTime);
    }
}

void Application::updateTransmitter(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateTransmitter()");

    // no transmitter drive implies transmitter pick
    if (!Menu::getInstance()->isOptionChecked(MenuOption::TransmitterDrive) && _myTransmitter.isConnected()) {
        _transmitterPickStart = _myAvatar.getSkeleton().joint[AVATAR_JOINT_CHEST].position;
        glm::vec3 direction = _myAvatar.getOrientation() *
            glm::quat(glm::radians(_myTransmitter.getEstimatedRotation())) * IDENTITY_FRONT;
        
        // check against voxels, avatars
        const float MAX_PICK_DISTANCE = 100.0f;
        float minDistance = MAX_PICK_DISTANCE;
        VoxelDetail detail;
        float distance;
        BoxFace face;
        if (_voxels.findRayIntersection(_transmitterPickStart, direction, detail, distance, face)) {
            minDistance = min(minDistance, distance);
        }
        NodeList* nodeList = NodeList::getInstance();
        for(NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            node->lock();
            if (node->getLinkedData() != NULL) {
                Avatar *avatar = (Avatar*)node->getLinkedData();
                if (!avatar->isInitialized()) {
                    avatar->init();
                }
                if (avatar->findRayIntersection(_transmitterPickStart, direction, distance)) {
                    minDistance = min(minDistance, distance);
                }
            }
            node->unlock();
        }
        _transmitterPickEnd = _transmitterPickStart + direction * minDistance;
        
    } else {
        _transmitterPickStart = _transmitterPickEnd = glm::vec3();
    }
}

void Application::updateCamera(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateCamera()");

    if (!OculusManager::isConnected()) {        
        if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
            if (_myCamera.getMode() != CAMERA_MODE_MIRROR) {
                _myCamera.setMode(CAMERA_MODE_MIRROR);
                _myCamera.setModeShiftRate(100.0f);
            }
        } else if (Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson)) {
            if (_myCamera.getMode() != CAMERA_MODE_FIRST_PERSON) {
                _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
                _myCamera.setModeShiftRate(1.0f);
            }
        } else {
            if (_myCamera.getMode() != CAMERA_MODE_THIRD_PERSON) {
                _myCamera.setMode(CAMERA_MODE_THIRD_PERSON);
                _myCamera.setModeShiftRate(1.0f);
            }
        }
        
        if (Menu::getInstance()->isOptionChecked(MenuOption::OffAxisProjection)) {
            float xSign = _myCamera.getMode() == CAMERA_MODE_MIRROR ? 1.0f : -1.0f;
            if (_faceshift.isActive()) {
                const float EYE_OFFSET_SCALE = 0.025f;
                glm::vec3 position = _faceshift.getHeadTranslation() * EYE_OFFSET_SCALE;
                _myCamera.setEyeOffsetPosition(glm::vec3(position.x * xSign, position.y, -position.z));    
                updateProjectionMatrix();
                
            } else if (_webcam.isActive()) {
                const float EYE_OFFSET_SCALE = 0.5f;
                glm::vec3 position = _webcam.getEstimatedPosition() * EYE_OFFSET_SCALE;
                _myCamera.setEyeOffsetPosition(glm::vec3(position.x * xSign, -position.y, position.z));
                updateProjectionMatrix();
            }
        }
    }
}

void Application::updateDialogs(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateDialogs()");

    // Update bandwidth dialog, if any
    BandwidthDialog* bandwidthDialog = Menu::getInstance()->getBandwidthDialog();
    if (bandwidthDialog) {
        bandwidthDialog->update();
    }
    
    VoxelStatsDialog* voxelStatsDialog = Menu::getInstance()->getVoxelStatsDialog();
    if (voxelStatsDialog) {
        voxelStatsDialog->update();
    }
}

void Application::updateAudio(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateAudio()");

    //  Update audio stats for procedural sounds
    #ifndef _WIN32
    _audio.setLastAcceleration(_myAvatar.getThrust());
    _audio.setLastVelocity(_myAvatar.getVelocity());
    _audio.eventuallyAnalyzePing();
    #endif
}

void Application::updateCursor(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateCursor()");

    // watch mouse position, if it hasn't moved, hide the cursor
    bool underMouse = _glWidget->underMouse();
    if (!_mouseHidden) {
        uint64_t now = usecTimestampNow();
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
    
    // check what's under the mouse and update the mouse voxel
    glm::vec3 mouseRayOrigin, mouseRayDirection;
    updateMouseRay(deltaTime, mouseRayOrigin, mouseRayDirection);
    
    // Set where I am looking based on my mouse ray (so that other people can see)
    glm::vec3 lookAtSpot;
    
    updateFaceshift();
    updateLookatTargetAvatar(mouseRayOrigin, mouseRayDirection, lookAtSpot);
    updateMyAvatarLookAtPosition(lookAtSpot, mouseRayOrigin, mouseRayDirection);
    
    //  Find the voxel we are hovering over, and respond if clicked
    float distance;
    BoxFace face;
    
    updateHoverVoxels(deltaTime, mouseRayOrigin, mouseRayDirection, distance, face); // clicking on voxels and making sounds
    updateMouseVoxels(deltaTime, mouseRayOrigin, mouseRayDirection, distance, face); // UI/UX related to voxels
    updateHandAndTouch(deltaTime); // Update state for touch sensors
    updateLeap(deltaTime); // Leap finger-sensing device
    updateSerialDevices(deltaTime); // Read serial port interface devices
    updateAvatar(deltaTime); // Sample hardware, update view frustum if needed, and send avatar data to mixer/nodes
    updateThreads(deltaTime); // If running non-threaded, then give the threads some time to process...
    updateAvatars(deltaTime, mouseRayOrigin, mouseRayDirection); //loop through all the other avatars and simulate them...
    updateMyAvatarSimulation(deltaTime); // Simulate myself
    updateParticles(deltaTime); // Simulate particle cloud movements
    updateTransmitter(deltaTime); // transmitter drive or pick
    updateCamera(deltaTime); // handle various camera tweaks like off axis projection
    updateDialogs(deltaTime); // update various stats dialogs if present
    updateAudio(deltaTime); // Update audio stats for procedural sounds
    updateCursor(deltaTime); // Handle cursor updates
}

void Application::updateAvatar(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateAvatar()");
    
    // rotate body yaw for yaw received from multitouch
    _myAvatar.setOrientation(_myAvatar.getOrientation()
                             * glm::quat(glm::vec3(0, _yawFromTouch, 0)));
    _yawFromTouch = 0.f;
    
    // Update my avatar's state from gyros and/or webcam
    _myAvatar.updateFromGyrosAndOrWebcam(_pitchFromTouch, Menu::getInstance()->isOptionChecked(MenuOption::TurnWithHead));
    
    // Update head mouse from faceshift if active
    if (_faceshift.isActive()) {
        glm::vec3 headVelocity = _faceshift.getHeadAngularVelocity();
        
        // sets how quickly head angular rotation moves the head mouse
        const float HEADMOUSE_FACESHIFT_YAW_SCALE = 40.f;
        const float HEADMOUSE_FACESHIFT_PITCH_SCALE = 30.f;
        _headMouseX -= headVelocity.y * HEADMOUSE_FACESHIFT_YAW_SCALE;
        _headMouseY -= headVelocity.x * HEADMOUSE_FACESHIFT_PITCH_SCALE;
    }
    
    if (_serialHeadSensor.isActive()) {

        //  Grab latest readings from the gyros
        float measuredPitchRate = _serialHeadSensor.getLastPitchRate();
        float measuredYawRate = _serialHeadSensor.getLastYawRate();
        
        //  Update gyro-based mouse (X,Y on screen)
        const float MIN_MOUSE_RATE = 3.0;
        const float HORIZONTAL_PIXELS_PER_DEGREE = 2880.f / 45.f;
        const float VERTICAL_PIXELS_PER_DEGREE = 1800.f / 30.f;
        if (powf(measuredYawRate * measuredYawRate +
                 measuredPitchRate * measuredPitchRate, 0.5) > MIN_MOUSE_RATE) {
            _headMouseX -= measuredYawRate * HORIZONTAL_PIXELS_PER_DEGREE * deltaTime;
            _headMouseY -= measuredPitchRate * VERTICAL_PIXELS_PER_DEGREE * deltaTime;
        }

        const float MIDPOINT_OF_SCREEN = 0.5;
        
        // Only use gyro to set lookAt if mouse hasn't selected an avatar
        if (!_lookatTargetAvatar) {

            // Set lookAtPosition if an avatar is at the center of the screen
            glm::vec3 screenCenterRayOrigin, screenCenterRayDirection;
            _viewFrustum.computePickRay(MIDPOINT_OF_SCREEN, MIDPOINT_OF_SCREEN, screenCenterRayOrigin, screenCenterRayDirection);

            glm::vec3 eyePosition;
            updateLookatTargetAvatar(screenCenterRayOrigin, screenCenterRayDirection, eyePosition);
            if (_lookatTargetAvatar) {
                glm::vec3 myLookAtFromMouse(eyePosition);
                _myAvatar.getHead().setLookAtPosition(myLookAtFromMouse);
            }
        }

    }
    
    //  Constrain head-driven mouse to edges of screen
    _headMouseX = glm::clamp(_headMouseX, 0, _glWidget->width());
    _headMouseY = glm::clamp(_headMouseY, 0, _glWidget->height());

    if (OculusManager::isConnected()) {
        float yaw, pitch, roll;
        OculusManager::getEulerAngles(yaw, pitch, roll);
    
        _myAvatar.getHead().setYaw(yaw + _yawFromTouch);
        _myAvatar.getHead().setPitch(pitch + _pitchFromTouch);
        _myAvatar.getHead().setRoll(roll);
    }
     
    //  Get audio loudness data from audio input device
    #ifndef _WIN32
        _myAvatar.getHead().setAudioLoudness(_audio.getLastInputLoudness());
    #endif

    NodeList* nodeList = NodeList::getInstance();
    
    // send head/hand data to the avatar mixer and voxel server
    unsigned char broadcastString[MAX_PACKET_SIZE];
    unsigned char* endOfBroadcastStringWrite = broadcastString;
    
    endOfBroadcastStringWrite += populateTypeAndVersion(endOfBroadcastStringWrite, PACKET_TYPE_HEAD_DATA);
    
    QByteArray ownerUUID = nodeList->getOwnerUUID().toRfc4122();
    memcpy(endOfBroadcastStringWrite, ownerUUID.constData(), ownerUUID.size());
    endOfBroadcastStringWrite += ownerUUID.size();
    
    endOfBroadcastStringWrite += _myAvatar.getBroadcastData(endOfBroadcastStringWrite);
    
    const char nodeTypesOfInterest[] = { NODE_TYPE_AVATAR_MIXER };
    controlledBroadcastToNodes(broadcastString, endOfBroadcastStringWrite - broadcastString,
                               nodeTypesOfInterest, sizeof(nodeTypesOfInterest));
    
    // once in a while, send my urls
    const float AVATAR_URLS_SEND_INTERVAL = 1.0f; // seconds
    if (shouldDo(AVATAR_URLS_SEND_INTERVAL, deltaTime)) {
        Avatar::sendAvatarURLsMessage(_myAvatar.getVoxels()->getVoxelURL());
    }

    // Update _viewFrustum with latest camera and view frustum data...
    // NOTE: we get this from the view frustum, to make it simpler, since the
    // loadViewFrumstum() method will get the correct details from the camera
    // We could optimize this to not actually load the viewFrustum, since we don't
    // actually need to calculate the view frustum planes to send these details 
    // to the server.
    loadViewFrustum(_myCamera, _viewFrustum);
    
    // Update my voxel servers with my current voxel query...
    queryVoxels();
}

void Application::queryVoxels() {

    // if voxels are disabled, then don't send this at all...
    if (!Menu::getInstance()->isOptionChecked(MenuOption::Voxels)) {
        return;
    }
    
    bool wantExtraDebugging = Menu::getInstance()->isOptionChecked(MenuOption::ExtraDebugging);
    
    // These will be the same for all servers, so we can set them up once and then reuse for each server we send to.
    _voxelQuery.setWantLowResMoving(Menu::getInstance()->isOptionChecked(MenuOption::LowRes));
    _voxelQuery.setWantColor(Menu::getInstance()->isOptionChecked(MenuOption::SendVoxelColors));
    _voxelQuery.setWantDelta(Menu::getInstance()->isOptionChecked(MenuOption::DeltaSending));
    _voxelQuery.setWantOcclusionCulling(Menu::getInstance()->isOptionChecked(MenuOption::OcclusionCulling));
    
    _voxelQuery.setCameraPosition(_viewFrustum.getPosition());
    _voxelQuery.setCameraOrientation(_viewFrustum.getOrientation());
    _voxelQuery.setCameraFov(_viewFrustum.getFieldOfView());
    _voxelQuery.setCameraAspectRatio(_viewFrustum.getAspectRatio());
    _voxelQuery.setCameraNearClip(_viewFrustum.getNearClip());
    _voxelQuery.setCameraFarClip(_viewFrustum.getFarClip());
    _voxelQuery.setCameraEyeOffsetPosition(_viewFrustum.getEyeOffsetPosition());
    _voxelQuery.setVoxelSizeScale(Menu::getInstance()->getVoxelSizeScale());
    _voxelQuery.setBoundaryLevelAdjust(Menu::getInstance()->getBoundaryLevelAdjust());

    unsigned char voxelQueryPacket[MAX_PACKET_SIZE];

    NodeList* nodeList = NodeList::getInstance();

    // Iterate all of the nodes, and get a count of how many voxel servers we have...
    int totalServers = 0;
    int inViewServers = 0;
    int unknownJurisdictionServers = 0;
    
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are NODE_TYPE_VOXEL_SERVER
        if (node->getActiveSocket() != NULL && node->getType() == NODE_TYPE_VOXEL_SERVER) {
            totalServers++;

            // get the server bounds for this server
            QUuid nodeUUID = node->getUUID();
            
            // if we haven't heard from this voxel server, go ahead and send it a query, so we
            // can get the jurisdiction...
            if (_voxelServerJurisdictions.find(nodeUUID) == _voxelServerJurisdictions.end()) {
                unknownJurisdictionServers++;
            } else {
                const JurisdictionMap& map = (_voxelServerJurisdictions)[nodeUUID];

                unsigned char* rootCode = map.getRootOctalCode();
            
                if (rootCode) {
                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode, rootDetails);
                    AABox serverBounds(glm::vec3(rootDetails.x, rootDetails.y, rootDetails.z), rootDetails.s);
                    serverBounds.scale(TREE_SCALE);

                    ViewFrustum::location serverFrustumLocation = _viewFrustum.boxInFrustum(serverBounds);

                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
                        inViewServers++;
                    }
                }
            }
        }
    }
    
    if (wantExtraDebugging && unknownJurisdictionServers > 0) {
        qDebug("Servers: total %d, in view %d, unknown jurisdiction %d \n", 
            totalServers, inViewServers, unknownJurisdictionServers);
    }

    int perServerPPS = 0;
    const int SMALL_BUDGET = 10;
    int perUnknownServer = SMALL_BUDGET;

    // determine PPS based on number of servers
    if (inViewServers >= 1) {
        // set our preferred PPS to be exactly evenly divided among all of the voxel servers... and allocate 1 PPS
        // for each unknown jurisdiction server
        perServerPPS = (DEFAULT_MAX_VOXEL_PPS / inViewServers) - (unknownJurisdictionServers * perUnknownServer);
    } else {
        if (unknownJurisdictionServers > 0) {
            perUnknownServer = (DEFAULT_MAX_VOXEL_PPS / unknownJurisdictionServers);
        }
    }
    
    if (wantExtraDebugging && unknownJurisdictionServers > 0) {
        qDebug("perServerPPS: %d perUnknownServer: %d\n", perServerPPS, perUnknownServer);
    }
    
    UDPSocket* nodeSocket = NodeList::getInstance()->getNodeSocket();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        // only send to the NodeTypes that are NODE_TYPE_VOXEL_SERVER
        if (node->getActiveSocket() != NULL && node->getType() == NODE_TYPE_VOXEL_SERVER) {


            // get the server bounds for this server
            QUuid nodeUUID = node->getUUID();

            bool inView = false;
            bool unknownView = false;
            
            // if we haven't heard from this voxel server, go ahead and send it a query, so we
            // can get the jurisdiction...
            if (_voxelServerJurisdictions.find(nodeUUID) == _voxelServerJurisdictions.end()) {
                unknownView = true; // assume it's in view
                if (wantExtraDebugging) {
                    qDebug() << "no known jurisdiction for node " << *node << ", assume it's visible.\n";
                }
            } else {
                const JurisdictionMap& map = (_voxelServerJurisdictions)[nodeUUID];

                unsigned char* rootCode = map.getRootOctalCode();
            
                if (rootCode) {
                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode, rootDetails);
                    AABox serverBounds(glm::vec3(rootDetails.x, rootDetails.y, rootDetails.z), rootDetails.s);
                    serverBounds.scale(TREE_SCALE);

                    ViewFrustum::location serverFrustumLocation = _viewFrustum.boxInFrustum(serverBounds);
                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
                        inView = true;
                    } else {
                        inView = false;
                    }
                } else {
                    if (wantExtraDebugging) {
                        qDebug() << "Jurisdiction without RootCode for node " << *node << ". That's unusual!\n";
                    }
                }
            }
            
            if (inView) {
                _voxelQuery.setMaxVoxelPacketsPerSecond(perServerPPS);
            } else if (unknownView) {
                if (wantExtraDebugging) {
                    qDebug() << "no known jurisdiction for node " << *node << ", give it budget of " 
                            << perUnknownServer << " to send us jurisdiction.\n";
                }
                
                // set the query's position/orientation to be degenerate in a manner that will get the scene quickly
                // If there's only one server, then don't do this, and just let the normal voxel query pass through 
                // as expected... this way, we will actually get a valid scene if there is one to be seen
                if (totalServers > 1) {
                    _voxelQuery.setCameraPosition(glm::vec3(-0.1,-0.1,-0.1));
                    const glm::quat OFF_IN_NEGATIVE_SPACE = glm::quat(-0.5, 0, -0.5, 1.0);
                    _voxelQuery.setCameraOrientation(OFF_IN_NEGATIVE_SPACE);
                    _voxelQuery.setCameraNearClip(0.1);
                    _voxelQuery.setCameraFarClip(0.1);
                    if (wantExtraDebugging) {
                        qDebug() << "Using 'minimal' camera position for node " << *node << "\n";
                    }
                } else {
                    if (wantExtraDebugging) {
                        qDebug() << "Using regular camera position for node " << *node << "\n";
                    }
                }
                _voxelQuery.setMaxVoxelPacketsPerSecond(perUnknownServer);
            } else {
                _voxelQuery.setMaxVoxelPacketsPerSecond(0);
            }
            // set up the packet for sending...
            unsigned char* endOfVoxelQueryPacket = voxelQueryPacket;

            // insert packet type/version and node UUID
            endOfVoxelQueryPacket += populateTypeAndVersion(endOfVoxelQueryPacket, PACKET_TYPE_VOXEL_QUERY);
            QByteArray ownerUUID = nodeList->getOwnerUUID().toRfc4122();
            memcpy(endOfVoxelQueryPacket, ownerUUID.constData(), ownerUUID.size());
            endOfVoxelQueryPacket += ownerUUID.size();

            // encode the query data...
            endOfVoxelQueryPacket += _voxelQuery.getBroadcastData(endOfVoxelQueryPacket);
    
            int packetLength = endOfVoxelQueryPacket - voxelQueryPacket;

            nodeSocket->send(node->getActiveSocket(), voxelQueryPacket, packetLength);

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
    float fov         = camera.getFieldOfView();
    float nearClip    = camera.getNearClip();
    float farClip     = camera.getFarClip();
    float aspectRatio = camera.getAspectRatio();

    glm::quat rotation = camera.getRotation();

    // Set the viewFrustum up with the correct position and orientation of the camera    
    viewFrustum.setPosition(position);
    viewFrustum.setOrientation(rotation);
    
    // Also make sure it's got the correct lens details from the camera
    viewFrustum.setAspectRatio(aspectRatio);
    viewFrustum.setFieldOfView(fov);
    viewFrustum.setNearClip(nearClip);
    viewFrustum.setFarClip(farClip);
    viewFrustum.setEyeOffsetPosition(camera.getEyeOffsetPosition());
    viewFrustum.setEyeOffsetOrientation(camera.getEyeOffsetOrientation());

    // Ask the ViewFrustum class to calculate our corners
    viewFrustum.calculate();
}

// this shader is an adaptation (HLSL -> GLSL, removed conditional) of the one in the Oculus sample
// code (Samples/OculusRoomTiny/RenderTiny_D3D1X_Device.cpp), which is under the Apache license
// (http://www.apache.org/licenses/LICENSE-2.0)
static const char* DISTORTION_FRAGMENT_SHADER =
    "#version 120\n"
    "uniform sampler2D texture;"
    "uniform vec2 lensCenter;"
    "uniform vec2 screenCenter;"
    "uniform vec2 scale;"
    "uniform vec2 scaleIn;"
    "uniform vec4 hmdWarpParam;"
    "vec2 hmdWarp(vec2 in01) {"
    "   vec2 theta = (in01 - lensCenter) * scaleIn;"
    "   float rSq = theta.x * theta.x + theta.y * theta.y;"
    "   vec2 theta1 = theta * (hmdWarpParam.x + hmdWarpParam.y * rSq + "
    "                 hmdWarpParam.z * rSq * rSq + hmdWarpParam.w * rSq * rSq * rSq);"
    "   return lensCenter + scale * theta1;"
    "}"
    "void main(void) {"
    "   vec2 tc = hmdWarp(gl_TexCoord[0].st);"
    "   vec2 below = step(screenCenter.st + vec2(-0.25, -0.5), tc.st);"
    "   vec2 above = vec2(1.0, 1.0) - step(screenCenter.st + vec2(0.25, 0.5), tc.st);"
    "   gl_FragColor = mix(vec4(0.0, 0.0, 0.0, 1.0), texture2D(texture, tc), "
    "       above.s * above.t * below.s * below.t);"
    "}";
    
void Application::displayOculus(Camera& whichCamera) {
    _glowEffect.prepare();

    // magic numbers ahoy! in order to avoid pulling in the Oculus utility library that calculates
    // the rendering parameters from the hardware stats, i just folded their calculations into
    // constants using the stats for the current-model hardware as contained in the SDK file
    // LibOVR/Src/Util/Util_Render_Stereo.cpp

    // eye 

    // render the left eye view to the left side of the screen
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.151976, 0, 0); // +h, see Oculus SDK docs p. 26
    gluPerspective(whichCamera.getFieldOfView(), whichCamera.getAspectRatio(),
        whichCamera.getNearClip(), whichCamera.getFarClip());
    
    glViewport(0, 0, _glWidget->width() / 2, _glWidget->height());
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.032, 0, 0); // dip/2, see p. 27
    
    displaySide(whichCamera);

    // and the right eye to the right side
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glTranslatef(-0.151976, 0, 0); // -h
    gluPerspective(whichCamera.getFieldOfView(), whichCamera.getAspectRatio(),
        whichCamera.getNearClip(), whichCamera.getFarClip());
    
    glViewport(_glWidget->width() / 2, 0, _glWidget->width() / 2, _glWidget->height());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-0.032, 0, 0);
    
    displaySide(whichCamera);

    glPopMatrix();
    
    // restore our normal viewport
    glViewport(0, 0, _glWidget->width(), _glWidget->height());

    QOpenGLFramebufferObject* fbo = _glowEffect.render(true);
    glBindTexture(GL_TEXTURE_2D, fbo->texture());

    if (_oculusProgram == 0) {
        _oculusProgram = new ProgramObject();
        _oculusProgram->addShaderFromSourceCode(QGLShader::Fragment, DISTORTION_FRAGMENT_SHADER);
        _oculusProgram->link();
        
        _textureLocation = _oculusProgram->uniformLocation("texture");
        _lensCenterLocation = _oculusProgram->uniformLocation("lensCenter");
        _screenCenterLocation = _oculusProgram->uniformLocation("screenCenter");
        _scaleLocation = _oculusProgram->uniformLocation("scale");
        _scaleInLocation = _oculusProgram->uniformLocation("scaleIn");
        _hmdWarpParamLocation = _oculusProgram->uniformLocation("hmdWarpParam");        
    }
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, _glWidget->width(), 0, _glWidget->height());
    glDisable(GL_DEPTH_TEST);
    
    // for reference on setting these values, see SDK file Samples/OculusRoomTiny/RenderTiny_Device.cpp
    
    float scaleFactor = 1.0 / _oculusDistortionScale;
    float aspectRatio = (_glWidget->width() * 0.5) / _glWidget->height();
    
    glDisable(GL_BLEND);
    _oculusProgram->bind();
    _oculusProgram->setUniformValue(_textureLocation, 0);
    _oculusProgram->setUniformValue(_lensCenterLocation, 0.287994, 0.5); // see SDK docs, p. 29
    _oculusProgram->setUniformValue(_screenCenterLocation, 0.25, 0.5);
    _oculusProgram->setUniformValue(_scaleLocation, 0.25 * scaleFactor, 0.5 * scaleFactor * aspectRatio);
    _oculusProgram->setUniformValue(_scaleInLocation, 4, 2 / aspectRatio);
    _oculusProgram->setUniformValue(_hmdWarpParamLocation, 1.0, 0.22, 0.24, 0);

    glColor3f(1, 0, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(0, 0);
    glTexCoord2f(0.5, 0);
    glVertex2f(_glWidget->width()/2, 0);
    glTexCoord2f(0.5, 1);
    glVertex2f(_glWidget->width() / 2, _glWidget->height());
    glTexCoord2f(0, 1);
    glVertex2f(0, _glWidget->height());
    glEnd();
    
    _oculusProgram->setUniformValue(_lensCenterLocation, 0.787994, 0.5);
    _oculusProgram->setUniformValue(_screenCenterLocation, 0.75, 0.5);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.5, 0);
    glVertex2f(_glWidget->width() / 2, 0);
    glTexCoord2f(1, 0);
    glVertex2f(_glWidget->width(), 0);
    glTexCoord2f(1, 1);
    glVertex2f(_glWidget->width(), _glWidget->height());
    glTexCoord2f(0.5, 1);
    glVertex2f(_glWidget->width() / 2, _glWidget->height());
    glEnd();
    
    glEnable(GL_BLEND);           
    glBindTexture(GL_TEXTURE_2D, 0);
    _oculusProgram->release();
    
    glPopMatrix();
}

const GLfloat WHITE_SPECULAR_COLOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat NO_SPECULAR_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };

void Application::setupWorldLight(Camera& whichCamera) {
    
    //  Setup 3D lights (after the camera transform, so that they are positioned in world space)
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    glm::vec3 relativeSunLoc = glm::normalize(_environment.getClosestData(whichCamera.getPosition()).getSunLocation() -
                                              whichCamera.getPosition());
    GLfloat light_position0[] = { relativeSunLoc.x, relativeSunLoc.y, relativeSunLoc.z, 0.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
    GLfloat ambient_color[] = { 0.7, 0.7, 0.8 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_color);
    GLfloat diffuse_color[] = { 0.8, 0.7, 0.7 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_color);
    
    glLightfv(GL_LIGHT0, GL_SPECULAR, WHITE_SPECULAR_COLOR);    
    glMaterialfv(GL_FRONT, GL_SPECULAR, WHITE_SPECULAR_COLOR);
    glMateriali(GL_FRONT, GL_SHININESS, 96);
}

void Application::loadTranslatedViewMatrix(const glm::vec3& translation) {
    glLoadMatrixf((const GLfloat*)&_untranslatedViewMatrix);
    glTranslatef(translation.x + _viewMatrixTranslation.x, translation.y + _viewMatrixTranslation.y,
        translation.z + _viewMatrixTranslation.z);
}

void Application::computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& near,
    float& far, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const {
    
    _viewFrustum.computeOffAxisFrustum(left, right, bottom, top, near, far, nearClipPlane, farClipPlane);
}

void Application::displaySide(Camera& whichCamera, bool selfAvatarOnly) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "Application::displaySide()");
    // transform by eye offset

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
    glRotatef(-glm::angle(eyeOffsetOrient), eyeOffsetAxis.x, eyeOffsetAxis.y, eyeOffsetAxis.z);
    glTranslatef(-eyeOffsetPos.x, -eyeOffsetPos.y, -eyeOffsetPos.z);

    // transform view according to whichCamera
    // could be myCamera (if in normal mode)
    // or could be viewFrustumOffsetCamera if in offset mode

    glm::quat rotation = whichCamera.getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(-glm::angle(rotation), axis.x, axis.y, axis.z);

    // store view matrix without translation, which we'll use for precision-sensitive objects
    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&_untranslatedViewMatrix);
    _viewMatrixTranslation = -whichCamera.getPosition();

    glTranslatef(_viewMatrixTranslation.x, _viewMatrixTranslation.y, _viewMatrixTranslation.z);

    //  Setup 3D lights (after the camera transform, so that they are positioned in world space)
    setupWorldLight(whichCamera);
    
    if (!selfAvatarOnly && Menu::getInstance()->isOptionChecked(MenuOption::Stars)) {
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
            float height = glm::distance(whichCamera.getPosition(), closestData.getAtmosphereCenter());
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

    // draw the sky dome
    if (!selfAvatarOnly && Menu::getInstance()->isOptionChecked(MenuOption::Atmosphere)) {
        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
            "Application::displaySide() ... atmosphere...");
        _environment.renderAtmospheres(whichCamera);
    }
    
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    
    //  Enable to show line from me to the voxel I am touching
    //renderLineToTouchedVoxel();
    //renderThrustAtVoxel(_voxelThrust);
    
    if (!selfAvatarOnly) {
        // draw a red sphere  
        float sphereRadius = 0.25f;
        glColor3f(1,0,0);
        glPushMatrix();
            glutSolidSphere(sphereRadius, 15, 15);
        glPopMatrix();

        // disable specular lighting for ground and voxels
        glMaterialfv(GL_FRONT, GL_SPECULAR, NO_SPECULAR_COLOR);

        //  Draw Cloud Particles
        if (Menu::getInstance()->isOptionChecked(MenuOption::ParticleCloud)) {
            _cloud.render();
        }
        //  Draw voxels
        if (Menu::getInstance()->isOptionChecked(MenuOption::Voxels)) {
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                "Application::displaySide() ... voxels...");
            if (!Menu::getInstance()->isOptionChecked(MenuOption::DontRenderVoxels)) {
                _voxels.render(Menu::getInstance()->isOptionChecked(MenuOption::VoxelTextures));
            }
        }
    
    
        // restore default, white specular
        glMaterialfv(GL_FRONT, GL_SPECULAR, WHITE_SPECULAR_COLOR);
    
        // indicate what we'll be adding/removing in mouse mode, if anything
        if (_mouseVoxel.s != 0 && whichCamera.getMode() != CAMERA_MODE_MIRROR) {
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                "Application::displaySide() ... voxels TOOLS UX...");

            glDisable(GL_LIGHTING);
            glPushMatrix();
            glScalef(TREE_SCALE, TREE_SCALE, TREE_SCALE);
            const float CUBE_EXPANSION = 1.01f;
            if (_nudgeStarted) {
                renderNudgeGuide(_nudgeGuidePosition.x, _nudgeGuidePosition.y, _nudgeGuidePosition.z, _nudgeVoxel.s);
                renderNudgeGrid(_nudgeVoxel.x, _nudgeVoxel.y, _nudgeVoxel.z, _nudgeVoxel.s, _mouseVoxel.s);
                glPushMatrix();
                glTranslatef(_nudgeVoxel.x + _nudgeVoxel.s * 0.5f,
                    _nudgeVoxel.y + _nudgeVoxel.s * 0.5f,
                    _nudgeVoxel.z + _nudgeVoxel.s * 0.5f);
                glColor3ub(255, 255, 255);
                glLineWidth(4.0f);
                glutWireCube(_nudgeVoxel.s * CUBE_EXPANSION);
                glPopMatrix();
            } else {
                renderMouseVoxelGrid(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
            }

            if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelAddMode)) {
                // use a contrasting color so that we can see what we're doing
                glColor3ub(_mouseVoxel.red + 128, _mouseVoxel.green + 128, _mouseVoxel.blue + 128);
            } else {
                glColor3ub(_mouseVoxel.red, _mouseVoxel.green, _mouseVoxel.blue);
            }
        
            if (_nudgeStarted) {
                // render nudge guide cube
                glTranslatef(_nudgeGuidePosition.x + _nudgeVoxel.s*0.5f,
                    _nudgeGuidePosition.y + _nudgeVoxel.s*0.5f,
                    _nudgeGuidePosition.z + _nudgeVoxel.s*0.5f);
                glLineWidth(4.0f);
                glutWireCube(_nudgeVoxel.s * CUBE_EXPANSION);
            } else {
                glTranslatef(_mouseVoxel.x + _mouseVoxel.s*0.5f,
                    _mouseVoxel.y + _mouseVoxel.s*0.5f,
                    _mouseVoxel.z + _mouseVoxel.s*0.5f);
                glLineWidth(4.0f);
                glutWireCube(_mouseVoxel.s * CUBE_EXPANSION);
            }
            glLineWidth(1.0f);
            glPopMatrix();
            glEnable(GL_LIGHTING);
        }
    
        if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelSelectMode) && _pasteMode && whichCamera.getMode() != CAMERA_MODE_MIRROR) {
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                "Application::displaySide() ... PASTE Preview...");

            glPushMatrix();
            glTranslatef(_mouseVoxel.x * TREE_SCALE,
                         _mouseVoxel.y * TREE_SCALE,
                         _mouseVoxel.z * TREE_SCALE);
            glScalef(_mouseVoxel.s,
                     _mouseVoxel.s,
                     _mouseVoxel.s);

            _sharedVoxelSystem.render(true);
            glPopMatrix();
        }
    }

    _myAvatar.renderScreenTint(SCREEN_TINT_BEFORE_AVATARS, whichCamera);
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::Avatars)) {
        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
            "Application::displaySide() ... Avatars...");


        if (!selfAvatarOnly) {
            //  Render avatars of other nodes
            NodeList* nodeList = NodeList::getInstance();
        
            for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                node->lock();
            
                if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
                    Avatar *avatar = (Avatar *)node->getLinkedData();
                    if (!avatar->isInitialized()) {
                        avatar->init();
                    }
                    avatar->render(false, Menu::getInstance()->isOptionChecked(MenuOption::AvatarAsBalls));
                    avatar->setDisplayingLookatVectors(Menu::getInstance()->isOptionChecked(MenuOption::LookAtVectors));
                }
            
                node->unlock();
            }
        }
        
        // Render my own Avatar
        _myAvatar.render(whichCamera.getMode() == CAMERA_MODE_MIRROR,
                         Menu::getInstance()->isOptionChecked(MenuOption::AvatarAsBalls));
        _myAvatar.setDisplayingLookatVectors(Menu::getInstance()->isOptionChecked(MenuOption::LookAtVectors));

        if (Menu::getInstance()->isOptionChecked(MenuOption::LookAtIndicator) && _lookatTargetAvatar) {
            renderLookatIndicator(_lookatOtherPosition, whichCamera);
        }
    }

    _myAvatar.renderScreenTint(SCREEN_TINT_AFTER_AVATARS, whichCamera);

    if (!selfAvatarOnly) {
        //  Render the world box
        if (whichCamera.getMode() != CAMERA_MODE_MIRROR && Menu::getInstance()->isOptionChecked(MenuOption::Stats)) {
            renderWorldBox();
        }
    
        // render the ambient occlusion effect if enabled
        if (Menu::getInstance()->isOptionChecked(MenuOption::AmbientOcclusion)) {
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                "Application::displaySide() ... AmbientOcclusion...");
            _ambientOcclusionEffect.render();
        }
    
        // brad's frustum for debugging
        if (Menu::getInstance()->isOptionChecked(MenuOption::DisplayFrustum) && whichCamera.getMode() != CAMERA_MODE_MIRROR) {
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                "Application::displaySide() ... renderViewFrustum...");
            renderViewFrustum(_viewFrustum);
        }

        // render voxel fades if they exist
        if (_voxelFades.size() > 0) {
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                "Application::displaySide() ... voxel fades...");
            for(std::vector<VoxelFade>::iterator fade = _voxelFades.begin(); fade != _voxelFades.end();) {
                fade->render();
                if(fade->isDone()) {
                    fade = _voxelFades.erase(fade);
                } else {
                    ++fade;
                }
            }
        }

        {        
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                "Application::displaySide() ... renderFollowIndicator...");
            renderFollowIndicator();
        }
    
        // render transmitter pick ray, if non-empty
        if (_transmitterPickStart != _transmitterPickEnd) {
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                "Application::displaySide() ... transmitter pick ray...");

            Glower glower;
            const float TRANSMITTER_PICK_COLOR[] = { 1.0f, 1.0f, 0.0f };
            glColor3fv(TRANSMITTER_PICK_COLOR);
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glVertex3f(_transmitterPickStart.x, _transmitterPickStart.y, _transmitterPickStart.z);
            glVertex3f(_transmitterPickEnd.x, _transmitterPickEnd.y, _transmitterPickEnd.z);
            glEnd();
            glLineWidth(1.0f);
        
            glPushMatrix();
            glTranslatef(_transmitterPickEnd.x, _transmitterPickEnd.y, _transmitterPickEnd.z);
        
            const float PICK_END_RADIUS = 0.025f;
            glutSolidSphere(PICK_END_RADIUS, 8, 8);
        
            glPopMatrix();
        }
    }
}

void Application::displayOverlay() {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "Application::displayOverlay()");

    //  Render 2D overlay:  I/O level bar graphs and text  
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity(); 
        gluOrtho2D(0, _glWidget->width(), _glWidget->height(), 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
    
        //  Display a single screen-size quad to create an alpha blended 'collision' flash
        float collisionSoundMagnitude = _audio.getCollisionSoundMagnitude();
        const float VISIBLE_COLLISION_SOUND_MAGNITUDE = 0.5f;
        if (collisionSoundMagnitude > VISIBLE_COLLISION_SOUND_MAGNITUDE) {
                renderCollisionOverlay(_glWidget->width(), _glWidget->height(), _audio.getCollisionSoundMagnitude());
        }
   
        #ifndef _WIN32
        if (Menu::getInstance()->isOptionChecked(MenuOption::Stats)) {
            _audio.render(_glWidget->width(), _glWidget->height());
            if (Menu::getInstance()->isOptionChecked(MenuOption::Oscilloscope)) {
                _audioScope.render(45, _glWidget->height() - 200);
            }
        }
        #endif

       //noiseTest(_glWidget->width(), _glWidget->height());
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::HeadMouse)
        && USING_INVENSENSE_MPU9150) {
        //  Display small target box at center or head mouse target that can also be used to measure LOD
        glColor3f(1.0, 1.0, 1.0);
        glDisable(GL_LINE_SMOOTH);
        const int PIXEL_BOX = 16;
        glBegin(GL_LINES);
        glVertex2f(_headMouseX - PIXEL_BOX/2, _headMouseY);
        glVertex2f(_headMouseX + PIXEL_BOX/2, _headMouseY);
        glVertex2f(_headMouseX, _headMouseY - PIXEL_BOX/2);
        glVertex2f(_headMouseX, _headMouseY + PIXEL_BOX/2);
        glEnd();            
        glEnable(GL_LINE_SMOOTH);
        glColor3f(1.f, 0.f, 0.f);
        glPointSize(3.0f);
        glDisable(GL_POINT_SMOOTH);
        glBegin(GL_POINTS);
        glVertex2f(_headMouseX - 1, _headMouseY + 1);
        glEnd();
        //  If Faceshift is active, show eye pitch and yaw as separate pointer
        if (_faceshift.isActive()) {
            const float EYE_TARGET_PIXELS_PER_DEGREE = 40.0;
            int eyeTargetX = (_glWidget->width() / 2) -  _faceshift.getEstimatedEyeYaw() * EYE_TARGET_PIXELS_PER_DEGREE;
            int eyeTargetY = (_glWidget->height() / 2) -  _faceshift.getEstimatedEyePitch() * EYE_TARGET_PIXELS_PER_DEGREE;
            
            glColor3f(0.0, 1.0, 1.0);
            glDisable(GL_LINE_SMOOTH);
            glBegin(GL_LINES);
            glVertex2f(eyeTargetX - PIXEL_BOX/2, eyeTargetY);
            glVertex2f(eyeTargetX + PIXEL_BOX/2, eyeTargetY);
            glVertex2f(eyeTargetX, eyeTargetY - PIXEL_BOX/2);
            glVertex2f(eyeTargetX, eyeTargetY + PIXEL_BOX/2);
            glEnd();

        }
    }
        
    //  Show detected levels from the serial I/O ADC channel sensors
    if (_displayLevels) _serialHeadSensor.renderLevels(_glWidget->width(), _glWidget->height());
    
    //  Show hand transmitter data if detected
    if (_myTransmitter.isConnected()) {
        _myTransmitter.renderLevels(_glWidget->width(), _glWidget->height());
    }
    //  Display stats and log text onscreen
    glLineWidth(1.0f);
    glPointSize(1.0f);
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::Stats)) {
        //  Onscreen text about position, servers, etc 
        displayStats();
        //  Bandwidth meter 
        if (Menu::getInstance()->isOptionChecked(MenuOption::Bandwidth)) {
            _bandwidthMeter.render(_glWidget->width(), _glWidget->height());
        }
        //  Stats at upper right of screen about who domain server is telling us about
        glPointSize(1.0f);
        char nodes[100];
        
        NodeList* nodeList = NodeList::getInstance();
        int totalAvatars = 0, totalServers = 0;
        
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            node->getType() == NODE_TYPE_AGENT ? totalAvatars++ : totalServers++;
        }
        sprintf(nodes, "Servers: %d, Avatars: %d\n", totalServers, totalAvatars);
        drawtext(_glWidget->width() - 150, 20, 0.10, 0, 1.0, 0, nodes, 1, 0, 0);
    }

    // testing rendering coverage map
    if (Menu::getInstance()->isOptionChecked(MenuOption::CoverageMapV2)) {
        renderCoverageMapV2();
    }
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::CoverageMap)) {
        renderCoverageMap();
    }
    

    if (Menu::getInstance()->isOptionChecked(MenuOption::Log)) {
        LogDisplay::instance.render(_glWidget->width(), _glWidget->height());
    }

    //  Show chat entry field
    if (_chatEntryOn) {
        _chatEntry.render(_glWidget->width(), _glWidget->height());
    }
    
    //  Show on-screen msec timer
    if (Menu::getInstance()->isOptionChecked(MenuOption::FrameTimer)) {
        char frameTimer[10];
        uint64_t mSecsNow = floor(usecTimestampNow() / 1000.0 + 0.5);
        sprintf(frameTimer, "%d\n", (int)(mSecsNow % 1000));
        drawtext(_glWidget->width() - 100, _glWidget->height() - 20, 0.30, 0, 1.0, 0, frameTimer, 0, 0, 0);
        drawtext(_glWidget->width() - 102, _glWidget->height() - 22, 0.30, 0, 1.0, 0, frameTimer, 1, 1, 1);
    }

    
    // render the webcam input frame
    _webcam.renderPreview(_glWidget->width(), _glWidget->height());

    _palette.render(_glWidget->width(), _glWidget->height());

    QAction* paintColorAction = NULL;
    if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelGetColorMode)
        && (paintColorAction = Menu::getInstance()->getActionForOption(MenuOption::VoxelPaintColor))->data().value<QColor>()
            != _swatch.getColor()) {
        QColor color = paintColorAction->data().value<QColor>();
        TextRenderer textRenderer(SANS_FONT_FAMILY, 11, 50);
        const char line1[] = "Assign this color to a swatch";
        const char line2[] = "by choosing a key from 1 to 8.";

        int left = (_glWidget->width() - POPUP_WIDTH - 2 * POPUP_MARGIN) / 2;
        int top = _glWidget->height() / 40;

        glBegin(GL_POLYGON);
        glColor3f(0.0f, 0.0f, 0.0f);
        for (double a = M_PI; a < 1.5f * M_PI; a += POPUP_STEP) {
            glVertex2f(left + POPUP_MARGIN * cos(a)              , top + POPUP_MARGIN * sin(a));
        }
        for (double a = 1.5f * M_PI; a < 2.0f * M_PI; a += POPUP_STEP) {
            glVertex2f(left + POPUP_WIDTH + POPUP_MARGIN * cos(a), top + POPUP_MARGIN * sin(a));
        }
        for (double a = 0.0f; a < 0.5f * M_PI; a += POPUP_STEP) {
            glVertex2f(left + POPUP_WIDTH + POPUP_MARGIN * cos(a), top + POPUP_HEIGHT + POPUP_MARGIN * sin(a));
        }
        for (double a = 0.5f * M_PI; a < 1.0f * M_PI; a += POPUP_STEP) {
            glVertex2f(left + POPUP_MARGIN * cos(a)              , top + POPUP_HEIGHT + POPUP_MARGIN * sin(a));
        }
        glEnd();

        glBegin(GL_QUADS);
        glColor3f(color.redF(),
                  color.greenF(),
                  color.blueF());
        glVertex2f(left               , top);
        glVertex2f(left + SWATCH_WIDTH, top);
        glVertex2f(left + SWATCH_WIDTH, top + SWATCH_HEIGHT);
        glVertex2f(left               , top + SWATCH_HEIGHT);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        textRenderer.draw(left + SWATCH_WIDTH + POPUP_MARGIN, top + FIRST_LINE_OFFSET , line1);
        textRenderer.draw(left + SWATCH_WIDTH + POPUP_MARGIN, top + SECOND_LINE_OFFSET, line2);
    }
    else {
        _swatch.checkColor();
    }

    if (_pieMenu.isDisplayed()) {
        _pieMenu.render();
    }
    
    glPopMatrix();
}

void Application::displayStats() {
    int statsVerticalOffset = 8;
    const int PELS_PER_LINE = 15;
    char stats[200];
    statsVerticalOffset += PELS_PER_LINE;
    sprintf(stats, "%3.0f FPS, %d Pkts/sec, %3.2f Mbps   ", 
            _fps, _packetsPerSecond,  (float)_bytesPerSecond * 8.f / 1000000.f);
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, stats);

    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        int pingAudio = 0, pingAvatar = 0, pingVoxel = 0, pingVoxelMax = 0;

        NodeList* nodeList = NodeList::getInstance();
        Node* audioMixerNode = nodeList->soloNodeOfType(NODE_TYPE_AUDIO_MIXER);
        Node* avatarMixerNode = nodeList->soloNodeOfType(NODE_TYPE_AVATAR_MIXER);

        pingAudio = audioMixerNode ? audioMixerNode->getPingMs() : 0;
        pingAvatar = avatarMixerNode ? avatarMixerNode->getPingMs() : 0;


        // Now handle voxel servers, since there could be more than one, we average their ping times
        unsigned long totalPingVoxel = 0;
        int voxelServerCount = 0;
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            if (node->getType() == NODE_TYPE_VOXEL_SERVER) {
                totalPingVoxel += node->getPingMs();
                voxelServerCount++;
                if (pingVoxelMax < node->getPingMs()) {
                    pingVoxelMax = node->getPingMs();
                }
            }
        }
        if (voxelServerCount) {
            pingVoxel = totalPingVoxel/voxelServerCount;
        }

        char pingStats[200];
        statsVerticalOffset += PELS_PER_LINE;
        sprintf(pingStats, "Ping audio/avatar/voxel: %d / %d / %d avg %d max ", pingAudio, pingAvatar, pingVoxel, pingVoxelMax);
        drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, pingStats);
    }
    
    char avatarStats[200];
    statsVerticalOffset += PELS_PER_LINE;
    glm::vec3 avatarPos = _myAvatar.getPosition();
    sprintf(avatarStats, "Avatar: pos %.3f, %.3f, %.3f, vel %.1f, yaw = %.2f", avatarPos.x, avatarPos.y, avatarPos.z, glm::length(_myAvatar.getVelocity()), _myAvatar.getBodyYaw());
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, avatarStats);

 
    QLocale locale(QLocale::English);

    std::stringstream voxelStats;
    voxelStats.precision(4);
    voxelStats << "Voxels " << 
        "Max: " << _voxels.getMaxVoxels() / 1000.f << "K " << 
        "Rendered: " << _voxels.getVoxelsRendered() / 1000.f << "K " <<
        "Written: " << _voxels.getVoxelsWritten() / 1000.f << "K " <<
        "Abandoned: " << _voxels.getAbandonedVoxels() / 1000.f << "K " <<
        "Updated: " << _voxels.getVoxelsUpdated() / 1000.f << "K ";
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, (char*)voxelStats.str().c_str());

    voxelStats.str("");
    voxelStats << 
        "Voxels Memory Nodes: " << VoxelNode::getTotalMemoryUsage() / 1000000.f << "MB "
        "Geometry RAM: " << _voxels.getVoxelMemoryUsageRAM() / 1000000.f << "MB " <<
        "VBO: " << _voxels.getVoxelMemoryUsageVBO() / 1000000.f << "MB ";
    if (_voxels.hasVoxelMemoryUsageGPU()) {
        voxelStats << "GPU: " << _voxels.getVoxelMemoryUsageGPU() / 1000000.f << "MB ";
    }
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, (char*)voxelStats.str().c_str());

    unsigned long localTotal = VoxelNode::getNodeCount();
    unsigned long localInternal = VoxelNode::getInternalNodeCount();
    unsigned long localLeaves = VoxelNode::getLeafNodeCount();
    QString localTotalString = locale.toString((uint)localTotal); // consider adding: .rightJustified(10, ' ');
    QString localInternalString = locale.toString((uint)localInternal);
    QString localLeavesString = locale.toString((uint)localLeaves);


    voxelStats.str("");
    voxelStats << 
        "Local Voxels Total: " << localTotalString.toLocal8Bit().constData() << " / " <<
        "Internal: " << localInternalString.toLocal8Bit().constData() << " / " <<
        "Leaves: " << localLeavesString.toLocal8Bit().constData() << "";
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, (char*)voxelStats.str().c_str());

    voxelStats.str("");
    char* voxelDetails = _voxelSceneStats.getItemValue(VoxelSceneStats::ITEM_VOXELS);
    voxelStats << "Voxels Sent from Server: " << voxelDetails;
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, (char*)voxelStats.str().c_str());

    voxelStats.str("");
    voxelDetails = _voxelSceneStats.getItemValue(VoxelSceneStats::ITEM_ELAPSED);
    voxelStats << "Scene Send Time from Server: " << voxelDetails;
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, (char*)voxelStats.str().c_str());

    voxelStats.str("");
    voxelDetails = _voxelSceneStats.getItemValue(VoxelSceneStats::ITEM_ENCODE);
    voxelStats << "Encode Time on Server: " << voxelDetails;
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, (char*)voxelStats.str().c_str());

    voxelStats.str("");
    voxelDetails = _voxelSceneStats.getItemValue(VoxelSceneStats::ITEM_MODE);
    voxelStats << "Sending Mode: " << voxelDetails;
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, (char*)voxelStats.str().c_str());

    voxelStats.str("");
    int voxelPacketsToProcess = _voxelProcessor.packetsToProcessCount();
    QString packetsString = locale.toString((int)voxelPacketsToProcess);
    QString maxString = locale.toString((int)_recentMaxPackets);

    voxelStats << "Voxel Packets to Process: " << packetsString.toLocal8Bit().constData() 
                << " [Recent Max: " << maxString.toLocal8Bit().constData() << "]";
                
    if (_resetRecentMaxPacketsSoon && voxelPacketsToProcess > 0) {
        _recentMaxPackets = 0;
        _resetRecentMaxPacketsSoon = false;
    }
    if (voxelPacketsToProcess == 0) {
        _resetRecentMaxPacketsSoon = true;
    } else {
        if (voxelPacketsToProcess > _recentMaxPackets) {
            _recentMaxPackets = voxelPacketsToProcess;
        }
    }
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, (char*)voxelStats.str().c_str());

    
    Node *avatarMixer = NodeList::getInstance()->soloNodeOfType(NODE_TYPE_AVATAR_MIXER);
    char avatarMixerStats[200];
    
    if (avatarMixer) {
        sprintf(avatarMixerStats, "Avatar Mixer: %.f kbps, %.f pps",
                roundf(avatarMixer->getAverageKilobitsPerSecond()),
                roundf(avatarMixer->getAveragePacketsPerSecond()));
    } else {
        sprintf(avatarMixerStats, "No Avatar Mixer");
    }
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, avatarMixerStats);
    statsVerticalOffset += PELS_PER_LINE;
    drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, (char*)LeapManager::statusString().c_str());
    
    if (_perfStatsOn) {
        // Get the PerfStats group details. We need to allocate and array of char* long enough to hold 1+groups
        char** perfStatLinesArray = new char*[PerfStat::getGroupCount()+1];
        int lines = PerfStat::DumpStats(perfStatLinesArray);
    
        for (int line=0; line < lines; line++) {
            statsVerticalOffset += PELS_PER_LINE;
            drawtext(10, statsVerticalOffset, 0.10f, 0, 1.0, 0, perfStatLinesArray[line]);
            delete perfStatLinesArray[line]; // we're responsible for cleanup
            perfStatLinesArray[line]=NULL;
        }
        delete []perfStatLinesArray; // we're responsible for cleanup
    }
}

void Application::renderThrustAtVoxel(const glm::vec3& thrust) {
    if (_mousePressed) {
        glColor3f(1, 0, 0);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glm::vec3 voxelTouched = getMouseVoxelWorldCoordinates(_mouseVoxelDragging);
        glVertex3f(voxelTouched.x, voxelTouched.y, voxelTouched.z);
        glVertex3f(voxelTouched.x + thrust.x, voxelTouched.y + thrust.y, voxelTouched.z + thrust.z);
        glEnd();
    }
}

void Application::renderLineToTouchedVoxel() {
    //  Draw a teal line to the voxel I am currently dragging on
    if (_mousePressed) {
        glColor3f(0, 1, 1);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glm::vec3 voxelTouched = getMouseVoxelWorldCoordinates(_mouseVoxelDragging);
        glVertex3f(voxelTouched.x, voxelTouched.y, voxelTouched.z);
        glm::vec3 headPosition = _myAvatar.getHeadJointPosition();
        glVertex3fv(&headPosition.x);
        glEnd();
    }
}


glm::vec2 Application::getScaledScreenPoint(glm::vec2 projectedPoint) {
    float horizontalScale = _glWidget->width() / 2.0f;
    float verticalScale   = _glWidget->height() / 2.0f;
    
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
        ((projectedPoint.y + 1.0) * -verticalScale) + _glWidget->height());
        
    return screenPoint;
}

// render the coverage map on screen
void Application::renderCoverageMapV2() {
    
    //qDebug("renderCoverageMap()\n");
    
    glDisable(GL_LIGHTING);
    glLineWidth(2.0);
    glBegin(GL_LINES);
    glColor3f(0,1,1);
    
    renderCoverageMapsV2Recursively(&_voxels.myCoverageMapV2);

    glEnd();
    glEnable(GL_LIGHTING);
}

void Application::renderCoverageMapsV2Recursively(CoverageMapV2* map) {
    // render ourselves...
    if (map->isCovered()) {
        BoundingBox box = map->getBoundingBox();

        glm::vec2 firstPoint = getScaledScreenPoint(box.getVertex(0));
        glm::vec2 lastPoint(firstPoint);
    
        for (int i = 1; i < box.getVertexCount(); i++) {
            glm::vec2 thisPoint = getScaledScreenPoint(box.getVertex(i));

            glVertex2f(lastPoint.x, lastPoint.y);
            glVertex2f(thisPoint.x, thisPoint.y);
            lastPoint = thisPoint;
        }

        glVertex2f(lastPoint.x, lastPoint.y);
        glVertex2f(firstPoint.x, firstPoint.y);
    } else {
        // iterate our children and call render on them.
        for (int i = 0; i < CoverageMapV2::NUMBER_OF_CHILDREN; i++) {
            CoverageMapV2* childMap = map->getChild(i);
            if (childMap) {
                renderCoverageMapsV2Recursively(childMap);
            }
        }
    }
}

// render the coverage map on screen
void Application::renderCoverageMap() {
    
    //qDebug("renderCoverageMap()\n");
    
    glDisable(GL_LIGHTING);
    glLineWidth(2.0);
    glBegin(GL_LINES);
    glColor3f(0,0,1);
    
    renderCoverageMapsRecursively(&_voxels.myCoverageMap);

    glEnd();
    glEnable(GL_LIGHTING);
}

void Application::renderCoverageMapsRecursively(CoverageMap* map) {
    for (int i = 0; i < map->getPolygonCount(); i++) {
    
        VoxelProjectedPolygon* polygon = map->getPolygon(i);
        
        if (polygon->getProjectionType()        == (PROJECTION_RIGHT | PROJECTION_NEAR | PROJECTION_BOTTOM)) {
            glColor3f(.5,0,0); // dark red
        } else if (polygon->getProjectionType() == (PROJECTION_NEAR | PROJECTION_RIGHT)) {
            glColor3f(.5,.5,0); // dark yellow
        } else if (polygon->getProjectionType() == (PROJECTION_NEAR | PROJECTION_LEFT)) {
            glColor3f(.5,.5,.5); // gray
        } else if (polygon->getProjectionType() == (PROJECTION_NEAR | PROJECTION_LEFT | PROJECTION_BOTTOM)) {
            glColor3f(.5,0,.5); // dark magenta
        } else if (polygon->getProjectionType() == (PROJECTION_NEAR | PROJECTION_BOTTOM)) {
            glColor3f(.75,0,0); // red
        } else if (polygon->getProjectionType() == (PROJECTION_NEAR | PROJECTION_TOP)) {
            glColor3f(1,0,1); // magenta
        } else if (polygon->getProjectionType() == (PROJECTION_NEAR | PROJECTION_LEFT | PROJECTION_TOP)) {
            glColor3f(0,0,1); // Blue
        } else if (polygon->getProjectionType() == (PROJECTION_NEAR | PROJECTION_RIGHT | PROJECTION_TOP)) {
            glColor3f(0,1,0); // green
        } else if (polygon->getProjectionType() == (PROJECTION_NEAR)) {
            glColor3f(1,1,0); // yellow
        } else if (polygon->getProjectionType() == (PROJECTION_FAR | PROJECTION_RIGHT | PROJECTION_BOTTOM)) {
            glColor3f(0,.5,.5); // dark cyan
        } else {
            glColor3f(1,0,0);
        }

        glm::vec2 firstPoint = getScaledScreenPoint(polygon->getVertex(0));
        glm::vec2 lastPoint(firstPoint);
    
        for (int i = 1; i < polygon->getVertexCount(); i++) {
            glm::vec2 thisPoint = getScaledScreenPoint(polygon->getVertex(i));

            glVertex2f(lastPoint.x, lastPoint.y);
            glVertex2f(thisPoint.x, thisPoint.y);
            lastPoint = thisPoint;
        }

        glVertex2f(lastPoint.x, lastPoint.y);
        glVertex2f(firstPoint.x, firstPoint.y);
    }

    // iterate our children and call render on them.
    for (int i = 0; i < CoverageMapV2::NUMBER_OF_CHILDREN; i++) {
        CoverageMap* childMap = map->getChild(i);
        if (childMap) {
            renderCoverageMapsRecursively(childMap);
        }
    }
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

void Application::injectVoxelAddedSoundEffect() {
    AudioInjector* voxelInjector = AudioInjectionManager::injectorWithCapacity(11025);
    
    if (voxelInjector) {
        voxelInjector->setPosition(glm::vec3(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z));
        //voxelInjector->setBearing(-1 * _myAvatar.getAbsoluteHeadYaw());
        voxelInjector->setVolume (16 * pow (_mouseVoxel.s, 2) / .0000001); //255 is max, and also default value
    
        /* for (int i = 0; i
         < 22050; i++) {
         if (i % 4 == 0) {
         voxelInjector->addSample(4000);
         } else if (i % 4 == 1) {
         voxelInjector->addSample(0);
         } else if (i % 4 == 2) {
         voxelInjector->addSample(-4000);
         } else {
         voxelInjector->addSample(0);
         }
         */
    
        const float BIG_VOXEL_MIN_SIZE = .01f;
    
        for (int i = 0; i < 11025; i++) {
        
            /*
             A440 square wave
             if (sin(i * 2 * PIE / 50)>=0) {
             voxelInjector->addSample(4000);
             } else {
             voxelInjector->addSample(-4000);
             }
             */
        
            if (_mouseVoxel.s > BIG_VOXEL_MIN_SIZE) {
                voxelInjector->addSample(20000 * sin((i * 2 * PIE) / (500 * sin((i + 1) / 200))));
            } else {
                voxelInjector->addSample(16000 * sin(i / (1.5 * log (_mouseVoxel.s / .0001) * ((i + 11025) / 5512.5)))); //808
            }
        }
    
        //voxelInjector->addSample(32500 * sin(i/(2 * 1 * ((i+5000)/5512.5)))); //80
        //voxelInjector->addSample(20000 * sin(i/(6 * (_mouseVoxel.s/.001) *((i+5512.5)/5512.5)))); //808
        //voxelInjector->addSample(20000 * sin(i/(6 * ((i+5512.5)/5512.5)))); //808
        //voxelInjector->addSample(4000 * sin(i * 2 * PIE /50)); //A440 sine wave
        //voxelInjector->addSample(4000 * sin(i * 2 * PIE /50) * sin (i/500)); //A440 sine wave with amplitude modulation
    
        //FM library
        //voxelInjector->addSample(20000 * sin((i * 2 * PIE) /(500*sin((i+1)/200))));  //FM 1 dubstep
        //voxelInjector->addSample(20000 * sin((i * 2 * PIE) /(300*sin((i+1)/5.0))));  //FM 2 flange sweep
        //voxelInjector->addSample(10000 * sin((i * 2 * PIE) /(500*sin((i+1)/500.0))));  //FM 3 resonant pulse

        AudioInjectionManager::threadInjector(voxelInjector);
    }
}

bool Application::maybeEditVoxelUnderCursor() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelAddMode)
        || Menu::getInstance()->isOptionChecked(MenuOption::VoxelColorMode)) {
        if (_mouseVoxel.s != 0) {
            PACKET_TYPE message = Menu::getInstance()->isOptionChecked(MenuOption::DestructiveAddVoxel)
                ? PACKET_TYPE_SET_VOXEL_DESTRUCTIVE
                : PACKET_TYPE_SET_VOXEL;
            _voxelEditSender.sendVoxelEditMessage(message, _mouseVoxel);
            
            // create the voxel locally so it appears immediately
            _voxels.createVoxel(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s,
                                _mouseVoxel.red, _mouseVoxel.green, _mouseVoxel.blue,
                                Menu::getInstance()->isOptionChecked(MenuOption::DestructiveAddVoxel));

            // Implement voxel fade effect
            VoxelFade fade(VoxelFade::FADE_OUT, 1.0f, 1.0f, 1.0f);
            const float VOXEL_BOUNDS_ADJUST = 0.01f;
            float slightlyBigger = _mouseVoxel.s * VOXEL_BOUNDS_ADJUST;
            fade.voxelDetails.x = _mouseVoxel.x - slightlyBigger;
            fade.voxelDetails.y = _mouseVoxel.y - slightlyBigger;
            fade.voxelDetails.z = _mouseVoxel.z - slightlyBigger;
            fade.voxelDetails.s = _mouseVoxel.s + slightlyBigger + slightlyBigger;
            _voxelFades.push_back(fade);
            
            // inject a sound effect
            injectVoxelAddedSoundEffect();
            
            // remember the position for drag detection
            _justEditedVoxel = true;
            
        }
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelDeleteMode)) {
        deleteVoxelUnderCursor();
        VoxelFade fade(VoxelFade::FADE_OUT, 1.0f, 1.0f, 1.0f);
        const float VOXEL_BOUNDS_ADJUST = 0.01f;
        float slightlyBigger = _mouseVoxel.s * VOXEL_BOUNDS_ADJUST;
        fade.voxelDetails.x = _mouseVoxel.x - slightlyBigger;
        fade.voxelDetails.y = _mouseVoxel.y - slightlyBigger;
        fade.voxelDetails.z = _mouseVoxel.z - slightlyBigger;
        fade.voxelDetails.s = _mouseVoxel.s + slightlyBigger + slightlyBigger;
        _voxelFades.push_back(fade);
        
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::VoxelGetColorMode)) {
        eyedropperVoxelUnderCursor();
    } else {
        return false;
    }

    return true;
}

void Application::deleteVoxelUnderCursor() {
    if (_mouseVoxel.s != 0) {
        // sending delete to the server is sufficient, server will send new version so we see updates soon enough
        _voxelEditSender.sendVoxelEditMessage(PACKET_TYPE_ERASE_VOXEL, _mouseVoxel);

        // delete it locally to see the effect immediately (and in case no voxel server is present)
        _voxels.deleteVoxelAt(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);

        AudioInjector* voxelInjector = AudioInjectionManager::injectorWithCapacity(5000);
        
        if (voxelInjector) {
            voxelInjector->setPosition(glm::vec3(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z));
            //voxelInjector->setBearing(0); //straight down the z axis
            voxelInjector->setVolume (255); //255 is max, and also default value
            
            
            for (int i = 0; i < 5000; i++) {
                voxelInjector->addSample(10000 * sin((i * 2 * PIE) / (500 * sin((i + 1) / 500.0))));  //FM 3 resonant pulse
                //voxelInjector->addSample(20000 * sin((i) /((4 / _mouseVoxel.s) * sin((i)/(20 * _mouseVoxel.s / .001)))));  //FM 2 comb filter
            }
            
            AudioInjectionManager::threadInjector(voxelInjector);
        }
    }
    // remember the position for drag detection
    _justEditedVoxel = true;
}

void Application::eyedropperVoxelUnderCursor() {
    VoxelNode* selectedNode = _voxels.getVoxelAt(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
    if (selectedNode && selectedNode->isColored()) {
        QColor selectedColor(selectedNode->getColor()[RED_INDEX], 
                             selectedNode->getColor()[GREEN_INDEX], 
                             selectedNode->getColor()[BLUE_INDEX]);

        if (selectedColor.isValid()) {
            QAction* voxelPaintColorAction = Menu::getInstance()->getActionForOption(MenuOption::VoxelPaintColor);
            voxelPaintColorAction->setData(selectedColor);
            voxelPaintColorAction->setIcon(Swatch::createIcon(selectedColor));
        }
    }
}

void Application::toggleFollowMode() {
    glm::vec3 mouseRayOrigin, mouseRayDirection;
    _viewFrustum.computePickRay(_pieMenu.getX() / (float)_glWidget->width(),
                                _pieMenu.getY() / (float)_glWidget->height(),
                                mouseRayOrigin, mouseRayDirection);
    glm::vec3 eyePositionIgnored;
    QUuid nodeUUIDIgnored;
    Avatar* leadingAvatar = findLookatTargetAvatar(mouseRayOrigin, mouseRayDirection, eyePositionIgnored, nodeUUIDIgnored);

    _myAvatar.follow(leadingAvatar);
}

void Application::resetSensors() {
    _headMouseX = _mouseX = _glWidget->width() / 2;
    _headMouseY = _mouseY = _glWidget->height() / 2;
    
    if (_serialHeadSensor.isActive()) {
        _serialHeadSensor.resetAverages();
    }
    _webcam.reset();
    _faceshift.reset();
    QCursor::setPos(_headMouseX, _headMouseY);
    _myAvatar.reset();
    _myTransmitter.resetLevels();
    _myAvatar.setVelocity(glm::vec3(0,0,0));
    _myAvatar.setThrust(glm::vec3(0,0,0));
    
    _audio.reset();
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

void Application::updateCursor() {
    _glWidget->setCursor(OculusManager::isConnected() && _window->windowState().testFlag(Qt::WindowFullScreen) ?
        Qt::BlankCursor : Qt::ArrowCursor);
}

void Application::attachNewHeadToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        newNode->setLinkedData(new Avatar(newNode));
    }
}

void Application::updateWindowTitle(){
    QString title = "";
    QString username = _profile.getUsername();
    if(!username.isEmpty()){
        title += _profile.getUsername();
        title += " @ ";
    }
    title += _profile.getLastDomain();

    qDebug("Application title set to: %s.\n", title.toStdString().c_str());
    _window->setWindowTitle(title);
}

void Application::domainChanged(QString domain) {
    // update the user's last domain in their Profile (which will propagate to data-server)
    _profile.updateDomain(domain);
    
    updateWindowTitle();

    // reset the environment so that we don't erroneously end up with multiple
    _environment.resetToDefault();
}

void Application::nodeAdded(Node* node) {
    
}

void Application::nodeKilled(Node* node) {
    if (node->getType() == NODE_TYPE_VOXEL_SERVER) {
        QUuid nodeUUID = node->getUUID();
        // see if this is the first we've heard of this node...
        if (_voxelServerJurisdictions.find(nodeUUID) != _voxelServerJurisdictions.end()) {
            unsigned char* rootCode = _voxelServerJurisdictions[nodeUUID].getRootOctalCode();
            VoxelPositionSize rootDetails;
            voxelDetailsForCode(rootCode, rootDetails);

            printf("voxel server going away...... v[%f, %f, %f, %f]\n",
                rootDetails.x, rootDetails.y, rootDetails.z, rootDetails.s);
                
            // Add the jurisditionDetails object to the list of "fade outs"
            VoxelFade fade(VoxelFade::FADE_OUT, NODE_KILLED_RED, NODE_KILLED_GREEN, NODE_KILLED_BLUE);
            fade.voxelDetails = rootDetails;
            const float slightly_smaller = 0.99;
            fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
            _voxelFades.push_back(fade);
        }
    } else if (node->getLinkedData() == _lookatTargetAvatar) {
        _lookatTargetAvatar = NULL;
    }
}

int Application::parseVoxelStats(unsigned char* messageData, ssize_t messageLength, sockaddr senderAddress) {

    // parse the incoming stats data, and stick it into our averaging stats object for now... even though this
    // means mixing in stats from potentially multiple servers.
    int statsMessageLength = _voxelSceneStats.unpackFromMessage(messageData, messageLength);

    // But, also identify the sender, and keep track of the contained jurisdiction root for this server
    Node* voxelServer = NodeList::getInstance()->nodeWithAddress(&senderAddress);
    
    // quick fix for crash... why would voxelServer be NULL?
    if (voxelServer) {
        QUuid nodeUUID = voxelServer->getUUID();

        VoxelPositionSize rootDetails;
        voxelDetailsForCode(_voxelSceneStats.getJurisdictionRoot(), rootDetails);
        
        // see if this is the first we've heard of this node...
        if (_voxelServerJurisdictions.find(nodeUUID) == _voxelServerJurisdictions.end()) {
            printf("stats from new voxel server... v[%f, %f, %f, %f]\n",
                rootDetails.x, rootDetails.y, rootDetails.z, rootDetails.s);

            // Add the jurisditionDetails object to the list of "fade outs"
            VoxelFade fade(VoxelFade::FADE_OUT, NODE_ADDED_RED, NODE_ADDED_GREEN, NODE_ADDED_BLUE);
            fade.voxelDetails = rootDetails;
            const float slightly_smaller = 0.99;
            fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
            _voxelFades.push_back(fade);
        }
        // store jurisdiction details for later use
        // This is bit of fiddling is because JurisdictionMap assumes it is the owner of the values used to construct it
        // but VoxelSceneStats thinks it's just returning a reference to it's contents. So we need to make a copy of the
        // details from the VoxelSceneStats to construct the JurisdictionMap
        JurisdictionMap jurisdictionMap;
        jurisdictionMap.copyContents(_voxelSceneStats.getJurisdictionRoot(), _voxelSceneStats.getJurisdictionEndNodes());
        _voxelServerJurisdictions[nodeUUID] = jurisdictionMap;
    }
    return statsMessageLength;
}

//  Receive packets from other nodes/servers and decide what to do with them!
void* Application::networkReceive(void* args) {
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
        "Application::networkReceive()");

    sockaddr senderAddress;
    ssize_t bytesReceived;
    
    Application* app = Application::getInstance();
    while (!app->_stopNetworkReceiveThread) {
        if (NodeList::getInstance()->getNodeSocket()->receive(&senderAddress, app->_incomingPacket, &bytesReceived)) {
        
            app->_packetCount++;
            app->_bytesCount += bytesReceived;
            
            if (packetVersionMatch(app->_incomingPacket)) {
                // only process this packet if we have a match on the packet version
                switch (app->_incomingPacket[0]) {
                    case PACKET_TYPE_TRANSMITTER_DATA_V2:
                        //  V2 = IOS transmitter app
                        app->_myTransmitter.processIncomingData(app->_incomingPacket, bytesReceived);
                        
                        break;
                    case PACKET_TYPE_MIXED_AUDIO:
                        app->_audio.addReceivedAudioToBuffer(app->_incomingPacket, bytesReceived);
                        break;
                    case PACKET_TYPE_VOXEL_DATA:
                    case PACKET_TYPE_VOXEL_DATA_MONOCHROME:
                    case PACKET_TYPE_Z_COMMAND:
                    case PACKET_TYPE_ERASE_VOXEL:
                    case PACKET_TYPE_VOXEL_STATS:
                    case PACKET_TYPE_ENVIRONMENT_DATA: {
                        PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), 
                            "Application::networkReceive()... _voxelProcessor.queueReceivedPacket()");
                    
                        // add this packet to our list of voxel packets and process them on the voxel processing
                        app->_voxelProcessor.queueReceivedPacket(senderAddress, app->_incomingPacket, bytesReceived);
                        break;
                    }
                    case PACKET_TYPE_BULK_AVATAR_DATA:
                        NodeList::getInstance()->processBulkNodeData(&senderAddress,
                                                                     app->_incomingPacket,
                                                                     bytesReceived);
                        getInstance()->_bandwidthMeter.inputStream(BandwidthMeter::AVATARS).updateValue(bytesReceived);
                        break;
                    case PACKET_TYPE_AVATAR_URLS:
                        processAvatarURLsMessage(app->_incomingPacket, bytesReceived);
                        break;
                    case PACKET_TYPE_AVATAR_FACE_VIDEO:
                        processAvatarFaceVideoMessage(app->_incomingPacket, bytesReceived);
                        break;
                    case PACKET_TYPE_DATA_SERVER_GET:
                    case PACKET_TYPE_DATA_SERVER_PUT:
                    case PACKET_TYPE_DATA_SERVER_SEND:
                    case PACKET_TYPE_DATA_SERVER_CONFIRM:
                        DataServerClient::processMessageFromDataServer(app->_incomingPacket, bytesReceived);
                        break;
                    default:
                        NodeList::getInstance()->processNodeData(&senderAddress, app->_incomingPacket, bytesReceived);
                        break;
                }
            }
        } else if (!app->_enableNetworkThread) {
            break;
        }
    }
    
    if (app->_enableNetworkThread) {
        pthread_exit(0); 
    }
    return NULL; 
}

void Application::packetSentNotification(ssize_t length) {
    _bandwidthMeter.outputStream(BandwidthMeter::VOXELS).updateValue(length); 
}

