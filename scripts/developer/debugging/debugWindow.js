//
//  debugWindow.js
//
//  Brad Hefta-Gaub, created on 12/19/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// Set up the qml ui
var qml = Script.resolvePath('debugWindow.qml');
var window = new OverlayWindow({
    title: 'Debug Window',
    source: qml,
    width: 400, height: 900,
});
window.setPosition(25, 50);
window.closed.connect(function() { Script.stop(); });

// Demonstrate sending a message to the QML window
ScriptDiscoveryService.printedMessage.connect(function(message, scriptFileName) {
    window.sendToQml("[" + scriptFileName + "] " + message);
});

ScriptDiscoveryService.warningMessage.connect(function(message, scriptFileName) {
    window.sendToQml("[" + scriptFileName + "] WARNING - " + message);
});

ScriptDiscoveryService.errorMessage.connect(function(message, scriptFileName) {
    window.sendToQml("[" + scriptFileName + "] ERROR - " + message);
});

ScriptDiscoveryService.infoMessage.connect(function(message, scriptFileName) {
    window.sendToQml("[" + scriptFileName + "] INFO - " + message);
});
