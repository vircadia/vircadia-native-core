'use strict';

//
//  sit.js
//
//  Created by Kalila L. on Nov 4 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var PICK_FILTERS = Picks.PICK_ENTITIES | Picks.PICK_OVERLAYS | Picks.PICK_AVATARS | Picks.PICK_INCLUDE_NONCOLLIDABLE;
var HAND_JOINT = '_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND'.replace('RIGHT', MyAvatar.getDominantHand().toUpperCase());
var JOINT_NAME = HMD.active ? HAND_JOINT : 'Mouse';
var mainPick;
var lastTriggeredPick;
var isAvatarSitting = false;

function toggleSit () {
    if (!isAvatarSitting) {
        var position = lastTriggeredPick.intersection;
        MyAvatar.beginSit(position, MyAvatar.orientation);
        isAvatarSitting = true;
    } else {
        MyAvatar.endSit(MyAvatar.position, MyAvatar.orientation);
        isAvatarSitting = false;
    }
}

function capturePickPosition () {
    lastTriggeredPick = Picks.getPrevPickResult(mainPick);
}

function startup() {
    mainPick = Picks.createPick(PickType.Ray, {
        joint: JOINT_NAME,
        filter: PICK_FILTERS,
        enabled: true
    });
}

startup();

Script.scriptEnding.connect(function () {
    Picks.removePick(mainPick);
});

module.exports = {
    toggleSit: toggleSit,
    capturePickPosition: capturePickPosition
}