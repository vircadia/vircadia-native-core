"use strict";
/* jslint vars: true, plusplus: true */

//
//  simplifiedUI.js
//
//  Authors: Wayne Chen & Zach Fox
//  Created on: 5/1/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// START CONFIG OPTIONS
var DOCKED_QML_SUPPORTED = true;
var TOOLBAR_NAME = "com.highfidelity.interface.toolbar.system";
var DEFAULT_SCRIPTS_PATH_PREFIX = ScriptDiscoveryService.defaultScriptsPath + "/";
// END CONFIG OPTIONS


var DEFAULT_SCRIPTS_SEPARATE = [
    DEFAULT_SCRIPTS_PATH_PREFIX + "system/controllers/controllerScripts.js"
];
function loadNewSeparateDefaults() {
    for (var i in DEFAULT_SCRIPTS_SEPARATE) {
        Script.load(DEFAULT_SCRIPTS_SEPARATE[i]);
    }
}


var DEFAULT_SCRIPTS_COMBINED = [
    DEFAULT_SCRIPTS_PATH_PREFIX + "system/request-service.js",
    DEFAULT_SCRIPTS_PATH_PREFIX + "system/progress.js",
    DEFAULT_SCRIPTS_PATH_PREFIX + "system/away.js"
];
function runNewDefaultsTogether() {
    for (var i in DEFAULT_SCRIPTS_COMBINED) {
        Script.include(DEFAULT_SCRIPTS_COMBINED[i]);
    }
}


// Uncomment this out once the work is actually complete.
// Until then, users are required to access some functionality from the top menu bar.
//var MENU_NAMES = ["File", "Edit", "Display", "View", "Navigate", "Settings", "Developer", "Help"];
var MENU_NAMES = ["File", "Edit", "Display", "View", "Navigate", "Help",
    "Settings > General...", "Settings > Controls...", "Settings > Audio...", "Settings > Graphics...", "Settings > Security..."];
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
var AVATAR_APP_WINDOW_TITLE = "Your Avatars";
var AVATAR_APP_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var AVATAR_APP_WIDTH_PX = 480;
var AVATAR_APP_HEIGHT_PX = 615;
var avatarAppWindow = false;
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
        }
    });

    avatarAppWindow.fromQml.connect(onMessageFromAvatarApp);
    avatarAppWindow.closed.connect(onAvatarAppClosed);
}


function handleAvatarNametagMode(newAvatarNametagMode) {
    simplifiedNametag.handleAvatarNametagMode(newAvatarNametagMode);
}


var SETTINGS_APP_MESSAGE_SOURCE = "SettingsApp.qml";
function onMessageFromSettingsApp(message) {
    if (message.source !== SETTINGS_APP_MESSAGE_SOURCE) {
        return;
    }

    switch (message.method) {
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
        }
    });

    settingsAppWindow.fromQml.connect(onMessageFromSettingsApp);
    settingsAppWindow.closed.connect(onSettingsAppClosed);
}


function maybeDeleteOutputDeviceMutedOverlay() {
    if (outputDeviceMutedOverlay) {
        Overlays.deleteOverlay(outputDeviceMutedOverlay);
        outputDeviceMutedOverlay = false;
    }
}


var outputDeviceMutedOverlay = false;
var OUTPUT_DEVICE_MUTED_OVERLAY_DEFAULT_DIMS_PX = 300;
var OUTPUT_DEVICE_MUTED_MARGIN_BOTTOM_PX = 20;
var OUTPUT_DEVICE_MUTED_MARGIN_LEFT_RIGHT_PX = 20;
function updateOutputDeviceMutedOverlay(isMuted) {
    if (isMuted) {
        var props = {
            imageURL: Script.resolvePath("images/outputDeviceMuted.svg"),
            alpha: 0.5
        };
        var overlayDims = OUTPUT_DEVICE_MUTED_OVERLAY_DEFAULT_DIMS_PX;
        props.x = Window.innerWidth / 2 - overlayDims / 2;
        props.y = Window.innerHeight / 2 - overlayDims / 2;

        var outputDeviceMutedOverlayBottomY = props.y + overlayDims;
        var inputDeviceMutedOverlayTopY = getInputDeviceMutedOverlayTopY();
        if (outputDeviceMutedOverlayBottomY + OUTPUT_DEVICE_MUTED_MARGIN_BOTTOM_PX > inputDeviceMutedOverlayTopY) {
            overlayDims = 2 * (inputDeviceMutedOverlayTopY - Window.innerHeight / 2 - OUTPUT_DEVICE_MUTED_MARGIN_BOTTOM_PX);
        }

        if (overlayDims + OUTPUT_DEVICE_MUTED_MARGIN_LEFT_RIGHT_PX > Window.innerWidth) {
            overlayDims = Math.min(Window.innerWidth - OUTPUT_DEVICE_MUTED_MARGIN_LEFT_RIGHT_PX, overlayDims);
        } else {
            overlayDims = Math.min(OUTPUT_DEVICE_MUTED_OVERLAY_DEFAULT_DIMS_PX, overlayDims);
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

        case "setOutputMuted":
            setOutputMuted(message.data.outputMuted);
            break;

        case "toggleStatus":
            si.toggleStatus();
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
var TOP_BAR_HEIGHT_PX = 48;
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
    if (DOCKED_QML_SUPPORTED) {
        windowProps.presentationWindowInfo = {
            dockArea: Desktop.DockArea.TOP
        };
    } else {
        windowProps.position = {
            x: Window.x,
            y: Window.y
        };
    }
    topBarWindow = Desktop.createWindow(TOP_BAR_QML_PATH, windowProps);

    topBarWindow.fromQml.connect(onMessageFromTopBar);
    topBarWindow.closed.connect(onTopBarClosed);

    // The eventbridge takes a nonzero time to initialize, so we have to wait a bit
    // for the QML to load and for that to happen before updating the UI.
    Script.setTimeout(function() {    
        sendLocalStatusToQml();
    },  WAIT_FOR_TOP_BAR_MS);
}


var pausedScriptList = [];
var SCRIPT_NAME_WHITELIST = ["simplifiedUI.js", "statusIndicator.js"];
function pauseCurrentScripts() {
    var currentlyRunningScripts = ScriptDiscoveryService.getRunning();
    
    for (var i = 0; i < currentlyRunningScripts.length; i++) {
        var currentScriptObject = currentlyRunningScripts[i];
        if (SCRIPT_NAME_WHITELIST.indexOf(currentScriptObject.name) === -1) {
            ScriptDiscoveryService.stopScript(currentScriptObject.url);
            pausedScriptList.push(currentScriptObject.url);
        }
    }
}


function maybeDeleteInputDeviceMutedOverlay() {
    if (inputDeviceMutedOverlay) {
        Overlays.deleteOverlay(inputDeviceMutedOverlay);
        inputDeviceMutedOverlay = false;
    }
}


function getInputDeviceMutedOverlayTopY() {
    return (Window.innerHeight - INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_Y_PX - INPUT_DEVICE_MUTED_MARGIN_BOTTOM_PX);
}


var inputDeviceMutedOverlay = false;
var INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_X_PX = 353;
var INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_Y_PX = 95;
var INPUT_DEVICE_MUTED_MARGIN_BOTTOM_PX = 20;
function updateInputDeviceMutedOverlay(isMuted) {
    if (isMuted) {
        var props = {
            imageURL: Script.resolvePath("images/inputDeviceMuted.svg"),
            alpha: 0.5
        };
        props.width = INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_X_PX;
        props.height = INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_Y_PX;
        props.x = Window.innerWidth / 2 - INPUT_DEVICE_MUTED_OVERLAY_DEFAULT_X_PX / 2;
        props.y = getInputDeviceMutedOverlayTopY();
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
    updateInputDeviceMutedOverlay(isMuted);
}


function onGeometryChanged(rect) {
    updateInputDeviceMutedOverlay(Audio.muted);
    updateOutputDeviceMutedOverlay(isOutputMuted());
    if (topBarWindow && !DOCKED_QML_SUPPORTED) {
        topBarWindow.size = {
            "x": rect.width,
            "y": TOP_BAR_HEIGHT_PX
        };
        topBarWindow.position = {
            "x": rect.x,
            "y": rect.y
        };
    }
}

function ensureFirstPersonCameraInHMD(isHMDMode) {
    if (isHMDMode) {
        Camera.setModeString("first person");
    }
}


function onStatusChanged() {
    sendLocalStatusToQml();
}


function maybeUpdateOutputDeviceMutedOverlay() {
    updateOutputDeviceMutedOverlay(isOutputMuted());
}


var simplifiedNametag = Script.require("./simplifiedNametag/simplifiedNametag.js?" + Date.now());
var SimplifiedStatusIndicator = Script.require("./simplifiedStatusIndicator/simplifiedStatusIndicator.js?" + Date.now());
var si;
var oldShowAudioTools;
var oldShowBubbleTools;
var keepExistingUIAndScriptsSetting = Settings.getValue("simplifiedUI/keepExistingUIAndScripts", false);
function startup() {
    maybeRemoveDesktopMenu();

    if (!keepExistingUIAndScriptsSetting) {
        pauseCurrentScripts();
        runNewDefaultsTogether();
        loadNewSeparateDefaults();

        if (!HMD.active) {
            var toolbar = Toolbars.getToolbar(TOOLBAR_NAME);
            toolbar.writeProperty("visible", false);
        }
    }

    loadSimplifiedTopBar();

    simplifiedNametag.create();
    si = new SimplifiedStatusIndicator({
        statusChanged: onStatusChanged
    });
    si.startup();
    updateInputDeviceMutedOverlay(Audio.muted);
    updateOutputDeviceMutedOverlay(isOutputMuted());
    Audio.mutedDesktopChanged.connect(onDesktopInputDeviceMutedChanged);
    Window.geometryChanged.connect(onGeometryChanged);
    HMD.displayModeChanged.connect(ensureFirstPersonCameraInHMD);
    Audio.avatarGainChanged.connect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.localInjectorGainChanged.connect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.serverInjectorGainChanged.connect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.systemInjectorGainChanged.connect(maybeUpdateOutputDeviceMutedOverlay);

    oldShowAudioTools = AvatarInputs.showAudioTools;
    AvatarInputs.showAudioTools = false;
    oldShowBubbleTools = AvatarInputs.showBubbleTools;
    AvatarInputs.showBubbleTools = false;
}


function restoreScripts() {
    pausedScriptList.forEach(function(url) {
        ScriptDiscoveryService.loadScript(url);
    });

    pausedScriptList = [];
}


function shutdown() {
    restoreScripts();

    if (!keepExistingUIAndScriptsSetting) {
        Window.confirm("You'll have to restart Interface to get full functionality back. Clicking yes or no will dismiss this dialog.");

        if (!HMD.active) {
            var toolbar = Toolbars.getToolbar(TOOLBAR_NAME);
            toolbar.writeProperty("visible", true);
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

    maybeDeleteInputDeviceMutedOverlay();
    maybeDeleteOutputDeviceMutedOverlay();

    simplifiedNametag.destroy();
    si.unload();

    Audio.mutedDesktopChanged.disconnect(onDesktopInputDeviceMutedChanged);
    Window.geometryChanged.disconnect(onGeometryChanged);
    HMD.displayModeChanged.disconnect(ensureFirstPersonCameraInHMD);
    Audio.avatarGainChanged.disconnect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.localInjectorGainChanged.disconnect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.serverInjectorGainChanged.disconnect(maybeUpdateOutputDeviceMutedOverlay);
    Audio.systemInjectorGainChanged.disconnect(maybeUpdateOutputDeviceMutedOverlay);

    AvatarInputs.showAudioTools = oldShowAudioTools;
    AvatarInputs.showBubbleTools = oldShowBubbleTools;
}


Script.scriptEnding.connect(shutdown);
startup();
