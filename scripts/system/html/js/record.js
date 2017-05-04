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
    isDisplayingInstructions = false,
    isRecording = false,
    numberOfPlayers = 0,
    recordingsBeingPlayed = [],
    elRecordings,
    elRecordingsTable,
    elRecordingsList,
    elInstructions,
    elPlayersUnused,
    elHideInfoButton,
    elShowInfoButton,
    elLoadButton,
    elSpinner,
    elCountdownNumber,
    elRecordButton,
    elFinishOnOpen,
    elFinishOnOpenLabel,
    EVENT_BRIDGE_TYPE = "record",
    BODY_LOADED_ACTION = "bodyLoaded",
    USING_TOOLBAR_ACTION = "usingToolbar",
    RECORDINGS_BEING_PLAYED_ACTION = "recordingsBeingPlayed",
    NUMBER_OF_PLAYERS_ACTION = "numberOfPlayers",
    STOP_PLAYING_RECORDING_ACTION = "stopPlayingRecording",
    LOAD_RECORDING_ACTION = "loadRecording",
    START_RECORDING_ACTION = "startRecording",
    SET_COUNTDOWN_NUMBER_ACTION = "setCountdownNumber",
    STOP_RECORDING_ACTION = "stopRecording",
    FINISH_ON_OPEN_ACTION = "finishOnOpen";

function stopPlayingRecording(event) {
    var playerID = event.target.getAttribute("playerID");
    EventBridge.emitWebEvent(JSON.stringify({
        type: EVENT_BRIDGE_TYPE,
        action: STOP_PLAYING_RECORDING_ACTION,
        value: playerID
    }));
}

function updatePlayersUnused() {
    elPlayersUnused.innerHTML = numberOfPlayers - recordingsBeingPlayed.length;
}

function updateRecordings() {
    var tbody,
        tr,
        td,
        input,
        ths,
        tds,
        length,
        i,
        HIFI_GLYPH_CLOSE = "w";

    tbody = document.createElement("tbody");
    tbody.id = "recordings-list";


    // <tr><td>Filename</td><td><input type="button" class="glyph red" value="w" playerID=id /></td></tr>
    for (i = 0, length = recordingsBeingPlayed.length; i < length; i += 1) {
        tr = document.createElement("tr");
        td = document.createElement("td");
        td.innerHTML = recordingsBeingPlayed[i].filename.slice(4);
        tr.appendChild(td);
        td = document.createElement("td");
        input = document.createElement("input");
        input.setAttribute("type", "button");
        input.setAttribute("class", "glyph red");
        input.setAttribute("value", HIFI_GLYPH_CLOSE);
        input.setAttribute("playerID", recordingsBeingPlayed[i].playerID);
        input.addEventListener("click", stopPlayingRecording);
        td.appendChild(input);
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

    // Filler row for extra table space.
    tr = document.createElement("tr");
    tr.classList.add("filler");
    td = document.createElement("td");
    td.colSpan = 2;
    tr.appendChild(td);
    tbody.appendChild(tr);

    // Update table content.
    elRecordingsTable.replaceChild(tbody, elRecordingsList);
    elRecordingsList = document.getElementById("recordings-list");

    // Update header cell widths to match content widths.
    ths = document.querySelectorAll("#recordings-table thead th");
    tds = document.querySelectorAll("#recordings-table tbody tr:first-child td");
    for (i = 0; i < ths.length; i += 1) {
        ths[i].width = tds[i].offsetWidth;
    }
}

function updateInstructions() {
    // Display show/hide instructions buttons if players are available.
    if (numberOfPlayers === 0) {
        elHideInfoButton.classList.add("hidden");
        elShowInfoButton.classList.add("hidden");
    } else {
        elHideInfoButton.classList.remove("hidden");
        elShowInfoButton.classList.remove("hidden");
    }

    // Display instructions if user requested or no players available.
    if (isDisplayingInstructions || numberOfPlayers === 0) {
        elRecordingsList.classList.add("hidden");
        elInstructions.classList.remove("hidden");
    } else {
        elInstructions.classList.add("hidden");
        elRecordingsList.classList.remove("hidden");
    }
}

function showInstructions() {
    isDisplayingInstructions = true;
    updateInstructions();
}

function hideInstructions() {
    isDisplayingInstructions = false;
    updateInstructions();
}

function updateLoadButton() {
    if (isRecording || numberOfPlayers <= recordingsBeingPlayed.length) {
        elLoadButton.setAttribute("disabled", "disabled");
    } else {
        elLoadButton.removeAttribute("disabled");
    }
}

function updateSpinner() {
    if (isRecording) {
        elRecordings.classList.add("hidden");
        elSpinner.classList.remove("hidden");
    } else {
        elSpinner.classList.add("hidden");
        elRecordings.classList.remove("hidden");
    }
}

function updateFinishOnOpenLabel() {
    var WINDOW_FINISH_ON_OPEN_LABEL = "Stop recording automatically when reopen this window",
        TABLET_FINISH_ON_OPEN_LABEL = "Stop recording automatically when reopen tablet or window";

    elFinishOnOpenLabel.innerHTML = isUsingToolbar ? WINDOW_FINISH_ON_OPEN_LABEL : TABLET_FINISH_ON_OPEN_LABEL;
}

function onScriptEventReceived(data) {
    var message = JSON.parse(data);
    if (message.type === EVENT_BRIDGE_TYPE) {
        switch (message.action) {
        case USING_TOOLBAR_ACTION:
            isUsingToolbar = message.value;
            updateFinishOnOpenLabel();
            break;
        case FINISH_ON_OPEN_ACTION:
            elFinishOnOpen.checked = message.value;
            break;
        case START_RECORDING_ACTION:
            isRecording = true;
            elRecordButton.value = "Stop";
            updateSpinner();
            updateLoadButton();
            break;
        case SET_COUNTDOWN_NUMBER_ACTION:
            elCountdownNumber.innerHTML = message.value;
            break;
        case STOP_RECORDING_ACTION:
            isRecording = false;
            elRecordButton.value = "Record";
            updateSpinner();
            updateLoadButton();
            break;
        case RECORDINGS_BEING_PLAYED_ACTION:
            recordingsBeingPlayed = JSON.parse(message.value);
            updateRecordings();
            updatePlayersUnused();
            updateInstructions();
            updateLoadButton();
            break;
        case NUMBER_OF_PLAYERS_ACTION:
            numberOfPlayers = message.value;
            updateRecordings();
            updatePlayersUnused();
            updateInstructions();
            updateLoadButton();
            break;
        }
    }
}

function onLoadButtonClicked() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: EVENT_BRIDGE_TYPE,
        action: LOAD_RECORDING_ACTION
    }));
}

function onRecordButtonClicked() {
    if (!isRecording) {
        elRecordButton.value = "Stop";
        EventBridge.emitWebEvent(JSON.stringify({
            type: EVENT_BRIDGE_TYPE,
            action: START_RECORDING_ACTION
        }));
        isRecording = true;
        updateSpinner();
        updateLoadButton();
    } else {
        elRecordButton.value = "Record";
        EventBridge.emitWebEvent(JSON.stringify({
            type: EVENT_BRIDGE_TYPE,
            action: STOP_RECORDING_ACTION
        }));
        isRecording = false;
        updateSpinner();
        updateLoadButton();
    }
}

function onFinishOnOpenClicked() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: EVENT_BRIDGE_TYPE,
        action: FINISH_ON_OPEN_ACTION,
        value: elFinishOnOpen.checked
    }));
}

function signalBodyLoaded() {
    EventBridge.emitWebEvent(JSON.stringify({
        type: EVENT_BRIDGE_TYPE,
        action: BODY_LOADED_ACTION
    }));
}

function onBodyLoaded() {

    EventBridge.scriptEventReceived.connect(onScriptEventReceived);

    elRecordings = document.getElementById("recordings");

    elRecordingsTable = document.getElementById("recordings-table");
    elRecordingsList = document.getElementById("recordings-list");
    elInstructions = document.getElementById("instructions");
    elPlayersUnused = document.getElementById("players-unused");

    elHideInfoButton = document.getElementById("hide-info-button");
    elHideInfoButton.onclick = hideInstructions;
    elShowInfoButton = document.getElementById("show-info-button");
    elShowInfoButton.onclick = showInstructions;

    elLoadButton = document.getElementById("load-button");
    elLoadButton.onclick = onLoadButtonClicked;

    elSpinner = document.getElementById("spinner");
    elCountdownNumber = document.getElementById("countdown-number");

    elRecordButton = document.getElementById("record-button");
    elRecordButton.onclick = onRecordButtonClicked;

    elFinishOnOpen = document.getElementById("finish-on-open");
    elFinishOnOpen.onclick = onFinishOnOpenClicked;

    elFinishOnOpenLabel = document.getElementById("finish-on-open-label");

    signalBodyLoaded();
}
