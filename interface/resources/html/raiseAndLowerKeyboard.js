//
//  Created by Anthony Thibault on 2016-09-02
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Sends messages over the EventBridge when text input is required.
//
(function () {
    var POLL_FREQUENCY = 500; // ms
    var MAX_WARNINGS = 3;
    var numWarnings = 0;
    var isKeyboardRaised = false;

    function shouldRaiseKeyboard() {
        if (document.activeElement.nodeName == "INPUT" || document.activeElement.nodeName == "TEXTAREA") {
            return true;
        } else {
            // check for contenteditable attribute
            for (var i = 0; i < document.activeElement.attributes.length; i++) {
                if (document.activeElement.attributes[i].name === "contenteditable" &&
                    document.activeElement.attributes[i].value === "true") {
                    return true;
                }
            }
            return false;
        }
    };

    setInterval(function () {
        if (isKeyboardRaised !== shouldRaiseKeyboard()) {
            isKeyboardRaised = !isKeyboardRaised;
            if (typeof EventBridge != "undefined") {
                EventBridge.emitWebEvent(isKeyboardRaised ? "_RAISE_KEYBOARD" : "_LOWER_KEYBOARD");
            } else {
                if (numWarnings < MAX_WARNINGS) {
                    console.log("WARNING: no global EventBridge object found");
                    numWarnings++;
                }
            }
        }
    }, POLL_FREQUENCY);
})();
