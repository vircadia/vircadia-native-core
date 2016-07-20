//
//  debugDeferredLighting.js
//
//  Created by Sam Gateau on 6/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('deferredLighting.qml');
var window = new OverlayWindow({
    title: 'Lighting',
    source: qml,
    width: 400, height:220,
});
window.setPosition(Window.innerWidth - 420, 50);
window.closed.connect(function() { Script.stop(); });


var DDB = Render.RenderDeferredTask.DebugDeferredBuffer;
DDB.enabled = true;
DDB.mode = 0;

// Debug buffer sizing
var resizing = false;
Controller.mousePressEvent.connect(function (e) {
    if (shouldStartResizing(e.x)) {
        resizing = true;
    }
});
Controller.mouseReleaseEvent.connect(function() { resizing = false; });
Controller.mouseMoveEvent.connect(function (e) { resizing && setDebugBufferSize(e.x); });
Script.scriptEnding.connect(function () { DDB.enabled = false; });

function shouldStartResizing(eventX) {
    var x = Math.abs(eventX - Window.innerWidth * (1.0 + DDB.size.x) / 2.0);
    var mode = DDB.mode;
    return mode !== 0 && x < 20;
}

function setDebugBufferSize(x) {
    x = (2.0 * (x / Window.innerWidth) - 1.0); // scale
    x = Math.min(Math.max(-1, x), 1); // clamp
    DDB.size = { x: x, y: -1, z: 1, w: 1 };
}


