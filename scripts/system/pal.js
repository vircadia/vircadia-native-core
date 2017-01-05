"use strict";
/*jslint vars: true, plusplus: true, forin: true*/
/*globals Script, AvatarList, Users, Entities, MyAvatar, Camera, Overlays, OverlayWindow, Toolbars, Vec3, Quat, Controller, print, getControllerWorldLocation */
//
// pal.js
//
// Created by Howard Stearns on December 9, 2016
// Copyright 2016 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// FIXME when we make this a defaultScript: (function() { // BEGIN LOCAL_SCOPE

Script.include("/~/system/libraries/controllers.js");

//
// Overlays.
//
var overlays = {}; // Keeps track of all our extended overlay data objects, keyed by target identifier.
function ExtendedOverlay(key, type, properties, selected) { // A wrapper around overlays to store the key it is associated with.
    overlays[key] = this;
    this.key = key;
    this.selected = selected || false; // not undefined
    this.activeOverlay = Overlays.addOverlay(type, properties); // We could use different overlays for (un)selected...
}
// Instance methods:
ExtendedOverlay.prototype.deleteOverlay = function () { // remove display and data of this overlay
    Overlays.deleteOverlay(this.activeOverlay);
    delete overlays[this.key];
};

ExtendedOverlay.prototype.editOverlay = function (properties) { // change display of this overlay
    Overlays.editOverlay(this.activeOverlay, properties);
};
const UNSELECTED_COLOR = {red: 20, green: 250, blue: 20};
const SELECTED_COLOR = {red: 250, green: 20, blue: 20};
function color(selected) { return selected ? SELECTED_COLOR : UNSELECTED_COLOR; }
ExtendedOverlay.prototype.select = function (selected) {
    if (this.selected === selected) {
        return;
    }
    this.editOverlay({color: color(selected)});
    this.selected = selected;
};
// Class methods:
var selectedIds = [];
ExtendedOverlay.isSelected = function (id) {
    return -1 !== selectedIds.indexOf(id);
};
ExtendedOverlay.get = function (key) { // answer the extended overlay data object associated with the given avatar identifier
    return overlays[key];
};
ExtendedOverlay.some = function (iterator) { // Bails early as soon as iterator returns truthy.
    var key;
    for (key in overlays) {
        if (iterator(ExtendedOverlay.get(key))) {
            return;
        }
    }
};
ExtendedOverlay.applyPickRay = function (pickRay, cb) { // cb(overlay) on the one overlay intersected by pickRay, if any.
    var pickedOverlay = Overlays.findRayIntersection(pickRay); // Depends on nearer coverOverlays to extend closer to us than farther ones.
    if (!pickedOverlay.intersects) {
        return;
    }
    ExtendedOverlay.some(function (overlay) { // See if pickedOverlay is one of ours.
        if ((overlay.activeOverlay) === pickedOverlay.overlayID) {
            cb(overlay);
            return true;
        }
    });
};


//
// Similar, for entities
//
function HighlightedEntity(id, entityProperties) {
    this.id = id;
    this.overlay = Overlays.addOverlay('cube', {
        position: entityProperties.position,
        rotation: entityProperties.rotation,
        dimensions: entityProperties.dimensions,
        solid: false,
        color: {
            red: 0xF3,
            green: 0x91,
            blue: 0x29
        },
        lineWidth: 1.0,
        ignoreRayIntersection: true,
        drawInFront: true
    });
    HighlightedEntity.overlays.push(this);
}
HighlightedEntity.overlays = [];
HighlightedEntity.clearOverlays = function clearHighlightedEntities() {
    HighlightedEntity.overlays.forEach(function (highlighted) {
        Overlays.deleteOverlay(highlighted.overlay);
    });
    HighlightedEntity.overlays = [];
};
HighlightedEntity.updateOverlays = function updateHighlightedEntities() {
    HighlightedEntity.overlays.forEach(function (highlighted) {
        var properties = Entities.getEntityProperties(highlighted.id, ['position', 'rotation', 'dimensions']);
        Overlays.editOverlay(highlighted.overlay, {
            position: properties.position,
            rotation: properties.rotation,
            dimensions: properties.dimensions
        });
    });
};

//
// The qml window and communications.
//
var pal = new OverlayWindow({
    title: 'People Action List',
    source: 'hifi/Pal.qml',
    width: 580,
    height: 640,
    visible: false
});
pal.fromQml.connect(function (message) { // messages are {method, params}, like json-rpc. See also sendToQml.
    print('From PAL QML:', JSON.stringify(message));
    switch (message.method) {
    case 'selected':
        selectedIds = message.params;
        ExtendedOverlay.some(function (overlay) {
            var id = overlay.key;
            var selected = ExtendedOverlay.isSelected(id);
            overlay.select(selected);
        });

        HighlightedEntity.clearOverlays();
        if (selectedIds.length) {
            Entities.findEntitiesInFrustum(Camera.frustum).forEach(function (id) {
                // Because lastEditedBy is per session, the vast majority of entities won't match,
                // so it would probably be worth reducing marshalling costs by asking for just we need.
                // However, providing property name(s) is advisory and some additional properties are
                // included anyway. As it turns out, asking for 'lastEditedBy' gives 'position', 'rotation',
                // and 'dimensions', too, so we might as well make use of them instead of making a second
                // getEntityProperties call.
                // It would be nice if we could harden this against future changes by specifying all
                // and only these four in an array, but see
                // https://highfidelity.fogbugz.com/f/cases/2728/Entities-getEntityProperties-id-lastEditedBy-name-lastEditedBy-doesn-t-work
                var properties = Entities.getEntityProperties(id, 'lastEditedBy');
                if (ExtendedOverlay.isSelected(properties.lastEditedBy)) {
                    new HighlightedEntity(id, properties);
                }
            });
        }
        break;
    case 'refresh':
        removeOverlays();
        populateUserList();
        break;
    default:
        print('Unrecognized message from Pal.qml:', JSON.stringify(message));
    }
});

//
// Main operations.
//
function addAvatarNode(id) {
    var selected = ExtendedOverlay.isSelected(id);
    return new ExtendedOverlay(id, "sphere", { // 3d so we don't go cross-eyed looking at it, but on top of everything
        solid: true,
        alpha: 0.8,
        color: color(selected),
        drawInFront: true
    }, selected);
}
function populateUserList() {
    var data = [];
    AvatarList.getAvatarIdentifiers().sort().forEach(function (id) { // sorting the identifiers is just an aid for debugging
        var avatar = AvatarList.getAvatar(id);
        var avatarPalDatum = {
            displayName: avatar.sessionDisplayName,
            userName: '',
            sessionId: id || '',
            audioLevel: 0.0
        };
        // If the current user is an admin OR
        // they're requesting their own username ("id" is blank)...
        if (Users.canKick || !id) {
            // Request the username from the given UUID
            Users.requestUsernameFromID(id);
        }
        // Request personal mute status and ignore status
        // from NodeList (as long as we're not requesting it for our own ID)
        if (id) {
            avatarPalDatum['personalMute'] = Users.getPersonalMuteStatus(id);
            avatarPalDatum['ignore'] = Users.getIgnoreStatus(id);
            addAvatarNode(id); // No overlay for ourselves
        }
        data.push(avatarPalDatum);
        print('PAL data:', JSON.stringify(avatarPalDatum));
    });
    pal.sendToQml({method: 'users', params: data});
}

// The function that handles the reply from the server
function usernameFromIDReply(id, username, machineFingerprint) {
    var data;
    // If the ID we've received is our ID...
    if (MyAvatar.sessionUUID === id) {
        // Set the data to contain specific strings.
        data = ['', username];
    } else {
        // Set the data to contain the ID and the username (if we have one)
        // or fingerprint (if we don't have a username) string.
        data = [id, username || machineFingerprint];
    }
    print('Username Data:', JSON.stringify(data));
    // Ship the data off to QML
    pal.sendToQml({ method: 'updateUsername', params: data });
}

var pingPong = true;
function updateOverlays() {
    var eye = Camera.position;
    AvatarList.getAvatarIdentifiers().forEach(function (id) {
        if (!id) {
            return; // don't update ourself
        }
        var overlay = ExtendedOverlay.get(id);
        if (!overlay) { // For now, we're treating this as a temporary loss, as from the personal space bubble. Add it back.
            print('Adding non-PAL avatar node', id);
            overlay = addAvatarNode(id);
        }
        var avatar = AvatarList.getAvatar(id);
        var target = avatar.position;
        var distance = Vec3.distance(target, eye);
        overlay.ping = pingPong;
        overlay.editOverlay({
            position: target,
            dimensions: 0.05 * distance // constant apparent size
        });
    });
    pingPong = !pingPong;
    ExtendedOverlay.some(function (overlay) { // Remove any that weren't updated. (User is gone.)
        if (overlay.ping === pingPong) {
            overlay.deleteOverlay();
        }
    });
    // We could re-populateUserList if anything added or removed, but not for now.
    HighlightedEntity.updateOverlays();
}
function removeOverlays() {
    selectedIds = [];
    HighlightedEntity.clearOverlays();
    ExtendedOverlay.some(function (overlay) { overlay.deleteOverlay(); });
}

//
// Clicks.
//
function handleClick(pickRay) {
    ExtendedOverlay.applyPickRay(pickRay, function (overlay) {
        // Don't select directly. Tell qml, who will give us back a list of ids.
        var message = {method: 'select', params: [overlay.key, !overlay.selected]};
        pal.sendToQml(message);
        return true;
    });
}
function handleMouseEvent(mousePressEvent) { // handleClick if we get one.
    if (!mousePressEvent.isLeftButton) {
        return;
    }
    handleClick(Camera.computePickRay(mousePressEvent.x, mousePressEvent.y));
}
// We get mouseMoveEvents from the handControllers, via handControllerPointer.
// But we don't get mousePressEvents.
var triggerMapping = Controller.newMapping(Script.resolvePath('') + '-click');
function controllerComputePickRay(hand) {
    var controllerPose = getControllerWorldLocation(hand, true);
    if (controllerPose.valid) {
        return { origin: controllerPose.position, direction: Quat.getUp(controllerPose.orientation) };
    }
}
function makeClickHandler(hand) {
    return function (clicked) {
        if (clicked > 0.85) {
            var pickRay = controllerComputePickRay(hand);
            handleClick(pickRay);
        }
    };
}
triggerMapping.from(Controller.Standard.RTClick).peek().to(makeClickHandler(Controller.Standard.RightHand));
triggerMapping.from(Controller.Standard.LTClick).peek().to(makeClickHandler(Controller.Standard.LeftHand));

//
// Manage the connection between the button and the window.
//
var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
var buttonName = "pal";
var button = toolBar.addButton({
    objectName: buttonName,
    imageURL: Script.resolvePath("assets/images/tools/people.svg"),
    visible: true,
    hoverState: 2,
    defaultState: 1,
    buttonState: 1,
    alpha: 0.9
});
var isWired = false;
function off() {
    if (isWired) { // It is not ok to disconnect these twice, hence guard.
        Script.update.disconnect(updateOverlays);
        Controller.mousePressEvent.disconnect(handleMouseEvent);
        isWired = false;
    }
    triggerMapping.disable(); // It's ok if we disable twice.
    removeOverlays();
    Users.requestsDomainListData = false;
}
function onClicked() {
    if (!pal.visible) {
        Users.requestsDomainListData = true;
        populateUserList();
        pal.raise();
        isWired = true;
        Script.update.connect(updateOverlays);
        Controller.mousePressEvent.connect(handleMouseEvent);
        triggerMapping.enable();
    } else {
        off();
    }
    pal.setVisible(!pal.visible);
}

var AVERAGING_RATIO = 0.05;
var LOUDNESS_FLOOR = 11.0;
var LOUDNESS_SCALE = 2.8 / 5.0;
var LOG2 = Math.log(2.0);
var AUDIO_LEVEL_UPDATE_INTERVAL_MS = 100; // 10hz for now (change this and change the AVERAGING_RATIO too)
var accumulatedLevels = {};

function getAudioLevel(id) {
    // the VU meter should work similarly to the one in AvatarInputs: log scale, exponentially averaged
    // But of course it gets the data at a different rate, so we tweak the averaging ratio and frequency
    // of updating (the latter for efficiency too).
    var avatar = AvatarList.getAvatar(id);
    var audioLevel = 0.0;

    // we will do exponential moving average by taking some the last loudness and averaging
    accumulatedLevels[id] = AVERAGING_RATIO * (accumulatedLevels[id] || 0) + (1 - AVERAGING_RATIO) * (avatar.audioLoudness);

    // add 1 to insure we don't go log() and hit -infinity.  Math.log is
    // natural log, so to get log base 2, just divide by ln(2).
    var logLevel = Math.log(accumulatedLevels[id] + 1) / LOG2;

    if (logLevel <= LOUDNESS_FLOOR) {
        audioLevel = logLevel / LOUDNESS_FLOOR * LOUDNESS_SCALE;
    } else {
        audioLevel = (logLevel - (LOUDNESS_FLOOR - 1.0)) * LOUDNESS_SCALE;
    }
    if (audioLevel > 1.0) {
        audioLevel = 1;
    }
    return audioLevel;
}


// we will update the audioLevels periodically
// TODO: tune for efficiency - expecially with large numbers of avatars
Script.setInterval(function () {
    if (pal.visible) {
        var param = {};
        AvatarList.getAvatarIdentifiers().sort().forEach(function (id) {
            var level = getAudioLevel(id);
            // qml didn't like an object with null/empty string for a key, so...
            var userId = id || 0;
            param[userId] = level;
        });
        pal.sendToQml({method: 'updateAudioLevel', params: param});
    }
}, AUDIO_LEVEL_UPDATE_INTERVAL_MS);
//
// Button state.
//
function onVisibleChanged() {
    button.writeProperty('buttonState', pal.visible ? 0 : 1);
    button.writeProperty('defaultState', pal.visible ? 0 : 1);
    button.writeProperty('hoverState', pal.visible ? 2 : 3);
}
button.clicked.connect(onClicked);
pal.visibleChanged.connect(onVisibleChanged);
pal.closed.connect(off);
Users.usernameFromIDReply.connect(usernameFromIDReply);
function clearIgnoredInQMLAndClosePAL() {
    pal.sendToQml({ method: 'clearIgnored' });
    if (pal.visible) {
        onClicked(); // Close the PAL
    }
}
Window.domainChanged.connect(clearIgnoredInQMLAndClosePAL);
Window.domainConnectionRefused.connect(clearIgnoredInQMLAndClosePAL);

//
// Cleanup.
//
Script.scriptEnding.connect(function () {
    button.clicked.disconnect(onClicked);
    toolBar.removeButton(buttonName);
    pal.visibleChanged.disconnect(onVisibleChanged);
    pal.closed.disconnect(off);
    Users.usernameFromIDReply.disconnect(usernameFromIDReply);
    Window.domainChanged.disconnect(clearIgnoredInQMLAndClosePAL);
    Window.domainConnectionRefused.disconnect(clearIgnoredInQMLAndClosePAL);
    off();
});


// FIXME: }()); // END LOCAL_SCOPE
