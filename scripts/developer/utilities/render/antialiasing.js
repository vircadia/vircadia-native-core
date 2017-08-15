//
//  antialiasing.js
//
//  Created by Sam Gateau on 8/14/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('antialiasing.qml');
var window = new OverlayWindow({
    title: 'Antialiasing',
    source: qml,
    width: 400, height:400,
});
window.setPosition(Window.innerWidth - 420, 50);
window.closed.connect(function() { Script.stop(); });


