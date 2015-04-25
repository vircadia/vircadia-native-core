import QtQuick 2.4
import QtQuick.Controls 1.3
import Hifi 1.0

// Currently for testing a pure QML replacement menu
Item {
    Item {
        objectName: "AllActions"
        Action {
            id: aboutApp
            objectName: "HifiAction_" + MenuConstants.AboutApp
            text: qsTr("About Interface")
        }

        //
        // File Menu
        //
        Action {
            id: login
            objectName: "HifiAction_" + MenuConstants.Login
            text: qsTr("Login")
        }
        Action {
            id: quit
            objectName: "HifiAction_" + MenuConstants.Quit
            text: qsTr("Quit")
            //shortcut: StandardKey.Quit
            shortcut: "Ctrl+Q"
        }

        // Scripts
        Action {
            id: loadScript
            objectName: "HifiAction_" + MenuConstants.LoadScript
            text: qsTr("Open and Run Script File...")
            shortcut: "Ctrl+O"
            onTriggered: {
                console.log("Shortcut ctrl+o hit");
            }
        }
        Action {
            id: loadScriptURL
            objectName: "HifiAction_" + MenuConstants.LoadScriptURL
            text: qsTr("Open and Run Script from URL...")
            shortcut: "Ctrl+Shift+O"
        }
        Action {
            id: reloadAllScripts
            objectName: "HifiAction_" + MenuConstants.ReloadAllScripts
            text: qsTr("Reload All Scripts")
        }
        Action {
            id: runningScripts
            objectName: "HifiAction_" + MenuConstants.RunningScripts
            text: qsTr("Running Scripts")
        }
        Action {
            id: stopAllScripts
            objectName: "HifiAction_" + MenuConstants.StopAllScripts
            text: qsTr("Stop All Scripts")
        }

        // Locations
        Action {
            id: bookmarkLocation
            objectName: "HifiAction_" + MenuConstants.BookmarkLocation
            text: qsTr("Bookmark Location")
        }
        Action {
            id: bookmarks
            objectName: "HifiAction_" + MenuConstants.Bookmarks
            text: qsTr("Bookmarks")
        }
        Action {
            id: addressBar
            objectName: "HifiAction_" + MenuConstants.AddressBar
            text: qsTr("Show Address Bar")
        }

        //
        // Edit menu
        //
        Action {
            id: undo
            text: "Undo"
            shortcut: StandardKey.Undo
        }

        Action {
            id: redo
            text: "Redo"
            shortcut: StandardKey.Redo
        }

        Action {
            id: animations
            objectName: "HifiAction_" + MenuConstants.Animations
            text: qsTr("Animations...")
        }
        Action {
            id: attachments
            objectName: "HifiAction_" + MenuConstants.Attachments
            text: qsTr("Attachments...")
        }
        
        //
        // Tools menu
        //
        Action {
            id: scriptEditor
            objectName: "HifiAction_" + MenuConstants.ScriptEditor
            text: qsTr("Script Editor...")
        }
        Action {
            id: controlWithSpeech
            objectName: "HifiAction_" + MenuConstants.ControlWithSpeech
            text: qsTr("Control With Speech")
        }
        Action {
            id: chat
            objectName: "HifiAction_" + MenuConstants.Chat
            text: qsTr("Chat...")
        }
        Action {
            id: addRemoveFriends
            objectName: "HifiAction_" + MenuConstants.AddRemoveFriends
            text: qsTr("Add/Remove Friends...")
        }
        ExclusiveGroup {
            Action {
                id: visibleToEveryone
                objectName: "HifiAction_" + MenuConstants.VisibleToEveryone
                text: qsTr("Everyone")
            }
            Action {
                id: visibleToFriends
                objectName: "HifiAction_" + MenuConstants.VisibleToFriends
                text: qsTr("Friends")
            }
            Action {
                id: visibleToNoOne
                objectName: "HifiAction_" + MenuConstants.VisibleToNoOne
                text: qsTr("No one")
            }
        }
        Action {
            id: toolWindow
            objectName: "HifiAction_" + MenuConstants.ToolWindow
            text: qsTr("Tool Window")
        }
        Action {
            id: javascriptConsole
            objectName: "HifiAction_" + MenuConstants.JavascriptConsole
            text: qsTr("Console...")
        }
        Action {
            id: resetSensors
            objectName: "HifiAction_" + MenuConstants.ResetSensors
            text: qsTr("Reset Sensors")
        }

        
        
        
        
        
        
        
        
        
        
        
        Action {
            id: alignForearmsWithWrists
            objectName: "HifiAction_" + MenuConstants.AlignForearmsWithWrists
            text: qsTr("Align Forearms with Wrists")
            checkable: true
        }
        Action {
            id: alternateIK
            objectName: "HifiAction_" + MenuConstants.AlternateIK
            text: qsTr("Alternate IK")
            checkable: true
        }
        Action {
            id: ambientOcclusion
            objectName: "HifiAction_" + MenuConstants.AmbientOcclusion
            text: qsTr("Ambient Occlusion")
            checkable: true
        }
        Action {
            id: atmosphere
            objectName: "HifiAction_" + MenuConstants.Atmosphere
            text: qsTr("Atmosphere")
        }
        Action {
            id: audioNoiseReduction
            objectName: "HifiAction_" + MenuConstants.AudioNoiseReduction
            text: qsTr("Audio Noise Reduction")
            checkable: true
        }
        Action {
            id: audioScope
            objectName: "HifiAction_" + MenuConstants.AudioScope
            text: qsTr("Show Scope")
            checkable: true
        }
        ExclusiveGroup {
            Action {
                id: audioScopeFiveFrames
                objectName: "HifiAction_" + MenuConstants.AudioScopeFiveFrames
                text: qsTr("Five")
                checkable: true
                checked: true
            }
            Action {
                id: audioScopeFiftyFrames
                objectName: "HifiAction_" + MenuConstants.AudioScopeFiftyFrames
                text: qsTr("Fifty")
                checkable: true
            }
            Action {
                id: audioScopeTwentyFrames
                objectName: "HifiAction_" + MenuConstants.AudioScopeTwentyFrames
                text: qsTr("Twenty")
                checkable: true
            }
        }
        Action {
            id: audioScopePause
            objectName: "HifiAction_" + MenuConstants.AudioScopePause
            text: qsTr("Pause Scope")
            checkable: true
        }
        Action {
            id: audioStats
            objectName: "HifiAction_" + MenuConstants.AudioStats
            text: qsTr("Audio Stats")
            checkable: true
        }
        Action {
            id: audioStatsShowInjectedStreams
            objectName: "HifiAction_" + MenuConstants.AudioStatsShowInjectedStreams
            text: qsTr("Audio Stats Show Injected Streams")
            checkable: true
        }
        Action {
            id: bandwidthDetails
            objectName: "HifiAction_" + MenuConstants.BandwidthDetails
            text: qsTr("Bandwidth Details")
            checkable: true
        }
        Action {
            id: blueSpeechSphere
            objectName: "HifiAction_" + MenuConstants.BlueSpeechSphere
            text: qsTr("Blue Sphere While Speaking")
            checkable: true
        }
        Action {
            id: cascadedShadows
            objectName: "HifiAction_" + MenuConstants.CascadedShadows
            text: qsTr("Cascaded")
        }
        Action {
            id: cachesSize
            objectName: "HifiAction_" + MenuConstants.CachesSize
            text: qsTr("RAM Caches Size")
        }
        Action {
            id: collisions
            objectName: "HifiAction_" + MenuConstants.Collisions
            text: qsTr("Collisions")
        }
        Action {
            id: copyAddress
            objectName: "HifiAction_" + MenuConstants.CopyAddress
            text: qsTr("Copy Address to Clipboard")
        }
        Action {
            id: copyPath
            objectName: "HifiAction_" + MenuConstants.CopyPath
            text: qsTr("Copy Path to Clipboard")
        }
        Action {
            id: decreaseAvatarSize
            objectName: "HifiAction_" + MenuConstants.DecreaseAvatarSize
            text: qsTr("Decrease Avatar Size")
        }
        Action {
            id: deleteBookmark
            objectName: "HifiAction_" + MenuConstants.DeleteBookmark
            text: qsTr("Delete Bookmark...")
        }
        Action {
            id: disableActivityLogger
            objectName: "HifiAction_" + MenuConstants.DisableActivityLogger
            text: qsTr("Disable Activity Logger")
        }
        Action {
            id: disableLightEntities
            objectName: "HifiAction_" + MenuConstants.DisableLightEntities
            text: qsTr("Disable Light Entities")
        }
        Action {
            id: disableNackPackets
            objectName: "HifiAction_" + MenuConstants.DisableNackPackets
            text: qsTr("Disable NACK Packets")
        }
        Action {
            id: diskCacheEditor
            objectName: "HifiAction_" + MenuConstants.DiskCacheEditor
            text: qsTr("Disk Cache Editor")
        }
        Action {
            id: displayHands
            objectName: "HifiAction_" + MenuConstants.DisplayHands
            text: qsTr("Show Hand Info")
        }
        Action {
            id: displayHandTargets
            objectName: "HifiAction_" + MenuConstants.DisplayHandTargets
            text: qsTr("Show Hand Targets")
        }
        Action {
            id: displayModelBounds
            objectName: "HifiAction_" + MenuConstants.DisplayModelBounds
            text: qsTr("Display Model Bounds")
        }
        Action {
            id: displayModelTriangles
            objectName: "HifiAction_" + MenuConstants.DisplayModelTriangles
            text: qsTr("Display Model Triangles")
        }
        Action {
            id: displayModelElementChildProxies
            objectName: "HifiAction_" + MenuConstants.DisplayModelElementChildProxies
            text: qsTr("Display Model Element Children")
        }
        Action {
            id: displayModelElementProxy
            objectName: "HifiAction_" + MenuConstants.DisplayModelElementProxy
            text: qsTr("Display Model Element Bounds")
        }
        Action {
            id: displayDebugTimingDetails
            objectName: "HifiAction_" + MenuConstants.DisplayDebugTimingDetails
            text: qsTr("Display Timing Details")
        }
        Action {
            id: dontDoPrecisionPicking
            objectName: "HifiAction_" + MenuConstants.DontDoPrecisionPicking
            text: qsTr("Don't Do Precision Picking")
        }
        Action {
            id: dontFadeOnOctreeServerChanges
            objectName: "HifiAction_" + MenuConstants.DontFadeOnOctreeServerChanges
            text: qsTr("Don't Fade In/Out on Octree Server Changes")
        }
        Action {
            id: dontRenderEntitiesAsScene
            objectName: "HifiAction_" + MenuConstants.DontRenderEntitiesAsScene
            text: qsTr("Don't Render Entities as Scene")
        }
        Action {
            id: echoLocalAudio
            objectName: "HifiAction_" + MenuConstants.EchoLocalAudio
            text: qsTr("Echo Local Audio")
        }
        Action {
            id: echoServerAudio
            objectName: "HifiAction_" + MenuConstants.EchoServerAudio
            text: qsTr("Echo Server Audio")
        }
        Action {
            id: editEntitiesHelp
            objectName: "HifiAction_" + MenuConstants.EditEntitiesHelp
            text: qsTr("Edit Entities Help...")
        }
        Action {
            id: enable3DTVMode
            objectName: "HifiAction_" + MenuConstants.Enable3DTVMode
            text: qsTr("Enable 3DTV Mode")
        }
        Action {
            id: enableCharacterController
            objectName: "HifiAction_" + MenuConstants.EnableCharacterController
            text: qsTr("Enable avatar collisions")
        }
        Action {
            id: enableGlowEffect
            objectName: "HifiAction_" + MenuConstants.EnableGlowEffect
            text: qsTr("Enable Glow Effect (Warning: Poor Oculus Performance)")
        }
        Action {
            id: enableVRMode
            objectName: "HifiAction_" + MenuConstants.EnableVRMode
            text: qsTr("Enable VR Mode")
        }
        Action {
            id: expandMyAvatarSimulateTiming
            objectName: "HifiAction_" + MenuConstants.ExpandMyAvatarSimulateTiming
            text: qsTr("Expand /myAvatar/simulation")
        }
        Action {
            id: expandMyAvatarTiming
            objectName: "HifiAction_" + MenuConstants.ExpandMyAvatarTiming
            text: qsTr("Expand /myAvatar")
        }
        Action {
            id: expandOtherAvatarTiming
            objectName: "HifiAction_" + MenuConstants.ExpandOtherAvatarTiming
            text: qsTr("Expand /otherAvatar")
        }
        Action {
            id: expandPaintGLTiming
            objectName: "HifiAction_" + MenuConstants.ExpandPaintGLTiming
            text: qsTr("Expand /paintGL")
        }
        Action {
            id: expandUpdateTiming
            objectName: "HifiAction_" + MenuConstants.ExpandUpdateTiming
            text: qsTr("Expand /update")
        }
        Action {
            id: faceshift
            objectName: "HifiAction_" + MenuConstants.Faceshift
            text: qsTr("Faceshift")
        }
        Action {
            id: filterSixense
            objectName: "HifiAction_" + MenuConstants.FilterSixense
            text: qsTr("Smooth Sixense Movement")
        }
        Action {
            id: firstPerson
            objectName: "HifiAction_" + MenuConstants.FirstPerson
            text: qsTr("First Person")
        }
        Action {
            id: frameTimer
            objectName: "HifiAction_" + MenuConstants.FrameTimer
            text: qsTr("Show Timer")
        }
        Action {
            id: fullscreen
            objectName: "HifiAction_" + MenuConstants.Fullscreen
            text: qsTr("Fullscreen")
        }
        Action {
            id: fullscreenMirror
            objectName: "HifiAction_" + MenuConstants.FullscreenMirror
            text: qsTr("Fullscreen Mirror")
        }
        Action {
            id: glowWhenSpeaking
            objectName: "HifiAction_" + MenuConstants.GlowWhenSpeaking
            text: qsTr("Glow When Speaking")
        }
        Action {
            id: namesAboveHeads
            objectName: "HifiAction_" + MenuConstants.NamesAboveHeads
            text: qsTr("Names Above Heads")
        }
        Action {
            id: goToUser
            objectName: "HifiAction_" + MenuConstants.GoToUser
            text: qsTr("Go To User")
        }
        Action {
            id: hMDTools
            objectName: "HifiAction_" + MenuConstants.HMDTools
            text: qsTr("HMD Tools")
        }
        Action {
            id: increaseAvatarSize
            objectName: "HifiAction_" + MenuConstants.IncreaseAvatarSize
            text: qsTr("Increase Avatar Size")
        }
        Action {
            id: keyboardMotorControl
            objectName: "HifiAction_" + MenuConstants.KeyboardMotorControl
            text: qsTr("Enable Keyboard Motor Control")
        }
        Action {
            id: leapMotionOnHMD
            objectName: "HifiAction_" + MenuConstants.LeapMotionOnHMD
            text: qsTr("Leap Motion on HMD")
        }
        Action {
            id: loadRSSDKFile
            objectName: "HifiAction_" + MenuConstants.LoadRSSDKFile
            text: qsTr("Load .rssdk file")
        }
        Action {
            id: lodTools
            objectName: "HifiAction_" + MenuConstants.LodTools
            text: qsTr("LOD Tools")
        }
        Action {
            id: log
            objectName: "HifiAction_" + MenuConstants.Log
            text: qsTr("Log")
        }
        Action {
            id: lowVelocityFilter
            objectName: "HifiAction_" + MenuConstants.LowVelocityFilter
            text: qsTr("Low Velocity Filter")
        }
        Action {
            id: mirror
            objectName: "HifiAction_" + MenuConstants.Mirror
            text: qsTr("Mirror")
        }
        Action {
            id: muteAudio
            objectName: "HifiAction_" + MenuConstants.MuteAudio
            text: qsTr("Mute Microphone")
        }
        Action {
            id: muteEnvironment
            objectName: "HifiAction_" + MenuConstants.MuteEnvironment
            text: qsTr("Mute Environment")
        }
        Action {
            id: noFaceTracking
            objectName: "HifiAction_" + MenuConstants.NoFaceTracking
            text: qsTr("None")
        }
        Action {
            id: noShadows
            objectName: "HifiAction_" + MenuConstants.NoShadows
            text: qsTr("None")
        }
        Action {
            id: octreeStats
            objectName: "HifiAction_" + MenuConstants.OctreeStats
            text: qsTr("Entity Statistics")
        }
        Action {
            id: offAxisProjection
            objectName: "HifiAction_" + MenuConstants.OffAxisProjection
            text: qsTr("Off-Axis Projection")
        }
        Action {
            id: onlyDisplayTopTen
            objectName: "HifiAction_" + MenuConstants.OnlyDisplayTopTen
            text: qsTr("Only Display Top Ten")
        }
        Action {
            id: packageModel
            objectName: "HifiAction_" + MenuConstants.PackageModel
            text: qsTr("Package Model...")
        }
        Action {
            id: pair
            objectName: "HifiAction_" + MenuConstants.Pair
            text: qsTr("Pair")
        }
        Action {
            id: pipelineWarnings
            objectName: "HifiAction_" + MenuConstants.PipelineWarnings
            text: qsTr("Log Render Pipeline Warnings")
        }
        Action {
            id: preferences
            objectName: "HifiAction_" + MenuConstants.Preferences
            text: qsTr("Preferences...")
        }
        Action {
            id: renderBoundingCollisionShapes
            objectName: "HifiAction_" + MenuConstants.RenderBoundingCollisionShapes
            text: qsTr("Show Bounding Collision Shapes")
            checkable: true
        }
        Action {
            id: renderFocusIndicator
            objectName: "HifiAction_" + MenuConstants.RenderFocusIndicator
            text: qsTr("Show Eye Focus")
            checkable: true
        }
        Action {
            id: renderHeadCollisionShapes
            objectName: "HifiAction_" + MenuConstants.RenderHeadCollisionShapes
            text: qsTr("Show Head Collision Shapes")
            checkable: true
        }
        Action {
            id: renderLookAtVectors
            objectName: "HifiAction_" + MenuConstants.RenderLookAtVectors
            text: qsTr("Show Look-at Vectors")
            checkable: true
        }
        Action {
            id: renderSkeletonCollisionShapes
            objectName: "HifiAction_" + MenuConstants.RenderSkeletonCollisionShapes
            text: qsTr("Show Skeleton Collision Shapes")
            checkable: true
        }
        ExclusiveGroup {
            Action {
                id: renderTargetFramerateUnlimited
                objectName: "HifiAction_" + MenuConstants.RenderTargetFramerateUnlimited
                text: qsTr("Unlimited")
                checked: true
                checkable: true
            }
            Action {
                id: renderTargetFramerate60
                objectName: "HifiAction_" + MenuConstants.RenderTargetFramerate60
                text: qsTr("60")
                checkable: true
            }
            Action {
                id: renderTargetFramerate50
                objectName: "HifiAction_" + MenuConstants.RenderTargetFramerate50
                text: qsTr("50")
                checkable: true
            }
            Action {
                id: renderTargetFramerate40
                objectName: "HifiAction_" + MenuConstants.RenderTargetFramerate40
                text: qsTr("40")
                checkable: true
            }
            Action {
                id: renderTargetFramerate30
                objectName: "HifiAction_" + MenuConstants.RenderTargetFramerate30
                text: qsTr("30")
                checkable: true
            }
        }
        Action {
            id: renderTargetFramerateVSyncOn
            objectName: "HifiAction_" + MenuConstants.RenderTargetFramerateVSyncOn
            text: qsTr("V-Sync On")
            checkable: true
        }
        Action {
            id: renderResolution
            objectName: "HifiAction_" + MenuConstants.RenderResolution
            text: qsTr("Scale Resolution")
        }
        ExclusiveGroup {
            Action {
                id: renderResolutionOne
                objectName: "HifiAction_" + MenuConstants.RenderResolutionOne
                text: qsTr("1")
                checkable: true
                checked: true
            }
            Action {
                id: renderResolutionTwoThird
                objectName: "HifiAction_" + MenuConstants.RenderResolutionTwoThird
                text: qsTr("2/3")
                checkable: true
            }
            Action {
                id: renderResolutionHalf
                objectName: "HifiAction_" + MenuConstants.RenderResolutionHalf
                text: qsTr("1/2")
                checkable: true
            }
            Action {
                id: renderResolutionThird
                objectName: "HifiAction_" + MenuConstants.RenderResolutionThird
                text: qsTr("1/3")
                checkable: true
            }
            Action {
                id: renderResolutionQuarter
                objectName: "HifiAction_" + MenuConstants.RenderResolutionQuarter
                text: qsTr("1/4")
                checkable: true
            }
        }
        Action {
            id: renderAmbientLight
            objectName: "HifiAction_" + MenuConstants.RenderAmbientLight
            text: qsTr("Ambient Light")
        }
        ExclusiveGroup {
            Action {
                id: renderAmbientLightGlobal
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLightGlobal
                text: qsTr("Global")
                checked: true
                checkable: true
            }
            Action {
                id: renderAmbientLight0
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight0
                text: qsTr("OLD_TOWN_SQUARE")
                checkable: true
            }
            Action {
                id: renderAmbientLight1
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight1
                text: qsTr("GRACE_CATHEDRAL")
                checkable: true
            }
            Action {
                id: renderAmbientLight2
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight2
                text: qsTr("EUCALYPTUS_GROVE")
                checkable: true
            }
            Action {
                id: renderAmbientLight3
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight3
                text: qsTr("ST_PETERS_BASILICA")
                checkable: true
            }
            Action {
                id: renderAmbientLight4
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight4
                text: qsTr("UFFIZI_GALLERY")
                checkable: true
            }
            Action {
                id: renderAmbientLight5
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight5
                text: qsTr("GALILEOS_TOMB")
                checkable: true
            }
            Action {
                id: renderAmbientLight6
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight6
                text: qsTr("VINE_STREET_KITCHEN")
                checkable: true
            }
            Action {
                id: renderAmbientLight7
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight7
                text: qsTr("BREEZEWAY")
                checkable: true
            }
            Action {
                id: renderAmbientLight8
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight8
                text: qsTr("CAMPUS_SUNSET")
                checkable: true
            }
            Action {
                id: renderAmbientLight9
                objectName: "HifiAction_" + MenuConstants.RenderAmbientLight9
                text: qsTr("FUNSTON_BEACH_SUNSET")
                checkable: true
            }
        }
        Action {
            id: resetAvatarSize
            objectName: "HifiAction_" + MenuConstants.ResetAvatarSize
            text: qsTr("Reset Avatar Size")
        }
        Action {
            id: runTimingTests
            objectName: "HifiAction_" + MenuConstants.RunTimingTests
            text: qsTr("Run Timing Tests")
        }
        Action {
            id: scriptedMotorControl
            objectName: "HifiAction_" + MenuConstants.ScriptedMotorControl
            text: qsTr("Enable Scripted Motor Control")
        }
        Action {
            id: showBordersEntityNodes
            objectName: "HifiAction_" + MenuConstants.ShowBordersEntityNodes
            text: qsTr("Show Entity Nodes")
        }
        Action {
            id: showIKConstraints
            objectName: "HifiAction_" + MenuConstants.ShowIKConstraints
            text: qsTr("Show IK Constraints")
        }
        Action {
            id: simpleShadows
            objectName: "HifiAction_" + MenuConstants.SimpleShadows
            text: qsTr("Simple")
        }
        Action {
            id: sixenseEnabled
            objectName: "HifiAction_" + MenuConstants.SixenseEnabled
            text: qsTr("Enable Hydra Support")
        }
        Action {
            id: sixenseMouseInput
            objectName: "HifiAction_" + MenuConstants.SixenseMouseInput
            text: qsTr("Enable Sixense Mouse Input")
        }
        Action {
            id: sixenseLasers
            objectName: "HifiAction_" + MenuConstants.SixenseLasers
            text: qsTr("Enable Sixense UI Lasers")
        }
        Action {
            id: shiftHipsForIdleAnimations
            objectName: "HifiAction_" + MenuConstants.ShiftHipsForIdleAnimations
            text: qsTr("Shift hips for idle animations")
        }
        Action {
            id: stars
            objectName: "HifiAction_" + MenuConstants.Stars
            text: qsTr("Stars")
        }
        Action {
            id: stats
            objectName: "HifiAction_" + MenuConstants.Stats
            text: qsTr("Stats")
        }
        Action {
            id: stereoAudio
            objectName: "HifiAction_" + MenuConstants.StereoAudio
            text: qsTr("Stereo Audio (disables spatial sound)")
        }
        Action {
            id: suppressShortTimings
            objectName: "HifiAction_" + MenuConstants.SuppressShortTimings
            text: qsTr("Suppress Timings Less than 10ms")
        }
        Action {
            id: testPing
            objectName: "HifiAction_" + MenuConstants.TestPing
            text: qsTr("Test Ping")
        }
        Action {
            id: transmitterDrive
            objectName: "HifiAction_" + MenuConstants.TransmitterDrive
            text: qsTr("Transmitter Drive")
        }
        Action {
            id: turnWithHead
            objectName: "HifiAction_" + MenuConstants.TurnWithHead
            text: qsTr("Turn using Head")
        }
        Action {
            id: useAudioForMouth
            objectName: "HifiAction_" + MenuConstants.UseAudioForMouth
            text: qsTr("Use Audio for Mouth")
        }
        Action {
            id: useCamera
            objectName: "HifiAction_" + MenuConstants.UseCamera
            text: qsTr("Use Camera")
        }
        Action {
            id: velocityFilter
            objectName: "HifiAction_" + MenuConstants.VelocityFilter
            text: qsTr("Velocity Filter")
        }
        Action {
            id: wireframe
            objectName: "HifiAction_" + MenuConstants.Wireframe
            text: qsTr("Wireframe")
        }
    }

    Menu {
        objectName: "rootMenu";
        Menu {
            title: "File"
            MenuItem {
                action: login
            }
            MenuItem {
                action: aboutApp
            }
            MenuSeparator {} MenuItem { text: "Scripts"; enabled: false }
            MenuItem { action: loadScript; }
            MenuItem { action: loadScriptURL; }
            MenuItem { action: stopAllScripts; }
            MenuItem { action: reloadAllScripts; }
            MenuItem { action: runningScripts; }
            MenuSeparator {} MenuItem { text: "Locations"; enabled: false }
            MenuItem { action: addressBar; }
            MenuItem { action: copyAddress; }
            MenuItem { action: copyPath; }
            MenuSeparator {}
            MenuItem { action: quit; }
        }
        Menu {
            title: "Edit"
            MenuItem { action: preferences; }
            MenuItem { action: animations; }
        }
        Menu {
            title: "Tools"
            MenuItem { action: scriptEditor; }
            MenuItem { action: controlWithSpeech; }
            MenuItem { action: chat; }
            MenuItem { action: addRemoveFriends; }
            Menu {
                title: "I Am Visible To"
                MenuItem { action: visibleToEveryone; }
                MenuItem { action: visibleToFriends; }
                MenuItem { action: visibleToNoOne; }
            }
            MenuItem { action: toolWindow; }
            MenuItem { action: javascriptConsole; }
            MenuItem { action: resetSensors; }
            MenuItem { action: packageModel; }
        }
        Menu {
            title: "Avatar"
            Menu {
                title: "Size"
                MenuItem { action: increaseAvatarSize; }
                MenuItem { action: decreaseAvatarSize; }
                MenuItem { action: resetAvatarSize; }
            }
            MenuItem { action: keyboardMotorControl; }
            MenuItem { action: scriptedMotorControl; }
            MenuItem { action: namesAboveHeads; }
            MenuItem { action: glowWhenSpeaking; }
            MenuItem { action: blueSpeechSphere; }
            MenuItem { action: enableCharacterController; }
            MenuItem { action: shiftHipsForIdleAnimations; }
        }
        Menu {
            title: "View"
            MenuItem { action: fullscreen; }
            MenuItem { action: firstPerson; }
            MenuItem { action: mirror; }
            MenuItem { action: fullscreenMirror; }
            MenuItem { action: hMDTools; }
            MenuItem { action: enableVRMode; }
            MenuItem { action: enable3DTVMode; }
            MenuItem { action: showBordersEntityNodes; }
            MenuItem { action: offAxisProjection; }
            MenuItem { action: turnWithHead; }
            MenuItem { action: stats; }
            MenuItem { action: log; }
            MenuItem { action: bandwidthDetails; }
            MenuItem { action: octreeStats; }
        }
        Menu {
            title: "Developer"
            Menu {
                title: "Render"
                MenuItem { action: atmosphere; }
                MenuItem { action: ambientOcclusion; }
                MenuItem { action: dontFadeOnOctreeServerChanges; }
                Menu {
                    title: "Ambient Light"
                    MenuItem { action: renderAmbientLightGlobal; }
                    MenuItem { action: renderAmbientLight0; }
                    MenuItem { action: renderAmbientLight1; }
                    MenuItem { action: renderAmbientLight2; }
                    MenuItem { action: renderAmbientLight3; }
                    MenuItem { action: renderAmbientLight4; }
                    MenuItem { action: renderAmbientLight5; }
                    MenuItem { action: renderAmbientLight6; }
                    MenuItem { action: renderAmbientLight7; }
                    MenuItem { action: renderAmbientLight8; }
                    MenuItem { action: renderAmbientLight9; }
                }
                Menu {
                    title: "Shadows"
                    MenuItem { action: noShadows; }
                    MenuItem { action: simpleShadows; }
                    MenuItem { action: cascadedShadows; }
                }
                Menu {
                    title: "Framerate"
                    MenuItem { action: renderTargetFramerateUnlimited; }
                    MenuItem { action: renderTargetFramerate60; }
                    MenuItem { action: renderTargetFramerate50; }
                    MenuItem { action: renderTargetFramerate40; }
                    MenuItem { action: renderTargetFramerate30; }
                }
                MenuItem { action: renderTargetFramerateVSyncOn; }
                Menu {
                    title: "Scale Resolution"
                    MenuItem { action: renderResolutionOne; }
                    MenuItem { action: renderResolutionTwoThird; }
                    MenuItem { action: renderResolutionHalf; }
                    MenuItem { action: renderResolutionThird; }
                    MenuItem { action: renderResolutionQuarter; }
                }
                MenuItem { action: stars; }
                MenuItem { action: enableGlowEffect; }
                MenuItem { action: wireframe; }
                MenuItem { action: lodTools; }
            }
            Menu {
                title: "Avatar"
                Menu {
                    title: "Face Tracking"
                    MenuItem { action: noFaceTracking; }
                    MenuItem { action: faceshift; }
                    MenuItem { action: useCamera; }
                    MenuItem { action: useAudioForMouth; }
                    MenuItem { action: velocityFilter; }
                }
                MenuItem { action: renderSkeletonCollisionShapes; }
                MenuItem { action: renderHeadCollisionShapes; }
                MenuItem { action: renderBoundingCollisionShapes; }
                MenuItem { action: renderLookAtVectors; }
                MenuItem { action: renderFocusIndicator; }
            }
            Menu {
                title: "Hands"
                MenuItem { action: alignForearmsWithWrists; }
                MenuItem { action: alternateIK; }
                MenuItem { action: displayHands; }
                MenuItem { action: displayHandTargets; }
                MenuItem { action: showIKConstraints; }
                Menu {
                    title: "Sixense"
                    MenuItem { action: sixenseEnabled; }
                    MenuItem { action: filterSixense; }
                    MenuItem { action: lowVelocityFilter; }
                    MenuItem { action: sixenseMouseInput; }
                    MenuItem { action: sixenseLasers; }
                }
                Menu {
                    title: "Leap Motion"
                    MenuItem { action: leapMotionOnHMD; }
                }
                MenuItem { action: loadRSSDKFile; }
            }
            Menu {
                title: "Network"
                MenuItem { action: disableNackPackets; }
                MenuItem { action: disableActivityLogger; }
                MenuItem { action: cachesSize; }
                MenuItem { action: diskCacheEditor; }
            }
            Menu {
                title: "Timing and Stats"
                Menu {
                    title: "Performance Timer"
                    MenuItem { action: displayDebugTimingDetails; }
                    MenuItem { action: onlyDisplayTopTen; }
                    MenuItem { action: expandUpdateTiming; }
                    MenuItem { action: expandMyAvatarTiming; }
                    MenuItem { action: expandMyAvatarSimulateTiming; }
                    MenuItem { action: expandOtherAvatarTiming; }
                    MenuItem { action: expandPaintGLTiming; }
                }
                MenuItem { action: testPing; }
                MenuItem { action: frameTimer; }
                MenuItem { action: runTimingTests; }
                MenuItem { action: pipelineWarnings; }
                MenuItem { action: suppressShortTimings; }
            }
            Menu {
                title: "Audio"
                MenuItem { action: audioNoiseReduction; }
                MenuItem { action: echoServerAudio; }
                MenuItem { action: echoLocalAudio; }
                MenuItem { action: stereoAudio; }
                MenuItem { action: muteAudio; }
                MenuItem { action: muteEnvironment; }
                Menu { 
                    title: "Audio Scope" 
                    MenuItem { action: audioScope; }
                    MenuItem { action: audioScopePause; }
                    MenuSeparator {} MenuItem { text: "Audio Scope";  enabled: false; }
                    MenuItem { action: audioScopeFiveFrames; }
                    MenuItem { action: audioScopeTwentyFrames; }
                    MenuItem { action: audioScopeFiftyFrames; }
                }
                MenuItem { action: audioStats; }
                MenuItem { action: audioStatsShowInjectedStreams; }
            }
        }
        Menu {
            title: "Help"
            MenuItem { action: editEntitiesHelp; }
            MenuItem { action: aboutApp; }
        }
    } 
}
