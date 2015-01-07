//
//  goTo.js
//  examples
//
//  Created by Thijs Wenker on 12/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Control a virtual keyboard using your favorite HMD.
//  Usage: Enable VR-mode and go to First person mode,
//  look at the key that you would like to press, and press the spacebar on your "REAL" keyboard.
//
//  Enter a location URL using your HMD. Press Enter to pop-up the virtual keyboard and location input.
//  Press Space on the keyboard or the X button on your gamepad to press a key that you have selected.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/virtualKeyboard.js");

var windowDimensions = Controller.getViewportDimensions();
var cursor = null;
var keyboard = new Keyboard({visible: false});
var textFontSize = 9;
var text = null;
var locationURL = "";

function appendChar(char) {
    locationURL += char;
    updateTextOverlay();
    Overlays.editOverlay(text, {text: locationURL});
}

function deleteChar() {
    if (locationURL.length > 0) {
        locationURL = locationURL.substring(0, locationURL.length - 1);
        updateTextOverlay();
    }
}

function updateTextOverlay() {
    var maxLineWidth = Overlays.textSize(text, locationURL).width;
    var suggestedFontSize = (windowDimensions.x / maxLineWidth) * textFontSize * 0.90;
    var maxFontSize = 140;
    textFontSize = (suggestedFontSize > maxFontSize) ? maxFontSize : suggestedFontSize;
    var topMargin = (250 - textFontSize) / 4;
    Overlays.editOverlay(text, {text: locationURL, font: {size: textFontSize}, topMargin: topMargin, visible: keyboard.visible});
    maxLineWidth = Overlays.textSize(text, locationURL).width;
    Overlays.editOverlay(text, {leftMargin: (windowDimensions.x - maxLineWidth) / 2});
}

keyboard.onKeyPress = function(event) {
    if (event.event == 'keypress') {
        appendChar(event.char);
    }
};

keyboard.onKeyRelease = function(event) {
    print("Key release event test");
    // you can cancel a key by releasing its focusing before releasing it
    if (event.focus) {
        if (event.event == 'delete') {
           deleteChar();
        } else if (event.event == 'submit' || event.event == 'enter') {
           print("going to hifi://" + locationURL);
           location = "hifi://" + locationURL;
           locationURL = "";
		   keyboard.hide();
           updateTextOverlay();
        }
    }
};

keyboard.onFullyLoaded = function() {
    print("Virtual-keyboard fully loaded.");
    var dimensions = Controller.getViewportDimensions();
    text = Overlays.addOverlay("text", {
         x: 0,
         y: dimensions.y - keyboard.height() - 260,
         width: dimensions.x,
         height: 250,
         backgroundColor: { red: 255, green: 255, blue: 255},
         color: { red: 0, green: 0, blue: 0},
         topMargin: 5,
         leftMargin: 0,
         font: {size: textFontSize},
         text: "",
         alpha: 0.8,
         visible: keyboard.visible
    });
    updateTextOverlay();
    // the cursor is being loaded after the keyboard, else it will be on the background of the keyboard 
    cursor = new Cursor();
    cursor.onUpdate = function(position) {
        keyboard.setFocusPosition(position.x, position.y);
    };
};

function keyPressEvent(event) {
    if (event.key === SPACEBAR_CHARCODE) {
        keyboard.pressFocussedKey();
    } else if (event.key === ENTER_CHARCODE || event.key === RETURN_CHARCODE) {
        keyboard.toggle();
        Overlays.editOverlay(text, {visible: keyboard.visible});
    }
}

function keyReleaseEvent(event) {
    if (event.key === SPACEBAR_CHARCODE) {
        keyboard.releaseKeys();
    }
}

function scriptEnding() {
    keyboard.remove();
    cursor.remove();
    Overlays.deleteOverlay(text);
    Overlays.deleteOverlay(textSizeMeasureOverlay);
    Controller.releaseKeyEvents({key: SPACEBAR_CHARCODE});
    Controller.releaseKeyEvents({key: RETURN_CHARCODE});
    Controller.releaseKeyEvents({key: ENTER_CHARCODE});
}

function reportButtonValue(button, newValue, oldValue) {
    if (button == Joysticks.BUTTON_FACE_LEFT) {
        if (newValue) {
            keyboard.pressFocussedKey();
        } else {
            keyboard.releaseKeys();
        }
    }
}

function addJoystick(gamepad) {
    gamepad.buttonStateChanged.connect(reportButtonValue);
}

Joysticks.joystickAdded.connect(addJoystick);
Controller.captureKeyEvents({key: RETURN_CHARCODE});
Controller.captureKeyEvents({key: ENTER_CHARCODE});
Controller.captureKeyEvents({key: SPACEBAR_CHARCODE});
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.scriptEnding.connect(scriptEnding);
