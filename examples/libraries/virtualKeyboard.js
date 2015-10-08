//
//  virtualKeyboard.js
//  examples
//
//  Created by Thijs Wenker on 11/18/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Control a virtual keyboard using your favorite HMD.
//  Usage: Enable VR-mode and go to First person mode,
//  look at the key that you would like to press, and press the spacebar on your "REAL" keyboard.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// experimental 3dmode
THREE_D_MODE = false;

KBD_UPPERCASE_DEFAULT = 0;
KBD_LOWERCASE_DEFAULT = 1;
KBD_UPPERCASE_HOVER   = 2;
KBD_LOWERCASE_HOVER   = 3;
KBD_BACKGROUND        = 4;

KEYBOARD_URL = HIFI_PUBLIC_BUCKET + "images/keyboard.svg";
CURSOR_URL = HIFI_PUBLIC_BUCKET + "images/cursor.svg";

RETURN_CHARCODE = 0x01000004;
ENTER_CHARCODE = 0x01000005;
SPACEBAR_CHARCODE = 32;

KEYBOARD_WIDTH  = 1174.7;
KEYBOARD_HEIGHT = 434.1;

CURSOR_WIDTH = 33.9;
CURSOR_HEIGHT = 33.9;

KEYBOARD_SCALE_MULTIPLIER = 0.50;

// VIEW_ANGLE can be adjusted to your likings, the smaller the faster movement.
// Try setting it to 60 if it goes too fast for you.
const VIEW_ANGLE = 40.0;
const VIEW_ANGLE_BY_TWO = VIEW_ANGLE / 2;

const BOUND_X = 0;
const BOUND_Y = 1;
const BOUND_W = 2;
const BOUND_H = 3;

KEY_STATE_LOWER = 0;
KEY_STATE_UPPER = 1;

var windowDimensions = Controller.getViewportDimensions();

KeyboardKey = (function(keyboard, keyProperties) {
    var tthis = this;
    this._focus = false;
    this._beingPressed = false;
    this.event = keyProperties.event != undefined ?
        keyProperties.event : 'keypress';
    this.bounds = keyProperties.bounds;
    this.states = keyProperties.states;
    this.keyboard = keyboard;
    this.keyState = keyProperties.keyState != undefined ? keyProperties.keyState : KBD_LOWERCASE_DEFAULT;
    // one overlay per bound vector [this.bounds]
    this.overlays = [];
    this.getKeyEvent = function() {
        if (tthis.event == 'keypress') {
            var state = tthis.states[(tthis.keyboard.shift ? 1 : 2) % tthis.states.length];
            return {key: state.charCode, char: state.char, event: tthis.event, focus: tthis._focus};
        }
        return {event: tthis.event, focus: tthis._focus};
    };
    this.containsCoord = function(x, y) {
        for (var i = 0; i < this.bounds.length; i++) {
             if (x >= this.bounds[i][BOUND_X] &&
                 x <= (this.bounds[i][BOUND_X] + this.bounds[i][BOUND_W]) &&
                 y >= this.bounds[i][BOUND_Y] &&
                 y <= (this.bounds[i][BOUND_Y] + this.bounds[i][BOUND_H]))
             {
                 return true;
             }
        }
        return false;
    };
    this.updateState = function() {
        tthis.setState(eval('KBD_' + (tthis.keyboard.shift ? 'UPPERCASE' : 'LOWERCASE') + '_' + (tthis._focus ? 'HOVER' : 'DEFAULT')));
    };
    this.updateColor = function() {
        var colorIntensity = this._beingPressed ? 128 : 255;
        for (var i = 0; i < tthis.bounds.length; i++) {
             Overlays.editOverlay(tthis.overlays[i], 
                 {color: {red: colorIntensity, green: colorIntensity, blue: colorIntensity}}
             );
        }
    };
    this.press = function() {
        tthis._beingPressed = true;
        tthis.updateColor();
    };
    this.release = function() {
        tthis._beingPressed = false;
        tthis.updateColor();
    };
    this.blur = function() {
        tthis._focus = false;
        tthis.updateState();
    };
    this.focus = function() {
        tthis._focus = true;
        tthis.updateState();
    };
    this.setState = function(state) {
        tthis.keyState = state;
        for (var i = 0; i < tthis.bounds.length; i++) {
             Overlays.editOverlay(tthis.overlays[i], {
                 subImage: {width: tthis.bounds[i][BOUND_W], height: tthis.bounds[i][BOUND_H], x: tthis.bounds[i][BOUND_X], y: (KEYBOARD_HEIGHT * tthis.keyState) + tthis.bounds[i][BOUND_Y]}
             });
        }
    };
    this.updateVisibility = function() {
        for (var i = 0; i < tthis.bounds.length; i++) {
             Overlays.editOverlay(tthis.overlays[i], {visible: tthis.keyboard.visible});
        }
    };
    this.rescale = function() {
        for (var i = 0; i < tthis.bounds.length; i++) {
             Overlays.editOverlay(tthis.overlays[i], {
                 x: tthis.keyboard.getX() + tthis.bounds[i][BOUND_X] * keyboard.scale,
                 y: tthis.keyboard.getY() + tthis.bounds[i][BOUND_Y] * keyboard.scale,
                 width: this.bounds[i][BOUND_W] * keyboard.scale,
                 height: this.bounds[i][BOUND_H] * keyboard.scale
             });
        }
    };
    this.remove = function() {
        for (var i = 0; i < this.overlays.length; i++) {
             Overlays.deleteOverlay(this.overlays[i]);
        }
    };
    this.isLoaded = function() {
        for (var i = 0; i < this.overlays.length; i++) {
             if (!Overlays.isLoaded(this.overlays[i])) {
                 return false;
             }
        }
        return true;
    };
    for (var i = 0; i < this.bounds.length; i++) {
        if (THREE_D_MODE) {
            this.overlays.push(Overlays.addOverlay("image3d", {
                scale: 1,
                rotation: MyAvatar.rotation,
                isFacingAvatar: false,
                url: KEYBOARD_URL,
                alpha: 1,
                position: {
                    x: MyAvatar.position.x,// + this.bounds[i][BOUND_X] * 0.01,// /*+ this.keyboard.getX()*/ + this.bounds[i][BOUND_X] * keyboard.scale,
                    y: MyAvatar.position.y,// - this.bounds[i][BOUND_Y] * 0.01,// /*+ this.keyboard.getY()*/ + this.bounds[i][BOUND_Y] * keyboard.scale,
                    z: MyAvatar.position.z
                },
                width: this.bounds[i][BOUND_W] * keyboard.scale,
                height: this.bounds[i][BOUND_H] * keyboard.scale,
                subImage: {width: this.bounds[i][BOUND_W], height: this.bounds[i][BOUND_H], x: this.bounds[i][BOUND_X], y: (KEYBOARD_HEIGHT * this.keyState) + this.bounds[i][BOUND_Y]},
                alpha: 1,
                visible: tthis.keyboard.visible
            }));
        } else {
            this.overlays.push(Overlays.addOverlay("image", {
                imageURL: KEYBOARD_URL,
                x: this.keyboard.getX() + this.bounds[i][BOUND_X] * keyboard.scale,
                y: this.keyboard.getY() + this.bounds[i][BOUND_Y] * keyboard.scale,
                width: this.bounds[i][BOUND_W] * keyboard.scale,
                height: this.bounds[i][BOUND_H] * keyboard.scale,
                subImage: {width: this.bounds[i][BOUND_W], height: this.bounds[i][BOUND_H], x: this.bounds[i][BOUND_X], y: (KEYBOARD_HEIGHT * this.keyState) + this.bounds[i][BOUND_Y]},
                alpha: 1,
                visible: tthis.keyboard.visible
            }));
        }
    }
});

Keyboard = (function(params) {
    if (params === undefined) {
        params = {};
    }
    var tthis = this;
    this.focussed_key = -1;
    this.scale = (windowDimensions.x / KEYBOARD_WIDTH) * KEYBOARD_SCALE_MULTIPLIER;
    this.shift = false;
    this.visible = params.visible != undefined ? params.visible : true;
    this.width = function() {
        return KEYBOARD_WIDTH * tthis.scale;
    };
    this.height = function() {
        return KEYBOARD_HEIGHT * tthis.scale;
    };
    this.getX = function() {
        return (windowDimensions.x / 2) - (this.width() / 2);
    };
    this.getY = function() {
        return windowDimensions.y - this.height();
    };
    if (THREE_D_MODE) {
        this.background = Overlays.addOverlay("image3d", {
            scale: 1,
            position: MyAvatar.position,
            rotation: MyAvatar.rotation,
            width: this.width(),
            height: this.height(),
            subImage: {width: KEYBOARD_WIDTH, height: KEYBOARD_HEIGHT, y: KEYBOARD_HEIGHT * KBD_BACKGROUND},
            isFacingAvatar: false,
            url: KEYBOARD_URL,
            alpha: 1,
            visible: this.visible
        });
    } else {
        this.background = Overlays.addOverlay("image", {
            x: this.getX(),
            y: this.getY(),
            width: this.width(),
            height: this.height(),
            subImage: {width: KEYBOARD_WIDTH, height: KEYBOARD_HEIGHT, y: KEYBOARD_HEIGHT * KBD_BACKGROUND},
            imageURL: KEYBOARD_URL,
            alpha: 1,
            visible: this.visible
        });
    }
    this.rescale = function() {
        this.scale = (windowDimensions.x / KEYBOARD_WIDTH) * KEYBOARD_SCALE_MULTIPLIER;
        Overlays.editOverlay(tthis.background, {
            x: this.getX(),
            y: this.getY(),
            width: this.width(),
            height: this.height()
        });
        for (var i = 0; i < tthis.keys.length; i++) {
            tthis.keys[i].rescale();
        }
    };

    this.setFocusPosition = function(x, y) {
        // set to local unscaled position
        var localx = (x - tthis.getX()) / tthis.scale;
        var localy = (y - tthis.getY()) / tthis.scale;
        var new_focus_key = -1;
        if (localx >= 0 && localy >= 0 && localx <= KEYBOARD_WIDTH && localy <= KEYBOARD_HEIGHT) {
            for (var i = 0; i < tthis.keys.length; i++) {
                if (tthis.keys[i].containsCoord(localx, localy)) {
                    new_focus_key = i;
                    break;
                }
            }
        }
        if (new_focus_key != tthis.focussed_key) {
            if (tthis.focussed_key != -1) {
                tthis.keys[tthis.focussed_key].blur();
            }
            tthis.focussed_key = new_focus_key;
            if (tthis.focussed_key != -1) {
                tthis.keys[tthis.focussed_key].focus();
            }
        }
        return tthis;
    };

    this.pressFocussedKey = function() {
        if (!tthis.visible) {
            return tthis;
        }
        if (tthis.focussed_key != -1) {
            if (tthis.keys[tthis.focussed_key].event == 'shift') {
                tthis.toggleShift();
            } else {
                tthis.keys[tthis.focussed_key].press();
            }
            if (this.onKeyPress != null) {
                this.onKeyPress(tthis.keys[tthis.focussed_key].getKeyEvent());
            }
        }

        return tthis;
    };
    
    this.releaseKeys = function() {
        for (var i = 0; i < tthis.keys.length; i++) {
             if (tthis.keys[i]._beingPressed) {
                 if (tthis.keys[i].event != 'shift') {
                     tthis.keys[i].release();
                 }
                 if (this.onKeyRelease != null) {
                     this.onKeyRelease(tthis.keys[i].getKeyEvent());
                 }
             }
        }
    };

    this.toggleShift = function() {
        tthis.shift = !tthis.shift;
        for (var i = 0; i < tthis.keys.length; i++) {
             tthis.keys[i].updateState();
             if (tthis.keys[i].event == 'shift') {
                 if (tthis.shift) {
                     tthis.keys[i].press();
                     continue;
                 }
                 tthis.keys[i].release();
             }
        }
    };

    this.getFocussedKey = function() {
        if (tthis.focussed_key == -1) {
            return null;
        }
        return tthis.keys[tthis.focussed_key];
    };

    this.remove = function() {
        Overlays.deleteOverlay(this.background);
        for (var i = 0; i < this.keys.length; i++) {
            this.keys[i].remove();
        }
        // resets the cursor and magnifier
        this.updateVisibility(false);
    };

    this.show = function() {
        tthis.updateVisibility(true);
    };

    this.hide = function() {
        tthis.updateVisibility(false);
    };

    this.toggle = function() {
        tthis.updateVisibility(!tthis.visible);
    };
    
    this.updateVisibility = function(visible) {
        tthis.visible = visible;
        if (HMD.magnifier == visible) {
            HMD.toggleMagnifier();
        }
        Overlays.editOverlay(tthis.background, { visible: tthis.visible });
        for (var i = 0; i < this.keys.length; i++) {
            this.keys[i].updateVisibility();
        }
    };

    this.onKeyPress = null;
    this.onKeyRelease = null;
    this.onSubmit = null;
    this.onFullyLoaded = null;

    this.keys = [];
    //
    // keyProperties contains the key data
    //
    // coords [[x,y,w,h],[x,y,w,h]]
    // states array of 1 or 2 objects [lowercase, uppercase] each object contains a charCode and a char
    var keyProperties = [
        {bounds: [[12, 12, 65, 52]], states: [{charCode: 126, char: '~'}]},
        {bounds: [[84, 12, 65, 52]], states: [{charCode: 33, char: '!'}]},
        {bounds: [[156, 12, 65, 52]], states: [{charCode: 64, char: '@'}]},
        {bounds: [[228, 12, 65, 52]], states: [{charCode: 35, char: '#'}]},
        {bounds: [[300, 12, 65, 52]], states: [{charCode: 36, char: '$'}]},
        {bounds: [[372, 12, 65, 52]], states: [{charCode: 37, char: '%'}]},
        {bounds: [[445, 12, 65, 52]], states: [{charCode: 94, char: '^'}]},
        {bounds: [[517, 12, 65, 52]], states: [{charCode: 38, char: '&'}]},
        {bounds: [[589, 12, 65, 52]], states: [{charCode: 42, char: '*'}]},
        {bounds: [[662, 12, 65, 52]], states: [{charCode: 40, char: '('}]},
        {bounds: [[734, 12, 65, 52]], states: [{charCode: 41, char: ')'}]},
        {bounds: [[806, 12, 65, 52]], states: [{charCode: 95, char: '_'}]},
        {bounds: [[881, 12, 65, 52]], states: [{charCode: 123, char: '{'}]},
        {bounds: [[953, 12, 65, 52]], states: [{charCode: 125, char: '}'}]},
        {bounds: [[1025, 12, 65, 52]], states: [{charCode: 60, char: '<'}]},
        {bounds: [[1097, 12, 65, 52]], states: [{charCode: 62, char: '>'}]},

        {bounds: [[12, 71, 65, 63]], states: [{charCode: 96, char: '`'}]},
        {bounds: [[84, 71, 65, 63]], states: [{charCode: 49, char: '1'}]},
        {bounds: [[156, 71, 65, 63]], states: [{charCode: 50, char: '2'}]},
        {bounds: [[228, 71, 65, 63]], states: [{charCode: 51, char: '3'}]},
        {bounds: [[300, 71, 65, 63]], states: [{charCode: 52, char: '4'}]},
        {bounds: [[372, 71, 65, 63]], states: [{charCode: 53, char: '5'}]},
        {bounds: [[445, 71, 65, 63]], states: [{charCode: 54, char: '6'}]},
        {bounds: [[517, 71, 65, 63]], states: [{charCode: 55, char: '7'}]},
        {bounds: [[589, 71, 65, 63]], states: [{charCode: 56, char: '8'}]},
        {bounds: [[661, 71, 65, 63]], states: [{charCode: 57, char: '9'}]},
        {bounds: [[733, 71, 65, 63]], states: [{charCode: 48, char: '0'}]},
        {bounds: [[806, 71, 65, 63]], states: [{charCode: 45, char: '-'}]},
        {bounds: [[880, 71, 65, 63]], states: [{charCode: 61, char: '='}]},
        {bounds: [[953, 71, 65, 63]], states: [{charCode: 43, char: '+'}]},
        {bounds: [[1024, 71, 139, 63]], event: 'delete'},

        // enter key has 2 bounds and one state
        {bounds: [[11, 143, 98, 71], [11, 213, 121, 62]], event: 'enter'},

        {bounds: [[118, 142, 64, 63]], states: [{charCode: 113, char: 'q'}, {charCode: 81, char: 'Q'}]},
        {bounds: [[190, 142, 64, 63]], states: [{charCode: 119, char: 'w'}, {charCode: 87, char: 'W'}]},
        {bounds: [[262, 142, 64, 63]], states: [{charCode: 101, char: 'e'}, {charCode: 69, char: 'E'}]},
        {bounds: [[334, 142, 64, 63]], states: [{charCode: 114, char: 'r'}, {charCode: 82, char: 'R'}]},
        {bounds: [[407, 142, 64, 63]], states: [{charCode: 116, char: 't'}, {charCode: 84, char: 'T'}]},
        {bounds: [[479, 142, 64, 63]], states: [{charCode: 121, char: 'y'}, {charCode: 89, char: 'Y'}]},
        {bounds: [[551, 142, 65, 63]], states: [{charCode: 117, char: 'u'}, {charCode: 85, char: 'U'}]},
        {bounds: [[623, 142, 65, 63]], states: [{charCode: 105, char: 'i'}, {charCode: 73, char: 'I'}]},
        {bounds: [[695, 142, 65, 63]], states: [{charCode: 111, char: 'o'}, {charCode: 79, char: 'O'}]},
        {bounds: [[768, 142, 64, 63]], states: [{charCode: 112, char: 'p'}, {charCode: 80, char: 'P'}]},
        {bounds: [[840, 142, 64, 63]], states: [{charCode: 91, char: '['}]},
        {bounds: [[912, 142, 65, 63]], states: [{charCode: 93, char: ']'}]},
        {bounds: [[984, 142, 65, 63]], states: [{charCode: 92, char: '\\'}]},
        {bounds: [[1055, 142, 65, 63]], states: [{charCode: 124, char: '|'}]},
        
        {bounds: [[1126, 143, 35, 72], [1008, 214, 153, 62]], event: 'enter'},

        {bounds: [[140, 213, 65, 63]], states: [{charCode: 97, char: 'a'}, {charCode: 65, char: 'A'}]},
        {bounds: [[211, 213, 64, 63]], states: [{charCode: 115, char: 's'}, {charCode: 83, char: 'S'}]},
        {bounds: [[283, 213, 65, 63]], states: [{charCode: 100, char: 'd'}, {charCode: 68, char: 'D'}]},
        {bounds: [[355, 213, 65, 63]], states: [{charCode: 102, char: 'f'}, {charCode: 70, char: 'F'}]},
        {bounds: [[428, 213, 64, 63]], states: [{charCode: 103, char: 'g'}, {charCode: 71, char: 'G'}]},
        {bounds: [[500, 213, 64, 63]], states: [{charCode: 104, char: 'h'}, {charCode: 72, char: 'H'}]},
        {bounds: [[572, 213, 65, 63]], states: [{charCode: 106, char: 'j'}, {charCode: 74, char: 'J'}]},
        {bounds: [[644, 213, 65, 63]], states: [{charCode: 107, char: 'k'}, {charCode: 75, char: 'K'}]},
        {bounds: [[716, 213, 65, 63]], states: [{charCode: 108, char: 'l'}, {charCode: 76, char: 'L'}]},
        {bounds: [[789, 213, 64, 63]], states: [{charCode: 59, char: ';'}]},
        {bounds: [[861, 213, 64, 63]], states: [{charCode: 39, char: '\''}]},
        {bounds: [[934, 213, 65, 63]], states: [{charCode: 58, char: ':'}]},

        {bounds: [[12, 283, 157, 63]], event: 'shift'},

        {bounds: [[176, 283, 65, 63]], states: [{charCode: 122, char: 'z'}, {charCode: 90, char: 'Z'}]},
        {bounds: [[249, 283, 64, 63]], states: [{charCode: 120, char: 'x'}, {charCode: 88, char: 'X'}]},
        {bounds: [[321, 283, 64, 63]], states: [{charCode: 99, char: 'c'}, {charCode: 67, char: 'C'}]},
        {bounds: [[393, 283, 64, 63]], states: [{charCode: 118, char: 'v'}, {charCode: 86, char: 'V'}]},
        {bounds: [[465, 283, 65, 63]], states: [{charCode: 98, char: 'b'}, {charCode: 66, char: 'B'}]},
        {bounds: [[537, 283, 65, 63]], states: [{charCode: 110, char: 'n'}, {charCode: 78, char: 'N'}]},
        {bounds: [[610, 283, 64, 63]], states: [{charCode: 109, char: 'm'}, {charCode: 77, char: 'M'}]},
        {bounds: [[682, 283, 64, 63]], states: [{charCode: 44, char: ','}]},
        {bounds: [[754, 283, 65, 63]], states: [{charCode: 46, char: '.'}]},
        {bounds: [[826, 283, 65, 63]], states: [{charCode: 47, char: '/'}]},
        {bounds: [[899, 283, 64, 63]], states: [{charCode: 63, char: '?'}]},

        {bounds: [[972, 283, 190, 63]], event: 'shift'},

        {bounds: [[249, 355, 573, 67]], states: [{charCode: 32, char: ' '}]},

        {bounds: [[899, 355, 263, 67]], event: 'submit'}
    ];
    for (var i = 0; i < keyProperties.length; i++) {
        this.keys.push(new KeyboardKey(this, keyProperties[i]));
    }
    this.keyboardTextureLoaded = function() {
        if (Overlays.isLoaded(tthis.background)) {
            Script.clearInterval(tthis.keyboardTextureLoaded_timer);
            if (keyboard.onFullyLoaded != null) {
                tthis.onFullyLoaded();
            }
        }
    };
    this.keyboardTextureLoaded_timer = Script.setInterval(this.keyboardTextureLoaded, 250);
});

Cursor = (function(params) {
    if (params === undefined) {
        params = {};
    }
    var tthis = this;
    this.initialize = function() {
        this.visible = params.visible != undefined ? params.visible : true;
        this.x = windowDimensions.x / 2;
        this.y = windowDimensions.y / 2;
        this.overlay = Overlays.addOverlay("image", {
            x: this.x,
            y: this.y,
            width: CURSOR_WIDTH,
            height: CURSOR_HEIGHT,
            imageURL: CURSOR_URL,
            alpha: 1,
            visible: this.visible
        });
        this.remove = function() {
            Overlays.deleteOverlay(this.overlay);
        };
        this.getPosition = function() {
            return {x: tthis.getX(), y: tthis.getY()};
        };
        this.getX = function() {
            return tthis.x;
        };
        this.getY = function() {
            return tthis.y;
        };
        this.show = function() {
            tthis.updateVisibility(true);
        };
        this.hide = function() {
            tthis.updateVisibility(false);
        };
        this.toggle = function() {
            tthis.updateVisibility(!tthis.visible);
        };
        this.updateVisibility = function(visible) {
            tthis.visible = visible;
            Overlays.editOverlay(this.overlay, { visible: tthis.visible });
        };
        this.onUpdate = null;
        this.update = function() {
            var newWindowDimensions = Controller.getViewportDimensions();
            if (newWindowDimensions.x != windowDimensions.x || newWindowDimensions.y != windowDimensions.y) {
                windowDimensions = newWindowDimensions;
                keyboard.rescale();
                Overlays.editOverlay(text, {
                    y: windowDimensions.y - keyboard.height() - 260,
                    width: windowDimensions.x
                });
            }
            var editobject = {};
            var hudLookatPosition = HMD.getHUDLookAtPosition2D();
            if (hudLookatPosition === null) {
                return;
            }
            if (tthis.x !== hudLookatPosition.x) {
                 tthis.x = hudLookatPosition.x;
                 editobject.x = tthis.x - (CURSOR_WIDTH / 2);
            }
            if (tthis.y !== hudLookatPosition.y) {
                 tthis.y = hudLookatPosition.y;
                 editobject.y = tthis.y - (CURSOR_HEIGHT / 2);
            }
            if (Object.keys(editobject).length > 0) {
                Overlays.editOverlay(tthis.overlay, editobject);
                if (tthis.onUpdate != null) {
                    tthis.onUpdate(tthis.getPosition());
                } 
            }
        };
        Script.update.connect(this.update);
    };
});
