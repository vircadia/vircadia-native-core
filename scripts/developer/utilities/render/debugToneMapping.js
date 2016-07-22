//
//  debugToneMapping.js
//
//  Created by Sam Gateau on 6/30/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('toneMapping.qml');
var window = new OverlayWindow({
    title: 'Tone Mapping',
    source: qml,
    width: 400, height: 200,
});
window.setPosition(250, 1000);
window.closed.connect(function() { Script.stop(); });

