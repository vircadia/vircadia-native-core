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

// Set up the qml ui
var qml = Script.resolvePath('debugWindow.qml');
var window = new OverlayWindow({
    title: 'Debug Window',
    source: qml,
    width: 400, height: 900,
});
window.setPosition(25, 50);
window.closed.connect(function() { Script.stop(); });

var getFormattedDate = function() {
    var date = new Date();
    return date.getMonth() + "/" + date.getDate() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
};

var sendToLogWindow = function(type, message, scriptFileName) {
    var typeFormatted = "";
    if (type) {
        typeFormatted = type + " - ";
    }
    window.sendToQml("[" + getFormattedDate() + "] " + "[" + scriptFileName + "] " + typeFormatted + message);
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

}()); // END LOCAL_SCOPE