//
//  disableAvatarAnimations.js
//  examples
//
//  Copyright 2016 High Fidelity, Inc.
//
//  When launched, it will replace all of the avatars animations with a single frame idle pose.
//  full body IK and hand grabbing animations will still continue to function, but all other
//  animations will be replaced.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var skeletonModelURL = "";
var jointCount = 0;

var excludedRoles = ["rightHandGraspOpen", "rightHandGraspClosed", "leftHandGraspOpen", "leftHandGraspClosed"];
var IDLE_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/ozan/dev/anim/standard_anims_160127/idle.fbx");

function overrideAnims() {
    var roles = MyAvatar.getAnimationRoles();
    var i, l = roles.length;
    for (i = 0; i < l; i++) {
        if (excludedRoles.indexOf(roles[i]) == -1) {
            MyAvatar.overrideRoleAnimation(roles[i], IDLE_URL, 30, false, 1, 1);
        }
    }
}

function restoreAnims() {
    var roles = MyAvatar.getAnimationRoles();
    var i, l = roles.length;
    for (i = 0; i < l; i++) {
        if (excludedRoles.indexOf(roles[i]) == -1) {
            MyAvatar.restoreRoleAnimation(roles[i]);
        }
    }
}

overrideAnims();

MyAvatar.onLoadComplete.connect(function () {
    overrideAnims();
});

Script.scriptEnding.connect(function () {
    restoreAnims();
});


