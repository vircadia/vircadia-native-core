/*

    simplifiedEmoji.js
    Created by Milad Nazeri on 2019-04-25
    Copyright 2019 High Fidelity, Inc.

    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

*/

// *************************************
// START dependencies
// *************************************
// #region dependencies

// Add nice smoothing functions to the animations
var EasingFunctions = Script.require("./resources/modules/easing.js");

// The information needed to properly use the sprite sheets and get the general information
// about the emojis
var emojiList = Script.require("./resources/modules/emojiList.js");
var customEmojiList = Script.require("./resources/modules/customEmojiList.js");
// The contents of this remote folder must always contain all possible emojis for users of `simplifiedEmoji.js`
var imageURLBase = Script.getExternalPath(Script.ExternalPaths.HF_Content, "MarketplaceItems/avimoji/appResources/appData/resources/images/emojis/png512/");
// Uncomment below for local testing
//imageURLBase = Script.resolvePath("./resources/images/emojis/512px/");


// #endregion
// *************************************
// END dependencies
// *************************************

// *************************************
// START utility
// *************************************
// #region utility


// CONSTS
// UTF-Codes are stored on the first index of one of the keys on a returned emoji object.
// Just makes the code a little easier to read 
var UTF_CODE = 0;

// Play sound borrowed from bingo app:
// Plays the specified sound at the specified position, volume, and localOnly
// Only plays a sound if it is downloaded.
// Only plays one sound at a time.
var emojiCreateSound = SoundCache.getSound(Script.resolvePath('resources/sounds/emojiPopSound1.wav'));
var injector;
var DEFAULT_VOLUME = 0.01;
var local = false;
function playSound(sound, volume, position, localOnly) {
    volume = volume || DEFAULT_VOLUME;
    position = position || MyAvatar.position;
    localOnly = localOnly || local;
    if (sound.downloaded) {
        if (injector) {
            injector.stop();
        }
        injector = Audio.playSound(sound, {
            position: position,
            volume: volume,
            localOnly: localOnly
        });
    }
}


// Checks to see if there are any avimoji entities on you already to delete
function pruneOldAvimojis() {
    MyAvatar.getAvatarEntitiesVariant()
        .forEach(function (avatarEntity) {
            if (avatarEntity && avatarEntity.properties.name.toLowerCase().indexOf("avimoji") > -1) {
                Entities.deleteEntity(avatarEntity.id);
            }
        });
}

function maybeClearTimeoutDelete() {
    if (defaultTimeout) {
        Script.clearTimeout(defaultTimeout);
        defaultTimeout = null;
    }
}


// Start the delete process and handle the right animation path for turning off
var TOTAL_EMOJI_DURATION_MS = 7000;
var defaultTimeout = null;
function startTimeoutDelete() {
    defaultTimeout = Script.setTimeout(function () {
        // This is called to start the shrink animation and also where the deleting happens when that is done
        maybePlayPop("off");
        selectedEmojiFilename = null;
        defaultTimeout = false;
    }, TOTAL_EMOJI_DURATION_MS - POP_ANIMATION_OUT_DURATION_MS);
}


// The QML has a property called archEnd on the pie chart that controls how much pie is showing
function beginCountDownTimer() {
    _this._avimojiQMLWindow.sendToQml({
        "source": "simplifiedEmoji.js",
        "method": "beginCountdownTimer"
    });
    
}


function clearCountDownTimerHandler() {
    if (_this._avimojiQMLWindow) {
        _this._avimojiQMLWindow.sendToQml({
            "source": "simplifiedEmoji.js",
            "method": "clearCountdownTimer"
        });
    }
}


function resetEmojis() {
    pruneOldAvimojis();
    maybeClearPop();
    clearCountDownTimerHandler();
    if (currentEmoji) {
        Entities.deleteEntity(currentEmoji);
    }
    currentEmoji = false;
    selectedEmojiFilename = null;
}


// #endregion
// *************************************
// END utility
// *************************************

// *************************************
// START ui_handlers
// *************************************
// #region ui_handlers


// what to do when someone selects an emoji in the tablet
var selectedEmojiFilename = null;
function handleSelectedEmoji(emojiFilename) {
    if (selectedEmojiFilename === emojiFilename) {
        return;
    } else {
        maybeClearTimeoutDelete();
        clearCountDownTimerHandler();
        selectedEmojiFilename = emojiFilename;
        addEmoji(selectedEmojiFilename);
    }
}


function onDomainChanged() {
    resetEmojis();
}


function onScaleChanged() {
    resetEmojis();
}


function onAddingWearable(id) {
    var props = Entities.getEntityProperties(id, ["name"]);
    if (props.name.toLowerCase().indexOf("avimoji") > -1) {
        Entities.deleteEntity(id);
    }
}


// #endregion
// *************************************
// END ui_handlers
// *************************************

// *************************************
// START avimoji
// *************************************
// #region avimoji


// what happens when we need to add an emoji over a user
var firstEmojiMadeOnStartup = false;
function addEmoji(emojiFilename) {
    if (!firstEmojiMadeOnStartup) {
        firstEmojiMadeOnStartup = true;
        Entities.addingWearable.disconnect(onAddingWearable);
    }

    if (currentEmoji) {
        resetEmojis();
    }
    createEmoji(emojiFilename);
}


// creating the actual emoji that isn't an animation
var ABOVE_HEAD = 0.61;
var EMOJI_X_OFFSET = 0.0;
var DEFAULT_EMOJI_SIZE = 0.37;
var currentEmoji = false;
var emojiMaxDimensions = null;
function createEmoji(emojiFilename) {
    var emojiPosition;
    var imageURL = imageURLBase + emojiFilename;

    var IMAGE_SIZE = MyAvatar.scale * DEFAULT_EMOJI_SIZE;
    emojiMaxDimensions = { x: IMAGE_SIZE, y: IMAGE_SIZE, z: IMAGE_SIZE };
    
    emojiPosition = [
        EMOJI_X_OFFSET, 
        ABOVE_HEAD,
        0
    ];

    currentEmoji = Entities.addEntity({
        "type": "Image",
        "name": "AVIMOJI",
        "localPosition": emojiPosition,
        "dimensions": [0, 0, 0],
        "parentID": MyAvatar.sessionUUID,
        "parentJointIndex": MyAvatar.getJointIndex("Head"),
        "emissive": true,
        "collisionless": true,
        "imageURL": imageURL,
        "billboardMode": "full",
        "ignorePickIntersection": true,
        "alpha": 1,
        "grab": { "grabbable": false }
    }, "avatar");
        
    maybePlayPop("in");
    beginCountDownTimer();
    startTimeoutDelete();
}


// #endregion
// *************************************
// END avimoji
// *************************************

// *************************************
// START animation
// *************************************
// #region animation


/*
    see what we need to do when an emoji gets clicked
    First clear any current pop animations
    Then check what kind of pop animation might be requested, either an in or an out.
*/
function maybePlayPop(type) {
    maybeClearPop();
    popType = type;
    if (popType === "in") {
        playPopInterval = Script.setInterval(playPopAnimation, POP_DURATION_IN_PER_STEP);
    } else {
        playPopInterval = Script.setInterval(playPopAnimation, POP_DURATION_OUT_PER_STEP);
    }
}


// maybe clear a pop up animation not in animation mode
function maybeClearPop() {
    if (playPopInterval) {
        Script.clearTimeout(playPopInterval);
        playPopInterval = null;
        currentPopScale = null;
        currentPopStep = 1;
        popType = null;
    }
}


// play an animation pop in
/*
    We are accounting for an in animation and an out animation
    We need the following to setup an animation
        1. The duration of the animation
        2. How many animation steps there will be for that duration
            The animation duration / the animation steps
        3. The max and min values 
*/

var currentPopStep = 1;
var currentPopScale = null;
var popType = null;
var playPopInterval = null;
var finalInPopScale = null;

var MAX_POP_SCALE = 1;
var MIN_POP_SCALE = 0;
var POP_SCALE_DISTANCE = MAX_POP_SCALE - MIN_POP_SCALE;

// POP IN ANIMATION SETUP
var POP_ANIMATION_IN_DURATION_MS = 200;
var POP_ANIMATION_IN_STEPS = 10;
var POP_DURATION_IN_PER_STEP = POP_ANIMATION_IN_DURATION_MS / POP_ANIMATION_IN_STEPS;
var POP_IN_PER_STEP = POP_SCALE_DISTANCE / POP_ANIMATION_IN_STEPS;

// POP OUT ANIMATION SETUP
var POP_ANIMATION_OUT_DURATION_MS = 2000;
var POP_ANIMATION_OUT_STEPS = 100;
var POP_DURATION_OUT_PER_STEP = POP_ANIMATION_OUT_DURATION_MS / POP_ANIMATION_OUT_STEPS;
var POP_OUT_PER_STEP = POP_SCALE_DISTANCE / POP_ANIMATION_OUT_STEPS;

function playPopAnimation() {
    // Get the right kind of settings from above based on the animation requested
    var POP_PER_STEP = popType === "in" ? POP_IN_PER_STEP : POP_OUT_PER_STEP;
    var POP_ANIMATION_STEPS = popType === "in" ? POP_ANIMATION_IN_STEPS : POP_ANIMATION_OUT_STEPS;

    var dimensions;

    // Handle if this is the first step of the animation

    if (currentPopStep === 1) {
        if (popType === "in") {
            currentPopScale = MIN_POP_SCALE;
        } else {
            currentPopScale = finalInPopScale ? finalInPopScale : MAX_POP_SCALE;
        }
    }

    // Setup the animation step

    if (popType === "in") {
        currentPopScale += POP_PER_STEP;
        currentPopScale = Math.max(currentPopScale, MIN_POP_SCALE);
        currentPopScale = Math.min(currentPopScale, MAX_POP_SCALE);
        dimensions = Vec3.multiply(emojiMaxDimensions, EasingFunctions.easeInCubic(currentPopScale));
    } else {
        currentPopScale -= POP_PER_STEP;
        currentPopScale = Math.max(currentPopScale, MIN_POP_SCALE);
        currentPopScale = Math.min(currentPopScale, MAX_POP_SCALE);
        dimensions = Vec3.multiply(emojiMaxDimensions, EasingFunctions.easeOutCubic(currentPopScale));

        // Make sure we use the maximum dimensions just in case
        dimensions = [
            Math.min(dimensions.x, emojiMaxDimensions.x),
            Math.min(dimensions.y, emojiMaxDimensions.y),
            Math.min(dimensions.z, emojiMaxDimensions.z)
        ];
    }

    currentPopStep++;
    if (currentEmoji) {
        Entities.editEntity(currentEmoji, {"dimensions": dimensions});
    }

    // Handle if it's the end of the animation step

    if (currentPopStep === POP_ANIMATION_STEPS) {
        if (popType === "in") {
            finalInPopScale = currentPopScale;
            playSound(emojiCreateSound);
            dimensions = [
                Math.max(dimensions.x, emojiMaxDimensions.x),
                Math.max(dimensions.y, emojiMaxDimensions.y),
                Math.max(dimensions.z, emojiMaxDimensions.z)
            ];
            if (currentEmoji) {
                Entities.editEntity(currentEmoji, {"dimensions": dimensions});
            }
        } else {
            pruneOldAvimojis();
            if (currentEmoji) {
                Entities.deleteEntity(currentEmoji);
            }
            currentEmoji = false;
            finalInPopScale = null;
            selectedEmojiFilename = null;
            clearCountDownTimerHandler();
        }
        maybeClearPop();
    }
}


// #endregion
// *************************************
// END animation
// *************************************

// *************************************
// START main
// *************************************
// #region main


// startup the app
var emojiCodeMap;
var customEmojiCodeMap;
var signalsConnected = false;
var _this;
function startup() {
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

    pruneOldAvimojis();

    Script.scriptEnding.connect(unload);
    Window.domainChanged.connect(onDomainChanged);
    MyAvatar.scaleChanged.connect(onScaleChanged);
    Entities.addingWearable.connect(onAddingWearable);
    signalsConnected = true;

    function AviMoji() {
        _this = this;
        this._avimojiQMLWindow = null;
    }

    AviMoji.prototype = {
        addEmoji: addEmojiFromQML,
        registerAvimojiQMLWindow: registerAvimojiQMLWindow
    };

    return new AviMoji();
}


function registerAvimojiQMLWindow(avimojiQMLWindow) {
    this._avimojiQMLWindow = avimojiQMLWindow;
}


function addEmojiFromQML(code) {
    var emojiObject = emojiList[emojiCodeMap[code]];
    var emojiFilename;
    // If `emojiObject` isn't defined here, that probably means we're looking for a custom emoji
    if (!emojiObject) {
        emojiFilename = customEmojiList[customEmojiCodeMap[code]].filename;
    } else {
        emojiFilename = emojiObject.code[UTF_CODE] + ".png";
    }
    handleSelectedEmoji(emojiFilename);
}


function unload() {
    resetEmojis();
    if (signalsConnected) {
        Window.domainChanged.disconnect(onDomainChanged);
        MyAvatar.scaleChanged.disconnect(onScaleChanged);
        if (!firstEmojiMadeOnStartup) {
            Entities.addingWearable.disconnect(onAddingWearable);
        }

        signalsConnected = false;
    }
}


var aviMoji = startup();

module.exports = aviMoji;