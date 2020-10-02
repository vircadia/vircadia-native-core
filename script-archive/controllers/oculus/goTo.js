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

Script.include("../../libraries/globals.js");
Script.include("../../libraries/virtualKeyboard.js");
Script.include("../../libraries/soundArray.js");

const MAX_SHOW_INSTRUCTION_TIMES = 2;
const INSTRUCTIONS_SETTING = "GoToInstructionsShowCounter"

var windowDimensions = Controller.getViewportDimensions();

function Instructions(imageURL, originalWidth, originalHeight) {
    var tthis = this;
    this.originalSize = {w: originalWidth, h: originalHeight};
    this.visible = false;
    this.overlay = Overlays.addOverlay("image", {
                imageURL: imageURL,
                x: 0,
                y: 0,
                width: originalWidth,
                height: originalHeight,
                alpha: 1,
                visible: this.visible
    });

    this.show = function() {
        var timesShown = Settings.getValue(INSTRUCTIONS_SETTING);
        timesShown = timesShown === "" ? 0 : parseInt(timesShown);
        print(timesShown);
        if (timesShown < MAX_SHOW_INSTRUCTION_TIMES) {
            Settings.setValue(INSTRUCTIONS_SETTING, timesShown + 1);
            tthis.visible = true;
            tthis.rescale();
            Overlays.editOverlay(tthis.overlay, {visible: tthis.visible});
            return;
        }
        tthis.remove();
    }

    this.remove = function() {
        Overlays.deleteOverlay(tthis.overlay);
        tthis.visible = false;
    };

    this.rescale = function() {
        var scale = Math.min(windowDimensions.x / tthis.originalSize.w, windowDimensions.y / tthis.originalSize.h);
        var newWidth = tthis.originalSize.w * scale;
        var newHeight = tthis.originalSize.h * scale;
        Overlays.editOverlay(tthis.overlay, {
            x: (windowDimensions.x / 2) - (newWidth / 2),
            y: (windowDimensions.y / 2) - (newHeight / 2),
            width: newWidth,
            height: newHeight
        });
    };
    this.rescale();
};

var theInstruction = new Instructions(VIRCADIA_PUBLIC_CDN + "images/tutorial-goTo.svg", 457, 284.1);

var firstControllerPlugged = false;


var cursor = new Cursor({visible: false});
var keyboard = new Keyboard({visible: false});
var textFontSize = 9;
var text = null;
var locationURL = "";

var randomSounds = new SoundArray({}, true);
var numberOfSounds = 7;
for (var i = 1; i <= numberOfSounds; i++) {
    randomSounds.addSound(VIRCADIA_PUBLIC_CDN + "sounds/UI/virtualKeyboard-press" + i + ".raw");
}

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
    randomSounds.playRandom();
    if (event.event == 'keypress') {
        appendChar(event.char);
    }
};

keyboard.onKeyRelease = function(event) {
    // you can cancel a key by releasing its focusing before releasing it
    if (event.focus) {
        if (event.event == 'delete') {
            deleteChar();
        } else if (event.event == 'submit' || event.event == 'enter') {
            print("going to hifi://" + locationURL);
            location = "hifi://" + locationURL;
            locationURL = "";
            keyboard.hide();
            cursor.hide();
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
    cursor.initialize();
    cursor.updateVisibility(keyboard.visible);
    cursor.onUpdate = function(position) {
        keyboard.setFocusPosition(position.x, position.y);
    };
};

function keyPressEvent(event) {
    if (theInstruction.visible) {
        return;
    }
    if (event.key === SPACEBAR_CHARCODE) {
        keyboard.pressFocussedKey();
    } else if (event.key === ENTER_CHARCODE || event.key === RETURN_CHARCODE) {
        keyboard.toggle();
        if (cursor !== undefined) {
            cursor.updateVisibility(keyboard.visible);
        }
        Overlays.editOverlay(text, {visible: keyboard.visible});
    }
}

function keyReleaseEvent(event) {
    if (theInstruction.visible) {
        return;
    }
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
    theInstruction.remove();
}

function reportButtonValue(button, newValue, oldValue) {
    if (theInstruction.visible) {
        if (button == Joysticks.BUTTON_FACE_BOTTOM && newValue) {
            theInstruction.remove();
        }
    } else if (button == Joysticks.BUTTON_FACE_BOTTOM) {
        if (newValue) {
            keyboard.pressFocussedKey();
        } else {
            keyboard.releaseKeys();
        }
    } else if (button == Joysticks.BUTTON_FACE_RIGHT && newValue) {
        deleteChar();
    } else if (button == Joysticks.BUTTON_FACE_LEFT && newValue) {
        keyboard.hide();
        if (cursor !== undefined) {
            cursor.hide();
        }
        Overlays.editOverlay(text, {visible: false});
    } else if (button == Joysticks.BUTTON_RIGHT_SHOULDER && newValue) {
        if (keyboard.visible) {
            print("going to hifi://" + locationURL);
            location = "hifi://" + locationURL;
            locationURL = "";
            keyboard.hide();
            cursor.hide();
            updateTextOverlay();
            return;
        }
        keyboard.show();
        if (cursor !== undefined) {
            cursor.show();
        }
        Overlays.editOverlay(text, {visible: true});            
    }
}

function addJoystick(gamepad) {
    gamepad.buttonStateChanged.connect(reportButtonValue);
    if (!firstControllerPlugged) {
        firstControllerPlugged = true;
        theInstruction.show();
    }
}

var allJoysticks = Joysticks.getAllJoysticks();
for (var i = 0; i < allJoysticks.length; i++) {
    addJoystick(allJoysticks[i]);
}

Joysticks.joystickAdded.connect(addJoystick);
Controller.captureKeyEvents({key: RETURN_CHARCODE});
Controller.captureKeyEvents({key: ENTER_CHARCODE});
Controller.captureKeyEvents({key: SPACEBAR_CHARCODE});
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.scriptEnding.connect(scriptEnding);
