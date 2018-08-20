//
//  miniTablet.js
//
//  Created by David Rowe on 20 Aug 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global EventBridge */
/* eslint-env browser */

(function () {

    "use strict";

    var // EventBridge
        READY_MESSAGE = "ready", // Engine <== Dialog
        MUTE_MESSAGE = "mute", // Engine <=> Dialog
        BUBBLE_MESSAGE = "bubble", // Engine <=> Dialog

        muteButton,
        muteImage,
        bubbleButton,
        bubbleImage;

    // #region Communications ==================================================================================================

    function onScriptEventReceived(data) {
        var message;

        try {
            message = JSON.parse(data);
        } catch (e) {
            console.error("EventBridge message error");
            return;
        }

        switch (message.type) {
            case MUTE_MESSAGE:
                muteImage.src = message.icon;
                break;
            case BUBBLE_MESSAGE:
                bubbleButton.classList.remove(message.on ? "off" : "on");
                bubbleButton.classList.add(message.on ? "on" : "off");
                bubbleImage.src = message.icon;
                break;
        }
    }

    function connectEventBridge() {
        EventBridge.scriptEventReceived.connect(onScriptEventReceived);
        EventBridge.emitWebEvent(JSON.stringify({
            type: READY_MESSAGE
        }));
    }

    function disconnectEventBridge() {
        EventBridge.scriptEventReceived.disconnect(onScriptEventReceived);
    }

    // #endregion

    // #region Set-up and tear-down ============================================================================================

    function onUnload() {
        disconnectEventBridge();
    }

    function onLoad() {
        muteButton = document.getElementById("mute");
        muteImage = document.getElementById("mute-img");
        bubbleButton = document.getElementById("bubble");
        bubbleImage = document.getElementById("bubble-img");

        connectEventBridge();

        document.body.onunload = function () {
            onUnload();
        };
    }

    onLoad();

    // #endregion

}());
