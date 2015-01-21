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

#include <MenuItemProperties.h>

class Settings;

class Menu : public QMenuBar {
    Q_OBJECT
public:
    static Menu* getInstance();
    
    void loadSettings();
    void saveSettings();
    
    QMenu* getMenu(const QString& menuName);

    void triggerOption(const QString& menuOption);
    QAction* getActionForOption(const QString& menuOption);
    
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
    
public slots:
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
    
private:
    static Menu* _instance;
    Menu();
    
    typedef void(*settingsAction)(Settings&, QAction&);
    static void loadAction(Settings& settings, QAction& action);
    static void saveAction(Settings& settings, QAction& action);
    void scanMenuBar(settingsAction modifySetting);
    void scanMenu(QMenu& menu, settingsAction modifySetting, Settings& settings);
    
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
    
    QAction* getActionFromName(const QString& menuName, QMenu* menu);
    QMenu* getSubMenuFromName(const QString& menuName, QMenu* menu);
    QMenu* getMenuParent(const QString& menuName, QString& finalMenuPart);
    
    QAction* getMenuAction(const QString& menuName);
    int findPositionOfMenuItem(QMenu* menu, const QString& searchMenuItem);
    int positionBeforeSeparatorIfNeeded(QMenu* menu, int requestedPosition);
    
    
    QHash<QString, QAction*> _actionHash;
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
    const QString AudioSourceInject = "Generated Audio";
    const QString AudioSourcePinkNoise = "Pink Noise";
    const QString AudioSourceSine440 = "Sine 440hz";
    const QString Avatars = "Avatars";
    const QString Bandwidth = "Bandwidth Display";
    const QString BandwidthDetails = "Bandwidth Details";
    const QString BlueSpeechSphere = "Blue Sphere While Speaking";
    const QString BookmarkLocation = "Bookmark Location";
    const QString Bookmarks = "Bookmarks";
    const QString CascadedShadows = "Cascaded";
    const QString CachesSize = "Caches Size";
    const QString Chat = "Chat...";
    const QString ChatCircling = "Chat Circling";
    const QString CollideAsRagdoll = "Collide With Self (Ragdoll)";
    const QString CollideWithAvatars = "Collide With Other Avatars";
    const QString CollideWithEnvironment = "Collide With World Boundaries";
    const QString Collisions = "Collisions";
    const QString Console = "Console...";
    const QString CopyAddress = "Copy Address to Clipboard";
    const QString CopyPath = "Copy Path to Clipboard";
    const QString ControlWithSpeech = "Control With Speech";
    const QString DeleteBookmark = "Delete Bookmark...";
    const QString DontRenderEntitiesAsScene = "Don't Render Entities as Scene";
    const QString DontDoPrecisionPicking = "Don't Do Precision Picking";
    const QString DecreaseAvatarSize = "Decrease Avatar Size";
    const QString DisableActivityLogger = "Disable Activity Logger";
    const QString DisableAutoAdjustLOD = "Disable Automatically Adjusting LOD";
    const QString DisableLightEntities = "Disable Light Entities";
    const QString DisableNackPackets = "Disable NACK Packets";
    const QString DisplayHands = "Show Hand Info";
    const QString DisplayHandTargets = "Show Hand Targets";
    const QString DisplayHermiteData = "Display Hermite Data";
    const QString DisplayModelBounds = "Display Model Bounds";
    const QString DisplayModelTriangles = "Display Model Triangles";
    const QString DisplayModelElementChildProxies = "Display Model Element Children";
    const QString DisplayModelElementProxy = "Display Model Element Bounds";
    const QString DisplayTimingDetails = "Display Timing Details";
    const QString DontFadeOnOctreeServerChanges = "Don't Fade In/Out on Octree Server Changes";
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
    const QString Fullscreen = "Fullscreen";
    const QString FullscreenMirror = "Fullscreen Mirror";
    const QString GlowWhenSpeaking = "Glow When Speaking";
    const QString NamesAboveHeads = "Names Above Heads";
    const QString GoToUser = "Go To User";
    const QString HMDTools = "HMD Tools";
    const QString IncreaseAvatarSize = "Increase Avatar Size";
    const QString KeyboardMotorControl = "Enable Keyboard Motor Control";
    const QString LeapMotionOnHMD = "Leap Motion on HMD";
    const QString LoadScript = "Open and Run Script File...";
    const QString LoadScriptURL = "Open and Run Script from URL...";
    const QString LoadRSSDKFile = "Load .rssdk file";
    const QString LodTools = "LOD Tools";
    const QString Login = "Login";
    const QString Log = "Log";
    const QString LowVelocityFilter = "Low Velocity Filter";
    const QString MetavoxelEditor = "Metavoxel Editor...";
    const QString Metavoxels = "Metavoxels";
    const QString Mirror = "Mirror";
    const QString MuteAudio = "Mute Microphone";
    const QString MuteEnvironment = "Mute Environment";
    const QString NetworkSimulator = "Network Simulator...";
    const QString NewVoxelCullingMode = "New Voxel Culling Mode";
    const QString ObeyEnvironmentalGravity = "Obey Environmental Gravity";
    const QString OctreeStats = "Voxel and Entity Statistics";
    const QString OffAxisProjection = "Off-Axis Projection";
    const QString OldVoxelCullingMode = "Old Voxel Culling Mode";
    const QString Pair = "Pair";
    const QString PipelineWarnings = "Log Render Pipeline Warnings";
    const QString Preferences = "Preferences...";
    const QString Quit =  "Quit";
    const QString ReloadAllScripts = "Reload All Scripts";
    const QString RenderBoundingCollisionShapes = "Show Bounding Collision Shapes";
    const QString RenderDualContourSurfaces = "Render Dual Contour Surfaces";
    const QString RenderFocusIndicator = "Show Eye Focus";
    const QString RenderHeadCollisionShapes = "Show Head Collision Shapes";
    const QString RenderHeightfields = "Render Heightfields";
    const QString RenderLookAtVectors = "Show Look-at Vectors";
    const QString RenderSkeletonCollisionShapes = "Show Skeleton Collision Shapes";
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
    const QString RenderAmbientLight = "Ambient Light";
    const QString RenderAmbientLightGlobal = "Global";
    const QString RenderAmbientLight0 = "OLD_TOWN_SQUARE";
    const QString RenderAmbientLight1 = "GRACE_CATHEDRAL";
    const QString RenderAmbientLight2 = "EUCALYPTUS_GROVE";
    const QString RenderAmbientLight3 = "ST_PETERS_BASILICA";
    const QString RenderAmbientLight4 = "UFFIZI_GALLERY";
    const QString RenderAmbientLight5 = "GALILEOS_TOMB";
    const QString RenderAmbientLight6 = "VINE_STREET_KITCHEN";
    const QString RenderAmbientLight7 = "BREEZEWAY";
    const QString RenderAmbientLight8 = "CAMPUS_SUNSET";
    const QString RenderAmbientLight9 = "FUNSTON_BEACH_SUNSET";
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
    const QString SuppressShortTimings = "Suppress Timings Less than 10ms";
    const QString TestPing = "Test Ping";
    const QString ToolWindow = "Tool Window";
    const QString TransmitterDrive = "Transmitter Drive";
    const QString TurnWithHead = "Turn using Head";
    const QString UploadAttachment = "Upload Attachment Model";
    const QString UploadEntity = "Upload Entity Model";
    const QString UploadHead = "Upload Head Model";
    const QString UploadSkeleton = "Upload Skeleton Model";
    const QString UserInterface = "User Interface";
    const QString Visage = "Visage";
    const QString Wireframe = "Wireframe";
}

#endif // hifi_Menu_h
