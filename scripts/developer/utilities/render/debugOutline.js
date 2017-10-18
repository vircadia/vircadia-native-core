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

"use strict";

// Created by Sam Gondelman on 9/7/2017
// Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() { // BEGIN LOCAL_SCOPE

var END_DIMENSIONS = {
    x: 0.15,
    y: 0.15,
    z: 0.15
};
var COLOR = {red: 97, green: 247, blue: 255};
var end = {
    type: "sphere",
    dimensions: END_DIMENSIONS,
    color: COLOR,
    ignoreRayIntersection: true,
    alpha: 1.0,
    visible: true
}

var COLOR2 = {red: 247, green: 97, blue: 255};
var end2 = {
    type: "sphere",
    dimensions: END_DIMENSIONS,
    color: COLOR2,
    ignoreRayIntersection: true,
    alpha: 1.0,
    visible: true
}

var outlineGroupIndex = 0

function setOutlineGroupIndex(index) {
    print("Switching to outline group "+index)
    outlineGroupIndex = index
}

window.fromQml.connect(setOutlineGroupIndex);

var renderStates = [{name: "test", end: end}];
var defaultRenderStates = [{name: "test", distance: 20.0, end: end2}];

var ray = LaserPointers.createLaserPointer({
    joint: "Mouse",
    filter: RayPick.PICK_ENTITIES | RayPick.PICK_OVERLAYS | RayPick.PICK_AVATARS | RayPick.PICK_INVISIBLE | RayPick.PICK_NONCOLLIDABLE,
    renderStates: renderStates,
    defaultRenderStates: defaultRenderStates,
    enabled: true
});

function cleanup() {
    LaserPointers.removeLaserPointer(ray);
}
Script.scriptEnding.connect(cleanup);

var prevID = 0;
var prevType = "";
function update() {
    // you have to do this repeatedly because there's a bug but I'll fix it
    LaserPointers.setRenderState(ray, "test");

    var result = LaserPointers.getPrevRayPickResult(ray);
    var selectionName = "contextOverlayHighlightList"

    if (outlineGroupIndex>0) {
        selectionName += outlineGroupIndex
    }

    if (result.type != RayPick.INTERSECTED_NONE) {
        if (result.objectID != prevID) { 
            if (prevID != 0) {
                Selection.removeFromSelectedItemsList(selectionName, prevType, prevID)
            }
            
            var typeName = ""
            if (result.type == RayPick.INTERSECTED_ENTITY) {
                typeName = "entity"
            } else if (result.type == RayPick.INTERSECTED_OVERLAY) {
                typeName = "overlay"
            } else if (result.type == RayPick.INTERSECTED_AVATAR) {
                typeName = "avatar"
            }
            
            Selection.addToSelectedItemsList(selectionName, typeName, result.objectID)
            print("OUTLINE " + outlineGroupIndex + " picked type: " + result.type + ", id: " + result.objectID);

            prevID = result.objectID;
            prevType = typeName;
        }
    } else {
        if (prevID != 0) {
            Selection.removeFromSelectedItemsList(selectionName, prevType, prevID)
        }
        prevID = 0;
    }
}
Script.update.connect(update);

}()); // END LOCAL_SCOPE