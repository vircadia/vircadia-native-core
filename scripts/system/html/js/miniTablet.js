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
        HOVER_MESSAGE = "hover", // Engine <== Dialog
        UNHOVER_MESSAGE = "unhover", // Engine <== Dialog
        MUTE_MESSAGE = "mute", // Engine <=> Dialog
        GOTO_MESSAGE = "goto", // Engine <=> Dialog
        EXPAND_MESSAGE = "expand", // Engine <== Dialog

        muteButton,
        muteImage,
        gotoButton,
        gotoImage,
        expandButton,

        // Work around buttons staying hovered when mini tablet is replaced by tablet proper then subsequently redisplayed.
        isUnhover = true;


    function setUnhover() {
        if (!isUnhover) {
            if (gotoButton) {
                gotoButton.classList.add("unhover");
            }
            expandButton.classList.add("unhover");
            isUnhover = true;
        }
    }

    function clearUnhover() {
        if (isUnhover) {
            if (gotoButton) {
                gotoButton.classList.remove("unhover");
            }
            expandButton.classList.remove("unhover");
            isUnhover = false;
        }
    }


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
                if (muteImage) {
                    muteImage.src = message.icon;
                }
                break;
            case GOTO_MESSAGE:
                if (gotoImage) {
                    gotoImage.src = message.icon;
                }
                break;
        }
    }

    function onBodyHover() {
        EventBridge.emitWebEvent(JSON.stringify({
            type: HOVER_MESSAGE,
            target: "body"
        }));
    }

    function onBodyUnhover() {
        EventBridge.emitWebEvent(JSON.stringify({
            type: UNHOVER_MESSAGE,
            target: "body"
        }));
    }

    function onButtonHover() {
        EventBridge.emitWebEvent(JSON.stringify({
            type: HOVER_MESSAGE,
            target: "button"
        }));
        clearUnhover();
    }

    function onMuteButtonClick() {
        EventBridge.emitWebEvent(JSON.stringify({
            type: MUTE_MESSAGE
        }));
    }

    function onGotoButtonClick() {
        setUnhover();
        EventBridge.emitWebEvent(JSON.stringify({
            type: GOTO_MESSAGE
        }));
    }

    function onExpandButtonClick() {
        setUnhover();
        EventBridge.emitWebEvent(JSON.stringify({
            type: EXPAND_MESSAGE
        }));
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


    function onUnload() {
        disconnectEventBridge();
    }

    function onLoad() {
        muteButton = document.getElementById("mute");
        gotoButton = document.getElementById("goto");
        expandButton = document.getElementById("expand");

        connectEventBridge();

        document.body.addEventListener("mouseenter", onBodyHover, false);
        document.body.addEventListener("mouseleave", onBodyUnhover, false);

        if (muteButton) {
            muteImage = document.getElementById("mute-img");
            muteButton.addEventListener("mouseenter", onButtonHover, false);
            muteButton.addEventListener("click", onMuteButtonClick, true);
        }

        if (gotoButton) {
            gotoImage = document.getElementById("goto-img");
            gotoButton.addEventListener("mouseenter", onButtonHover, false);
            gotoButton.addEventListener("click", onGotoButtonClick, true);
        }

        expandButton.addEventListener("mouseenter", onButtonHover, false);
        expandButton.addEventListener("click", onExpandButtonClick, true);

        document.body.onunload = function () {
            onUnload();
        };
    }

    onLoad();

}());
