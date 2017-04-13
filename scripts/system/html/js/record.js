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
    numberOfPlayers = 0,
    recordingsBeingPlayed = [],
    elRecordingsPlaying,
    elPlayersUnused,
    elLoadButton,
    elRecordButton,
    EVENT_BRIDGE_TYPE = "record",
    BODY_LOADED_ACTION = "bodyLoaded",
    RECORDINGS_BEING_PLAYED_ACTION = "recordingsBeingPlayed",
    NUMBER_OF_PLAYERS_ACTION = "numberOfPlayers",
    STOP_PLAYING_RECORDING_ACTION = "stopPlayingRecording",
    LOAD_RECORDING_ACTION = "loadRecording",
    START_RECORDING_ACTION = "startRecording";

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

function updatePlayersUnused() {
    elPlayersUnused.innerHTML = numberOfPlayers - recordingsBeingPlayed.length;
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

    // Empty rows representing available players.
    for (i = recordingsBeingPlayed.length, length = numberOfPlayers; i < length; i += 1) {
        tr = document.createElement("tr");
        td = document.createElement("td");
        td.colSpan = 2;
        tr.appendChild(td);
        tbody.appendChild(tr);
    }

    elRecordingsPlaying.replaceChild(tbody, elRecordingsPlaying.getElementsByTagName("tbody")[0]);
}

function updateLoadButton() {
    if (numberOfPlayers > recordingsBeingPlayed.length) {
        elLoadButton.removeAttribute("disabled");
    } else {
        elLoadButton.setAttribute("disabled", "disabled");
    }
}

function onScriptEventReceived(data) {
    var message = JSON.parse(data);
    if (message.type === EVENT_BRIDGE_TYPE) {
        switch (message.action) {
        case RECORDINGS_BEING_PLAYED_ACTION:
            recordingsBeingPlayed = JSON.parse(message.value);
            updateRecordings();
            updatePlayersUnused();
            updateLoadButton();
            break;
        case NUMBER_OF_PLAYERS_ACTION:
            numberOfPlayers = message.value;
            updateRecordings();
            updatePlayersUnused();
            updateLoadButton();
            break;
        }
    }
}

function onBodyLoaded() {

    EventBridge.scriptEventReceived.connect(onScriptEventReceived);

    elRecordingsPlaying = document.getElementById("recordings-playing");
    elPlayersUnused = document.getElementById("players-unused");

    elLoadButton = document.getElementById("load-button");
    elLoadButton.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: EVENT_BRIDGE_TYPE,
            action: LOAD_RECORDING_ACTION
        }));
    }

    elRecordButton = document.getElementById("record-button");
    elRecordButton.onclick = function () {
        EventBridge.emitWebEvent(JSON.stringify({
            type: EVENT_BRIDGE_TYPE,
            action: START_RECORDING_ACTION
        }));
    }

    EventBridge.emitWebEvent(JSON.stringify({
        type: EVENT_BRIDGE_TYPE,
        action: BODY_LOADED_ACTION
    }));
}
