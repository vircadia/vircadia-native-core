"use strict";

//
//  debugAvatarMixer.js
//  scripts/developer/debugging
//
//  Created by Brad Hefta-Gaub on 01/09/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* global Toolbars, Script, Users, Overlays, AvatarList, Controller, Camera, getControllerWorldLocation */


(function() { // BEGIN LOCAL_SCOPE

Script.include("/~/system/libraries/controllers.js");

// grab the toolbar
var toolbar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");

var ASSETS_PATH = Script.resolvePath("assets");
var TOOLS_PATH = Script.resolvePath("assets/images/tools/");

function buttonImageURL() {
    return TOOLS_PATH + (Users.canKick ? 'kick.svg' : 'ignore.svg');
}

// setup the mod button and add it to the toolbar
var button = toolbar.addButton({
    objectName: 'debugAvatarMixer',
    imageURL: buttonImageURL(),
    visible: true,
    buttonState: 1,
    defaultState: 1,
    hoverState: 3,
    alpha: 0.9
});

var isShowingOverlays = false;
var debugOverlays = {};

function removeOverlays() {
    // enumerate the overlays and remove them
    var overlayKeys = Object.keys(debugOverlays);

    for (var i = 0; i < overlayKeys.length; ++i) {
        var avatarID = overlayKeys[i];
        for (var j = 0; j < debugOverlays[avatarID].length; ++j) {
            Overlays.deleteOverlay(debugOverlays[avatarID][j]);
        }
    }

    debugOverlays = {};
}

// handle clicks on the toolbar button
function buttonClicked(){
    if (isShowingOverlays) {
        removeOverlays();
        isShowingOverlays = false;
    } else {
        isShowingOverlays = true;
    }

    button.writeProperty('buttonState', isShowingOverlays ? 0 : 1);
    button.writeProperty('defaultState', isShowingOverlays ? 0 : 1);
    button.writeProperty('hoverState', isShowingOverlays ? 2 : 3);
}

button.clicked.connect(buttonClicked);

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
            var overlayPosition = avatar.getJointPosition("Head");
            overlayPosition.y += 1.05;

            var text = " All: " + AvatarManager.getAvatarDataRate(avatarID).toFixed(2) + "\n"
                       +" GP: " + AvatarManager.getAvatarDataRate(avatarID,"globalPosition").toFixed(2) + "\n"
                       +" LP: " + AvatarManager.getAvatarDataRate(avatarID,"localPosition").toFixed(2) + "\n"
                       +" AD: " + AvatarManager.getAvatarDataRate(avatarID,"avatarDimensions").toFixed(2) + "\n"
                       +" AO: " + AvatarManager.getAvatarDataRate(avatarID,"avatarOrientation").toFixed(2) + "\n"
                       +" AS: " + AvatarManager.getAvatarDataRate(avatarID,"avatarScale").toFixed(2) + "\n"
                       +" LA: "  + AvatarManager.getAvatarDataRate(avatarID,"lookAtPosition").toFixed(2) + "\n"
                       +" AL: " + AvatarManager.getAvatarDataRate(avatarID,"audioLoudness").toFixed(2) + "\n"
                       +" SW: " + AvatarManager.getAvatarDataRate(avatarID,"sensorToWorkMatrix").toFixed(2) + "\n"
                       +" AF: " + AvatarManager.getAvatarDataRate(avatarID,"additionalFlags").toFixed(2) + "\n"
                       +" PI: " + AvatarManager.getAvatarDataRate(avatarID,"parentInfo").toFixed(2) + "\n"
                       +" FT: " + AvatarManager.getAvatarDataRate(avatarID,"faceTracker").toFixed(2) + "\n"
                       +" JD: " + AvatarManager.getAvatarDataRate(avatarID,"jointData").toFixed(2);

            if (avatarID in debugOverlays) {
                // keep the overlay above the current position of this avatar
                Overlays.editOverlay(debugOverlays[avatarID][0], {
                    position: overlayPosition,
                    text: text
                });
            } else {
                // add the overlay above this avatar
                var newOverlay = Overlays.addOverlay("text3d", {
                    position: overlayPosition,
                    dimensions: {
                        x: 1,
                        y: 13 * 0.13
                    },
                    lineHeight: 0.1,
                    font:{size:0.1},
                    text: text,
                    size: 1,
                    scale: 0.4,
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    solid: true,
                    isFacingAvatar: true,
                    drawInFront: true
                });

                debugOverlays[avatarID]=[newOverlay];
            }
        }
    }
}

Script.update.connect(updateOverlays);

AvatarList.avatarRemovedEvent.connect(function(avatarID){
    if (isShowingOverlays) {
        // we are currently showing overlays and an avatar just went away

        // first remove the rendered overlays
        for (var j = 0; j < debugOverlays[avatarID].length; ++j) {
            Overlays.deleteOverlay(debugOverlays[avatarID][j]);
        }
        
        // delete the saved ID of the overlay from our mod overlays object
        delete debugOverlays[avatarID];
    }
});

// cleanup the toolbar button and overlays when script is stopped
Script.scriptEnding.connect(function() {
    toolbar.removeButton('debugAvatarMixer');
    removeOverlays();
});

}()); // END LOCAL_SCOPE
