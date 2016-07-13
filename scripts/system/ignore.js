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

            // setup a position for the overlay that is just above this avatar's head
            var overlayPosition = avatar.getJointPosition("Head");
            overlayPosition.y += 0.45;

            if (avatarID in ignoreOverlays) {
                // keep the overlay above the current position of this avatar
                Overlays.editOverlay(ignoreOverlays[avatarID], {
                    position: overlayPosition
                });
            } else {
                // add the overlay above this avatar
                var newOverlay = Overlays.addOverlay("image3d", {
                    url: Script.resolvePath("assets/images/ignore-target-01.svg"),
                    position: overlayPosition,
                    size: 0.4,
                    scale: 0.4,
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    solid: true,
                    isFacingAvatar: true,
                    drawInFront: true
                });

                // push this overlay to our array of overlays
                ignoreOverlays[avatarID] = newOverlay;
            }
        }
    }
}

Script.update.connect(updateOverlays);

AvatarList.avatarRemovedEvent.connect(function(avatarID){
    if (isShowingOverlays) {
        // we are currently showing overlays and an avatar just went away

        // first remove the rendered overlay
        Overlays.deleteOverlay(ignoreOverlays[avatarID]);

        // delete the saved ID of the overlay from our ignored overlays object
        delete ignoreOverlays[avatarID];
    }
})

Controller.mousePressEvent.connect(function(event){
    // handle click events so we can detect when our overlays are clicked

    if (!event.isLeftButton) {
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

            // cleanup of the overlay is handled by the connection to avatarRemovedEvent
        }
    }
});


// cleanup the toolbar button and overlays when script is stopped
Script.scriptEnding.connect(function() {
    toolbar.removeButton('ignore');
    removeOverlays();
});
