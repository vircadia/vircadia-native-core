//  stick-hydra.js
//  examples
//
//  Created by Seth Alves on 2015-7-9
//  Copyright 2015 High Fidelity, Inc.
//
//  Allow avatar to hold a stick and control it with a hand-tracker
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var hand = "left";
var nullActionID = "00000000-0000-0000-0000-000000000000";
var controllerID;
var controllerActive;
var stickID = null;
var actionID = nullActionID;
var makingNewStick = false;

function makeNewStick() {
    if (makingNewStick) {
        return;
    }
    makingNewStick = true;
    cleanUp();
    // sometimes if this is run immediately the stick doesn't get created?  use a timer.
    Script.setTimeout(function() {
        stickID = Entities.addEntity({
            type: "Model",
            name: "stick",
            modelURL: "https://hifi-public.s3.amazonaws.com/eric/models/stick.fbx",
            compoundShapeURL: "https://hifi-public.s3.amazonaws.com/eric/models/stick.obj",
            dimensions: {x: .11, y: .11, z: 1.0},
            position: MyAvatar.getRightPalmPosition(), // initial position doesn't matter, as long as it's close
            rotation: MyAvatar.orientation,
            damping: .1,
            collisionSoundURL: "http://public.highfidelity.io/sounds/Collisions-hitsandslaps/67LCollision07.wav",
            restitution: 0.01,
            dynamic: true
        });
        actionID = Entities.addAction("hold", stickID,
                                      {relativePosition: {x: 0.0, y: 0.0, z: -0.5},
                                       relativeRotation: Quat.fromVec3Degrees({x: 0.0, y: 90.0, z: 0.0}),
                                       hand: hand,
                                       timeScale: 0.15});
        if (actionID == nullActionID) {
            cleanUp();
        }
        makingNewStick = false;
    }, 3000);
}


function cleanUp() {
    if (stickID) {
        Entities.deleteEntity(stickID);
        stickID = null;
    }
}


function initControls(){
    if (hand == "right") {
        controllerID = 3; // right handed
    } else {
        controllerID = 4; // left handed
    }
}


Script.scriptEnding.connect(cleanUp);
makeNewStick();
