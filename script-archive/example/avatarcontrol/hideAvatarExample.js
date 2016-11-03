//
//  hideAvatarExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates how to enable or disable local rendering of your own avatar
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function keyReleaseEvent(event) {
    if (event.text == "r") {
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
