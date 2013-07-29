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

#include <QActionGroup>
#include <QBoxLayout>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDesktopWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QGLWidget>
#include <QImage>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QNetworkAccessManager>
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
#include <Logstash.h>
#include <OctalCode.h>
#include <PacketHeaders.h>
#include <PairingHandler.h>
#include <PerfStat.h>

#include <VoxelSceneStats.h>

#include "Application.h"
#include "InterfaceConfig.h"
#include "LogDisplay.h"
#include "LeapManager.h"
#include "OculusManager.h"
#include "Util.h"
#include "renderer/ProgramObject.h"
#include "ui/TextRenderer.h"
#include "Swatch.h"
#include "fvupdater.h"

using namespace std;

//  Starfield information
static char STAR_FILE[] = "http://s3-us-west-1.amazonaws.com/highfidelity/stars.txt";
static char STAR_CACHE_FILE[] = "cachedStars.txt";

static const int BANDWIDTH_METER_CLICK_MAX_DRAG_LENGTH = 6; // farther dragged clicks are ignored 

const glm::vec3 START_LOCATION(4.f, 0.f, 5.f);   //  Where one's own node begins in the world
                                                 // (will be overwritten if avatar data file is found)

const int IDLE_SIMULATE_MSECS = 16;              //  How often should call simulate and other stuff
                                                 //  in the idle loop?  (60 FPS is default)
static QTimer* idleTimer = NULL;

const int STARTUP_JITTER_SAMPLES = PACKET_LENGTH_SAMPLES_PER_CHANNEL / 2;
                                                 //  Startup optimistically with small jitter buffer that 
                                                 //  will start playback on the second received audio packet.

// customized canvas that simply forwards requests/events to the singleton application
class GLCanvas : public QGLWidget {
protected:
    
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    
    virtual bool event(QEvent* event);
    
    virtual void wheelEvent(QWheelEvent* event);
};

void GLCanvas::initializeGL() {
    Application::getInstance()->initializeGL();
    setAttribute(Qt::WA_AcceptTouchEvents);
}

void GLCanvas::paintGL() {
    Application::getInstance()->paintGL();
}

void GLCanvas::resizeGL(int width, int height) {
    Application::getInstance()->resizeGL(width, height);
}

void GLCanvas::keyPressEvent(QKeyEvent* event) {
    Application::getInstance()->keyPressEvent(event);
}

void GLCanvas::keyReleaseEvent(QKeyEvent* event) {
    Application::getInstance()->keyReleaseEvent(event);
}

void GLCanvas::mouseMoveEvent(QMouseEvent* event) {
    Application::getInstance()->mouseMoveEvent(event);
}

void GLCanvas::mousePressEvent(QMouseEvent* event) {
    Application::getInstance()->mousePressEvent(event);
}

void GLCanvas::mouseReleaseEvent(QMouseEvent* event) {
    Application::getInstance()->mouseReleaseEvent(event);
}

int updateTime = 0;
bool GLCanvas::event(QEvent* event) {
    switch (event->type()) {
        case QEvent::TouchBegin:
            Application::getInstance()->touchBeginEvent(static_cast<QTouchEvent*>(event));
            event->accept();
            return true;
        case QEvent::TouchEnd:
            Application::getInstance()->touchEndEvent(static_cast<QTouchEvent*>(event));
            return true;
        case QEvent::TouchUpdate:
            Application::getInstance()->touchUpdateEvent(static_cast<QTouchEvent*>(event));
            return true;
        default:
            break;
    }
    return QGLWidget::event(event);
}

void GLCanvas::wheelEvent(QWheelEvent* event) {
    Application::getInstance()->wheelEvent(event);
}

void messageHandler(QtMsgType type, const char* message) {
    fprintf(stdout, "%s", message);    
    LogDisplay::instance.addMessage(message);
}

Application::Application(int& argc, char** argv, timeval &startup_time) :
        QApplication(argc, argv),
        _window(new QMainWindow(desktop())),
        _glWidget(new GLCanvas()),
        _bandwidthDialog(NULL),
        _voxelStatsDialog(NULL),
        _displayLevels(false),
        _frameCount(0),
        _fps(120.0f),
        _justStarted(true),
        _particleSystemInitialized(false),     
        _coolDemoParticleEmitter(-1),
        _wantToKillLocalVoxels(false),
        _frustumDrawingMode(FRUSTUM_DRAW_MODE_ALL),
        _viewFrustumOffsetYaw(-135.0),
        _viewFrustumOffsetPitch(0.0),
        _viewFrustumOffsetRoll(0.0),
        _viewFrustumOffsetDistance(25.0),
        _viewFrustumOffsetUp(0.0),
        _audioScope(256, 200, true),
        _mouseX(0),
        _mouseY(0),
        _touchAvgX(0.0f),
        _touchAvgY(0.0f),
        _isTouchPressed(false),
        _yawFromTouch(0.0f),
        _pitchFromTouch(0.0f),
        _groundPlaneImpact(0.0f),
        _mousePressed(false),
        _mouseVoxelScale(1.0f / 1024.0f),
        _justEditedVoxel(false),
        _isLookingAtOtherAvatar(false),
        _lookatIndicatorScale(1.0f),
        _paintOn(false),
        _dominantColor(0),
        _perfStatsOn(false),
        _chatEntryOn(false),
        _oculusTextureID(0),
        _oculusProgram(0),
        _oculusDistortionScale(1.25),
#ifndef _WIN32
        _audio(&_audioScope, STARTUP_JITTER_SAMPLES),
#endif
        _stopNetworkReceiveThread(false),  
        _packetCount(0),
        _packetsPerSecond(0),
        _bytesPerSecond(0),
        _bytesCount(0),
        _swatch(NULL)
{
    _applicationStartupTime = startup_time;
    _window->setWindowTitle("Interface");
    
    qInstallMsgHandler(messageHandler);
    
    unsigned int listenPort = 0; // bind to an ephemeral port by default
    const char** constArgv = const_cast<const char**>(argv);
    const char* portStr = getCmdOption(argc, constArgv, "--listenPort");
    if (portStr) {
        listenPort = atoi(portStr);
    }
    
    NodeList::createInstance(NODE_TYPE_AGENT, listenPort);
    
    _enableNetworkThread = !cmdOptionExists(argc, constArgv, "--nonblocking");
    if (!_enableNetworkThread) {
        NodeList::getInstance()->getNodeSocket()->setBlocking(false);
    }
    
    // setup QSettings    
#ifdef Q_WS_MAC
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
    
    // check if there is a saved domain server hostname
    // this must be done now instead of with the other setting checks to allow manual override with
    // --domain or --local options
    NodeList::getInstance()->loadData(_settings);
    
    const char* domainIP = getCmdOption(argc, constArgv, "--domain");
    if (domainIP) {
        NodeList::getInstance()->setDomainIP(domainIP);
    }
    
    // Handle Local Domain testing with the --local command line
    if (cmdOptionExists(argc, constArgv, "--local")) {
        qDebug("Local Domain MODE!\n");
        
        NodeList::getInstance()->setDomainIPToLocalhost();
    }
    
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
    
    _window->setCentralWidget(_glWidget);
    
#if defined(Q_WS_MAC) && defined(QT_NO_DEBUG)
    // if this is a release OS X build use fervor to check for an update    
    FvUpdater::sharedUpdater()->SetFeedURL("https://s3-us-west-1.amazonaws.com/highfidelity/appcast.xml");
    FvUpdater::sharedUpdater()->CheckForUpdatesSilent();
#endif

    initMenu();
    
    QRect available = desktop()->availableGeometry();
    _window->resize(available.size());
    _window->setVisible(true);
    _glWidget->setFocusPolicy(Qt::StrongFocus);
    _glWidget->setFocus();
    
    // enable mouse tracking; otherwise, we only get drag events
    _glWidget->setMouseTracking(true);
    
    // initialization continues in initializeGL when OpenGL context is ready
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
        _window->setWindowTitle(title);
        
        const char LOGSTASH_INTERFACE_START_TIME_KEY[] = "interface-start-time";
        
        // ask the Logstash class to record the startup time
        Logstash::stashValue(STAT_TYPE_TIMER, LOGSTASH_INTERFACE_START_TIME_KEY, startupTime);
    }
    
    // update before the first render
    update(0.0f);
}

void Application::paintGL() {
    PerfStat("display");

    glEnable(GL_LINE_SMOOTH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
    if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        _myCamera.setTightness     (100.0f); 
        _myCamera.setTargetPosition(_myAvatar.getUprightHeadPosition());
        _myCamera.setTargetRotation(_myAvatar.getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PIf, 0.0f)));
        
    } else if (OculusManager::isConnected()) {
        _myCamera.setUpShift       (0.0f);
        _myCamera.setDistance      (0.0f);
        _myCamera.setTightness     (0.0f);     //  Camera is directly connected to head without smoothing
        _myCamera.setTargetPosition(_myAvatar.getHeadJointPosition());
        _myCamera.setTargetRotation(_myAvatar.getHead().getOrientation());
    
    } else if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON) {
        _myCamera.setTightness(0.0f);  //  In first person, camera follows head exactly without delay
        _myCamera.setTargetPosition(_myAvatar.getUprightEyeLevelPosition());
        _myCamera.setTargetRotation(_myAvatar.getHead().getCameraOrientation());
        
    } else if (_myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
        _myCamera.setTargetPosition(_myAvatar.getUprightHeadPosition());
        _myCamera.setTargetRotation(_myAvatar.getHead().getCameraOrientation());
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

    if (_viewFrustumFromOffset->isChecked() && _frustumOn->isChecked()) {

        // set the camera to third-person view but offset so we can see the frustum
        _viewFrustumOffsetCamera.setTargetPosition(_myCamera.getTargetPosition());
        _viewFrustumOffsetCamera.setTargetRotation(_myCamera.getTargetRotation() * glm::quat(glm::radians(glm::vec3(
            _viewFrustumOffsetPitch, _viewFrustumOffsetYaw, _viewFrustumOffsetRoll))));
        _viewFrustumOffsetCamera.setUpShift  (_viewFrustumOffsetUp      );
        _viewFrustumOffsetCamera.setDistance (_viewFrustumOffsetDistance);
        _viewFrustumOffsetCamera.initialize(); // force immediate snap to ideal position and orientation
        _viewFrustumOffsetCamera.update(1.f/_fps);
        whichCamera = _viewFrustumOffsetCamera;
    }        

    if (OculusManager::isConnected()) {
        displayOculus(whichCamera);
    } else {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        displaySide(whichCamera);
        glPopMatrix();
        
        displayOverlay();
    }
    
    _frameCount++;
}

void Application::resizeGL(int width, int height) {
    float aspectRatio = ((float)width/(float)height); // based on screen resize

    // reset the camera FOV to our preference...
    _myCamera.setFieldOfView(_horizontalFieldOfView);

    // get the lens details from the current camera
    Camera& camera = _viewFrustumFromOffset->isChecked() ? _viewFrustumOffsetCamera : _myCamera;
    float nearClip = camera.getNearClip();
    float farClip = camera.getFarClip();
    float fov;

    if (OculusManager::isConnected()) {
        // more magic numbers; see Oculus SDK docs, p. 32
        camera.setAspectRatio(aspectRatio *= 0.5);
        camera.setFieldOfView(fov = 2 * atan((0.0468 * _oculusDistortionScale) / 0.041) * (180 / PIf));
        
        // resize the render texture
        if (_oculusTextureID != 0) {
            glBindTexture(GL_TEXTURE_2D, _oculusTextureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    } else {
        camera.setAspectRatio(aspectRatio);
        camera.setFieldOfView(fov = _horizontalFieldOfView);
    }

    // Tell our viewFrustum about this change
    _viewFrustum.setAspectRatio(aspectRatio);

    glViewport(0, 0, width, height); // shouldn't this account for the menu???

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // XXXBHG - If we're in view frustum mode, then we need to do this little bit of hackery so that
    // OpenGL won't clip our frustum rendering lines. This is a debug hack for sure! Basically, this makes
    // the near clip a little bit closer (therefor you see more) and the far clip a little bit farther (also,
    // to see more.)
    if (_frustumOn->isChecked()) {
        nearClip -= 0.01f;
        farClip  += 0.01f;
    }
    
    // On window reshape, we need to tell OpenGL about our new setting
    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;
    loadViewFrustum(camera, _viewFrustum);
    _viewFrustum.computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    glFrustum(left, right, bottom, top, nearVal, farVal);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Application::controlledBroadcastToNodes(unsigned char* broadcastData, size_t dataBytes, 
                                             const char* nodeTypes, int numNodeTypes) {
    Application* self = getInstance();
    for (int i = 0; i < numNodeTypes; ++i) {

        // Intercept data to voxel server when voxels are disabled
        if (nodeTypes[i] == NODE_TYPE_VOXEL_SERVER && ! self->_renderVoxels->isChecked()) {
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

void Application::sendVoxelServerAddScene() {
    char message[100];
    sprintf(message,"%c%s",'Z',"add scene");
    int messageSize = strlen(message) + 1;
    controlledBroadcastToNodes((unsigned char*)message, messageSize, & NODE_TYPE_VOXEL_SERVER, 1);
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
        
        bool shifted = event->modifiers().testFlag(Qt::ShiftModifier);
        switch (event->key()) {
            case Qt::Key_BracketLeft:
                _viewFrustumOffsetYaw -= 0.5;
                break;
                
            case Qt::Key_BracketRight:
                _viewFrustumOffsetYaw += 0.5;
                break;
                
            case Qt::Key_BraceLeft:
                _viewFrustumOffsetPitch -= 0.5;
                break;
                
            case Qt::Key_BraceRight:
                _viewFrustumOffsetPitch += 0.5;
                break;
                
            case Qt::Key_ParenLeft:
                _viewFrustumOffsetRoll -= 0.5;
                break;
                
            case Qt::Key_ParenRight:
                _viewFrustumOffsetRoll += 0.5;
                break;
                
            case Qt::Key_Less:
                _viewFrustumOffsetDistance -= 0.5;
                break;
                
            case Qt::Key_Greater:
                _viewFrustumOffsetDistance += 0.5;
                break;
                
            case Qt::Key_Comma:
                _viewFrustumOffsetUp -= 0.05;
                break;
                
            case Qt::Key_Period:
                _viewFrustumOffsetUp += 0.05;
                break;
                
            case Qt::Key_Ampersand:
                _paintOn = !_paintOn;
                setupPaintingVoxel();
                break;
                
            case Qt::Key_AsciiCircum:
                shiftPaintingColor();
                break;
                
            case Qt::Key_Percent:
                sendVoxelServerAddScene();
                break;
                
            case Qt::Key_Semicolon:
                _audio.ping();
                break;
            case Qt::Key_Apostrophe:
                _audioScope.inputPaused = !_audioScope.inputPaused;
                break; 
            case Qt::Key_L:
                _displayLevels = !_displayLevels;
                break;
                
            case Qt::Key_E:
                if (!_myAvatar.getDriveKeys(UP)) {
                    _myAvatar.jump();
                }
                _myAvatar.setDriveKeys(UP, 1);
                break;
                
            case Qt::Key_C:
                _myAvatar.setDriveKeys(DOWN, 1);
                break;
                
            case Qt::Key_W:
                _myAvatar.setDriveKeys(FWD, 1);
                break;
                
            case Qt::Key_S:
                _myAvatar.setDriveKeys(BACK, 1);
                break;
                
            case Qt::Key_Space:
                resetSensors();
                _audio.reset();
                break;
                
            case Qt::Key_G:
                goHome();
                break;
                
            case Qt::Key_A:
                _myAvatar.setDriveKeys(ROT_LEFT, 1);
                break;
                
            case Qt::Key_D:
                _myAvatar.setDriveKeys(ROT_RIGHT, 1);
                break;
                
            case Qt::Key_Return:
            case Qt::Key_Enter:
                _chatEntryOn = true;
                _myAvatar.setKeyState(NO_KEY_DOWN);
                _myAvatar.setChatMessage(string());
                setMenuShortcutsEnabled(false);
                break;
                
            case Qt::Key_Up:
                _myAvatar.setDriveKeys(shifted ? UP : FWD, 1);
                break;
                
            case Qt::Key_Down:
                _myAvatar.setDriveKeys(shifted ? DOWN : BACK, 1);
                break;
                
            case Qt::Key_Left:
                _myAvatar.setDriveKeys(shifted ? LEFT : ROT_LEFT, 1);
                break;
                
            case Qt::Key_Right:
                _myAvatar.setDriveKeys(shifted ? RIGHT : ROT_RIGHT, 1);
                break;
                
            case Qt::Key_I:
                if (shifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0.002f, 0, 0)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, 0.001, 0));
                }
                resizeGL(_glWidget->width(), _glWidget->height());
                break;
                
            case Qt::Key_K:
                if (shifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(-0.002f, 0, 0)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, -0.001, 0));
                }
                resizeGL(_glWidget->width(), _glWidget->height());
                break;
                
            case Qt::Key_J:
                if (shifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0, 0.002f, 0)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(-0.001, 0, 0));
                }
                resizeGL(_glWidget->width(), _glWidget->height());
                break;
                
            case Qt::Key_M:
                if (shifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0, -0.002f, 0)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0.001, 0, 0));
                }
                resizeGL(_glWidget->width(), _glWidget->height());
                break;
                
            case Qt::Key_U:
                if (shifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0, 0, -0.002f)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, 0, -0.001));
                }
                resizeGL(_glWidget->width(), _glWidget->height());
                break;
                
            case Qt::Key_Y:
                if (shifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0, 0, 0.002f)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, 0, 0.001));
                }
                resizeGL(_glWidget->width(), _glWidget->height());
                break;
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
                if (_selectVoxelMode->isChecked()) {
                    deleteVoxelUnderCursor();
                }
                break;
            case Qt::Key_1:
            case Qt::Key_2:
            case Qt::Key_3:
            case Qt::Key_4:
            case Qt::Key_5:
            case Qt::Key_6:
            case Qt::Key_7:
            case Qt::Key_8:
                _swatch.handleEvent(event->key(), _eyedropperMode->isChecked());
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
    if (activeWindow() == _window) {
        _mouseX = event->x();
        _mouseY = event->y();
        
        // detect drag
        glm::vec3 mouseVoxelPos(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z);
        if (!_justEditedVoxel && mouseVoxelPos != _lastMouseVoxelPos) {
            if (event->buttons().testFlag(Qt::LeftButton)) {
                maybeEditVoxelUnderCursor();
                
            } else if (event->buttons().testFlag(Qt::RightButton) && checkedVoxelModeAction() != 0) {
                deleteVoxelUnderCursor();
            }
        }
    }
}

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
            
        } else if (event->button() == Qt::RightButton && checkedVoxelModeAction() != 0) {
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

void Application::wheelEvent(QWheelEvent* event) {
    if (activeWindow() == _window) {
        if (checkedVoxelModeAction() == 0) {
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
    
    *(uint16_t*)packetPosition = NodeList::getInstance()->getOwnerID();
    packetPosition += sizeof(uint16_t);
    
    *(uint32_t*)packetPosition = frameCount;
    packetPosition += sizeof(uint32_t);
    
    *(uint32_t*)packetPosition = data.size();
    packetPosition += sizeof(uint32_t);
    
    uint32_t* offsetPosition = (uint32_t*)packetPosition;
    packetPosition += sizeof(uint32_t);
    
    int headerSize = packetPosition - packet;
    
    // break the data up into submessages of the maximum size
    *offsetPosition = 0;
    while (*offsetPosition < data.size()) {
        int payloadSize = min(data.size() - (int)*offsetPosition, MAX_PACKET_SIZE - headerSize);
        memcpy(packetPosition, data.constData() + *offsetPosition, payloadSize);
        getInstance()->controlledBroadcastToNodes(packet, headerSize + payloadSize, &NODE_TYPE_AVATAR_MIXER, 1);
        *offsetPosition += payloadSize;
    }
}

//  Every second, check the frame rates and other stuff
void Application::timer() {
    gettimeofday(&_timerEnd, NULL);

    if (_testPing->isChecked()) {
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
    
    timeval check;
    gettimeofday(&check, NULL);
    
    //  Only run simulation code if more than IDLE_SIMULATE_MSECS have passed since last time we ran

    double timeSinceLastUpdate = diffclock(&_lastTimeUpdated, &check);
    if (timeSinceLastUpdate > IDLE_SIMULATE_MSECS) {
        const float BIGGEST_DELTA_TIME_SECS = 0.25f;
        update(glm::clamp((float)timeSinceLastUpdate / 1000.f, 0.f, BIGGEST_DELTA_TIME_SECS));
        _glWidget->updateGL();
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
void Application::terminate() {
    // Close serial port
    // close(serial_fd);
    
    LeapManager::terminate();
    
    if (_settingsAutosave->isChecked()) {
        saveSettings();
        _settings->sync();
    }

    if (_enableNetworkThread) {
        _stopNetworkReceiveThread = true;
        pthread_join(_networkReceiveThread, NULL); 
    }
}

void Application::sendAvatarVoxelURLMessage(const QUrl& url) {
    uint16_t ownerID = NodeList::getInstance()->getOwnerID();
    
    if (ownerID == UNKNOWN_NODE_ID) {
        return; // we don't yet know who we are
    }
    QByteArray message;
    
    char packetHeader[MAX_PACKET_HEADER_BYTES];
    int numBytesPacketHeader = populateTypeAndVersion((unsigned char*) packetHeader, PACKET_TYPE_AVATAR_VOXEL_URL);

    message.append(packetHeader, numBytesPacketHeader);
    message.append((const char*)&ownerID, sizeof(ownerID));
    message.append(url.toEncoded());

    controlledBroadcastToNodes((unsigned char*)message.data(), message.size(), & NODE_TYPE_AVATAR_MIXER, 1);
}

static Avatar* processAvatarMessageHeader(unsigned char*& packetData, size_t& dataBytes) {
    // skip the header
    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    packetData += numBytesPacketHeader;
    dataBytes -= numBytesPacketHeader;
    
    // read the node id
    uint16_t nodeID = *(uint16_t*)packetData;
    packetData += sizeof(nodeID);
    dataBytes -= sizeof(nodeID);
    
    // make sure the node exists
    Node* node = NodeList::getInstance()->nodeWithID(nodeID);
    if (!node || !node->getLinkedData()) {
        return NULL;
    }
    Avatar* avatar = static_cast<Avatar*>(node->getLinkedData());
    return avatar->isInitialized() ? avatar : NULL;
}

void Application::processAvatarVoxelURLMessage(unsigned char* packetData, size_t dataBytes) {
    Avatar* avatar = processAvatarMessageHeader(packetData, dataBytes);
    if (!avatar) {
        return;
    } 
    QUrl url = QUrl::fromEncoded(QByteArray((char*)packetData, dataBytes));
    
    // invoke the set URL function on the simulate/render thread
    QMetaObject::invokeMethod(avatar->getVoxels(), "setVoxelURL", Q_ARG(QUrl, url));
}

void Application::processAvatarFaceVideoMessage(unsigned char* packetData, size_t dataBytes) {
    Avatar* avatar = processAvatarMessageHeader(packetData, dataBytes);
    if (!avatar) {
        return;
    }
    avatar->getHead().getFace().processVideoMessage(packetData, dataBytes);
}

void Application::checkBandwidthMeterClick() {
    // ... to be called upon button release

    if (_bandwidthDisplayOn->isChecked() &&
        glm::compMax(glm::abs(glm::ivec2(_mouseX - _mouseDragStartedX, _mouseY - _mouseDragStartedY))) <= BANDWIDTH_METER_CLICK_MAX_DRAG_LENGTH &&
        _bandwidthMeter.isWithinArea(_mouseX, _mouseY, _glWidget->width(), _glWidget->height())) {

        // The bandwidth meter is visible, the click didn't get dragged too far and
        // we actually hit the bandwidth meter
        bandwidthDetails();
    }
}

void Application::bandwidthDetails() {

    if (! _bandwidthDialog) {
        _bandwidthDialog = new BandwidthDialog(_glWidget, getBandwidthMeter());
        connect(_bandwidthDialog, SIGNAL(closed()), SLOT(bandwidthDetailsClosed()));

        _bandwidthDialog->show();
    } 
    _bandwidthDialog->raise();
}

void Application::bandwidthDetailsClosed() {

    QDialog* dlg = _bandwidthDialog;
    _bandwidthDialog = NULL;
    delete dlg;
}

void Application::voxelStatsDetails() {
    if (!_voxelStatsDialog) {
        _voxelStatsDialog = new VoxelStatsDialog(_glWidget, &_voxelSceneStats);
        connect(_voxelStatsDialog, SIGNAL(closed()), SLOT(voxelStatsDetailsClosed()));
        _voxelStatsDialog->show();
    } 
    _voxelStatsDialog->raise();
}

void Application::voxelStatsDetailsClosed() {
    QDialog* dlg = _voxelStatsDialog;
    _voxelStatsDialog = NULL;
    delete dlg;
}

void Application::editPreferences() {
    QDialog dialog(_glWidget);
    dialog.setWindowTitle("Interface Preferences");
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
    dialog.setLayout(layout);
    
    QFormLayout* form = new QFormLayout();
    layout->addLayout(form, 1);
    
    const int QLINE_MINIMUM_WIDTH = 400;
    
    QLineEdit* domainServerHostname = new QLineEdit(QString(NodeList::getInstance()->getDomainHostname()));
    domainServerHostname->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    form->addRow("Domain server:", domainServerHostname);
    
    QLineEdit* avatarURL = new QLineEdit(_myAvatar.getVoxels()->getVoxelURL().toString());
    avatarURL->setMinimumWidth(QLINE_MINIMUM_WIDTH);
    form->addRow("Avatar URL:", avatarURL);
    
    QSpinBox* horizontalFieldOfView = new QSpinBox();
    horizontalFieldOfView->setMaximum(180);
    horizontalFieldOfView->setMinimum(1);
    horizontalFieldOfView->setValue(_horizontalFieldOfView);
    form->addRow("Horizontal field of view (degrees):", horizontalFieldOfView);
    
    QDoubleSpinBox* headCameraPitchYawScale = new QDoubleSpinBox();
    headCameraPitchYawScale->setValue(_headCameraPitchYawScale);
    form->addRow("Head Camera Pitch/Yaw Scale:", headCameraPitchYawScale);

    QDoubleSpinBox* leanScale = new QDoubleSpinBox();
    leanScale->setValue(_myAvatar.getLeanScale());
    form->addRow("Lean Scale:", leanScale);

    QSpinBox* audioJitterBufferSamples = new QSpinBox();
    audioJitterBufferSamples->setMaximum(10000);
    audioJitterBufferSamples->setMinimum(-10000);
    audioJitterBufferSamples->setValue(_audioJitterBufferSamples);
    form->addRow("Audio Jitter Buffer Samples (0 for automatic):", audioJitterBufferSamples);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialog.connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    dialog.connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    layout->addWidget(buttons);
    
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    
    QByteArray newHostname;
    
    if (domainServerHostname->text().size() > 0) {
        // the user input a new hostname, use that
        newHostname = domainServerHostname->text().toAscii();
    } else {
        // the user left the field blank, use the default hostname
        newHostname = QByteArray(DEFAULT_DOMAIN_HOSTNAME);
    }
   
    // check if the domain server hostname is new 
    if (memcmp(NodeList::getInstance()->getDomainHostname(), newHostname.constData(), newHostname.size()) != 0) {
        
        NodeList::getInstance()->clear();
        
        // kill the local voxels
        _voxels.killLocalVoxels();
        
        // reset the environment to default
        _environment.resetToDefault();
        
        // set the new hostname
        NodeList::getInstance()->setDomainHostname(newHostname.constData());
    }
    
    QUrl url(avatarURL->text());
    _myAvatar.getVoxels()->setVoxelURL(url);
    sendAvatarVoxelURLMessage(url);
    
    _headCameraPitchYawScale = headCameraPitchYawScale->value();
    _myAvatar.setLeanScale(leanScale->value());
    _audioJitterBufferSamples = audioJitterBufferSamples->value();
    if (!shouldDynamicallySetJitterBuffer()) {
        _audio.setJitterBufferSamples(_audioJitterBufferSamples);
    }
    _horizontalFieldOfView = horizontalFieldOfView->value();
    resizeGL(_glWidget->width(), _glWidget->height());
    
}

void Application::pair() {
    PairingHandler::sendPairRequest();
}

void Application::setRenderMirrored(bool mirrored) {
    if (mirrored) {
        _manualFirstPerson->setChecked(false);
        _manualThirdPerson->setChecked(false);
    }
}

void Application::setNoise(bool noise) {
    _myAvatar.setNoise(noise);
}

void Application::setFullscreen(bool fullscreen) {
    _window->setWindowState(fullscreen ? (_window->windowState() | Qt::WindowFullScreen) :
        (_window->windowState() & ~Qt::WindowFullScreen));
    updateCursor();
}

void Application::setRenderFirstPerson(bool firstPerson) {
    if (firstPerson) {
        _lookingInMirror->setChecked(false);
        _manualThirdPerson->setChecked(false);
    }
}

void Application::setRenderThirdPerson(bool thirdPerson) {
    if (thirdPerson) {
        _lookingInMirror->setChecked(false);
        _manualFirstPerson->setChecked(false);
    }
}

void Application::increaseAvatarSize() {
    if (5.0f < _myAvatar.getScale() + 0.05f) {
        return;
    }

    _myAvatar.setScale(_myAvatar.getScale() + 0.05f);
    _myCamera.setScale(_myAvatar.getScale() + 0.05f);
}

void Application::decreaseAvatarSize() {
    if (_myAvatar.getScale() - 0.05f < 0.15f) {
        return;
    }

    _myAvatar.setScale(_myAvatar.getScale() - 0.05f);
    _myCamera.setScale(_myAvatar.getScale() - 0.05f);
}

void Application::setFrustumOffset(bool frustumOffset) {
    // reshape so that OpenGL will get the right lens details for the camera of choice  
    resizeGL(_glWidget->width(), _glWidget->height());
}

void Application::cycleFrustumRenderMode() {
    _frustumDrawingMode = (FrustumDrawMode)((_frustumDrawingMode + 1) % FRUSTUM_DRAW_MODE_COUNT);
    updateFrustumRenderModeAction();
}

void Application::setRenderWarnings(bool renderWarnings) {
    _voxels.setRenderPipelineWarnings(renderWarnings);
}

void Application::setRenderVoxels(bool voxelRender) {
    if (!voxelRender) {
        doKillLocalVoxels();
    }
}

void Application::doKillLocalVoxels() {
    _wantToKillLocalVoxels = true;
}

void Application::doRandomizeVoxelColors() {
    _voxels.randomizeVoxelColors();
}

void Application::doFalseRandomizeVoxelColors() {
    _voxels.falseColorizeRandom();
}

void Application::doFalseRandomizeEveryOtherVoxelColors() {
    _voxels.falseColorizeRandomEveryOther();
}

void Application::doFalseColorizeByDistance() {
    loadViewFrustum(_myCamera, _viewFrustum);
    _voxels.falseColorizeDistanceFromView(&_viewFrustum);
}

void Application::doFalseColorizeInView() {
    loadViewFrustum(_myCamera, _viewFrustum);
    // we probably want to make sure the viewFrustum is initialized first
    _voxels.falseColorizeInView(&_viewFrustum);
}

void Application::doFalseColorizeOccluded() {
    CoverageMap::wantDebugging = true;
    _voxels.falseColorizeOccluded();
}

void Application::doFalseColorizeOccludedV2() {
    _voxels.falseColorizeOccludedV2();
}

void Application::doTrueVoxelColors() {
    _voxels.trueColorize();
}

void Application::doTreeStats() {
    _voxels.collectStatsForTreesAndVBOs();
}

void Application::setWantsLowResMoving(bool wantsLowResMoving) {
    _myAvatar.setWantLowResMoving(wantsLowResMoving);
}

void Application::setWantsMonochrome(bool wantsMonochrome) {
    _myAvatar.setWantColor(!wantsMonochrome);
}

void Application::disableDeltaSending(bool disableDeltaSending) {
    _myAvatar.setWantDelta(!disableDeltaSending);
}

void Application::disableOcclusionCulling(bool disableOcclusionCulling) {
    _myAvatar.setWantOcclusionCulling(!disableOcclusionCulling);
}

void Application::updateVoxelModeActions() {
    // only the sender can be checked
    foreach (QAction* action, _voxelModeActions->actions()) {
        if (action->isChecked() && action != sender()) {
            action->setChecked(false);
        }
    } 
}

void Application::sendVoxelEditMessage(PACKET_TYPE type, VoxelDetail& detail) {
    unsigned char* bufferOut;
    int sizeOut;

    if (createVoxelEditMessage(type, 0, 1, &detail, bufferOut, sizeOut)){
        Application::controlledBroadcastToNodes(bufferOut, sizeOut, & NODE_TYPE_VOXEL_SERVER, 1);
        delete[] bufferOut;
    }
}

const glm::vec3 Application::getMouseVoxelWorldCoordinates(const VoxelDetail _mouseVoxel) {
    return glm::vec3((_mouseVoxel.x + _mouseVoxel.s / 2.f) * TREE_SCALE,
                     (_mouseVoxel.y + _mouseVoxel.s / 2.f) * TREE_SCALE,
                     (_mouseVoxel.z + _mouseVoxel.s / 2.f) * TREE_SCALE);
}

void Application::decreaseVoxelSize() {
    _mouseVoxelScale /= 2;
}

void Application::increaseVoxelSize() {
    _mouseVoxelScale *= 2;
}

void Application::resetSwatchColors() {
    _swatch.reset();
}

static QIcon createSwatchIcon(const QColor& color) {
    QPixmap map(16, 16);
    map.fill(color);
    return QIcon(map);
}

void Application::chooseVoxelPaintColor() {
    QColor selected = QColorDialog::getColor(_voxelPaintColor->data().value<QColor>(), _glWidget, "Voxel Paint Color");
    if (selected.isValid()) {
        _voxelPaintColor->setData(selected);
        _voxelPaintColor->setIcon(createSwatchIcon(selected));
    }
    
    // restore the main window's active state
    _window->activateWindow();
}

const int MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE = 1500;
struct SendVoxelsOperationArgs {
    unsigned char* newBaseOctCode;
    unsigned char messageBuffer[MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE];
    int bufferInUse;
    uint64_t lastSendTime;
    int packetsSent;
    uint64_t bytesSent;
};

bool Application::sendVoxelsOperation(VoxelNode* node, void* extraData) {
    SendVoxelsOperationArgs* args = (SendVoxelsOperationArgs*)extraData;
    if (node->isColored()) {
        unsigned char* nodeOctalCode = node->getOctalCode();
        
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
        
        // if we have room don't have room in the buffer, then send the previously generated message first
        if (args->bufferInUse + codeAndColorLength > MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE) {

            args->packetsSent++;
            args->bytesSent += args->bufferInUse;

            controlledBroadcastToNodes(args->messageBuffer, args->bufferInUse, & NODE_TYPE_VOXEL_SERVER, 1);
            args->bufferInUse = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_SET_VOXEL_DESTRUCTIVE)
                + sizeof(unsigned short int); // reset
                
            uint64_t now = usecTimestampNow();
            // dynamically sleep until we need to fire off the next set of voxels
            const uint64_t CLIENT_TO_SERVER_VOXEL_SEND_INTERVAL_USECS = 1000 * 5; // 1 packet every 10 milliseconds
            uint64_t elapsed = now - args->lastSendTime;
            int usecToSleep =  CLIENT_TO_SERVER_VOXEL_SEND_INTERVAL_USECS - elapsed;
            if (usecToSleep > 0) {
                qDebug("sendVoxelsOperation: packet: %d bytes:%lld elapsed %lld usecs, sleeping for %d usecs!\n",
                       args->packetsSent, (long long int)args->bytesSent, (long long int)elapsed, usecToSleep);
                usleep(usecToSleep);
            } else {
                qDebug("sendVoxelsOperation: packet: %d bytes:%lld elapsed %lld usecs, no need to sleep!\n",
                       args->packetsSent, (long long int)args->bytesSent, (long long int)elapsed);
            }
            args->lastSendTime = now;
        }
        
        // copy this node's code color details into our buffer.
        memcpy(&args->messageBuffer[args->bufferInUse], codeColorBuffer, codeAndColorLength);
        args->bufferInUse += codeAndColorLength;
    }
    return true; // keep going
}

void Application::exportVoxels() {
    QString desktopLocation = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
    QString suggestedName = desktopLocation.append("/voxels.svo");

    QString fileNameString = QFileDialog::getSaveFileName(_glWidget, tr("Export Voxels"), suggestedName, 
                                                          tr("Sparse Voxel Octree Files (*.svo)"));
    QByteArray fileNameAscii = fileNameString.toAscii();
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

const char* IMPORT_FILE_TYPES = "Sparse Voxel Octree Files, Square PNG, Schematic Files (*.svo *.png *.schematic)";
void Application::importVoxelsToClipboard() {
    QString desktopLocation = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
    QString fileNameString = QFileDialog::getOpenFileName(_glWidget, tr("Import Voxels to Clipboard"), desktopLocation,
                                                          tr(IMPORT_FILE_TYPES));

    QByteArray fileNameAscii = fileNameString.toAscii();
    const char* fileName = fileNameAscii.data();
    
    _clipboardTree.eraseAllVoxels();
    if (fileNameString.endsWith(".png", Qt::CaseInsensitive)) {
        QImage pngImage = QImage(fileName);
        if (pngImage.height() != pngImage.width()) {
            qDebug("ERROR: Bad PNG size: height != width.\n");
            return;
        }
        
        const uint32_t* pixels;
        if (pngImage.format() == QImage::Format_ARGB32) {
            pixels = reinterpret_cast<const uint32_t*>(pngImage.constBits());
        } else {
            QImage tmp = pngImage.convertToFormat(QImage::Format_ARGB32);
            pixels = reinterpret_cast<const uint32_t*>(tmp.constBits());
        }
        _clipboardTree.readFromSquareARGB32Pixels(pixels, pngImage.height());
    } else if (fileNameString.endsWith(".svo", Qt::CaseInsensitive)) {
        _clipboardTree.readFromSVOFile(fileName);
    } else if (fileNameString.endsWith(".schematic", Qt::CaseInsensitive)) {
        _clipboardTree.readFromSchematicFile(fileName);
    }

    // restore the main window's active state
    _window->activateWindow();
}

void Application::importVoxels() {
    QString desktopLocation = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
    QString fileNameString = QFileDialog::getOpenFileName(_glWidget, tr("Import Voxels"), desktopLocation,
                                                          tr(IMPORT_FILE_TYPES));

    QByteArray fileNameAscii = fileNameString.toAscii();
    const char* fileName = fileNameAscii.data();
    
    VoxelTree importVoxels;
    if (fileNameString.endsWith(".png", Qt::CaseInsensitive)) {
        QImage pngImage = QImage(fileName);
        if (pngImage.height() != pngImage.width()) {
            qDebug("ERROR: Bad PNG size: height != width.\n");
            return;
        }
        
        const uint32_t* pixels;
        if (pngImage.format() == QImage::Format_ARGB32) {
            pixels = reinterpret_cast<const uint32_t*>(pngImage.constBits());
        } else {
            QImage tmp = pngImage.convertToFormat(QImage::Format_ARGB32);
            pixels = reinterpret_cast<const uint32_t*>(tmp.constBits());
        }
        
        importVoxels.readFromSquareARGB32Pixels(pixels, pngImage.height());        
    } else if (fileNameString.endsWith(".svo", Qt::CaseInsensitive)) {
        importVoxels.readFromSVOFile(fileName);
    } else if (fileNameString.endsWith(".schematic", Qt::CaseInsensitive)) {
        importVoxels.readFromSchematicFile(fileName);
    }
    
    VoxelNode* selectedNode = _voxels.getVoxelAt(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
    
    // Recurse the Import Voxels tree, where everything is root relative, and send all the colored voxels to 
    // the server as an set voxel message, this will also rebase the voxels to the new location
    unsigned char* calculatedOctCode = NULL;
    SendVoxelsOperationArgs args;
    args.lastSendTime = usecTimestampNow();
    args.packetsSent = 0;
    args.bytesSent = 0;
    
    int numBytesPacketHeader = populateTypeAndVersion(args.messageBuffer, PACKET_TYPE_SET_VOXEL_DESTRUCTIVE);
    
    unsigned short int* sequenceAt = (unsigned short int*)&args.messageBuffer[numBytesPacketHeader];
    *sequenceAt = 0;
    args.bufferInUse = numBytesPacketHeader + sizeof(unsigned short int); // set to command + sequence

    // we only need the selected voxel to get the newBaseOctCode, which we can actually calculate from the
    // voxel size/position details.
    if (selectedNode) {
        args.newBaseOctCode = selectedNode->getOctalCode();
    } else {
        args.newBaseOctCode = calculatedOctCode = pointToVoxel(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
    }

    importVoxels.recurseTreeWithOperation(sendVoxelsOperation, &args);

    // If we have voxels left in the packet, then send the packet
    if (args.bufferInUse > (numBytesPacketHeader + sizeof(unsigned short int))) {
        controlledBroadcastToNodes(args.messageBuffer, args.bufferInUse, & NODE_TYPE_VOXEL_SERVER, 1);
    }
    
    if (calculatedOctCode) {
        delete[] calculatedOctCode;
    }

    // restore the main window's active state
    _window->activateWindow();
}

void Application::cutVoxels() {
    copyVoxels();
    deleteVoxelUnderCursor();
}

void Application::copyVoxels() {
    VoxelNode* selectedNode = _voxels.getVoxelAt(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
    if (selectedNode) {
        // clear the clipboard first...
        _clipboardTree.eraseAllVoxels();

        // then copy onto it
        _voxels.copySubTreeIntoNewTree(selectedNode, &_clipboardTree, true);
    }
}

void Application::pasteVoxels() {
    unsigned char* calculatedOctCode = NULL;
    VoxelNode* selectedNode = _voxels.getVoxelAt(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);

    // Recurse the clipboard tree, where everything is root relative, and send all the colored voxels to 
    // the server as an set voxel message, this will also rebase the voxels to the new location
    SendVoxelsOperationArgs args;
    args.lastSendTime = usecTimestampNow();
    args.packetsSent = 0;
    args.bytesSent = 0;
    
    int numBytesPacketHeader = populateTypeAndVersion(args.messageBuffer, PACKET_TYPE_SET_VOXEL_DESTRUCTIVE);
    
    unsigned short int* sequenceAt = (unsigned short int*)&args.messageBuffer[numBytesPacketHeader];
    *sequenceAt = 0;
    args.bufferInUse = numBytesPacketHeader + sizeof(unsigned short int); // set to command + sequence

    // we only need the selected voxel to get the newBaseOctCode, which we can actually calculate from the
    // voxel size/position details. If we don't have an actual selectedNode then use the mouseVoxel to create a 
    // target octalCode for where the user is pointing.
    if (selectedNode) {
        args.newBaseOctCode = selectedNode->getOctalCode();
    } else {
        args.newBaseOctCode = calculatedOctCode = pointToVoxel(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s);
    }

    _clipboardTree.recurseTreeWithOperation(sendVoxelsOperation, &args);
    
    // If we have voxels left in the packet, then send the packet
    if (args.bufferInUse > (numBytesPacketHeader + sizeof(unsigned short int))) {
        controlledBroadcastToNodes(args.messageBuffer, args.bufferInUse, & NODE_TYPE_VOXEL_SERVER, 1);
        qDebug("sending packet: %d\n", ++args.packetsSent);
        args.bytesSent += args.bufferInUse;
        qDebug("total bytes sent: %lld\n", (long long int)args.bytesSent);
    }
    
    if (calculatedOctCode) {
        delete[] calculatedOctCode;
    }
}

void Application::initMenu() {
    QMenuBar* menuBar = new QMenuBar();
    _window->setMenuBar(menuBar);
    
    QMenu* fileMenu = menuBar->addMenu("File");
    fileMenu->addAction("Quit", this, SLOT(quit()), Qt::CTRL | Qt::Key_Q);

    QMenu* editMenu = menuBar->addMenu("Edit");
    editMenu->addAction("Preferences...", this, SLOT(editPreferences()));

    QMenu* pairMenu = menuBar->addMenu("Pair");
    pairMenu->addAction("Pair", this, SLOT(pair()));
    
    QMenu* optionsMenu = menuBar->addMenu("Options");
    (_lookingInMirror = optionsMenu->addAction("Mirror", this, SLOT(setRenderMirrored(bool)), Qt::Key_H))->setCheckable(true);
    (_echoAudioMode = optionsMenu->addAction("Echo Audio"))->setCheckable(true);
    
    optionsMenu->addAction("Noise", this, SLOT(setNoise(bool)), Qt::Key_N)->setCheckable(true);
    (_gyroLook = optionsMenu->addAction("Smooth Gyro Look"))->setCheckable(true);
    _gyroLook->setChecked(true);
    (_showHeadMouse = optionsMenu->addAction("Head Mouse"))->setCheckable(true);
    _showHeadMouse->setChecked(false);
    (_transmitterDrives = optionsMenu->addAction("Transmitter Drive"))->setCheckable(true);
    _transmitterDrives->setChecked(true);
    (_gravityUse = optionsMenu->addAction("Use Gravity"))->setCheckable(true);
    _gravityUse->setChecked(true);
    _gravityUse->setShortcut(Qt::SHIFT | Qt::Key_G);
    (_testPing = optionsMenu->addAction("Test Ping"))->setCheckable(true);
    _testPing->setChecked(true);
    (_fullScreenMode = optionsMenu->addAction("Fullscreen", this, SLOT(setFullscreen(bool)), Qt::Key_F))->setCheckable(true);
    optionsMenu->addAction("Webcam", &_webcam, SLOT(setEnabled(bool)))->setCheckable(true);
    optionsMenu->addAction("Go Home", this, SLOT(goHome()));
    
    QMenu* renderMenu = menuBar->addMenu("Render");
    (_renderVoxels = renderMenu->addAction("Voxels", this, SLOT(setRenderVoxels(bool)), Qt::SHIFT | Qt::Key_V))->setCheckable(true);
    _renderVoxels->setChecked(true);
    (_renderVoxelTextures = renderMenu->addAction("Voxel Textures"))->setCheckable(true);
    (_renderStarsOn = renderMenu->addAction("Stars"))->setCheckable(true);
    _renderStarsOn->setChecked(true);
    _renderStarsOn->setShortcut(Qt::Key_Asterisk);
    (_renderAtmosphereOn = renderMenu->addAction("Atmosphere"))->setCheckable(true);
    _renderAtmosphereOn->setChecked(true);
    _renderAtmosphereOn->setShortcut(Qt::SHIFT | Qt::Key_A);
    (_renderGroundPlaneOn = renderMenu->addAction("Ground Plane"))->setCheckable(true);
    _renderGroundPlaneOn->setChecked(true);
    (_renderAvatarsOn = renderMenu->addAction("Avatars"))->setCheckable(true);
    _renderAvatarsOn->setChecked(true);
    (_renderAvatarBalls = renderMenu->addAction("Avatar as Balls"))->setCheckable(true);
    _renderAvatarBalls->setChecked(false);
    renderMenu->addAction("Cycle Voxel Mode", _myAvatar.getVoxels(), SLOT(cycleMode()));
    renderMenu->addAction("Cycle Face Mode", &_myAvatar.getHead().getFace(), SLOT(cycleRenderMode()));
    (_renderFrameTimerOn = renderMenu->addAction("Show Timer"))->setCheckable(true);
    _renderFrameTimerOn->setChecked(false);
    (_renderLookatOn = renderMenu->addAction("Lookat Vectors"))->setCheckable(true);
    _renderLookatOn->setChecked(false);
    (_renderLookatIndicatorOn = renderMenu->addAction("Lookat Indicator"))->setCheckable(true);
    _renderLookatIndicatorOn->setChecked(true);
    (_renderParticleSystemOn = renderMenu->addAction("Particle System"))->setCheckable(true);
    _renderParticleSystemOn->setChecked(true);
    (_manualFirstPerson = renderMenu->addAction(
        "First Person", this, SLOT(setRenderFirstPerson(bool)), Qt::Key_P))->setCheckable(true);
    (_manualThirdPerson = renderMenu->addAction(
        "Third Person", this, SLOT(setRenderThirdPerson(bool))))->setCheckable(true);
    renderMenu->addAction("Increase Avatar Size", this, SLOT(increaseAvatarSize()), Qt::ALT | Qt::Key_Plus);
    renderMenu->addAction("Decrease Avatar Size", this, SLOT(decreaseAvatarSize()), Qt::ALT | Qt::Key_Minus);

    
    QMenu* toolsMenu = menuBar->addMenu("Tools");
    (_renderStatsOn = toolsMenu->addAction("Stats"))->setCheckable(true);
    _renderStatsOn->setShortcut(Qt::Key_Slash);
    (_logOn = toolsMenu->addAction("Log"))->setCheckable(true);
    _logOn->setChecked(false);
    _logOn->setShortcut(Qt::CTRL | Qt::Key_L);
    (_oscilloscopeOn = toolsMenu->addAction("Audio Oscilloscope"))->setCheckable(true);
    _oscilloscopeOn->setChecked(true);
    (_bandwidthDisplayOn = toolsMenu->addAction("Bandwidth Display"))->setCheckable(true);
    _bandwidthDisplayOn->setChecked(true);
    toolsMenu->addAction("Bandwidth Details", this, SLOT(bandwidthDetails()));
    toolsMenu->addAction("Voxel Stats Details", this, SLOT(voxelStatsDetails()));

 
    QMenu* voxelMenu = menuBar->addMenu("Voxels");
    _voxelModeActions = new QActionGroup(this);
    _voxelModeActions->setExclusive(false); // exclusivity implies one is always checked

    (_addVoxelMode = voxelMenu->addAction(
        "Add Voxel Mode", this, SLOT(updateVoxelModeActions()),    Qt::Key_V))->setCheckable(true);
    _voxelModeActions->addAction(_addVoxelMode);
    (_deleteVoxelMode = voxelMenu->addAction(
        "Delete Voxel Mode", this, SLOT(updateVoxelModeActions()), Qt::Key_R))->setCheckable(true);
    _voxelModeActions->addAction(_deleteVoxelMode);
    (_colorVoxelMode = voxelMenu->addAction(
        "Color Voxel Mode", this, SLOT(updateVoxelModeActions()),  Qt::Key_B))->setCheckable(true);
    _voxelModeActions->addAction(_colorVoxelMode);
    (_selectVoxelMode = voxelMenu->addAction(
        "Select Voxel Mode", this, SLOT(updateVoxelModeActions()), Qt::Key_O))->setCheckable(true);
    _voxelModeActions->addAction(_selectVoxelMode);
    (_eyedropperMode = voxelMenu->addAction(
        "Get Color Mode", this, SLOT(updateVoxelModeActions()),   Qt::Key_G))->setCheckable(true);
    _voxelModeActions->addAction(_eyedropperMode);

    voxelMenu->addAction("Decrease Voxel Size", this, SLOT(decreaseVoxelSize()),       QKeySequence::ZoomOut);
    voxelMenu->addAction("Increase Voxel Size", this, SLOT(increaseVoxelSize()),       QKeySequence::ZoomIn);
    voxelMenu->addAction("Reset Swatch Colors", this, SLOT(resetSwatchColors()));

    _voxelPaintColor = voxelMenu->addAction("Voxel Paint Color", this, 
                                                      SLOT(chooseVoxelPaintColor()),   Qt::META | Qt::Key_C);
    _swatch.setAction(_voxelPaintColor);

    QColor paintColor(128, 128, 128);
    _voxelPaintColor->setData(paintColor);
    _voxelPaintColor->setIcon(createSwatchIcon(paintColor));
    (_destructiveAddVoxel = voxelMenu->addAction("Create Voxel is Destructive"))->setCheckable(true);
    
    voxelMenu->addAction("Export Voxels", this, SLOT(exportVoxels()), Qt::CTRL | Qt::Key_E);
    voxelMenu->addAction("Import Voxels", this, SLOT(importVoxels()), Qt::CTRL | Qt::Key_I);
    voxelMenu->addAction("Import Voxels to Clipboard", this, SLOT(importVoxelsToClipboard()), Qt::SHIFT | Qt::CTRL | Qt::Key_I);
    voxelMenu->addAction("Cut Voxels",    this, SLOT(cutVoxels()),    Qt::CTRL | Qt::Key_X);
    voxelMenu->addAction("Copy Voxels",   this, SLOT(copyVoxels()),   Qt::CTRL | Qt::Key_C);
    voxelMenu->addAction("Paste Voxels",  this, SLOT(pasteVoxels()),  Qt::CTRL | Qt::Key_V);
    
    QMenu* debugMenu = menuBar->addMenu("Debug");

    QMenu* frustumMenu = debugMenu->addMenu("View Frustum Debugging Tools");
    (_frustumOn = frustumMenu->addAction("Display Frustum"))->setCheckable(true); 
    _frustumOn->setShortcut(Qt::SHIFT | Qt::Key_F);
    (_viewFrustumFromOffset = frustumMenu->addAction(
        "Use Offset Camera", this, SLOT(setFrustumOffset(bool)), Qt::SHIFT | Qt::Key_O))->setCheckable(true); 
    _frustumRenderModeAction = frustumMenu->addAction(
        "Render Mode", this, SLOT(cycleFrustumRenderMode()), Qt::SHIFT | Qt::Key_R); 
    updateFrustumRenderModeAction();
    
    debugMenu->addAction("Run Timing Tests", this, SLOT(runTests()));
    debugMenu->addAction("Calculate Tree Stats", this, SLOT(doTreeStats()), Qt::SHIFT | Qt::Key_S);

    QMenu* renderDebugMenu = debugMenu->addMenu("Render Debugging Tools");
    renderDebugMenu->addAction("Show Render Pipeline Warnings", this, SLOT(setRenderWarnings(bool)))->setCheckable(true);
    renderDebugMenu->addAction("Kill Local Voxels", this, SLOT(doKillLocalVoxels()), Qt::CTRL | Qt::Key_K);
    renderDebugMenu->addAction("Randomize Voxel TRUE Colors", this, SLOT(doRandomizeVoxelColors()), Qt::CTRL | Qt::Key_R);
    renderDebugMenu->addAction("FALSE Color Voxels Randomly", this, SLOT(doFalseRandomizeVoxelColors()));
    renderDebugMenu->addAction("FALSE Color Voxel Every Other Randomly", this, SLOT(doFalseRandomizeEveryOtherVoxelColors()));
    renderDebugMenu->addAction("FALSE Color Voxels by Distance", this, SLOT(doFalseColorizeByDistance()));
    renderDebugMenu->addAction("FALSE Color Voxel Out of View", this, SLOT(doFalseColorizeInView()));
    renderDebugMenu->addAction("FALSE Color Occluded Voxels", this, SLOT(doFalseColorizeOccluded()), Qt::CTRL | Qt::Key_O);
    renderDebugMenu->addAction("FALSE Color Occluded V2 Voxels", this, SLOT(doFalseColorizeOccludedV2()), Qt::CTRL | Qt::Key_P);
    renderDebugMenu->addAction("Show TRUE Colors", this, SLOT(doTrueVoxelColors()), Qt::CTRL | Qt::Key_T);

    (_shouldLowPassFilter = debugMenu->addAction("Test: LowPass filter"))->setCheckable(true);

    debugMenu->addAction("Wants Monochrome", this, SLOT(setWantsMonochrome(bool)))->setCheckable(true);
    debugMenu->addAction("Use Lower Resolution While Moving", this, SLOT(setWantsLowResMoving(bool)))->setCheckable(true);
    debugMenu->addAction("Disable Delta Sending", this, SLOT(disableDeltaSending(bool)))->setCheckable(true);
    debugMenu->addAction("Disable Occlusion Culling", this, SLOT(disableOcclusionCulling(bool)), 
                         Qt::SHIFT | Qt::Key_C)->setCheckable(true);

    (_renderCoverageMap = debugMenu->addAction("Render Coverage Map"))->setCheckable(true);
    _renderCoverageMap->setShortcut(Qt::SHIFT | Qt::CTRL | Qt::Key_O);
    (_renderCoverageMapV2 = debugMenu->addAction("Render Coverage Map V2"))->setCheckable(true);
    _renderCoverageMapV2->setShortcut(Qt::SHIFT | Qt::CTRL | Qt::Key_P);

    (_simulateLeapHand = debugMenu->addAction("Simulate Leap Hand"))->setCheckable(true);
    (_testRaveGlove = debugMenu->addAction("Test RaveGlove"))->setCheckable(true);

    QMenu* audioDebugMenu = debugMenu->addMenu("Audio Debugging Tools");
    audioDebugMenu->addAction("Listen Mode Normal", this, SLOT(setListenModeNormal()), Qt::CTRL | Qt::Key_1);
    audioDebugMenu->addAction("Listen Mode Point/Radius", this, SLOT(setListenModePoint()), Qt::CTRL | Qt::Key_2);
    audioDebugMenu->addAction("Listen Mode Single Source", this, SLOT(setListenModeSingleSource()), Qt::CTRL | Qt::Key_3);

    QMenu* settingsMenu = menuBar->addMenu("Settings");
    (_settingsAutosave = settingsMenu->addAction("Autosave"))->setCheckable(true);
    _settingsAutosave->setChecked(true);
    settingsMenu->addAction("Load settings", this, SLOT(loadSettings()));
    settingsMenu->addAction("Save settings", this, SLOT(saveSettings()));
    settingsMenu->addAction("Import settings", this, SLOT(importSettings()));
    settingsMenu->addAction("Export settings", this, SLOT(exportSettings()));
    
    _networkAccessManager = new QNetworkAccessManager(this);
}

void Application::setListenModeNormal() {
    _audio.setListenMode(AudioRingBuffer::NORMAL);
}

void Application::setListenModePoint() {
    _audio.setListenMode(AudioRingBuffer::OMNI_DIRECTIONAL_POINT);
    _audio.setListenRadius(1.0);
}

void Application::setListenModeSingleSource() {
    _audio.setListenMode(AudioRingBuffer::SELECTED_SOURCES);
    _audio.clearListenSources();

    glm::vec3 mouseRayOrigin = _myAvatar.getMouseRayOrigin();
    glm::vec3 mouseRayDirection  = _myAvatar.getMouseRayDirection();
    glm::vec3 eyePositionIgnored;
    uint16_t nodeID;

    if (isLookingAtOtherAvatar(mouseRayOrigin, mouseRayDirection, eyePositionIgnored, nodeID)) {
        _audio.addListenSource(nodeID);
    }
}


void Application::updateFrustumRenderModeAction() {
    switch (_frustumDrawingMode) {
        default:
        case FRUSTUM_DRAW_MODE_ALL: 
            _frustumRenderModeAction->setText("Render Mode - All");
            break;
        case FRUSTUM_DRAW_MODE_VECTORS: 
            _frustumRenderModeAction->setText("Render Mode - Vectors");
            break;
        case FRUSTUM_DRAW_MODE_PLANES:
            _frustumRenderModeAction->setText("Render Mode - Planes");
            break;
        case FRUSTUM_DRAW_MODE_NEAR_PLANE: 
            _frustumRenderModeAction->setText("Render Mode - Near");
            break;
        case FRUSTUM_DRAW_MODE_FAR_PLANE: 
            _frustumRenderModeAction->setText("Render Mode - Far");
            break; 
        case FRUSTUM_DRAW_MODE_KEYHOLE: 
            _frustumRenderModeAction->setText("Render Mode - Keyhole");
            break; 
    }
}

void Application::runTests() {
    runTimingTests();
}

void Application::initDisplay() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel (GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
}

void Application::init() {
    _voxels.init();
    
    _environment.init();
    
    _handControl.setScreenDimensions(_glWidget->width(), _glWidget->height());

    _headMouseX = _mouseX = _glWidget->width() / 2;
    _headMouseY = _mouseY = _glWidget->height() / 2;

    _myAvatar.init();
    _myAvatar.setPosition(START_LOCATION);
    _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
    _myCamera.setModeShiftRate(1.0f);
    _myAvatar.setDisplayingLookatVectors(false);  
    
    QCursor::setPos(_headMouseX, _headMouseY);
    
    OculusManager::connect();
    if (OculusManager::isConnected()) {
        QMetaObject::invokeMethod(_fullScreenMode, "trigger", Qt::QueuedConnection);
    }
    
    LeapManager::initialize();
    
    gettimeofday(&_timerStart, NULL);
    gettimeofday(&_lastTimeUpdated, NULL);

    loadSettings();
    if (!shouldDynamicallySetJitterBuffer()) {
        _audio.setJitterBufferSamples(_audioJitterBufferSamples);
    }
    
    qDebug("Loaded settings.\n");

    sendAvatarVoxelURLMessage(_myAvatar.getVoxels()->getVoxelURL());

    _palette.init(_glWidget->width(), _glWidget->height());
    _palette.addAction(_addVoxelMode, 0, 0);
    _palette.addAction(_deleteVoxelMode, 0, 1);
    _palette.addTool(&_swatch);
    _palette.addAction(_colorVoxelMode, 0, 2);
    _palette.addAction(_eyedropperMode, 0, 3);
    _palette.addAction(_selectVoxelMode, 0, 4);    
}


const float MAX_AVATAR_EDIT_VELOCITY = 1.0f;
const float MAX_VOXEL_EDIT_DISTANCE = 20.0f;
const float HEAD_SPHERE_RADIUS = 0.07;


bool Application::isLookingAtOtherAvatar(glm::vec3& mouseRayOrigin, glm::vec3& mouseRayDirection, 
                                         glm::vec3& eyePosition, uint16_t& nodeID) {
                                         
    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
            Avatar* avatar = (Avatar *) node->getLinkedData();
            glm::vec3 headPosition = avatar->getHead().getPosition();
            if (rayIntersectsSphere(mouseRayOrigin, mouseRayDirection, headPosition, HEAD_SPHERE_RADIUS)) {
                eyePosition = avatar->getHead().getEyeLevelPosition();
                _lookatIndicatorScale = avatar->getScale();
                _lookatOtherPosition = headPosition;
                nodeID = avatar->getOwningNode()->getNodeID();
                return true;
            }
        }
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

void Application::update(float deltaTime) {
    //  Use Transmitter Hand to move hand if connected, else use mouse
    if (_myTransmitter.isConnected()) {
        const float HAND_FORCE_SCALING = 0.01f;
        glm::vec3 estimatedRotation = _myTransmitter.getEstimatedRotation();
        glm::vec3 handForce(-estimatedRotation.z, -estimatedRotation.x, estimatedRotation.y);
        _myAvatar.setMovedHandOffset(handForce *  HAND_FORCE_SCALING);
    } else {
        // update behaviors for avatar hand movement: handControl takes mouse values as input,
        // and gives back 3D values modulated for smooth transitioning between interaction modes.
        _handControl.update(_mouseX, _mouseY);
        _myAvatar.setMovedHandOffset(_handControl.getValues());
    }
    
    // tell my avatar if the mouse is being pressed...
    _myAvatar.setMousePressed(_mousePressed);
    
    // check what's under the mouse and update the mouse voxel
    glm::vec3 mouseRayOrigin, mouseRayDirection;
    _viewFrustum.computePickRay(_mouseX / (float)_glWidget->width(),
        _mouseY / (float)_glWidget->height(), mouseRayOrigin, mouseRayDirection);

    // tell my avatar the posiion and direction of the ray projected ino the world based on the mouse position        
    _myAvatar.setMouseRay(mouseRayOrigin, mouseRayDirection);
    
    // Set where I am looking based on my mouse ray (so that other people can see)
    glm::vec3 eyePosition;
    uint16_t  ignored;
    _isLookingAtOtherAvatar = isLookingAtOtherAvatar(mouseRayOrigin, mouseRayDirection, eyePosition, ignored);
    if (_isLookingAtOtherAvatar) {
        // If the mouse is over another avatar's head...
        glm::vec3 myLookAtFromMouse(eyePosition);
         _myAvatar.getHead().setLookAtPosition(myLookAtFromMouse);
    } else {
        glm::vec3 myLookAtFromMouse(mouseRayOrigin + mouseRayDirection);
        _myAvatar.getHead().setLookAtPosition(myLookAtFromMouse);
    }

    //  If we are dragging on a voxel, add thrust according to the amount the mouse is dragging
    const float VOXEL_GRAB_THRUST = 0.0f;
    if (_mousePressed && (_mouseVoxel.s != 0)) {
        glm::vec2 mouseDrag(_mouseX - _mouseDragStartedX, _mouseY - _mouseDragStartedY);
        glm::quat orientation = _myAvatar.getOrientation();
        glm::vec3 front = orientation * IDENTITY_FRONT;
        glm::vec3 up = orientation * IDENTITY_UP;
        glm::vec3 towardVoxel = getMouseVoxelWorldCoordinates(_mouseVoxelDragging)
                                - _myAvatar.getCameraPosition();
        towardVoxel = front * glm::length(towardVoxel);
        glm::vec3 lateralToVoxel = glm::cross(up, glm::normalize(towardVoxel)) * glm::length(towardVoxel);
        _voxelThrust = glm::vec3(0, 0, 0);
        _voxelThrust += towardVoxel * VOXEL_GRAB_THRUST * deltaTime * mouseDrag.y;
        _voxelThrust += lateralToVoxel * VOXEL_GRAB_THRUST * deltaTime * mouseDrag.x;
        
        //  Add thrust from voxel grabbing to the avatar 
        _myAvatar.addThrust(_voxelThrust);

    }
    
    _mouseVoxel.s = 0.0f;
    if (checkedVoxelModeAction() != 0 &&
        (fabs(_myAvatar.getVelocity().x) +
         fabs(_myAvatar.getVelocity().y) +
         fabs(_myAvatar.getVelocity().z)) / 3 < MAX_AVATAR_EDIT_VELOCITY) {
        float distance;
        BoxFace face;
        if (_voxels.findRayIntersection(mouseRayOrigin, mouseRayDirection, _mouseVoxel, distance, face)) {
            if (distance < MAX_VOXEL_EDIT_DISTANCE) {
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
                    if (_addVoxelMode->isChecked()) {
                        // use the face to determine the side on which to create a neighbor
                        _mouseVoxel.x += faceVector.x * _mouseVoxel.s;
                        _mouseVoxel.y += faceVector.y * _mouseVoxel.s;
                        _mouseVoxel.z += faceVector.z * _mouseVoxel.s;
                    }
                }
            } else {
                _mouseVoxel.s = 0.0f;
            }
        } else if (_addVoxelMode->isChecked() || _selectVoxelMode->isChecked()) {
            // place the voxel a fixed distance away
            float worldMouseVoxelScale = _mouseVoxelScale * TREE_SCALE;
            glm::vec3 pt = mouseRayOrigin + mouseRayDirection * (2.0f + worldMouseVoxelScale * 0.5f);
            _mouseVoxel.x = _mouseVoxelScale * floorf(pt.x / worldMouseVoxelScale);
            _mouseVoxel.y = _mouseVoxelScale * floorf(pt.y / worldMouseVoxelScale);
            _mouseVoxel.z = _mouseVoxelScale * floorf(pt.z / worldMouseVoxelScale);
            _mouseVoxel.s = _mouseVoxelScale;
        }
        
        if (_deleteVoxelMode->isChecked()) {
            // red indicates deletion
            _mouseVoxel.red = 255;
            _mouseVoxel.green = _mouseVoxel.blue = 0;
        } else if (_selectVoxelMode->isChecked()) {
            // yellow indicates deletion
            _mouseVoxel.red = _mouseVoxel.green = 255;
            _mouseVoxel.blue = 0;
        } else { // _addVoxelMode->isChecked() || _colorVoxelMode->isChecked()
            QColor paintColor = _voxelPaintColor->data().value<QColor>();
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
    
    // walking triggers the handControl to stop
    if (_myAvatar.getMode() == AVATAR_MODE_WALKING) {
        _handControl.stop();
    }
       
    //  Update from Touch
    if (_isTouchPressed) {
        float TOUCH_YAW_SCALE = -50.0f;
        float TOUCH_PITCH_SCALE = -50.0f;
        _yawFromTouch += ((_touchAvgX - _lastTouchAvgX) * TOUCH_YAW_SCALE * deltaTime);
        _pitchFromTouch += ((_touchAvgY - _lastTouchAvgY) * TOUCH_PITCH_SCALE * deltaTime);
        
        _lastTouchAvgX = _touchAvgX;
        _lastTouchAvgY = _touchAvgY;
    }
    
    // Leap finger-sensing device
    LeapManager::enableFakeFingers(_simulateLeapHand->isChecked() || _testRaveGlove->isChecked());
    _myAvatar.getHand().setRaveGloveActive(_testRaveGlove->isChecked());
    LeapManager::nextFrame(_myAvatar);
    
     //  Read serial port interface devices
    if (_serialHeadSensor.isActive()) {
        _serialHeadSensor.readData(deltaTime);
    }
    
    //  Update transmitter
    
    //  Sample hardware, update view frustum if needed, and send avatar data to mixer/nodes
    updateAvatar(deltaTime);

    // read incoming packets from network
    if (!_enableNetworkThread) {
        networkReceive(0);
    }
    
    //loop through all the other avatars and simulate them...
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

    //  Simulate myself
    if (_gravityUse->isChecked()) {
        _myAvatar.setGravity(_environment.getGravity(_myAvatar.getPosition()));
    }
    else {
        _myAvatar.setGravity(glm::vec3(0.0f, 0.0f, 0.0f));
    }

    if (_transmitterDrives->isChecked() && _myTransmitter.isConnected()) {
        _myAvatar.simulate(deltaTime, &_myTransmitter);
    } else {
        _myAvatar.simulate(deltaTime, NULL);
    }
    
    _myAvatar.getHand().simulate(deltaTime, true);
    
    if (!OculusManager::isConnected()) {        
        if (_lookingInMirror->isChecked()) {
            if (_myCamera.getMode() != CAMERA_MODE_MIRROR) {
                _myCamera.setMode(CAMERA_MODE_MIRROR);
                _myCamera.setModeShiftRate(100.0f);
            }
        } else if (_manualFirstPerson->isChecked()) {
            if (_myCamera.getMode() != CAMERA_MODE_FIRST_PERSON) {
                _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
                _myCamera.setModeShiftRate(1.0f);
            }
        } else if (_manualThirdPerson->isChecked()) {
            if (_myCamera.getMode() != CAMERA_MODE_THIRD_PERSON) {
                _myCamera.setMode(CAMERA_MODE_THIRD_PERSON);
                _myCamera.setModeShiftRate(1.0f);
            }
        } else {
            const float THIRD_PERSON_SHIFT_VELOCITY = 1000.0f;
            const float TIME_BEFORE_SHIFT_INTO_FIRST_PERSON = 0.75f;
            const float TIME_BEFORE_SHIFT_INTO_THIRD_PERSON = 0.1f;
            
            if ((_myAvatar.getElapsedTimeStopped() > TIME_BEFORE_SHIFT_INTO_FIRST_PERSON)
                    && (_myCamera.getMode() != CAMERA_MODE_FIRST_PERSON)) {
                _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
                _myCamera.setModeShiftRate(1.0f);
            }
            if ((_myAvatar.getSpeed() > THIRD_PERSON_SHIFT_VELOCITY)
                    && (_myAvatar.getElapsedTimeMoving() > TIME_BEFORE_SHIFT_INTO_THIRD_PERSON)
                    && (_myCamera.getMode() != CAMERA_MODE_THIRD_PERSON)) {
                _myCamera.setMode(CAMERA_MODE_THIRD_PERSON);
                _myCamera.setModeShiftRate(1000.0f);
            }
        }
    }
   
    // Update bandwidth dialog, if any
    if (_bandwidthDialog) {
        _bandwidthDialog->update();
    }
    if (_voxelStatsDialog) {
        _voxelStatsDialog->update();
    }

    //  Update audio stats for procedural sounds
    #ifndef _WIN32
    _audio.setLastAcceleration(_myAvatar.getThrust());
    _audio.setLastVelocity(_myAvatar.getVelocity());
    _audio.eventuallyAnalyzePing();
    #endif
    
    if (_renderParticleSystemOn->isChecked()) {
        updateParticleSystem(deltaTime);
    }        
}

void Application::updateAvatar(float deltaTime) {

    // When head is rotated via touch/mouse look, slowly turn body to follow
    const float BODY_FOLLOW_HEAD_RATE = 0.5f;
    // update body yaw by body yaw delta
    _myAvatar.setOrientation(_myAvatar.getOrientation()
                             * glm::quat(glm::vec3(0, _yawFromTouch * deltaTime * BODY_FOLLOW_HEAD_RATE, 0) * deltaTime));
    _yawFromTouch -= _yawFromTouch * deltaTime * BODY_FOLLOW_HEAD_RATE;
    
    // Update my avatar's state from gyros and/or webcam
    _myAvatar.updateFromGyrosAndOrWebcam(_gyroLook->isChecked(),
                                         glm::vec3(_headCameraPitchYawScale,
                                                   _headCameraPitchYawScale,
                                                   _headCameraPitchYawScale),
                                         _yawFromTouch,
                                         _pitchFromTouch);
        
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
        _headMouseX = max(_headMouseX, 0);
        _headMouseX = min(_headMouseX, _glWidget->width());
        _headMouseY = max(_headMouseY, 0);
        _headMouseY = min(_headMouseY, _glWidget->height());

        const float MIDPOINT_OF_SCREEN = 0.5;

        // Set lookAtPosition if an avatar is at the center of the screen
        glm::vec3 screenCenterRayOrigin, screenCenterRayDirection;
        _viewFrustum.computePickRay(MIDPOINT_OF_SCREEN, MIDPOINT_OF_SCREEN, screenCenterRayOrigin, screenCenterRayDirection);

        glm::vec3 eyePosition;
        uint16_t  ignored;
        _isLookingAtOtherAvatar = isLookingAtOtherAvatar(screenCenterRayOrigin, screenCenterRayDirection, eyePosition, ignored);
        if (_isLookingAtOtherAvatar) {
            glm::vec3 myLookAtFromMouse(eyePosition);
            _myAvatar.getHead().setLookAtPosition(myLookAtFromMouse);
        }

    }

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

    // Update Avatar with latest camera and view frustum data...
    // NOTE: we get this from the view frustum, to make it simpler, since the
    // loadViewFrumstum() method will get the correct details from the camera
    // We could optimize this to not actually load the viewFrustum, since we don't
    // actually need to calculate the view frustum planes to send these details 
    // to the server.
    loadViewFrustum(_myCamera, _viewFrustum);
    _myAvatar.setCameraPosition(_viewFrustum.getPosition());
    _myAvatar.setCameraOrientation(_viewFrustum.getOrientation());
    _myAvatar.setCameraFov(_viewFrustum.getFieldOfView());
    _myAvatar.setCameraAspectRatio(_viewFrustum.getAspectRatio());
    _myAvatar.setCameraNearClip(_viewFrustum.getNearClip());
    _myAvatar.setCameraFarClip(_viewFrustum.getFarClip());
    
    NodeList* nodeList = NodeList::getInstance();
    if (nodeList->getOwnerID() != UNKNOWN_NODE_ID) {
        // if I know my ID, send head/hand data to the avatar mixer and voxel server
        unsigned char broadcastString[200];
        unsigned char* endOfBroadcastStringWrite = broadcastString;
        
        endOfBroadcastStringWrite += populateTypeAndVersion(endOfBroadcastStringWrite, PACKET_TYPE_HEAD_DATA);
        
        endOfBroadcastStringWrite += packNodeId(endOfBroadcastStringWrite, nodeList->getOwnerID());
        
        endOfBroadcastStringWrite += _myAvatar.getBroadcastData(endOfBroadcastStringWrite);

        const char nodeTypesOfInterest[] = { NODE_TYPE_VOXEL_SERVER, NODE_TYPE_AVATAR_MIXER }; 
        controlledBroadcastToNodes(broadcastString, endOfBroadcastStringWrite - broadcastString,
                                   nodeTypesOfInterest, sizeof(nodeTypesOfInterest));
        
        // once in a while, send my voxel url
        const float AVATAR_VOXEL_URL_SEND_INTERVAL = 1.0f; // seconds
        if (shouldDo(AVATAR_VOXEL_URL_SEND_INTERVAL, deltaTime)) {
            sendAvatarVoxelURLMessage(_myAvatar.getVoxels()->getVoxelURL());
        }
    }

    // If I'm in paint mode, send a voxel out to VOXEL server nodes.
    if (_paintOn) {
    
        glm::vec3 avatarPos = _myAvatar.getPosition();

        // For some reason, we don't want to flip X and Z here.
        _paintingVoxel.x = avatarPos.x / 10.0;
        _paintingVoxel.y = avatarPos.y / 10.0;
        _paintingVoxel.z = avatarPos.z / 10.0;
        
        if (_paintingVoxel.x >= 0.0 && _paintingVoxel.x <= 1.0 &&
            _paintingVoxel.y >= 0.0 && _paintingVoxel.y <= 1.0 &&
            _paintingVoxel.z >= 0.0 && _paintingVoxel.z <= 1.0) {

            PACKET_TYPE message = (_destructiveAddVoxel->isChecked() ?
                PACKET_TYPE_SET_VOXEL_DESTRUCTIVE : PACKET_TYPE_SET_VOXEL);
            sendVoxelEditMessage(message, _paintingVoxel);
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

    glm::quat rotation = camera.getRotation();

    // Set the viewFrustum up with the correct position and orientation of the camera    
    viewFrustum.setPosition(position);
    viewFrustum.setOrientation(rotation);
    
    // Also make sure it's got the correct lens details from the camera
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

    if (_oculusTextureID == 0) {
        glGenTextures(1, &_oculusTextureID);
        glBindTexture(GL_TEXTURE_2D, _oculusTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _glWidget->width(), _glWidget->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   
        
        _oculusProgram = new ProgramObject();
        _oculusProgram->addShaderFromSourceCode(QGLShader::Fragment, DISTORTION_FRAGMENT_SHADER);
        _oculusProgram->link();
        
        _textureLocation = _oculusProgram->uniformLocation("texture");
        _lensCenterLocation = _oculusProgram->uniformLocation("lensCenter");
        _screenCenterLocation = _oculusProgram->uniformLocation("screenCenter");
        _scaleLocation = _oculusProgram->uniformLocation("scale");
        _scaleInLocation = _oculusProgram->uniformLocation("scaleIn");
        _hmdWarpParamLocation = _oculusProgram->uniformLocation("hmdWarpParam");
        
    } else {
        glBindTexture(GL_TEXTURE_2D, _oculusTextureID);
    }
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, _glWidget->width(), _glWidget->height());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, _glWidget->width(), 0, _glWidget->height());           
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    
    // for reference on setting these values, see SDK file Samples/OculusRoomTiny/RenderTiny_Device.cpp
    
    float scaleFactor = 1.0 / _oculusDistortionScale;
    float aspectRatio = (_glWidget->width() * 0.5) / _glWidget->height();
    
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
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
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    _oculusProgram->release();
    
    glPopMatrix();
}

void Application::displaySide(Camera& whichCamera) {
    // transform by eye offset

    // flip x if in mirror mode (also requires reversing winding order for backface culling)
    if (_lookingInMirror->isChecked()) {
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

    glTranslatef(-whichCamera.getPosition().x, -whichCamera.getPosition().y, -whichCamera.getPosition().z);
    
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
    GLfloat specular_color[] = { 1.0, 1.0, 1.0, 1.0};
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular_color);
    
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular_color);
    glMateriali(GL_FRONT, GL_SHININESS, 96);
    
    if (_renderStarsOn->isChecked()) {
        if (!_stars.getFileLoaded()) {
            _stars.readInput(STAR_FILE, STAR_CACHE_FILE, 0);
        }
        // should be the first rendering pass - w/o depth buffer / lighting

        // compute starfield alpha based on distance from atmosphere
        float alpha = 1.0f;
        if (_renderAtmosphereOn->isChecked()) {
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
    if (_renderAtmosphereOn->isChecked()) {
        _environment.renderAtmospheres(whichCamera);
    }
    
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    
    //  Enable to show line from me to the voxel I am touching
    //renderLineToTouchedVoxel();
    //renderThrustAtVoxel(_voxelThrust);
    
    // draw a red sphere  
    float sphereRadius = 0.25f;
    glColor3f(1,0,0);
    glPushMatrix();
        glutSolidSphere(sphereRadius, 15, 15);
    glPopMatrix();

    //draw a grid ground plane....
    if (_renderGroundPlaneOn->isChecked()) {
        renderGroundPlaneGrid(EDGE_SIZE_GROUND_PLANE, _audio.getCollisionSoundMagnitude());
    } 
    //  Draw voxels
    if (_renderVoxels->isChecked()) {
        _voxels.render(_renderVoxelTextures->isChecked());
    }
    
    // indicate what we'll be adding/removing in mouse mode, if anything
    if (_mouseVoxel.s != 0) {
        glDisable(GL_LIGHTING);
        glPushMatrix();
        if (_addVoxelMode->isChecked()) {
            // use a contrasting color so that we can see what we're doing
            glColor3ub(_mouseVoxel.red + 128, _mouseVoxel.green + 128, _mouseVoxel.blue + 128);
        } else {
            glColor3ub(_mouseVoxel.red, _mouseVoxel.green, _mouseVoxel.blue);
        }
        glScalef(TREE_SCALE, TREE_SCALE, TREE_SCALE);
        glTranslatef(_mouseVoxel.x + _mouseVoxel.s*0.5f,
                     _mouseVoxel.y + _mouseVoxel.s*0.5f,
                     _mouseVoxel.z + _mouseVoxel.s*0.5f);
        glLineWidth(4.0f);
        glutWireCube(_mouseVoxel.s);
        glLineWidth(1.0f);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }
    
    if (_renderAvatarsOn->isChecked()) {
        //  Render avatars of other nodes
        NodeList* nodeList = NodeList::getInstance();
        
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            node->lock();
            
            if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
                Avatar *avatar = (Avatar *)node->getLinkedData();
                if (!avatar->isInitialized()) {
                    avatar->init();
                }
                avatar->render(false, _renderAvatarBalls->isChecked());
                avatar->setDisplayingLookatVectors(_renderLookatOn->isChecked());
            }
            
            node->unlock();
        }
        
        // Render my own Avatar
        if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
            _myAvatar.getHead().setLookAtPosition(_myCamera.getPosition());
        } 
        _myAvatar.render(_lookingInMirror->isChecked(), _renderAvatarBalls->isChecked());

        if (_renderLookatIndicatorOn->isChecked() && _isLookingAtOtherAvatar) {
            renderLookatIndicator(_lookatOtherPosition, whichCamera);
        }
    }

    if (_renderParticleSystemOn->isChecked()) {
        if (_particleSystemInitialized) {
            _particleSystem.render();    
        }
    }
    
    //  Render the world box
    if (!_lookingInMirror->isChecked() && _renderStatsOn->isChecked()) { render_world_box(); }
    
    // brad's frustum for debugging
    if (_frustumOn->isChecked()) renderViewFrustum(_viewFrustum);
    
}

void Application::displayOverlay() {
    //  Render 2D overlay:  I/O level bar graphs and text  
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity(); 
        gluOrtho2D(0, _glWidget->width(), _glWidget->height(), 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
    
        //  Display a single screen-size quad to 
        renderCollisionOverlay(_glWidget->width(), _glWidget->height(), _audio.getCollisionSoundMagnitude());
   
        #ifndef _WIN32
        _audio.render(_glWidget->width(), _glWidget->height());
        if (_oscilloscopeOn->isChecked()) {
            _audioScope.render(20, _glWidget->height() - 200);
        }
        #endif

       //noiseTest(_glWidget->width(), _glWidget->height());
    
    if (_showHeadMouse->isChecked() && !_lookingInMirror->isChecked() && USING_INVENSENSE_MPU9150) {
            //  Display small target box at center or head mouse target that can also be used to measure LOD
            glColor3f(1.0, 1.0, 1.0);
            glDisable(GL_LINE_SMOOTH);
            const int PIXEL_BOX = 20;
            glBegin(GL_LINE_STRIP);
            glVertex2f(_headMouseX - PIXEL_BOX/2, _headMouseY - PIXEL_BOX/2);
            glVertex2f(_headMouseX + PIXEL_BOX/2, _headMouseY - PIXEL_BOX/2);
            glVertex2f(_headMouseX + PIXEL_BOX/2, _headMouseY + PIXEL_BOX/2);
            glVertex2f(_headMouseX - PIXEL_BOX/2, _headMouseY + PIXEL_BOX/2);
            glVertex2f(_headMouseX - PIXEL_BOX/2, _headMouseY - PIXEL_BOX/2);
            glEnd();            
            glEnable(GL_LINE_SMOOTH);
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
    
    if (_renderStatsOn->isChecked()) { displayStats(); }

    // testing rendering coverage map
    if (_renderCoverageMapV2->isChecked()) { renderCoverageMapV2(); }
    if (_renderCoverageMap->isChecked())   { renderCoverageMap(); }
    if (_bandwidthDisplayOn->isChecked()) { _bandwidthMeter.render(_glWidget->width(), _glWidget->height()); }

    if (_logOn->isChecked()) { LogDisplay::instance.render(_glWidget->width(), _glWidget->height()); }

    //  Show chat entry field
    if (_chatEntryOn) {
        _chatEntry.render(_glWidget->width(), _glWidget->height());
    }
    
    //  Show on-screen msec timer
    if (_renderFrameTimerOn->isChecked()) {
        char frameTimer[10];
        uint64_t mSecsNow = floor(usecTimestampNow() / 1000.0 + 0.5);
        sprintf(frameTimer, "%d\n", (int)(mSecsNow % 1000));
        drawtext(_glWidget->width() - 100, _glWidget->height() - 20, 0.30, 0, 1.0, 0, frameTimer, 0, 0, 0);
        drawtext(_glWidget->width() - 102, _glWidget->height() - 22, 0.30, 0, 1.0, 0, frameTimer, 1, 1, 1);
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
    
    if (_paintOn) {
    
        char paintMessage[100];
        sprintf(paintMessage,"Painting (%.3f,%.3f,%.3f/%.3f/%d,%d,%d)",
            _paintingVoxel.x, _paintingVoxel.y, _paintingVoxel.z, _paintingVoxel.s,
            (unsigned int)_paintingVoxel.red, (unsigned int)_paintingVoxel.green, (unsigned int)_paintingVoxel.blue);
        drawtext(_glWidget->width() - 350, 50, 0.10, 0, 1.0, 0, paintMessage, 1, 1, 0);
    }
    
    // render the webcam input frame
    _webcam.renderPreview(_glWidget->width(), _glWidget->height());

    _palette.render(_glWidget->width(), _glWidget->height());

    if (_eyedropperMode->isChecked() && _voxelPaintColor->data().value<QColor>() != _swatch.getColor()) {
        QColor color = _voxelPaintColor->data().value<QColor>();
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

    glPopMatrix();
}

void Application::displayStats() {
    int statsVerticalOffset = 8;

    char stats[200];
    sprintf(stats, "%3.0f FPS, %d Pkts/sec, %3.2f Mbps", 
            _fps, _packetsPerSecond,  (float)_bytesPerSecond * 8.f / 1000000.f);
    drawtext(10, statsVerticalOffset + 15, 0.10f, 0, 1.0, 0, stats);

    if (_testPing->isChecked()) {
        int pingAudio = 0, pingAvatar = 0, pingVoxel = 0;

        NodeList *nodeList = NodeList::getInstance();
        Node *audioMixerNode = nodeList->soloNodeOfType(NODE_TYPE_AUDIO_MIXER);
        Node *avatarMixerNode = nodeList->soloNodeOfType(NODE_TYPE_AVATAR_MIXER);
        Node *voxelServerNode = nodeList->soloNodeOfType(NODE_TYPE_VOXEL_SERVER);

        pingAudio = audioMixerNode ? audioMixerNode->getPingMs() : 0;
        pingAvatar = avatarMixerNode ? avatarMixerNode->getPingMs() : 0;
        pingVoxel = voxelServerNode ? voxelServerNode->getPingMs() : 0;

        char pingStats[200];
        sprintf(pingStats, "Ping audio/avatar/voxel: %d / %d / %d ", pingAudio, pingAvatar, pingVoxel);
        drawtext(10, statsVerticalOffset + 35, 0.10f, 0, 1.0, 0, pingStats);
    }
 
    std::stringstream voxelStats;
    voxelStats.precision(4);
    voxelStats << "Voxels Rendered: " << _voxels.getVoxelsRendered() / 1000.f << "K Updated: " << _voxels.getVoxelsUpdated()/1000.f << "K";
    drawtext(10, statsVerticalOffset + 230, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
    voxelStats.str("");
    char* voxelDetails = _voxelSceneStats.getItemValue(VoxelSceneStats::ITEM_VOXELS);
    voxelStats << "Voxels Sent from Server: " << voxelDetails;
    drawtext(10, statsVerticalOffset + 250, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());

    voxelStats.str("");
    voxelDetails = _voxelSceneStats.getItemValue(VoxelSceneStats::ITEM_ELAPSED);
    voxelStats << "Scene Send Time from Server: " << voxelDetails;
    drawtext(10, statsVerticalOffset + 270, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());

    voxelStats.str("");
    voxelDetails = _voxelSceneStats.getItemValue(VoxelSceneStats::ITEM_ENCODE);
    voxelStats << "Encode Time on Server: " << voxelDetails;
    drawtext(10, statsVerticalOffset + 290, 0.10f, 0, 1.0, 0, (char *)voxelStats.str().c_str());
    
    Node *avatarMixer = NodeList::getInstance()->soloNodeOfType(NODE_TYPE_AVATAR_MIXER);
    char avatarMixerStats[200];
    
    if (avatarMixer) {
        sprintf(avatarMixerStats, "Avatar Mixer: %.f kbps, %.f pps",
                roundf(avatarMixer->getAverageKilobitsPerSecond()),
                roundf(avatarMixer->getAveragePacketsPerSecond()));
    } else {
        sprintf(avatarMixerStats, "No Avatar Mixer");
    }
    
    drawtext(10, statsVerticalOffset + 330, 0.10f, 0, 1.0, 0, avatarMixerStats);
    drawtext(10, statsVerticalOffset + 450, 0.10f, 0, 1.0, 0, (char *)LeapManager::statusString().c_str());
    
    if (_perfStatsOn) {
        // Get the PerfStats group details. We need to allocate and array of char* long enough to hold 1+groups
        char** perfStatLinesArray = new char*[PerfStat::getGroupCount()+1];
        int lines = PerfStat::DumpStats(perfStatLinesArray);
        int atZ = 150; // arbitrary place on screen that looks good
        for (int line=0; line < lines; line++) {
            drawtext(10, statsVerticalOffset + atZ, 0.10f, 0, 1.0, 0, perfStatLinesArray[line]);
            delete perfStatLinesArray[line]; // we're responsible for cleanup
            perfStatLinesArray[line]=NULL;
            atZ+=20; // height of a line
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



/////////////////////////////////////////////////////////////////////////////////////
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

    if (_frustumDrawingMode == FRUSTUM_DRAW_MODE_ALL || _frustumDrawingMode == FRUSTUM_DRAW_MODE_VECTORS) {
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

    if (_frustumDrawingMode == FRUSTUM_DRAW_MODE_ALL || _frustumDrawingMode == FRUSTUM_DRAW_MODE_PLANES
            || _frustumDrawingMode == FRUSTUM_DRAW_MODE_NEAR_PLANE) {
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

    if (_frustumDrawingMode == FRUSTUM_DRAW_MODE_ALL || _frustumDrawingMode == FRUSTUM_DRAW_MODE_PLANES
            || _frustumDrawingMode == FRUSTUM_DRAW_MODE_FAR_PLANE) {
        // viewFrustum.getFar plane - bottom edge 
        glColor3f(0,1,0); // GREEN!!!
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

    if (_frustumDrawingMode == FRUSTUM_DRAW_MODE_ALL || _frustumDrawingMode == FRUSTUM_DRAW_MODE_PLANES) {
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
    }
    glEnd();
    glEnable(GL_LIGHTING);

    if (_frustumDrawingMode == FRUSTUM_DRAW_MODE_ALL || _frustumDrawingMode == FRUSTUM_DRAW_MODE_KEYHOLE) {
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

void Application::setupPaintingVoxel() {
    glm::vec3 avatarPos = _myAvatar.getPosition();

    _paintingVoxel.x = avatarPos.z/-10.0;    // voxel space x is negative z head space
    _paintingVoxel.y = avatarPos.y/-10.0;  // voxel space y is negative y head space
    _paintingVoxel.z = avatarPos.x/-10.0;  // voxel space z is negative x head space
    _paintingVoxel.s = 1.0/256;
    
    shiftPaintingColor();
}

void Application::shiftPaintingColor() {
    // About the color of the paintbrush... first determine the dominant color
    _dominantColor = (_dominantColor + 1) % 3; // 0=red,1=green,2=blue
    _paintingVoxel.red   = (_dominantColor == 0) ? randIntInRange(200, 255) : randIntInRange(40, 100);
    _paintingVoxel.green = (_dominantColor == 1) ? randIntInRange(200, 255) : randIntInRange(40, 100);
    _paintingVoxel.blue  = (_dominantColor == 2) ? randIntInRange(200, 255) : randIntInRange(40, 100);
}


void Application::maybeEditVoxelUnderCursor() {
    if (_addVoxelMode->isChecked() || _colorVoxelMode->isChecked()) {
        if (_mouseVoxel.s != 0) {
            PACKET_TYPE message = (_destructiveAddVoxel->isChecked() ?
                PACKET_TYPE_SET_VOXEL_DESTRUCTIVE : PACKET_TYPE_SET_VOXEL);
            sendVoxelEditMessage(message, _mouseVoxel);
            
            // create the voxel locally so it appears immediately
            _voxels.createVoxel(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z, _mouseVoxel.s,
                                _mouseVoxel.red, _mouseVoxel.green, _mouseVoxel.blue, _destructiveAddVoxel->isChecked());
            
            // remember the position for drag detection
            _justEditedVoxel = true;
            
            AudioInjector* voxelInjector = AudioInjectionManager::injectorWithCapacity(11025);
            voxelInjector->setPosition(glm::vec3(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z));
            //_myAvatar.getPosition()
//            voxelInjector->setBearing(-1 * _myAvatar.getAbsoluteHeadYaw());
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
    } else if (_deleteVoxelMode->isChecked()) {
        deleteVoxelUnderCursor();
    } else if (_eyedropperMode->isChecked()) {
        eyedropperVoxelUnderCursor();
    }
}

void Application::deleteVoxelUnderCursor() {
    if (_mouseVoxel.s != 0) {
        // sending delete to the server is sufficient, server will send new version so we see updates soon enough
        sendVoxelEditMessage(PACKET_TYPE_ERASE_VOXEL, _mouseVoxel);
        AudioInjector* voxelInjector = AudioInjectionManager::injectorWithCapacity(5000);
        voxelInjector->setPosition(glm::vec3(_mouseVoxel.x, _mouseVoxel.y, _mouseVoxel.z));
//        voxelInjector->setBearing(0); //straight down the z axis
        voxelInjector->setVolume (255); //255 is max, and also default value
        
        
        for (int i = 0; i < 5000; i++) {
            voxelInjector->addSample(10000 * sin((i * 2 * PIE) / (500 * sin((i + 1) / 500.0))));  //FM 3 resonant pulse
            //voxelInjector->addSample(20000 * sin((i) /((4 / _mouseVoxel.s) * sin((i)/(20 * _mouseVoxel.s / .001)))));  //FM 2 comb filter
        }
        
        AudioInjectionManager::threadInjector(voxelInjector);
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
            _voxelPaintColor->setData(selectedColor);
            _voxelPaintColor->setIcon(createSwatchIcon(selectedColor));
        }
    }
}

void Application::goHome() {
    qDebug("Going Home!\n");
    _myAvatar.setPosition(START_LOCATION);
}

void Application::resetSensors() {
    _headMouseX = _mouseX = _glWidget->width() / 2;
    _headMouseY = _mouseY = _glWidget->height() / 2;
    
    if (_serialHeadSensor.isActive()) {
        _serialHeadSensor.resetAverages();
    }
    _webcam.reset();
    QCursor::setPos(_headMouseX, _headMouseY);
    _myAvatar.reset();
    _myTransmitter.resetLevels();
    _myAvatar.setVelocity(glm::vec3(0,0,0));
    _myAvatar.setThrust(glm::vec3(0,0,0));
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

// when QActionGroup is set to non-exclusive, it doesn't return anything as checked;
// hence, we must check ourselves
QAction* Application::checkedVoxelModeAction() const {
    foreach (QAction* action, _voxelModeActions->actions()) {
        if (action->isChecked()) {
            return action;
        }
    }
    return 0;
}

void Application::attachNewHeadToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        newNode->setLinkedData(new Avatar(newNode));
    }
}

//  Receive packets from other nodes/servers and decide what to do with them!
void* Application::networkReceive(void* args) {
    sockaddr senderAddress;
    ssize_t bytesReceived;
    
    Application* app = Application::getInstance();
    while (!app->_stopNetworkReceiveThread) {
        // check to see if the UI thread asked us to kill the voxel tree. since we're the only thread allowed to do that
        if (app->_wantToKillLocalVoxels) {
            app->_voxels.killLocalVoxels();
            app->_wantToKillLocalVoxels = false;
        }       
    
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
                    
                        unsigned char* messageData = app->_incomingPacket;
                        ssize_t messageLength = bytesReceived;

                        // note: PACKET_TYPE_VOXEL_STATS can have PACKET_TYPE_VOXEL_DATA or PACKET_TYPE_VOXEL_DATA_MONOCHROME
                        // immediately following them inside the same packet. So, we process the PACKET_TYPE_VOXEL_STATS first
                        // then process any remaining bytes as if it was another packet
                        if (messageData[0] == PACKET_TYPE_VOXEL_STATS) {
                            int statsMessageLength = app->_voxelSceneStats.unpackFromMessage(messageData, messageLength);
                            if (messageLength > statsMessageLength) {
                                messageData += statsMessageLength;
                                messageLength -= statsMessageLength;
                                if (!packetVersionMatch(messageData)) {
                                    break; // bail since piggyback data doesn't match our versioning
                                }
                            } else {
                                break; // bail since no piggyback data
                            }
                        } // fall through to piggyback message
                        
                        if (app->_renderVoxels->isChecked()) {
                            Node* voxelServer = NodeList::getInstance()->soloNodeOfType(NODE_TYPE_VOXEL_SERVER);
                            if (voxelServer && socketMatch(voxelServer->getActiveSocket(), &senderAddress)) {
                                voxelServer->lock();
                                
                                if (messageData[0] == PACKET_TYPE_ENVIRONMENT_DATA) {
                                    app->_environment.parseData(&senderAddress, messageData, messageLength);
                                } else {
                                    app->_voxels.parseData(messageData, messageLength);
                                }
                                
                                voxelServer->unlock();
                            }
                        }
                        break;
                    }
                    case PACKET_TYPE_BULK_AVATAR_DATA:
                        NodeList::getInstance()->processBulkNodeData(&senderAddress,
                                                                     app->_incomingPacket,
                                                                     bytesReceived);
                        getInstance()->_bandwidthMeter.inputStream(BandwidthMeter::AVATARS).updateValue(bytesReceived);
                        break;
                    case PACKET_TYPE_AVATAR_VOXEL_URL:
                        processAvatarVoxelURLMessage(app->_incomingPacket, bytesReceived);
                        break;
                    case PACKET_TYPE_AVATAR_FACE_VIDEO:
                        processAvatarFaceVideoMessage(app->_incomingPacket, bytesReceived);
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

void Application::scanMenuBar(settingsAction modifySetting, QSettings* set) {
  if (!_window->menuBar())  {
        return;
    }

    QList<QMenu*> menus = _window->menuBar()->findChildren<QMenu *>();

    for (QList<QMenu *>::const_iterator it = menus.begin(); menus.end() != it; ++it) {
        scanMenu(*it, modifySetting, set);
    }
}

void Application::scanMenu(QMenu* menu, settingsAction modifySetting, QSettings* set) {
    QList<QAction*> actions = menu->actions();

    set->beginGroup(menu->title());
    for (QList<QAction *>::const_iterator it = actions.begin(); actions.end() != it; ++it) {
        if ((*it)->menu()) {
            scanMenu((*it)->menu(), modifySetting, set);
        }
        if ((*it)->isCheckable()) {
            modifySetting(set, *it);
        }
    }
    set->endGroup();
}

void Application::loadAction(QSettings* set, QAction* action) {
    if (action->isChecked() != set->value(action->text(), action->isChecked()).toBool()) {
        action->trigger();
    }
}

void Application::saveAction(QSettings* set, QAction* action) {
    set->setValue(action->text(),  action->isChecked());
}

void Application::loadSettings(QSettings* settings) {
    if (!settings) { 
        settings = getSettings();
    }

    _headCameraPitchYawScale = loadSetting(settings, "headCameraPitchYawScale", 0.0f);
    _audioJitterBufferSamples = loadSetting(settings, "audioJitterBufferSamples", 0);
    _horizontalFieldOfView = loadSetting(settings, "horizontalFieldOfView", HORIZONTAL_FIELD_OF_VIEW_DEGREES);

    settings->beginGroup("View Frustum Offset Camera");
    // in case settings is corrupt or missing loadSetting() will check for NaN
    _viewFrustumOffsetYaw      = loadSetting(settings, "viewFrustumOffsetYaw"     , 0.0f);
    _viewFrustumOffsetPitch    = loadSetting(settings, "viewFrustumOffsetPitch"   , 0.0f);
    _viewFrustumOffsetRoll     = loadSetting(settings, "viewFrustumOffsetRoll"    , 0.0f);
    _viewFrustumOffsetDistance = loadSetting(settings, "viewFrustumOffsetDistance", 0.0f);
    _viewFrustumOffsetUp       = loadSetting(settings, "viewFrustumOffsetUp"      , 0.0f);
    settings->endGroup();

    scanMenuBar(&Application::loadAction, settings);
    getAvatar()->loadData(settings);
    _swatch.loadData(settings);
}


void Application::saveSettings(QSettings* settings) {
    if (!settings) { 
        settings = getSettings();
    }
    
    settings->setValue("headCameraPitchYawScale", _headCameraPitchYawScale);
    settings->setValue("audioJitterBufferSamples", _audioJitterBufferSamples);
    settings->setValue("horizontalFieldOfView", _horizontalFieldOfView);
    settings->beginGroup("View Frustum Offset Camera");
    settings->setValue("viewFrustumOffsetYaw",      _viewFrustumOffsetYaw);
    settings->setValue("viewFrustumOffsetPitch",    _viewFrustumOffsetPitch);
    settings->setValue("viewFrustumOffsetRoll",     _viewFrustumOffsetRoll);
    settings->setValue("viewFrustumOffsetDistance", _viewFrustumOffsetDistance);
    settings->setValue("viewFrustumOffsetUp",       _viewFrustumOffsetUp);
    settings->endGroup();
    
    scanMenuBar(&Application::saveAction, settings);
    getAvatar()->saveData(settings);
    _swatch.saveData(settings);
    
    // ask the NodeList to save its data
    NodeList::getInstance()->saveData(settings);
}

void Application::importSettings() {
    QString locationDir(QDesktopServices::displayName(QDesktopServices::DesktopLocation));
    QString fileName = QFileDialog::getOpenFileName(_window,
                                                    tr("Open .ini config file"),
                                                    locationDir,
                                                    tr("Text files (*.ini)"));
    if (fileName != "") {
        QSettings tmp(fileName, QSettings::IniFormat);
        loadSettings(&tmp);
    }
}

void Application::exportSettings() {
    QString locationDir(QDesktopServices::displayName(QDesktopServices::DesktopLocation));
    QString fileName = QFileDialog::getSaveFileName(_window,
                                                   tr("Save .ini config file"),
						    locationDir,
                                                   tr("Text files (*.ini)"));
    if (fileName != "") {
        QSettings tmp(fileName, QSettings::IniFormat);
        saveSettings(&tmp);
        tmp.sync();
    }
}



void Application::updateParticleSystem(float deltaTime) {

    if (!_particleSystemInitialized) {
        // create a stable test emitter and spit out a bunch of particles
        _coolDemoParticleEmitter = _particleSystem.addEmitter();
        
        if (_coolDemoParticleEmitter != -1) {
            _particleSystem.setShowingEmitter(_coolDemoParticleEmitter, true);
            glm::vec3 particleEmitterPosition = glm::vec3(5.0f, 1.0f, 5.0f);   
            _particleSystem.setEmitterPosition(_coolDemoParticleEmitter, particleEmitterPosition);
            glm::vec3 velocity(0.0f, 0.1f, 0.0f);
            float lifespan = 100000.0f;
            _particleSystem.emitParticlesNow(_coolDemoParticleEmitter, 1500, velocity, lifespan);   
        }
        
        // signal that the particle system has been initialized 
        _particleSystemInitialized = true;         
    } else {
        // update the particle system
        
        static float t = 0.0f;
        t += deltaTime;
        
        if (_coolDemoParticleEmitter != -1) {
                       
           glm::vec3 tilt = glm::vec3
            (
                30.0f * sinf( t * 0.55f ),
                0.0f,
                30.0f * cosf( t * 0.75f )
            );
         
            _particleSystem.setEmitterRotation(_coolDemoParticleEmitter, glm::quat(glm::radians(tilt)));
            
            ParticleSystem::ParticleAttributes attributes;

            attributes.radius                  = 0.01f;
            attributes.color                   = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f);
            attributes.gravity                 = 0.0f   + 0.05f  * sinf( t * 0.52f );
            attributes.airFriction             = 2.5    + 2.0f   * sinf( t * 0.32f );
            attributes.jitter                  = 0.05f  + 0.05f  * sinf( t * 0.42f );
            attributes.emitterAttraction       = 0.015f + 0.015f * cosf( t * 0.6f  );
            attributes.tornadoForce            = 0.0f   + 0.03f  * sinf( t * 0.7f  );
            attributes.neighborAttraction      = 0.1f   + 0.1f   * cosf( t * 0.8f  );
            attributes.neighborRepulsion       = 0.2f   + 0.2f   * sinf( t * 0.4f  );
            attributes.bounce                  = 1.0f;
            attributes.usingCollisionSphere    = true;
            attributes.collisionSpherePosition = glm::vec3( 5.0f, 0.5f, 5.0f );
            attributes.collisionSphereRadius   = 0.5f;
            
            if (attributes.gravity < 0.0f) {
                attributes.gravity = 0.0f;
            }
            
            _particleSystem.setParticleAttributes(_coolDemoParticleEmitter, attributes);            
        }
        
        _particleSystem.setUpDirection(glm::vec3(0.0f, 1.0f, 0.0f));  
        _particleSystem.simulate(deltaTime); 
    }
}



