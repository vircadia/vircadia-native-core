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
    imageURL: Script.resolvePath("assets/images/tools/mic.svg"),
    visible: true,
    buttonState: 1,
    alpha: 0.9
});

var isShowingOverlays = false;
var ignoreOverlays = [];

function hideOverlays() {
    // enumerate the overlays and remove them
    while (ignoreOverlays.length) {
        // shift the element to remove it from the current overlays array when it is removed
        Overlays.deleteOverlay(ignoreOverlays.shift()['overlayID']);
    }
}

var OVERLAY_SIZE = 0.25;

function showOverlays() {
    var identifiers = AvatarList.getAvatarIdentifiers();

    for (i = 0; i < identifiers.length; ++i) {
        // get the position for this avatar
        var identifier = identifiers[i];

        if (identifier === null) {
            // this is our avatar, skip it
            break;
        }

        var avatar = AvatarList.getAvatar(identifier);
        var avatarPosition = avatar && avatar.position;

        if (!avatarPosition) {
            // we don't have a valid position for this avatar, skip it
            break;
        }

        // add the overlay above this avatar
        var newOverlay = Overlays.addOverlay("cube", {
            position: avatarPosition,
            size: 0.25,
            color: { red: 0, green: 0, blue: 255},
            alpha: 1,
            solid: true
        });

        // push this overlay to our array of overlays
        ignoreOverlays.push({
            avatarID: identifiers[i],
            overlayID: newOverlay
        });
    }
}

// handle clicks on the toolbar button
function buttonClicked(){
    if (isShowingOverlays) {
        hideOverlays();
        isShowingOverlays = false;
    } else {
        showOverlays();
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
    for (i = 0; i < ignoreOverlays.length; ++i) {
        var ignoreOverlay = ignoreOverlays[i]['overlayID']
        if (clickedOverlay.overlayID == ignoreOverlay) {
            // matched to an overlay, ask for the matching avatar to be ignored
            Users.ignore(ignoreOverlays[i]['avatarID']);

            // remove the actual overlay
            Overlays.deleteOverlay(ignoreOverlay);

            // remove the overlay from our internal array
            ignoreOverlays.splice(i, 1);
        }
    }
});

// cleanup the toolbar button and overlays when script is stopped
Script.scriptEnding.connect(function() {
    toolbar.removeButton('ignore');
    hideOverlays();
});
