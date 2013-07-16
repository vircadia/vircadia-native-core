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

#include <NodeList.h>

#include "BandwidthMeter.h"
#include "ui/BandwidthDialog.h"

#ifndef _WIN32
#include "Audio.h"
#endif

#include "Camera.h"
#include "Environment.h"
#include "HandControl.h"
#include "SerialInterface.h"
#include "Stars.h"
#include "ViewFrustum.h"
#include "VoxelSystem.h"
#include "PacketHeaders.h"
#include "Webcam.h"
#include "renderer/GeometryCache.h"
#include "ui/ChatEntry.h"
#include "ToolsPalette.h"
#include "Swatch.h"
#include "ParticleSystem.h"

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

class Application : public QApplication {
    Q_OBJECT

public:
    static Application* getInstance() { return static_cast<Application*>(QCoreApplication::instance()); }

    Application(int& argc, char** argv, timeval &startup_time);

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
    
    Avatar* getAvatar() { return &_myAvatar; }
    Audio* getAudio() { return &_audio; }
    Camera* getCamera() { return &_myCamera; }
    ViewFrustum* getViewFrustum() { return &_viewFrustum; }
    VoxelSystem* getVoxels() { return &_voxels; }
    QSettings* getSettings() { return _settings; }
    Environment* getEnvironment() { return &_environment; }
    SerialInterface* getSerialHeadSensor() { return &_serialHeadSensor; }
    Webcam* getWebcam() { return &_webcam; }
    BandwidthMeter* getBandwidthMeter() { return &_bandwidthMeter; }
    bool shouldEchoAudio() { return _echoAudioMode->isChecked(); }
    bool shouldLowPassFilter() { return _shouldLowPassFilter->isChecked(); }
    
    bool shouldDynamicallySetJitterBuffer() { return _audioJitterBufferSamples == 0; }
    
    QNetworkAccessManager* getNetworkAccessManager() { return _networkAccessManager; }
    GeometryCache* getGeometryCache() { return &_geometryCache; }
    
private slots:
    
    void timer();
    void idle();
    void terminate();

    void bandwidthDetails();
    void editPreferences();
    void bandwidthDetailsClosed();
    
    void pair();
    
    void setRenderMirrored(bool mirrored);
    void setNoise(bool noise);
    void setFullscreen(bool fullscreen);
    
    void setRenderFirstPerson(bool firstPerson);
    void setRenderThirdPerson(bool thirdPerson);
    void increaseAvatarSize();
    void decreaseAvatarSize();
    
    void renderThrustAtVoxel(const glm::vec3& thrust);
    void renderLineToTouchedVoxel();
    
    void setFrustumOffset(bool frustumOffset);
    void cycleFrustumRenderMode();
    
    void setRenderWarnings(bool renderWarnings);
    void setRenderVoxels(bool renderVoxels);
    void doKillLocalVoxels();
    void doRandomizeVoxelColors();
    void doFalseRandomizeVoxelColors();
    void doFalseRandomizeEveryOtherVoxelColors();
    void doFalseColorizeByDistance();
    void doFalseColorizeOccluded();
    void doFalseColorizeOccludedV2();
    void doFalseColorizeInView();
    void doTrueVoxelColors();
    void doTreeStats();
    void setWantsMonochrome(bool wantsMonochrome);
    void setWantsLowResMoving(bool wantsLowResMoving);
    void disableDeltaSending(bool disableDeltaSending);
    void disableOcclusionCulling(bool disableOcclusionCulling);
    void updateVoxelModeActions();
    void decreaseVoxelSize();
    void increaseVoxelSize();
    void resetSwatchColors();
    void chooseVoxelPaintColor();
    void loadSettings(QSettings* set = NULL);
    void saveSettings(QSettings* set = NULL);
    void importSettings();
    void exportSettings();
    void exportVoxels();
    void importVoxels();
    void importVoxelsToClipboard();
    void cutVoxels();
    void copyVoxels();
    void pasteVoxels();
    void runTests();

    void renderCoverageMap();
    void renderCoverageMapsRecursively(CoverageMap* map);

    void renderCoverageMapV2();
    void renderCoverageMapsV2Recursively(CoverageMapV2* map);

    glm::vec2 getScaledScreenPoint(glm::vec2 projectedPoint);
    void goHome();

private:

    static void controlledBroadcastToNodes(unsigned char* broadcastData, size_t dataBytes, 
                                           const char* nodeTypes, int numNodeTypes);

    static void sendVoxelServerAddScene();
    static bool sendVoxelsOperation(VoxelNode* node, void* extraData);
    static void sendVoxelEditMessage(PACKET_TYPE type, VoxelDetail& detail);
    static void sendAvatarVoxelURLMessage(const QUrl& url);
    static void processAvatarVoxelURLMessage(unsigned char *packetData, size_t dataBytes);
    static void sendPingPackets();
    
    void initMenu();
    void updateFrustumRenderModeAction();
    void initDisplay();
    void init();
    
    void update(float deltaTime);
    bool isLookingAtOtherAvatar(glm::vec3& mouseRayOrigin, glm::vec3& mouseRayDirection, glm::vec3& eyePosition);
    void updateAvatar(float deltaTime);
    void loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum);
    
    void displayOculus(Camera& whichCamera);
    void displaySide(Camera& whichCamera);
    void displayOverlay();
    void displayStats();
    void renderViewFrustum(ViewFrustum& viewFrustum);
   
    void checkBandwidthMeterClick();
     
    void setupPaintingVoxel();
    void shiftPaintingColor();
    void maybeEditVoxelUnderCursor();
    void deleteVoxelUnderCursor();
    void eyedropperVoxelUnderCursor();
    void resetSensors();
    
    void setMenuShortcutsEnabled(bool enabled);
    
    void updateCursor();
    
    QAction* checkedVoxelModeAction() const;
    
    static void attachNewHeadToNode(Node *newNode);
    static void* networkReceive(void* args);
    
    // methodes handling menu settings
    typedef void(*settingsAction)(QSettings*, QAction*);
    static void loadAction(QSettings* set, QAction* action);
    static void saveAction(QSettings* set, QAction* action);
    void scanMenuBar(settingsAction modifySetting, QSettings* set);
    void scanMenu(QMenu* menu, settingsAction modifySetting, QSettings* set);

    QMainWindow* _window;
    QGLWidget* _glWidget;
    
    QAction* _lookingInMirror;       // Are we currently rendering one's own head as if in mirror?
    QAction* _echoAudioMode;         // Are we asking the mixer to echo back our audio?
    QAction* _shouldLowPassFilter;   // Use test lowpass filter
    QAction* _gyroLook;              // Whether to allow the gyro data from head to move your view
    QAction* _renderAvatarBalls;     // Switch between voxels and joints/balls for avatar render
    QAction* _showHeadMouse;         // Whether the have the mouse near edge of screen move your view
    QAction* _transmitterDrives;     // Whether to have Transmitter data move/steer the Avatar
    QAction* _gravityUse;            // Whether gravity is on or not
    QAction* _testPing;              // Whether to display ping or not
    QAction* _renderVoxels;          // Whether to render voxels
    QAction* _renderVoxelTextures;   // Whether to render noise textures on voxels
    QAction* _renderStarsOn;         // Whether to display the stars 
    QAction* _renderAtmosphereOn;    // Whether to display the atmosphere
    QAction* _renderGroundPlaneOn;   // Whether to display the ground plane
    QAction* _renderAvatarsOn;       // Whether to render avatars
    QAction* _renderStatsOn;         // Whether to show onscreen text overlay with stats
    QAction* _renderFrameTimerOn;    // Whether to show onscreen text overlay with stats
    QAction* _renderLookatOn;        // Whether to show lookat vectors from avatar eyes if looking at something
    QAction* _manualFirstPerson;     // Whether to force first-person mode
    QAction* _manualThirdPerson;     // Whether to force third-person mode
    QAction* _logOn;                 // Whether to show on-screen log
    QAction* _oscilloscopeOn;        // Whether to show the oscilloscope
    QAction* _bandwidthDisplayOn;    // Whether to show on-screen bandwidth bars
    QActionGroup* _voxelModeActions; // The group of voxel edit mode actions
    QAction* _addVoxelMode;          // Whether add voxel mode is enabled
    QAction* _deleteVoxelMode;       // Whether delete voxel mode is enabled
    QAction* _colorVoxelMode;        // Whether color voxel mode is enabled
    QAction* _selectVoxelMode;       // Whether select voxel mode is enabled
    QAction* _eyedropperMode;        // Whether voxel color eyedropper mode is enabled
    QAction* _voxelPaintColor;       // The color with which to paint voxels
    QAction* _destructiveAddVoxel;   // when doing voxel editing do we want them to be destructive
    QAction* _frustumOn;             // Whether or not to display the debug view frustum 
    QAction* _viewFrustumFromOffset; // Whether or not to offset the view of the frustum
    QAction* _fullScreenMode;        // whether we are in full screen mode
    QAction* _frustumRenderModeAction;
    QAction* _settingsAutosave;      // Whether settings are saved automatically

    QAction* _renderCoverageMapV2;
    QAction* _renderCoverageMap;

    QAction* _simulateLeapHand;      // When there's no Leap, use this to pretend there is one and feed fake hand data
    QAction* _testRaveGlove;         // Test fancy sparkle-rave-glove mode
    
    BandwidthMeter _bandwidthMeter;
    BandwidthDialog* _bandwidthDialog;

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
    
    VoxelSystem _voxels;
    VoxelTree _clipboardTree; // if I copy/paste

    QByteArray _voxelsFilename;
    bool _wantToKillLocalVoxels;
    
    ViewFrustum _viewFrustum;  // current state of view frustum, perspective, orientation, etc.
    
    enum FrustumDrawMode { FRUSTUM_DRAW_MODE_ALL, FRUSTUM_DRAW_MODE_VECTORS, FRUSTUM_DRAW_MODE_PLANES,
        FRUSTUM_DRAW_MODE_NEAR_PLANE, FRUSTUM_DRAW_MODE_FAR_PLANE, FRUSTUM_DRAW_MODE_KEYHOLE, FRUSTUM_DRAW_MODE_COUNT };
    FrustumDrawMode _frustumDrawingMode;
    
    float _viewFrustumOffsetYaw;      // the following variables control yaw, pitch, roll and distance form regular
    float _viewFrustumOffsetPitch;    // camera to the offset camera
    float _viewFrustumOffsetRoll;
    float _viewFrustumOffsetDistance;
    float _viewFrustumOffsetUp;

    Oscilloscope _audioScope;
    
    Avatar _myAvatar;                  // The rendered avatar of oneself
    
    Transmitter _myTransmitter;        // Gets UDP data from transmitter app used to animate the avatar
    
    Webcam _webcam;                    // The webcam interface
    
    Camera _myCamera;                  // My view onto the world
    Camera _viewFrustumOffsetCamera;   // The camera we use to sometimes show the view frustum from an offset mode
    
    Environment _environment;
    
    int _headMouseX, _headMouseY;
    float _headCameraPitchYawScale;
    
    int _audioJitterBufferSamples;     // Number of extra samples to wait before starting audio playback
    
    float _horizontalFieldOfView;      // In Degrees, doesn't apply to HMD like Oculus
    
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

    
    VoxelDetail _mouseVoxel;      // details of the voxel under the mouse cursor
    float _mouseVoxelScale;       // the scale for adding/removing voxels
    glm::vec3 _lastMouseVoxelPos; // the position of the last mouse voxel edit
    bool _justEditedVoxel;        // set when we've just added/deleted/colored a voxel
    
    bool _paintOn;                // Whether to paint voxels as you fly around
    unsigned char _dominantColor; // The dominant color of the voxel we're painting
    VoxelDetail _paintingVoxel;   // The voxel we're painting if we're painting 
    
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
    
    ParticleSystem _particleSystem;
    
    #ifndef _WIN32
    Audio _audio;
    #endif
    
    bool _enableNetworkThread;
    pthread_t _networkReceiveThread;
    bool _stopNetworkReceiveThread;
    
    unsigned char _incomingPacket[MAX_PACKET_SIZE];
    int _packetCount;
    int _packetsPerSecond;
    int _bytesPerSecond;
    int _bytesCount;
    
    StDev _idleLoopStdev;
    float _idleLoopMeasuredJitter;

    ToolsPalette _palette;
    Swatch _swatch;
};

#endif /* defined(__interface__Application__) */
