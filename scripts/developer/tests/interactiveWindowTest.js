//
//  interactiveWindowTest.js
//
//  Created by Thijs Wenker on 2018-07-03
//  Copyright 2018 High Fidelity, Inc.
//
//  An example of an interactive window that toggles presentation mode when toggling HMD on/off
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

function getPreferredPresentationMode() {
    return HMD.active ? Desktop.PresentationMode.VIRTUAL : Desktop.PresentationMode.NATIVE;
}

function getPreferredTitle() {
    return HMD.active ? 'Virtual Desktop Window' : 'Native Desktop Window';
}

var virtualWindow = Desktop.createWindow(Script.resourcesPath() + 'qml/OverlayWindowTest.qml', {
    title: getPreferredTitle(),
    additionalFlags: Desktop.ALWAYS_ON_TOP,
    presentationMode: getPreferredPresentationMode(),
    size: {x: 500, y: 400}
});

HMD.displayModeChanged.connect(function() {
    virtualWindow.presentationMode = getPreferredPresentationMode();
    virtualWindow.title = getPreferredTitle();
});

Script.scriptEnding.connect(function() {
    virtualWindow.close();
});
