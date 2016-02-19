//
//  debug.js
//  examples/utilities/tools/render
//
//  Zach Pomerantz, created on 1/27/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

oldConfig = Render.toJSON();
Render.RenderShadowTask.enabled = true;
var RDT = Render.RenderDeferredTask;
RDT.AmbientOcclusion.enabled = true;
RDT.DebugDeferredBuffer.enabled = false;

// Set up the qml ui
var qml = Script.resolvePath('main.qml');
var window = new OverlayWindow({
    title: 'Render Engine Configuration',
    source: qml,
    width: 400, height: 900,
});
window.setPosition(25, 50);
window.closed.connect(function() { Script.stop(); });

// Debug buffer sizing
var resizing = false;
Controller.mousePressEvent.connect(function() { resizing = true; });
Controller.mouseReleaseEvent.connect(function() { resizing = false; });
Controller.mouseMoveEvent.connect(function(e) { resizing && setDebugBufferSize(e.x); });
function setDebugBufferSize(x) {
    x = (2.0 * (x / Window.innerWidth) - 1.0); // scale
    x = Math.min(Math.max(-1, x), 1); // clamp
    Render.RenderDeferredTask.DebugDeferredBuffer.size = {x: x, y: -1, z: 1, w: 1};
}

Script.scriptEnding.connect(function() { Render.load(oldConfig); } );
