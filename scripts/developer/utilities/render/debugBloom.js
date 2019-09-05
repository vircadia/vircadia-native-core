//
//  debugBloom.js
//  developer/utilities/render
//
//  Olivier Prat, created on 09/25/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var window = Desktop.createWindow(Script.resolvePath('./luci/Bloom.qml'), {
    title: "Bloom",
    presentationMode: Desktop.PresentationMode.NATIVE,
    size: {x: 285, y: 40}
});