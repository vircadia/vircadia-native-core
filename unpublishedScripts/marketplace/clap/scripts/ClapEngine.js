"use strict";

/*
	clapEngine.js
	unpublishedScripts/marketplace/clap/clapApp.js

	Created by Matti 'Menithal' Lahtinen on 9/11/2017
	Copyright 2017 High Fidelity, Inc.

	Distributed under the Apache License, Version 2.0.
	See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


  Main Heart of the clap script> Does both keyboard binding and tracking of the gear..

*/
var DEG_TO_RAD = Math.PI / 180;
// If angle is closer to 0 from 25 degrees, then "clap" can happen;
var COS_OF_TOLERANCE = Math.cos(25 * DEG_TO_RAD);
var DISTANCE = 0.3;
var CONTROL_MAP_PACKAGE = "com.highfidelity.avatar.clap.active";

var CLAP_MENU = "Clap";
var ENABLE_PARTICLE_MENU = "Enable Clap Particles";
var ENABLE_DEBUG_MENU = "Enable Claping Helper";

var sounds = [
    "clap1.wav",
    "clap2.wav",
    "clap3.wav",
    "clap4.wav",
    "clap5.wav",
    "clap6.wav"
];


var ClapParticle = Script.require(Script.resolvePath("../entities/ClapParticle.json?V3"));
var ClapAnimation = Script.require(Script.resolvePath("../animations/ClapAnimation.json?V3"));
var ClapDebugger = Script.require(Script.resolvePath("ClapDebugger.js?V3"));

var settingDebug = false;
var settingParticlesOn = true;

function setJointRotation(map) {
    Object.keys(map).forEach(function (key, index) {
        MyAvatar.setJointRotation(MyAvatar.getJointIndex(key), map[key].rotations);
    });
}
// Load Sounds to Cache
var cache = [];
for (var index in sounds) {
    cache.push(SoundCache.getSound(Script.resolvePath("../sounds/" + sounds[index])));
}


var previousIndex;
var clapOn = false;
var animClap = false;
var animThrottle;

function clap(volume, position, rotation) {
    var index;
    // Make sure one does not generate consequtive sounds
    do {
        index = Math.floor(Math.random() * cache.length);
    } while (index === previousIndex);

    previousIndex = index;

    Audio.playSound(cache[index], {
        position: position,
        volume: volume / 4 + Math.random() * (volume / 3)
    });

    if (settingParticlesOn) {
        ClapParticle.orientation = Quat.multiply(MyAvatar.orientation, rotation);
        ClapParticle.position = position;
        ClapParticle.emitSpeed = volume > 1 ? 1 : volume;
        Entities.addEntity(ClapParticle, true);
    }


}
// Helper Functions
function getHandFingerAnim(side) {
    return Script.resolvePath("../animations/Clap_" + side + '.fbx');
}

// Disable all role animations related to fingers for side
function overrideFingerRoleAnimation(side) {
    var anim = getHandFingerAnim(side);
    MyAvatar.overrideRoleAnimation(side + "HandGraspOpen", anim, 30, true, 0, 0);
    MyAvatar.overrideRoleAnimation(side + "HandGraspClosed", anim, 30, true, 0, 0);
    if (HMD.active) {
        MyAvatar.overrideRoleAnimation(side + "HandPointIntro", anim, 30, true, 0, 0);
        MyAvatar.overrideRoleAnimation(side + "HandPointHold", anim, 30, true, 0, 0);
        MyAvatar.overrideRoleAnimation(side + "HandPointOutro", anim, 30, true, 0, 0);

        MyAvatar.overrideRoleAnimation(side + "IndexPointOpen", anim, 30, true, 0, 0);
        MyAvatar.overrideRoleAnimation(side + "IndexPointClosed", anim, 30, true, 0, 0);
        MyAvatar.overrideRoleAnimation(side + "IndexPointAndThumbRaiseOpen", anim, 30, true, 0, 0);
        MyAvatar.overrideRoleAnimation(side + "IndexPointAndThumbRaiseClosed", anim, 30, true, 0, 0);

        MyAvatar.overrideRoleAnimation(side + "ThumbRaiseOpen", anim, 30, true, 0, 0);
        MyAvatar.overrideRoleAnimation(side + "ThumbRaiseClosed", anim, 30, true, 0, 0);
    }
}
// Re-enable all role animations for fingers
function restoreFingerRoleAnimation(side) {
    MyAvatar.restoreRoleAnimation(side + "HandGraspOpen");
    MyAvatar.restoreRoleAnimation(side + "HandGraspClosed");
    if (HMD.active) {
        MyAvatar.restoreRoleAnimation(side + "HandPointIntro");
        MyAvatar.restoreRoleAnimation(side + "HandPointHold");
        MyAvatar.restoreRoleAnimation(side + "HandPointOutro");
        MyAvatar.restoreRoleAnimation(side + "IndexPointOpen");

        MyAvatar.restoreRoleAnimation(side + "IndexPointClosed");
        MyAvatar.restoreRoleAnimation(side + "IndexPointAndThumbRaiseOpen");
        MyAvatar.restoreRoleAnimation(side + "IndexPointAndThumbRaiseClosed");
        MyAvatar.restoreRoleAnimation(side + "ThumbRaiseOpen");
        MyAvatar.restoreRoleAnimation(side + "ThumbRaiseClosed");
    }
}


function menuListener(menuItem) {
    if (menuItem === ENABLE_PARTICLE_MENU) {
        settingParticlesOn = Menu.isOptionChecked(ENABLE_PARTICLE_MENU);
    } else if (menuItem === ENABLE_DEBUG_MENU) {
        var debugOn = Menu.isOptionChecked(ENABLE_DEBUG_MENU);

        if (debugOn) {
            settingDebug = true;
            ClapDebugger.enableDebug();
        } else {
            settingDebug = false;
            ClapDebugger.disableDebug();
        }
    }
}

function update(dt) {

    // NOTICE: Someof this stuff is unnessary for the actual: But they are done for Debug Purposes!
    // Forexample, the controller doesnt really need to know where it is in the world, only its relation to the other controller!

    var leftHand = Controller.getPoseValue(Controller.Standard.LeftHand);
    var rightHand = Controller.getPoseValue(Controller.Standard.RightHand);

    // Get Offset position for palms, not the controllers that are at the wrists (7.5 cm up)

    var leftWorldRotation = Quat.multiply(MyAvatar.orientation, leftHand.rotation);
    var rightWorldRotation = Quat.multiply(MyAvatar.orientation, rightHand.rotation);

    var leftWorldUpNormal = Quat.getUp(leftWorldRotation);
    var rightWorldUpNormal = Quat.getUp(rightWorldRotation);

    var leftHandDownWorld = Vec3.multiply(-1, Quat.getForward(leftWorldRotation));
    var rightHandDownWorld = Vec3.multiply(-1, Quat.getForward(rightWorldRotation));

    //
    var leftHandWorldPosition = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, leftHand.translation));
    var rightHandWorldPosition = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, rightHand.translation));

    var rightHandPositionOffset = Vec3.sum(rightHandWorldPosition, Vec3.multiply(rightWorldUpNormal, 0.035));
    var leftHandPositionOffset = Vec3.sum(leftHandWorldPosition, Vec3.multiply(leftWorldUpNormal, 0.035));

    var leftToRightWorld = Vec3.subtract(leftHandPositionOffset, rightHandPositionOffset);
    var rightToLeftWorld = Vec3.subtract(rightHandPositionOffset, leftHandPositionOffset);

    var leftAlignmentWorld = -1 * Vec3.dot(Vec3.normalize(leftToRightWorld), leftHandDownWorld);
    var rightAlignmentWorld = -1 * Vec3.dot(Vec3.normalize(rightToLeftWorld), rightHandDownWorld);

    var distance = Vec3.distance(rightHandPositionOffset, leftHandPositionOffset);

    var matchTolerance = leftAlignmentWorld > COS_OF_TOLERANCE && rightAlignmentWorld > COS_OF_TOLERANCE;

    if (settingDebug) {
        ClapDebugger.debugPositions(leftAlignmentWorld,
            leftHandPositionOffset, leftHandDownWorld,
            rightAlignmentWorld, rightHandPositionOffset,
            rightHandDownWorld, COS_OF_TOLERANCE);
    }
    // Using subtract, because these will be heading to opposite directions
    var angularVelocity = Vec3.length(Vec3.subtract(leftHand.angularVelocity, rightHand.angularVelocity));
    var velocity = Vec3.length(Vec3.subtract(leftHand.velocity, rightHand.velocity));

    if (matchTolerance && distance < DISTANCE && !animClap) {
        if (settingDebug) {
            ClapDebugger.debugClapLine(leftHandPositionOffset, rightHandPositionOffset, true);
        }
        if (!animThrottle) {
            overrideFingerRoleAnimation("left");
            overrideFingerRoleAnimation("right");
            animClap = true;
        } else {
            Script.clearTimeout(animThrottle);
            animThrottle = false;
        }
    } else if (animClap && distance > DISTANCE) {
        if (settingDebug) {
            ClapDebugger.debugClapLine(leftHandPositionOffset, rightHandPositionOffset, false);
        }
        animThrottle = Script.setTimeout(function () {
            restoreFingerRoleAnimation("left");
            restoreFingerRoleAnimation("right");
            animClap = false;
        }, 500);
    }

    if (distance < DISTANCE && matchTolerance && !clapOn) {
        clapOn = true;

        var midClap = Vec3.mix(rightHandPositionOffset, leftHandPositionOffset, 0.5);
        var volume = velocity / 2 + angularVelocity / 5;

        clap(volume, midClap, Quat.lookAtSimple(rightHandPositionOffset, leftHandPositionOffset));
    } else if (distance > DISTANCE && !matchTolerance) {
        clapOn = false;
    }
}


module.exports = {
    connectEngine: function () {
        if (!Menu.menuExists(CLAP_MENU)) {
            Menu.addMenu(CLAP_MENU);
        }

        if (!Menu.menuItemExists(CLAP_MENU, ENABLE_PARTICLE_MENU)) {
            Menu.addMenuItem({
                menuName: CLAP_MENU,
                menuItemName: ENABLE_PARTICLE_MENU,
                isCheckable: true,
                isChecked: settingParticlesOn
            });
        }
        if (!Menu.menuItemExists(CLAP_MENU, ENABLE_DEBUG_MENU)) {
            Menu.addMenuItem({
                menuName: CLAP_MENU,
                menuItemName: ENABLE_DEBUG_MENU,
                isCheckable: true,
                isChecked: settingDebug
            });
        }


        Menu.menuItemEvent.connect(menuListener);

        var controls = Controller.newMapping(CONTROL_MAP_PACKAGE);
        var Keyboard = Controller.Hardware.Keyboard;

        controls.from(Keyboard.K).to(function (down) {
            if (down) {

                setJointRotation(ClapAnimation);
                Script.setTimeout(function () {
                    // As soon as an animation bug is fixed, this will kick  and get fixed.s.
                    overrideFingerRoleAnimation("left");
                    overrideFingerRoleAnimation("right");

                    var lh = MyAvatar.getJointPosition("LeftHand");
                    var rh = MyAvatar.getJointPosition("RightHand");
                    var midClap = Vec3.mix(rh, lh, 0.5);
                    var volume = 0.5 + Math.random() * 0.5;
                    var position = midClap;

                    clap(volume, position, Quat.fromVec3Degrees(0, 0, 0));
                }, 50); // delay is present to allow for frames to catch up.
            } else {

                restoreFingerRoleAnimation("left");

                restoreFingerRoleAnimation("right");
                MyAvatar.clearJointsData();
            }
        });
        Controller.enableMapping(CONTROL_MAP_PACKAGE);

        if (settingDebug) {
            ClapDebugger.enableDebug();
        }

        Script.update.connect(update);

        Script.scriptEnding.connect(this.disconnectEngine);
    },
    disconnectEngine: function () {
        if (settingDebug) {
            ClapDebugger.disableDebug();
        }
        if (Menu.menuItemExists(CLAP_MENU, ENABLE_PARTICLE_MENU)) {
            Menu.removeMenuItem(CLAP_MENU, ENABLE_PARTICLE_MENU);
        }
        if (Menu.menuItemExists(CLAP_MENU, ENABLE_DEBUG_MENU)) {
            Menu.removeMenuItem(CLAP_MENU, ENABLE_DEBUG_MENU);
        }

        if (Menu.menuExists(CLAP_MENU)) {
            Menu.removeMenu(CLAP_MENU);
        }
        restoreFingerRoleAnimation('left');
        restoreFingerRoleAnimation('right');

        Controller.disableMapping(CONTROL_MAP_PACKAGE);
        try {
            Script.update.disconnect(update);
        } catch (e) {
            print("Script.update connection did not exist on disconnection. Skipping");
        }
    }
};
