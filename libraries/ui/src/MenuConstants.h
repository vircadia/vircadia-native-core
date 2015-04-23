//
//  MenuConstants.h
//
//  Created by Bradley Austin Davis on 2015/04/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_MenuContants_h
#define hifi_MenuConstants_h

#include <QQuickItem>
#include <QHash>
#include <QList>
#include "OffscreenQmlDialog.h"

namespace MenuOption {
    const QString AboutApp = "About Interface";
    const QString AddRemoveFriends = "Add/Remove Friends...";
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
    const QString BandwidthDetails = "Bandwidth Details";
    const QString BlueSpeechSphere = "Blue Sphere While Speaking";
    const QString BookmarkLocation = "Bookmark Location";
    const QString Bookmarks = "Bookmarks";
    const QString CascadedShadows = "Cascaded";
    const QString CachesSize = "RAM Caches Size";
    const QString Chat = "Chat...";
    const QString Collisions = "Collisions";
    const QString Console = "Console...";
    const QString ControlWithSpeech = "Control With Speech";
    const QString CopyAddress = "Copy Address to Clipboard";
    const QString CopyPath = "Copy Path to Clipboard";
    const QString DDEFaceRegression = "DDE Face Regression";
    const QString DDEFiltering = "DDE Filtering";
    const QString DecreaseAvatarSize = "Decrease Avatar Size";
    const QString DeleteBookmark = "Delete Bookmark...";
    const QString DisableActivityLogger = "Disable Activity Logger";
    const QString DisableLightEntities = "Disable Light Entities";
    const QString DisableNackPackets = "Disable NACK Packets";
    const QString DiskCacheEditor = "Disk Cache Editor";
    const QString DisplayHands = "Show Hand Info";
    const QString DisplayHandTargets = "Show Hand Targets";
    const QString DisplayModelBounds = "Display Model Bounds";
    const QString DisplayModelTriangles = "Display Model Triangles";
    const QString DisplayModelElementChildProxies = "Display Model Element Children";
    const QString DisplayModelElementProxy = "Display Model Element Bounds";
    const QString DisplayDebugTimingDetails = "Display Timing Details";
    const QString DontDoPrecisionPicking = "Don't Do Precision Picking";
    const QString DontFadeOnOctreeServerChanges = "Don't Fade In/Out on Octree Server Changes";
    const QString DontRenderEntitiesAsScene = "Don't Render Entities as Scene";
    const QString EchoLocalAudio = "Echo Local Audio";
    const QString EchoServerAudio = "Echo Server Audio";
    const QString EditEntitiesHelp = "Edit Entities Help...";
    const QString Enable3DTVMode = "Enable 3DTV Mode";
    const QString EnableCharacterController = "Enable avatar collisions";
    const QString EnableGlowEffect = "Enable Glow Effect (Warning: Poor Oculus Performance)";
    const QString EnableVRMode = "Enable VR Mode";
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
    const QString Mirror = "Mirror";
    const QString MuteAudio = "Mute Microphone";
    const QString MuteEnvironment = "Mute Environment";
    const QString NoFaceTracking = "None";
    const QString OctreeStats = "Entity Statistics";
    const QString OffAxisProjection = "Off-Axis Projection";
    const QString OnlyDisplayTopTen = "Only Display Top Ten";
    const QString PackageModel = "Package Model...";
    const QString Pair = "Pair";
    const QString PipelineWarnings = "Log Render Pipeline Warnings";
    const QString Preferences = "Preferences...";
    const QString Quit =  "Quit";
    const QString ReloadAllScripts = "Reload All Scripts";
    const QString RenderBoundingCollisionShapes = "Show Bounding Collision Shapes";
    const QString RenderFocusIndicator = "Show Eye Focus";
    const QString RenderHeadCollisionShapes = "Show Head Collision Shapes";
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
    const QString ResetDDETracking = "Reset DDE Tracking";
    const QString ResetSensors = "Reset Sensors";
    const QString RunningScripts = "Running Scripts";
    const QString RunTimingTests = "Run Timing Tests";
    const QString ScriptEditor = "Script Editor...";
    const QString ScriptedMotorControl = "Enable Scripted Motor Control";
    const QString ShowBordersEntityNodes = "Show Entity Nodes";
    const QString ShowIKConstraints = "Show IK Constraints";
    const QString SimpleShadows = "Simple";
    const QString SixenseEnabled = "Enable Hydra Support";
    const QString SixenseMouseInput = "Enable Sixense Mouse Input";
    const QString SixenseLasers = "Enable Sixense UI Lasers";
    const QString ShiftHipsForIdleAnimations = "Shift hips for idle animations";
    const QString Stars = "Stars";
    const QString Stats = "Stats";
    const QString StereoAudio = "Stereo Audio (disables spatial sound)";
    const QString StopAllScripts = "Stop All Scripts";
    const QString SuppressShortTimings = "Suppress Timings Less than 10ms";
    const QString TestPing = "Test Ping";
    const QString ToolWindow = "Tool Window";
    const QString TransmitterDrive = "Transmitter Drive";
    const QString TurnWithHead = "Turn using Head";
    const QString UseAudioForMouth = "Use Audio for Mouth";
    const QString VisibleToEveryone = "Everyone";
    const QString VisibleToFriends = "Friends";
    const QString VisibleToNoOne = "No one";
    const QString Wireframe = "Wireframe";
}


class HifiMenu : public QQuickItem
{
    Q_OBJECT
    QML_DIALOG_DECL

    Q_PROPERTY(QString aboutApp READ aboutApp CONSTANT)
    Q_PROPERTY(QString addRemoveFriends READ addRemoveFriends CONSTANT)
    Q_PROPERTY(QString addressBar READ addressBar CONSTANT)
    Q_PROPERTY(QString alignForearmsWithWrists READ alignForearmsWithWrists CONSTANT)
    Q_PROPERTY(QString alternateIK READ alternateIK CONSTANT)
    Q_PROPERTY(QString ambientOcclusion READ ambientOcclusion CONSTANT)
    Q_PROPERTY(QString animations READ animations CONSTANT)
    Q_PROPERTY(QString atmosphere READ atmosphere CONSTANT)
    Q_PROPERTY(QString attachments READ attachments CONSTANT)
    Q_PROPERTY(QString audioNoiseReduction READ audioNoiseReduction CONSTANT)
    Q_PROPERTY(QString audioScope READ audioScope CONSTANT)
    Q_PROPERTY(QString audioScopeFiftyFrames READ audioScopeFiftyFrames CONSTANT)
    Q_PROPERTY(QString audioScopeFiveFrames READ audioScopeFiveFrames CONSTANT)
    Q_PROPERTY(QString audioScopeFrames READ audioScopeFrames CONSTANT)
    Q_PROPERTY(QString audioScopePause READ audioScopePause CONSTANT)
    Q_PROPERTY(QString audioScopeTwentyFrames READ audioScopeTwentyFrames CONSTANT)
    Q_PROPERTY(QString audioStats READ audioStats CONSTANT)
    Q_PROPERTY(QString audioStatsShowInjectedStreams READ audioStatsShowInjectedStreams CONSTANT)
    Q_PROPERTY(QString bandwidthDetails READ bandwidthDetails CONSTANT)
    Q_PROPERTY(QString blueSpeechSphere READ blueSpeechSphere CONSTANT)
    Q_PROPERTY(QString bookmarkLocation READ bookmarkLocation CONSTANT)
    Q_PROPERTY(QString bookmarks READ bookmarks CONSTANT)
    Q_PROPERTY(QString cascadedShadows READ cascadedShadows CONSTANT)
    Q_PROPERTY(QString cachesSize READ cachesSize CONSTANT)
    Q_PROPERTY(QString chat READ chat CONSTANT)
    Q_PROPERTY(QString collisions READ collisions CONSTANT)
    Q_PROPERTY(QString console READ console CONSTANT)
    Q_PROPERTY(QString controlWithSpeech READ controlWithSpeech CONSTANT)
    Q_PROPERTY(QString copyAddress READ copyAddress CONSTANT)
    Q_PROPERTY(QString copyPath READ copyPath CONSTANT)
    Q_PROPERTY(QString ddeFaceRegression READ ddeFaceRegression CONSTANT)
    Q_PROPERTY(QString ddeFiltering READ ddeFiltering CONSTANT)
    Q_PROPERTY(QString decreaseAvatarSize READ decreaseAvatarSize CONSTANT)
    Q_PROPERTY(QString deleteBookmark READ deleteBookmark CONSTANT)
    Q_PROPERTY(QString disableActivityLogger READ disableActivityLogger CONSTANT)
    Q_PROPERTY(QString disableLightEntities READ disableLightEntities CONSTANT)
    Q_PROPERTY(QString disableNackPackets READ disableNackPackets CONSTANT)
    Q_PROPERTY(QString diskCacheEditor READ diskCacheEditor CONSTANT)
    Q_PROPERTY(QString displayHands READ displayHands CONSTANT)
    Q_PROPERTY(QString displayHandTargets READ displayHandTargets CONSTANT)
    Q_PROPERTY(QString displayModelBounds READ displayModelBounds CONSTANT)
    Q_PROPERTY(QString displayModelTriangles READ displayModelTriangles CONSTANT)
    Q_PROPERTY(QString displayModelElementChildProxies READ displayModelElementChildProxies CONSTANT)
    Q_PROPERTY(QString displayModelElementProxy READ displayModelElementProxy CONSTANT)
    Q_PROPERTY(QString displayDebugTimingDetails READ displayDebugTimingDetails CONSTANT)
    Q_PROPERTY(QString dontDoPrecisionPicking READ dontDoPrecisionPicking CONSTANT)
    Q_PROPERTY(QString dontFadeOnOctreeServerChanges READ dontFadeOnOctreeServerChanges CONSTANT)
    Q_PROPERTY(QString dontRenderEntitiesAsScene READ dontRenderEntitiesAsScene CONSTANT)
    Q_PROPERTY(QString echoLocalAudio READ echoLocalAudio CONSTANT)
    Q_PROPERTY(QString echoServerAudio READ echoServerAudio CONSTANT)
    Q_PROPERTY(QString editEntitiesHelp READ editEntitiesHelp CONSTANT)
    Q_PROPERTY(QString enable3DTVMode READ enable3DTVMode CONSTANT)
    Q_PROPERTY(QString enableCharacterController READ enableCharacterController CONSTANT)
    Q_PROPERTY(QString enableGlowEffect READ enableGlowEffect CONSTANT)
    Q_PROPERTY(QString enableVRMode READ enableVRMode CONSTANT)
    Q_PROPERTY(QString expandMyAvatarSimulateTiming READ expandMyAvatarSimulateTiming CONSTANT)
    Q_PROPERTY(QString expandMyAvatarTiming READ expandMyAvatarTiming CONSTANT)
    Q_PROPERTY(QString expandOtherAvatarTiming READ expandOtherAvatarTiming CONSTANT)
    Q_PROPERTY(QString expandPaintGLTiming READ expandPaintGLTiming CONSTANT)
    Q_PROPERTY(QString expandUpdateTiming READ expandUpdateTiming CONSTANT)
    Q_PROPERTY(QString faceshift READ faceshift CONSTANT)
    Q_PROPERTY(QString filterSixense READ filterSixense CONSTANT)
    Q_PROPERTY(QString firstPerson READ firstPerson CONSTANT)
    Q_PROPERTY(QString frameTimer READ frameTimer CONSTANT)
    Q_PROPERTY(QString fullscreen READ fullscreen CONSTANT)
    Q_PROPERTY(QString fullscreenMirror READ fullscreenMirror CONSTANT)
    Q_PROPERTY(QString glowWhenSpeaking READ glowWhenSpeaking CONSTANT)
    Q_PROPERTY(QString namesAboveHeads READ namesAboveHeads CONSTANT)
    Q_PROPERTY(QString goToUser READ goToUser CONSTANT)
    Q_PROPERTY(QString hmdTools READ hmdTools CONSTANT)
    Q_PROPERTY(QString increaseAvatarSize READ increaseAvatarSize CONSTANT)
    Q_PROPERTY(QString keyboardMotorControl READ keyboardMotorControl CONSTANT)
    Q_PROPERTY(QString leapMotionOnHMD READ leapMotionOnHMD CONSTANT)
    Q_PROPERTY(QString loadScript READ loadScript CONSTANT)
    Q_PROPERTY(QString loadScriptURL READ loadScriptURL CONSTANT)
    Q_PROPERTY(QString loadRSSDKFile READ loadRSSDKFile CONSTANT)
    Q_PROPERTY(QString lodTools READ lodTools CONSTANT)
    Q_PROPERTY(QString login READ login CONSTANT)
    Q_PROPERTY(QString log READ log CONSTANT)
    Q_PROPERTY(QString lowVelocityFilter READ lowVelocityFilter CONSTANT)
    Q_PROPERTY(QString mirror READ mirror CONSTANT)
    Q_PROPERTY(QString muteAudio READ muteAudio CONSTANT)
    Q_PROPERTY(QString muteEnvironment READ muteEnvironment CONSTANT)
    Q_PROPERTY(QString noFaceTracking READ noFaceTracking CONSTANT)
    Q_PROPERTY(QString octreeStats READ octreeStats CONSTANT)
    Q_PROPERTY(QString offAxisProjection READ offAxisProjection CONSTANT)
    Q_PROPERTY(QString onlyDisplayTopTen READ onlyDisplayTopTen CONSTANT)
    Q_PROPERTY(QString packageModel READ packageModel CONSTANT)
    Q_PROPERTY(QString pair READ pair CONSTANT)
    Q_PROPERTY(QString pipelineWarnings READ pipelineWarnings CONSTANT)
    Q_PROPERTY(QString preferences READ preferences CONSTANT)
    Q_PROPERTY(QString quit READ quit CONSTANT)
    Q_PROPERTY(QString reloadAllScripts READ reloadAllScripts CONSTANT)
    Q_PROPERTY(QString renderBoundingCollisionShapes READ renderBoundingCollisionShapes CONSTANT)
    Q_PROPERTY(QString renderFocusIndicator READ renderFocusIndicator CONSTANT)
    Q_PROPERTY(QString renderHeadCollisionShapes READ renderHeadCollisionShapes CONSTANT)
    Q_PROPERTY(QString renderLookAtVectors READ renderLookAtVectors CONSTANT)
    Q_PROPERTY(QString renderSkeletonCollisionShapes READ renderSkeletonCollisionShapes CONSTANT)
    Q_PROPERTY(QString renderTargetFramerate READ renderTargetFramerate CONSTANT)
    Q_PROPERTY(QString renderTargetFramerateUnlimited READ renderTargetFramerateUnlimited CONSTANT)
    Q_PROPERTY(QString renderTargetFramerate60 READ renderTargetFramerate60 CONSTANT)
    Q_PROPERTY(QString renderTargetFramerate50 READ renderTargetFramerate50 CONSTANT)
    Q_PROPERTY(QString renderTargetFramerate40 READ renderTargetFramerate40 CONSTANT)
    Q_PROPERTY(QString renderTargetFramerate30 READ renderTargetFramerate30 CONSTANT)
    Q_PROPERTY(QString renderTargetFramerateVSyncOn READ renderTargetFramerateVSyncOn CONSTANT)
    Q_PROPERTY(QString renderResolution READ renderResolution CONSTANT)
    Q_PROPERTY(QString renderResolutionOne READ renderResolutionOne CONSTANT)
    Q_PROPERTY(QString renderResolutionTwoThird READ renderResolutionTwoThird CONSTANT)
    Q_PROPERTY(QString renderResolutionHalf READ renderResolutionHalf CONSTANT)
    Q_PROPERTY(QString renderResolutionThird READ renderResolutionThird CONSTANT)
    Q_PROPERTY(QString renderResolutionQuarter READ renderResolutionQuarter CONSTANT)
    Q_PROPERTY(QString renderAmbientLight READ renderAmbientLight CONSTANT)
    Q_PROPERTY(QString renderAmbientLightGlobal READ renderAmbientLightGlobal CONSTANT)
    Q_PROPERTY(QString renderAmbientLight0 READ renderAmbientLight0 CONSTANT)
    Q_PROPERTY(QString renderAmbientLight1 READ renderAmbientLight1 CONSTANT)
    Q_PROPERTY(QString renderAmbientLight2 READ renderAmbientLight2 CONSTANT)
    Q_PROPERTY(QString renderAmbientLight3 READ renderAmbientLight3 CONSTANT)
    Q_PROPERTY(QString renderAmbientLight4 READ renderAmbientLight4 CONSTANT)
    Q_PROPERTY(QString renderAmbientLight5 READ renderAmbientLight5 CONSTANT)
    Q_PROPERTY(QString renderAmbientLight6 READ renderAmbientLight6 CONSTANT)
    Q_PROPERTY(QString renderAmbientLight7 READ renderAmbientLight7 CONSTANT)
    Q_PROPERTY(QString renderAmbientLight8 READ renderAmbientLight8 CONSTANT)
    Q_PROPERTY(QString renderAmbientLight9 READ renderAmbientLight9 CONSTANT)
    Q_PROPERTY(QString resetAvatarSize READ resetAvatarSize CONSTANT)
    Q_PROPERTY(QString resetDDETracking READ resetDDETracking CONSTANT)
    Q_PROPERTY(QString resetSensors READ resetSensors CONSTANT)
    Q_PROPERTY(QString runningScripts READ runningScripts CONSTANT)
    Q_PROPERTY(QString runTimingTests READ runTimingTests CONSTANT)
    Q_PROPERTY(QString scriptEditor READ scriptEditor CONSTANT)
    Q_PROPERTY(QString scriptedMotorControl READ scriptedMotorControl CONSTANT)
    Q_PROPERTY(QString showBordersEntityNodes READ showBordersEntityNodes CONSTANT)
    Q_PROPERTY(QString showIKConstraints READ showIKConstraints CONSTANT)
    Q_PROPERTY(QString simpleShadows READ simpleShadows CONSTANT)
    Q_PROPERTY(QString sixenseEnabled READ sixenseEnabled CONSTANT)
    Q_PROPERTY(QString sixenseMouseInput READ sixenseMouseInput CONSTANT)
    Q_PROPERTY(QString sixenseLasers READ sixenseLasers CONSTANT)
    Q_PROPERTY(QString shiftHipsForIdleAnimations READ shiftHipsForIdleAnimations CONSTANT)
    Q_PROPERTY(QString stars READ stars CONSTANT)
    Q_PROPERTY(QString stats READ stats CONSTANT)
    Q_PROPERTY(QString stereoAudio READ stereoAudio CONSTANT)
    Q_PROPERTY(QString stopAllScripts READ stopAllScripts CONSTANT)
    Q_PROPERTY(QString suppressShortTimings READ suppressShortTimings CONSTANT)
    Q_PROPERTY(QString testPing READ testPing CONSTANT)
    Q_PROPERTY(QString toolWindow READ toolWindow CONSTANT)
    Q_PROPERTY(QString transmitterDrive READ transmitterDrive CONSTANT)
    Q_PROPERTY(QString turnWithHead READ turnWithHead CONSTANT)
    Q_PROPERTY(QString useAudioForMouth READ useAudioForMouth CONSTANT)
    Q_PROPERTY(QString visibleToEveryone READ visibleToEveryone CONSTANT)
    Q_PROPERTY(QString visibleToFriends READ visibleToFriends CONSTANT)
    Q_PROPERTY(QString visibleToNoOne READ visibleToNoOne CONSTANT)
    Q_PROPERTY(QString wireframe READ wireframe CONSTANT)

public:

    const QString & aboutApp() { return MenuOption::AboutApp; }
    const QString & addRemoveFriends() { return MenuOption::AddRemoveFriends; }
    const QString & addressBar() { return MenuOption::AddressBar; }
    const QString & alignForearmsWithWrists() { return MenuOption::AlignForearmsWithWrists; }
    const QString & alternateIK() { return MenuOption::AlternateIK; }
    const QString & ambientOcclusion() { return MenuOption::AmbientOcclusion; }
    const QString & animations() { return MenuOption::Animations; }
    const QString & atmosphere() { return MenuOption::Atmosphere; }
    const QString & attachments() { return MenuOption::Attachments; }
    const QString & audioNoiseReduction() { return MenuOption::AudioNoiseReduction; }
    const QString & audioScope() { return MenuOption::AudioScope; }
    const QString & audioScopeFiftyFrames() { return MenuOption::AudioScopeFiftyFrames; }
    const QString & audioScopeFiveFrames() { return MenuOption::AudioScopeFiveFrames; }
    const QString & audioScopeFrames() { return MenuOption::AudioScopeFrames; }
    const QString & audioScopePause() { return MenuOption::AudioScopePause; }
    const QString & audioScopeTwentyFrames() { return MenuOption::AudioScopeTwentyFrames; }
    const QString & audioStats() { return MenuOption::AudioStats; }
    const QString & audioStatsShowInjectedStreams() { return MenuOption::AudioStatsShowInjectedStreams; }
    const QString & bandwidthDetails() { return MenuOption::BandwidthDetails; }
    const QString & blueSpeechSphere() { return MenuOption::BlueSpeechSphere; }
    const QString & bookmarkLocation() { return MenuOption::BookmarkLocation; }
    const QString & bookmarks() { return MenuOption::Bookmarks; }
    const QString & cascadedShadows() { return MenuOption::CascadedShadows; }
    const QString & cachesSize() { return MenuOption::CachesSize; }
    const QString & chat() { return MenuOption::Chat; }
    const QString & collisions() { return MenuOption::Collisions; }
    const QString & console() { return MenuOption::Console; }
    const QString & controlWithSpeech() { return MenuOption::ControlWithSpeech; }
    const QString & copyAddress() { return MenuOption::CopyAddress; }
    const QString & copyPath() { return MenuOption::CopyPath; }
    const QString & ddeFaceRegression() { return MenuOption::DDEFaceRegression; }
    const QString & ddeFiltering() { return MenuOption::DDEFiltering; }
    const QString & decreaseAvatarSize() { return MenuOption::DecreaseAvatarSize; }
    const QString & deleteBookmark() { return MenuOption::DeleteBookmark; }
    const QString & disableActivityLogger() { return MenuOption::DisableActivityLogger; }
    const QString & disableLightEntities() { return MenuOption::DisableLightEntities; }
    const QString & disableNackPackets() { return MenuOption::DisableNackPackets; }
    const QString & diskCacheEditor() { return MenuOption::DiskCacheEditor; }
    const QString & displayHands() { return MenuOption::DisplayHands; }
    const QString & displayHandTargets() { return MenuOption::DisplayHandTargets; }
    const QString & displayModelBounds() { return MenuOption::DisplayModelBounds; }
    const QString & displayModelTriangles() { return MenuOption::DisplayModelTriangles; }
    const QString & displayModelElementChildProxies() { return MenuOption::DisplayModelElementChildProxies; }
    const QString & displayModelElementProxy() { return MenuOption::DisplayModelElementProxy; }
    const QString & displayDebugTimingDetails() { return MenuOption::DisplayDebugTimingDetails; }
    const QString & dontDoPrecisionPicking() { return MenuOption::DontDoPrecisionPicking; }
    const QString & dontFadeOnOctreeServerChanges() { return MenuOption::DontFadeOnOctreeServerChanges; }
    const QString & dontRenderEntitiesAsScene() { return MenuOption::DontRenderEntitiesAsScene; }
    const QString & echoLocalAudio() { return MenuOption::EchoLocalAudio; }
    const QString & echoServerAudio() { return MenuOption::EchoServerAudio; }
    const QString & editEntitiesHelp() { return MenuOption::EditEntitiesHelp; }
    const QString & enable3DTVMode() { return MenuOption::Enable3DTVMode; }
    const QString & enableCharacterController() { return MenuOption::EnableCharacterController; }
    const QString & enableGlowEffect() { return MenuOption::EnableGlowEffect; }
    const QString & enableVRMode() { return MenuOption::EnableVRMode; }
    const QString & expandMyAvatarSimulateTiming() { return MenuOption::ExpandMyAvatarSimulateTiming; }
    const QString & expandMyAvatarTiming() { return MenuOption::ExpandMyAvatarTiming; }
    const QString & expandOtherAvatarTiming() { return MenuOption::ExpandOtherAvatarTiming; }
    const QString & expandPaintGLTiming() { return MenuOption::ExpandPaintGLTiming; }
    const QString & expandUpdateTiming() { return MenuOption::ExpandUpdateTiming; }
    const QString & faceshift() { return MenuOption::Faceshift; }
    const QString & filterSixense() { return MenuOption::FilterSixense; }
    const QString & firstPerson() { return MenuOption::FirstPerson; }
    const QString & frameTimer() { return MenuOption::FrameTimer; }
    const QString & fullscreen() { return MenuOption::Fullscreen; }
    const QString & fullscreenMirror() { return MenuOption::FullscreenMirror; }
    const QString & glowWhenSpeaking() { return MenuOption::GlowWhenSpeaking; }
    const QString & namesAboveHeads() { return MenuOption::NamesAboveHeads; }
    const QString & goToUser() { return MenuOption::GoToUser; }
    const QString & hmdTools() { return MenuOption::HMDTools; }
    const QString & increaseAvatarSize() { return MenuOption::IncreaseAvatarSize; }
    const QString & keyboardMotorControl() { return MenuOption::KeyboardMotorControl; }
    const QString & leapMotionOnHMD() { return MenuOption::LeapMotionOnHMD; }
    const QString & loadScript() { return MenuOption::LoadScript; }
    const QString & loadScriptURL() { return MenuOption::LoadScriptURL; }
    const QString & loadRSSDKFile() { return MenuOption::LoadRSSDKFile; }
    const QString & lodTools() { return MenuOption::LodTools; }
    const QString & login() { return MenuOption::Login; }
    const QString & log() { return MenuOption::Log; }
    const QString & lowVelocityFilter() { return MenuOption::LowVelocityFilter; }
    const QString & mirror() { return MenuOption::Mirror; }
    const QString & muteAudio() { return MenuOption::MuteAudio; }
    const QString & muteEnvironment() { return MenuOption::MuteEnvironment; }
    const QString & noFaceTracking() { return MenuOption::NoFaceTracking; }
    const QString & octreeStats() { return MenuOption::OctreeStats; }
    const QString & offAxisProjection() { return MenuOption::OffAxisProjection; }
    const QString & onlyDisplayTopTen() { return MenuOption::OnlyDisplayTopTen; }
    const QString & packageModel() { return MenuOption::PackageModel; }
    const QString & pair() { return MenuOption::Pair; }
    const QString & pipelineWarnings() { return MenuOption::PipelineWarnings; }
    const QString & preferences() { return MenuOption::Preferences; }
    const QString & quit() { return MenuOption::Quit; }
    const QString & reloadAllScripts() { return MenuOption::ReloadAllScripts; }
    const QString & renderBoundingCollisionShapes() { return MenuOption::RenderBoundingCollisionShapes; }
    const QString & renderFocusIndicator() { return MenuOption::RenderFocusIndicator; }
    const QString & renderHeadCollisionShapes() { return MenuOption::RenderHeadCollisionShapes; }
    const QString & renderLookAtVectors() { return MenuOption::RenderLookAtVectors; }
    const QString & renderSkeletonCollisionShapes() { return MenuOption::RenderSkeletonCollisionShapes; }
    const QString & renderTargetFramerate() { return MenuOption::RenderTargetFramerate; }
    const QString & renderTargetFramerateUnlimited() { return MenuOption::RenderTargetFramerateUnlimited; }
    const QString & renderTargetFramerate60() { return MenuOption::RenderTargetFramerate60; }
    const QString & renderTargetFramerate50() { return MenuOption::RenderTargetFramerate50; }
    const QString & renderTargetFramerate40() { return MenuOption::RenderTargetFramerate40; }
    const QString & renderTargetFramerate30() { return MenuOption::RenderTargetFramerate30; }
    const QString & renderTargetFramerateVSyncOn() { return MenuOption::RenderTargetFramerateVSyncOn; }
    const QString & renderResolution() { return MenuOption::RenderResolution; }
    const QString & renderResolutionOne() { return MenuOption::RenderResolutionOne; }
    const QString & renderResolutionTwoThird() { return MenuOption::RenderResolutionTwoThird; }
    const QString & renderResolutionHalf() { return MenuOption::RenderResolutionHalf; }
    const QString & renderResolutionThird() { return MenuOption::RenderResolutionThird; }
    const QString & renderResolutionQuarter() { return MenuOption::RenderResolutionQuarter; }
    const QString & renderAmbientLight() { return MenuOption::RenderAmbientLight; }
    const QString & renderAmbientLightGlobal() { return MenuOption::RenderAmbientLightGlobal; }
    const QString & renderAmbientLight0() { return MenuOption::RenderAmbientLight0; }
    const QString & renderAmbientLight1() { return MenuOption::RenderAmbientLight1; }
    const QString & renderAmbientLight2() { return MenuOption::RenderAmbientLight2; }
    const QString & renderAmbientLight3() { return MenuOption::RenderAmbientLight3; }
    const QString & renderAmbientLight4() { return MenuOption::RenderAmbientLight4; }
    const QString & renderAmbientLight5() { return MenuOption::RenderAmbientLight5; }
    const QString & renderAmbientLight6() { return MenuOption::RenderAmbientLight6; }
    const QString & renderAmbientLight7() { return MenuOption::RenderAmbientLight7; }
    const QString & renderAmbientLight8() { return MenuOption::RenderAmbientLight8; }
    const QString & renderAmbientLight9() { return MenuOption::RenderAmbientLight9; }
    const QString & resetAvatarSize() { return MenuOption::ResetAvatarSize; }
    const QString & resetDDETracking() { return MenuOption::ResetDDETracking; }
    const QString & resetSensors() { return MenuOption::ResetSensors; }
    const QString & runningScripts() { return MenuOption::RunningScripts; }
    const QString & runTimingTests() { return MenuOption::RunTimingTests; }
    const QString & scriptEditor() { return MenuOption::ScriptEditor; }
    const QString & scriptedMotorControl() { return MenuOption::ScriptedMotorControl; }
    const QString & showBordersEntityNodes() { return MenuOption::ShowBordersEntityNodes; }
    const QString & showIKConstraints() { return MenuOption::ShowIKConstraints; }
    const QString & simpleShadows() { return MenuOption::SimpleShadows; }
    const QString & sixenseEnabled() { return MenuOption::SixenseEnabled; }
    const QString & sixenseMouseInput() { return MenuOption::SixenseMouseInput; }
    const QString & sixenseLasers() { return MenuOption::SixenseLasers; }
    const QString & shiftHipsForIdleAnimations() { return MenuOption::ShiftHipsForIdleAnimations; }
    const QString & stars() { return MenuOption::Stars; }
    const QString & stats() { return MenuOption::Stats; }
    const QString & stereoAudio() { return MenuOption::StereoAudio; }
    const QString & stopAllScripts() { return MenuOption::StopAllScripts; }
    const QString & suppressShortTimings() { return MenuOption::SuppressShortTimings; }
    const QString & testPing() { return MenuOption::TestPing; }
    const QString & toolWindow() { return MenuOption::ToolWindow; }
    const QString & transmitterDrive() { return MenuOption::TransmitterDrive; }
    const QString & turnWithHead() { return MenuOption::TurnWithHead; }
    const QString & useAudioForMouth() { return MenuOption::UseAudioForMouth; }
    const QString & visibleToEveryone() { return MenuOption::VisibleToEveryone; }
    const QString & visibleToFriends() { return MenuOption::VisibleToFriends; }
    const QString & visibleToNoOne() { return MenuOption::VisibleToNoOne; }
    const QString & wireframe() { return MenuOption::Wireframe; }

public slots:
    void onTriggeredByName(const QString & name);
    void onToggledByName(const QString & name);

public:
    HifiMenu(QQuickItem * parent = nullptr);
    static void setToggleAction(const QString & name, std::function<void(bool)> f);
    static void setTriggerAction(const QString & name, std::function<void()> f);
private:
    static QHash<QString, std::function<void()>> triggerActions;
    static QHash<QString, std::function<void(bool)>> toggleActions;
};

#endif // hifi_MenuConstants_h




