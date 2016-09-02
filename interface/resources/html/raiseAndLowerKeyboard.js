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
    var lastRaiseKeyboard = false;
    function shouldRaiseKeyboard() {
        return document.activeElement.nodeName == "INPUT" || document.activeElement.nodeName == "TEXTAREA";
    };
    setInterval(function () {
        var newRaiseKeyboard = shouldRaiseKeyboard();
        if (newRaiseKeyboard != lastRaiseKeyboard) {
            var event = newRaiseKeyboard ? "_RAISE_KEYBOARD" : "_LOWER_KEYBOARD";
            if (typeof EventBridge != "undefined") {
                EventBridge.emitWebEvent(event);
            } else {
                console.log("WARNING: no global EventBridge object found");
            }
            lastRaiseKeyboard = newRaiseKeyboard;
        }
    }, POLL_FREQUENCY);
})();
