"use strict";
/* jslint vars: true, plusplus: true */

//
//  simplifiedUI.js
//
//  Authors: Wayne Chen & Zach Fox
//  Created: 2019-05-01
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// START CONFIG OPTIONS
var TOOLBAR_NAME = "com.highfidelity.interface.toolbar.system";
var DEFAULT_SCRIPTS_PATH_PREFIX = ScriptDiscoveryService.defaultScriptsPath + "/";
// END CONFIG OPTIONS


var MENU_NAMES = ["File", "Edit", "Display", "View", "Navigate", "Settings", "Developer", "Help"];
var keepMenusSetting = Settings.getValue("simplifiedUI/keepMenus", false);
function maybeRemoveDesktopMenu() {    
    if (!keepMenusSetting) {
        MENU_NAMES.forEach(function(menu) {
            Menu.removeMenu(menu);
        });
    }
}


function handleUpdateAvatarThumbnailURL(avatarThumbnailURL) {
    if (topBarWindow) {
        topBarWindow.sendToQml({
            "source": "simplifiedUI.js",
            "method": "updateAvatarThumbnailURL",
            "data": {
                "avatarThumbnailURL": avatarThumbnailURL
            }
        });
    }
}


var AVATAR_APP_MESSAGE_SOURCE = "AvatarApp.qml";
function onMessageFromAvatarApp(message) {
    if (message.source !== AVATAR_APP_MESSAGE_SOURCE) {
        return;
    }

    switch (message.method) {
        case "updateAvatarThumbnailURL":
            handleUpdateAvatarThumbnailURL(message.data.avatarThumbnailURL);
            break;

        default:
            console.log("Unrecognized message from " + AVATAR_APP_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}


function onAvatarAppClosed() {
    if (avatarAppWindow) {
        avatarAppWindow.fromQml.disconnect(onMessageFromAvatarApp);
        avatarAppWindow.closed.disconnect(onAvatarAppClosed);
    }
    avatarAppWindow = false;
}


var AVATAR_APP_QML_PATH = Script.resourcesPath() + "qml/hifi/simplifiedUI/avatarApp/AvatarApp.qml";
var AVATAR_APP_WINDOW_TITLE = "Your Profile";
var AVATAR_APP_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var AVATAR_APP_WIDTH_PX = 480;
var AVATAR_APP_HEIGHT_PX = 615;
var avatarAppWindow = false;
var POPOUT_SAFE_MARGIN_X = 30;
var POPOUT_SAFE_MARGIN_Y = 30;
var AVATAR_APP_WINDOW_FLAGS = 0x00000001 | // Qt::Window
    0x00001000 | // Qt::WindowTitleHint
    0x00002000 | // Qt::WindowSystemMenuHint
    0x08000000 | // Qt::WindowCloseButtonHint
    0x00008000 | // Qt::WindowMaximizeButtonHint
    0x00004000; // Qt::WindowMinimizeButtonHint
function toggleAvatarApp() {
    if (avatarAppWindow) {
        avatarAppWindow.close();
        // This really shouldn't be necessary.
        // This signal really should automatically be called by the signal handler set up below.
        // But fixing that requires an engine change, so this workaround will do.
        onAvatarAppClosed();
        return;
    }

    avatarAppWindow = Desktop.createWindow(AVATAR_APP_QML_PATH, {
        title: AVATAR_APP_WINDOW_TITLE,
        presentationMode: AVATAR_APP_PRESENTATION_MODE,
        size: {
            x: AVATAR_APP_WIDTH_PX,
            y: AVATAR_APP_HEIGHT_PX
        },
        position: {
            x: Math.max(Window.x + POPOUT_SAFE_MARGIN_X, Window.x + Window.innerWidth / 2 - AVATAR_APP_WIDTH_PX / 2),
            y: Math.max(Window.y + POPOUT_SAFE_MARGIN_Y, Window.y + Window.innerHeight / 2 - AVATAR_APP_HEIGHT_PX / 2)
        },
        overrideFlags: AVATAR_APP_WINDOW_FLAGS
    });

    avatarAppWindow.fromQml.connect(onMessageFromAvatarApp);
    avatarAppWindow.closed.connect(onAvatarAppClosed);
}


function handleAvatarNametagMode(newAvatarNametagMode) {
    nametag.handleAvatarNametagMode(newAvatarNametagMode);
}


var SETTINGS_APP_MESSAGE_SOURCE = "SettingsApp.qml";
function onMessageFromSettingsApp(message) {
    if (message.source !== SETTINGS_APP_MESSAGE_SOURCE) {
        return;
    }

    switch (message.method) {
        /* 
            This calls an update methon on the module apis because I can't get a signal from Settings.QML to the emoji.js without it being a module like
            nametag.  
        */
        case "handleAvatarNametagMode":
            handleAvatarNametagMode(message.avatarNametagMode);
            break;
        default:
            console.log("Unrecognized message from " + SETTINGS_APP_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}


function onSettingsAppClosed() {
    if (settingsAppWindow) {
        settingsAppWindow.fromQml.disconnect(onMessageFromSettingsApp);
        settingsAppWindow.closed.disconnect(onSettingsAppClosed);
    }
    settingsAppWindow = false;
}


var SETTINGS_APP_QML_PATH = Script.resourcesPath() + "qml/hifi/simplifiedUI/settingsApp/SettingsApp.qml";
var SETTINGS_APP_WINDOW_TITLE = "Settings";
var SETTINGS_APP_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var SETTINGS_APP_WIDTH_PX = 480;
var SETTINGS_APP_HEIGHT_PX = 615;
var SETTINGS_APP_WINDOW_FLAGS = 0x00000001 | // Qt::Window
    0x00001000 | // Qt::WindowTitleHint
    0x00002000 | // Qt::WindowSystemMenuHint
    0x08000000 | // Qt::WindowCloseButtonHint
    0x00008000 | // Qt::WindowMaximizeButtonHint
    0x00004000; // Qt::WindowMinimizeButtonHint
var SETTINGS_APP_RIGHT_MARGIN = 48;
var settingsAppWindow = false;
function toggleSettingsApp() {
    if (settingsAppWindow) {
        settingsAppWindow.close();
        // This really shouldn't be necessary.
        // This signal really should automatically be called by the signal handler set up below.
        // But fixing that requires an engine change, so this workaround will do.
        onSettingsAppClosed();
        return;
    }

    settingsAppWindow = Desktop.createWindow(SETTINGS_APP_QML_PATH, {
        title: SETTINGS_APP_WINDOW_TITLE,
        presentationMode: SETTINGS_APP_PRESENTATION_MODE,
        size: {
            x: SETTINGS_APP_WIDTH_PX,
            y: SETTINGS_APP_HEIGHT_PX
        },
        position: {
            x: Window.x + Window.innerWidth - SETTINGS_APP_WIDTH_PX - SETTINGS_APP_RIGHT_MARGIN,
            y: Math.max(Window.y + POPOUT_SAFE_MARGIN_Y, Window.y + Window.innerHeight / 2 - SETTINGS_APP_HEIGHT_PX / 2)
        },
        overrideFlags: SETTINGS_APP_WINDOW_FLAGS
    });

    settingsAppWindow.fromQml.connect(onMessageFromSettingsApp);
    settingsAppWindow.closed.connect(onSettingsAppClosed);
}


function handleGoToAudioSettings() {
    if (!settingsAppWindow) {
        toggleSettingsApp();
    }

    settingsAppWindow.sendToQml({
        "source": "simplifiedUI.js",
        "method": "goToSettingsTab",
        "data": {
            "settingsTab": "audio"
        }
    });
}


var HELP_APP_MESSAGE_SOURCE = "HelpApp.qml";
function onMessageFromHelpApp(message) {
    if (message.source !== HELP_APP_MESSAGE_SOURCE) {
        return;
    }

    switch (message.method) {
        case "goToAudioSettings":
            handleGoToAudioSettings();
            break;

        default:
            console.log("Unrecognized message from " + HELP_APP_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}


function onHelpAppClosed() {
    if (helpAppWindow) {
        helpAppWindow.fromQml.disconnect(onMessageFromHelpApp);
        helpAppWindow.closed.disconnect(onHelpAppClosed);
    }
    helpAppWindow = false;
}


var HELP_APP_QML_PATH = Script.resourcesPath() + "qml/hifi/simplifiedUI/helpApp/HelpApp.qml";
var HELP_APP_WINDOW_TITLE = "Help";
var HELP_APP_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var HELP_APP_WIDTH_PX = 480;
var HELP_APP_HEIGHT_PX = 615;
var HELP_APP_WINDOW_FLAGS = 0x00000001 | // Qt::Window
    0x00001000 | // Qt::WindowTitleHint
    0x00002000 | // Qt::WindowSystemMenuHint
    0x08000000 | // Qt::WindowCloseButtonHint
    0x00008000 | // Qt::WindowMaximizeButtonHint
    0x00004000; // Qt::WindowMinimizeButtonHint
var helpAppWindow = false;
function toggleHelpApp() {
    if (helpAppWindow) {
        helpAppWindow.close();
        // This really shouldn't be necessary.
        // This signal really should automatically be called by the signal handler set up below.
        // But fixing that requires an engine change, so this workaround will do.
        onHelpAppClosed();
        return;
    }

    helpAppWindow = Desktop.createWindow(HELP_APP_QML_PATH, {
        title: HELP_APP_WINDOW_TITLE,
        presentationMode: HELP_APP_PRESENTATION_MODE,
        size: {
            x: HELP_APP_WIDTH_PX,
            y: HELP_APP_HEIGHT_PX
        },
        position: {
            x: Math.max(Window.x + POPOUT_SAFE_MARGIN_X, Window.x + Window.innerWidth / 2 - HELP_APP_WIDTH_PX / 2),
            y: Math.max(Window.y + POPOUT_SAFE_MARGIN_Y, Window.y + Window.innerHeight / 2 - HELP_APP_HEIGHT_PX / 2)
        },
        overrideFlags: HELP_APP_WINDOW_FLAGS
    });

    helpAppWindow.fromQml.connect(onMessageFromHelpApp);
    helpAppWindow.closed.connect(onHelpAppClosed);
}


function maybeDeleteOutputDeviceMutedOverlay() {
    if (outputDeviceMutedOverlay) {
        Overlays.deleteOverlay(outputDeviceMutedOverlay);
        outputDeviceMutedOverlay = false;
    }
}


var outputDeviceMutedOverlay = false;
var OUTPUT_DEVICE_MUTED_OVERLAY_DEFAULT_DIMS_PX = 300;
var OUTPUT_DEVICE_MUTED_OVERLAY_DEFAULT_MARGINS_PX = 20;
var OUTPUT_DEVICE_MUTED_DIMS_RATIO_TO_WINDOW_SIZE = 0.8;
function updateOutputDeviceMutedOverlay(isMuted) {
    if (isMuted) {
        var props = {
            imageURL: Script.resolvePath("images/outputDeviceMuted.svg"),
            alpha: 0.5
        };

        var overlayDims = OUTPUT_DEVICE_MUTED_OVERLAY_DEFAULT_DIMS_PX;
        var overlayWithMarginsDims = overlayDims + 2 * OUTPUT_DEVICE_MUTED_OVERLAY_DEFAULT_MARGINS_PX;

        if  (overlayWithMarginsDims > Window.innerHeight || overlayWithMarginsDims > Window.innerWidth) {
            var minWindowDims = Math.min(Window.innerHeight,  Window.innerWidth);
            overlayDims = Math.round(minWindowDims * OUTPUT_DEVICE_MUTED_DIMS_RATIO_TO_WINDOW_SIZE);
        }

        props.width = overlayDims;
        props.height = overlayDims;
        props.x = Window.innerWidth / 2 - overlayDims / 2;
        props.y = Window.innerHeight / 2 - overlayDims / 2;
        if (outputDeviceMutedOverlay) {
            Overlays.editOverlay(outputDeviceMutedOverlay, props);
        } else {
            outputDeviceMutedOverlay = Overlays.addOverlay("image", props);
        }
    } else {
        maybeDeleteOutputDeviceMutedOverlay();
    }
}


var savedAvatarGain = Audio.avatarGain;
var savedServerInjectorGain = Audio.serverInjectorGain;
var savedLocalInjectorGain = Audio.localInjectorGain;
var savedSystemInjectorGain = Audio.systemInjectorGain;
var MUTED_VALUE_DB = -60; // This should always match `SimplifiedConstants.qml` -> numericConstants -> mutedValue!
function setOutputMuted(outputMuted) {
    if (outputMuted) {
        savedAvatarGain = Audio.avatarGain;
        savedServerInjectorGain = Audio.serverInjectorGain;
        savedLocalInjectorGain = Audio.localInjectorGain;
        savedSystemInjectorGain = Audio.systemInjectorGain;

        Audio.avatarGain = MUTED_VALUE_DB;
        Audio.serverInjectorGain = MUTED_VALUE_DB;
        Audio.localInjectorGain = MUTED_VALUE_DB;
        Audio.systemInjectorGain = MUTED_VALUE_DB;
    } else {
        if (savedAvatarGain === MUTED_VALUE_DB) {
            savedAvatarGain = 0;
        }
        Audio.avatarGain = savedAvatarGain;
        if (savedServerInjectorGain === MUTED_VALUE_DB) {
            savedServerInjectorGain = 0;
        }
        Audio.serverInjectorGain = savedServerInjectorGain;
        if (savedLocalInjectorGain === MUTED_VALUE_DB) {
            savedLocalInjectorGain = 0;
        }
        Audio.localInjectorGain = savedLocalInjectorGain;
        if (savedSystemInjectorGain === MUTED_VALUE_DB) {
            savedSystemInjectorGain = 0;
        }
        Audio.systemInjectorGain = savedSystemInjectorGain;
    }
}

var TOP_BAR_HEIGHT_PX = 48;
var INITIAL_LAUNCH_QML_PATH = Script.resolvePath("./simplifiedFTUE/InitialLaunchWindow.qml");
var INITIAL_LAUNCH_WINDOW_TITLE = "Initial Launch";
var INITIAL_LAUNCH_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var INITIAL_WINDOW_FLAGS = 0x00000001 | // Qt::Window
0x00000008 | // Qt::Popup
0x00000002 | // Qt::Tool
0x00000800 | // Qt::FramelessWindowHint
0x40000000; // Qt::NoDropShadowWindowHint
var initialLaunchWindow = false;
function displayInitialLaunchWindow() {
    if (initialLaunchWindow) {
        return;
    }

    simplifiedEmote.handleFTUEScreensVisibilityChanged(true);

    initialLaunchWindow = Desktop.createWindow(INITIAL_LAUNCH_QML_PATH, {
        title: INITIAL_LAUNCH_WINDOW_TITLE,
        presentationMode: INITIAL_LAUNCH_PRESENTATION_MODE,
        isFullScreenWindow: true,
        overrideFlags: INITIAL_WINDOW_FLAGS
    });

    initialLaunchWindow.fromQml.connect(onMessageFromInitialLaunchWindow);

    Window.location = "file:///~/serverless/empty.json";
}

var SECOND_LAUNCH_QML_PATH = Script.resolvePath("simplifiedFTUE/SecondLaunchWindow.qml");
var SECOND_LAUNCH_WINDOW_TITLE = "Second Launch";
var SECOND_LAUNCH_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var SECOND_WINDOW_FLAGS = 0x00000001 | // Qt::Window
0x00000008 | // Qt::Popup
0x00000002 | // Qt::Tool
0x00000800 | // Qt::FramelessWindowHint
0x40000000; // Qt::NoDropShadowWindowHint
var secondLaunchWindow = false;
function displaySecondLaunchWindow() {
    if (secondLaunchWindow) {
        return;
    }

    simplifiedEmote.handleFTUEScreensVisibilityChanged(true);

    secondLaunchWindow = Desktop.createWindow(SECOND_LAUNCH_QML_PATH, {
        title: SECOND_LAUNCH_WINDOW_TITLE,
        presentationMode: SECOND_LAUNCH_PRESENTATION_MODE,
        isFullScreenWindow: true,
        overrideFlags: SECOND_WINDOW_FLAGS
    });

    secondLaunchWindow.fromQml.connect(onMessageFromSecondLaunchWindow);

    Window.location = "file:///~/serverless/empty.json";
}

function closeInitialLaunchWindow() {
    if (initialLaunchWindow) {
        initialLaunchWindow.fromQml.disconnect(onMessageFromInitialLaunchWindow);
        initialLaunchWindow.close();
        initialLaunchWindow = null;
    }

    simplifiedEmote.handleFTUEScreensVisibilityChanged(false);
}

function closeSecondLaunchWindow() {
    if (secondLaunchWindow) {
        secondLaunchWindow.fromQml.disconnect(onMessageFromSecondLaunchWindow);
        secondLaunchWindow.close();
        secondLaunchWindow = null;
    }

    simplifiedEmote.handleFTUEScreensVisibilityChanged(false);
}

var INITIAL_LAUNCH_WINDOW_MESSAGE_SOURCE = "InitialLaunchWindow.qml";
function onMessageFromInitialLaunchWindow(message) {
    if (message.source !== INITIAL_LAUNCH_WINDOW_MESSAGE_SOURCE) {
        return;
    }

    switch (message.method) {
        case "closeInitialLaunchWindow":
            closeInitialLaunchWindow();
            var homeLocation = LocationBookmarks.getAddress("hqhome");
            if (homeLocation) {
                Window.location = homeLocation;
            }
            break;

        default:
            console.log("Unrecognized message from " + INITIAL_LAUNCH_WINDOW_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}

var SECOND_LAUNCH_WINDOW_MESSAGE_SOURCE = "SecondLaunchWindow.qml";
function onMessageFromSecondLaunchWindow(message) {
    if (message.source !== SECOND_LAUNCH_WINDOW_MESSAGE_SOURCE) {
        return;
    }

    switch (message.method) {
        case "closeSecondLaunchWindow":
            closeSecondLaunchWindow();
            var homeLocation = LocationBookmarks.getAddress("hqhome");
            if (homeLocation) {
                Window.location = homeLocation;
            }
            break;

        default:
            console.log("Unrecognized message from " + SECOND_LAUNCH_WINDOW_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}

var WAIT_FOR_TOP_BAR_MS = 1000;
function sendLocalStatusToQml() {
    var currentStatus = si.getLocalStatus();
    
    if (topBarWindow && currentStatus) {
        topBarWindow.sendToQml({
            "source": "simplifiedUI.js",
            "method": "updateStatusButton",
            "data": {
                "currentStatus": currentStatus
            }
        });
    } else {
        Script.setTimeout(sendLocalStatusToQml, WAIT_FOR_TOP_BAR_MS);
    }
}


var TOP_BAR_MESSAGE_SOURCE = "SimplifiedTopBar.qml";
function onMessageFromTopBar(message) {
    if (message.source !== TOP_BAR_MESSAGE_SOURCE) {
        return;
    }
    switch (message.method) {
        case "toggleAvatarApp":
            toggleAvatarApp();
            break;

        case "toggleSettingsApp":
            toggleSettingsApp();
            break;

        case "toggleHelpApp":
            toggleHelpApp();
            break;

        case "setOutputMuted":
            setOutputMuted(message.data.outputMuted);
            break;

        case "toggleStatus":
            si.toggleStatus();
            break;

        case "displayInitialLaunchWindow":
            displayInitialLaunchWindow();
            break;

        case "displaySecondLaunchWindow":
            displaySecondLaunchWindow();
            break;

        default:
            console.log("Unrecognized message from " + TOP_BAR_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}


function onTopBarClosed() {
    if (topBarWindow) {
        topBarWindow.fromQml.disconnect(onMessageFromTopBar);
        topBarWindow.closed.disconnect(onTopBarClosed);
    }
    topBarWindow = false;
}


function isOutputMuted() {
    return Audio.avatarGain === MUTED_VALUE_DB &&
        Audio.serverInjectorGain === MUTED_VALUE_DB &&
        Audio.localInjectorGain === MUTED_VALUE_DB &&
        Audio.systemInjectorGain === MUTED_VALUE_DB;
}


var TOP_BAR_QML_PATH = Script.resourcesPath() + "qml/hifi/simplifiedUI/topBar/SimplifiedTopBar.qml";
var TOP_BAR_WINDOW_TITLE = "Simplified Top Bar";
var TOP_BAR_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var TOP_BAR_WIDTH_PX = Window.innerWidth;
var topBarWindow = false;
function loadSimplifiedTopBar() {
    var windowProps = {
        title: TOP_BAR_WINDOW_TITLE,
        presentationMode: TOP_BAR_PRESENTATION_MODE,
        size: {
            x: TOP_BAR_WIDTH_PX,
            y: TOP_BAR_HEIGHT_PX
        }
    };
    windowProps.presentationWindowInfo = {
        dockArea: Desktop.DockArea.TOP
    };
    topBarWindow = Desktop.createWindow(TOP_BAR_QML_PATH, windowProps);

    topBarWindow.fromQml.connect(onMessageFromTopBar);
    topBarWindow.closed.connect(onTopBarClosed);

    // The eventbridge takes a nonzero time to initialize, so we have to wait a bit
    // for the QML to load and for that to happen before updating the UI.
    Script.setTimeout(function() {    
        sendLocalStatusToQml();
    },  WAIT_FOR_TOP_BAR_MS);
}


function maybeDeleteInputDeviceMutedOverlay() {
    if (inputDeviceMutedOverlay) {
        Overlays.deleteOverlay(inputDeviceMutedOverlay);
        inputDeviceMutedOverlay = false;
    }
}


var inputDeviceMutedOverlay = false;
var INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_X_PX = 237;
var INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_Y_PX = 64;
var INPUT_DEVICE_MUTED_MARGIN_LEFT_PX = 20;
var INPUT_DEVICE_MUTED_MARGIN_TOP_PX = 20;
function updateInputDeviceMutedOverlay(isMuted) {
    if (isMuted) {
        var props = {
            imageURL: Script.resolvePath("images/inputDeviceMuted.svg"),
            alpha: 0.5
        };
        props.width = INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_X_PX;
        props.height = INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_Y_PX;
        props.x = INPUT_DEVICE_MUTED_MARGIN_LEFT_PX;
        props.y = INPUT_DEVICE_MUTED_MARGIN_TOP_PX;
        if (inputDeviceMutedOverlay) {
            Overlays.editOverlay(inputDeviceMutedOverlay, props);
        } else {
            inputDeviceMutedOverlay = Overlays.addOverlay("image", props);
        }
    } else {
        maybeDeleteInputDeviceMutedOverlay();
    }
}


function onDesktopInputDeviceMutedChanged(isMuted) {
    if (!HMD.active) {
        updateInputDeviceMutedOverlay(isMuted);
    }
}


function onHMDInputDeviceMutedChanged(isMuted) {
    if (HMD.active) {
        updateInputDeviceMutedOverlay(isMuted);
    }
}


function onGeometryChanged(rect) {
    updateInputDeviceMutedOverlay(Audio.muted);
    updateOutputDeviceMutedOverlay(isOutputMuted());
}

var initialLaunchWindowIsMinimized = false;
var secondLaunchWindowIsMinimized = false;
function onWindowMinimizedChanged(isMinimized) {
    if (isMinimized) {
        handleInitialLaunchWindowVisibleChanged(false);
        handleSecondLaunchWindowVisibleChanged(false);
    } else if (!HMD.active) {
        handleInitialLaunchWindowVisibleChanged(true);
        handleSecondLaunchWindowVisibleChanged(true);
    }
}

function handleInitialLaunchWindowVisibleChanged(shouldBeVisible) {
    if (shouldBeVisible && !initialLaunchWindow && initialLaunchWindowIsMinimized) {
        displayInitialLaunchWindow();
        initialLaunchWindowIsMinimized = false;
    } else if (!shouldBeVisible && initialLaunchWindow) {
        closeInitialLaunchWindow();
        initialLaunchWindowIsMinimized = true;
    }
}

function handleSecondLaunchWindowVisibleChanged(shouldBeVisible) {
    if (shouldBeVisible && !secondLaunchWindow && secondLaunchWindowIsMinimized) {
        displaySecondLaunchWindow();
        secondLaunchWindowIsMinimized = false;
    } else if (!shouldBeVisible && secondLaunchWindow) {
        closeSecondLaunchWindow();
        secondLaunchWindowIsMinimized = true;
    }
}

function onDisplayModeChanged(isHMDMode) {
    if (isHMDMode) {
        Camera.setModeString("first person look at");
    }

    if (isHMDMode) {
        onHMDInputDeviceMutedChanged(Audio.mutedHMD);
        handleInitialLaunchWindowVisibleChanged(false);
        handleSecondLaunchWindowVisibleChanged(false);
    } else {
        onDesktopInputDeviceMutedChanged(Audio.mutedDesktop);
        handleInitialLaunchWindowVisibleChanged(true);
        handleSecondLaunchWindowVisibleChanged(true);
    }
}

function onToolbarVisibleChanged(isVisible, toolbarName) {
    if (isVisible && toolbarName == TOOLBAR_NAME && !Settings.getValue("simplifiedUI/keepExistingUIAndScripts", false)) {
        var toolbar = Toolbars.getToolbar(toolbarName);
        if (toolbar) {
            toolbar.writeProperty("visible", false);
        }
    }
}


function onStatusChanged() {
    sendLocalStatusToQml();
}


function maybeUpdateOutputDeviceMutedOverlay() {
    updateOutputDeviceMutedOverlay(isOutputMuted());
}


var oldAutomaticLODAdjust;
var oldLODAngleDeg;
var SIMPLIFIED_UI_AUTO_LOD_ADJUST = false;
var SIMPLIFIED_UI_LOD_ANGLE_DEG = 0.248;
function modifyLODSettings() {
    oldAutomaticLODAdjust = LODManager.automaticLODAdjust;
    oldLODAngleDeg = LODManager.lodAngleDeg;

    LODManager.automaticLODAdjust = SIMPLIFIED_UI_AUTO_LOD_ADJUST;
    LODManager.lodAngleDeg = SIMPLIFIED_UI_LOD_ANGLE_DEG;
}


function restoreLODSettings() {
    LODManager.automaticLODAdjust = oldAutomaticLODAdjust;
    LODManager.lodAngleDeg = oldLODAngleDeg;
}


var nametag = Script.require("./simplifiedNametag/simplifiedNametag.js");
var si = Script.require("./simplifiedStatusIndicator/simplifiedStatusIndicator.js");
var simplifiedEmote = Script.require("../simplifiedEmote/simplifiedEmote.js");
var oldShowAudioTools;
var oldShowBubbleTools;
var keepExistingUIAndScriptsSetting = Settings.getValue("simplifiedUI/keepExistingUIAndScripts", false);
function startup() {
    maybeRemoveDesktopMenu();
    modifyLODSettings();

    if (!keepExistingUIAndScriptsSetting) {
        if (!HMD.active) {
            var toolbar = Toolbars.getToolbar(TOOLBAR_NAME);
            if (toolbar) {
                toolbar.writeProperty("visible", false);
            }
        }
    }

    loadSimplifiedTopBar();

    
    si.updateProperties({ statusChanged: onStatusChanged });

    updateInputDeviceMutedOverlay(Audio.muted);
    updateOutputDeviceMutedOverlay(isOutputMuted());
    Audio.mutedDesktopChanged.connect(onDesktopInputDeviceMutedChanged);
    Audio.mutedHMDChanged.connect(onHMDInputDeviceMutedChanged);
    Window.geometryChanged.connect(onGeometryChanged);
    Window.minimizedChanged.connect(onWindowMinimizedChanged);
    HMD.displayModeChanged.connect(onDisplayModeChanged);
    Audio.avatarGainChanged.connect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.localInjectorGainChanged.connect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.serverInjectorGainChanged.connect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.systemInjectorGainChanged.connect(maybeUpdateOutputDeviceMutedOverlay);
    Toolbars.toolbarVisibleChanged.connect(onToolbarVisibleChanged);

    oldShowAudioTools = AvatarInputs.showAudioTools;
    AvatarInputs.showAudioTools = false;
    oldShowBubbleTools = AvatarInputs.showBubbleTools;
    AvatarInputs.showBubbleTools = false;
}


function shutdown() {
    restoreLODSettings();

    if (!keepExistingUIAndScriptsSetting) {

        if (!HMD.active) {
            var toolbar = Toolbars.getToolbar(TOOLBAR_NAME);
            if (toolbar) {
                toolbar.writeProperty("visible", true);
            }
        }
    }
    
    if (topBarWindow) {
        topBarWindow.close();
    }

    if (avatarAppWindow) {
        avatarAppWindow.close();
    }

    if (settingsAppWindow) {
        settingsAppWindow.close();
    }

    if (initialLaunchWindow) {
        closeInitialLaunchWindow();
    }

    if (secondLaunchWindow) {
        closeSecondLaunchWindow();
    }

    maybeDeleteInputDeviceMutedOverlay();
    maybeDeleteOutputDeviceMutedOverlay();

    Audio.mutedDesktopChanged.disconnect(onDesktopInputDeviceMutedChanged);
    Audio.mutedHMDChanged.disconnect(onHMDInputDeviceMutedChanged);
    Window.geometryChanged.disconnect(onGeometryChanged);
    Window.minimizedChanged.disconnect(onWindowMinimizedChanged);
    HMD.displayModeChanged.disconnect(onDisplayModeChanged);
    Audio.avatarGainChanged.disconnect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.localInjectorGainChanged.disconnect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.serverInjectorGainChanged.disconnect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.systemInjectorGainChanged.disconnect(maybeUpdateOutputDeviceMutedOverlay);
    Toolbars.toolbarVisibleChanged.disconnect(onToolbarVisibleChanged);

    AvatarInputs.showAudioTools = oldShowAudioTools;
    AvatarInputs.showBubbleTools = oldShowBubbleTools;
}


Script.scriptEnding.connect(shutdown);
startup();
