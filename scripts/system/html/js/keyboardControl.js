//
//  keyboardControl.js
//
//  Created by David Rowe on 28 Sep 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function setUpKeyboardControl() {

    var lowerTimer = null;
    var isRaised = false;
    var KEYBOARD_HEIGHT = 200;

    function raiseKeyboard() {
        if (lowerTimer !== null) {
            clearTimeout(lowerTimer);
            lowerTimer = null;
        }

        EventBridge.emitWebEvent("_RAISE_KEYBOARD" + (this.type === "number" ? "_NUMERIC" : ""));

        if (!isRaised) {
            var delta = this.getBoundingClientRect().bottom + 10 - (document.body.clientHeight - KEYBOARD_HEIGHT);
            if (delta > 0) {
                setTimeout(function () {
                    document.body.scrollTop += delta;
                }, 500);  // Allow time for keyboard to be raised in QML.
            }
        }

        isRaised = true;
    }

    function doLowerKeyboard() {
        EventBridge.emitWebEvent("_LOWER_KEYBOARD");
        lowerTimer = null;
        isRaised = false;
    }

    function lowerKeyboard() {
        // Delay lowering keyboard a little in case immediately raise it again.
        if (lowerTimer === null) {
            lowerTimer = setTimeout(doLowerKeyboard, 20);
        }
    }

    function documentBlur() {
        // Action any pending Lower keyboard event immediately upon leaving document window so that they don't interfere with
        // other Entities Editor tab.
        if (lowerTimer !== null) {
            clearTimeout(lowerTimer);
            doLowerKeyboard();
        }
    }

    var inputs = document.querySelectorAll("input[type=text], input[type=password], input[type=number], textarea");
    for (var i = 0, length = inputs.length; i < length; i++) {
        inputs[i].addEventListener("focus", raiseKeyboard);
        inputs[i].addEventListener("blur", lowerKeyboard);
    }

    window.addEventListener("blur", documentBlur);
}

