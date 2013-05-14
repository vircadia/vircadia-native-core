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
class QGLWidget;
class QKeyEvent;
class QMainWindow;
class QMouseEvent;

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
    void addVoxelInFrontOfAvatar();
    void addVoxelUnderCursor();
    void deleteVoxelUnderCursor();
    
    void resetSensors();
    
    void setMenuShortcutsEnabled(bool enabled);
    
    static void attachNewHeadToAgent(Agent *newAgent);
    #ifndef _WIN32
    static void audioMixerUpdate(in_addr_t newMixerAddress, in_port_t newMixerPort);
    #endif
    static void* networkReceive(void* args);
    
    QMainWindow* _window;
    QGLWidget* _glWidget;
    
    QAction* _lookingInMirror;
    QAction* _gyroLook;
    QAction* _renderVoxels;
    QAction* _renderStarsOn;
    QAction* _renderAtmosphereOn;
    QAction* _renderAvatarsOn;
    QAction* _oculusOn;
    QAction* _renderStatsOn;
    QAction* _logOn;
    QAction* _frustumOn;
    QAction* _viewFrustumFromOffset;
    QAction* _cameraFrustum;
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

    float _mouseViewShiftYaw;
    float _mouseViewShiftPitch;

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
    
    // The current mode for mouse interaction
    enum MouseMode { NO_EDIT_MODE, ADD_VOXEL_MODE, DELETE_VOXEL_MODE, COLOR_VOXEL_MODE };
    MouseMode _mouseMode;
    VoxelDetail _mouseVoxel; // details of the voxel under the mouse cursor
    float _mouseVoxelScale; // the scale for adding/removing voxels
    
    bool _paintOn;
    unsigned char _dominantColor;
    VoxelDetail _paintingVoxel;
    
    bool _perfStatsOn;
    
    ChatEntry _chatEntry;
    bool _chatEntryOn;
    
    GLuint _oculusTextureID;
    ProgramObject* _oculusProgram;
    float _oculusDistortionScale;
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
