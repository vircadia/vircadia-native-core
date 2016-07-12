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
var currentOverlays = [];

function hideOverlays() {
    // enumerate the overlays and remove them
    while (currentOverlays.length) {
        // shift the element to remove it from the current overlays array when it is removed
        Overlays.deleteOverlay(currentOverlays.shift());
    }
}

var OVERLAY_SIZE = 0.25;

function showOverlays() {
    var identifiers = AvatarList.getAvatarIdentifiers();

    for (i = 0; i < identifiers.length; ++i) {
        // get the position for this avatar
        var avatar = AvatarList.getAvatar(identifiers[i]);
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
        currentOverlays.push(newOverlay);
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

// remove the toolbar button when script is stopped
Script.scriptEnding.connect(function() {
    toolbar.removeButton('ignore');
    hideOverlays();
});
