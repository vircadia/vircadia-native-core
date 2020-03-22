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

#include <ui/Menu.h>

#ifndef hifi_Menu_h
#define hifi_Menu_h

class MenuItemProperties;

class Menu : public ui::Menu {
    Q_OBJECT

public:
    static Menu* getInstance();
    Menu();
    Q_INVOKABLE void addMenuItem(const MenuItemProperties& properties);
};

namespace MenuOption {
    const QString AddRemoveFriends = QT_TRANSLATE_NOOP("MenuOption", "Add/Remove Friends...");
    const QString AddressBar = QT_TRANSLATE_NOOP("MenuOption", "Show Address Bar");
    const QString Animations = QT_TRANSLATE_NOOP("MenuOption", "Animations...");
    const QString AnimDebugDrawAnimPose = QT_TRANSLATE_NOOP("MenuOption", "Debug Draw Animation");
    const QString AnimDebugDrawBaseOfSupport = QT_TRANSLATE_NOOP("MenuOption", "Debug Draw Base of Support");
    const QString AnimDebugDrawDefaultPose = QT_TRANSLATE_NOOP("MenuOption", "Debug Draw Default Pose");
    const QString AnimDebugDrawPosition = QT_TRANSLATE_NOOP("MenuOption", "Debug Draw Position");
    const QString AnimDebugDrawOtherSkeletons = QT_TRANSLATE_NOOP("MenuOption", "Debug Draw Other Skeletons");
    const QString AskToResetSettings = QT_TRANSLATE_NOOP("MenuOption", "Ask To Reset Settings on Start");
    const QString AssetMigration = QT_TRANSLATE_NOOP("MenuOption", "ATP Asset Migration");
    const QString AssetServer = QT_TRANSLATE_NOOP("MenuOption", "Asset Browser");
    const QString AudioScope = QT_TRANSLATE_NOOP("MenuOption", "Show Scope");
    const QString AudioScopeFiftyFrames = QT_TRANSLATE_NOOP("MenuOption", "Fifty");
    const QString AudioScopeFiveFrames = QT_TRANSLATE_NOOP("MenuOption", "Five");
    const QString AudioScopeFrames = QT_TRANSLATE_NOOP("MenuOption", "Display Frames");
    const QString AudioScopePause = QT_TRANSLATE_NOOP("MenuOption", "Pause Scope");
    const QString AudioScopeTwentyFrames = QT_TRANSLATE_NOOP("MenuOption", "Twenty");
    const QString AudioStatsShowInjectedStreams = QT_TRANSLATE_NOOP("MenuOption", "Audio Stats Show Injected Streams");
    const QString AutoMuteAudio = QT_TRANSLATE_NOOP("MenuOption", "Auto Mute Microphone");
    const QString AvatarReceiveStats = QT_TRANSLATE_NOOP("MenuOption", "Show Receive Stats");
    const QString AvatarBookmarks = QT_TRANSLATE_NOOP("MenuOption", "Avatar Bookmarks");
    const QString AvatarPackager = QT_TRANSLATE_NOOP("MenuOption", "Avatar Packager");
    const QString Back = QT_TRANSLATE_NOOP("MenuOption", "Back");
    const QString BinaryEyelidControl = QT_TRANSLATE_NOOP("MenuOption", "Binary Eyelid Control");
    const QString BookmarkAvatar = QT_TRANSLATE_NOOP("MenuOption", "Bookmark Avatar");
    const QString BookmarkAvatarEntities = QT_TRANSLATE_NOOP("MenuOption", "Bookmark Avatar Entities");
    const QString BookmarkLocation = QT_TRANSLATE_NOOP("MenuOption", "Bookmark Location");
    const QString CalibrateCamera = QT_TRANSLATE_NOOP("MenuOption", "Calibrate Camera");
    const QString CenterPlayerInView = QT_TRANSLATE_NOOP("MenuOption", "Center Player In View");
    const QString Chat = QT_TRANSLATE_NOOP("MenuOption", "Chat...");
    const QString ClearDiskCache = QT_TRANSLATE_NOOP("MenuOption", "Clear Disk Cache");
    const QString Collisions = QT_TRANSLATE_NOOP("MenuOption", "Collisions");
    const QString Connexion = QT_TRANSLATE_NOOP("MenuOption", "Activate 3D Connexion Devices");
    const QString Console = QT_TRANSLATE_NOOP("MenuOption", "Console...");
    const QString ControlWithSpeech = QT_TRANSLATE_NOOP("MenuOption", "Enable Speech Control API");
    const QString CopyAddress = QT_TRANSLATE_NOOP("MenuOption", "Copy Address to Clipboard");
    const QString CopyPath = QT_TRANSLATE_NOOP("MenuOption", "Copy Path to Clipboard");
    const QString CoupleEyelids = QT_TRANSLATE_NOOP("MenuOption", "Couple Eyelids");
    const QString CrashPureVirtualFunction = QT_TRANSLATE_NOOP("MenuOption", "Pure Virtual Function Call");
    const QString CrashPureVirtualFunctionThreaded = QT_TRANSLATE_NOOP("MenuOption", "Pure Virtual Function Call (threaded)");
    const QString CrashDoubleFree = QT_TRANSLATE_NOOP("MenuOption", "Double Free");
    const QString CrashDoubleFreeThreaded = QT_TRANSLATE_NOOP("MenuOption", "Double Free (threaded)");
    const QString CrashNullDereference = QT_TRANSLATE_NOOP("MenuOption", "Null Dereference");
    const QString CrashNullDereferenceThreaded = QT_TRANSLATE_NOOP("MenuOption", "Null Dereference (threaded)");
    const QString CrashAbort = QT_TRANSLATE_NOOP("MenuOption", "Abort");
    const QString CrashAbortThreaded = QT_TRANSLATE_NOOP("MenuOption", "Abort (threaded)");
    const QString CrashOnShutdown = QT_TRANSLATE_NOOP("MenuOption", "Crash During Shutdown");
    const QString CrashOutOfBoundsVectorAccess = QT_TRANSLATE_NOOP("MenuOption", "Out of Bounds Vector Access");
    const QString CrashOutOfBoundsVectorAccessThreaded = QT_TRANSLATE_NOOP("MenuOption", "Out of Bounds Vector Access (threaded)");
    const QString CrashNewFault = QT_TRANSLATE_NOOP("MenuOption", "New Fault");
    const QString CrashNewFaultThreaded = QT_TRANSLATE_NOOP("MenuOption", "New Fault (threaded)");
    const QString CreateEntitiesGrabbable = QT_TRANSLATE_NOOP("MenuOption", "Create Entities As Grabbable (except Zones, Particles, and Lights)");
    const QString DeadlockInterface = QT_TRANSLATE_NOOP("MenuOption", "Deadlock Interface");
    const QString UnresponsiveInterface = QT_TRANSLATE_NOOP("MenuOption", "Unresponsive Interface");
    const QString DecreaseAvatarSize = QT_TRANSLATE_NOOP("MenuOption", "Decrease Avatar Size");
    const QString DefaultSkybox = QT_TRANSLATE_NOOP("MenuOption", "Default Skybox");
    const QString DeleteAvatarBookmark = QT_TRANSLATE_NOOP("MenuOption", "Delete Avatar Bookmark...");
    const QString DeleteAvatarEntitiesBookmark = QT_TRANSLATE_NOOP("MenuOption", "Delete Avatar Entities Bookmark");
    const QString DeleteBookmark = QT_TRANSLATE_NOOP("MenuOption", "Delete Bookmark...");
    const QString DisableActivityLogger = QT_TRANSLATE_NOOP("MenuOption", "Disable Activity Logger");
    const QString DisableEyelidAdjustment = QT_TRANSLATE_NOOP("MenuOption", "Disable Eyelid Adjustment");
    const QString DisableLightEntities = QT_TRANSLATE_NOOP("MenuOption", "Disable Light Entities");
    const QString DisplayCrashOptions = QT_TRANSLATE_NOOP("MenuOption", "Display Crash Options");
    const QString DisplayHandTargets = QT_TRANSLATE_NOOP("MenuOption", "Show Hand Targets");
    const QString DisplayModelBounds = QT_TRANSLATE_NOOP("MenuOption", "Display Model Bounds");
    const QString DisplayModelTriangles = QT_TRANSLATE_NOOP("MenuOption", "Display Model Triangles");
    const QString DisplayModelElementChildProxies = QT_TRANSLATE_NOOP("MenuOption", "Display Model Element Children");
    const QString DisplayModelElementProxy = QT_TRANSLATE_NOOP("MenuOption", "Display Model Element Bounds");
    const QString DisplayDebugTimingDetails = QT_TRANSLATE_NOOP("MenuOption", "Display Timing Details");
    const QString LocationBookmarks = QT_TRANSLATE_NOOP("MenuOption", "Bookmarks");
    const QString DontDoPrecisionPicking = QT_TRANSLATE_NOOP("MenuOption", "Don't Do Precision Picking");
    const QString DontRenderEntitiesAsScene = QT_TRANSLATE_NOOP("MenuOption", "Don't Render Entities as Scene");
    const QString EchoLocalAudio = QT_TRANSLATE_NOOP("MenuOption", "Echo Local Audio");
    const QString EchoServerAudio = QT_TRANSLATE_NOOP("MenuOption", "Echo Server Audio");
    const QString EnableFlying = QT_TRANSLATE_NOOP("MenuOption", "Enable Flying");
    const QString EnableAvatarCollisions = QT_TRANSLATE_NOOP("MenuOption", "Enable Avatar Collisions");
    const QString EnableInverseKinematics = QT_TRANSLATE_NOOP("MenuOption", "Enable Inverse Kinematics");
    const QString EntityScriptServerLog = QT_TRANSLATE_NOOP("MenuOption", "Entity Script Server Log");
    const QString ExpandMyAvatarSimulateTiming = QT_TRANSLATE_NOOP("MenuOption", "Expand /myAvatar/simulation");
    const QString ExpandMyAvatarTiming = QT_TRANSLATE_NOOP("MenuOption", "Expand /myAvatar");
    const QString ExpandOtherAvatarTiming = QT_TRANSLATE_NOOP("MenuOption", "Expand /otherAvatar");
    const QString ExpandPaintGLTiming = QT_TRANSLATE_NOOP("MenuOption", "Expand /paintGL");
    const QString ExpandSimulationTiming = QT_TRANSLATE_NOOP("MenuOption", "Expand /simulation");
    const QString ExpandPhysicsTiming = QT_TRANSLATE_NOOP("MenuOption", "Expand /physics");
    const QString ExpandUpdateTiming = QT_TRANSLATE_NOOP("MenuOption", "Expand /update");
    const QString FirstPerson = QT_TRANSLATE_NOOP("MenuOption", "First Person Legacy");
    const QString FirstPersonLookAt = QT_TRANSLATE_NOOP("MenuOption", "First Person");
    const QString FirstPersonHMD = QT_TRANSLATE_NOOP("MenuOption", "Enter First Person Mode in HMD");
    const QString FivePointCalibration = QT_TRANSLATE_NOOP("MenuOption", "5 Point Calibration");
    const QString FixGaze = QT_TRANSLATE_NOOP("MenuOption", "Fix Gaze (no saccade)");
    const QString Forward = QT_TRANSLATE_NOOP("MenuOption", "Forward");
    const QString FrameTimer = QT_TRANSLATE_NOOP("MenuOption", "Show Timer");
    const QString FullscreenMirror = QT_TRANSLATE_NOOP("MenuOption", "Mirror");
    const QString Help = QT_TRANSLATE_NOOP("MenuOption", "Help...");
    const QString HomeLocation = QT_TRANSLATE_NOOP("MenuOption", "Home ");
    const QString IncreaseAvatarSize = QT_TRANSLATE_NOOP("MenuOption", "Increase Avatar Size");
    const QString ActionMotorControl = QT_TRANSLATE_NOOP("MenuOption", "Enable Default Motor Control");
    const QString LastLocation = QT_TRANSLATE_NOOP("MenuOption", "Last Location");
    const QString LoadScript = QT_TRANSLATE_NOOP("MenuOption", "Open and Run Script File...");
    const QString LoadScriptURL = QT_TRANSLATE_NOOP("MenuOption", "Open and Run Script from URL...");
    const QString LodTools = QT_TRANSLATE_NOOP("MenuOption", "LOD Tools");
    const QString Login = QT_TRANSLATE_NOOP("MenuOption", "Login/Sign Up");
    const QString Log = QT_TRANSLATE_NOOP("MenuOption", "Log");
    const QString LogExtraTimings = QT_TRANSLATE_NOOP("MenuOption", "Log Extra Timing Details");
    const QString LookAtCamera = QT_TRANSLATE_NOOP("MenuOption", "Third Person");
    const QString LowVelocityFilter = QT_TRANSLATE_NOOP("MenuOption", "Low Velocity Filter");
    const QString MeshVisible = QT_TRANSLATE_NOOP("MenuOption", "Draw Mesh");
    const QString MuteEnvironment = QT_TRANSLATE_NOOP("MenuOption", "Mute Environment");
    const QString MuteFaceTracking = QT_TRANSLATE_NOOP("MenuOption", "Mute Face Tracking");
    const QString NamesAboveHeads = QT_TRANSLATE_NOOP("MenuOption", "Names Above Heads");
    const QString Networking = QT_TRANSLATE_NOOP("MenuOption", "Networking...");
    const QString NoFaceTracking = QT_TRANSLATE_NOOP("MenuOption", "None");
    const QString OctreeStats = QT_TRANSLATE_NOOP("MenuOption", "Entity Statistics");
    const QString OnePointCalibration = QT_TRANSLATE_NOOP("MenuOption", "1 Point Calibration");
    const QString OnlyDisplayTopTen = QT_TRANSLATE_NOOP("MenuOption", "Only Display Top Ten");
    const QString OpenVrThreadedSubmit = QT_TRANSLATE_NOOP("MenuOption", "OpenVR Threaded Submit"); 
    const QString OutputMenu = QT_TRANSLATE_NOOP("MenuOption", "Display");
    const QString Overlays = QT_TRANSLATE_NOOP("MenuOption", "Show Overlays");
    const QString PackageModel = QT_TRANSLATE_NOOP("MenuOption", "Package Avatar as .fst...");
    const QString Pair = QT_TRANSLATE_NOOP("MenuOption", "Pair");
    const QString PhysicsShowOwned = QT_TRANSLATE_NOOP("MenuOption", "Highlight Simulation Ownership");
    const QString VerboseLogging = QT_TRANSLATE_NOOP("MenuOption", "Verbose Logging");
    const QString PhysicsShowBulletWireframe = QT_TRANSLATE_NOOP("MenuOption", "Show Bullet Collision");
    const QString PhysicsShowBulletAABBs = QT_TRANSLATE_NOOP("MenuOption", "Show Bullet Bounding Boxes");
    const QString PhysicsShowBulletContactPoints = QT_TRANSLATE_NOOP("MenuOption", "Show Bullet Contact Points");
    const QString PhysicsShowBulletConstraints = QT_TRANSLATE_NOOP("MenuOption", "Show Bullet Constraints");
    const QString PhysicsShowBulletConstraintLimits = QT_TRANSLATE_NOOP("MenuOption", "Show Bullet Constraint Limits");
    const QString PipelineWarnings = QT_TRANSLATE_NOOP("MenuOption", "Log Render Pipeline Warnings");
    const QString Preferences = QT_TRANSLATE_NOOP("MenuOption", "General...");
    const QString Quit = QT_TRANSLATE_NOOP("MenuOption",  "Quit");
    const QString ReloadAllScripts = QT_TRANSLATE_NOOP("MenuOption", "Reload All Scripts");
    const QString ReloadContent = QT_TRANSLATE_NOOP("MenuOption", "Reload Content (Clears all caches)");
    const QString RenderClearKtxCache = QT_TRANSLATE_NOOP("MenuOption", "Clear KTX Cache (requires restart)");
    const QString RenderMaxTextureMemory = QT_TRANSLATE_NOOP("MenuOption", "Maximum Texture Memory");
    const QString RenderMaxTextureAutomatic = QT_TRANSLATE_NOOP("MenuOption", "Automatic Texture Memory");
    const QString RenderMaxTexture4MB = QT_TRANSLATE_NOOP("MenuOption", "4 MB");
    const QString RenderMaxTexture64MB = QT_TRANSLATE_NOOP("MenuOption", "64 MB");
    const QString RenderMaxTexture256MB = QT_TRANSLATE_NOOP("MenuOption", "256 MB");
    const QString RenderMaxTexture512MB = QT_TRANSLATE_NOOP("MenuOption", "512 MB");
    const QString RenderMaxTexture1024MB = QT_TRANSLATE_NOOP("MenuOption", "1024 MB");
    const QString RenderMaxTexture2048MB = QT_TRANSLATE_NOOP("MenuOption", "2048 MB");
    const QString RenderMaxTexture3072MB = QT_TRANSLATE_NOOP("MenuOption", "3072 MB");
    const QString RenderMaxTexture4096MB = QT_TRANSLATE_NOOP("MenuOption", "4096 MB");
    const QString RenderMaxTexture6144MB = QT_TRANSLATE_NOOP("MenuOption", "6144 MB");
    const QString RenderMaxTexture8192MB = QT_TRANSLATE_NOOP("MenuOption", "8192 MB");
    const QString RenderSensorToWorldMatrix = QT_TRANSLATE_NOOP("MenuOption", "Show SensorToWorld Matrix");
    const QString RenderIKTargets = QT_TRANSLATE_NOOP("MenuOption", "Show IK Targets");
    const QString RenderIKConstraints = QT_TRANSLATE_NOOP("MenuOption", "Show IK Constraints");
    const QString RenderIKChains = QT_TRANSLATE_NOOP("MenuOption", "Show IK Chains");
    const QString RenderDetailedCollision = QT_TRANSLATE_NOOP("MenuOption", "Show Detailed Collision");
    const QString ResetAvatarSize = QT_TRANSLATE_NOOP("MenuOption", "Reset Avatar Size");
    const QString ResetSensors = QT_TRANSLATE_NOOP("MenuOption", "Reset Sensors");
    const QString RunningScripts = QT_TRANSLATE_NOOP("MenuOption", "Running Scripts...");
    const QString RunTimingTests = QT_TRANSLATE_NOOP("MenuOption", "Run Timing Tests");
    const QString ScriptedMotorControl = QT_TRANSLATE_NOOP("MenuOption", "Enable Scripted Motor Control");
    const QString EntityScriptQMLWhitelist = QT_TRANSLATE_NOOP("MenuOption", "Entity Script / QML Whitelist");
    const QString ShowTrackedObjects = QT_TRANSLATE_NOOP("MenuOption", "Show Tracked Objects");
    const QString SelfieCamera = QT_TRANSLATE_NOOP("MenuOption", "Selfie");
    const QString SendWrongDSConnectVersion = QT_TRANSLATE_NOOP("MenuOption", "Send wrong DS connect version");
    const QString SendWrongProtocolVersion = QT_TRANSLATE_NOOP("MenuOption", "Send wrong protocol version");
    const QString SetHomeLocation = QT_TRANSLATE_NOOP("MenuOption", "Set Home Location");
    const QString ShowBordersEntityNodes = QT_TRANSLATE_NOOP("MenuOption", "Show Entity Nodes");
    const QString ShowBoundingCollisionShapes = QT_TRANSLATE_NOOP("MenuOption", "Show Bounding Collision Shapes");
    const QString ShowDSConnectTable = QT_TRANSLATE_NOOP("MenuOption", "Show Domain Connection Timing");
    const QString ShowMyLookAtVectors = QT_TRANSLATE_NOOP("MenuOption", "Show My Eye Vectors");
    const QString ShowMyLookAtTarget = QT_TRANSLATE_NOOP("MenuOption", "Show My Look-At Target");
    const QString ShowOtherLookAtVectors = QT_TRANSLATE_NOOP("MenuOption", "Show Other Eye Vectors");
    const QString ShowOtherLookAtTarget = QT_TRANSLATE_NOOP("MenuOption", "Show Other Look-At Target");
    const QString EnableLookAtSnapping = QT_TRANSLATE_NOOP("MenuOption", "Enable LookAt Snapping");
    const QString ShowRealtimeEntityStats = QT_TRANSLATE_NOOP("MenuOption", "Show Realtime Entity Stats");
    const QString SimulateEyeTracking = QT_TRANSLATE_NOOP("MenuOption", "Simulate");
    const QString SMIEyeTracking = QT_TRANSLATE_NOOP("MenuOption", "SMI Eye Tracking");
    const QString SparseTextureManagement = QT_TRANSLATE_NOOP("MenuOption", "Enable Sparse Texture Management");
    const QString StartUpLocation = QT_TRANSLATE_NOOP("MenuOption", "Start-Up Location");
    const QString Stats = QT_TRANSLATE_NOOP("MenuOption", "Show Statistics");
    const QString AnimStats = QT_TRANSLATE_NOOP("MenuOption", "Show Animation Stats");
    const QString StopAllScripts = QT_TRANSLATE_NOOP("MenuOption", "Stop All Scripts");
    const QString SuppressShortTimings = QT_TRANSLATE_NOOP("MenuOption", "Suppress Timings Less than 10ms");
    const QString ThirdPerson = QT_TRANSLATE_NOOP("MenuOption", "Third Person Legacy");
    const QString ThreePointCalibration = QT_TRANSLATE_NOOP("MenuOption", "3 Point Calibration");
    const QString ThrottleFPSIfNotFocus = QT_TRANSLATE_NOOP("MenuOption", "Throttle FPS If Not Focus";) // FIXME - this value duplicated in Basic2DWindowOpenGLDisplayPlugin.cpp
    const QString ToggleHipsFollowing = QT_TRANSLATE_NOOP("MenuOption", "Toggle Hips Following");
    const QString ToolWindow = QT_TRANSLATE_NOOP("MenuOption", "Tool Window");
    const QString TransmitterDrive = QT_TRANSLATE_NOOP("MenuOption", "Transmitter Drive");
    const QString TurnWithHead = QT_TRANSLATE_NOOP("MenuOption", "Turn using Head");
    const QString UseAudioForMouth = QT_TRANSLATE_NOOP("MenuOption", "Use Audio for Mouth");
    const QString UseCamera = QT_TRANSLATE_NOOP("MenuOption", "Use Camera");
    const QString VelocityFilter = QT_TRANSLATE_NOOP("MenuOption", "Velocity Filter");
    const QString VisibleToEveryone = QT_TRANSLATE_NOOP("MenuOption", "Everyone");
    const QString VisibleToFriends = QT_TRANSLATE_NOOP("MenuOption", "Friends");
    const QString VisibleToNoOne = QT_TRANSLATE_NOOP("MenuOption", "No one");
    const QString WorldAxes = QT_TRANSLATE_NOOP("MenuOption", "World Axes");
    const QString DesktopTabletToToolbar = QT_TRANSLATE_NOOP("MenuOption", "Desktop Tablet Becomes Toolbar");
    const QString HMDTabletToToolbar = QT_TRANSLATE_NOOP("MenuOption", "HMD Tablet Becomes Toolbar");
    const QString Shadows = QT_TRANSLATE_NOOP("MenuOption", "Shadows");
    const QString AntiAliasing = QT_TRANSLATE_NOOP("MenuOption", "Temporal Antialiasing (FXAA if disabled)");
    const QString AmbientOcclusion = QT_TRANSLATE_NOOP("MenuOption", "Ambient Occlusion");
    const QString NotificationSounds = QT_TRANSLATE_NOOP("MenuOption", "play_notification_sounds");
    const QString NotificationSoundsSnapshot = QT_TRANSLATE_NOOP("MenuOption", "play_notification_sounds_snapshot");
    const QString NotificationSoundsTablet = QT_TRANSLATE_NOOP("MenuOption", "play_notification_sounds_tablet");
    const QString ForceCoarsePicking = QT_TRANSLATE_NOOP("MenuOption", "Force Coarse Picking");
    const QString ComputeBlendshapes = QT_TRANSLATE_NOOP("MenuOption", "Compute Blendshapes");
    const QString HighlightTransitions = QT_TRANSLATE_NOOP("MenuOption", "Highlight Transitions");
    const QString MaterialProceduralShaders = QT_TRANSLATE_NOOP("MenuOption", "Enable Procedural Materials");
}

#endif // hifi_Menu_h

