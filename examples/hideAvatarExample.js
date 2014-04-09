//
//  hideAvatarExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates how to enable or disable local rendering of your own avatar
//
//

function keyReleaseEvent(event) {
    if (event.text == "F2") {
        MyAvatar.shouldRenderLocally = !MyAvatar.shouldRenderLocally;
    }
}

// Map keyPress and mouse move events to our callbacks
Controller.keyReleaseEvent.connect(keyReleaseEvent);


function scriptEnding() {
    // re-enabled the standard behavior
    MyAvatar.shouldRenderLocally = true;
}

Script.scriptEnding.connect(scriptEnding);
