//
//  xylophoneRezzer.js
//
//  Created by Patrick Gosch on 03/28/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var soundFiles = ["C2.wav", "D2.wav", "E2.wav", "F2.wav", "G2.wav", "A2.wav", "B2.wav", "C3.wav"];
var keyModelURL = Script.resolvePath("xyloKey_2_a_e.fbx");
var keyScriptURL = Script.resolvePath("xylophoneKey.js");
var TEXBLACK = Script.resolvePath("xylotex_bar_black.png");
var malletModelURL = Script.resolvePath("Mallet2-2pc.fbx");
var center = MyAvatar.position;
var fwd = {x:0, y:0, z:-1};

var xyloFramePos = Vec3.sum(center, Vec3.multiply(fwd, 0.8));
var xyloFrameID = Entities.addEntity( {
    name: "Xylophone",
    type: "Model",
    modelURL: Script.resolvePath("xylophoneFrameWithWave.fbx"),
    position: xyloFramePos,
    rotation: Quat.fromVec3Radians({x:0, y:Math.PI, z:0}),
    shapeType: "static-mesh"
});

center.y += (0.45); //key Y offset from frame
var keyPos, keyRot, ud, td, keyID;
for (var i = 1; i <= soundFiles.length; i++) {

    keyRot = Quat.fromVec3Radians({x:0, y:(0.9 - (i*0.2)), z:0});
    keyPos = Vec3.sum(center, Vec3.multiplyQbyV(keyRot, fwd));

    ud = {
        soundFile: soundFiles[i-1]
    };

    td = {
        "file4": Script.resolvePath("xylotex_bar" + i + ".png"),
        "file5": TEXBLACK
    };

    keyID = Entities.addEntity( {
        name: ("XyloKey" + i),
        type: "Model",
        modelURL: keyModelURL,
        position: keyPos,
        rotation: keyRot,
        shapeType: "static-mesh",
        script: keyScriptURL,
        textures: JSON.stringify(td),
        userData: JSON.stringify(ud),
        parentID: xyloFrameID,
    } );
}

//if rezzed on/above something, wait until after model has loaded so you can read its dimensions then move object on to that surface.
var pickRay = {origin: center, direction: {x:0, y:-1, z:0}};
var intersection = Entities.findRayIntersection(pickRay, true);
if (intersection.intersects && (intersection.distance < 10)) {
    var surfaceY = intersection.intersection.y;
    Script.setTimeout( function() {
        //should add loop to check for fbx loaded instead of delay
        var xyloDimensions = Entities.getEntityProperties(xyloFrameID, ["dimensions"]).dimensions;
        xyloFramePos.y = surfaceY + (xyloDimensions.y/2);
        Entities.editEntity(xyloFrameID, {position: xyloFramePos});
        rezMallets();
    }, 2000); //timeout (ms)
} else {
    print("No surface detected.");
    rezMallets();
}

function rezMallets() {
    var malletProps = {
        name: "Xylophone Mallet",
        type: "Model",
        modelURL: malletModelURL,
        collidesWith: "static,dynamic,kinematic,",
        collisionMask: 7,
        collisionsWillMove: 1,
        dynamic: 1,
        damping: 1,
        angularDamping: 1,
        shapeType: "simple-compound",
        userData: "{\"grabbableKey\":{\"grabbable\":true}}"
    };

    malletProps.position = Vec3.sum(xyloFramePos, {x: 0.1, y: 0.55, z: 0});
    malletProps.rotation = Quat.fromVec3Radians({x:0, y:Math.PI - 0.1, z:0});
    Entities.addEntity(malletProps);

    malletProps.position = Vec3.sum(xyloFramePos, {x: -0.1, y: 0.55, z: 0});
    malletProps.rotation = Quat.fromVec3Radians({x:0, y:Math.PI + 0.1, z:0});
    Entities.addEntity(malletProps);
    Script.stop();
}
