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

var elEnableRecording,
    elInstructions,
    EVENT_BRIDGE_TYPE = "record",
    BODY_LOADED_ACTION = "bodyLoaded",
    ENABLE_RECORDING_ACTION = "enableRecording";

function onScriptEventReceived(data) {
    var message = JSON.parse(data);
    if (message.type === EVENT_BRIDGE_TYPE) {
        if (message.action === ENABLE_RECORDING_ACTION) {
            elEnableRecording.checked = message.value;
        }
    }
}

function onBodyLoaded() {

    EventBridge.scriptEventReceived.connect(onScriptEventReceived);

    elEnableRecording = document.getElementById("enable-recording");
    elEnableRecording.onchange = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: EVENT_BRIDGE_TYPE,
            action: ENABLE_RECORDING_ACTION,
            value: elEnableRecording.checked
        }));
    };

    EventBridge.emitWebEvent(JSON.stringify({
        type: EVENT_BRIDGE_TYPE,
        action: BODY_LOADED_ACTION
    }));
}
