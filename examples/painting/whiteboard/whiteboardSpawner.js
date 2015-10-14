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



Script.include("../../libraries/utils.js");
var scriptURL = Script.resolvePath("whiteBoardEntityScript.js?v2");

var rotation = Quat.safeEulerAngles(Camera.getOrientation());
rotation = Quat.fromPitchYawRollDegrees(0, rotation.y, 0);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(rotation)));
center.y += 0.4;
var whiteboardDimensions = {
    x: 2,
    y: 1.5,
    z: 0.01
};

var colors = [
    hexToRgb("#2F8E84"),
    hexToRgb("#66CCB3"),
    hexToRgb("#A43C37"),
    hexToRgb("#491849"),
    hexToRgb("#6AB03B"),
    hexToRgb("#993369"),
    hexToRgb("#9B47C2")
];
var whiteboard = Entities.addEntity({
    type: "Box",
    position: center,
    rotation: rotation,
    script: scriptURL,
    dimensions: whiteboardDimensions,
    color: {
        red: 255,
        green: 255,
        blue: 255
    },
    userData: JSON.stringify({
        currentColor: colors[0]
    })
});

//COLORS

var direction = Quat.getRight(rotation);
var colorBoxPosition = Vec3.subtract(center, Vec3.multiply(direction, whiteboardDimensions.x / 2));

var colorBoxes = [];

var colorSquareDimensions = {
    x: (whiteboardDimensions.x / 2) / (colors.length - 1),
    y: .1,
    z: 0.05
};
colorBoxPosition.y += whiteboardDimensions.y / 2 + colorSquareDimensions.y / 2 - 0.01;
var spaceBetweenColorBoxes = Vec3.multiply(direction, colorSquareDimensions.x * 2);
var scriptURL = Script.resolvePath("colorSelectorEntityScript.js");
for (var i = 0; i < colors.length; i++) {
    var colorBox = Entities.addEntity({
        type: "Box",
        position: colorBoxPosition,
        dimensions: colorSquareDimensions,
        rotation: rotation,
        color: colors[i],
        script: scriptURL,
        userData: JSON.stringify({
            colorPalette: true,
            whiteboard: whiteboard
        })
    });
    colorBoxes.push(colorBox);
    colorBoxPosition = Vec3.sum(colorBoxPosition, spaceBetweenColorBoxes);
}


var eraseBoxDimensions = {
    x: 0.5,
    y: 0.1,
    z: 0.01
};


var eraseBoxPosition = Vec3.sum(center, Vec3.multiply(direction, whiteboardDimensions.x / 2 + eraseBoxDimensions.x / 2 + 0.01));
eraseBoxPosition.y += 0.3;
scriptURL = Script.resolvePath("eraseBoardEntityScript.js");
var eraseAllText = Entities.addEntity({
    type: "Text",
    position: eraseBoxPosition,
    script: scriptURL,
    rotation: rotation,
    dimensions: eraseBoxDimensions,
    backgroundColor: {
        red: 0,
        green: 60,
        blue: 0
    },
    textColor: {
        red: 255,
        green: 10,
        blue: 10
    },
    text: "ERASE BOARD",
    lineHeight: 0.07,
    userData: JSON.stringify({
        whiteboard: whiteboard
    })
});
print(JSON.stringify(Entities.getEntityProperties(eraseAllText)))



function cleanup() {
    Entities.deleteEntity(whiteboard);
    Entities.deleteEntity(eraseAllText);
    colorBoxes.forEach(function(colorBox) {
        Entities.deleteEntity(colorBox);
    });
}


Script.scriptEnding.connect(cleanup);