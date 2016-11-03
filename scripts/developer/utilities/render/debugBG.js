//
//  debugBG.js
//  examples/utilities/tools/render
//
//  Zach Pomerantz, created on 1/27/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('BG.qml');
var window = new OverlayWindow({
    title: 'Background Timer',
    source: qml,
    width: 300
});
window.setPosition(25, 50);
window.closed.connect(function() { Script.stop(); });

