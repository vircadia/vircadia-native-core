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
    width: 400, height: 150,
});
window.setPosition(250, 800);a
window.closed.connect(function() { Script.stop(); });

