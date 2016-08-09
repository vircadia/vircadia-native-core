//
// snapshot.js
//
// Created by David Kelly on 1 August 2016
// Copyright 2016 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var SNAPSHOT_DELAY = 500; // 500ms
var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
var resetOverlays;
var button = toolBar.addButton({
    objectName: "snapshot",
    imageURL: Script.resolvePath("assets/images/tools/snap.svg"),
    visible: true,
    buttonState: 1,
    defaultState: 1,
    hoverState: 2,
    alpha: 0.9,
});

function confirmShare(data) {
    if (!Window.confirm("Share snapshot " + data.localPath + "?")) { // This dialog will be more elaborate...
        return;
    }
    Window.alert("Pretending to upload. That code will go here.");
}

function onClicked() {
    // update button states
    resetOverlays = Menu.isOptionChecked("Overlays");
    Window.snapshotTaken.connect(resetButtons);
    
    button.writeProperty("buttonState", 0);
    button.writeProperty("defaultState", 0);
    button.writeProperty("hoverState", 2);
     
    // hide overlays if they are on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Overlays", false);
    }
    
    // hide hud
    toolBar.writeProperty("visible", false);

    // take snapshot (with no notification)
    Script.setTimeout(function () {
        Window.takeSnapshot(false);
    }, SNAPSHOT_DELAY);
}

function resetButtons(path, notify) {
    // show overlays if they were on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Overlays", true); 
    }
    // show hud
    toolBar.writeProperty("visible", true);
    
    // update button states
    button.writeProperty("buttonState", 1);
    button.writeProperty("defaultState", 1);
    button.writeProperty("hoverState", 3);
    Window.snapshotTaken.disconnect(resetButtons);
    confirmShare({localPath: path});
 }

button.clicked.connect(onClicked);

Script.scriptEnding.connect(function () {
    toolBar.removeButton("snapshot");
    button.clicked.disconnect(onClicked);
});
