//
//  virtualKeyboardTextEntityExample.js
//  examples
//
//  Created by Thijs Wenker on 12/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Control a virtual keyboard using your favorite HMD.
//  Usage: Enable VR-mode and go to First person mode,
//  look at the key that you would like to press, and press the spacebar on your "REAL" keyboard.
//
//  leased some code from newEditEntities.js for Text Entity example
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/globals.js");
Script.include("../../libraries/virtualKeyboard.js");
Script.include("../../libraries/soundArray.js");

const SPAWN_DISTANCE = 1;
const DEFAULT_TEXT_DIMENSION_Z = 0.02;

const TEXT_MARGIN_TOP = 0.15;
const TEXT_MARGIN_LEFT = 0.15;
const TEXT_MARGIN_RIGHT = 0.17;
const TEXT_MARGIN_BOTTOM = 0.17;

var windowDimensions = Controller.getViewportDimensions();
var cursor = new Cursor();
var keyboard = new Keyboard();
var textFontSize = 9;
var text = null;
var textText = "";
var textSizeMeasureOverlay = Overlays.addOverlay("text3d", {visible: false});

var randomSounds = new SoundArray({}, true);
var numberOfSounds = 7;
for (var i = 1; i <= numberOfSounds; i++) {
    randomSounds.addSound(VIRCADIA_PUBLIC_CDN + "sounds/UI/virtualKeyboard-press" + i + ".raw");
}

function appendChar(char) {
    textText += char;
    updateTextOverlay();
    Overlays.editOverlay(text, {text: textText});
}

function deleteChar() {
    if (textText.length > 0) {
        textText = textText.substring(0, textText.length - 1);
        updateTextOverlay();
    }
}
  
function updateTextOverlay() {
    var textLines = textText.split("\n");
    var suggestedFontSize = (windowDimensions.x / Overlays.textSize(text, textText).width) * textFontSize * 0.90;
    var maxFontSize = 170 / textLines.length;
    textFontSize = (suggestedFontSize > maxFontSize) ? maxFontSize : suggestedFontSize;
    var topMargin = (250 - (textFontSize * textLines.length)) / 4;
    Overlays.editOverlay(text, {text: textText, font: {size: textFontSize}, topMargin: topMargin});
    Overlays.editOverlay(text, {leftMargin: (windowDimensions.x - Overlays.textSize(text, textLines).width) / 2});
}

keyboard.onKeyPress = function(event) {
    randomSounds.playRandom();
    if (event.event == 'keypress') {
        appendChar(event.char);
    } else if (event.event == 'enter') {
        appendChar("\n");
    }
};

keyboard.onKeyRelease = function(event) {
    print("Key release event test");
    // you can cancel a key by releasing its focusing before releasing it
    if (event.focus) {
        if (event.event == 'delete') {
            deleteChar();
        } else if (event.event == 'submit') {
           print(textText);

           var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));

           var textLines = textText.split("\n");
           var maxLineWidth = Overlays.textSize(textSizeMeasureOverlay, textText).width;
           var usernameLine = "--" + GlobalServices.username;
           var usernameWidth = Overlays.textSize(textSizeMeasureOverlay, usernameLine).width;
           if (maxLineWidth < usernameWidth) {
               maxLineWidth = usernameWidth;
           } else {
                var spaceableWidth = maxLineWidth - usernameWidth;
                //TODO: WORKAROUND WARNING BELOW Fix this when spaces are not trimmed out of the textsize calculation anymore
                var spaceWidth = Overlays.textSize(textSizeMeasureOverlay, "| |").width
                    - Overlays.textSize(textSizeMeasureOverlay, "||").width;
                var numberOfSpaces = Math.floor(spaceableWidth / spaceWidth);
                for (var i = 0; i < numberOfSpaces; i++) {
                    usernameLine = " " + usernameLine;
                }
           }
           var dimension_x = maxLineWidth + TEXT_MARGIN_RIGHT + TEXT_MARGIN_LEFT;
           if (position.x > 0 && position.y > 0 && position.z > 0) {
               Entities.addEntity({ 
                   type: "Text",
                   rotation: MyAvatar.orientation,
                   position: position,
                   dimensions: { x: dimension_x, y: (textLines.length + 1) * 0.14 + TEXT_MARGIN_TOP + TEXT_MARGIN_BOTTOM, z: DEFAULT_TEXT_DIMENSION_Z },
                   backgroundColor: { red: 0, green: 0, blue: 0 },
                   textColor: { red: 255, green: 255, blue: 255 },
                   text: textText + "\n" + usernameLine,
                   lineHeight: 0.1
               });
           }
           textText = "";
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
         alpha: 0.8
    });
    updateTextOverlay();
    // the cursor is being loaded after the keyboard, else it will be on the background of the keyboard 
    cursor.initialize();
    cursor.onUpdate = function(position) {
        keyboard.setFocusPosition(position.x, position.y);
    };
};

function keyPressEvent(event) {
    if (event.key === SPACEBAR_CHARCODE) {
        keyboard.pressFocussedKey();
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
}

function reportButtonValue(button, newValue, oldValue) {
    if (button == Joysticks.BUTTON_FACE_BOTTOM) {
        if (newValue) {
            keyboard.pressFocussedKey();
        } else {
            keyboard.releaseKeys();
        }
    } else if (button == Joysticks.BUTTON_FACE_RIGHT && newValue) {
        deleteChar();
    }
}

function addJoystick(gamepad) {
    gamepad.buttonStateChanged.connect(reportButtonValue);
}

var allJoysticks = Joysticks.getAllJoysticks();
for (var i = 0; i < allJoysticks.length; i++) {
    addJoystick(allJoysticks[i]);
}

Joysticks.joystickAdded.connect(addJoystick);
Controller.captureKeyEvents({key: SPACEBAR_CHARCODE});
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.scriptEnding.connect(scriptEnding);
