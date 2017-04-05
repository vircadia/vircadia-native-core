"use strict";

//
//  record.js
//
//  Created by David Rowe on 5 Apr 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var isUsingToolbar = false,
    elEnableRecording,
    elInstructions,
    EVENT_BRIDGE_TYPE = "record",
    BODY_LOADED_ACTION = "bodyLoaded",
    USING_TOOLBAR_ACTION = "usingToolbar",
    ENABLE_RECORDING_ACTION = "enableRecording",
    TABLET_INSTRUCTIONS = "Close the tablet to start recording",
    WINDOW_INSTRUCTIONS = "Close the window to start recording";

function updateInstructions() {
    elInstructions.innerHTML = elEnableRecording.checked ? (isUsingToolbar ? WINDOW_INSTRUCTIONS : TABLET_INSTRUCTIONS) : "";
}

function onScriptEventReceived(data) {
    var message = JSON.parse(data);
    if (message.type === EVENT_BRIDGE_TYPE) {
        if (message.action === ENABLE_RECORDING_ACTION) {
            elEnableRecording.checked = message.value;
            updateInstructions();
        } else if (message.action === USING_TOOLBAR_ACTION) {
            isUsingToolbar = message.value;
            updateInstructions();
        }
    }
}

function onBodyLoaded() {

    EventBridge.scriptEventReceived.connect(onScriptEventReceived);

    elEnableRecording = document.getElementById("enable-recording");
    elEnableRecording.onchange = function () {
        updateInstructions();
        EventBridge.emitWebEvent(JSON.stringify({
            type: EVENT_BRIDGE_TYPE,
            action: ENABLE_RECORDING_ACTION,
            value: elEnableRecording.checked
        }));
    };

    elInstructions = document.getElementById("instructions");

    EventBridge.emitWebEvent(JSON.stringify({
        type: EVENT_BRIDGE_TYPE,
        action: BODY_LOADED_ACTION
    }));
}
