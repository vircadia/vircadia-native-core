//
//  ignore.js
//  scripts/system/
//
//  Created by Stephen Birarda on 07/11/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// grab the toolbar
var toolbar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");

// setup the ignore button and add it to the toolbar
var button = toolbar.addButton({
    objectName: 'ignore',
    imageURL: Script.resolvePath("assets/images/tools/ignore.svg"),
    visible: true,
    buttonState: 1,
    alpha: 0.9
});

var isShowingOverlays = false;
var ignoreOverlays = {};

function removeOverlays() {
    // enumerate the overlays and remove them
    var ignoreOverlayKeys = Object.keys(ignoreOverlays);

    for (i = 0; i < ignoreOverlayKeys.length; ++i) {
        var avatarID = ignoreOverlayKeys[i];
        Overlays.deleteOverlay(ignoreOverlays[avatarID]);
    }

    ignoreOverlays = {};
}

var OVERLAY_SIZE = 0.25;

function updateOverlays() {
    if (isShowingOverlays) {

        var identifiers = AvatarList.getAvatarIdentifiers();

        for (i = 0; i < identifiers.length; ++i) {
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

            if (avatarID in ignoreOverlays) {

            } else {

                // add the overlay above this avatar
                var newOverlay = Overlays.addOverlay("cube", {
                    position: avatarPosition,
                    size: 0.25,
                    color: { red: 0, green: 0, blue: 255},
                    alpha: 1,
                    solid: true
                });

                // push this overlay to our array of overlays
                ignoreOverlays[avatarID] = newOverlay;
            }
        }
    }
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
}

button.clicked.connect(buttonClicked);

Controller.mousePressEvent.connect(function(event){
    // handle click events so we can detect when our overlays are clicked

    if (!event.isLeftButton && !that.triggered) {
        // if another mouse button than left is pressed ignore it
        return false;
    }

    // compute the pick ray from the event
    var pickRay = Camera.computePickRay(event.x, event.y);

    // grab the clicked overlay for the given pick ray
    var clickedOverlay = Overlays.findRayIntersection(pickRay);

    // see this is one of our ignore overlays
    var ignoreOverlayKeys = Object.keys(ignoreOverlays)
    for (i = 0; i < ignoreOverlayKeys.length; ++i) {
        var avatarID = ignoreOverlayKeys[i];
        var ignoreOverlay = ignoreOverlays[avatarID];

        if (clickedOverlay.overlayID == ignoreOverlay) {
            // matched to an overlay, ask for the matching avatar to be ignored
            Users.ignore(avatarID);

            // remove the actual overlay so it is no longer rendered
            Overlays.deleteOverlay(ignoreOverlay);

            // remove the overlay ID and avatar ID from our internal array
            delete ignoreOverlays[avatarID];
        }
    }
});

Script.update.connect(updateOverlays);

// cleanup the toolbar button and overlays when script is stopped
Script.scriptEnding.connect(function() {
    toolbar.removeButton('ignore');
    removeOverlays();
});
