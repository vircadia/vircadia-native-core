/*

    Avimoji
    avimoji_app.js
    Created by Milad Nazeri on 2019-04-25
    Copyright 2019 High Fidelity, Inc.

    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

*/

// *************************************
// START dependencies
// *************************************
// #region dependencies

// Custom module for handling entities
var EntityMaker = Script.require("./resources/modules/entityMaker.js?" + Date.now());
// Add nice smoothing functions to the animations
var EasingFunctions = Script.require("./resources/modules/easing.js?");

// The information needed to properly use the sprite sheets and get the general information
// about the emojis
var emojiList = Script.require("./resources/modules/emojiList.js?" + Date.now());
// The location to find where the individual emoji icons are
var CONFIG = Script.require("./resources/config.json?" + Date.now());
// Where to load the images from taken from the Config above
var imageURLBase = Script.resolvePath(CONFIG.baseImagesURL);


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
var soundUrl = Script.resolvePath('./resources/sounds/emojiPopSound.wav');
var popSound = SoundCache.getSound(soundUrl);
var injector;
var DEFAULT_VOLUME = 0.0003;
var local = false;
function playSound(sound, volume, position, localOnly) {
    sound = sound || popSound;
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
var DEFAULT_TIMEOUT_MS = 5000;
var defaultTimeout = null;
function startTimeoutDelete() {
    defaultTimeout = Script.setTimeout(function () {
        // This is called to start the shrink animation and also where the deleting happens when that is done
        maybePlayPop("off");
        countDownTimer = Script.setInterval(countDownTimerHandler, COUNT_DOWN_INTERVAL_MS);
        selectedEmoji = null;
    }, DEFAULT_TIMEOUT_MS);
}


// The QML has a property called archEnd on the pie chart that controls how much pie is showing
var COUNT_DOWN_INTERVAL_MS = 100;
var countDownTimer = null;
function countDownTimerHandler() {
    currentArch += ARCH_END_PER_ANIMATION_STEP;
    _this._avimojiQMLWindow.sendToQml({
        "source": "simplifiedEmoji.js",
        "method": "updateArchEnd",
        "data": {
            "archEnd": currentArch
        }
    });
    
}


function maybeClearCountDownTimerHandler() {
    if (countDownTimer) {
        Script.clearTimeout(countDownTimer);
        _this._avimojiQMLWindow.sendToQml({
            "source": "simplifiedEmoji.js",
            "method": "updateArchEnd",
            "data": {
                "archEnd": 0
            }
        });
    }
}



function resetEmojis() {
    pruneOldAvimojis();
    maybeClearPop();
    maybeClearCountDownTimerHandler();
    if (currentEmoji && currentEmoji.id) {
        currentEmoji.destroy();
        selectedEmoji = null;
    }
    _this._avimojiQMLWindow.sendToQml({
        "source": "simplifiedEmoji.js",
        "method": "updateArchEnd",
        "data": {
            "archEnd": 0
        }
    });
    // Turns off locking the selected emoji in the app
    _this._avimojiQMLWindow.sendToQml({
        "source": "simplifiedEmoji.js",
        "method": "isSelected",
        "data": {
            "isSelected": false
        }
    });
    currentArch = 0;
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
var selectedEmoji = null;
var lastEmoji = null;
function handleSelectedEmoji(emoji) {
    console.log("in handleSelectedEmoji");
    if (selectedEmoji && selectedEmoji.code[UTF_CODE] === emoji.code[UTF_CODE]) {
        return;
    } else {
        maybeClearTimeoutDelete();
        maybeClearCountDownTimerHandler();
        selectedEmoji = emoji;
        lastEmoji = emoji;
        console.log("handleSelected about to go to addEmoji - selectedEmoji")
        addEmoji(selectedEmoji);
    }
}

function onDomainChanged() {
    resetEmojis();
}


function onScaleChanged() {
    resetEmojis();
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
function addEmoji(emoji) {
    if (currentEmoji && currentEmoji.id) {
        resetEmojis();
    }
    createEmoji(emoji);
}


// creating the actual emoji that isn't an animation
/*
    MILAD NOTE:
    The Above head should be what you need to control how high the distance is.  
    The calcuation might need to be played with more depending on what kind of avatar is brought in and what 
    their scale maybe.  So far this seems to be ok. 

    EntityMaker is a helper library I made.  Don't be too scared about it, you just make an entity wrapper
    with new, use .add to gather properties, .create to add those property, you can also send in a quick edit with .edit
    then .destroy to kill the entity

    The images come from the entity object compiled from entityList.json.  Right now code is an array where just one index
    which is why there is a const UTF_CODE that represents 0.  
    This can probably be fixed by just editing that .code to not be an array,
    but as long as you know what that is, I think it is fine and not worth the trouble. 

    There was a note about falling back to other joints in the spec, 
    but I don't think that is necessary if this is meant for simplified
    as we can guarentee an avatar that is supported has to have a head joint.  
    That is unless we are future proofing for the ability
    to use any kind of avatar.
*/
var ABOVE_HEAD = 0.61;
var EMOJI_X_OFFSET = 0.0;
var DEFAULT_EMOJI_SIZE = 0.37;
var currentEmoji = new EntityMaker("avatar");
var emojiMaxDimensions = null;
function createEmoji(emoji) {
    var emojiPosition;
    var imageURL = imageURLBase + emoji.code[UTF_CODE] + ".png";

    var IMAGE_SIZE = MyAvatar.scale * DEFAULT_EMOJI_SIZE;
    emojiMaxDimensions = { x: IMAGE_SIZE, y: IMAGE_SIZE, z: IMAGE_SIZE };
    
    emojiPosition = [
        EMOJI_X_OFFSET, 
        ABOVE_HEAD,
        0
    ];

    currentEmoji
        .add('type', 'Image')
        .add('name', 'AVIMOJI')
        .add('localPosition', emojiPosition)
        .add('dimensions', [0, 0, 0])
        .add('parentID', MyAvatar.sessionUUID)
        .add('parentJointIndex', MyAvatar.getJointIndex("Head"))
        .add('emissive', true)
        .add('collisionless', true)
        .add('imageURL', imageURL)
        .add('billboardMode', "full")
        .add('ignorePickIntersection', true)
        .add('alpha', 1)
        .add('grab', { grabbable: false })
        .create();
        
    maybePlayPop("in");
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


// see what we need to do when an emoji gets clicked
/*
    MILAD NOTE:
    First clear any current pop animations

    Then we check what kind of pop animation might be requested, either an in or an out.
    If it's an out, reset the pie just in case.
*/
function maybePlayPop(type) {
    console.log("In maybe play pop");
    maybeClearPop();
    popType = type;
    if (popType === "in") {
        playPopInterval = Script.setInterval(playPopAnimation, POP_DURATION_IN_PER_STEP);
    } else {
        _this._avimojiQMLWindow.sendToQml({
            "source": "simplifiedEmoji.js",
            "method": "updateArchEnd",
            "data": {
                "archEnd": 0
            }
        });
        currentArch = 0;
        playPopInterval = Script.setInterval(playPopAnimation, POP_DURATION_OUT_PER_STEP);
    }
}


// maybe clear a pop up animation not in animation mode
function maybeClearPop() {
    console.log("In maybe clear pop");
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
        1. The duration of the animiation
        2. How many animation steps there will be for that duration
            The animation duration / the animation steps
        3. The max and min values 
*/

var currentPopStep = 1;
var currentPopScale = null;
var popType = null;
var playPopInterval = null;
var finalInPopScale = null;
var currentArch = 0;

// Degrees % Animation steps to know how much to fill in for each step
var DEGREES_IN_CIRCLE = 360;
var ARCH_END_PER_ANIMATION_STEP = DEGREES_IN_CIRCLE / POP_ANIMATION_OUT_STEPS; 

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
            // Start with the pop sound on the out
            currentPopScale = finalInPopScale ? finalInPopScale : MAX_POP_SCALE;
            playSound();
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
        currentArch += ARCH_END_PER_ANIMATION_STEP;
    }

    currentPopStep++;
    currentEmoji.edit("dimensions", dimensions);

    // Handle if it's the end of the animation step

    if (currentPopStep === POP_ANIMATION_STEPS) {
        if (popType === "in") {
            finalInPopScale = currentPopScale;
            playSound();
            dimensions = [
                Math.max(dimensions.x, emojiMaxDimensions.x),
                Math.max(dimensions.y, emojiMaxDimensions.y),
                Math.max(dimensions.z, emojiMaxDimensions.z)
            ];
            currentEmoji.edit("dimensions", dimensions);
        } else {
            // make sure there is a currentEmoji entity before trying to delete
            if (currentEmoji && currentEmoji.id) {
                currentEmoji.destroy();
                currentEmoji = new EntityMaker("avatar");
            }
            finalInPopScale = null;
            selectedEmoji = null;
            maybeClearCountDownTimerHandler();
            // Tell the UI we can show other icons in the selected window ok
            _this._avimojiQMLWindow.sendToQml({
                "source": "simplifiedEmoji.js",
                "method": "isSelected",
                "data": {
                    "isSelected": false
                }
            });
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
function init() {
    
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

    pruneOldAvimojis();

    MyAvatar.scaleChanged.connect(onScaleChanged);
    Script.scriptEnding.connect(scriptEnding);
}


// #endregion
// *************************************
// END main
// *************************************

// *************************************
// START cleanup
// *************************************
// #region cleanup


function scriptEnding() {
    resetEmojis();
    Window.domainChanged.disconnect(onDomainChanged);
    MyAvatar.scaleChanged.disconnect(onScaleChanged);

}


// #endregion
// *************************************
// END cleanup
// *************************************

// *************************************
// START API
// *************************************
// #region API

var _this;
function AviMoji() {
    _this = this;
    this._avimojiQMLWindow;
}

function registerAvimojiQMLWindow(avimojiQMLWindow) {
    this._avimojiQMLWindow = avimojiQMLWindow;
}

function addEmojiFromQML(code) {
    var emoji = emojiList[emojiCodeMap[code]];
    handleSelectedEmoji(emoji);
    _this._avimojiQMLWindow.sendToQml({
        "source": "simplifiedEmoji.js",
        "method": "isSelected",
        "data": {
            "isSelected": true
        }
    });
}

function unload() {
    scriptEnding();
}

function startup() {
    init();
}

AviMoji.prototype = {
    startup: startup,
    addEmoji: addEmojiFromQML,
    unload: unload,
    registerAvimojiQMLWindow: registerAvimojiQMLWindow
};


module.exports = AviMoji;

// #endregion
// *************************************
// END API
// *************************************