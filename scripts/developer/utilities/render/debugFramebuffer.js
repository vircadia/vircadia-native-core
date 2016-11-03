//
//  ddebugFramBuffer.js
//  examples/utilities/tools/render
//
//  Sam Gateau created on 2/18/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var DDB = Render.RenderDeferredTask.DebugDeferredBuffer;
oldConfig = DDB.toJSON();
DDB.enabled = true;


// Set up the qml ui
var qml = Script.resolvePath('framebuffer.qml');
var window = new OverlayWindow({
    title: 'Framebuffer Debug',
    source: qml,
    width: 400, height: 50,
});
window.setPosition(25, 50);
window.closed.connect(function() { Script.stop(); });

// Debug buffer sizing
var resizing = false;

Controller.mousePressEvent.connect(function (e) {
    if (shouldStartResizing(e.x)) {
        resizing = true;
    }
});
Controller.mouseReleaseEvent.connect(function() { resizing = false; });
Controller.mouseMoveEvent.connect(function (e) { resizing && setDebugBufferSize(e.x); });


function shouldStartResizing(eventX) {
    var x = Math.abs(eventX - Window.innerWidth * (1.0 + DDB.size.x) / 2.0);
    var mode = DDB.mode;
    return mode !== -1 && x < 20;
}

function setDebugBufferSize(x) {
    x = (2.0 * (x / Window.innerWidth) - 1.0); // scale
    x = Math.min(Math.max(-1, x), 1); // clamp
    DDB.size = { x: x, y: -1, z: 1, w: 1 };
}

Script.scriptEnding.connect(function () { DDB.fromJSON(oldConfig); });
