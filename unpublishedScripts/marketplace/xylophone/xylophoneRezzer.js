//
//  xylophoneRezzer.js
//
//  Created by Patrick Gosch on 03/28/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var SOUND_FILES = ["C4.wav", "D4.wav", "E4.wav", "F4.wav", "G4.wav", "A4.wav", "B4.wav", "C5.wav"];
var KEY_MODEL_URL = Script.resolvePath("xyloKey_2_a_e.fbx");
var KEY_SCRIPT_URL = Script.resolvePath("xylophoneKey.js");
var MALLET_SCRIPT_URL = Script.resolvePath("xylophoneMallet.js");
var TEXTURE_BLACK = Script.resolvePath("xylotex_bar_black.png");
var MALLET_MODEL_URL = Script.resolvePath("Mallet3-2pc.fbx");
var MALLET_MODEL_COLLIDER_URL = Script.resolvePath("Mallet3-2bpc_phys.obj");
var FORWARD = { x: 0, y: 0, z: -1 };
var center = MyAvatar.position;

var XYLOPHONE_FORWARD_OFFSET = 0.8;
var xylophoneFramePosition = Vec3.sum(center, Vec3.multiply(FORWARD, XYLOPHONE_FORWARD_OFFSET));
var xylophoneFrameID = Entities.addEntity({
    name: "Xylophone",
    type: "Model",
    modelURL: Script.resolvePath("xylophoneFrameWithWave.fbx"),
    position: xylophoneFramePosition,
    rotation: Quat.fromVec3Radians({ x: 0, y: Math.PI, z: 0 }),
    shapeType: "static-mesh"
});

var KEY_Y_OFFSET = 0.45;
center.y += KEY_Y_OFFSET;
var keyPosition, keyRotation, userData, textureData, keyID;
var ROTATION_START = 0.9;
var ROTATION_DELTA = 0.2;
for (var i = 1; i <= SOUND_FILES.length; i++) {
    
    keyRotation = Quat.fromVec3Radians({ x: 0, y: ROTATION_START - (i*ROTATION_DELTA), z: 0 });
    keyPosition = Vec3.sum(center, Vec3.multiplyQbyV(keyRotation, FORWARD));

    userData = {
        soundFile: SOUND_FILES[i-1]
    };

    textureData = {
        "file4": Script.resolvePath("xylotex_bar" + i + ".png"),
        "file5": TEXTURE_BLACK
    };

    keyID = Entities.addEntity({
        name: ("XyloKey" + i),
        type: "Model",
        modelURL: KEY_MODEL_URL,
        position: keyPosition,
        rotation: keyRotation,
        shapeType: "static-mesh",
        script: KEY_SCRIPT_URL,
        textures: JSON.stringify(textureData),
        userData: JSON.stringify(userData),
        parentID: xylophoneFrameID
    });
}

// if rezzed on/above something, wait until after model has loaded so you can read its dimensions then move object on to that surface.
var pickRay = {origin: center, direction: {x: 0, y: -1, z: 0}};
var intersection = Entities.findRayIntersection(pickRay, true);
if (intersection.intersects && (intersection.distance < 10)) {
    var surfaceY = intersection.intersection.y;
    Script.setTimeout( function() {
        // should add loop to check for fbx loaded instead of delay
        var xylophoneDimensions = Entities.getEntityProperties(xylophoneFrameID, ["dimensions"]).dimensions;
        xylophoneFramePosition.y = surfaceY + (xylophoneDimensions.y/2);
        Entities.editEntity(xylophoneFrameID, {position: xylophoneFramePosition});
        rezMallets();
    }, 2000);
} else {
    print("No surface detected.");
    rezMallets();
}

function rezMallets() {
    var malletProperties = {
        name: "Xylophone Mallet",
        type: "Model",
        modelURL: MALLET_MODEL_URL,
        compoundShapeURL: MALLET_MODEL_COLLIDER_URL,
        collidesWith: "static,dynamic,kinematic",
        collisionMask: 7,
        collisionsWillMove: 1,
        dynamic: 1,
        damping: 1,
        angularDamping: 1,
        shapeType: "compound",
        script: MALLET_SCRIPT_URL,
        userData: JSON.stringify({
            grabbableKey: {
                invertSolidWhileHeld: true
            },
            wearable: {
                joints: {
                    LeftHand: [
                        { x: 0, y: 0.2, z: 0.04 },
                        Quat.fromVec3Degrees({ x: 0, y: 90, z: 90 })
                    ],
                    RightHand: [
                        { x: 0, y: 0.2, z: 0.04 },
                        Quat.fromVec3Degrees({ x: 0, y: 90, z: 90 })
                    ]
                }
            }
        }),
        dimensions: { "x": 0.057845603674650192, "y": 0.057845607399940491, "z": 0.30429631471633911 } // not being set from fbx for some reason.
    };

    var LEFT_MALLET_POSITION = { x: 0.1, y: 0.55, z: 0 };
    var LEFT_MALLET_ROTATION = { x: 0, y: Math.PI - 0.1, z: 0 };
    var RIGHT_MALLET_POSITION = { x: -0.1, y: 0.55, z: 0 };
    var RIGHT_MALLET_ROTATION = { x: 0, y: Math.PI + 0.1, z: 0 };

    malletProperties.position = Vec3.sum(xylophoneFramePosition, LEFT_MALLET_POSITION);
    malletProperties.rotation = Quat.fromVec3Radians(LEFT_MALLET_ROTATION);
    Entities.addEntity(malletProperties);

    malletProperties.position = Vec3.sum(xylophoneFramePosition, RIGHT_MALLET_POSITION);
    malletProperties.rotation = Quat.fromVec3Radians(RIGHT_MALLET_ROTATION);
    Entities.addEntity(malletProperties);
    Script.stop();
}