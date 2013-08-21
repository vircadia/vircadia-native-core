//
//  Application.h
//  interface
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Application__
#define __interface__Application__

#include <map>
#include <pthread.h> 
#include <time.h>

#include <QApplication>
#include <QAction>
#include <QSettings>
#include <QTouchEvent>
#include <QList>

#include <NetworkPacket.h>
#include <NodeList.h>
#include <PacketHeaders.h>

#ifndef _WIN32
#include "Audio.h"
#endif

#include "BandwidthMeter.h"
#include "Camera.h"
#include "Environment.h"
#include "GLCanvas.h"
#include "PacketHeaders.h"
#include "PieMenu.h"
#include "SerialInterface.h"
#include "Stars.h"
#include "Swatch.h"
#include "ToolsPalette.h"
#include "ViewFrustum.h"
#include "VoxelFade.h"
#include "VoxelEditPacketSender.h"
#include "VoxelPacketProcessor.h"
#include "VoxelSystem.h"
#include "VoxelImporter.h"
#include "Webcam.h"
#include "avatar/Avatar.h"
#include "avatar/MyAvatar.h"
#include "avatar/HandControl.h"
#include "renderer/AmbientOcclusionEffect.h"
#include "renderer/GeometryCache.h"
#include "renderer/GlowEffect.h"
#include "renderer/TextureCache.h"
#include "ui/BandwidthDialog.h"
#include "ui/ChatEntry.h"
#include "ui/VoxelStatsDialog.h"

class QAction;
class QActionGroup;
class QGLWidget;
class QKeyEvent;
class QMainWindow;
class QMouseEvent;
class QNetworkAccessManager;
class QSettings;
class QWheelEvent;

class Node;
class ProgramObject;

static const float NODE_ADDED_RED   = 0.0f;
static const float NODE_ADDED_GREEN = 1.0f;
static const float NODE_ADDED_BLUE  = 0.0f;
static const float NODE_KILLED_RED   = 1.0f;
static const float NODE_KILLED_GREEN = 0.0f;
static const float NODE_KILLED_BLUE  = 0.0f;

class Application : public QApplication, public NodeListHook, public PacketSenderNotify {
    Q_OBJECT

    friend class VoxelPacketProcessor;
    friend class VoxelEditPacketSender;

public:
    static Application* getInstance() { return static_cast<Application*>(QCoreApplication::instance()); }

    Application(int& argc, char** argv, timeval &startup_time);
    ~Application();

    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    void touchBeginEvent(QTouchEvent* event);
    void touchEndEvent(QTouchEvent* event);
    void touchUpdateEvent(QTouchEvent* event);
    
    void wheelEvent(QWheelEvent* event);
    
    const glm::vec3 getMouseVoxelWorldCoordinates(const VoxelDetail _mouseVoxel);
    
    QGLWidget* getGLWidget() { return _glWidget; }
    MyAvatar* getAvatar() { return &_myAvatar; }
    Audio* getAudio() { return &_audio; }
    Camera* getCamera() { return &_myCamera; }
    ViewFrustum* getViewFrustum() { return &_viewFrustum; }
    VoxelSystem* getVoxels() { return &_voxels; }
    Environment* getEnvironment() { return &_environment; }
    SerialInterface* getSerialHeadSensor() { return &_serialHeadSensor; }
    Webcam* getWebcam() { return &_webcam; }
    BandwidthMeter* getBandwidthMeter() { return &_bandwidthMeter; }
    QSettings* getSettings() { return _settings; }
    Swatch*  getSwatch() { return &_swatch; }
    QMainWindow* getWindow() { return _window; }
    VoxelSceneStats* getVoxelSceneStats() { return &_voxelSceneStats; }
    
    QNetworkAccessManager* getNetworkAccessManager() { return _networkAccessManager; }
    GeometryCache* getGeometryCache() { return &_geometryCache; }
    TextureCache* getTextureCache() { return &_textureCache; }
    GlowEffect* getGlowEffect() { return &_glowEffect; }
    
    static void controlledBroadcastToNodes(unsigned char* broadcastData, size_t dataBytes,
                                           const char* nodeTypes, int numNodeTypes);
    
    void setupWorldLight(Camera& whichCamera);

    virtual void nodeAdded(Node* node);
    virtual void nodeKilled(Node* node);
    virtual void packetSentNotification(ssize_t length);

public slots:
    void sendAvatarFaceVideoMessage(int frameCount, const QByteArray& data);
    void exportVoxels();
    void importVoxels();
    void cutVoxels();
    void copyVoxels();
    void togglePasteMode();
    void pasteVoxels();
    
    void setRenderVoxels(bool renderVoxels);
    void doKillLocalVoxels();
    void decreaseVoxelSize();
    void increaseVoxelSize();
    void setListenModeNormal();
    void setListenModePoint();
    void setListenModeSingleSource();
    
private slots:
    
    void timer();
    void idle();
    void terminate();
    
    void setFullscreen(bool fullscreen);
    
    void renderThrustAtVoxel(const glm::vec3& thrust);
    void renderLineToTouchedVoxel();
    
    void renderCoverageMap();
    void renderCoverageMapsRecursively(CoverageMap* map);

    void renderCoverageMapV2();
    void renderCoverageMapsV2Recursively(CoverageMapV2* map);

    glm::vec2 getScaledScreenPoint(glm::vec2 projectedPoint);

    void toggleFollowMode();

private:
    void resetCamerasOnResizeGL(Camera& camera, int width, int height);

    static bool sendVoxelsOperation(VoxelNode* node, void* extraData);
    static void processAvatarVoxelURLMessage(unsigned char* packetData, size_t dataBytes);
    static void processAvatarFaceVideoMessage(unsigned char* packetData, size_t dataBytes);
    static void sendPingPackets();
    
    void initDisplay();
    void init();
    
    void update(float deltaTime);
    Avatar* isLookingAtOtherAvatar(glm::vec3& mouseRayOrigin, glm::vec3& mouseRayDirection,
                                glm::vec3& eyePosition, uint16_t& nodeID);
    bool isLookingAtMyAvatar(Avatar* avatar);
                                
    void renderLookatIndicator(glm::vec3 pointOfInterest, Camera& whichCamera);
    void renderFollowIndicator();
    void updateAvatar(float deltaTime);
    void loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum);
    
    void displayOculus(Camera& whichCamera);
    void displaySide(Camera& whichCamera);
    void displayOverlay();
    void displayStats();
    void renderViewFrustum(ViewFrustum& viewFrustum);
   
    void checkBandwidthMeterClick();
     
    bool maybeEditVoxelUnderCursor();
    void deleteVoxelUnderCursor();
    void eyedropperVoxelUnderCursor();
    void resetSensors();
    void injectVoxelAddedSoundEffect();
            
    void setMenuShortcutsEnabled(bool enabled);
    
    void updateCursor();
    
    static void attachNewHeadToNode(Node *newNode);
    static void* networkReceive(void* args); // network receive thread    

    QMainWindow* _window;
    QGLWidget* _glWidget;
    
    QAction* _followMode;
    
    BandwidthMeter _bandwidthMeter;

    SerialInterface _serialHeadSensor;
    QNetworkAccessManager* _networkAccessManager;
    QSettings* _settings;
    bool _displayLevels;
    
    glm::vec3 _gravity;
    
    // Frame Rate Measurement
    int _frameCount;
    float _fps;
    timeval _applicationStartupTime;
    timeval _timerStart, _timerEnd;
    timeval _lastTimeUpdated;
    bool _justStarted;

    Stars _stars;
    
    VoxelSystem   _voxels;
    VoxelSystem   _clipboard; // if I copy/paste
    ViewFrustum   _clipboardViewFrustum;
    VoxelImporter _voxelImporter;

    QByteArray _voxelsFilename;
    bool _wantToKillLocalVoxels;
    
    ViewFrustum _viewFrustum;  // current state of view frustum, perspective, orientation, etc.

    Oscilloscope _audioScope;
    
    MyAvatar _myAvatar;                  // The rendered avatar of oneself
    
    Transmitter _myTransmitter;        // Gets UDP data from transmitter app used to animate the avatar
    
    Webcam _webcam;                    // The webcam interface
    
    Camera _myCamera;                  // My view onto the world
    Camera _viewFrustumOffsetCamera;   // The camera we use to sometimes show the view frustum from an offset mode
    
    Environment _environment;
    
    int _headMouseX, _headMouseY;
    
    HandControl _handControl;
    
    int _mouseX;
    int _mouseY;
    int _mouseDragStartedX;
    int _mouseDragStartedY;

    float _touchAvgX;
    float _touchAvgY;
    float _lastTouchAvgX;
    float _lastTouchAvgY;
    float _touchDragStartedAvgX;
    float _touchDragStartedAvgY;
    bool _isTouchPressed; //  true if multitouch has been pressed (clear when finished)
    float _yawFromTouch;
    float _pitchFromTouch;
    
    VoxelDetail _mouseVoxelDragging;
    glm::vec3 _voxelThrust;
    bool _mousePressed; //  true if mouse has been pressed (clear when finished)

    VoxelDetail _hoverVoxel;      // Stuff about the voxel I am hovering or clicking
    bool _isHoverVoxel;
    bool _isHoverVoxelSounding;
    nodeColor _hoverVoxelOriginalColor;
    
    VoxelDetail _mouseVoxel;      // details of the voxel to be edited
    float _mouseVoxelScale;       // the scale for adding/removing voxels
    glm::vec3 _lastMouseVoxelPos; // the position of the last mouse voxel edit
    bool _justEditedVoxel;        // set when we've just added/deleted/colored a voxel

    bool _isLookingAtOtherAvatar;
    glm::vec3 _lookatOtherPosition;
    float _lookatIndicatorScale;
    
    bool _perfStatsOn; //  Do we want to display perfStats? 
    
    ChatEntry _chatEntry; // chat entry field 
    bool _chatEntryOn;    // Whether to show the chat entry 
    
    GLuint _oculusTextureID;        // The texture to which we render for Oculus distortion
    ProgramObject* _oculusProgram;  // The GLSL program containing the distortion shader 
    float _oculusDistortionScale;   // Controls the Oculus field of view
    int _textureLocation;
    int _lensCenterLocation;
    int _screenCenterLocation;
    int _scaleLocation;
    int _scaleInLocation;
    int _hmdWarpParamLocation;
    
    GeometryCache _geometryCache;
    TextureCache _textureCache;
    
    GlowEffect _glowEffect;
    AmbientOcclusionEffect _ambientOcclusionEffect;
    
    #ifndef _WIN32
    Audio _audio;
    #endif
    
    bool _enableNetworkThread;
    pthread_t _networkReceiveThread;
    bool _stopNetworkReceiveThread;
    
    bool _enableProcessVoxelsThread;
    VoxelPacketProcessor     _voxelProcessor;
    VoxelEditPacketSender   _voxelEditSender;
    
    unsigned char _incomingPacket[MAX_PACKET_SIZE];
    int _packetCount;
    int _packetsPerSecond;
    int _bytesPerSecond;
    int _bytesCount;
    
    StDev _idleLoopStdev;
    float _idleLoopMeasuredJitter;

    ToolsPalette _palette;
    Swatch _swatch;

    bool _pasteMode;

    PieMenu _pieMenu;
    
    VoxelSceneStats _voxelSceneStats;
    int parseVoxelStats(unsigned char* messageData, ssize_t messageLength, sockaddr senderAddress);
    
    NodeToJurisdictionMap _voxelServerJurisdictions;
    
    std::vector<VoxelFade> _voxelFades;
};

#endif /* defined(__interface__Application__) */
