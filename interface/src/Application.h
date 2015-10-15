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

#include <functional>

#include <QApplication>
#include <QHash>
#include <QImage>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QUndoStack>

#include <AbstractScriptingServicesInterface.h>
#include <AbstractViewStateInterface.h>
#include <EntityEditPacketSender.h>
#include <EntityTreeRenderer.h>
#include <input-plugins/KeyboardMouseDevice.h>
#include <OctreeQuery.h>
#include <PhysicalEntitySimulation.h>
#include <PhysicsEngine.h>
#include <plugins/Forward.h>
#include <ScriptEngine.h>
#include <ShapeManager.h>
#include <SimpleMovingAverage.h>
#include <StDev.h>
#include <ViewFrustum.h>

#include "avatar/AvatarUpdate.h"
#include "avatar/MyAvatar.h"
#include "Bookmarks.h"
#include "Camera.h"
#include "Environment.h"
#include "FileLogger.h"
#include "gpu/Context.h"
#include "Menu.h"
#include "octree/OctreePacketProcessor.h"
#include "render/Engine.h"
#include "scripting/ControllerScriptingInterface.h"
#include "scripting/DialogsManagerScriptingInterface.h"
#include "ui/ApplicationCompositor.h"
#include "ui/ApplicationOverlay.h"
#include "ui/AudioStatsDialog.h"
#include "ui/BandwidthDialog.h"
#include "ui/LodToolsDialog.h"
#include "ui/LogDialog.h"
#include "ui/OctreeStatsDialog.h"
#include "ui/OverlayConductor.h"
#include "ui/overlays/Overlays.h"
#include "ui/RunningScriptsWidget.h"
#include "ui/SnapshotShareDialog.h"
#include "ui/ToolWindow.h"
#include "UndoStackScriptingInterface.h"

class OffscreenGlCanvas;
class GLCanvas;
class FaceTracker;
class MainWindow;
class AssetUpload;

#ifdef Q_OS_WIN
static const UINT UWM_IDENTIFY_INSTANCES =
    RegisterWindowMessage("UWM_IDENTIFY_INSTANCES_{8AB82783-B74A-4258-955B-8188C22AA0D6}_" + qgetenv("USERNAME"));
static const UINT UWM_SHOW_APPLICATION =
    RegisterWindowMessage("UWM_SHOW_APPLICATION_{71123FD6-3DA8-4DC1-9C27-8A12A6250CBA}_" + qgetenv("USERNAME"));
#endif

class Application;
#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application*>(QCoreApplication::instance()))

class Application : public QApplication, public AbstractViewStateInterface, public AbstractScriptingServicesInterface {
    Q_OBJECT
    
    // TODO? Get rid of those
    friend class OctreePacketProcessor;
    friend class PluginContainerProxy;

public:
    // FIXME? Empty methods, do we still need them?
    static void initPlugins();
    static void shutdownPlugins();

    Application(int& argc, char** argv, QElapsedTimer& startup_time);
    ~Application();

    void postLambdaEvent(std::function<void()> f);

    void loadScripts();
    QString getPreviousScriptLocation();
    void setPreviousScriptLocation(const QString& previousScriptLocation);
    void clearScriptsBeforeRunning();
    void initializeGL();
    void initializeUi();
    void paintGL();
    void resizeGL();

    bool event(QEvent* event);
    bool eventFilter(QObject* object, QEvent* event);

    glm::uvec2 getCanvasSize() const;
    glm::uvec2 getUiSize() const;
    QSize getDeviceSize() const;
    bool hasFocus() const;
    PickRay computePickRay() const;

    bool isThrottleRendering() const;

    Camera* getCamera() { return &_myCamera; }
    // Represents the current view frustum of the avatar.
    ViewFrustum* getViewFrustum();
    const ViewFrustum* getViewFrustum() const;
    // Represents the view frustum of the current rendering pass,
    // which might be different from the viewFrustum, i.e. shadowmap
    // passes, mirror window passes, etc
    ViewFrustum* getDisplayViewFrustum();
    const ViewFrustum* getDisplayViewFrustum() const;
    ViewFrustum* getShadowViewFrustum() { return &_shadowViewFrustum; }
    const OctreePacketProcessor& getOctreePacketProcessor() const { return _octreeProcessor; }
    EntityTreeRenderer* getEntities() { return &_entities; }
    QUndoStack* getUndoStack() { return &_undoStack; }
    MainWindow* getWindow() { return _window; }
    EntityTreePointer getEntityClipboard() { return _entityClipboard; }
    EntityTreeRenderer* getEntityClipboardRenderer() { return &_entityClipboardRenderer; }
    EntityEditPacketSender* getEntityEditPacketSender() { return &_entityEditSender; }

    ivec2 getMouse() const;
    ivec2 getTrueMouse() const;
    bool getLastMouseMoveWasSimulated() const { return _lastMouseMoveWasSimulated; }

    FaceTracker* getActiveFaceTracker();
    FaceTracker* getSelectedFaceTracker();

    ApplicationOverlay& getApplicationOverlay() { return _applicationOverlay; }
    const ApplicationOverlay& getApplicationOverlay() const { return _applicationOverlay; }
    ApplicationCompositor& getApplicationCompositor() { return _compositor; }
    const ApplicationCompositor& getApplicationCompositor() const { return _compositor; }
    Overlays& getOverlays() { return _overlays; }

    bool isForeground() const { return _isForeground; }
    
    float getFps() const { return _fps; }

    float getFieldOfView() { return _fieldOfView.get(); }
    void setFieldOfView(float fov);

    NodeToOctreeSceneStats* getOcteeSceneStats() { return &_octreeServerSceneStats; }

    ToolWindow* getToolWindow() { return _toolWindow ; }

    virtual AbstractControllerScriptingInterface* getControllerScriptingInterface() { return &_controllerScriptingInterface; }
    virtual void registerScriptEngineWithApplicationServices(ScriptEngine* scriptEngine);

    QImage renderAvatarBillboard(RenderArgs* renderArgs);

    virtual ViewFrustum* getCurrentViewFrustum() { return getDisplayViewFrustum(); }
    virtual QThread* getMainThread() { return thread(); }
    virtual float getSizeScale() const;
    virtual int getBoundaryLevelAdjust() const;
    virtual PickRay computePickRay(float x, float y) const;
    virtual const glm::vec3& getAvatarPosition() const;
    virtual void overrideEnvironmentData(const EnvironmentData& newData) { _environment.override(newData); }
    virtual void endOverrideEnvironmentData() { _environment.endOverride(); }
    virtual qreal getDevicePixelRatio();

    void setActiveDisplayPlugin(const QString& pluginName);

    DisplayPlugin* getActiveDisplayPlugin();
    const DisplayPlugin* getActiveDisplayPlugin() const;

    FileLogger* getLogger() { return _logger; }

    glm::vec2 getViewportDimensions() const;

    NodeToJurisdictionMap& getEntityServerJurisdictions() { return _entityServerJurisdictions; }

    QStringList getRunningScripts() { return _scriptEnginesHash.keys(); }
    ScriptEngine* getScriptEngine(const QString& scriptHash) { return _scriptEnginesHash.value(scriptHash, NULL); }

    float getRenderResolutionScale() const;

    bool isAboutToQuit() const { return _aboutToQuit; }

    // the isHMDmode is true whenever we use the interface from an HMD and not a standard flat display
    // rendering of several elements depend on that
    // TODO: carry that information on the Camera as a setting
    bool isHMDMode() const;
    glm::mat4 getHMDSensorPose() const;
    glm::mat4 getEyeOffset(int eye) const;
    glm::mat4 getEyeProjection(int eye) const;

    QRect getDesirableApplicationGeometry();
    RunningScriptsWidget* getRunningScriptsWidget() { return _runningScriptsWidget; }

    Bookmarks* getBookmarks() const { return _bookmarks; }

    QString getScriptsLocation();
    void setScriptsLocation(const QString& scriptsLocation);

    bool canAcceptURL(const QString& url);
    bool acceptURL(const QString& url, bool defaultUpload = false);

    void setMaxOctreePacketsPerSecond(int maxOctreePPS);
    int getMaxOctreePacketsPerSecond();

    render::ScenePointer getMain3DScene() { return _main3DScene; }
    render::ScenePointer getMain3DScene() const { return _main3DScene; }
    render::EnginePointer getRenderEngine() { return _renderEngine; }
    gpu::ContextPointer getGPUContext() const { return _gpuContext; }

    const QRect& getMirrorViewRect() const { return _mirrorViewRect; }

    void updateMyAvatarLookAtPosition();
    AvatarUpdate* getAvatarUpdater() { return _avatarUpdate; }
    float getAvatarSimrate();
    void setAvatarSimrateSample(float sample);

    float getAverageSimsPerSecond();

signals:
    void scriptLocationChanged(const QString& newPath);

    void svoImportRequested(const QString& url);

    void checkBackgroundDownloads();
    void domainConnectionRefused(const QString& reason);

    void fullAvatarURLChanged(const QString& newValue, const QString& modelName);

    void beforeAboutToQuit();
    void activeDisplayPluginChanged();

public slots:
    QVector<EntityItemID> pasteEntities(float x, float y, float z);
    bool exportEntities(const QString& filename, const QVector<EntityItemID>& entityIDs);
    bool exportEntities(const QString& filename, float x, float y, float z, float scale);
    bool importEntities(const QString& url);

    void setLowVelocityFilter(bool lowVelocityFilter);
    void loadDialog();
    void loadScriptURLDialog();
    void toggleLogDialog();

    ScriptEngine* loadScript(const QString& scriptFilename = QString(), bool isUserLoaded = true,
        bool loadScriptFromEditor = false, bool activateMainWindow = false, bool reload = false);
    void stopAllScripts(bool restart = false);
    bool stopScript(const QString& scriptHash, bool restart = false);
    void reloadAllScripts();
    void reloadOneScript(const QString& scriptName);
    void loadDefaultScripts();
    void toggleRunningScriptsWidget();

    void showFriendsWindow();

    void packageModel();

    void openUrl(const QUrl& url);

    void setAvatarUpdateThreading();
    void setAvatarUpdateThreading(bool isThreaded);
    void setRawAvatarUpdateThreading();
    void setRawAvatarUpdateThreading(bool isThreaded);

    void resetSensors(bool andReload = false);
    void setActiveFaceTracker();
    
#ifdef HAVE_IVIEWHMD
    void setActiveEyeTracker();
    void calibrateEyeTracker1Point();
    void calibrateEyeTracker3Points();
    void calibrateEyeTracker5Points();
#endif

    void aboutApp();
    void showEditEntitiesHelp();

    void cameraMenuChanged();
    
    void reloadResourceCaches();

    void crashApplication();
    
    void rotationModeChanged();
    
    void runTests();
    
private slots:
    void clearDomainOctreeDetails();
    void checkFPS();
    void idle();
    void aboutToQuit();

    void handleScriptEngineLoaded(const QString& scriptFilename);
    void handleScriptLoadError(const QString& scriptFilename);

    void connectedToDomain(const QString& hostname);

    void audioMuteToggled();
    void faceTrackerMuteToggled();

    void activeChanged(Qt::ApplicationState state);
    
    void domainSettingsReceived(const QJsonObject& domainSettingsObject);
    void handleDomainConnectionDeniedPacket(QSharedPointer<NLPacket> packet);
    
    void notifyPacketVersionMismatch();
    
    void loadSettings();
    void saveSettings();
    
    void scriptFinished(const QString& scriptName);
    void saveScripts();
    void reloadScript(const QString& scriptName, bool isUserLoaded = true);
    
    bool acceptSnapshot(const QString& urlString);
    bool askToSetAvatarUrl(const QString& url);
    bool askToLoadScript(const QString& scriptFilenameOrURL);
    bool askToUploadAsset(const QString& asset);
    void modelUploadFinished(AssetUpload* upload, const QString& hash);
    
    void setSessionUUID(const QUuid& sessionUUID);
    void domainChanged(const QString& domainHostname);
    void updateWindowTitle();
    void nodeAdded(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
    void packetSent(quint64 length);
    void updateDisplayMode();
    void updateInputModes();
    
private:
    void initDisplay();
    void init();

    void cleanupBeforeQuit();
    
    void emptyLocalCache();

    void update(float deltaTime);

    void setPalmData(Hand* hand, UserInputMapper::PoseValue pose, float deltaTime, int index, float triggerValue);
    void emulateMouse(Hand* hand, float click, float shift, int index);

    // Various helper functions called during update()
    void updateLOD();
    void updateThreads(float deltaTime);
    void updateDialogs(float deltaTime);

    void queryOctree(NodeType_t serverType, PacketType packetType, NodeToJurisdictionMap& jurisdictions);
    void loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum);

    glm::vec3 getSunDirection();

    void renderRearViewMirror(RenderArgs* renderArgs, const QRect& region, bool billboard = false);

    int sendNackPackets();
    
    void takeSnapshot();
    
    MyAvatar* getMyAvatar() const;
    
    void checkSkeleton();
    
    void initializeAcceptedFiles();
    int getRenderAmbientLight() const;
    
    void displaySide(RenderArgs* renderArgs, Camera& whichCamera, bool selfAvatarOnly = false, bool billboard = false);
    
    bool importSVOFromURL(const QString& urlString);
    
    int processOctreeStats(NLPacket& packet, SharedNodePointer sendingNode);
    void trackIncomingOctreePacket(NLPacket& packet, SharedNodePointer sendingNode, bool wasStatsPacket);
    
    void resizeEvent(QResizeEvent* size);
    
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    
    void focusOutEvent(QFocusEvent* event);
    void focusInEvent(QFocusEvent* event);
    
    void mouseMoveEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mousePressEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mouseDoublePressEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID = 0);
    
    void touchBeginEvent(QTouchEvent* event);
    void touchEndEvent(QTouchEvent* event);
    void touchUpdateEvent(QTouchEvent* event);
    
    void wheelEvent(QWheelEvent* event);
    void dropEvent(QDropEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    

    bool _dependencyManagerIsSetup;

    OffscreenGlCanvas* _offscreenContext;
    DisplayPluginPointer _displayPlugin;
    InputPluginList _activeInputPlugins;

    MainWindow* _window;

    ToolWindow* _toolWindow;

    QUndoStack _undoStack;
    UndoStackScriptingInterface _undoStackScriptingInterface;

    // Frame Rate Measurement
    int _frameCount;
    float _fps;
    QElapsedTimer _timerStart;
    QElapsedTimer _lastTimeUpdated;

    ShapeManager _shapeManager;
    PhysicalEntitySimulation _entitySimulation;
    PhysicsEnginePointer _physicsEngine;

    EntityTreeRenderer _entities;
    EntityTreeRenderer _entityClipboardRenderer;
    EntityTreePointer _entityClipboard;

    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    ViewFrustum _lastQueriedViewFrustum; /// last view frustum used to query octree servers (voxels)
    ViewFrustum _displayViewFrustum;
    ViewFrustum _shadowViewFrustum;
    quint64 _lastQueriedTime;

    OctreeQuery _octreeQuery; // NodeData derived class for querying octee cells from octree servers

    KeyboardMouseDevice* _keyboardMouseDevice{ nullptr };   // Default input device, the good old keyboard mouse and maybe touchpad
    AvatarUpdate* _avatarUpdate {nullptr};
    SimpleMovingAverage _avatarSimsPerSecond {10};
    int _avatarSimsPerSecondReport {0};
    quint64 _lastAvatarSimsPerSecondUpdate {0};
    Camera _myCamera;                            // My view onto the world
    Camera _mirrorCamera;                        // Cammera for mirror view
    QRect _mirrorViewRect;

    Setting::Handle<bool> _firstRun;
    Setting::Handle<QString> _previousScriptLocation;
    Setting::Handle<QString> _scriptsLocationHandle;
    Setting::Handle<float> _fieldOfView;

    float _scaleMirror;
    float _rotateMirror;
    float _raiseMirror;

    Environment _environment;

    bool _lastMouseMoveWasSimulated;

    QSet<int> _keysPressed;

    bool _enableProcessOctreeThread;

    OctreePacketProcessor _octreeProcessor;
    EntityEditPacketSender _entityEditSender;

    StDev _idleLoopStdev;
    float _idleLoopMeasuredJitter;

    NodeToJurisdictionMap _entityServerJurisdictions;
    NodeToOctreeSceneStats _octreeServerSceneStats;

    ControllerScriptingInterface _controllerScriptingInterface;
    QPointer<LogDialog> _logDialog;
    QPointer<SnapshotShareDialog> _snapshotShareDialog;

    FileLogger* _logger;

    TouchEvent _lastTouchEvent;

    RunningScriptsWidget* _runningScriptsWidget;
    QHash<QString, ScriptEngine*> _scriptEnginesHash;
    bool _runningScriptsWidgetWasVisible;
    QString _scriptsLocation;

    quint64 _lastNackTime;
    quint64 _lastSendDownstreamAudioStats;
    
    bool _aboutToQuit;

    Bookmarks* _bookmarks;

    bool _notifiedPacketVersionMismatchThisDomain;

    QThread _settingsThread;
    QTimer _settingsTimer;
    
    GLCanvas* _glWidget{ nullptr };
    
    typedef bool (Application::* AcceptURLMethod)(const QString &);
    static const QHash<QString, AcceptURLMethod> _acceptedExtensions;

    QList<QString> _domainConnectionRefusals;
    glm::uvec2 _renderResolution;

    int _maxOctreePPS = DEFAULT_MAX_OCTREE_PPS;

    quint64 _lastFaceTrackerUpdate;

    render::ScenePointer _main3DScene{ new render::Scene() };
    render::EnginePointer _renderEngine{ new render::Engine() };
    gpu::ContextPointer _gpuContext; // initialized during window creation

    Overlays _overlays;
    ApplicationOverlay _applicationOverlay;
    ApplicationCompositor _compositor;
    OverlayConductor _overlayConductor;

    int _oldHandMouseX[2];
    int _oldHandMouseY[2];
    bool _oldHandLeftClick[2];
    bool _oldHandRightClick[2];

    DialogsManagerScriptingInterface* _dialogsManagerScriptingInterface = new DialogsManagerScriptingInterface();

    EntityItemID _keyboardFocusedItem;
    quint64 _lastAcceptedKeyPress = 0;

    SimpleMovingAverage _simsPerSecond{10};
    int _simsPerSecondReport = 0;
    quint64 _lastSimsPerSecondUpdate = 0;
    bool _isForeground = true; // starts out assumed to be in foreground
    bool _inPaint = false;
};

#endif // hifi_Application_h
