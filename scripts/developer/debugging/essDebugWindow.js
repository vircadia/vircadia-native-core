//
//  essDebugWindow.js
//
//  Created by Clement Brisset on 2/2/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE

// Set up the qml ui
var qml = Script.resolvePath('debugWindow.qml');
var window = new OverlayWindow({
    title: 'Entity Script Server Log Window',
    source: qml,
    width: 400, height: 900,
});
window.setPosition(25, 50);
window.closed.connect(function() { Script.stop(); });

EntityScriptServerLog.receivedNewLogLines.connect(function(message) {
    window.sendToQml(message.trim());
});


}()); // END LOCAL_SCOPE
