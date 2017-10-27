//
//  debugSurfaceGeometryPass.js
//
//  Created by Sam Gateau on 6/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('ambientOcclusionPass.qml');
var window = new OverlayWindow({
    title: 'Ambient Occlusion Pass',
    source: qml,
    width: 400, height: 300,
});
window.setPosition(Window.innerWidth - 420, 50 + 550 + 50);
window.closed.connect(function() { Script.stop(); });


var moveDebugCursor = false;
Controller.mousePressEvent.connect(function (e) {
	if (e.isMiddleButton) {
      	moveDebugCursor = true;
        setDebugCursor(e.x, e.y);
    }
});
Controller.mouseReleaseEvent.connect(function() { moveDebugCursor = false; });
Controller.mouseMoveEvent.connect(function (e) { if (moveDebugCursor) setDebugCursor(e.x, e.y); });


function setDebugCursor(x, y) {
    nx = (x / Window.innerWidth);
    ny = 1.0 - ((y) / (Window.innerHeight - 32));

     Render.getConfig("RenderMainView").getConfig("DebugAmbientOcclusion").debugCursorTexcoord = { x: nx, y: ny };
}
