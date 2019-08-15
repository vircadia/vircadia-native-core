/*

    Simplified Emote
    simplifiedEmote.js
    Created by Milad Nazeri on 2019-08-06
    Copyright 2019 High Fidelity, Inc.

    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


*/


// *************************************
// START EMOTE
// *************************************
// #region EMOTE


// *************************************
// START EMOTE_UTILITY
// *************************************
// #region EMOTE_UTILITY


// REMOVE THIS WHEN ENGINE CAN HANDLE BLENDING
function maybeEndOtherReactions(currentEmote) {
    print("CURRENT EMOTE: ", currentEmote);
    if (raiseHandPlaying && "raiseHand" != currentEmote) {
        print("ENDING RAISE HAND");
        MyAvatar.endReaction("raiseHand");
        raiseHandPlaying = false;
    }
    if (pointPlaying && "point" != currentEmote) {
        print("ENDING POINT");
        MyAvatar.endReaction("point");
        pointPlaying = false;
    }
    if (applaudPlaying && "applaud" != currentEmote) {
        print("ENDING APPLAUD");
        MyAvatar.endReaction("applaud");
        maybeClearClapSoundInterval();
        applaudPlaying = false;
    }
}



function updateEmoteAppBarPosition() {
    if (!emoteAppBarWindow) {
        return;
    }

    emoteAppBarWindow.position = {
        x: Window.x + EMOTE_APP_BAR_LEFT_MARGIN,
        y: Window.y + Window.innerHeight - EMOTE_APP_BAR_BOTTOM_MARGIN
    };
}


function randomFloat(min, max) {
    return Math.random() * (max - min) + min;
}


// Returns a linearly scaled value based on `factor` and the other inputs
function linearScale(factor, minInput, maxInput, minOutput, maxOutput) {
    return minOutput + (maxOutput - minOutput) *
    (factor - minInput) / (maxInput - minInput);
}


// #endregion
// *************************************
// END EMOTE_UTILITY
// *************************************
// *************************************
// START EMOTE_HANDLERS
// *************************************
// #region EMOTE_HANDLERS


// Calculates the audio injector volume based on 
// the current global appreciation intensity and some min/max values.
var MAX_CLAP_INTENSITY = 1.0; // Unitless, determined empirically
var MIN_VOLUME_CLAP = 0.05;
var MAX_VOLUME_CLAP = 1.0;
function calculateInjectorVolume(clapIntensity) {
    var minInputVolume = 0;
    var maxInputVolume = MAX_CLAP_INTENSITY;
    var minOutputVolume = MIN_VOLUME_CLAP;
    var maxOutputVolume = MAX_VOLUME_CLAP;

    var vol = linearScale(clapIntensity, minInputVolume,
        maxInputVolume, minOutputVolume, maxOutputVolume);
    return vol;
}


var soundInjector = false;
var MINIMUM_PITCH = 0.85;
var MAXIMUM_PITCH = 1.15;
function playSound(sound, position) {
    if (soundInjector && soundInjector.isPlaying()) {
        return;
    }

    if (soundInjector) {
        soundInjector.stop();
        soundInjector = false;
    }

    soundInjector = Audio.playSound(sound, {
        position: position || MyAvatar.position,
        volume: calculateInjectorVolume(0.5),
        pitch: randomFloat(MINIMUM_PITCH, MAXIMUM_PITCH)
    });
}


var NUM_CLAP_SOUNDS = 16;
var clapSounds = [];
function getSounds() {
    for (var i = 1; i < NUM_CLAP_SOUNDS + 1; i++) {
        clapSounds.push(SoundCache.getSound(Script.resolvePath(
            "resources/sounds/claps/" + ("0" + i).slice(-2) + ".wav")));
    }
}


var clapSoundInterval = false;
var CLAP_SOUND_INTERVAL_MS = 260; // Must match the clap animation interval
function startClappingSounds() {
    maybeClearClapSoundInterval();

    clapSoundInterval = Script.setInterval(function() {
        playSound(clapSounds[Math.floor(Math.random() * clapSounds.length)]);
    }, CLAP_SOUND_INTERVAL_MS);
}


function maybeClearClapSoundInterval() {
    if (clapSoundInterval) {
        Script.clearInterval(clapSoundInterval);
        clapSoundInterval = false;
    }
}


function toggleApplaud() {
    if (applaudPlaying) {
        MyAvatar.endReaction("applaud");
        maybeClearClapSoundInterval();
        applaudPlaying = false;
        return;
    }
    maybeEndOtherReactions("applaud");
    MyAvatar.beginReaction("applaud");
    startClappingSounds();
    applaudPlaying = true;
    // REMOVE THESE WHEN ENGINE CAN HANDLE BLENDING
    pointPlaying = false;
    raiseHandPlaying = false;
}


/*
    MILAD NOTE:
    Right now you can do more than one reaction at a time so it has to be handled here.  
    I might see about adding a small flag to the engine so that if a reaction is currently playing it will end it.
    This will cut down on a lot of boiler code below checking to see if another reaction is playing.  Also because it doesn't
    make sense for more than one reaction to be playing.
*/
var EMOTE_APP_BAR_MESSAGE_SOURCE = "EmoteAppBar.qml";
var raiseHandPlaying = false;
var applaudPlaying = false;
var pointPlaying = false;
function onMessageFromEmoteAppBar(message) {
    console.log("MESSAGE From emote app bar: ", JSON.stringify(message));
    if (message.source !== EMOTE_APP_BAR_MESSAGE_SOURCE) {
        return;
    }
    switch (message.method) {
        case "positive":
            maybeEndOtherReactions("positive");
            MyAvatar.triggerReaction("positive");
            break;
        case "negative":
            maybeEndOtherReactions("negative");
            MyAvatar.triggerReaction("negative");
            break;
        case "raiseHand":
            if (raiseHandPlaying) {
                MyAvatar.endReaction("raiseHand");
                raiseHandPlaying = false;
                return;
            }
            maybeEndOtherReactions("raiseHand");
            MyAvatar.beginReaction("raiseHand");
            raiseHandPlaying = true;
            // REMOVE THESE WHEN ENGINE CAN HANDLE BLENDING
            pointPlaying = false;
            applaudPlaying = false;
            break;
        case "applaud":
            toggleApplaud();
            break;
        case "point":
            if (pointPlaying) {
                MyAvatar.endReaction("point");
                pointPlaying = false;
                return;
            }
            maybeEndOtherReactions("point");
            MyAvatar.beginReaction("point");
            pointPlaying = true;
            // REMOVE THESE WHEN ENGINE CAN HANDLE BLENDING
            raiseHandPlaying = false;
            applaudPlaying = false;
            break;
        case "toggleEmojiApp": 
            toggleEmojiApp();
            break;
        default:
            console.log("Unrecognized message from " + EMOTE_APP_BAR_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
    emoteAppBarWindow.sendToQml({
        "source": "simplifiedEmote.js",
        "method": "updateEmoteIndicator",
        "data": {
            "icon": message.method
        }
    });
}


function onGeometryChanged(rect) {
    updateEmoteAppBarPosition();
}


function onEmoteAppBarClosed() {
    if (emoteAppBarWindow) {
        emoteAppBarWindow.fromQml.disconnect(onMessageFromEmoteAppBar);
    }
    emoteAppBarWindow = false;
}


var POSITIVE_KEY = "z";
var NEGATIVE_KEY = "c";
var RAISE_HAND_KEY = "v";
var APPLAUD_KEY = "b";
var POINT_KEY = "n";
var EMOTE_WINDOW = "f";
function keyPressHandler(event) {
    if (!event.isAutoRepeat && ! event.isMeta && ! event.isControl && ! event.isAlt) {
        if (event.text === POSITIVE_KEY) {
            MyAvatar.triggerReaction("positive");
        } else if (event.text === NEGATIVE_KEY) {
            MyAvatar.triggerReaction("negative");
        } else if (event.text === RAISE_HAND_KEY) {
            MyAvatar.beginReaction("raiseHand");
        } else if (event.text === APPLAUD_KEY) {
            toggleApplaud();
        } else if (event.text === POINT_KEY) {
            MyAvatar.beginReaction("point");
        } else if (event.text === EMOTE_WINDOW) {
            toggleEmojiApp();
        }
    }
}


function keyReleaseHandler(event) {
    if (!event.isAutoRepeat) {
        if (event.text === RAISE_HAND_KEY) {
            MyAvatar.endReaction("raiseHand");
        } else if (event.text === APPLAUD_KEY) {
            if (applaudPlaying) {
                toggleApplaud();
            }
        } else if (event.text === POINT_KEY) {
            MyAvatar.endReaction("point");
        }
    }
}


// #endregion
// *************************************
// END EMOTE_HANDLERS
// *************************************

// *************************************
// START EMOTE_MAIN
// *************************************
// #region EMOTE_MAIN

var EMOTE_APP_BAR_QML_PATH = Script.resolvePath("./ui/qml/SimplifiedEmoteIndicator.qml");
var EMOTE_APP_BAR_WINDOW_TITLE = "Emote Reaction Bar";
var EMOTE_APP_BAR_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var EMOTE_APP_BAR_WIDTH_PX = 48;
var EMOTE_APP_BAR_HEIGHT_PX = 48;
var EMOTE_APP_BAR_LEFT_MARGIN = 48;
var EMOTE_APP_BAR_BOTTOM_MARGIN = 48;
var EMOTE_APP_BAR_WINDOW_FLAGS = 0x00000001 | // Qt::Window
    0x00000800 | // Qt::FramelessWindowHint
    0x40000000 | // Qt::NoDropShadowWindowHint
    0x00200000; // Qt::WindowDoesNotAcceptFocus
var emoteAppBarWindow = false;
var emoteIndicatorVisible = true;
function showEmoteAppBar() {
    emoteAppBarWindow = Desktop.createWindow(EMOTE_APP_BAR_QML_PATH, {
        title: EMOTE_APP_BAR_WINDOW_TITLE,
        presentationMode: EMOTE_APP_BAR_PRESENTATION_MODE,
        size: {
            x: EMOTE_APP_BAR_WIDTH_PX,
            y: EMOTE_APP_BAR_HEIGHT_PX
        },
        position: {
            x: Window.x + EMOTE_APP_BAR_LEFT_MARGIN,
            y: Window.y + Window.innerHeight - EMOTE_APP_BAR_BOTTOM_MARGIN
        },
        overrideFlags: EMOTE_APP_BAR_WINDOW_FLAGS
    });

    emoteAppBarWindow.fromQml.connect(onMessageFromEmoteAppBar);
    emoteAppBarWindow.closed.connect(onEmoteAppBarClosed);
}


var EmojiAPI = Script.require("./emojiApp/simplifiedEmoji.js?" + Date.now());
var emojiAPI = new EmojiAPI();
var geometryChangedSignalConnected = false;
var keyPressSignalsConnected = false;
function init() {
    Window.geometryChanged.connect(onGeometryChanged);
    geometryChangedSignalConnected = true;
    emojiAPI.startup();

    getSounds();
    showEmoteAppBar();
    
    Controller.keyPressEvent.connect(keyPressHandler);
    Controller.keyReleaseEvent.connect(keyReleaseHandler);
    keyPressSignalsConnected = true;

    Script.scriptEnding.connect(shutdown);
}


function shutdown() {
    if (emoteAppBarWindow) {
        emoteAppBarWindow.close();
    }

    if (emojiAppWindow) {
        emojiAppWindow.close();
    }

    emojiAPI.unload();
    maybeClearClapSoundInterval();

    if (geometryChangedSignalConnected) {
        Window.geometryChanged.disconnect(onGeometryChanged);
        geometryChangedSignalConnected = false;
    }

    if (keyPressSignalsConnected) {
        Controller.keyPressEvent.disconnect(keyPressHandler);
        Controller.keyReleaseEvent.disconnect(keyReleaseHandler);
        keyPressSignalsConnected = false;
    }
}


// #endregion
// *************************************
// END EMOTE_MAIN
// *************************************


// #endregion
// *************************************
// END EMOTE
// *************************************

// *************************************
// START EMOJI
// *************************************
// #region EMOJI

// *************************************
// START EMOJI_UTILITY
// *************************************
// #region EMOJI_UTILITY


function selectedEmoji(code) {
    emojiAPI.addEmoji(code);
}


// #endregion
// *************************************
// END EMOJI_UTILITY
// *************************************

// *************************************
// START EMOJI_HANDLERS
// *************************************
// #region EMOJI_HANDLERS


function onEmojiAppClosed() {
    if (emojiAppWindow) {
        emojiAppWindow.fromQml.disconnect(onMessageFromEmojiApp);
        emojiAppWindow.closed.disconnect(onEmojiAppClosed);
    }
    emojiAppWindow = false;
}


var EMOJI_APP_MESSAGE_SOURCE = "SimplifiedEmoji.qml";
function onMessageFromEmojiApp(message) {
    if (message.source !== EMOJI_APP_MESSAGE_SOURCE) {
        return;
    }

    switch (message.method) {
        case "selectedEmoji":
            selectedEmoji(message.code);
            break;
        default:
            console.log("Unrecognized message from " + EMOJI_APP_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}


// #endregion
// *************************************
// END EMOJI_HANDLERS
// *************************************

// *************************************
// START EMOJI_MAIN
// *************************************
// #region EMOJI_MAIN


var EMOJI_APP_QML_PATH = Script.resolvePath("./emojiApp/ui/qml/SimplifiedEmoji.qml");
var EMOJI_APP_WINDOW_TITLE = "Emoji";
var EMOJI_APP_PRESENTATION_MODE = Desktop.PresentationMode.NATIVE;
var EMOJI_APP_WIDTH_PX = 480;
var EMOJI_APP_HEIGHT_PX = 615;
var EMOJI_APP_WINDOW_FLAGS = 0x00000001 | // Qt::Window
    0x00001000 | // Qt::WindowTitleHint
    0x00002000 | // Qt::WindowSystemMenuHint
    0x08000000 | // Qt::WindowCloseButtonHint
    0x00008000 | // Qt::WindowMaximizeButtonHint
    0x00004000; // Qt::WindowMinimizeButtonHint
var emojiAppWindow = false;
var POPOUT_SAFE_MARGIN_X = 30;
var POPOUT_SAFE_MARGIN_Y = 30;
function toggleEmojiApp() {
    if (emojiAppWindow) {
        emojiAppWindow.close();
        // This really shouldn't be necessary.
        // This signal really should automatically be called by the signal handler set up below.
        // But fixing that requires an engine change, so this workaround will do.
        onEmojiAppClosed();
        return;
    }

    emojiAppWindow = Desktop.createWindow(EMOJI_APP_QML_PATH, {
        title: EMOJI_APP_WINDOW_TITLE,
        presentationMode: EMOJI_APP_PRESENTATION_MODE,
        size: {
            x: EMOJI_APP_WIDTH_PX,
            y: EMOJI_APP_HEIGHT_PX
        },
        position: {
            x: Math.max(Window.x + POPOUT_SAFE_MARGIN_X, Window.x + Window.innerWidth / 2 - EMOJI_APP_WIDTH_PX / 2),
            y: Math.max(Window.y + POPOUT_SAFE_MARGIN_Y, Window.y + Window.innerHeight / 2 - EMOJI_APP_HEIGHT_PX / 2)
        },
        overrideFlags: EMOJI_APP_WINDOW_FLAGS
    });

    emojiAppWindow.fromQml.connect(onMessageFromEmojiApp);
    emojiAppWindow.closed.connect(onEmojiAppClosed);
    // The actual emoji module needs this qml window object so it can send messages
    // to update the Selected emoji UI
    emojiAPI.registerAvimojiQMLWindow(emojiAppWindow);
}

// #endregion
// *************************************
// END EMOJI_MAIN
// *************************************

// #endregion
// *************************************
// END EMOJI
// *************************************

// *************************************
// START API
// *************************************
// #region API


function startup() {
    init();
}


function unload() {
    shutdown();
}

function handleEmoteIndicatorVisible(emoteIndicatorVisible) {
    if (!emoteAppBarWindow) {
        return;
    } 

    if (emoteIndicatorVisible) {
        showEmoteAppBar();
    } else {
        emoteAppBarWindow.close();
    }
}

var _this;
function EmoteBar() {
    _this = this;
}


EmoteBar.prototype = {
    startup: startup,
    handleEmoteIndicatorVisible: handleEmoteIndicatorVisible,
    unload: unload
};

module.exports = EmoteBar;


// #endregion
// *************************************
// END API
// *************************************

