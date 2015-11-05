//
//  whiteBoardSpawner.js
//  examples/painting/whiteboard
//
//  Created by Eric Levina on 10/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Run this script to spawn a whiteboard that one can paint on
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */
// Script specific
/*global hexToRgb */

Script.include("../../libraries/utils.js");
var scriptURL = Script.resolvePath("whiteboardEntityScript.js");
var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/whiteboard.fbx";

var colorIndicatorBorderModelURL = "https://s3.amazonaws.com/hifi-public/eric/models/colorIndicatorBorder.fbx";
var eraserModelURL = "https://s3.amazonaws.com/hifi-public/eric/models/eraser.fbx";
var surfaceModelURL = "https://s3.amazonaws.com/hifi-public/eric/models/boardSurface.fbx";
var rotation = Quat.safeEulerAngles(Camera.getOrientation());
rotation = Quat.fromPitchYawRollDegrees(0, rotation.y, 0);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(rotation)));

var whiteboardDimensions, colorIndicatorBoxDimensions, colorIndicatorBox, eraser, blocker;
var colorBoxes = [];

var colors = [
    hexToRgb("#66CCB3"),
    hexToRgb("#A43C37"),
    hexToRgb("#491849"),
    hexToRgb("#6AB03B"),
    hexToRgb("#993369"),
    hexToRgb("#000000")
];

var whiteboard = Entities.addEntity({
    type: "Model",
    shapeType: "box",
    modelURL: modelURL,
    name: "whiteboard base",
    position: center,
    rotation: rotation,
});

var colorIndicatorPosition = {
    x: center.x,
    y: center.y,
    z: center.z
};
colorIndicatorPosition.y += 1.55;
colorIndicatorPosition = Vec3.sum(colorIndicatorPosition, Vec3.multiply(-0.1, Quat.getFront(rotation)));
var colorIndicatorBorder = Entities.addEntity({
    type: "Model",
    position: colorIndicatorPosition,
    modelURL: colorIndicatorBorderModelURL,
    rotation: rotation,
    shapeType: "box"
});

var surfaceCenter = Vec3.sum(center, Vec3.multiply(-0.1, Quat.getFront(rotation)));
surfaceCenter.y += 0.6;
var drawingSurface = Entities.addEntity({
    type: "Model",
    modelURL: surfaceModelURL,
    shapeType: "box",
    name: "whiteboard surface",
    position: surfaceCenter,
    script: scriptURL,
    rotation: rotation,
    userData: JSON.stringify({
        color: {
            currentColor: colors[0]
        },
        "grabbableKey": {
            wantsTrigger:true
        }
    })

});

var lightPosition = Vec3.sum(center, Vec3.multiply(-3, Quat.getFront(rotation)));
var light = Entities.addEntity({
    type: 'Light',
    name: 'whiteboard light',
    position: lightPosition,
    dimensions: {x: 10, y: 10, z: 10},
    intensity: 2,
    color: {red: 255, green: 255, blue: 255}
});

var eraserPosition = Vec3.sum(center, {x: 0, y: 2.05, z: 0 });
eraserPosition = Vec3.sum(eraserPosition, Vec3.multiply(-0.1, rotation));
scriptURL = Script.resolvePath("eraseBoardEntityScript.js");
var eraser = Entities.addEntity({
    type: "Model",
    modelURL: eraserModelURL,
    position: eraserPosition,
    name: "Eraser",
    script: scriptURL,
    rotation: rotation,
    userData: JSON.stringify({
        whiteboard: drawingSurface
    })
});

Script.setTimeout(function() {
    whiteboardDimensions = Entities.getEntityProperties(whiteboard, "naturalDimensions").naturalDimensions;
    colorIndicatorBorderDimensions = Entities.getEntityProperties(colorIndicatorBorder, "naturalDimensions").naturalDimensions;
    setUp();
}, 2000)


function setUp() {
    var blockerPosition = Vec3.sum(center, {x: 0, y: -1, z: 0 });
    blockerPosition = Vec3.sum(blockerPosition, Vec3.multiply(-1, Quat.getFront(rotation)));
    blocker = Entities.addEntity({
        type: "Box",
        rotation: rotation,
        position: blockerPosition,
        dimensions: {x: whiteboardDimensions.x, y: 1, z: 0.1},
        shapeType: "box",
        visible: false
    });

    var eraseModelDimensions = Entities.getEntityProperties(eraser, "naturalDimensions").naturalDimensions;
    Entities.editEntity(eraser, {dimensions: eraseModelDimensions});
    Entities.editEntity(colorIndicatorBorder, {dimensions: colorIndicatorBorderDimensions});

    scriptURL = Script.resolvePath("colorIndicatorEntityScript.js");
    var colorIndicatorPosition = Vec3.sum(center, {
        x: 0,
        y: whiteboardDimensions.y / 2 + colorIndicatorBorderDimensions.y / 2,
        z: 0
    });
    colorIndicatorPosition = Vec3.sum(colorIndicatorPosition, Vec3.multiply(-.1, Quat.getFront(rotation)));
    var colorIndicatorBoxDimensions = Vec3.multiply(colorIndicatorBorderDimensions, 0.9);
    colorIndicatorBox = Entities.addEntity({
        type: "Box",
        name: "Color Indicator",
        color: colors[0],
        rotation: rotation,
        position: colorIndicatorPosition,
        dimensions: colorIndicatorBoxDimensions,
        script: scriptURL,
        userData: JSON.stringify({
            whiteboard: drawingSurface
        })
    });

    Entities.editEntity(drawingSurface, {
        userData: JSON.stringify({
            color: {
                currentColor: colors[0]
            },
            colorIndicator: colorIndicatorBox
        })
    });

    //COLOR BOXES
    var direction = Quat.getRight(rotation);
    var colorBoxPosition = Vec3.subtract(center, Vec3.multiply(direction, whiteboardDimensions.x / 2));
    var colorSquareDimensions = {
        x: 0.13,
        y: 0.13,
        z: 0.002
    };

    var palleteDepthOffset = -0.07;
    var palleteHeightOffset = -0.28;

    colorBoxPosition = Vec3.sum(colorBoxPosition, Vec3.multiply(palleteDepthOffset, Quat.getFront(rotation)));
    colorBoxPosition.y += palleteHeightOffset;
    var spaceBetweenColorBoxes = Vec3.multiply(direction, colorSquareDimensions.x * 1.76);
    var palleteXOffset = Vec3.multiply(direction, 0.43);
    colorBoxPosition = Vec3.sum(colorBoxPosition, palleteXOffset);
    var scriptURL = Script.resolvePath("colorSelectorEntityScript.js");
    for (var i = 0; i < colors.length; i++) {
        var colorBox = Entities.addEntity({
            type: "Box",
            name: "Color Selector",
            position: colorBoxPosition,
            dimensions: colorSquareDimensions,
            rotation: rotation,
            color: colors[i],
            script: scriptURL,
            userData: JSON.stringify({
                whiteboard: drawingSurface,
                colorIndicator: colorIndicatorBox
            })
        });
        colorBoxes.push(colorBox);
        colorBoxPosition = Vec3.sum(colorBoxPosition, spaceBetweenColorBoxes);
    }

}

function cleanup() {
    Entities.deleteEntity(whiteboard);
    Entities.deleteEntity(drawingSurface);
    Entities.deleteEntity(colorIndicatorBorder);
    Entities.deleteEntity(eraser);
    Entities.deleteEntity(colorIndicatorBox);
    Entities.deleteEntity(blocker);
    Entities.deleteEntity(light);
    colorBoxes.forEach(function(colorBox) {
        Entities.deleteEntity(colorBox);
    });
}



// Uncomment this line to delete whiteboard and all associated entity on script close
// Script.scriptEnding.connect(cleanup);
