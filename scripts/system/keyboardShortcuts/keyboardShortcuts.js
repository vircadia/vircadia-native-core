"use strict";

//
//  keyboardShortcuts.js
//  scripts/system/keyboardShortcuts
//
//  Created by Preston Bezos on 06/28/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () { // BEGIN LOCAL_SCOPE
    var snapActivateSound = SoundCache.getSound(Script.resourcesPath() + "sounds/snapshot/snap.wav");
    function keyPressEvent(event) {
        if (event.text.toUpperCase() === "B" && event.isControl && !event.isShifted && !event.isAlt && !event.isCommand) {
            Window.openWebBrowser();
        } else if (event.text.toUpperCase() === "N" && event.isControl && !event.isShifted && !event.isAlt && !event.isCommand) {
            Users.toggleIgnoreRadius();
        } else if (event.text.toUpperCase() === "P" && !event.isControl && !event.isShifted && !event.isAlt && !event.isCommand) {
            Audio.playSound(snapActivateSound, {
                position: { x: MyAvatar.position.x, y: MyAvatar.position.y, z: MyAvatar.position.z },
                localOnly: true,
                volume: 0.5
            });
            Window.takeSnapshot(true);
        }
    }

    function scriptEnding() {
        Controller.keyPressEvent.disconnect(keyPressEvent);
    }

    Controller.keyPressEvent.connect(keyPressEvent);
    Script.scriptEnding.connect(scriptEnding);
}()); // END LOCAL_SCOPE
