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
    function keyPressEvent(event) {
        if (event.text.toUpperCase() === "B" && event.isControl) {
            console.log("TEST B");
            Window.openWebBrowser();
        }
        else if (event.text.toUpperCase() === "N" && event.isControl) {
            Users.toggleIgnoreRadius();
        }
    }

    function scriptEnding() {
        Controller.keyPressEvent.disconnect(keyPressEvent);
    }

    Controller.keyPressEvent.connect(keyPressEvent);
    Script.scriptEnding.connect(scriptEnding);
}()); // END LOCAL_SCOPE
