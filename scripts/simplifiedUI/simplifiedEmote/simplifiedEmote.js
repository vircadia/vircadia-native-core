/*

    Simplified Emote
    simplifiedEmote.js
    Created by Milad Nazeri on 2019-08-06
    Copyright 2019 High Fidelity, Inc.

    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


*/

// *************************************
// START dependencies
// *************************************
// #region dependencies

// The information needed to properly use the sprite sheets and get the general information
// about the emojis
var emojiList = Script.require("./emojiApp/resources/modules/emojiList.js");
var customEmojiList = Script.require("./emojiApp/resources/modules/customEmojiList.js");
var imageURLBase = Script.resolvePath("./resources/images/emojis/1024px/");

// #endregion
// *************************************
// END dependencies
// *************************************

// *************************************
// START EMOTE
// *************************************
// #region EMOTE


// *************************************
// START EMOTE_UTILITY
// *************************************
// #region EMOTE_UTILITY



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

// CONSTS
// UTF-Codes are stored on the first index of one of the keys on a returned emoji object.
// Just makes the code a little easier to read 
var UTF_CODE = 0;

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
function playSound(sound, position, stopPrevious) {
    if (soundInjector && soundInjector.isPlaying() && !stopPrevious) {
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
        playSound(clapSounds[Math.floor(Math.random() * clapSounds.length)], MyAvatar.position, true);
    }, CLAP_SOUND_INTERVAL_MS);
}


function maybeClearClapSoundInterval() {
    if (clapSoundInterval) {
        Script.clearInterval(clapSoundInterval);
        clapSoundInterval = false;
    }
}

// URLs for this fn are relative to SimplifiedEmoteIndicator.qml
function toggleReaction(reaction) {
    var reactionEnding = reactionsBegun.indexOf(reaction) > -1;

    if (reactionEnding) {
        endReactionWrapper(reaction);
        updateEmoteIndicatorIcon("images/emote_Icon.svg");
    } else {
        beginReactionWrapper(reaction);
        updateEmoteIndicatorIcon("images/" + reaction + "_Icon.svg");
    }
}

var reactionsBegun = [];
var pointReticle = null;
var mouseMoveEventsConnected = false;
function beginReactionWrapper(reaction) {
    reactionsBegun.forEach(function(react) {
        endReactionWrapper(react);
    });

    if (MyAvatar.beginReaction(reaction)) {
        reactionsBegun.push(reaction);
    }

    // Insert reaction-specific logic here:
    switch (reaction) {
        case ("applaud"):
            startClappingSounds();
            break;
        case ("point"):
            deleteOldReticles();
            if (!mouseMoveEventsConnected) {
                Controller.mouseMoveEvent.connect(mouseMoveEvent);
                mouseMoveEventsConnected = true;
            }
    }
}

// Checks to see if there are any reticle entities already to delete
function deleteOldReticles() {
    MyAvatar.getAvatarEntitiesVariant()
        .forEach(function (avatarEntity) {
            if (avatarEntity && avatarEntity.properties.name.toLowerCase().indexOf("reticle") > -1) {
                Entities.deleteEntity(avatarEntity.id);
            }
        });
    pointReticle = null;
}


var MAX_INTERSECTION_DISTANCE_M = 50;
var reticleUpdateRateLimiterTimer = false;
var RETICLE_UPDATE_RATE_LIMITER_TIMER_MS = 75;
function mouseMoveEvent(event) {
    if (!reticleUpdateRateLimiterTimer) {
        reticleUpdateRateLimiterTimer = Script.setTimeout(function() {
            reticleUpdateRateLimiterTimer = false;
        }, RETICLE_UPDATE_RATE_LIMITER_TIMER_MS);
    } else {
        return;
    }


    var pickRay = Camera.computePickRay(event.x, event.y);
    var avatarIntersectionData = AvatarManager.findRayIntersection(pickRay);
    var entityIntersectionData = Entities.findRayIntersection(pickRay, true);
    var avatarIntersectionDistanceM = avatarIntersectionData.intersects && avatarIntersectionData.distance < MAX_INTERSECTION_DISTANCE_M ? avatarIntersectionData.distance : null;
    var entityIntersectionDistanceM = entityIntersectionData.intersects && entityIntersectionData.distance < MAX_INTERSECTION_DISTANCE_M ? entityIntersectionData.distance : null;
    var reticlePosition;

    if (avatarIntersectionDistanceM && entityIntersectionDistanceM) {
        if (avatarIntersectionDistanceM < entityIntersectionDistanceM) {
            reticlePosition = avatarIntersectionData.intersection;
        } else {
            reticlePosition = entityIntersectionData.intersection;
        }
    } else if (avatarIntersectionDistanceM) {
        reticlePosition = avatarIntersectionData.intersection;
    } else if (entityIntersectionDistanceM) {
        reticlePosition = entityIntersectionData.intersection;
    } else {
        deleteOldReticles();
        return;
    }

    if (pointReticle && reticlePosition) {
        Entities.editEntity(pointReticle, { position: reticlePosition });
    } else if (reticlePosition) {
        pointReticle = Entities.addEntity({
            type: "Sphere",
            name: "Point Reticle",
            position: reticlePosition,
            dimensions: { x: 0.1, y: 0.1, z: 0.1 },
            color: { red: 255, green: 0, blue: 0 },
            collisionless: true,
            ignorePickIntersection: true,
            grab: {
                grabbable: false
            }
        }, true);
    }
}


function triggerReactionWrapper(reaction) {
    reactionsBegun.forEach(function(react) {
        endReactionWrapper(react);
    });

    MyAvatar.triggerReaction(reaction);
}

function maybeClearReticleUpdateLimiterTimeout() {
    if (reticleUpdateRateLimiterTimer) {
        Script.clearTimeout(reticleUpdateRateLimiterTimer);
        reticleUpdateRateLimiterTimer = false;
    }
}


function endReactionWrapper(reaction) {
    var reactionsBegunIndex = reactionsBegun.indexOf(reaction);

    if (reactionsBegunIndex > -1) {
        if (MyAvatar.endReaction(reaction)) {
            reactionsBegun.splice(reactionsBegunIndex, 1);
        }
    }

    // Insert reaction-specific logic here:
    switch (reaction) {
        case ("applaud"):
            maybeClearClapSoundInterval();
            break;
        case ("point"):
            if (mouseMoveEventsConnected) {
                Controller.mouseMoveEvent.disconnect(mouseMoveEvent);
                mouseMoveEventsConnected = false;
            }
            maybeClearReticleUpdateLimiterTimeout();
            intersectedEntityOrAvatarID = null;
            deleteOldReticles();
            break;
    }
}


var EMOTE_APP_BAR_MESSAGE_SOURCE = "EmoteAppBar.qml";
function onMessageFromEmoteAppBar(message) {
    if (message.source !== EMOTE_APP_BAR_MESSAGE_SOURCE) {
        return;
    }
    switch (message.method) {
        case "positive":
            triggerReactionWrapper("positive");
            updateEmoteIndicatorIcon("images/" + message.method + "_Icon.svg");
            break;
        case "negative":
            triggerReactionWrapper("negative");
            updateEmoteIndicatorIcon("images/" + message.method + "_Icon.svg");
            break;
        case "raiseHand":
        case "applaud":
        case "point":
            toggleReaction(message.method);
            break;
        case "toggleEmojiApp": 
            toggleEmojiApp();
            break;
        default:
            console.log("Unrecognized message from " + EMOTE_APP_BAR_MESSAGE_SOURCE + ": " + JSON.stringify(message));
            break;
    }
}

function getEmojiURLFromCode(code) {
    var emojiObject = emojiList[emojiCodeMap[code]];
    var emojiFilename;
    // If `emojiObject` isn't defined here, that probably means we're looking for a custom emoji
    if (!emojiObject) {
        emojiFilename = customEmojiList[customEmojiCodeMap[code]].filename;
    } else {
        emojiFilename = emojiObject.code[UTF_CODE] + ".png";
    }
    return "../../emojiApp/resources/images/emojis/52px/" + emojiFilename;
}

function updateEmoteIndicatorIcon(iconURL) {
    emoteAppBarWindow.sendToQml({
        "source": "simplifiedEmote.js",
        "method": "updateEmoteIndicator",
        "data": {
            "iconURL": iconURL
        }
    });
}


function onGeometryChanged(rect) {
    updateEmoteAppBarPosition();
}


// These keys need to match what's in `SimplifiedEmoteIndicator.qml` in the `buttonsModel`
// for the tooltips to match the actual keys.
var POSITIVE_KEY = "z";
var NEGATIVE_KEY = "x";
var RAISE_HAND_KEY = "c";
var APPLAUD_KEY = "v";
var POINT_KEY = "b";
var EMOTE_WINDOW = "f";
function keyPressHandler(event) {
    if (HMD.active) {
        return;
    }

    if (!event.isAutoRepeat && ! event.isMeta && ! event.isControl && ! event.isAlt) {
        if (event.text === POSITIVE_KEY) {
            triggerReactionWrapper("positive");
        } else if (event.text === NEGATIVE_KEY) {
            triggerReactionWrapper("negative");
        } else if (event.text === RAISE_HAND_KEY) {
            toggleReaction("raiseHand");
        } else if (event.text === APPLAUD_KEY) {
            toggleReaction("applaud");
        } else if (event.text === POINT_KEY) {
            toggleReaction("point");
        } else if (event.text === EMOTE_WINDOW) {
            toggleEmojiApp();
        }
    }
}


function keyReleaseHandler(event) {
    if (!event.isAutoRepeat) {
        if (event.text === RAISE_HAND_KEY) {
            if (reactionsBegun.indexOf("raiseHand") > -1) {
                toggleReaction("raiseHand");
            }
        } else if (event.text === APPLAUD_KEY) {
            if (reactionsBegun.indexOf("applaud") > -1) {
                toggleReaction("applaud");
            }
        } else if (event.text === POINT_KEY) {
            if (reactionsBegun.indexOf("point") > -1) {
                toggleReaction("point");
            }
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
function showEmoteAppBar() {
    if (emoteAppBarWindow) {
        return;
    }

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
}


function handleEmoteIndicatorVisibleChanged(newValue) {
    if (newValue && !emoteAppBarWindow) {
        showEmoteAppBar();
    } else if (emoteAppBarWindow) {
        if (emoteAppBarWindow) {
            emoteAppBarWindow.fromQml.disconnect(onMessageFromEmoteAppBar);
        }
        emoteAppBarWindow.close();
        emoteAppBarWindow = false;
    }
}


function onSettingsValueChanged(settingName, newValue) {
    if (settingName === "simplifiedUI/emoteIndicatorVisible") {
        handleEmoteIndicatorVisibleChanged(newValue);
    }
}


function onDisplayModeChanged(isHMDMode) {
    reactionsBegun.forEach(function(react) {
        endReactionWrapper(react);
    });

    if (isHMDMode) {
        handleEmoteIndicatorVisibleChanged(false);
    } else if (Settings.getValue("simplifiedUI/emoteIndicatorVisible", true)) {
        handleEmoteIndicatorVisibleChanged(true);
    }
}


var EmojiAPI = Script.require("./emojiApp/simplifiedEmoji.js");
var emojiAPI = new EmojiAPI();
var keyPressSignalsConnected = false;
var emojiCodeMap;
function init() {
    deleteOldReticles();

    // make a map of just the utf codes to help with accesing
    emojiCodeMap = emojiList.reduce(function (codeMap, currentEmojiInList, index) {
        if (
            currentEmojiInList &&
            currentEmojiInList.code &&
            currentEmojiInList.code.length > 0 &&
            currentEmojiInList.code[UTF_CODE]) {

            codeMap[currentEmojiInList.code[UTF_CODE]] = index;
            return codeMap;
        }
    }, {});
    customEmojiCodeMap = customEmojiList.reduce(function (codeMap, currentEmojiInList, index) {
        if (
            currentEmojiInList &&
            currentEmojiInList.name &&
            currentEmojiInList.name.length > 0) {

            codeMap[currentEmojiInList.name] = index;
            return codeMap;
        }
    }, {});

    Window.geometryChanged.connect(onGeometryChanged);
    Settings.valueChanged.connect(onSettingsValueChanged);
    HMD.displayModeChanged.connect(onDisplayModeChanged);
    emojiAPI.startup();

    getSounds();
    handleEmoteIndicatorVisibleChanged(Settings.getValue("simplifiedUI/emoteIndicatorVisible", true));
    
    Controller.keyPressEvent.connect(keyPressHandler);
    Controller.keyReleaseEvent.connect(keyReleaseHandler);
    keyPressSignalsConnected = true;

    Script.scriptEnding.connect(shutdown);
}


function shutdown() {
    if (emoteAppBarWindow) {
        emoteAppBarWindow.fromQml.disconnect(onMessageFromEmoteAppBar);
        emoteAppBarWindow.close();
        emoteAppBarWindow = false;
    }

    if (emojiAppWindow) {
        emojiAppWindow.close();
    }

    reactionsBegun.forEach(function(react) {
        endReactionWrapper(react);
    });

    emojiAPI.unload();
    maybeClearClapSoundInterval();
    maybeClearReticleUpdateLimiterTimeout();

    Window.geometryChanged.disconnect(onGeometryChanged);
    Settings.valueChanged.disconnect(onSettingsValueChanged);
    HMD.displayModeChanged.disconnect(onDisplayModeChanged);

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

var EMOJI_52_BASE_URL = "../../resources/images/emojis/52px/";
function selectedEmoji(code) {
    emojiAPI.addEmoji(code);
    // this URL needs to be relative to SimplifiedEmoteIndicator.qml
    var emojiURL = getEmojiURLFromCode(code);

    updateEmoteIndicatorIcon(emojiURL);
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
    if (emojiAppWindow && emojiAppWindowSignalsConnected) {
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
var emojiAppWindowSignalsConnected = false;
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
    emojiAppWindowSignalsConnected = true;

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

var _this;
function EmoteBar() {
    _this = this;
}


EmoteBar.prototype = {
    startup: startup,
    unload: unload
};

module.exports = EmoteBar;


// #endregion
// *************************************
// END API
// *************************************

