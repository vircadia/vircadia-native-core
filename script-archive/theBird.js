//
// theBird.js
// examples
//
// Created by Anthony Thibault on 11/9/2015
// Copyright 2015 High Fidelity, Inc.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// Example of how to play an animation on an avatar.
//

var THE_BIRD_RIGHT_URL = "https://hifi-public.s3.amazonaws.com/ozan/anim/the_bird/the_bird_right.fbx";

var roles = MyAvatar.getAnimationRoles();
var i, l = roles.length
print("getAnimationRoles()");
for (i = 0; i < l; i++) {
    print(roles[i]);
}

// replace point animations with the bird!
MyAvatar.overrideRoleAnimation("rightHandPointIntro", THE_BIRD_RIGHT_URL, 30, false, 0, 12);
MyAvatar.overrideRoleAnimation("rightHandPointHold", THE_BIRD_RIGHT_URL, 30, false, 12, 12);
MyAvatar.overrideRoleAnimation("rightHandPointOutro", THE_BIRD_RIGHT_URL, 30, false, 19, 30);

Script.scriptEnding.connect(function() {
    MyAvatar.restoreRoleAnimation("rightHandPointIntro");
    MyAvatar.restoreRoleAnimation("rightHandPointHold");
    MyAvatar.restoreRoleAnimation("rightHandPointOutro");
});
