"use strict";
//
//  displayNames.js
//  scripts/system/
//
//  Created by Cristian Duarte & Gabriel Calero on May 3, 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() { // BEGIN LOCAL_SCOPE

var MAX_DISTANCE_PX = 20; // Should we use dp instead?
var UNKNOWN_NAME = "Unknown";
var METERS_ABOVE_HEAD = 0.4;

var TEXT_LINE_HEIGHT = .1;
var TEXT_MARGIN = 0.025;

var HIDE_MS = 10000;

var currentTouchToAnalyze = null;

var currentlyShownAvatar = {
    avatarID: null,
    avatar: null,
    overlay: null
};

var logEnabled = false;

var hideTimer = null;

function printd(str) {
    if (logEnabled) {       
        print("[displayNames.js] " + str);
    }
}

function clearOverlay() {
    currentlyShownAvatar.avatar = null;
    if (currentlyShownAvatar.overlay) {
        Overlays.editOverlay(currentlyShownAvatar.overlay, {visible: false});
    }
}

function touchedAvatar(avatarID, avatarData) {
    printd("[AVATARNAME] touchEnd FOUND " + JSON.stringify(avatarData));

    if (hideTimer) {
        Script.clearTimeout(hideTimer);
    }

    // Case: touching an already selected avatar
    if (currentlyShownAvatar.avatar && currentlyShownAvatar.avatarID == avatarID) {
        clearOverlay();
        return;
    }

    // Save currently selected avatar
    currentlyShownAvatar.avatarID = avatarID;
    currentlyShownAvatar.avatar = avatarData;

    if (currentlyShownAvatar.overlay == null) {
        var over = Overlays.addOverlay("text3d", {
            lineHeight: TEXT_LINE_HEIGHT,
            color: { red: 255, green: 255, blue: 255},
            backgroundColor: {red: 0, green: 0, blue: 0},
            leftMargin: TEXT_MARGIN,
            topMargin: TEXT_MARGIN,
            rightMargin: TEXT_MARGIN,
            bottomMargin: TEXT_MARGIN,
            alpha: 0.6,
            solid: true,
            isFacingAvatar: true,
            visible: false
        });
        currentlyShownAvatar.overlay = over;
    }

    var nameToShow = avatarData.displayName ? avatarData.displayName : 
        (avatarData.sessionDisplayName ? avatarData.sessionDisplayName : UNKNOWN_NAME);
    var textSize = Overlays.textSize(currentlyShownAvatar.overlay, nameToShow);

    Overlays.editOverlay(currentlyShownAvatar.overlay, {
        dimensions: {
            x: textSize.width + 2 * TEXT_MARGIN,
            y: TEXT_LINE_HEIGHT + 2 * TEXT_MARGIN
        },
        localPosition: { x: 0, y: METERS_ABOVE_HEAD, z: 0 },
        text: nameToShow,
        parentID: avatarData.sessionUUID,
        parentJointIndex: avatarData.getJointIndex("Head"),
        visible: true
    });

    hideTimer = Script.setTimeout(function() {
        clearOverlay();
    }, HIDE_MS);
}

function touchBegin(event) {
    currentTouchToAnalyze = event;
}

function touchEnd(event) {
    if (Vec3.distance({x: event.x, y: event.y }, {x: currentTouchToAnalyze.x, y: currentTouchToAnalyze.y}) > MAX_DISTANCE_PX) {
        printd("[AVATARNAME] touchEnd moved too much");
        currentTouchToAnalyze = null;
        return;
    }

    var pickRay = Camera.computePickRay(event.x, event.y);
    var avatarRay = AvatarManager.findRayIntersection(pickRay, [], [MyAvatar.sessionUUID])

    if (avatarRay.intersects) {
        touchedAvatar(avatarRay.avatarID, AvatarManager.getAvatar(avatarRay.avatarID));
    } else {
        printd("[AVATARNAME] touchEnd released outside the avatar");
    }

    currentTouchToAnalyze = null;
}

var runAtLeastOnce = false;

function ending() {
    if (!runAtLeastOnce) {
        return;
    }

    Controller.touchBeginEvent.disconnect(touchBegin);
    Controller.touchEndEvent.disconnect(touchEnd);
    Controller.mousePressEvent.disconnect(touchBegin);
    Controller.mouseReleaseEvent.disconnect(touchEnd);

    if (currentlyShownAvatar.overlay) {
        Overlays.deleteOverlay(currentlyShownAvatar.overlay);
        currentlyShownAvatar.overlay = null;
    }
    if (currentlyShownAvatar.avatar) {
        currentlyShownAvatar.avatar = null;
    }
}

function init() {
    Controller.touchBeginEvent.connect(touchBegin);
    Controller.touchEndEvent.connect(touchEnd);
    Controller.mousePressEvent.connect(touchBegin);
    Controller.mouseReleaseEvent.connect(touchEnd);

    Script.scriptEnding.connect(function () {
        ending();
    });

    runAtLeastOnce = true;
}

module.exports = {
    init: init,
    ending: ending
}

//init(); // Enable to use in desktop as a standalone

}()); // END LOCAL_SCOPE