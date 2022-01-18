/*
    Appreciate
    Created by Zach Fox on 2019-01-30
    Copyright 2019 High Fidelity, Inc.

    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

/* globals document EventBridge setTimeout */

// Called when the user clicks the switch in the top right of the app.
// Sends an event to the App JS and clears the `firstRun` `div`.
function appreciateSwitchClicked(checkbox) {
    EventBridge.emitWebEvent(JSON.stringify({
        app: "appreciate",
        method: "appreciateSwitchClicked",
        appreciateEnabled: checkbox.checked
    }));
    document.getElementById("firstRun").style.display = "none";
}

// Called when the user checks/unchecks the Never Whistle checkbox.
// Adds the crosshatch div to the UI and sends an event to the App JS.
function neverWhistleCheckboxClicked(checkbox) {
    var crosshatch = document.getElementById("crosshatch");
    if (checkbox.checked) {
        crosshatch.style.display = "inline-block";
    } else {
        crosshatch.style.display = "none";
    }

    EventBridge.emitWebEvent(JSON.stringify({
        app: "appreciate",
        method: "neverWhistleCheckboxClicked",
        neverWhistle: checkbox.checked
    }));
}

// Called when the user checks/unchecks the Show Appreciation Entity checkbox.
// Sends an event to the App JS.
function showAppreciationEntityCheckboxClicked(checkbox) {
    EventBridge.emitWebEvent(JSON.stringify({
        app: "appreciate",
        method: "showAppreciationEntityCheckboxClicked",
        showAppreciationEntity: checkbox.checked
    }));

    if (checkbox.checked) {
        document.getElementById("colorPickerContainer").style.visibility = "visible";
    } else {
        document.getElementById("colorPickerContainer").style.visibility = "hidden";
    }
}

// Called when the user changes the entity's color using the jscolor picker.
// Modifies the color of the Intensity Meter gradient and sends a message to the App JS.
var START_COLOR_MULTIPLIER = 0.2;
function setEntityColor(jscolor) {
    var newEntityColor = {
        "red": jscolor.rgb[0],
        "green": jscolor.rgb[1],
        "blue": jscolor.rgb[2]
    };

    var startColor = {
        "red": Math.floor(newEntityColor.red * START_COLOR_MULTIPLIER),
        "green": Math.floor(newEntityColor.green * START_COLOR_MULTIPLIER),
        "blue": Math.floor(newEntityColor.blue * START_COLOR_MULTIPLIER)
    };

    var currentIntensityDisplayWidth = document.getElementById("currentIntensityDisplay").offsetWidth;
    var bgString = "linear-gradient(to right, rgb(" + startColor.red + ", " +
        startColor.green + ", " + startColor.blue + ") 0, " +
        jscolor.toHEXString() + " " + currentIntensityDisplayWidth + "px)";
    document.getElementById("currentIntensity").style.backgroundImage = bgString;

    EventBridge.emitWebEvent(JSON.stringify({
        app: "appreciate",
        method: "setEntityColor",
        entityColor: newEntityColor
    }));
}

// Handle EventBridge messages from *_app.js.
function onScriptEventReceived(message) {
    try {
        message = JSON.parse(message);
    } catch (error) {
        console.log("Couldn't parse script event message: " + error);
        return;
    }

    // This message gets sent by `entityList.js` when it shouldn't!
    if (message.type === "removeEntities") {
        return;
    }

    switch (message.method) {
        case "updateUI":
            if (message.isFirstRun) {
                document.getElementById("firstRun").style.display = "block";
            }
            document.getElementById("appreciateSwitch").checked = message.appreciateEnabled;
            document.getElementById("neverWhistleCheckbox").checked = message.neverWhistleEnabled;

            var showAppreciationEntityCheckbox = document.getElementById("showAppreciationEntityCheckbox");
            showAppreciationEntityCheckbox.checked = message.showAppreciationEntity;
            if (showAppreciationEntityCheckbox.checked) {
                document.getElementById("colorPickerContainer").style.visibility = "visible";
            } else {
                document.getElementById("colorPickerContainer").style.visibility = "hidden";
            }

            if (message.neverWhistleEnabled) {
                var crosshatch = document.getElementById("crosshatch");
                crosshatch.style.display = "inline-block";
            }

            document.getElementById("loadingContainer").style.display = "none";

            var color = document.getElementById("colorPicker").jscolor;
            color.fromRGB(message.entityColor.red, message.entityColor.green, message.entityColor.blue);

            var startColor = {
                "red": Math.floor(color.rgb[0] * START_COLOR_MULTIPLIER),
                "green": Math.floor(color.rgb[1] * START_COLOR_MULTIPLIER),
                "blue": Math.floor(color.rgb[2] * START_COLOR_MULTIPLIER)
            };
            var currentIntensityDisplayWidth = document.getElementById("currentIntensityDisplay").offsetWidth;
            document.getElementById("currentIntensity").style.backgroundImage = 
                "linear-gradient(to right, rgb(" + startColor.red + ", " +
                startColor.green + ", " + startColor.blue + ") 0, " +
                color.toHEXString() + " " + currentIntensityDisplayWidth + "px)";
            break;

        case "updateCurrentIntensityUI":
            document.getElementById("currentIntensity").style.width = message.currentIntensity * 100 + "%";
            break;

        default:
            console.log("Unknown message received from appreciate_app.js! " + JSON.stringify(message));
            break;
    }
}

// This function detects a keydown on the document, which enables the app
// to forward these keypress events to the app JS.
function onKeyDown(e) {
    var key = e.key.toUpperCase();
    if (key === "Z") {
        EventBridge.emitWebEvent(JSON.stringify({
            app: "appreciate",
            method: "zKeyDown",
            repeat: e.repeat
        }));
    }
}

// This function detects a keyup on the document, which enables the app
// to forward these keypress events to the app JS.
function onKeyUp(e) {
    var key = e.key.toUpperCase();
    if (key === "Z") {
        EventBridge.emitWebEvent(JSON.stringify({
            app: "appreciate",
            method: "zKeyUp"
        }));
    }
}

// This delay is necessary to allow for the JS EventBridge to become active.
// The delay is still necessary for HTML apps in RC78+.
var EVENTBRIDGE_SETUP_DELAY = 500;
function onLoad() {
    setTimeout(function() {
        EventBridge.scriptEventReceived.connect(onScriptEventReceived);
        EventBridge.emitWebEvent(JSON.stringify({
            app: "appreciate",
            method: "eventBridgeReady"
        }));
    }, EVENTBRIDGE_SETUP_DELAY);

    document.addEventListener("keydown", onKeyDown);
    document.addEventListener("keyup", onKeyUp);
}

onLoad();