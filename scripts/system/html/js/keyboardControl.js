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

    function raiseKeyboard() {
        if (lowerTimer !== null) {
            clearTimeout(lowerTimer);
            lowerTimer = null;
        }
        EventBridge.emitWebEvent("_RAISE_KEYBOARD");
    }

    function doLowerKeyboard() {
        EventBridge.emitWebEvent("_LOWER_KEYBOARD");
        lowerTimer = null;
    }

    function lowerKeyboard() {
        // Delay lowering keyboard a little in case immediately raise it again.
        if (lowerTimer === null) {
            lowerTimer = setTimeout(doLowerKeyboard, 20);
        }
    }

    var inputs = document.querySelectorAll("input[type=text], input[type=number], textarea");
    for (var i = 0, length = inputs.length; i < length; i++) {
        inputs[i].addEventListener("focus", raiseKeyboard);
        inputs[i].addEventListener("blur", lowerKeyboard);
    }
}

