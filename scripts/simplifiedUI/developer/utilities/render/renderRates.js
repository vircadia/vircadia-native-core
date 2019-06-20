//
//  cacheStats.js
//  examples/utilities/cache
//
//  Zach Pomerantz, created on 4/1/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('rates.qml');
var window = new OverlayWindow({
    title: 'Render Rates',
    source: qml,
    width: 300, 
    height: 200
});
window.setPosition(500, 50);
window.closed.connect(function() { Script.stop(); });
