//
//  debugRender.js
//  examples/utilities/render
//
//  Sam Gateau, created on 3/22/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('culling.qml');
var window = new OverlayWindow({
    title: 'Render Draws',
    source: qml,
    width: 300, 
    height: 200
});
window.setPosition(200, 50);
window.closed.connect(function() { Script.stop(); });