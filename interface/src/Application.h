//
//  Application.h
//  interface
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Application__
#define __interface__Application__

#include <pthread.h> 
#include <time.h>

#include <QApplication>

#include <AgentList.h>

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
#include "ui/ChatEntry.h"

class QAction;
class QActionGroup;
class QGLWidget;
class QKeyEvent;
class QMainWindow;
class QMouseEvent;
class QWheelEvent;

class Agent;
class ProgramObject;

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);

    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    void wheelEvent(QWheelEvent* event);
    
    Avatar* getAvatar() { return &_myAvatar; }

private slots:
    
    void timer();
    void idle();
    void terminate();
    
    void pair();
    
    void setHead(bool head);
    void setNoise(bool noise);
    void setFullscreen(bool fullscreen);
    
    void setRenderFirstPerson(bool firstPerson);
    void setOculus(bool oculus);
    
    void setFrustumOffset(bool frustumOffset);
    void cycleFrustumRenderMode();
    
    void setRenderWarnings(bool renderWarnings);
    void doKillLocalVoxels();
    void doRandomizeVoxelColors();
    void doFalseRandomizeVoxelColors();
    void doFalseRandomizeEveryOtherVoxelColors();
    void doFalseColorizeByDistance();
    void doFalseColorizeInView();
    void doTrueVoxelColors();
    void doTreeStats();
    void setWantsMonochrome(bool wantsMonochrome);
    void setWantsResIn(bool wantsResIn);
    void setWantsDelta(bool wantsDelta);
    void updateVoxelModeActions();
    void addVoxelInFrontOfAvatar();
    void decreaseVoxelSize();
    void increaseVoxelSize();
    void chooseVoxelPaintColor();
    
private:
    
    void initMenu();
    void updateFrustumRenderModeAction();
    void initDisplay();
    void init();
    
    void updateAvatar(float deltaTime);
    void loadViewFrustum(ViewFrustum& viewFrustum);
    
    void displayOculus(Camera& whichCamera);
    void displaySide(Camera& whichCamera);
    void displayOverlay();
    void displayStats();
    
    void renderViewFrustum(ViewFrustum& viewFrustum);
    
    void setupPaintingVoxel();
    void shiftPaintingColor();
    void addVoxelUnderCursor();
    void deleteVoxelUnderCursor();
    
    void resetSensors();
    
    void setMenuShortcutsEnabled(bool enabled);
    
    QAction* checkedVoxelModeAction() const;
    
    static void attachNewHeadToAgent(Agent *newAgent);
    static void* networkReceive(void* args);
    
    QMainWindow* _window;
    QGLWidget* _glWidget;
    
    QAction* _lookingInMirror;       // Are we currently rendering one's own head as if in mirror? 
    QAction* _gyroLook;              // Whether to allow the gyro data from head to move your view
    QAction* _mouseLook;             // Whether the have the mouse near edge of screen move your view
    QAction* _renderVoxels;          // Whether to render voxels
    QAction* _renderStarsOn;         // Whether to display the stars 
    QAction* _renderAtmosphereOn;    // Whether to display the atmosphere
    QAction* _renderAvatarsOn;       // Whether to render avatars 
    QAction* _oculusOn;              // Whether to configure the display for the Oculus Rift 
    QAction* _renderStatsOn;         // Whether to show onscreen text overlay with stats
    QAction* _renderFrameTimerOn;    // Whether to show onscreen text overlay with stats
    QAction* _logOn;                 // Whether to show on-screen log
    QActionGroup* _voxelModeActions; // The group of voxel edit mode actions
    QAction* _addVoxelMode;          // Whether add voxel mode is enabled
    QAction* _deleteVoxelMode;       // Whether delete voxel mode is enabled
    QAction* _colorVoxelMode;        // Whether color voxel mode is enabled
    QAction* _voxelPaintColor;       // The color with which to paint voxels
    QAction* _destructiveAddVoxel;   // when doing voxel editing do we want them to be destructive
    QAction* _frustumOn;             // Whether or not to display the debug view frustum 
    QAction* _viewFrustumFromOffset; // Whether or not to offset the view of the frustum
    QAction* _cameraFrustum;         // which frustum to look at 
    QAction* _frustumRenderModeAction;
    
    SerialInterface _serialPort;
    bool _displayLevels;
    
    glm::vec3 _gravity;
    
    // Frame Rate Measurement
    int _frameCount;
    float _fps;
    timeval _applicationStartupTime;
    timeval _timerStart, _timerEnd;
    timeval _lastTimeIdle;
    bool _justStarted;
    
    Stars _stars;
    
    VoxelSystem _voxels;
    QByteArray _voxelsFilename;
    bool _wantToKillLocalVoxels;
    
    ViewFrustum _viewFrustum;  // current state of view frustum, perspective, orientation, etc.
    
    enum FrustumDrawMode { FRUSTUM_DRAW_MODE_ALL, FRUSTUM_DRAW_MODE_VECTORS, FRUSTUM_DRAW_MODE_PLANES,
        FRUSTUM_DRAW_MODE_NEAR_PLANE, FRUSTUM_DRAW_MODE_FAR_PLANE, FRUSTUM_DRAW_MODE_COUNT };
    FrustumDrawMode _frustumDrawingMode;
    
    float _viewFrustumOffsetYaw;      // the following variables control yaw, pitch, roll and distance form regular
    float _viewFrustumOffsetPitch;    // camera to the offset camera
    float _viewFrustumOffsetRoll;
    float _viewFrustumOffsetDistance;
    float _viewFrustumOffsetUp;

    Oscilloscope _audioScope;
    
    Avatar _myAvatar;                  // The rendered avatar of oneself
    Camera _myCamera;                  // My view onto the world
    Camera _viewFrustumOffsetCamera;   // The camera we use to sometimes show the view frustum from an offset mode
    
    Environment _environment;
    
    int _headMouseX, _headMouseY;
    
    HandControl _handControl;
    
    int _mouseX;
    int _mouseY;
    bool _mousePressed; //  true if mouse has been pressed (clear when finished)
    
    VoxelDetail _mouseVoxel;      // details of the voxel under the mouse cursor
    float _mouseVoxelScale;       // the scale for adding/removing voxels
    glm::vec3 _lastMouseVoxelPos; // the position of the last mouse voxel edit
    
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
};

#endif /* defined(__interface__Application__) */
