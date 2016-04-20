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

#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QStringList>

#include <QtGui/QImage>

#include <QtWidgets/QApplication>
#include <QtWidgets/QUndoStack>

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
#include <AbstractUriHandler.h>
#include <shared/RateCounter.h>

#include "avatar/MyAvatar.h"
#include "Bookmarks.h"
#include "Camera.h"
#include "FileLogger.h"
#include "gpu/Context.h"
#include "Menu.h"
#include "octree/OctreePacketProcessor.h"
#include "render/Engine.h"
#include "scripting/ControllerScriptingInterface.h"
#include "scripting/DialogsManagerScriptingInterface.h"
#include "ui/ApplicationOverlay.h"
#include "ui/AudioStatsDialog.h"
#include "ui/BandwidthDialog.h"
#include "ui/LodToolsDialog.h"
#include "ui/LogDialog.h"
#include "ui/OctreeStatsDialog.h"
#include "ui/OverlayConductor.h"
#include "ui/overlays/Overlays.h"
#include "UndoStackScriptingInterface.h"

class OffscreenGLCanvas;
class GLCanvas;
class FaceTracker;
class MainWindow;
class AssetUpload;
class CompositorHelper;

namespace controller {
    class StateController;
}

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

class Application : public QApplication, public AbstractViewStateInterface, public AbstractScriptingServicesInterface, public AbstractUriHandler {
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

    void postLambdaEvent(std::function<void()> f) override;

    QString getPreviousScriptLocation();
    void setPreviousScriptLocation(const QString& previousScriptLocation);

    void initializeGL();
    void initializeUi();
    void paintGL();
    void resizeGL();

    bool event(QEvent* event) override;
    bool eventFilter(QObject* object, QEvent* event) override;

    glm::uvec2 getCanvasSize() const;
    QRect getRenderingGeometry() const;

    glm::uvec2 getUiSize() const;
    QSize getDeviceSize() const;
    bool hasFocus() const;

    void showCursor(const QCursor& cursor);

    bool isThrottleRendering() const;

    Camera* getCamera() { return &_myCamera; }
    const Camera* getCamera() const { return &_myCamera; }
    // Represents the current view frustum of the avatar.
    ViewFrustum* getViewFrustum();
    const ViewFrustum* getViewFrustum() const;
    // Represents the view frustum of the current rendering pass,
    // which might be different from the viewFrustum, i.e. shadowmap
    // passes, mirror window passes, etc
    ViewFrustum* getDisplayViewFrustum();
    const ViewFrustum* getDisplayViewFrustum() const;
    ViewFrustum* getShadowViewFrustum() override { return &_shadowViewFrustum; }
    const OctreePacketProcessor& getOctreePacketProcessor() const { return _octreeProcessor; }
    EntityTreeRenderer* getEntities() const { return DependencyManager::get<EntityTreeRenderer>().data(); }
    QUndoStack* getUndoStack() { return &_undoStack; }
    MainWindow* getWindow() const { return _window; }
    EntityTreePointer getEntityClipboard() const { return _entityClipboard; }
    EntityTreeRenderer* getEntityClipboardRenderer() { return &_entityClipboardRenderer; }
    EntityEditPacketSender* getEntityEditPacketSender() { return &_entityEditSender; }

    ivec2 getMouse() const;

    FaceTracker* getActiveFaceTracker();
    FaceTracker* getSelectedFaceTracker();

    ApplicationOverlay& getApplicationOverlay() { return _applicationOverlay; }
    const ApplicationOverlay& getApplicationOverlay() const { return _applicationOverlay; }
    CompositorHelper& getApplicationCompositor() const;

    Overlays& getOverlays() { return _overlays; }

    bool isForeground() const { return _isForeground; }

    size_t getFrameCount() const { return _frameCount; }
    float getFps() const { return _frameCounter.rate(); }
    float getTargetFrameRate() const; // frames/second

    float getFieldOfView() { return _fieldOfView.get(); }
    void setFieldOfView(float fov);

    NodeToOctreeSceneStats* getOcteeSceneStats() { return &_octreeServerSceneStats; }

    virtual controller::ScriptingInterface* getControllerScriptingInterface() { return _controllerScriptingInterface; }
    virtual void registerScriptEngineWithApplicationServices(ScriptEngine* scriptEngine) override;

    virtual ViewFrustum* getCurrentViewFrustum() override { return getDisplayViewFrustum(); }
    virtual QThread* getMainThread() override { return thread(); }
    virtual PickRay computePickRay(float x, float y) const override;
    virtual glm::vec3 getAvatarPosition() const override;
    virtual qreal getDevicePixelRatio() override;

    void setActiveDisplayPlugin(const QString& pluginName);

    DisplayPlugin* getActiveDisplayPlugin();
    const DisplayPlugin* getActiveDisplayPlugin() const;

    FileLogger* getLogger() const { return _logger; }

    glm::vec2 getViewportDimensions() const;

    NodeToJurisdictionMap& getEntityServerJurisdictions() { return _entityServerJurisdictions; }

    float getRenderResolutionScale() const;

    bool isAboutToQuit() const { return _aboutToQuit; }

    // the isHMDMode is true whenever we use the interface from an HMD and not a standard flat display
    // rendering of several elements depend on that
    // TODO: carry that information on the Camera as a setting
    bool isHMDMode() const;
    glm::mat4 getHMDSensorPose() const;
    glm::mat4 getEyeOffset(int eye) const;
    glm::mat4 getEyeProjection(int eye) const;

    QRect getDesirableApplicationGeometry() const;
    Bookmarks* getBookmarks() const { return _bookmarks; }

    virtual bool canAcceptURL(const QString& url) const override;
    virtual bool acceptURL(const QString& url, bool defaultUpload = false) override;

    void setMaxOctreePacketsPerSecond(int maxOctreePPS);
    int getMaxOctreePacketsPerSecond() const;

    render::ScenePointer getMain3DScene() override { return _main3DScene; }
    render::ScenePointer getMain3DScene() const { return _main3DScene; }
    render::EnginePointer getRenderEngine() override { return _renderEngine; }
    gpu::ContextPointer getGPUContext() const { return _gpuContext; }

    virtual void pushPreRenderLambda(void* key, std::function<void()> func) override;

    const QRect& getMirrorViewRect() const { return _mirrorViewRect; }

    void updateMyAvatarLookAtPosition();

    float getAvatarSimrate() const { return _avatarSimCounter.rate(); }
    float getAverageSimsPerSecond() const { return _simCounter.rate(); }

signals:
    void svoImportRequested(const QString& url);

    void checkBackgroundDownloads();

    void fullAvatarURLChanged(const QString& newValue, const QString& modelName);

    void beforeAboutToQuit();
    void activeDisplayPluginChanged();

    void uploadRequest(QString path);

public slots:
    QVector<EntityItemID> pasteEntities(float x, float y, float z);
    bool exportEntities(const QString& filename, const QVector<EntityItemID>& entityIDs, const glm::vec3* givenOffset = nullptr);
    bool exportEntities(const QString& filename, float x, float y, float z, float scale);
    bool importEntities(const QString& url);

    static void setLowVelocityFilter(bool lowVelocityFilter);
    Q_INVOKABLE void loadDialog();
    Q_INVOKABLE void loadScriptURLDialog() const;
    void toggleLogDialog();
    void toggleRunningScriptsWidget() const;
    void toggleAssetServerWidget(QString filePath = "");

    void handleLocalServerConnection() const;
    void readArgumentsFromLocalSocket() const;

    static void packageModel();

    void openUrl(const QUrl& url) const;

    void resetSensors(bool andReload = false);
    void setActiveFaceTracker() const;
    void toggleSuppressDeadlockWatchdogStatus(bool checked);

#ifdef HAVE_IVIEWHMD
    void setActiveEyeTracker();
    void calibrateEyeTracker1Point();
    void calibrateEyeTracker3Points();
    void calibrateEyeTracker5Points();
#endif

    void aboutApp();
    static void showHelp();

    void cycleCamera();
    void cameraMenuChanged();
    void toggleOverlays();
    void setOverlaysVisible(bool visible);

    void resetPhysicsReadyInformation();

    void reloadResourceCaches();

    void updateHeartbeat() const;

    static void deadlockApplication();

    void rotationModeChanged() const;

    static void runTests();

private slots:
    void showDesktop();
    void clearDomainOctreeDetails();
    void idle(uint64_t now);
    void aboutToQuit();

    void resettingDomain();

    void audioMuteToggled() const;
    void faceTrackerMuteToggled();

    void activeChanged(Qt::ApplicationState state);

    void notifyPacketVersionMismatch();

    void loadSettings();
    void saveSettings() const;

    bool acceptSnapshot(const QString& urlString);
    bool askToSetAvatarUrl(const QString& url);
    bool askToLoadScript(const QString& scriptFilenameOrURL);

    bool askToWearAvatarAttachmentUrl(const QString& url);
    void displayAvatarAttachmentWarning(const QString& message) const;
    bool displayAvatarAttachmentConfirmationDialog(const QString& name) const;

    void setSessionUUID(const QUuid& sessionUUID) const;
    void domainChanged(const QString& domainHostname);
    void updateWindowTitle() const;
    void nodeAdded(SharedNodePointer node) const;
    void nodeActivated(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
    static void packetSent(quint64 length);
    void updateDisplayMode();
    void updateInputModes();

private:
    static void initDisplay();
    void init();

    void cleanupBeforeQuit();

    void update(float deltaTime);

    // Various helper functions called during update()
    void updateLOD() const;
    void updateThreads(float deltaTime);
    void updateDialogs(float deltaTime) const;

    void queryOctree(NodeType_t serverType, PacketType packetType, NodeToJurisdictionMap& jurisdictions, bool forceResend = false);
    static void loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum);

    glm::vec3 getSunDirection() const;

    void renderRearViewMirror(RenderArgs* renderArgs, const QRect& region);

    int sendNackPackets();

    void takeSnapshot();

    MyAvatar* getMyAvatar() const;

    void checkSkeleton() const;

    void initializeAcceptedFiles();

    void displaySide(RenderArgs* renderArgs, Camera& whichCamera, bool selfAvatarOnly = false);

    bool importSVOFromURL(const QString& urlString);

    bool nearbyEntitiesAreReadyForPhysics();
    int processOctreeStats(ReceivedMessage& message, SharedNodePointer sendingNode);
    void trackIncomingOctreePacket(ReceivedMessage& message, SharedNodePointer sendingNode, bool wasStatsPacket);

    void resizeEvent(QResizeEvent* size);

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void focusOutEvent(QFocusEvent* event);
    void focusInEvent(QFocusEvent* event);

    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseDoublePressEvent(QMouseEvent* event) const;
    void mouseReleaseEvent(QMouseEvent* event);

    void touchBeginEvent(QTouchEvent* event);
    void touchEndEvent(QTouchEvent* event);
    void touchUpdateEvent(QTouchEvent* event);

    void wheelEvent(QWheelEvent* event) const;
    void dropEvent(QDropEvent* event);
    static void dragEnterEvent(QDragEnterEvent* event);

    void maybeToggleMenuVisible(QMouseEvent* event) const;

    MainWindow* _window;
    QElapsedTimer& _sessionRunTimer;

    bool _previousSessionCrashed;

    OffscreenGLCanvas* _offscreenContext { nullptr };
    DisplayPluginPointer _displayPlugin;
    std::mutex _displayPluginLock;
    InputPluginList _activeInputPlugins;

    bool _activatingDisplayPlugin { false };
    QMap<gpu::TexturePointer, gpu::FramebufferPointer> _lockedFramebufferMap;

    QUndoStack _undoStack;
    UndoStackScriptingInterface _undoStackScriptingInterface;

    uint32_t _frameCount { 0 };

    // Frame Rate Measurement
    RateCounter<> _frameCounter;
    RateCounter<> _avatarSimCounter;
    RateCounter<> _simCounter;

    QElapsedTimer _timerStart;
    QElapsedTimer _lastTimeUpdated;

    ShapeManager _shapeManager;
    PhysicalEntitySimulation _entitySimulation;
    PhysicsEnginePointer _physicsEngine;

    EntityTreeRenderer _entityClipboardRenderer;
    EntityTreePointer _entityClipboard;

    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    ViewFrustum _lastQueriedViewFrustum; /// last view frustum used to query octree servers (voxels)
    ViewFrustum _displayViewFrustum;
    ViewFrustum _shadowViewFrustum;
    quint64 _lastQueriedTime;

    OctreeQuery _octreeQuery; // NodeData derived class for querying octee cells from octree servers

    std::shared_ptr<controller::StateController> _applicationStateDevice; // Default ApplicationDevice reflecting the state of different properties of the session
    std::shared_ptr<KeyboardMouseDevice> _keyboardMouseDevice;   // Default input device, the good old keyboard mouse and maybe touchpad
    SimpleMovingAverage _avatarSimsPerSecond {10};
    int _avatarSimsPerSecondReport {0};
    quint64 _lastAvatarSimsPerSecondUpdate {0};
    Camera _myCamera;                            // My view onto the world
    Camera _mirrorCamera;                        // Camera for mirror view
    QRect _mirrorViewRect;

    Setting::Handle<QString> _previousScriptLocation;
    Setting::Handle<float> _fieldOfView;

    float _scaleMirror;
    float _rotateMirror;
    float _raiseMirror;

    QSet<int> _keysPressed;

    bool _enableProcessOctreeThread;

    OctreePacketProcessor _octreeProcessor;
    EntityEditPacketSender _entityEditSender;

    StDev _idleLoopStdev;
    float _idleLoopMeasuredJitter;

    NodeToJurisdictionMap _entityServerJurisdictions;
    NodeToOctreeSceneStats _octreeServerSceneStats;
    ControllerScriptingInterface* _controllerScriptingInterface{ nullptr };
    QPointer<LogDialog> _logDialog;

    FileLogger* _logger;

    TouchEvent _lastTouchEvent;

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

    glm::uvec2 _renderResolution;

    int _maxOctreePPS = DEFAULT_MAX_OCTREE_PPS;

    quint64 _lastFaceTrackerUpdate;

    render::ScenePointer _main3DScene{ new render::Scene(glm::vec3(-0.5f * (float)TREE_SCALE), (float)TREE_SCALE) };
    render::EnginePointer _renderEngine{ new render::Engine() };
    gpu::ContextPointer _gpuContext; // initialized during window creation

    Overlays _overlays;
    ApplicationOverlay _applicationOverlay;
    OverlayConductor _overlayConductor;

    DialogsManagerScriptingInterface* _dialogsManagerScriptingInterface = new DialogsManagerScriptingInterface();

    EntityItemID _keyboardFocusedItem;
    quint64 _lastAcceptedKeyPress = 0;
    bool _isForeground = true; // starts out assumed to be in foreground
    bool _inPaint = false;
    bool _isGLInitialized { false };
    bool _physicsEnabled { false };

    bool _reticleClickPressed { false };

    int _avatarAttachmentRequest = 0;

    bool _settingsLoaded { false };
    QTimer* _idleTimer { nullptr };

    bool _fakedMouseEvent { false };

    void checkChangeCursor();
    mutable QMutex _changeCursorLock { QMutex::Recursive };
    QCursor _desiredCursor{ Qt::BlankCursor };
    bool _cursorNeedsChanging { false };

    QThread* _deadlockWatchdogThread;

    std::map<void*, std::function<void()>> _preRenderLambdas;
    std::mutex _preRenderLambdasLock;

    std::atomic<uint32_t> _fullSceneReceivedCounter { 0 }; // how many times have we received a full-scene octree stats packet
    uint32_t _fullSceneCounterAtLastPhysicsCheck { 0 }; // _fullSceneReceivedCounter last time we checked physics ready
    uint32_t _nearbyEntitiesCountAtLastPhysicsCheck { 0 }; // how many in-range entities last time we checked physics ready
    uint32_t _nearbyEntitiesStabilityCount { 0 }; // how many times has _nearbyEntitiesCountAtLastPhysicsCheck been the same
    quint64 _lastPhysicsCheckTime { 0 }; // when did we last check to see if physics was ready

    bool _keyboardDeviceHasFocus { true };

    bool _recentlyClearedDomain { false };

    QString _returnFromFullScreenMirrorTo;
};

#endif // hifi_Application_h
