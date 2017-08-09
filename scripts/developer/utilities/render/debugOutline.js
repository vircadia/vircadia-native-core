//
//  debugOutline.js
//  developer/utilities/render
//
//  Olivier Prat, created on 08/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('outline.qml');
var window = new OverlayWindow({
    title: 'Outline',
    source: qml,
    width: 285, 
    height: 370,
});
window.closed.connect(function() { Script.stop(); });