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


// Returns the first valid joint position from the list of supplied test joint positions.
// If none are valid, returns MyAvatar.position.
function getValidJointPosition(jointsToTest) {
    var currentJointIndex;

    for (var i = 0; i < jointsToTest.length; i++) {
        currentJointIndex = MyAvatar.getJointIndex(jointsToTest[i]);

        if (currentJointIndex > -1) {
            return MyAvatar.getJointPosition(jointsToTest[i]);
        }
    }

    return Vec3.sum(MyAvatar.position, Vec3.multiply(0.25, Quat.getForward(MyAvatar.orientation)));
} 


// Returns the world position halfway between the user's hands
var HALF = 0.5;
function getClapPosition() {
    var validLeftJoints = ["LeftHandMiddle2", "LeftHand", "LeftArm"];
    var leftPosition = getValidJointPosition(validLeftJoints);

    var validRightJoints = ["RightHandMiddle2", "RightHand", "RightArm"];
    var rightPosition = getValidJointPosition(validRightJoints);

    var centerPosition = Vec3.sum(leftPosition, rightPosition);
    centerPosition = Vec3.multiply(centerPosition, HALF);

    return centerPosition;
}


var clapSoundInterval = false;
var CLAP_SOUND_INTERVAL_MS_FLOOR = 260;
var CLAP_SOUND_INTERVAL_MS_CEIL = 320;
function startClappingSounds() {
    maybeClearClapSoundInterval();

    // Compute a random clap sound interval to avoid strange echos between many people clapping simultaneously
    var clapSoundIntervalMS = Math.floor(randomFloat(CLAP_SOUND_INTERVAL_MS_FLOOR, CLAP_SOUND_INTERVAL_MS_CEIL));

    clapSoundInterval = Script.setInterval(function() {
        playSound(clapSounds[Math.floor(Math.random() * clapSounds.length)], getClapPosition(), true);
    }, clapSoundIntervalMS);
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
    } else {
        beginReactionWrapper(reaction);
    }
}


function maybeDeleteRemoteIndicatorTimeout() {
    if (restoreEmoteIndicatorTimeout) {
        Script.clearTimeout(restoreEmoteIndicatorTimeout);
        restoreEmoteIndicatorTimeout = null;
    }
}

var reactionsBegun = [];
var pointReticle = null;
var mouseMoveEventsConnected = false;
var targetPointInterpolateConnected = false;
var pointAtTarget = Vec3.ZERO;
var isReticleVisible = true;

function targetPointInterpolate() {
    if (reticlePosition) {
        pointAtTarget = Vec3.mix(pointAtTarget, reticlePosition, POINT_AT_MIX_ALPHA);
        isReticleVisible = MyAvatar.setPointAt(pointAtTarget);
    }
}


function maybeClearInputAudioLevelsInterval() {
    if (checkInputAudioLevelsInterval) {
        Script.clearInterval(checkInputAudioLevelsInterval);
        checkInputAudioLevelsInterval = false;
        currentNumTimesAboveThreshold = 0;
    }
}


var currentNumTimesAboveThreshold = 0;
var checkInputAudioLevelsInterval;
// The values below are determined empirically and may require tweaking over time if users
// notice false-positives or false-negatives.
var CHECK_INPUT_AUDIO_LEVELS_INTERVAL_MS = 200;
var AUDIO_INPUT_THRESHOLD = 130;
var NUM_REQUIRED_LEVELS_ABOVE_AUDIO_INPUT_THRESHOLD = 4;
function checkInputLevelsCallback() {
    if (MyAvatar.audioLoudness > AUDIO_INPUT_THRESHOLD) {
        currentNumTimesAboveThreshold++;
    } else {
        currentNumTimesAboveThreshold = 0;
    }

    if (currentNumTimesAboveThreshold >= NUM_REQUIRED_LEVELS_ABOVE_AUDIO_INPUT_THRESHOLD) {
        endReactionWrapper("raiseHand");
        currentNumTimesAboveThreshold = 0;
    }
}


function beginReactionWrapper(reaction) {
    maybeDeleteRemoteIndicatorTimeout();

    reactionsBegun.forEach(function(react) {
        endReactionWrapper(react);
    });

    if (MyAvatar.beginReaction(reaction)) {
        reactionsBegun.push(reaction);
    }
    
    updateEmoteIndicatorIcon("images/" + reaction + "_Icon.svg");

    // Insert reaction-specific logic here:
    switch (reaction) {
        case ("applaud"):
            startClappingSounds();
            break;
        case ("point"):
            deleteOldReticles();
            pointAtTarget = MyAvatar.getHeadLookAt();
            if (!mouseMoveEventsConnected) {
                Controller.mouseMoveEvent.connect(mouseMoveEvent);
                mouseMoveEventsConnected = true;
            }
            if (!targetPointInterpolateConnected) {
                Script.update.connect(targetPointInterpolate);
                targetPointInterpolateConnected = true;
            }
            break;
        case ("raiseHand"):
            checkInputAudioLevelsInterval = Script.setInterval(checkInputLevelsCallback, CHECK_INPUT_AUDIO_LEVELS_INTERVAL_MS);
            break;
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
var POINT_AT_MIX_ALPHA = 0.15;
var reticlePosition = Vec3.ZERO;
function mouseMoveEvent(event) {
    if (!reticleUpdateRateLimiterTimer) {
        reticleUpdateRateLimiterTimer = Script.setTimeout(function() {
            reticleUpdateRateLimiterTimer = false;
        }, RETICLE_UPDATE_RATE_LIMITER_TIMER_MS);
    } else {
        return;
    }


    var pickRay = Camera.computePickRay(event.x, event.y);
    var avatarIntersectionData = AvatarManager.findRayIntersection(pickRay, [], [MyAvatar.sessionUUID], false);
    var entityIntersectionData = Entities.findRayIntersection(pickRay, true);
    var avatarIntersectionDistanceM = avatarIntersectionData.intersects && avatarIntersectionData.distance < MAX_INTERSECTION_DISTANCE_M ? avatarIntersectionData.distance : null;
    var entityIntersectionDistanceM = entityIntersectionData.intersects && entityIntersectionData.distance < MAX_INTERSECTION_DISTANCE_M ? entityIntersectionData.distance : null;

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
        Entities.editEntity(pointReticle, { position: reticlePosition, visible: isReticleVisible });
    } else if (reticlePosition) {
        pointReticle = Entities.addEntity({
            type: "Box",
            name: "Point Reticle",
            position: reticlePosition,
            dimensions: { x: 0.075, y: 0.075, z: 0.075 },
            color: { red: 255, green: 0, blue: 0 },
            collisionless: true,
            ignorePickIntersection: true,
            grab: {
                grabbable: false
            }
        }, true);
    }
}


var WAIT_TO_RESTORE_EMOTE_INDICATOR_ICON_MS = 2000;
var restoreEmoteIndicatorTimeout;
function triggerReactionWrapper(reaction) {
    maybeDeleteRemoteIndicatorTimeout();

    reactionsBegun.forEach(function(react) {
        endReactionWrapper(react);
    });

    MyAvatar.triggerReaction(reaction);
    updateEmoteIndicatorIcon("images/" + reaction + "_Icon.svg");

    restoreEmoteIndicatorTimeout = Script.setTimeout(function() {
        updateEmoteIndicatorIcon("images/emote_Icon.svg");
        restoreEmoteIndicatorTimeout = null;
    }, WAIT_TO_RESTORE_EMOTE_INDICATOR_ICON_MS);
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
    
    updateEmoteIndicatorIcon("images/emote_Icon.svg");

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
            if (targetPointInterpolateConnected) {
                Script.update.disconnect(targetPointInterpolate);
                targetPointInterpolateConnected = false;
            }
            maybeClearReticleUpdateLimiterTimeout();
            deleteOldReticles();
            break;
        case ("raiseHand"):
            maybeClearInputAudioLevelsInterval();
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
            if (!message.data.isPressingAndHolding) {
                return;
            }
            triggerReactionWrapper("positive");
            break;
        case "negative":
            if (!message.data.isPressingAndHolding) {
                return;
            }
            triggerReactionWrapper("negative");
            break;
        case "applaud":
            if (message.data.isPressingAndHolding) {
                beginReactionWrapper(message.method);
            } else {
                endReactionWrapper(message.method);
            }
            break;
        case "point":
        case "raiseHand":
            if (!message.data.isPressingAndHolding) {
                return;
            }
            toggleReaction(message.method);
            break;
        case "toggleEmojiApp": 
            if (!message.data.isPressingAndHolding) {
                return;
            }
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


function onWindowMinimizedChanged(isMinimized) {
    isWindowMinimized = isMinimized;
    maybeChangeEmoteIndicatorVisibility(!isMinimized);
}


// These keys need to match what's in `SimplifiedEmoteIndicator.qml` in the `buttonsModel`
// for the tooltips to match the actual keys.
var POSITIVE_KEY = "z";
var NEGATIVE_KEY = "x";
var APPLAUD_KEY = "c";
var RAISE_HAND_KEY = "v";
var POINT_KEY = "b";
var EMOTE_WINDOW = "f";
function keyPressHandler(event) {
    if (HMD.active) {
        return;
    }

    if (!event.isAutoRepeat && ! event.isMeta && ! event.isControl && ! event.isAlt) {
        if (event.text.toLowerCase() === POSITIVE_KEY) {
            triggerReactionWrapper("positive");
        } else if (event.text.toLowerCase() === NEGATIVE_KEY) {
            triggerReactionWrapper("negative");
        } else if (event.text.toLowerCase() === RAISE_HAND_KEY) {
            toggleReaction("raiseHand");
        } else if (event.text.toLowerCase() === APPLAUD_KEY) {
            // Make sure this doesn't get triggered if you are flying, falling, or jumping
            if (!MyAvatar.isInAir()) {
                toggleReaction("applaud");
            }
        } else if (event.text.toLowerCase() === POINT_KEY) {
            toggleReaction("point");
        } else if (event.text.toLowerCase() === EMOTE_WINDOW && !(Settings.getValue("io.highfidelity.isEditing", false))) {
            toggleEmojiApp();
        }
    }
}


function keyReleaseHandler(event) {
    if (!event.isAutoRepeat) {
        if (event.text.toLowerCase() === APPLAUD_KEY) {
            if (reactionsBegun.indexOf("applaud") > -1) {
                toggleReaction("applaud");
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
    0x00000008 | // Qt::Popup
    0x00000002 | // Qt::Tool
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
        relativePosition: {
            x: EMOTE_APP_BAR_LEFT_MARGIN,
            y: EMOTE_APP_BAR_BOTTOM_MARGIN
        },
        relativePositionAnchor: Desktop.RelativePositionAnchor.BOTTOM_LEFT,
        overrideFlags: EMOTE_APP_BAR_WINDOW_FLAGS
    });

    emoteAppBarWindow.fromQml.connect(onMessageFromEmoteAppBar);
}


// There is currently no property in the Window Scripting Interface to determine
// whether the Interface window is currently minimized. This feels like an oversight.
// We should add that functionality to the Window Scripting Interface, and remove `isWindowMinimized` below.
var isWindowMinimized = false;
function maybeChangeEmoteIndicatorVisibility(desiredVisibility) {
    if (isWindowMinimized || HMD.active) {
        desiredVisibility = false;
    }

    if (desiredVisibility && !emoteAppBarWindow) {
        showEmoteAppBar();
    } else if (!desiredVisibility && emoteAppBarWindow) {
        emoteAppBarWindow.fromQml.disconnect(onMessageFromEmoteAppBar);
        emoteAppBarWindow.close();
        emoteAppBarWindow = false;
    }
}


function handleFTUEScreensVisibilityChanged(ftueScreenVisible) {
    maybeChangeEmoteIndicatorVisibility(!ftueScreenVisible);
}


function onDisplayModeChanged(isHMDMode) {
    reactionsBegun.forEach(function(react) {
        endReactionWrapper(react);
    });

    maybeChangeEmoteIndicatorVisibility(!isHMDMode);
}


var emojiAPI = Script.require("./emojiApp/simplifiedEmoji.js");
var keyPressSignalsConnected = false;
var emojiCodeMap;
var customEmojiCodeMap;
var _this;
function setup() {
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

    Window.minimizedChanged.connect(onWindowMinimizedChanged);
    HMD.displayModeChanged.connect(onDisplayModeChanged);

    getSounds();
    maybeChangeEmoteIndicatorVisibility(true);
    
    Controller.keyPressEvent.connect(keyPressHandler);
    Controller.keyReleaseEvent.connect(keyReleaseHandler);
    keyPressSignalsConnected = true;
    Script.scriptEnding.connect(unload);
    
    function Emote() {
        _this = this;
    }

    Emote.prototype = {
        handleFTUEScreensVisibilityChanged: handleFTUEScreensVisibilityChanged
    };

    return new Emote();
}


function unload() {
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

    maybeClearClapSoundInterval();
    maybeClearReticleUpdateLimiterTimeout();
    maybeDeleteRemoteIndicatorTimeout();
    maybeClearInputAudioLevelsInterval();

    Window.minimizedChanged.disconnect(onWindowMinimizedChanged);
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
            x: Window.x + EMOTE_APP_BAR_LEFT_MARGIN,
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

var emote = setup();

module.exports = emote;