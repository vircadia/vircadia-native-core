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
    const QString AboutApp = "About Interface";
    const QString AddRemoveFriends = "Add/Remove Friends...";
    const QString AddressBar = "Show Address Bar";
    const QString Animations = "Animations...";
    const QString AnimDebugDrawAnimPose = "Debug Draw Animation";
    const QString AnimDebugDrawDefaultPose = "Debug Draw Default Pose";
    const QString AnimDebugDrawPosition= "Debug Draw Position";
    const QString AskToResetSettings = "Ask To Reset Settings";
    const QString AssetMigration = "ATP Asset Migration";
    const QString AssetServer = "Asset Browser";
    const QString Attachments = "Attachments...";
    const QString AudioNoiseReduction = "Audio Noise Reduction";
    const QString AudioScope = "Show Scope";
    const QString AudioScopeFiftyFrames = "Fifty";
    const QString AudioScopeFiveFrames = "Five";
    const QString AudioScopeFrames = "Display Frames";
    const QString AudioScopePause = "Pause Scope";
    const QString AudioScopeTwentyFrames = "Twenty";
    const QString AudioStatsShowInjectedStreams = "Audio Stats Show Injected Streams";
    const QString AudioTools = "Show Level Meter";
    const QString AutoMuteAudio = "Auto Mute Microphone";
    const QString AvatarReceiveStats = "Show Receive Stats";
    const QString Back = "Back";
    const QString BandwidthDetails = "Bandwidth Details";
    const QString BinaryEyelidControl = "Binary Eyelid Control";
    const QString BookmarkLocation = "Bookmark Location";
    const QString Bookmarks = "Bookmarks";
    const QString CachesSize = "RAM Caches Size";
    const QString CalibrateCamera = "Calibrate Camera";
    const QString CameraEntityMode = "Entity Mode";
    const QString CenterPlayerInView = "Center Player In View";
    const QString Chat = "Chat...";
    const QString Collisions = "Collisions";
    const QString Connexion = "Activate 3D Connexion Devices";
    const QString Console = "Console...";
    const QString ControlWithSpeech = "Control With Speech";
    const QString CopyAddress = "Copy Address to Clipboard";
    const QString CopyPath = "Copy Path to Clipboard";
    const QString CoupleEyelids = "Couple Eyelids";
    const QString CrashPureVirtualFunction = "Pure Virtual Function Call";
    const QString CrashPureVirtualFunctionThreaded = "Pure Virtual Function Call (threaded)";
    const QString CrashDoubleFree = "Double Free";
    const QString CrashDoubleFreeThreaded = "Double Free (threaded)";
    const QString CrashNullDereference = "Null Dereference";
    const QString CrashNullDereferenceThreaded = "Null Dereference (threaded)";
    const QString CrashAbort = "Abort";
    const QString CrashAbortThreaded = "Abort (threaded)";
    const QString CrashOutOfBoundsVectorAccess = "Out of Bounds Vector Access";
    const QString CrashOutOfBoundsVectorAccessThreaded = "Out of Bounds Vector Access (threaded)";
    const QString CrashNewFault = "New Fault";
    const QString CrashNewFaultThreaded = "New Fault (threaded)";
    const QString DeadlockInterface = "Deadlock Interface";
    const QString DecreaseAvatarSize = "Decrease Avatar Size";
    const QString DefaultSkybox = "Default Skybox";
    const QString DeleteBookmark = "Delete Bookmark...";
    const QString DisableActivityLogger = "Disable Activity Logger";
    const QString DisableEyelidAdjustment = "Disable Eyelid Adjustment";
    const QString DisableLightEntities = "Disable Light Entities";
    const QString DiskCacheEditor = "Disk Cache Editor";
    const QString DisplayCrashOptions = "Display Crash Options";
    const QString DisplayHandTargets = "Show Hand Targets";
    const QString DisplayLeftFootTrace = "Show Left Foot Trace";
    const QString DisplayRightFootTrace = "Show Right Foot Trace";
    const QString DisplayModelBounds = "Display Model Bounds";
    const QString DisplayModelTriangles = "Display Model Triangles";
    const QString DisplayModelElementChildProxies = "Display Model Element Children";
    const QString DisplayModelElementProxy = "Display Model Element Bounds";
    const QString DisplayDebugTimingDetails = "Display Timing Details";
    const QString DontDoPrecisionPicking = "Don't Do Precision Picking";
    const QString DontRenderEntitiesAsScene = "Don't Render Entities as Scene";
    const QString EchoLocalAudio = "Echo Local Audio";
    const QString EchoServerAudio = "Echo Server Audio";
    const QString EnableAvatarCollisions = "Enable Avatar Collisions";
    const QString EnableIncrementalTextureTransfer = "Enable Incremental Texture Transfer";
    const QString EnableDynamicTextureManagement = "Enable Dynamic Texture Management";
    const QString EnableInverseKinematics = "Enable Inverse Kinematics";
    const QString EnableVerticalComfortMode = "Enable Vertical Comfort Mode";
    const QString ExpandMyAvatarSimulateTiming = "Expand /myAvatar/simulation";
    const QString ExpandMyAvatarTiming = "Expand /myAvatar";
    const QString ExpandOtherAvatarTiming = "Expand /otherAvatar";
    const QString ExpandPaintGLTiming = "Expand /paintGL";
    const QString ExpandPhysicsSimulationTiming = "Expand /physics";
    const QString ExpandUpdateTiming = "Expand /update";
    const QString Faceshift = "Faceshift";
    const QString FirstPerson = "First Person";
    const QString FivePointCalibration = "5 Point Calibration";
    const QString FixGaze = "Fix Gaze (no saccade)";
    const QString Forward = "Forward";
    const QString FrameTimer = "Show Timer";
    const QString FullscreenMirror = "Mirror";
    const QString Help = "Help...";
    const QString IncreaseAvatarSize = "Increase Avatar Size";
    const QString IndependentMode = "Independent Mode";
    const QString ActionMotorControl = "Enable Default Motor Control";
    const QString LeapMotionOnHMD = "Leap Motion on HMD";
    const QString LoadScript = "Open and Run Script File...";
    const QString LoadScriptURL = "Open and Run Script from URL...";
    const QString LodTools = "LOD Tools";
    const QString Login = "Login";
    const QString Log = "Log";
    const QString LogExtraTimings = "Log Extra Timing Details";
    const QString LowVelocityFilter = "Low Velocity Filter";
    const QString MeshVisible = "Draw Mesh";
    const QString MiniMirror = "Mini Mirror";
    const QString MuteAudio = "Mute Microphone";
    const QString MuteEnvironment = "Mute Environment";
    const QString MuteFaceTracking = "Mute Face Tracking";
    const QString NamesAboveHeads = "Names Above Heads";
    const QString Networking = "Networking...";
    const QString NoFaceTracking = "None";
    const QString OctreeStats = "Entity Statistics";
    const QString OnePointCalibration = "1 Point Calibration";
    const QString OnlyDisplayTopTen = "Only Display Top Ten";
    const QString OpenVrThreadedSubmit = "OpenVR Threaded Submit"; 
    const QString OutputMenu = "Display";
    const QString Overlays = "Overlays";
    const QString PackageModel = "Package Model...";
    const QString Pair = "Pair";
    const QString PhysicsShowHulls = "Draw Collision Shapes";
    const QString PhysicsShowOwned = "Highlight Simulation Ownership";
    const QString PipelineWarnings = "Log Render Pipeline Warnings";
    const QString Preferences = "General...";
    const QString Quit =  "Quit";
    const QString ReloadAllScripts = "Reload All Scripts";
    const QString ReloadContent = "Reload Content (Clears all caches)";
    const QString RenderBoundingCollisionShapes = "Show Bounding Collision Shapes";
    const QString RenderMyLookAtVectors = "Show My Eye Vectors";
    const QString RenderOtherLookAtVectors = "Show Other Eye Vectors";
    const QString RenderMaxTextureMemory = "Maximum Texture Memory";
    const QString RenderMaxTextureAutomatic = "Automatic Texture Memory";
    const QString RenderMaxTexture64MB = "64 MB";
    const QString RenderMaxTexture256MB = "256 MB";
    const QString RenderMaxTexture512MB = "512 MB";
    const QString RenderMaxTexture1024MB = "1024 MB";
    const QString RenderMaxTexture2048MB = "2048 MB";
    const QString RenderResolution = "Scale Resolution";
    const QString RenderResolutionOne = "1";
    const QString RenderResolutionTwoThird = "2/3";
    const QString RenderResolutionHalf = "1/2";
    const QString RenderResolutionThird = "1/3";
    const QString RenderResolutionQuarter = "1/4";
    const QString RenderSensorToWorldMatrix = "Show SensorToWorld Matrix";
    const QString RenderIKTargets = "Show IK Targets";
    const QString ResetAvatarSize = "Reset Avatar Size";
    const QString ResetSensors = "Reset Sensors";
    const QString RunningScripts = "Running Scripts...";
    const QString RunTimingTests = "Run Timing Tests";
    const QString ScriptEditor = "Script Editor...";
    const QString ScriptedMotorControl = "Enable Scripted Motor Control";
    const QString SendWrongDSConnectVersion = "Send wrong DS connect version";
    const QString SendWrongProtocolVersion = "Send wrong protocol version";
    const QString SetHomeLocation = "Set Home Location";
    const QString ShowDSConnectTable = "Show Domain Connection Timing";
    const QString ShowBordersEntityNodes = "Show Entity Nodes";
    const QString ShowRealtimeEntityStats = "Show Realtime Entity Stats";
    const QString StandingHMDSensorMode = "Standing HMD Sensor Mode";
    const QString SimulateEyeTracking = "Simulate";
    const QString SMIEyeTracking = "SMI Eye Tracking";
    const QString SparseTextureManagement = "Enable Sparse Texture Management";
    const QString Stats = "Stats";
    const QString StopAllScripts = "Stop All Scripts";
    const QString SuppressShortTimings = "Suppress Timings Less than 10ms";
    const QString ThirdPerson = "Third Person";
    const QString ThreePointCalibration = "3 Point Calibration";
    const QString ThrottleFPSIfNotFocus = "Throttle FPS If Not Focus"; // FIXME - this value duplicated in Basic2DWindowOpenGLDisplayPlugin.cpp
    const QString ToolWindow = "Tool Window";
    const QString TransmitterDrive = "Transmitter Drive";
    const QString TurnWithHead = "Turn using Head";
    const QString UseAudioForMouth = "Use Audio for Mouth";
    const QString UseCamera = "Use Camera";
    const QString UseAnimPreAndPostRotations = "Use Anim Pre and Post Rotations";
    const QString MoveKinematically = "Move Kinematically"; // KINEMATIC_CONTROLLER_HACK
    const QString VelocityFilter = "Velocity Filter";
    const QString VisibleToEveryone = "Everyone";
    const QString VisibleToFriends = "Friends";
    const QString VisibleToNoOne = "No one";
    const QString WorldAxes = "World Axes";
}

#endif // hifi_Menu_h

