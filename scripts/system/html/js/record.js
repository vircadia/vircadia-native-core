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
    recordingsBeingPlayed = [],
    elEnableRecording,
    elInstructions,
    elRecordingsPlaying,
    elNumberOfPlayers,
    EVENT_BRIDGE_TYPE = "record",
    BODY_LOADED_ACTION = "bodyLoaded",
    USING_TOOLBAR_ACTION = "usingToolbar",
    ENABLE_RECORDING_ACTION = "enableRecording",
    RECORDINGS_BEING_PLAYED_ACTION = "recordingsBeingPlayed",
    NUMBER_OF_PLAYERS_ACTION = "numberOfPlayers",
    STOP_PLAYING_RECORDING_ACTION = "stopPlayingRecording",
    TABLET_INSTRUCTIONS = "Close the tablet to start recording",
    WINDOW_INSTRUCTIONS = "Close the window to start recording";

function updateInstructions() {
    elInstructions.innerHTML = elEnableRecording.checked ? (isUsingToolbar ? WINDOW_INSTRUCTIONS : TABLET_INSTRUCTIONS) : "";
}

function stopPlayingRecording(event) {
    var playerID = event.target.getElementsByTagName("input")[0].value;
    EventBridge.emitWebEvent(JSON.stringify({
        type: EVENT_BRIDGE_TYPE,
        action: STOP_PLAYING_RECORDING_ACTION,
        value: playerID
    }));
}

function orderRecording(a, b) {
    return a.filename > b.filename ? 1 : -1;
}

function updateRecordings() {
    var tbody,
        tr,
        td,
        span,
        input,
        length,
        i;

    recordingsBeingPlayed.sort(orderRecording);

    tbody = document.createElement("tbody");

    for (i = 0, length = recordingsBeingPlayed.length; i < length; i += 1) {
        tr = document.createElement("tr");
        td = document.createElement("td");
        td.innerHTML = recordingsBeingPlayed[i].filename.slice(4);
        tr.appendChild(td);
        td = document.createElement("td");
        span = document.createElement("span");
        span.innerHTML = "x";
        span.addEventListener("click", stopPlayingRecording);
        input = document.createElement("input");
        input.setAttribute("type", "hidden");
        input.setAttribute("value", recordingsBeingPlayed[i].playerID);
        span.appendChild(input);
        td.appendChild(span);
        tr.appendChild(td);
        tbody.appendChild(tr);
    }

    elRecordingsPlaying.replaceChild(tbody, elRecordingsPlaying.getElementsByTagName("tbody")[0]);
}

function onScriptEventReceived(data) {
    var message = JSON.parse(data);
    if (message.type === EVENT_BRIDGE_TYPE) {
        switch (message.action) {
        case ENABLE_RECORDING_ACTION:
            elEnableRecording.checked = message.value;
            updateInstructions();
            break;
        case USING_TOOLBAR_ACTION:
            isUsingToolbar = message.value;
            updateInstructions();
            break;
        case RECORDINGS_BEING_PLAYED_ACTION:
            recordingsBeingPlayed = JSON.parse(message.value);
            updateRecordings();
            break;
        case NUMBER_OF_PLAYERS_ACTION:
            elNumberOfPlayers.innerHTML = message.value;
            break;
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
    elRecordingsPlaying = document.getElementById("recordings-playing");
    elNumberOfPlayers = document.getElementById("number-of-players");

    EventBridge.emitWebEvent(JSON.stringify({
        type: EVENT_BRIDGE_TYPE,
        action: BODY_LOADED_ACTION
    }));
}
