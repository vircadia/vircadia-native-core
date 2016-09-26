//
//  stats.js
//  scripts/developer/utilities/audio
//
//  Zach Pomerantz, created on 9/22/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var INITIAL_WIDTH = 400;
var INITIAL_OFFSET = 50;

// Set up the qml ui
var qml = Script.resolvePath('stats.qml');
var window = new OverlayWindow({
    title: 'Audio Interface Statistics',
    source: qml,
    width: 400, height: 720 // stats.qml may be too large for some screens
});
window.setPosition(INITIAL_OFFSET, INITIAL_OFFSET);

window.closed.connect(function() { Script.stop(); });

