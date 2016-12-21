"use strict";

//
//  mod.js
//  scripts/system/
//
//  Created by Stephen Birarda on 07/11/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* global Toolbars, Script, Users, Overlays, AvatarList, Controller, Camera, getControllerWorldLocation */


(function() { // BEGIN LOCAL_SCOPE

Script.include("/~/system/libraries/controllers.js");

// grab the toolbar
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

var ASSETS_PATH = Script.resolvePath("assets");
var TOOLS_PATH = Script.resolvePath("assets/images/tools/");

function buttonImageURL() {
    return (Users.canKick ? "kick.svg" : "ignore.svg");
}

// setup the mod button and add it to the toolbar
var button = tablet.addButton({
    icon: "icons/tablet-icons/ignore-i.svg",
    text: "KICK"
});

// if this user's kick permissions change, change the state of the button in the HUD
Users.canKickChanged.connect(function(canKick){
    button.editProperties({text: buttonImageURL()});
});

var isShowingOverlays = false;
var modOverlays = {};

function removeOverlays() {
    // enumerate the overlays and remove them
    var modOverlayKeys = Object.keys(modOverlays);

    for (var i = 0; i < modOverlayKeys.length; ++i) {
        var avatarID = modOverlayKeys[i];
        for (var j = 0; j < modOverlays[avatarID].length; ++j) {
            Overlays.deleteOverlay(modOverlays[avatarID][j]);
        }
    }

    modOverlays = {};
}

// handle clicks on the toolbar button
function buttonClicked(){
    if (isShowingOverlays) {
        removeOverlays();
        isShowingOverlays = false;
    } else {
        isShowingOverlays = true;
    }

    //button.writeProperty('buttonState', isShowingOverlays ? 0 : 1);
    //button.writeProperty('defaultState', isShowingOverlays ? 0 : 1);
    //button.writeProperty('hoverState', isShowingOverlays ? 2 : 3);
}

button.clicked.connect(buttonClicked);

function kickOverlayURL() {
    return ASSETS_PATH + "/images/" + (Users.canKick ? "kick-target.svg" : "ignore-target.svg");
}

function muteOverlayURL() {
    return ASSETS_PATH + "/images/" + "mute-target.svg";
}

function updateOverlays() {
    if (isShowingOverlays) {

        var identifiers = AvatarList.getAvatarIdentifiers();

        for (var i = 0; i < identifiers.length; ++i) {
            var avatarID = identifiers[i];

            if (avatarID === null) {
                // this is our avatar, skip it
                continue;
            }

            // get the position for this avatar
            var avatar = AvatarList.getAvatar(avatarID);
            var avatarPosition = avatar && avatar.position;

            if (!avatarPosition) {
                // we don't have a valid position for this avatar, skip it
                continue;
            }

            // setup a position for the overlay that is just above this avatar's head
            var kickOverlayPosition = avatar.getJointPosition("Head");
            kickOverlayPosition.y += 0.45;
            var muteOverlayPosition = avatar.getJointPosition("Head");
            muteOverlayPosition.y += 0.70;

            if (avatarID in modOverlays) {
                // keep the overlay above the current position of this avatar
                Overlays.editOverlay(modOverlays[avatarID][0], {
                    position: kickOverlayPosition,
                    url: kickOverlayURL()
                });
                if (Users.canKick) {
                    Overlays.editOverlay(modOverlays[avatarID][1], {
                        position: muteOverlayPosition,
                        url: muteOverlayURL()
                    });
                }
            } else {
                // add the overlay above this avatar
                var newKickOverlay = Overlays.addOverlay("image3d", {
                    url: kickOverlayURL(),
                    position: kickOverlayPosition,
                    size: 1,
                    scale: 0.4,
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    solid: true,
                    isFacingAvatar: true,
                    drawInFront: true
                });

                modOverlays[avatarID]=[newKickOverlay];

                if (Users.canKick) {
                    var newMuteOverlay = Overlays.addOverlay("image3d", {
                        url: muteOverlayURL(),
                        position: muteOverlayPosition,
                        size: 1,
                        scale: 0.4,
                        color: { red: 255, green: 255, blue: 255},
                        alpha: 1,
                        solid: true,
                        isFacingAvatar: true,
                        drawInFront: true
                    });
                    // push this overlay to our array of overlays
                    modOverlays[avatarID].push(newMuteOverlay);
                }
            }
        }
    }
}

Script.update.connect(updateOverlays);

AvatarList.avatarRemovedEvent.connect(function(avatarID){
    if (isShowingOverlays) {
        // we are currently showing overlays and an avatar just went away

        // first remove the rendered overlays
        for (var j = 0; j < modOverlays[avatarID].length; ++j) {
            Overlays.deleteOverlay(modOverlays[avatarID][j]);
        }
        
        // delete the saved ID of the overlay from our mod overlays object
        delete modOverlays[avatarID];
    }
});

function handleSelectedOverlay(clickedOverlay) {
    // see this is one of our mod overlays

    var modOverlayKeys = Object.keys(modOverlays);
    for (var i = 0; i < modOverlayKeys.length; ++i) {
        var avatarID = modOverlayKeys[i];
        var modOverlay = modOverlays[avatarID][0];
        var muteOverlay = modOverlays[avatarID][1];

        if (clickedOverlay.overlayID == modOverlay) {
            // matched to an overlay, ask for the matching avatar to be kicked or ignored
            if (Users.canKick) {
                Users.kick(avatarID);
            } else {
                Users.ignore(avatarID);
            }
            // cleanup of the overlay is handled by the connection to avatarRemovedEvent

        } else if (muteOverlay && clickedOverlay.overlayID == muteOverlay) {
            Users.mute(avatarID);
        }
    }
}

Controller.mousePressEvent.connect(function(event){
    if (isShowingOverlays) {
        // handle click events so we can detect when our overlays are clicked

        if (!event.isLeftButton) {
            // if another mouse button than left is pressed ignore it
            return false;
        }

        // compute the pick ray from the event
        var pickRay = Camera.computePickRay(event.x, event.y);

        // grab the clicked overlay for the given pick ray
        var clickedOverlay = Overlays.findRayIntersection(pickRay);
        if (clickedOverlay.intersects) {
            handleSelectedOverlay(clickedOverlay);
        }
    }
});

// We get mouseMoveEvents from the handControllers, via handControllerPointer.
// But we dont' get mousePressEvents.
var triggerMapping = Controller.newMapping(Script.resolvePath('') + '-click');

function controllerComputePickRay(hand) {
    var controllerPose = getControllerWorldLocation(hand, true);
    if (controllerPose.valid) {
        return { origin: controllerPose.position, direction: Quat.getUp(controllerPose.orientation) };
    }
}

function makeClickHandler(hand) {
    return function(clicked) {
        if (clicked == 1.0 && isShowingOverlays) {
            var pickRay = controllerComputePickRay(hand);
            if (pickRay) {
                var overlayIntersection = Overlays.findRayIntersection(pickRay);
                if (overlayIntersection.intersects) {
                    handleSelectedOverlay(overlayIntersection);
                }
            }
        }
    };
}
triggerMapping.from(Controller.Standard.RTClick).peek().to(makeClickHandler(Controller.Standard.RightHand));
triggerMapping.from(Controller.Standard.LTClick).peek().to(makeClickHandler(Controller.Standard.LeftHand));

triggerMapping.enable();

// cleanup the toolbar button and overlays when script is stopped
Script.scriptEnding.connect(function() {
    tablet.removeButton(button);
    removeOverlays();
    triggerMapping.disable();
});

}()); // END LOCAL_SCOPE
