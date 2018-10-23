import QtQuick 2.5
import QtQuick.Controls 1.4

import "../../../interface/resources/qml/hifi"

Menu {
    property var menuOption: MenuOption {}
    Item {
        Action {
            id: login;
            text: menuOption.login
        }
        Action {
            id: update;
            text: "Update";
            enabled: false
        }
        Action {
            id: crashReporter;
            text: "Crash Reporter...";
            enabled: false
        }
        Action {
            id: help;
            text: menuOption.help
            onTriggered: Application.showHelp()
        }
        Action {
            id: aboutApp;
            text: menuOption.aboutApp
        }
        Action {
            id: quit;
            text: menuOption.quit
        }

        ExclusiveGroup { id: renderResolutionGroup }
        Action {
            id: renderResolutionOne;
            exclusiveGroup: renderResolutionGroup;
            text: menuOption.renderResolutionOne;
            checkable: true;
            checked: true
        }
        Action {
            id: renderResolutionTwoThird;
            exclusiveGroup: renderResolutionGroup;
            text: menuOption.renderResolutionTwoThird;
            checkable: true
        }
        Action {
            id: renderResolutionHalf;
            exclusiveGroup: renderResolutionGroup;
            text: menuOption.renderResolutionHalf;
            checkable: true
        }
        Action {
            id: renderResolutionThird;
            exclusiveGroup: renderResolutionGroup;
            text: menuOption.renderResolutionThird;
            checkable: true
        }
        Action {
            id: renderResolutionQuarter;
            exclusiveGroup: renderResolutionGroup;
            text: menuOption.renderResolutionQuarter;
            checkable: true
        }

        ExclusiveGroup { id: ambientLightGroup }
        Action {
            id: renderAmbientLightGlobal;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLightGlobal;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight0;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight0;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight1;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight1;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight2;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight2;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight3;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight3;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight4;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight4;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight5;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight5;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight6;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight6;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight7;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight7;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight8;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight8;
            checkable: true;
            checked: true
        }
        Action {
            id: renderAmbientLight9;
            exclusiveGroup: ambientLightGroup;
            text: menuOption.renderAmbientLight9;
            checkable: true;
            checked: true
        }
        Action {
            id: preferences
            shortcut: StandardKey.Preferences
            text: menuOption.preferences
            onTriggered: dialogsManager.editPreferences()
        }

    }

    Menu {
        title: "File"
        MenuItem {
            action: login
        }
        MenuItem {
            action: update
        }
        MenuItem {
            action: help
        }
        MenuItem {
            action: crashReporter
        }
        MenuItem {
            action: aboutApp
        }
        MenuItem {
            action: quit
        }
    }

    Menu {
        title: "Edit"
        MenuItem {
            text: "Undo" }
        MenuItem {
            text: "Redo" }
        MenuItem {
            text: menuOption.runningScripts
        }
        MenuItem {
            text: menuOption.loadScript
        }
        MenuItem {
            text: menuOption.loadScriptURL
        }
        MenuItem {
            text: menuOption.stopAllScripts
        }
        MenuItem {
            text: menuOption.reloadAllScripts
        }
        MenuItem {
            text: menuOption.scriptEditor
        }
        MenuItem {
            text: menuOption.console_
        }
        MenuItem {
            text: menuOption.reloadContent
        }
        MenuItem {
            text: menuOption.packageModel
        }
    }

    Menu {
        title: "Audio"
        MenuItem {
            text: menuOption.muteAudio;
            checkable: true
        }
        MenuItem {
            text: menuOption.audioTools;
            checkable: true
        }
    }
    Menu {
        title: "Avatar"
        // Avatar > Attachments...
        MenuItem {
            text: menuOption.attachments
        }
        Menu {
            title: "Size"
            // Avatar > Size > Increase
            MenuItem {
                text: menuOption.increaseAvatarSize
            }
            // Avatar > Size > Decrease
            MenuItem {
                text: menuOption.decreaseAvatarSize
            }
            // Avatar > Size > Reset
            MenuItem {
                text: menuOption.resetAvatarSize
            }
        }
        // Avatar > Reset Sensors
        MenuItem {
            text: menuOption.resetSensors
        }
    }
    Menu {
        title: "Display"
    }
    Menu {
        title: "View"
        ExclusiveGroup {
            id: cameraModeGroup
        }

        MenuItem {
            text: menuOption.firstPerson;
            checkable: true;
            exclusiveGroup: cameraModeGroup
        }
        MenuItem {
            text: menuOption.thirdPerson;
            checkable: true;
            exclusiveGroup: cameraModeGroup
        }
        MenuItem {
            text: menuOption.fullscreenMirror;
            checkable: true;
            exclusiveGroup: cameraModeGroup
        }
        MenuItem {
            text: menuOption.independentMode;
            checkable: true;
            exclusiveGroup: cameraModeGroup
        }
        MenuItem {
            text: menuOption.cameraEntityMode;
            checkable: true;
            exclusiveGroup: cameraModeGroup
        }
        MenuSeparator{}
        MenuItem {
            text: menuOption.miniMirror;
            checkable: true
        }
    }
    Menu {
        title: "Navigate"
        MenuItem {
            text: "Home" }
        MenuItem {
            text: menuOption.addressBar
        }
        MenuItem {
            text: "Directory" }
        MenuItem {
            text: menuOption.copyAddress
        }
        MenuItem {
            text: menuOption.copyPath
        }
    }
    Menu {
        title: "Settings"
        MenuItem {
            text: "Advanced Menus" }
        MenuItem {
            text: "Developer Menus" }
        MenuItem {
            text: menuOption.preferences
        }
        MenuItem {
            text: "Avatar..." }
        MenuItem {
            text: "Audio..." }
        MenuItem {
            text: "LOD..." }
        MenuItem {
            text: menuOption.inputMenu
        }
    }
    Menu {
        title: "Developer"
        Menu {
            title: "Render"
            MenuItem {
                text: menuOption.atmosphere;
                checkable: true
            }
            MenuItem {
                text: menuOption.worldAxes;
                checkable: true
            }
            MenuItem {
                text: menuOption.debugAmbientOcclusion;
                checkable: true
            }
            MenuItem {
                text: menuOption.antialiasing;
                checkable: true
            }
            MenuItem {
                text: menuOption.stars;
                checkable: true
            }
            Menu {
                title: menuOption.renderAmbientLight
                MenuItem {
                    action: renderAmbientLightGlobal; }
                MenuItem {
                    action: renderAmbientLight0; }
                MenuItem {
                    action: renderAmbientLight1; }
                MenuItem {
                    action: renderAmbientLight2; }
                MenuItem {
                    action: renderAmbientLight3; }
                MenuItem {
                    action: renderAmbientLight4; }
                MenuItem {
                    action: renderAmbientLight5; }
                MenuItem {
                    action: renderAmbientLight6; }
                MenuItem {
                    action: renderAmbientLight7; }
                MenuItem {
                    action: renderAmbientLight8; }
                MenuItem {
                    action: renderAmbientLight9; }
            }
            MenuItem {
                text: menuOption.throttleFPSIfNotFocus;
                checkable: true
            }
            Menu {
                title: menuOption.renderResolution
                MenuItem {
                    action: renderResolutionOne
                }
                MenuItem {
                    action: renderResolutionTwoThird
                }
                MenuItem {
                    action: renderResolutionHalf
                }
                MenuItem {
                    action: renderResolutionThird
                }
                MenuItem {
                    action: renderResolutionQuarter
                }
            }
            MenuItem {
                text: menuOption.lodTools
            }
        }
        Menu {
            title: "Assets"
            MenuItem {
                text: menuOption.uploadAsset
            }
            MenuItem {
                text: menuOption.assetMigration
            }
        }
        Menu {
            title: "Avatar"
            Menu {
                title: "Face Tracking"
                MenuItem {
                    text: menuOption.noFaceTracking;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.faceshift;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.useCamera;
                    checkable: true
                }
                MenuSeparator{}
                MenuItem {
                    text: menuOption.binaryEyelidControl;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.coupleEyelids;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.useAudioForMouth;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.velocityFilter;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.calibrateCamera
                }
                MenuSeparator{}
                MenuItem {
                    text: menuOption.muteFaceTracking;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.autoMuteAudio;
                    checkable: true
                }
            }
            Menu {
                title: "Eye Tracking"
                MenuItem {
                    text: menuOption.sMIEyeTracking;
                    checkable: true
                }
                Menu {
                    title: "Calibrate"
                    MenuItem {
                        text: menuOption.onePointCalibration
                    }
                    MenuItem {
                        text: menuOption.threePointCalibration
                    }
                    MenuItem {
                        text: menuOption.fivePointCalibration
                    }
                }
                MenuItem {
                    text: menuOption.simulateEyeTracking;
                    checkable: true
                }
            }
            MenuItem {
                text: menuOption.avatarReceiveStats;
                checkable: true
            }
            MenuItem {
                text: menuOption.renderBoundingCollisionShapes;
                checkable: true
            }
            MenuItem {
                text: menuOption.renderLookAtVectors;
                checkable: true
            }
            MenuItem {
                text: menuOption.renderLookAtTargets;
                checkable: true
            }
            MenuItem {
                text: menuOption.renderFocusIndicator;
                checkable: true
            }
            MenuItem {
                text: menuOption.showWhosLookingAtMe;
                checkable: true
            }
            MenuItem {
                text: menuOption.fixGaze;
                checkable: true
            }
            MenuItem {
                text: menuOption.animDebugDrawDefaultPose;
                checkable: true
            }
            MenuItem {
                text: menuOption.animDebugDrawAnimPose;
                checkable: true
            }
            MenuItem {
                text: menuOption.animDebugDrawPosition;
                checkable: true
            }
            MenuItem {
                text: menuOption.meshVisible;
                checkable: true
            }
            MenuItem {
                text: menuOption.disableEyelidAdjustment;
                checkable: true
            }
            MenuItem {
                text: menuOption.turnWithHead;
                checkable: true
            }
            MenuItem {
                text: menuOption.keyboardMotorControl;
                checkable: true
            }
            MenuItem {
                text: menuOption.scriptedMotorControl;
                checkable: true
            }
            MenuItem {
                text: menuOption.enableCharacterController;
                checkable: true
            }
        }
        Menu {
            title: "Hands"
            MenuItem {
                text: menuOption.displayHandTargets;
                checkable: true
            }
            MenuItem {
                text: menuOption.lowVelocityFilter;
                checkable: true
            }
            Menu {
                title: "Leap Motion"
                MenuItem {
                    text: menuOption.leapMotionOnHMD;
                    checkable: true
                }
            }
        }
        Menu {
            title: "Entities"
            MenuItem {
                text: menuOption.octreeStats
            }
            MenuItem {
                text: menuOption.showRealtimeEntityStats;
                checkable: true
            }
        }
        Menu {
            title: "Network"
            MenuItem {
                text: menuOption.reloadContent
            }
            MenuItem {
                text: menuOption.disableNackPackets;
                checkable: true
            }
            MenuItem {
                text: menuOption.disableActivityLogger;
                checkable: true
            }
            MenuItem {
                text: menuOption.cachesSize
            }
            MenuItem {
                text: menuOption.diskCacheEditor
            }
            MenuItem {
                text: menuOption.showDSConnectTable
            }
            MenuItem {
                text: menuOption.bandwidthDetails
            }
        }
        Menu {
            title: "Timing"
            Menu {
                title: "Performance Timer"
                MenuItem {
                    text: menuOption.displayDebugTimingDetails;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.onlyDisplayTopTen;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.expandUpdateTiming;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.expandMyAvatarTiming;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.expandMyAvatarSimulateTiming;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.expandOtherAvatarTiming;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.expandPaintGLTiming;
                    checkable: true
                }
            }
            MenuItem {
                text: menuOption.frameTimer;
                checkable: true
            }
            MenuItem {
                text: menuOption.runTimingTests
            }
            MenuItem {
                text: menuOption.pipelineWarnings;
                checkable: true
            }
            MenuItem {
                text: menuOption.logExtraTimings;
                checkable: true
            }
            MenuItem {
                text: menuOption.suppressShortTimings;
                checkable: true
            }
        }
        Menu {
            title: "Audio"
            MenuItem {
                text: menuOption.audioNoiseReduction;
                checkable: true
            }
            MenuItem {
                text: menuOption.echoServerAudio;
                checkable: true
            }
            MenuItem {
                text: menuOption.echoLocalAudio;
                checkable: true
            }
            MenuItem {
                text: menuOption.muteEnvironment
            }
            Menu {
                title: "Audio"
                MenuItem {
                    text: menuOption.audioScope;
                    checkable: true
                }
                MenuItem {
                    text: menuOption.audioScopePause;
                    checkable: true
                }
                Menu {
                    title: "Display Frames"
                    ExclusiveGroup {
                        id: audioScopeFramesGroup
                    }
                    MenuItem {
                        exclusiveGroup: audioScopeFramesGroup;
                        text: menuOption.audioScopeFiveFrames;
                        checkable: true
                    }
                    MenuItem {
                        exclusiveGroup: audioScopeFramesGroup;
                        text: menuOption.audioScopeTwentyFrames;
                        checkable: true
                    }
                    MenuItem {
                        exclusiveGroup: audioScopeFramesGroup;
                        text: menuOption.audioScopeFiftyFrames;
                        checkable: true
                    }
                }
                MenuItem {
                    text: menuOption.audioNetworkStats
                }
            }
        }
        Menu {
            title: "Physics"
            MenuItem {
                text: menuOption.physicsShowOwned;
                checkable: true
            }
            MenuItem {
                text: menuOption.physicsShowHulls;
                checkable: true
            }
        }
        MenuItem {
            text: menuOption.displayCrashOptions;
            checkable: true
        }
        MenuItem {
            text: menuOption.crashInterface
        }
        MenuItem {
            text: menuOption.log
        }
        MenuItem {
            text: menuOption.stats;
            checkable: true
        }
    }
}
