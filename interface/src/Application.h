//
//  Application.h
//  interface/src
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Application_h
#define hifi_Application_h

#include <map>
#include <time.h>

#include <QApplication>
#include <QMainWindow>
#include <QAction>
#include <QHash>
#include <QImage>
#include <QList>
#include <QPointer>
#include <QSet>
#include <QSettings>
#include <QStringList>
#include <QHash>
#include <QTouchEvent>
#include <QUndoStack>
#include <QSystemTrayIcon>

#include <EntityCollisionSystem.h>
#include <EntityEditPacketSender.h>
#include <NetworkPacket.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <ScriptEngine.h>
#include <OctreeQuery.h>
#include <ViewFrustum.h>
#include <VoxelEditPacketSender.h>

#include "MainWindow.h"
#include "Audio.h"
#include "Camera.h"
#include "DatagramProcessor.h"
#include "Environment.h"
#include "FileLogger.h"
#include "GLCanvas.h"
#include "Menu.h"
#include "MetavoxelSystem.h"
#include "PacketHeaders.h"
#include "Stars.h"
#include "avatar/Avatar.h"
#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"
#include "devices/PrioVR.h"
#include "devices/SixenseManager.h"
#include "entities/EntityTreeRenderer.h"
#include "renderer/AmbientOcclusionEffect.h"
#include "renderer/DeferredLightingEffect.h"
#include "renderer/GeometryCache.h"
#include "renderer/GlowEffect.h"
#include "renderer/TextureCache.h"
#include "scripting/ControllerScriptingInterface.h"
#include "ui/BandwidthDialog.h"
#include "ui/BandwidthMeter.h"
#include "ui/HMDToolsDialog.h"
#include "ui/ModelsBrowser.h"
#include "ui/NodeBounds.h"
#include "ui/OctreeStatsDialog.h"
#include "ui/RearMirrorTools.h"
#include "ui/SnapshotShareDialog.h"
#include "ui/LodToolsDialog.h"
#include "ui/LogDialog.h"
#include "ui/UpdateDialog.h"
#include "ui/overlays/Overlays.h"
#include "ui/ApplicationOverlay.h"
#include "ui/RunningScriptsWidget.h"
#include "ui/ToolWindow.h"
#include "ui/VoxelImportDialog.h"
#include "voxels/VoxelFade.h"
#include "voxels/VoxelHideShowThread.h"
#include "voxels/VoxelImporter.h"
#include "voxels/OctreePacketProcessor.h"
#include "voxels/VoxelSystem.h"


#include "UndoStackScriptingInterface.h"


class QAction;
class QActionGroup;
class QGLWidget;
class QKeyEvent;
class QMouseEvent;
class QSettings;
class QWheelEvent;

class FaceTracker;
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
static const float BILLBOARD_DISTANCE = 5.56f;       // meters

static const int MIRROR_VIEW_TOP_PADDING = 5;
static const int MIRROR_VIEW_LEFT_PADDING = 10;
static const int MIRROR_VIEW_WIDTH = 265;
static const int MIRROR_VIEW_HEIGHT = 215;
static const float MIRROR_FULLSCREEN_DISTANCE = 0.389f;
static const float MIRROR_REARVIEW_DISTANCE = 0.722f;
static const float MIRROR_REARVIEW_BODY_DISTANCE = 2.56f;
static const float MIRROR_FIELD_OF_VIEW = 30.0f;

static const quint64 TOO_LONG_SINCE_LAST_SEND_DOWNSTREAM_AUDIO_STATS = 1 * USECS_PER_SECOND;

static const QString INFO_HELP_PATH = "html/interface-welcome-allsvg.html";
static const QString INFO_EDIT_ENTITIES_PATH = "html/edit-entities-commands.html";

class Application : public QApplication {
    Q_OBJECT

    friend class OctreePacketProcessor;
    friend class VoxelEditPacketSender;
    friend class DatagramProcessor;

public:
    static Application* getInstance() { return static_cast<Application*>(QCoreApplication::instance()); }
    static QString& resourcesPath();
    static const glm::vec3& getPositionForPath() { return getInstance()->_myAvatar->getPosition(); }
    static glm::quat getOrientationForPath() { return getInstance()->_myAvatar->getOrientation(); }

    Application(int& argc, char** argv, QElapsedTimer &startup_time);
    ~Application();

    void restoreSizeAndPosition();
    void loadScripts();
    QString getPreviousScriptLocation();
    void setPreviousScriptLocation(const QString& previousScriptLocation);
    void storeSizeAndPosition();
    void clearScriptsBeforeRunning();
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void focusOutEvent(QFocusEvent* event);
    void focusInEvent(QFocusEvent* event);

    void mouseMoveEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mousePressEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID = 0);

    void touchBeginEvent(QTouchEvent* event);
    void touchEndEvent(QTouchEvent* event);
    void touchUpdateEvent(QTouchEvent* event);

    void wheelEvent(QWheelEvent* event);
    void dropEvent(QDropEvent *event);

    bool event(QEvent* event);
    bool eventFilter(QObject* object, QEvent* event);

    void makeVoxel(glm::vec3 position,
                   float scale,
                   unsigned char red,
                   unsigned char green,
                   unsigned char blue,
                   bool isDestructive);

    void removeVoxel(glm::vec3 position, float scale);

    glm::vec3 getMouseVoxelWorldCoordinates(const VoxelDetail& mouseVoxel);

    GLCanvas* getGLWidget() { return _glWidget; }
    bool isThrottleRendering() const { return _glWidget->isThrottleRendering(); }
    MyAvatar* getAvatar() { return _myAvatar; }
    Audio* getAudio() { return &_audio; }
    Camera* getCamera() { return &_myCamera; }
    ViewFrustum* getViewFrustum() { return &_viewFrustum; }
    ViewFrustum* getDisplayViewFrustum() { return &_displayViewFrustum; }
    ViewFrustum* getShadowViewFrustum() { return &_shadowViewFrustum; }
    VoxelImporter* getVoxelImporter() { return &_voxelImporter; }
    VoxelSystem* getVoxels() { return &_voxels; }
    VoxelTree* getVoxelTree() { return _voxels.getTree(); }
    const OctreePacketProcessor& getOctreePacketProcessor() const { return _octreeProcessor; }
    MetavoxelSystem* getMetavoxels() { return &_metavoxels; }
    EntityTreeRenderer* getEntities() { return &_entities; }
    bool getImportSucceded() { return _importSucceded; }
    VoxelSystem* getSharedVoxelSystem() { return &_sharedVoxelSystem; }
    VoxelTree* getClipboard() { return &_clipboard; }
    EntityTree* getEntityClipboard() { return &_entityClipboard; }
    EntityTreeRenderer* getEntityClipboardRenderer() { return &_entityClipboardRenderer; }
    Environment* getEnvironment() { return &_environment; }
    bool isMousePressed() const { return _mousePressed; }
    bool isMouseHidden() const { return _glWidget->cursor().shape() == Qt::BlankCursor; }
    void setCursorVisible(bool visible);
    const glm::vec3& getMouseRayOrigin() const { return _mouseRayOrigin; }
    const glm::vec3& getMouseRayDirection() const { return _mouseRayDirection; }
    bool mouseOnScreen() const;
    int getMouseX() const;
    int getMouseY() const;
    int getTrueMouseX() const { return _glWidget->mapFromGlobal(QCursor::pos()).x(); }
    int getTrueMouseY() const { return _glWidget->mapFromGlobal(QCursor::pos()).y(); }
    int getMouseDragStartedX() const;
    int getMouseDragStartedY() const;
    int getTrueMouseDragStartedX() const { return _mouseDragStartedX; }
    int getTrueMouseDragStartedY() const { return _mouseDragStartedY; }
    bool getLastMouseMoveWasSimulated() const { return _lastMouseMoveWasSimulated; }
    FaceTracker* getActiveFaceTracker();
    PrioVR* getPrioVR() { return &_prioVR; }
    BandwidthMeter* getBandwidthMeter() { return &_bandwidthMeter; }
    QUndoStack* getUndoStack() { return &_undoStack; }
    QSystemTrayIcon* getTrayIcon() { return _trayIcon; }
    ApplicationOverlay& getApplicationOverlay() { return _applicationOverlay; }
    Overlays& getOverlays() { return _overlays; }

    float getFps() const { return _fps; }
    float getPacketsPerSecond() const { return _packetsPerSecond; }
    float getBytesPerSecond() const { return _bytesPerSecond; }
    const glm::vec3& getViewMatrixTranslation() const { return _viewMatrixTranslation; }
    void setViewMatrixTranslation(const glm::vec3& translation) { _viewMatrixTranslation = translation; }

    const Transform& getViewTransform() const { return _viewTransform; }
    void setViewTransform(const Transform& view);

    /// if you need to access the application settings, use lockSettings()/unlockSettings()
    QSettings* lockSettings() { _settingsMutex.lock(); return _settings; }
    void unlockSettings() { _settingsMutex.unlock(); }

    void saveSettings();

    MainWindow* getWindow() { return _window; }
    NodeToOctreeSceneStats* getOcteeSceneStats() { return &_octreeServerSceneStats; }
    void lockOctreeSceneStats() { _octreeSceneStatsLock.lockForRead(); }
    void unlockOctreeSceneStats() { _octreeSceneStatsLock.unlock(); }

    ToolWindow* getToolWindow() { return _toolWindow ; }

    GeometryCache* getGeometryCache() { return &_geometryCache; }
    AnimationCache* getAnimationCache() { return &_animationCache; }
    TextureCache* getTextureCache() { return &_textureCache; }
    DeferredLightingEffect* getDeferredLightingEffect() { return &_deferredLightingEffect; }
    GlowEffect* getGlowEffect() { return &_glowEffect; }
    ControllerScriptingInterface* getControllerScriptingInterface() { return &_controllerScriptingInterface; }

    AvatarManager& getAvatarManager() { return _avatarManager; }
    void resetProfile(const QString& username);

    void controlledBroadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes);

    void setupWorldLight();

    QImage renderAvatarBillboard();

    void displaySide(Camera& whichCamera, bool selfAvatarOnly = false, RenderArgs::RenderSide renderSide = RenderArgs::MONO);

    /// Stores the current modelview matrix as the untranslated view matrix to use for transforms and the supplied vector as
    /// the view matrix translation.
    void updateUntranslatedViewMatrix(const glm::vec3& viewMatrixTranslation = glm::vec3());

    const glm::mat4& getUntranslatedViewMatrix() const { return _untranslatedViewMatrix; }

    /// Loads a view matrix that incorporates the specified model translation without the precision issues that can
    /// result from matrix multiplication at high translation magnitudes.
    void loadTranslatedViewMatrix(const glm::vec3& translation);

    void getModelViewMatrix(glm::dmat4* modelViewMatrix);
    void getProjectionMatrix(glm::dmat4* projectionMatrix);

    const glm::vec3& getShadowDistances() const { return _shadowDistances; }

    /// Computes the off-axis frustum parameters for the view frustum, taking mirroring into account.
    void computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
        float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const;

    NodeBounds& getNodeBoundsDisplay()  { return _nodeBoundsDisplay; }

    FileLogger* getLogger() { return _logger; }

    glm::vec2 getViewportDimensions() const { return glm::vec2(_glWidget->getDeviceWidth(), _glWidget->getDeviceHeight()); }
    NodeToJurisdictionMap& getVoxelServerJurisdictions() { return _voxelServerJurisdictions; }
    NodeToJurisdictionMap& getEntityServerJurisdictions() { return _entityServerJurisdictions; }
    void pasteVoxelsToOctalCode(const unsigned char* octalCodeDestination);

    void skipVersion(QString latestVersion);

    QStringList getRunningScripts() { return _scriptEnginesHash.keys(); }
    ScriptEngine* getScriptEngine(QString scriptHash) { return _scriptEnginesHash.contains(scriptHash) ? _scriptEnginesHash[scriptHash] : NULL; }
    
    bool isLookingAtMyAvatar(Avatar* avatar);

    float getRenderResolutionScale() const;

    unsigned int getRenderTargetFramerate() const;
    bool isVSyncOn() const;
    bool isVSyncEditable() const;
    bool isAboutToQuit() const { return _aboutToQuit; }


    void registerScriptEngineWithApplicationServices(ScriptEngine* scriptEngine);

    // the isHMDmode is true whenever we use the interface from an HMD and not a standard flat display
    // rendering of several elements depend on that
    // TODO: carry that information on the Camera as a setting
    bool isHMDMode() const;
    
    QRect getDesirableApplicationGeometry();
    RunningScriptsWidget* getRunningScriptsWidget() { return _runningScriptsWidget; }

signals:

    /// Fired when we're simulating; allows external parties to hook in.
    void simulating(float deltaTime);

    /// Fired when we're rendering in-world interface elements; allows external parties to hook in.
    void renderingInWorldInterface();

    /// Fired when we're rendering the overlay.
    void renderingOverlay();

    /// Fired when the import window is closed
    void importDone();

public slots:
    void changeDomainHostname(const QString& newDomainHostname);
    void domainChanged(const QString& domainHostname);
    void updateWindowTitle();
    void updateLocationInServer();
    void nodeAdded(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
    void packetSent(quint64 length);

    void pasteEntities(float x, float y, float z);
    bool exportEntities(const QString& filename, float x, float y, float z, float scale);
    bool importEntities(const QString& filename);

    void importVoxels(); // doesn't include source voxel because it goes to clipboard
    void cutVoxels(const VoxelDetail& sourceVoxel);
    void copyVoxels(const VoxelDetail& sourceVoxel);
    void pasteVoxels(const VoxelDetail& sourceVoxel);
    void deleteVoxels(const VoxelDetail& sourceVoxel);
    void exportVoxels(const VoxelDetail& sourceVoxel);
    void nudgeVoxelsByVector(const VoxelDetail& sourceVoxel, const glm::vec3& nudgeVec);

    void setRenderVoxels(bool renderVoxels);
    void setLowVelocityFilter(bool lowVelocityFilter);
    void doKillLocalVoxels();
    void loadDialog();
    void loadScriptURLDialog();
    void toggleLogDialog();
    void initAvatarAndViewFrustum();
    ScriptEngine* loadScript(const QString& scriptFilename = QString(), bool isUserLoaded = true, 
        bool loadScriptFromEditor = false, bool activateMainWindow = false);
    void scriptFinished(const QString& scriptName);
    void stopAllScripts(bool restart = false);
    void stopScript(const QString& scriptName);
    void reloadAllScripts();
    void loadDefaultScripts();
    void toggleRunningScriptsWidget();
    void saveScripts();

    void uploadHead();
    void uploadSkeleton();
    void uploadAttachment();
    
    void openUrl(const QUrl& url);

    void bumpSettings() { ++_numChangedSettings; }
    
    void domainSettingsReceived(const QJsonObject& domainSettingsObject);

    void setVSyncEnabled(bool vsyncOn);

    void resetSensors();

private slots:
    void clearDomainOctreeDetails();
    void timer();
    void idle();
    void aboutToQuit();
    
    void handleScriptEngineLoaded(const QString& scriptFilename);
    void handleScriptLoadError(const QString& scriptFilename);

    void connectedToDomain(const QString& hostname);

    friend class HMDToolsDialog;
    void setFullscreen(bool fullscreen);
    void setEnable3DTVMode(bool enable3DTVMode);
    void setEnableVRMode(bool enableVRMode);
    void cameraMenuChanged();

    glm::vec2 getScaledScreenPoint(glm::vec2 projectedPoint);

    void closeMirrorView();
    void restoreMirrorView();
    void shrinkMirrorView();

    void parseVersionXml();

    void manageRunningScriptsWidgetVisibility(bool shown);

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
    void updateDDE();
    void updateMyAvatarLookAtPosition();
    void updateThreads(float deltaTime);
    void updateMetavoxels(float deltaTime);
    void updateCamera(float deltaTime);
    void updateDialogs(float deltaTime);
    void updateCursor(float deltaTime);

    Avatar* findLookatTargetAvatar(glm::vec3& eyePosition, QUuid &nodeUUID);

    void renderLookatIndicator(glm::vec3 pointOfInterest);

    void updateMyAvatar(float deltaTime);
    void queryOctree(NodeType_t serverType, PacketType packetType, NodeToJurisdictionMap& jurisdictions);
    void loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum);

    glm::vec3 getSunDirection();

    void updateShadowMap();
    void renderRearViewMirror(const QRect& region, bool billboard = false);
    void renderViewFrustum(ViewFrustum& viewFrustum);

    void checkBandwidthMeterClick();

    void deleteVoxelAt(const VoxelDetail& voxel);
    void eyedropperVoxelUnderCursor();

    void setMenuShortcutsEnabled(bool enabled);

    void uploadModel(ModelType modelType);

    static void attachNewHeadToNode(Node *newNode);
    static void* networkReceive(void* args); // network receive thread

    int sendNackPackets();

    MainWindow* _window;
    GLCanvas* _glWidget; // our GLCanvas has a couple extra features

    ToolWindow* _toolWindow;

    BandwidthMeter _bandwidthMeter;

    QThread* _nodeThread;
    DatagramProcessor _datagramProcessor;

    QMutex _settingsMutex;
    QSettings* _settings;
    int _numChangedSettings;

    QUndoStack _undoStack;
    UndoStackScriptingInterface _undoStackScriptingInterface;

    glm::vec3 _gravity;

    // Frame Rate Measurement

    int _frameCount;
    float _fps;
    QElapsedTimer _applicationStartupTime;
    QElapsedTimer _timerStart;
    QElapsedTimer _lastTimeUpdated;
    bool _justStarted;
    Stars _stars;

    VoxelSystem _voxels;
    VoxelTree _clipboard; // if I copy/paste
    VoxelImportDialog* _voxelImportDialog;
    VoxelImporter _voxelImporter;
    bool _importSucceded;
    VoxelSystem _sharedVoxelSystem;
    ViewFrustum _sharedVoxelSystemViewFrustum;

    EntityTreeRenderer _entities;
    EntityCollisionSystem _entityCollisionSystem;
    EntityTreeRenderer _entityClipboardRenderer;
    EntityTree _entityClipboard;

    QByteArray _voxelsFilename;
    bool _wantToKillLocalVoxels;

    MetavoxelSystem _metavoxels;

    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    ViewFrustum _lastQueriedViewFrustum; /// last view frustum used to query octree servers (voxels)
    ViewFrustum _displayViewFrustum;
    ViewFrustum _shadowViewFrustum;
    quint64 _lastQueriedTime;

    float _trailingAudioLoudness;

    OctreeQuery _octreeQuery; // NodeData derived class for querying voxels from voxel server

    AvatarManager _avatarManager;
    MyAvatar* _myAvatar;            // TODO: move this and relevant code to AvatarManager (or MyAvatar as the case may be)

    PrioVR _prioVR;

    Camera _myCamera;                  // My view onto the world
    Camera _viewFrustumOffsetCamera;   // The camera we use to sometimes show the view frustum from an offset mode
    Camera _mirrorCamera;              // Cammera for mirror view
    QRect _mirrorViewRect;
    RearMirrorTools* _rearMirrorTools;

    Transform _viewTransform;
    glm::mat4 _untranslatedViewMatrix;
    glm::vec3 _viewMatrixTranslation;
    glm::mat4 _projectionMatrix;

    float _scaleMirror;
    float _rotateMirror;
    float _raiseMirror;

    static const int CASCADED_SHADOW_MATRIX_COUNT = 4;
    glm::mat4 _shadowMatrices[CASCADED_SHADOW_MATRIX_COUNT];
    glm::vec3 _shadowDistances;

    Environment _environment;

    int _mouseDragStartedX;
    int _mouseDragStartedY;
    quint64 _lastMouseMove;
    bool _lastMouseMoveWasSimulated;

    glm::vec3 _mouseRayOrigin;
    glm::vec3 _mouseRayDirection;

    float _touchAvgX;
    float _touchAvgY;
    float _touchDragStartedAvgX;
    float _touchDragStartedAvgY;
    bool _isTouchPressed; //  true if multitouch has been pressed (clear when finished)

    bool _mousePressed; //  true if mouse has been pressed (clear when finished)

    QSet<int> _keysPressed;


    GeometryCache _geometryCache;
    AnimationCache _animationCache;
    TextureCache _textureCache;

    DeferredLightingEffect _deferredLightingEffect;
    GlowEffect _glowEffect;
    AmbientOcclusionEffect _ambientOcclusionEffect;

    Audio _audio;

    bool _enableProcessVoxelsThread;
    OctreePacketProcessor _octreeProcessor;
    VoxelHideShowThread _voxelHideShowThread;
    VoxelEditPacketSender _voxelEditSender;
    EntityEditPacketSender _entityEditSender;

    int _packetsPerSecond;
    int _bytesPerSecond;

    StDev _idleLoopStdev;
    float _idleLoopMeasuredJitter;

    int parseOctreeStats(const QByteArray& packet, const SharedNodePointer& sendingNode);
    void trackIncomingVoxelPacket(const QByteArray& packet, const SharedNodePointer& sendingNode, bool wasStatsPacket);

    NodeToJurisdictionMap _voxelServerJurisdictions;
    NodeToJurisdictionMap _entityServerJurisdictions;
    NodeToOctreeSceneStats _octreeServerSceneStats;
    QReadWriteLock _octreeSceneStatsLock;

    NodeBounds _nodeBoundsDisplay;

    std::vector<VoxelFade> _voxelFades;
    QReadWriteLock _voxelFadesLock;
    ControllerScriptingInterface _controllerScriptingInterface;
    QPointer<LogDialog> _logDialog;
    QPointer<SnapshotShareDialog> _snapshotShareDialog;

    QString _previousScriptLocation;

    FileLogger* _logger;

    void checkVersion();
    void displayUpdateDialog();
    bool shouldSkipVersion(QString latestVersion);
    void takeSnapshot();

    TouchEvent _lastTouchEvent;

    Overlays _overlays;
    ApplicationOverlay _applicationOverlay;

    RunningScriptsWidget* _runningScriptsWidget;
    QHash<QString, ScriptEngine*> _scriptEnginesHash;
    bool _runningScriptsWidgetWasVisible;

    QSystemTrayIcon* _trayIcon;

    quint64 _lastNackTime;
    quint64 _lastSendDownstreamAudioStats;

    bool _isVSyncOn;
    
    bool _aboutToQuit;
};

#endif // hifi_Application_h
