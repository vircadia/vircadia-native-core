//
//  Menu.h
//  interface/src
//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Menu_h
#define hifi_Menu_h

#include <QDir>
#include <QMenuBar>
#include <QHash>
#include <QKeySequence>
#include <QPointer>
#include <QStandardPaths>

#include <EventTypes.h>
#include <MenuItemProperties.h>
#include <OctreeConstants.h>

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif

#include "ui/AddressBarDialog.h"
#include "ui/ChatWindow.h"
#include "ui/DataWebDialog.h"
#include "ui/JSConsole.h"
#include "ui/LoginDialog.h"
#include "ui/PreferencesDialog.h"
#include "ui/ScriptEditorWindow.h"

const float ADJUST_LOD_DOWN_FPS = 40.0;
const float ADJUST_LOD_UP_FPS = 55.0;
const float DEFAULT_ADJUST_AVATAR_LOD_DOWN_FPS = 30.0f;

const quint64 ADJUST_LOD_DOWN_DELAY = 1000 * 1000 * 5;
const quint64 ADJUST_LOD_UP_DELAY = ADJUST_LOD_DOWN_DELAY * 2;

const float ADJUST_LOD_DOWN_BY = 0.9f;
const float ADJUST_LOD_UP_BY = 1.1f;

const float ADJUST_LOD_MIN_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE * 0.25f;
const float ADJUST_LOD_MAX_SIZE_SCALE = DEFAULT_OCTREE_SIZE_SCALE;

const float MINIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER = 0.1f;
const float MAXIMUM_AVATAR_LOD_DISTANCE_MULTIPLIER = 15.0f;

const QString SETTINGS_ADDRESS_KEY = "address";

enum FrustumDrawMode {
    FRUSTUM_DRAW_MODE_ALL,
    FRUSTUM_DRAW_MODE_VECTORS,
    FRUSTUM_DRAW_MODE_PLANES,
    FRUSTUM_DRAW_MODE_NEAR_PLANE,
    FRUSTUM_DRAW_MODE_FAR_PLANE,
    FRUSTUM_DRAW_MODE_KEYHOLE,
    FRUSTUM_DRAW_MODE_COUNT
};

struct ViewFrustumOffset {
    float yaw;
    float pitch;
    float roll;
    float distance;
    float up;
};

class QSettings;

class AnimationsDialog;
class AttachmentsDialog;
class BandwidthDialog;
class LodToolsDialog;
class MetavoxelEditor;
class MetavoxelNetworkSimulator;
class ChatWindow;
class OctreeStatsDialog;
class MenuItemProperties;

class Menu : public QMenuBar {
    Q_OBJECT
public:
    static Menu* getInstance();
    ~Menu();

    void triggerOption(const QString& menuOption);
    QAction* getActionForOption(const QString& menuOption);

    const InboundAudioStream::Settings& getReceivedAudioStreamSettings() const { return _receivedAudioStreamSettings; }
    void setReceivedAudioStreamSettings(const InboundAudioStream::Settings& receivedAudioStreamSettings) { _receivedAudioStreamSettings = receivedAudioStreamSettings; }
    float getFieldOfView() const { return _fieldOfView; }
    void setFieldOfView(float fieldOfView) { _fieldOfView = fieldOfView; bumpSettings(); }
    float getRealWorldFieldOfView() const { return _realWorldFieldOfView; }
    void setRealWorldFieldOfView(float realWorldFieldOfView) { _realWorldFieldOfView = realWorldFieldOfView; bumpSettings(); }
    float getOculusUIAngularSize() const { return _oculusUIAngularSize; }
    void setOculusUIAngularSize(float oculusUIAngularSize) { _oculusUIAngularSize = oculusUIAngularSize; bumpSettings(); }
    float getSixenseReticleMoveSpeed() const { return _sixenseReticleMoveSpeed; }
    void setSixenseReticleMoveSpeed(float sixenseReticleMoveSpeed) { _sixenseReticleMoveSpeed = sixenseReticleMoveSpeed; bumpSettings(); }
    bool getInvertSixenseButtons() const { return _invertSixenseButtons; }
    void setInvertSixenseButtons(bool invertSixenseButtons) { _invertSixenseButtons = invertSixenseButtons; bumpSettings(); }

    float getFaceshiftEyeDeflection() const { return _faceshiftEyeDeflection; }
    void setFaceshiftEyeDeflection(float faceshiftEyeDeflection) { _faceshiftEyeDeflection = faceshiftEyeDeflection; bumpSettings(); }
    const QString& getFaceshiftHostname() const { return _faceshiftHostname; }
    void setFaceshiftHostname(const QString& hostname) { _faceshiftHostname = hostname; bumpSettings(); }
    QString getSnapshotsLocation() const;
    void setSnapshotsLocation(QString snapshotsLocation) { _snapshotsLocation = snapshotsLocation; bumpSettings(); }

    const QString& getScriptsLocation() const { return _scriptsLocation; }
    void setScriptsLocation(const QString& scriptsLocation);

    BandwidthDialog* getBandwidthDialog() const { return _bandwidthDialog; }
    FrustumDrawMode getFrustumDrawMode() const { return _frustumDrawMode; }
    ViewFrustumOffset getViewFrustumOffset() const { return _viewFrustumOffset; }
    OctreeStatsDialog* getOctreeStatsDialog() const { return _octreeStatsDialog; }
    LodToolsDialog* getLodToolsDialog() const { return _lodToolsDialog; }
    int getMaxVoxels() const { return _maxVoxels; }
    QAction* getUseVoxelShader() const { return _useVoxelShader; }

    bool getShadowsEnabled() const;

    void handleViewFrustumOffsetKeyModifier(int key);

    // User Tweakable LOD Items
    QString getLODFeedbackText();
    void autoAdjustLOD(float currentFPS);
    void resetLODAdjust();
    void setVoxelSizeScale(float sizeScale);
    float getVoxelSizeScale() const { return _voxelSizeScale; }
    void setAutomaticAvatarLOD(bool automaticAvatarLOD) { _automaticAvatarLOD = automaticAvatarLOD; bumpSettings(); }
    bool getAutomaticAvatarLOD() const { return _automaticAvatarLOD; }
    void setAvatarLODDecreaseFPS(float avatarLODDecreaseFPS) { _avatarLODDecreaseFPS = avatarLODDecreaseFPS; bumpSettings(); }
    float getAvatarLODDecreaseFPS() const { return _avatarLODDecreaseFPS; }
    void setAvatarLODIncreaseFPS(float avatarLODIncreaseFPS) { _avatarLODIncreaseFPS = avatarLODIncreaseFPS; bumpSettings(); }
    float getAvatarLODIncreaseFPS() const { return _avatarLODIncreaseFPS; }
    void setAvatarLODDistanceMultiplier(float multiplier) { _avatarLODDistanceMultiplier = multiplier; bumpSettings(); }
    float getAvatarLODDistanceMultiplier() const { return _avatarLODDistanceMultiplier; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust);
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }

    bool shouldRenderMesh(float largestDimension, float distanceToCamera);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    SpeechRecognizer* getSpeechRecognizer() { return &_speechRecognizer; }
#endif

    // User Tweakable PPS from Voxel Server
    int getMaxVoxelPacketsPerSecond() const { return _maxVoxelPacketsPerSecond; }
    void setMaxVoxelPacketsPerSecond(int maxVoxelPacketsPerSecond) { _maxVoxelPacketsPerSecond = maxVoxelPacketsPerSecond; bumpSettings(); }

    QAction* addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                           const QString& actionName,
                                           const QKeySequence& shortcut = 0,
                                           const QObject* receiver = NULL,
                                           const char* member = NULL,
                                           QAction::MenuRole role = QAction::NoRole,
                                           int menuItemLocation = UNSPECIFIED_POSITION);
    QAction* addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                           QAction* action,
                                           const QString& actionName = QString(),
                                           const QKeySequence& shortcut = 0,
                                           QAction::MenuRole role = QAction::NoRole,
                                           int menuItemLocation = UNSPECIFIED_POSITION);

    void removeAction(QMenu* menu, const QString& actionName);
    
    const QByteArray& getWalletPrivateKey() const { return _walletPrivateKey; }

signals:
    void scriptLocationChanged(const QString& newPath);

public slots:

    void clearLoginDialogDisplayedFlag();
    void loginForCurrentDomain();
    void showLoginForCurrentDomain();
    void bandwidthDetails();
    void octreeStatsDetails();
    void lodTools();
    void loadSettings(QSettings* settings = NULL);
    void saveSettings(QSettings* settings = NULL);
    void importSettings();
    void exportSettings();
    void toggleAddressBar();
    void pasteToVoxel();

    void toggleLoginMenuItem();
    void toggleSixense(bool shouldEnable);

    QMenu* addMenu(const QString& menuName);
    void removeMenu(const QString& menuName);
    bool menuExists(const QString& menuName);
    void addSeparator(const QString& menuName, const QString& separatorName);
    void removeSeparator(const QString& menuName, const QString& separatorName);
    void addMenuItem(const MenuItemProperties& properties);
    void removeMenuItem(const QString& menuName, const QString& menuitem);
    bool menuItemExists(const QString& menuName, const QString& menuitem);
    bool isOptionChecked(const QString& menuOption) const;
    void setIsOptionChecked(const QString& menuOption, bool isChecked);

private slots:
    void aboutApp();
    void showEditEntitiesHelp();
    void bumpSettings();
    void editPreferences();
    void editAttachments();
    void editAnimations();
    void changePrivateKey();
    void nameLocation();
    void toggleLocationList();
    void bandwidthDetailsClosed();
    void octreeStatsDetailsClosed();
    void lodToolsClosed();
    void cycleFrustumRenderMode();
    void runTests();
    void showMetavoxelEditor();
    void showMetavoxelNetworkSimulator();
    void showScriptEditor();
    void showChat();
    void toggleConsole();
    void toggleToolWindow();
    void toggleChat();
    void audioMuteToggled();
    void displayNameLocationResponse(const QString& errorString);
    void displayAddressOfflineMessage();
    void displayAddressNotFoundMessage();
    void muteEnvironment();
    void changeVSync();

private:
    static Menu* _instance;

    Menu();

    typedef void(*settingsAction)(QSettings*, QAction*);
    static void loadAction(QSettings* set, QAction* action);
    static void saveAction(QSettings* set, QAction* action);
    void scanMenuBar(settingsAction modifySetting, QSettings* set);
    void scanMenu(QMenu* menu, settingsAction modifySetting, QSettings* set);

    /// helper method to have separators with labels that are also compatible with OS X
    void addDisabledActionAndSeparator(QMenu* destinationMenu, const QString& actionName,
                                                int menuItemLocation = UNSPECIFIED_POSITION);

    QAction* addCheckableActionToQMenuAndActionHash(QMenu* destinationMenu,
                                                    const QString& actionName,
                                                    const QKeySequence& shortcut = 0,
                                                    const bool checked = false,
                                                    const QObject* receiver = NULL,
                                                    const char* member = NULL,
                                                    int menuItemLocation = UNSPECIFIED_POSITION);

    void updateFrustumRenderModeAction();

    QAction* getActionFromName(const QString& menuName, QMenu* menu);
    QMenu* getSubMenuFromName(const QString& menuName, QMenu* menu);
    QMenu* getMenuParent(const QString& menuName, QString& finalMenuPart);

    QAction* getMenuAction(const QString& menuName);
    int findPositionOfMenuItem(QMenu* menu, const QString& searchMenuItem);
    int positionBeforeSeparatorIfNeeded(QMenu* menu, int requestedPosition);
    QMenu* getMenu(const QString& menuName);


    QHash<QString, QAction*> _actionHash;
    InboundAudioStream::Settings _receivedAudioStreamSettings;
    BandwidthDialog* _bandwidthDialog;
    float _fieldOfView; /// in Degrees, doesn't apply to HMD like Oculus
    float _realWorldFieldOfView;   //  The actual FOV set by the user's monitor size and view distance
    float _faceshiftEyeDeflection;
    QString _faceshiftHostname;
    FrustumDrawMode _frustumDrawMode;
    ViewFrustumOffset _viewFrustumOffset;
    QPointer<MetavoxelEditor> _MetavoxelEditor;
    QPointer<MetavoxelNetworkSimulator> _metavoxelNetworkSimulator;
    QPointer<ScriptEditorWindow> _ScriptEditor;
    QPointer<ChatWindow> _chatWindow;
    QDialog* _jsConsole;
    OctreeStatsDialog* _octreeStatsDialog;
    LodToolsDialog* _lodToolsDialog;
    QPointer<DataWebDialog> _newLocationDialog;
    QPointer<DataWebDialog> _userLocationsDialog;
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    SpeechRecognizer _speechRecognizer;
#endif
    int _maxVoxels;
    float _voxelSizeScale;
    float _oculusUIAngularSize;
    float _sixenseReticleMoveSpeed;
    bool _invertSixenseButtons;
    bool _automaticAvatarLOD;
    float _avatarLODDecreaseFPS;
    float _avatarLODIncreaseFPS;
    float _avatarLODDistanceMultiplier;
    int _boundaryLevelAdjust;
    QAction* _useVoxelShader;
    int _maxVoxelPacketsPerSecond;
    QString replaceLastOccurrence(QChar search, QChar replace, QString string);
    quint64 _lastAdjust;
    quint64 _lastAvatarDetailDrop;
    SimpleMovingAverage _fpsAverage;
    SimpleMovingAverage _fastFPSAverage;
    QAction* _loginAction;
    QPointer<PreferencesDialog> _preferencesDialog;
    QPointer<AttachmentsDialog> _attachmentsDialog;
    QPointer<AnimationsDialog> _animationsDialog;
    QPointer<AddressBarDialog> _addressBarDialog;
    QPointer<LoginDialog> _loginDialog;
    bool _hasLoginDialogDisplayed;
    QAction* _chatAction;
    QString _snapshotsLocation;
    QString _scriptsLocation;
    QByteArray _walletPrivateKey;
    
    bool _shouldRenderTableNeedsRebuilding;
    QMap<float, float> _shouldRenderTable;

};

namespace MenuOption {
    const QString AboutApp = "About Interface";
    const QString AddressBar = "Show Address Bar";
    const QString AlignForearmsWithWrists = "Align Forearms with Wrists";
    const QString AlternateIK = "Alternate IK";
    const QString AmbientOcclusion = "Ambient Occlusion";
    const QString Animations = "Animations...";
    const QString Atmosphere = "Atmosphere";
    const QString Attachments = "Attachments...";
    const QString AudioNoiseReduction = "Audio Noise Reduction";
    const QString AudioScope = "Show Scope";
    const QString AudioScopeFiftyFrames = "Fifty";
    const QString AudioScopeFiveFrames = "Five";
    const QString AudioScopeFrames = "Display Frames";
    const QString AudioScopePause = "Pause Scope";
    const QString AudioScopeTwentyFrames = "Twenty";
    const QString AudioStats = "Audio Stats";
    const QString AudioStatsShowInjectedStreams = "Audio Stats Show Injected Streams";
    const QString AudioSpatialProcessingAlternateDistanceAttenuate = "Alternate distance attenuation";
    const QString AudioSpatialProcessing = "Audio Spatial Processing";
    const QString AudioSpatialProcessingDontDistanceAttenuate = "Don't calculate distance attenuation";
    const QString AudioSpatialProcessingHeadOriented = "Head Oriented";
    const QString AudioSpatialProcessingIncludeOriginal = "Includes Network Original";
    const QString AudioSpatialProcessingPreDelay = "Add Pre-Delay";
    const QString AudioSpatialProcessingProcessLocalAudio = "Process Local Audio";
    const QString AudioSpatialProcessingRenderPaths = "Render Paths";
    const QString AudioSpatialProcessingSeparateEars = "Separate Ears";
    const QString AudioSpatialProcessingSlightlyRandomSurfaces = "Slightly Random Surfaces";
    const QString AudioSpatialProcessingStereoSource = "Stereo Source";
    const QString AudioSpatialProcessingWithDiffusions = "With Diffusions";
    const QString AudioSourceInject = "Generated Audio";
    const QString AudioSourcePinkNoise = "Pink Noise";
    const QString AudioSourceSine440 = "Sine 440hz";
    const QString Avatars = "Avatars";
    const QString Bandwidth = "Bandwidth Display";
    const QString BandwidthDetails = "Bandwidth Details";
    const QString BlueSpeechSphere = "Blue Sphere While Speaking";
    const QString CascadedShadows = "Cascaded";
    const QString Chat = "Chat...";
    const QString ChatCircling = "Chat Circling";
    const QString CollideAsRagdoll = "Collide With Self (Ragdoll)";
    const QString CollideWithAvatars = "Collide With Other Avatars";
    const QString CollideWithEnvironment = "Collide With World Boundaries";
    const QString CollideWithVoxels = "Collide With Voxels";
    const QString Collisions = "Collisions";
    const QString Console = "Console...";
    const QString ControlWithSpeech = "Control With Speech";
    const QString DontCullOutOfViewMeshParts = "Don't Cull Out Of View Mesh Parts";
    const QString DontCullTooSmallMeshParts = "Don't Cull Too Small Mesh Parts";
    const QString DontReduceMaterialSwitches = "Don't Attempt to Reduce Material Switches";
    const QString DecreaseAvatarSize = "Decrease Avatar Size";
    const QString DecreaseVoxelSize = "Decrease Voxel Size";
    const QString DisableActivityLogger = "Disable Activity Logger";
    const QString DisableAutoAdjustLOD = "Disable Automatically Adjusting LOD";
    const QString DisableLightEntities = "Disable Light Entities";
    const QString DisableNackPackets = "Disable NACK Packets";
    const QString DisplayFrustum = "Display Frustum";
    const QString DisplayHands = "Show Hand Info";
    const QString DisplayHandTargets = "Show Hand Targets";
    const QString DisplayHermiteData = "Display Hermite Data";
    const QString DisplayModelBounds = "Display Model Bounds";
    const QString DisplayModelElementChildProxies = "Display Model Element Children";
    const QString DisplayModelElementProxy = "Display Model Element Bounds";
    const QString DisplayTimingDetails = "Display Timing Details";
    const QString DontFadeOnVoxelServerChanges = "Don't Fade In/Out on Voxel Server Changes";
    const QString EchoLocalAudio = "Echo Local Audio";
    const QString EchoServerAudio = "Echo Server Audio";
    const QString EditEntitiesHelp = "Edit Entities Help...";
    const QString Enable3DTVMode = "Enable 3DTV Mode";
    const QString EnableGlowEffect = "Enable Glow Effect (Warning: Poor Oculus Performance)";
    const QString EnableVRMode = "Enable VR Mode";
    const QString Entities = "Entities";
    const QString ExpandMyAvatarSimulateTiming = "Expand /myAvatar/simulation";
    const QString ExpandMyAvatarTiming = "Expand /myAvatar";
    const QString ExpandOtherAvatarTiming = "Expand /otherAvatar";
    const QString ExpandPaintGLTiming = "Expand /paintGL";
    const QString ExpandUpdateTiming = "Expand /update";
    const QString Faceshift = "Faceshift";
    const QString FilterSixense = "Smooth Sixense Movement";
    const QString FirstPerson = "First Person";
    const QString FrameTimer = "Show Timer";
    const QString FrustumRenderMode = "Render Mode";
    const QString Fullscreen = "Fullscreen";
    const QString FullscreenMirror = "Fullscreen Mirror";
    const QString GlowWhenSpeaking = "Glow When Speaking";
    const QString NamesAboveHeads = "Names Above Heads";
    const QString GoToUser = "Go To User";
    const QString HeadMouse = "Head Mouse";
    const QString IncreaseAvatarSize = "Increase Avatar Size";
    const QString IncreaseVoxelSize = "Increase Voxel Size";
    const QString KeyboardMotorControl = "Enable Keyboard Motor Control";
    const QString LeapMotionOnHMD = "Leap Motion on HMD";
    const QString LoadScript = "Open and Run Script File...";
    const QString LoadScriptURL = "Open and Run Script from URL...";
    const QString LodTools = "LOD Tools";
    const QString Login = "Login";
    const QString Log = "Log";
    const QString Logout = "Logout";
    const QString LowVelocityFilter = "Low Velocity Filter";
    const QString MetavoxelEditor = "Metavoxel Editor...";
    const QString Metavoxels = "Metavoxels";
    const QString Mirror = "Mirror";
    const QString MuteAudio = "Mute Microphone";
    const QString MuteEnvironment = "Mute Environment";
    const QString MyLocations = "My Locations...";
    const QString NameLocation = "Name this location";
    const QString NetworkSimulator = "Network Simulator...";
    const QString NewVoxelCullingMode = "New Voxel Culling Mode";
    const QString ObeyEnvironmentalGravity = "Obey Environmental Gravity";
    const QString OctreeStats = "Voxel and Entity Statistics";
    const QString OffAxisProjection = "Off-Axis Projection";
    const QString OldVoxelCullingMode = "Old Voxel Culling Mode";
    const QString Pair = "Pair";
    const QString PasteToVoxel = "Paste to Voxel...";
    const QString PipelineWarnings = "Log Render Pipeline Warnings";
    const QString Preferences = "Preferences...";
    const QString Quit =  "Quit";
    const QString ReloadAllScripts = "Reload All Scripts";
    const QString RenderBoundingCollisionShapes = "Show Bounding Collision Shapes";
    const QString RenderDualContourSurfaces = "Render Dual Contour Surfaces";
    const QString RenderEntitiesAsScene = "Render Entities as Scene";
    const QString RenderFocusIndicator = "Show Eye Focus";
    const QString RenderHeadCollisionShapes = "Show Head Collision Shapes";
    const QString RenderLookAtVectors = "Show Look-at Vectors";
    const QString RenderSkeletonCollisionShapes = "Show Skeleton Collision Shapes";
    const QString RenderSpanners = "Render Spanners";
    const QString RenderTargetFramerate = "Framerate";
    const QString RenderTargetFramerateUnlimited = "Unlimited";
    const QString RenderTargetFramerate60 = "60";
    const QString RenderTargetFramerate50 = "50";
    const QString RenderTargetFramerate40 = "40";
    const QString RenderTargetFramerate30 = "30";
    const QString RenderTargetFramerateVSyncOn = "V-Sync On";

    const QString RenderResolution = "Scale Resolution";
    const QString RenderResolutionOne = "1";
    const QString RenderResolutionTwoThird = "2/3";
    const QString RenderResolutionHalf = "1/2";
    const QString RenderResolutionThird = "1/3";
    const QString RenderResolutionQuarter = "1/4";
    const QString ResetAvatarSize = "Reset Avatar Size";
    const QString ResetSensors = "Reset Sensors";
    const QString RunningScripts = "Running Scripts";
    const QString RunTimingTests = "Run Timing Tests";
    const QString ScriptEditor = "Script Editor...";
    const QString ScriptedMotorControl = "Enable Scripted Motor Control";
    const QString SettingsExport = "Export Settings";
    const QString SettingsImport = "Import Settings";
    const QString ShowBordersEntityNodes = "Show Entity Nodes";
    const QString ShowBordersVoxelNodes = "Show Voxel Nodes";
    const QString ShowIKConstraints = "Show IK Constraints";
    const QString SimpleShadows = "Simple";
    const QString SixenseEnabled = "Enable Hydra Support";
    const QString SixenseMouseInput = "Enable Sixense Mouse Input";
    const QString SixenseLasers = "Enable Sixense UI Lasers";
    const QString StandOnNearbyFloors = "Stand on nearby floors";
    const QString ShiftHipsForIdleAnimations = "Shift hips for idle animations";
    const QString Stars = "Stars";
    const QString Stats = "Stats";
    const QString StereoAudio = "Stereo Audio";
    const QString StopAllScripts = "Stop All Scripts";
    const QString StringHair = "String Hair";
    const QString SuppressShortTimings = "Suppress Timings Less than 10ms";
    const QString TestPing = "Test Ping";
    const QString ToolWindow = "Tool Window";
    const QString TransmitterDrive = "Transmitter Drive";
    const QString TurnWithHead = "Turn using Head";
    const QString UploadAttachment = "Upload Attachment Model";
    const QString UploadHead = "Upload Head Model";
    const QString UploadSkeleton = "Upload Skeleton Model";
    const QString UserInterface = "User Interface";
    const QString Visage = "Visage";
    const QString VoxelMode = "Cycle Voxel Mode";
    const QString Voxels = "Voxels";
    const QString VoxelTextures = "Voxel Textures";
    const QString WalletPrivateKey = "Wallet Private Key...";
    const QString Wireframe = "Wireframe";
}

void sendFakeEnterEvent();

#endif // hifi_Menu_h
