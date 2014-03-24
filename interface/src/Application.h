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
#include <time.h>

#include <QApplication>
#include <QAction>
#include <QImage>
#include <QSettings>
#include <QTouchEvent>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QPointer>

#include <NetworkPacket.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <ParticleCollisionSystem.h>
#include <ParticleEditPacketSender.h>
#include <ScriptEngine.h>
#include <OctreeQuery.h>
#include <ViewFrustum.h>
#include <VoxelEditPacketSender.h>

#include "Audio.h"
#include "BuckyBalls.h"
#include "Camera.h"
#include "DatagramProcessor.h"
#include "Environment.h"
#include "FileLogger.h"
#include "GLCanvas.h"
#include "Menu.h"
#include "MetavoxelSystem.h"
#include "PacketHeaders.h"
#include "ParticleTreeRenderer.h"
#include "Stars.h"
#include "avatar/Avatar.h"
#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"
#include "devices/Faceshift.h"
#include "devices/SixenseManager.h"
#include "devices/Visage.h"
#include "renderer/AmbientOcclusionEffect.h"
#include "renderer/GeometryCache.h"
#include "renderer/GlowEffect.h"
#include "renderer/PointShader.h"
#include "renderer/TextureCache.h"
#include "renderer/VoxelShader.h"
#include "scripting/ControllerScriptingInterface.h"
#include "ui/BandwidthDialog.h"
#include "ui/BandwidthMeter.h"
#include "ui/OctreeStatsDialog.h"
#include "ui/RearMirrorTools.h"
#include "ui/LodToolsDialog.h"
#include "ui/LogDialog.h"
#include "ui/UpdateDialog.h"
#include "ui/overlays/Overlays.h"
#include "voxels/VoxelFade.h"
#include "voxels/VoxelHideShowThread.h"
#include "voxels/VoxelImporter.h"
#include "voxels/VoxelPacketProcessor.h"
#include "voxels/VoxelSystem.h"


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

static const QString SNAPSHOT_EXTENSION  = ".jpg";

static const float BILLBOARD_FIELD_OF_VIEW = 30.0f; // degrees
static const float BILLBOARD_DISTANCE = 5.0f;       // meters

class Application : public QApplication {
    Q_OBJECT

    friend class VoxelPacketProcessor;
    friend class VoxelEditPacketSender;
    friend class DatagramProcessor;

public:
    static Application* getInstance() { return static_cast<Application*>(QCoreApplication::instance()); }
    static QString& resourcesPath();

    Application(int& argc, char** argv, timeval &startup_time);
    ~Application();

    void restoreSizeAndPosition();
    void loadScript(const QString& fileNameString);    
    void loadScripts();
    void storeSizeAndPosition();
    void saveScripts();
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void focusOutEvent(QFocusEvent* event);

    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    void touchBeginEvent(QTouchEvent* event);
    void touchEndEvent(QTouchEvent* event);
    void touchUpdateEvent(QTouchEvent* event);

    void wheelEvent(QWheelEvent* event);
    void dropEvent(QDropEvent *event);
    
    bool event(QEvent* event);
    
    void makeVoxel(glm::vec3 position,
                   float scale,
                   unsigned char red,
                   unsigned char green,
                   unsigned char blue,
                   bool isDestructive);

    void removeVoxel(glm::vec3 position, float scale);

    glm::vec3 getMouseVoxelWorldCoordinates(const VoxelDetail& mouseVoxel);

    QGLWidget* getGLWidget() { return _glWidget; }
    bool isThrottleRendering() const { return _glWidget->isThrottleRendering(); }
    MyAvatar* getAvatar() { return _myAvatar; }
    Audio* getAudio() { return &_audio; }
    Camera* getCamera() { return &_myCamera; }
    ViewFrustum* getViewFrustum() { return &_viewFrustum; }
    ViewFrustum* getShadowViewFrustum() { return &_shadowViewFrustum; }
    VoxelSystem* getVoxels() { return &_voxels; }
    VoxelTree* getVoxelTree() { return _voxels.getTree(); }
    ParticleTreeRenderer* getParticles() { return &_particles; }
    MetavoxelSystem* getMetavoxels() { return &_metavoxels; }
    bool getImportSucceded() { return _importSucceded; }
    VoxelSystem* getSharedVoxelSystem() { return &_sharedVoxelSystem; }
    VoxelTree* getClipboard() { return &_clipboard; }
    Environment* getEnvironment() { return &_environment; }
    bool isMousePressed() const { return _mousePressed; }
    bool isMouseHidden() const { return _mouseHidden; }
    const glm::vec3& getMouseRayOrigin() const { return _mouseRayOrigin; }
    const glm::vec3& getMouseRayDirection() const { return _mouseRayDirection; }
    Faceshift* getFaceshift() { return &_faceshift; }
    Visage* getVisage() { return &_visage; }
    SixenseManager* getSixenseManager() { return &_sixenseManager; }
    BandwidthMeter* getBandwidthMeter() { return &_bandwidthMeter; }

    /// if you need to access the application settings, use lockSettings()/unlockSettings()
    QSettings* lockSettings() { _settingsMutex.lock(); return _settings; }
    void unlockSettings() { _settingsMutex.unlock(); }

    QMainWindow* getWindow() { return _window; }
    NodeToOctreeSceneStats* getOcteeSceneStats() { return &_octreeServerSceneStats; }
    void lockOctreeSceneStats() { _octreeSceneStatsLock.lockForRead(); }
    void unlockOctreeSceneStats() { _octreeSceneStatsLock.unlock(); }

    QNetworkAccessManager* getNetworkAccessManager() { return _networkAccessManager; }
    GeometryCache* getGeometryCache() { return &_geometryCache; }
    TextureCache* getTextureCache() { return &_textureCache; }
    GlowEffect* getGlowEffect() { return &_glowEffect; }
    ControllerScriptingInterface* getControllerScriptingInterface() { return &_controllerScriptingInterface; }

    AvatarManager& getAvatarManager() { return _avatarManager; }
    void resetProfile(const QString& username);

    void controlledBroadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes);

    void setupWorldLight();

    QImage renderAvatarBillboard();

    void displaySide(Camera& whichCamera, bool selfAvatarOnly = false);

    /// Loads a view matrix that incorporates the specified model translation without the precision issues that can
    /// result from matrix multiplication at high translation magnitudes.
    void loadTranslatedViewMatrix(const glm::vec3& translation);

    const glm::mat4& getShadowMatrix() const { return _shadowMatrix; }

    void getModelViewMatrix(glm::dmat4* modelViewMatrix);
    void getProjectionMatrix(glm::dmat4* projectionMatrix);

    /// Computes the off-axis frustum parameters for the view frustum, taking mirroring into account.
    void computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
        float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const;



    VoxelShader& getVoxelShader() { return _voxelShader; }
    PointShader& getPointShader() { return _pointShader; }
    FileLogger* getLogger() { return _logger; }

    glm::vec2 getViewportDimensions() const{ return glm::vec2(_glWidget->width(),_glWidget->height()); }
    NodeToJurisdictionMap& getVoxelServerJurisdictions() { return _voxelServerJurisdictions; }
    NodeToJurisdictionMap& getParticleServerJurisdictions() { return _particleServerJurisdictions; }
    void pasteVoxelsToOctalCode(const unsigned char* octalCodeDestination);

    void skipVersion(QString latestVersion);

signals:

    /// Fired when we're simulating; allows external parties to hook in.
    void simulating(float deltaTime);

    /// Fired when we're rendering in-world interface elements; allows external parties to hook in.
    void renderingInWorldInterface();
    
    /// Fired when the import window is closed
    void importDone();
    
public slots:
    void domainChanged(const QString& domainHostname);
    void updateWindowTitle();
    void nodeAdded(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
    void packetSent(quint64 length);

    void importVoxels(); // doesn't include source voxel because it goes to clipboard
    void cutVoxels(const VoxelDetail& sourceVoxel);
    void copyVoxels(const VoxelDetail& sourceVoxel);
    void pasteVoxels(const VoxelDetail& sourceVoxel);
    void deleteVoxels(const VoxelDetail& sourceVoxel);
    void exportVoxels(const VoxelDetail& sourceVoxel);
    void nudgeVoxelsByVector(const VoxelDetail& sourceVoxel, const glm::vec3& nudgeVec);

    void setRenderVoxels(bool renderVoxels);
    void doKillLocalVoxels();
    void loadDialog();
    void toggleLogDialog();
    void initAvatarAndViewFrustum();
    void stopAllScripts();
    void reloadAllScripts();
    
    void uploadFST();

private slots:
    void timer();
    void idle();
    
    void connectedToDomain(const QString& hostname);

    void setFullscreen(bool fullscreen);
    void setEnable3DTVMode(bool enable3DTVMode);
    void cameraMenuChanged();
    
    glm::vec2 getScaledScreenPoint(glm::vec2 projectedPoint);

    void closeMirrorView();
    void restoreMirrorView();
    void shrinkMirrorView();
    void resetSensors();
    
    void parseVersionXml();

    void removeScriptName(const QString& fileNameString);
    void cleanupScriptMenuItem(const QString& scriptMenuName);

private:
    void resetCamerasOnResizeGL(Camera& camera, int width, int height);
    void updateProjectionMatrix();
    void updateProjectionMatrix(Camera& camera, bool updateViewFrustum = true);

    static bool sendVoxelsOperation(OctreeElement* node, void* extraData);
    void sendPingPackets();

    void initDisplay();
    void init();

    void update(float deltaTime);

    // Various helper functions called during update()
    void updateLOD();
    void updateMouseRay();
    void updateFaceshift();
    void updateVisage();
    void updateMyAvatarLookAtPosition();
    void updateHandAndTouch(float deltaTime);
    void updateLeap(float deltaTime);
    void updateSixense(float deltaTime);
    void updateSerialDevices(float deltaTime);
    void updateThreads(float deltaTime);
    void updateMetavoxels(float deltaTime);
    void updateCamera(float deltaTime);
    void updateDialogs(float deltaTime);
    void updateAudio(float deltaTime);
    void updateCursor(float deltaTime);

    Avatar* findLookatTargetAvatar(glm::vec3& eyePosition, QUuid &nodeUUID);
    bool isLookingAtMyAvatar(Avatar* avatar);

    void renderLookatIndicator(glm::vec3 pointOfInterest);

    void updateMyAvatar(float deltaTime);
    void queryOctree(NodeType_t serverType, PacketType packetType, NodeToJurisdictionMap& jurisdictions);
    void loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum);

    glm::vec3 getSunDirection();

    void updateShadowMap();
    void displayOverlay();
    void displayStatsBackground(unsigned int rgba, int x, int y, int width, int height);
    void displayStats();
    void checkStatsClick();
    void toggleStatsExpanded();
    void renderRearViewMirror(const QRect& region, bool billboard = false);
    void renderViewFrustum(ViewFrustum& viewFrustum);

    void checkBandwidthMeterClick();

    void deleteVoxelAt(const VoxelDetail& voxel);
    void eyedropperVoxelUnderCursor();

    void setMenuShortcutsEnabled(bool enabled);

    static void attachNewHeadToNode(Node *newNode);
    static void* networkReceive(void* args); // network receive thread

    void findAxisAlignment();

    void displayRearMirrorTools();

    QMainWindow* _window;
    GLCanvas* _glWidget; // our GLCanvas has a couple extra features

    bool _statsExpanded;
    BandwidthMeter _bandwidthMeter;
    
    QThread* _nodeThread;
    DatagramProcessor _datagramProcessor;

    QNetworkAccessManager* _networkAccessManager;
    QMutex _settingsMutex;
    QSettings* _settings;

    glm::vec3 _gravity;

    // Frame Rate Measurement

    int _frameCount;
    float _fps;
    timeval _applicationStartupTime;
    timeval _timerStart, _timerEnd;
    timeval _lastTimeUpdated;
    bool _justStarted;
    Stars _stars;
    
    BuckyBalls _buckyBalls;

    VoxelSystem _voxels;
    VoxelTree _clipboard; // if I copy/paste
    VoxelImporter* _voxelImporter;
    bool _importSucceded;
    VoxelSystem _sharedVoxelSystem;
    ViewFrustum _sharedVoxelSystemViewFrustum;

    ParticleTreeRenderer _particles;
    ParticleCollisionSystem _particleCollisionSystem;

    QByteArray _voxelsFilename;
    bool _wantToKillLocalVoxels;

    MetavoxelSystem _metavoxels;

    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    ViewFrustum _lastQueriedViewFrustum; /// last view frustum used to query octree servers (voxels, particles)
    ViewFrustum _shadowViewFrustum;
    quint64 _lastQueriedTime;

    Oscilloscope _audioScope;

    OctreeQuery _octreeQuery; // NodeData derived class for querying voxels from voxel server

    AvatarManager _avatarManager;
    MyAvatar* _myAvatar;            // TODO: move this and relevant code to AvatarManager (or MyAvatar as the case may be)

    Faceshift _faceshift;
    Visage _visage;

    SixenseManager _sixenseManager;
    QStringList _activeScripts;

    Camera _myCamera;                  // My view onto the world
    Camera _viewFrustumOffsetCamera;   // The camera we use to sometimes show the view frustum from an offset mode
    Camera _mirrorCamera;              // Cammera for mirror view
    QRect _mirrorViewRect;
    RearMirrorTools* _rearMirrorTools;

    glm::mat4 _untranslatedViewMatrix;
    glm::vec3 _viewMatrixTranslation;
    glm::mat4 _projectionMatrix;

    glm::mat4 _shadowMatrix;

    Environment _environment;

    int _mouseX;
    int _mouseY;
    int _mouseDragStartedX;
    int _mouseDragStartedY;
    quint64 _lastMouseMove;
    bool _mouseHidden;
    bool _seenMouseMove;

    glm::vec3 _mouseRayOrigin;
    glm::vec3 _mouseRayDirection;

    float _touchAvgX;
    float _touchAvgY;
    float _lastTouchAvgX;
    float _lastTouchAvgY;
    float _touchDragStartedAvgX;
    float _touchDragStartedAvgY;
    bool _isTouchPressed; //  true if multitouch has been pressed (clear when finished)

    bool _mousePressed; //  true if mouse has been pressed (clear when finished)

    QSet<int> _keysPressed;

    GeometryCache _geometryCache;
    TextureCache _textureCache;

    GlowEffect _glowEffect;
    AmbientOcclusionEffect _ambientOcclusionEffect;
    VoxelShader _voxelShader;
    PointShader _pointShader;

    Audio _audio;

    bool _enableProcessVoxelsThread;
    VoxelPacketProcessor _voxelProcessor;
    VoxelHideShowThread _voxelHideShowThread;
    VoxelEditPacketSender _voxelEditSender;
    ParticleEditPacketSender _particleEditSender;

    int _packetsPerSecond;
    int _bytesPerSecond;

    int _recentMaxPackets; // recent max incoming voxel packets to process
    bool _resetRecentMaxPacketsSoon;

    StDev _idleLoopStdev;
    float _idleLoopMeasuredJitter;

    int parseOctreeStats(const QByteArray& packet, const SharedNodePointer& sendingNode);
    void trackIncomingVoxelPacket(const QByteArray& packet, const SharedNodePointer& sendingNode, bool wasStatsPacket);

    NodeToJurisdictionMap _voxelServerJurisdictions;
    NodeToJurisdictionMap _particleServerJurisdictions;
    NodeToOctreeSceneStats _octreeServerSceneStats;
    QReadWriteLock _octreeSceneStatsLock;

    std::vector<VoxelFade> _voxelFades;
    ControllerScriptingInterface _controllerScriptingInterface;
    QPointer<LogDialog> _logDialog;

    FileLogger* _logger;

    void checkVersion();
    void displayUpdateDialog();
    bool shouldSkipVersion(QString latestVersion);
    void takeSnapshot();
    
    TouchEvent _lastTouchEvent;
    
    Overlays _overlays;
};

#endif /* defined(__interface__Application__) */
