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

var isShowingOverlays = true;
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
            overlayPosition.y += 1.15;

            var rows = 8;

            var text = avatarID + "\n"
                       +"--- Data from Mixer ---\n"
                       +"All: " + AvatarManager.getAvatarDataRate(avatarID).toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID).toFixed(2) + "hz)" + "\n"
                       /*
                       +" GP: " + AvatarManager.getAvatarDataRate(avatarID,"globalPosition").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"globalPosition").toFixed(2) + "hz)" + "\n"
                       +" LP: " + AvatarManager.getAvatarDataRate(avatarID,"localPosition").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"localPosition").toFixed(2) + "hz)" + "\n"
                       +" BB: " + AvatarManager.getAvatarDataRate(avatarID,"avatarBoundingBox").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"avatarBoundingBox").toFixed(2) + "hz)" + "\n"
                       +" AO: " + AvatarManager.getAvatarDataRate(avatarID,"avatarOrientation").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"avatarOrientation").toFixed(2) + "hz)" + "\n"
                       +" AS: " + AvatarManager.getAvatarDataRate(avatarID,"avatarScale").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"avatarScale").toFixed(2) + "hz)" + "\n"
                       +" LA: "  + AvatarManager.getAvatarDataRate(avatarID,"lookAtPosition").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"lookAtPosition").toFixed(2) + "hz)" + "\n"
                       +" AL: " + AvatarManager.getAvatarDataRate(avatarID,"audioLoudness").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"audioLoudness").toFixed(2) + "hz)" + "\n"
                       +" SW: " + AvatarManager.getAvatarDataRate(avatarID,"sensorToWorkMatrix").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"sensorToWorkMatrix").toFixed(2) + "hz)" + "\n"
                       +" AF: " + AvatarManager.getAvatarDataRate(avatarID,"additionalFlags").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"additionalFlags").toFixed(2) + "hz)" + "\n"
                       +" PI: " + AvatarManager.getAvatarDataRate(avatarID,"parentInfo").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"parentInfo").toFixed(2) + "hz)" + "\n"
                       +" FT: " + AvatarManager.getAvatarDataRate(avatarID,"faceTracker").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"faceTracker").toFixed(2) + "hz)" + "\n"
                       */
                       +" JD: " + AvatarManager.getAvatarDataRate(avatarID,"jointData").toFixed(2) + "kbps (" + AvatarManager.getAvatarUpdateRate(avatarID,"jointData").toFixed(2) + "hz)" + "\n"
                       +"--- Simulation ---\n"
                       +"All: " + AvatarManager.getAvatarSimulationRate(avatarID,"avatar").toFixed(2) + "hz \n"
                       +" inView: " + AvatarManager.getAvatarSimulationRate(avatarID,"avatarInView").toFixed(2) + "hz \n"
                       //+" SM: " + AvatarManager.getAvatarSimulationRate(avatarID,"skeletonModel").toFixed(2) + "hz \n"
                       +" JD: " + AvatarManager.getAvatarSimulationRate(avatarID,"jointData").toFixed(2) + "hz \n"

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
                        x: 1.25,
                        y: rows * 0.13
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
    removeOverlays();
});

}()); // END LOCAL_SCOPE
