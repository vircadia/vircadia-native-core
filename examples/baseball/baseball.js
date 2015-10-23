//
//  baseball.js
//  examples/toys
//
//  Created by Stephen Birarda on 10/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var ROBOT_MODEL = "atp:785c81e117206c36205404beec0cc68529644fe377542dbb2d13fae4d665a5de.fbx";
var ROBOT_POSITION = { x: -0.81, y: 0.88, z: 2.12 };
var ROBOT_DIMENSIONS = { x: 0.95, y: 1.76, z: 0.56 };

var BAT_MODEL = "atp:07bdd769a57ff15ebe9331ae4e2c2eae8886a6792b4790cce03b4716eb3a81c7.fbx"
var BAT_COLLISION_MODEL = "atp:atp:9eafceb7510c41d50661130090de7e0632aa4da236ebda84a0059a4be2130e0c.obj"
var BAT_DIMENSIONS = { x: 1.35, y: 0.10, z: 0.10 };
var BAT_REGISTRATION_POINT = { x: 0.1, y: 0.5, z: 0.5 };

// add the fresh robot at home plate
var robot = Entities.addEntity({
    name: 'Robot',
    type: 'Model',
    modelURL: ROBOT_MODEL,
    position: ROBOT_POSITION,
//    dimensions: ROBOT_DIMENSIONS,a
    animationIsPlaying: true,
    animation: {
        url: ROBOT_MODEL,
        fps: 30
    }
});

// add the bat
var bat = Entities.addEntity({
    name: 'Bat',
    /*/
    type: 'Box',
    /*/
    type: 'Model',
    modelURL: BAT_COLLISION_MODEL,
    /**/
    collisionModelURL: BAT_COLLISION_MODEL,
//    dimensions: BAT_DIMENSIONS,
    registrationPoint: BAT_REGISTRATION_POINT,
    visible: false
})

var lastTriggerValue = 0.0;

function checkTriggers() {
    var rightTrigger = Controller.getTriggerValue(1);

    if (rightTrigger == 0) {
        if (lastTriggerValue > 0) {
            // the trigger was just released, play out to the last frame of the swing
            Entities.editEntity(robot, {
                animation: {
                    running: true,
                    currentFrame: 21,
                    lastFrame: 115
                }
            });
        }
    } else {
        if (lastTriggerValue == 0) {
            // the trigger was just depressed, start the swing
            Entities.editEntity(robot, {
                animation: {
                    running: true,
                    currentFrame: 0,
                    lastFrame: 21
                }
            });
        }
    }

    lastTriggerValue = rightTrigger;
}

var DISTANCE_HOLDING_ACTION_TIMEFRAME = 0.1; // how quickly objects move to their new position
var ACTION_LIFETIME = 15; // seconds

var action = null;
var factor = 0.0;
var STEP = 0.05;
function moveBat() {
    var JOINT_INDEX = 19;
    
    var forearmPosition = Entities.getJointPosition(robot, JOINT_INDEX);
    var forearmRotation = Entities.getJointRotation(robot, JOINT_INDEX);
    
    /*/
    var properties = Entities.getEntityProperties(bat, ["position", "rotation"]);
    var offsetPosition = Vec3.subtract(properties.position, forearmPosition);
    var offsetRotation = Quat.multiply(Quat.inverse(forearmRotation), properties.rotation);
    print("offsetPosition = " + JSON.stringify(offsetPosition));
    print("offsetRotation = " + JSON.stringify(Quat.safeEulerAngles(offsetRotation)));
    /*/
    Entities.editEntity(bat, {
        position: forearmPosition,
        rotation: forearmRotation,
    });
    /**/
    
//    var actionProperties = {
//        relativePosition: forearmPosition,
//        relativeRotation: forearmRotation,
////        tag: "bat-to-forearm",
////        linearTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
////        angularTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
////        lifetime: ACTION_LIFETIME
//        hand: "left",
//        timeScale: 0.15
//    };
//    
//    if (action === null) {
//        Entities.addAction("hold", bat, actionProperties);
//    } else {
//        Entities.editAction(bat, action, actionProperties);
//    }
}

function update() {
//    checkTriggers();
    moveBat();
}

function scriptEnding() {
    Entities.deleteEntity(robot);
    Entities.deleteEntity(bat);
    if (action) {
        Entities.deleteAction(bat, action);
    }
}

// hook the update so we can check controller triggers
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
