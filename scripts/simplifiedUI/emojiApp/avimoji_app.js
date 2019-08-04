/*

    Avimoji
    avimoji_app.js
    Created by Milad Nazeri on 2019-04-25
    Copyright 2019 High Fidelity, Inc.

    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

*/

(function () {

    // *************************************
    // START dependencies
    // *************************************
    // #region dependencies

    // Custom module for handling entities
    var EntityMaker = Script.require("./resources/modules/entityMaker.js?" + Date.now());
    // Add nice smoothing functions to the animations
    var EasingFunctions = Script.require("./resources/modules/easing.js");

    // The information needed to properly use the sprite sheets and get the general information
    // about the emojis
    var emojiList = Script.require("./resources/node/emojiList.json?" + Date.now());
    // The location to find where the individual emoji icons are
    var CONFIG = Script.require("./resources/config.json?" + Date.now());
    // Where to load the images from taken from the Config above
    var imageURLBase = CONFIG.baseImagesURL;

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

    // Make the emoji groups
    var MAX_PER_GROUP = 250;
    var emojiChunks = [];
    function makeEmojiChunks() {
        for (var i = 0, len = emojiList.length; i < len; i += MAX_PER_GROUP) {
            emojiChunks.push(emojiList.slice(i, i + MAX_PER_GROUP));
        }
    }


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


    // prune old emojis on you
    function pruneOldAvimojis() {
        MyAvatar.getAvatarEntitiesVariant()
            .forEach(function (avatarEntity) {
                if (avatarEntity && avatarEntity.properties.name.toLowerCase().indexOf("avimoji") > -1) {
                    Entities.deleteEntity(avatarEntity.id);
                }
            });
    }


    // help send an emoji chunk to load up faster in the tablet
    var INTERVAL = 200;
    var currentChunk = 0;
    function sendEmojiChunks() {
        if (currentChunk >= emojiChunks.length) {
            currentChunk = 0;
            return;
        } else {
            var chunk = emojiChunks[currentChunk];
            ui.sendMessage({
                app: "avimoji",
                method: "sendChunk",
                chunkNumber: currentChunk,
                totalChunks: emojiChunks.length,
                chunk: chunk
            });
            currentChunk++;
            Script.setTimeout(function () {
                sendEmojiChunks();
            }, INTERVAL);
        }
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
    function handleSelectedEmoji(data) {
        var emoji = data.emoji;
        if (selectedEmoji && selectedEmoji.code[UTF_CODE] === emoji.code[UTF_CODE]) {
            maybePlayPop("off");
            selectedEmoji = null;
            return;
        } else {
            selectedEmoji = emoji;
            lastEmoji = emoji;
            addEmoji(selectedEmoji);
        }
        ui.sendMessage({
            app: "avimoji",
            method: "updateEmojiPicks",
            selectedEmoji: selectedEmoji
        });
    }


    // handle what happens when unselect an emoji
    function handleSelectedRemoved() {
        maybePlayPop("off");
        selectedEmoji = null;
        ui.sendMessage({
            app: "avimoji",
            method: "updateEmojiPicks",
            selectedEmoji: selectedEmoji
        });
    }

    function onDomainChanged() {
        pruneOldAvimojis();
        maybeClearPop();
        if (currentEmoji && currentEmoji.id) {
            currentEmoji.destroy();
            selectedEmoji = null;
        }
        maybeClearPop();
        if (ui.isOpen) {
            ui.close();
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
    var billboardMode = "full";
    var currentSelectedDimensions = null;
    function addEmoji(emoji) {
        if (currentEmoji && currentEmoji.id) {
            maybePlayPop("offThenOn");
        } else {
            createEmoji(emoji);
        }
    }


    // creating the actual emoji that isn't an animation
    var ABOVE_HEAD = 0.60;
    var EMOJI_CONST_SCALER = 0.27;
    var currentEmoji = new EntityMaker("avatar");
    var EMOJI_X_OFFSET = -0.01;
    var DEFAULT_EMOJI_SIZE = 0.27;
    var emojiSize = Settings.getValue("avimoji/emojiSize", DEFAULT_EMOJI_SIZE);
    function createEmoji(emoji) {
        var neckPosition, avatarScale, aboveNeck, emojiPosition;
        avatarScale = MyAvatar.scale;
        aboveNeck = ABOVE_HEAD;
        neckPosition = Vec3.subtract(MyAvatar.getNeckPosition(), MyAvatar.position);
        emojiPosition = Vec3.sum(
            neckPosition, 
            [
                EMOJI_X_OFFSET, 
                avatarScale * aboveNeck * (1 + emojiSize * EMOJI_CONST_SCALER), 
                0
            ]);
        var IMAGE_SIZE = avatarScale * emojiSize;
        var dimensions = { x: IMAGE_SIZE, y: IMAGE_SIZE, z: IMAGE_SIZE };
        currentSelectedDimensions = dimensions;

        var parentID = MyAvatar.sessionUUID;
        var imageURL = imageURLBase + emoji.code[UTF_CODE] + ".png";
        currentEmoji
            .add('type', "Image")
            .add('name', 'AVIMOJI')
            .add('localPosition', emojiPosition)
            .add('dimensions', [0, 0, 0])
            .add('parentID', parentID)
            .add('emissive', true)
            .add('collisionless', true)
            .add('imageURL', imageURL)
            .add('billboardMode', billboardMode)
            .add('ignorePickIntersection', true)
            .add('alpha', 1)
            .add('grab', {grabbable: false})
            .create();
        maybePlayPop("on");
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
    var DURATION = POP_ANIMATION_DURATION_MS + 100;
    function maybePlayPop(type) {
        maybeClearPop();
        popType = type;
        playPopInterval = Script.setInterval(playPopAnimation, POP_DURATION_PER_STEP);
    }


    // maybe clear a pop up animation not in animation mode
    function maybeClearPop() {
        if (playPopInterval) {
            Script.clearTimeout(playPopInterval);
            playPopInterval = null;
            currentPopScale = null;
            currentPopStep = 1;
            isPopPlaying = false;
            popType = null;
        }
    }


    // play an animation pop in
    var currentPopStep = 1;
    var POP_ANIMATION_DURATION_MS = 170;
    var POP_ANIMATION_STEPS = 10;
    var POP_DURATION_PER_STEP = POP_ANIMATION_DURATION_MS / POP_ANIMATION_STEPS;
    var currentPopScale = null;
    var MAX_POP_SCALE = 1;
    var MIN_POP_SCALE = 0.0;
    var POP_SCALE_DISTANCE = MAX_POP_SCALE - MIN_POP_SCALE;
    var POP_PER_STEP = POP_SCALE_DISTANCE / POP_ANIMATION_STEPS;
    var isPopPlaying = false;
    var popType = null;
    var playPopInterval = null;
    function playPopAnimation() {
        var dimensions;

        // Handle if this is the first step of the animation

        if (currentPopStep === 1) {
            isPopPlaying = true;
            if (popType === "in") {
                currentPopScale = MIN_POP_SCALE;
            } else {
                currentPopScale = MAX_POP_SCALE;
                playSound();
            }
        }

        // Setup the animation step

        if (popType === "in") {
            currentPopScale += POP_PER_STEP;
            dimensions = Vec3.multiply(currentSelectedDimensions, EasingFunctions.easeInCubic(currentPopScale));
        } else {
            currentPopScale -= POP_PER_STEP;
            dimensions = Vec3.multiply(currentSelectedDimensions, EasingFunctions.easeOutCubic(currentPopScale));
        }
        currentPopStep++;
        currentEmoji.edit("dimensions", dimensions);

        // Handle if it's the end of the animation step

        if (currentPopStep === POP_ANIMATION_STEPS) {
            if (popType === "in") {
                playSound();
            } else {
                if (currentEmoji && currentEmoji.id) {
                    currentEmoji.destroy();
                    currentEmoji = new EntityMaker("avatar");
                    if (popType === "out") {
                        selectedEmoji = null;
                        ui.sendMessage({
                            app: "avimoji",
                            method: "updateEmojiPicks",
                            selectedEmoji: selectedEmoji
                        });
                    }
                    if (popType === "offThenOn") {
                        Script.setTimeout(function () {
                            createEmoji(selectedEmoji);
                        }, DURATION);
                    }                
                }
            }
            maybeClearPop();
        }
    }


    // #endregion
    // *************************************
    // END animation
    // *************************************

    // *************************************
    // START messages
    // *************************************
    // #region messages


    // handle getting a message from the tablet
    function onMessage(message) {
        if (message.app !== "avimoji") {
            return;
        }

        switch (message.method) {
            case "eventBridgeReady":
                ui.sendMessage({
                    app: "avimoji",
                    method: "updateUI",
                    selectedEmoji: selectedEmoji
                });
                sendEmojiChunks();
                break;

            case "handleSelectedEmoji":
                handleSelectedEmoji(message.data);
                break;

            case "handleSelectedRemoved":
                handleSelectedRemoved();
                break;

            default:
                console.log("Unhandled message from avimoji_ui.js", message);
                break;
        }
    }


    // #endregion
    // *************************************
    // END messages
    // *************************************

    // *************************************
    // START main
    // *************************************
    // #region main


    // startup the app
    var BUTTON_NAME = "AVIMOJI";
    var APP_UI_URL = Script.resolvePath('./resources/avimoji_ui.html');
    var AppUI = Script.require('./resources/modules/appUi.js?' + Date.now());
    var ui;
    var emojiCodeMap;
    function startup() {
        // make a map of just the utf codes to help with accesing
        emojiCodeMap = emojiList.reduce(function (previous, current, index) {
            if (current && current.code && current.code.length > 0 && current.code[UTF_CODE]) {
                previous[current.code[UTF_CODE]] = index;
                return previous;
            }
        }, {});
        ui = new AppUI({
            buttonName: BUTTON_NAME,
            home: APP_UI_URL,
            onMessage: onMessage,
            graphicsDirectory: Script.resolvePath("./resources/images/icons/")
        });

        pruneOldAvimojis();
        makeEmojiChunks();


        Script.scriptEnding.connect(scriptEnding);
    }


    startup();


    // #endregion
    // *************************************
    // END main
    // *************************************

    // *************************************
    // START cleanup
    // *************************************
    // #region cleanup


    function scriptEnding() {
        if (ui.isOpen) {
            ui.onClosed();
        }
        pruneOldAvimojis();
        Window.domainChanged.disconnect(onDomainChanged);
        if (currentEmoji && currentEmoji.id) {
            currentEmoji.destroy();
        }
        maybeClearPop();
    }


    // #endregion
    // *************************************
    // END cleanup
    // *************************************

})();