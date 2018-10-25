//
//  debugWindow.js
//
//  Brad Hefta-Gaub, created on 12/19/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

//check if script already running.
var scriptData = ScriptDiscoveryService.getRunning();
var scripts = scriptData.filter(function (datum) { return datum.name === 'debugWindow.js'; });
if (scripts.length >= 2) {
    //2nd instance of the script is too much
    ScriptDiscoveryService.stopScript(scripts[1].url);
    return;
}

var SUPPRESS_DEFAULT_SCRIPTS_MENU_NAME  = "Developer"
var SUPPRESS_DEFAULT_SCRIPTS_ITEM_NAME = "Suppress messages from default scripts in Debug Window";
var DEBUG_WINDOW_SUPPRESS_DEFAULTS_SCRIPTS = 'debugWindowSuppressDefaultScripts';
var suppressDefaultScripts = Settings.getValue(DEBUG_WINDOW_SUPPRESS_DEFAULTS_SCRIPTS, false)
Menu.addMenuItem({
    menuName: SUPPRESS_DEFAULT_SCRIPTS_MENU_NAME,
    menuItemName: SUPPRESS_DEFAULT_SCRIPTS_ITEM_NAME,
    isCheckable: true,
    isChecked: suppressDefaultScripts
});

Menu.menuItemEvent.connect(function(menuItem) {
    if (menuItem === SUPPRESS_DEFAULT_SCRIPTS_ITEM_NAME) {
        suppressDefaultScripts = Menu.isOptionChecked(SUPPRESS_DEFAULT_SCRIPTS_ITEM_NAME);
    }
});

// Set up the qml ui
var qml = Script.resolvePath('debugWindow.qml');

var HMD_DEBUG_WINDOW_GEOMETRY_KEY = 'hmdDebugWindowGeometry';
var hmdDebugWindowGeometryValue = Settings.getValue(HMD_DEBUG_WINDOW_GEOMETRY_KEY);

var windowWidth = 400;
var windowHeight = 900;

var hasPosition = false;
var windowX = 0;
var windowY = 0;

if (hmdDebugWindowGeometryValue !== '') {
    var geometry = JSON.parse(hmdDebugWindowGeometryValue);
    if ((geometry.x !== 0) && (geometry.y !== 0)) {
        windowWidth = geometry.width;
        windowHeight = geometry.height;
        windowX = geometry.x;
        windowY = geometry.y;
        hasPosition = true;
    }
}

var window = new OverlayWindow({
    title: 'Debug Window',
    source: qml,
    width: windowWidth, height: windowHeight,
});

if (hasPosition) {
    window.setPosition(windowX, windowY);
}

window.visibleChanged.connect(function() {
    if (!window.visible) {
        window.setVisible(true);
    }
});

window.closed.connect(function () { Script.stop(); });

function shouldLogMessage(scriptFileName) {
    return !suppressDefaultScripts
        || (scriptFileName !== "defaultScripts.js" && scriptFileName != "controllerScripts.js");
}

var getFormattedDate = function() {
    var date = new Date();
    return date.getMonth() + "/" + date.getDate() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
};

var sendToLogWindow = function(type, message, scriptFileName) {
    if (shouldLogMessage(scriptFileName)) {
        var typeFormatted = "";
        if (type) {
            typeFormatted = type + " - ";
        }
        window.sendToQml("[" + getFormattedDate() + "] " + "[" + scriptFileName + "] " + typeFormatted + message);
    }
};

ScriptDiscoveryService.printedMessage.connect(function(message, scriptFileName) {
    sendToLogWindow("", message, scriptFileName);
});

ScriptDiscoveryService.warningMessage.connect(function(message, scriptFileName) {
    sendToLogWindow("WARNING", message, scriptFileName);
});

ScriptDiscoveryService.errorMessage.connect(function(message, scriptFileName) {
    sendToLogWindow("ERROR", message, scriptFileName);
});

ScriptDiscoveryService.infoMessage.connect(function(message, scriptFileName) {
    sendToLogWindow("INFO", message, scriptFileName);
});

ScriptDiscoveryService.clearDebugWindow.connect(function() {
    window.clearDebugWindow();
});

Script.scriptEnding.connect(function () {
    Settings.setValue(DEBUG_WINDOW_SUPPRESS_DEFAULTS_SCRIPTS,
                      Menu.isOptionChecked(SUPPRESS_DEFAULT_SCRIPTS_ITEM_NAME));
    Menu.removeMenuItem(SUPPRESS_DEFAULT_SCRIPTS_MENU_NAME, SUPPRESS_DEFAULT_SCRIPTS_ITEM_NAME);

    var geometry = JSON.stringify({
        x: window.position.x,
        y: window.position.y,
        width: window.size.x,
        height: window.size.y
    });

    Settings.setValue(HMD_DEBUG_WINDOW_GEOMETRY_KEY, geometry);
    window.close();
});

}());
