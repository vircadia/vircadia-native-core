/*
- Built with same tech as settings screen. Use SettingsApp.qml as template to create the app page
- On hover or select, each emoji icon highlights. Add function for on hover to swap inactive icon with active
- Has scrolling field of emoji icons to choose from called emoji icon list. Add rows of emoji icons
- On hover or select, each emoji icon highlights. Add function for on hover to swap inactive icon with active
- Has emoji indicator at top to show current emoji icon selection
    - Emoji indicator will initially be empty
- When user clicks emoji icon, an entity image of the selected emoji appears over their avatar's head.
    - Create a function called 'addEmoji'. The 'addEmoji' fn will call 'createEmoji' and several other functions. 
    - In 'createEmoji' use entity API to add an avatar entity parented to head or fall back to neck if no head joint, hips if no neck joint.
- A 5 second timer begins Set 5 second timeout in 'addEmoji' function
- After 5 seconds, the emoji entity image shrinks and disappears over an 2 second interval When the timeout is up, start an interval for every 200ms to shrink the entity icon. After 10 intervals entity icon should be very small. Delete it and clear the interval by calling 'removeEmoji'.
- The emoji indicator and emote indicator display the time remaining until the entity icon will disappear as a highlighted pie section of the circular emoji image. Use a QML pieSeries over the emoji and emote indicators. The image for the pieSeries can be a 10% opacity white overlaycircle. Update the values of the pie slices on the 200ms interval to show the time left before the emoji disappears.
- The emote indicator is replaced with the current emoji icon (with timer display) while the emoji entity image appears over the user's head. In the 'addEmoji' function call a function that swaps the icon on the emote indicator.
- After the 7 seconds, the emoji entity image has disappeared
    - The emote indicator returns to default. In 'removeEmoji', restore the default image for the emote indicator.
    - The emoji indicator returns to empty. In 'removeEmoji', restore the default image for the emoji indicator.
    - Emoji icon highlight is removed if not hovered. In 'removeEmoji', restore the default icon for the emoji that just ended.
*/
// NOTE: play Last emoji from keypress
    // Open up on ctrl + enter
    var ENTER_KEY = 16777220;
    var SEMI_COLON_KEY = 59;
    function keyPress(event) {
        if (event.key === ENTER_KEY && event.isControl) {
            if (ui.isOpen) {
                ui.close();
            } else {
                ui.open();
            }
        } if (event.key === SEMI_COLON_KEY) {
            playEmojiAgain();
        }
    }


// NOTE: turn the favorites object into an array of top 10 favorites
    var MAX_FAVORITES = 10;
    function makeFavoritesArray() {
        var i = 0, favoritesArray = [];
        for (var emoji in favorites ) {
            favoritesArray[i++] = favorites[emoji];
        }

        favoritesArray.sort(function (a, b) {
            if (a.count > b.count) {
                return -1;
            }

            if (a.count < b.count) {
                return 1;
            }

            return 0;
        });
        favoritesArray = favoritesArray.slice(0, MAX_FAVORITES);
        return favoritesArray;
    }

// NOTE: grab an emoji in a looping ring
    function findValue(index, array, offset) {
        offset = offset || 0;
        return array[(index + offset) % array.length];
    }
// NOTE: Wear emoji like a mask
    // choose to wear a mask or above their head
    var mask = Settings.getValue("avimoji/mask", false);
    function handleMask(data) {
        mask = data.mask;
        if (currentEmoji && currentEmoji.id) {
            addEmojiToUser(selectedEmoji);
        }
        if (mask) {
            shouldTimeoutDelete = false;
        } else {
            shouldTimeoutDelete = true;
        }
        Settings.setValue("avimoji/mask", mask);
        Settings.setValue("avimoji/shouldDefaultDelete", shouldTimeoutDelete);
    }
// NOTE: Render the emojis locally
    // don't render the emojis for anyone else
    var local = Settings.getValue("avimoji/local", false);
    var entityType = Settings.getValue("avimoji/entityType", "avatar");
    function handleLocal(data) {
        local = data.local;
        Settings.setValue("avimoji/local", local);

        if (local) {
            entityType = "local";
            Settings.setValue("avimoji/entityType", "local");
        } else {
            entityType = "avatar";
            Settings.setValue("avimoji/entityType", "avatar");
        }

        if (!advanced && currentEmoji && currentEmoji.id) {
            addEmojiToUser(selectedEmoji);
        }

        maybeRedrawAnimation();
    }
// NOTE: Favorite emojis shown as overlays
    // show the favorite emojis as overlays
    var ezFavorites = Settings.getValue("avimoji/ezFavorites", false);
    function handleEZFavorites(data) {
        ezFavorites = data.ezFavorites;
        log("EZ FAVORITES", ezFavorites, PRINT);
        Settings.setValue("avimoji/ezFavorites", ezFavorites);
        if (ezFavorites) {
            maybeClearEZFavoritesTimer();
            renderEZFavoritesOverlays();   
        } else {
            deleteEZFavoritesOverlays();
            maybeClearEZFavoritesTimer();
        }    
    }
// NOTE: Handle playing emojis in sequence
    // handle moving to sequence mode
    var sequenceMode = Settings.getValue("avimoji/sequenceMode", false);
    function handleSequenceMode(data) {
        sequenceMode = data.sequenceMode;
        Settings.setValue("avimoji/sequenceMode", sequenceMode);
        if (!sequenceMode) {
            maybeClearPlayEmojiSequenceInterval();
        }
        ui.sendMessage({
            app: "avimoji",
            method: "updateEmojiPicks",
            selectedEmoji: selectedEmoji,
            emojiSequence: emojiSequence
        });
    }
// NOTE: Original Handling of all the emojis
    // what to do when someone selects an emoji in the tablet
    var MAX_EMOJI_SEQUENCE = 40;
    var emojiSequence = Settings.getValue("avimoji/emojiSequence", []);
    var selectedEmoji = null;
    function handleSelectedEmoji(data) {
        var emoji = data.emoji;
        if (advanced) {
            if (!sequenceMode) {
                selectedEmoji = emoji;
                lastEmoji = emoji;
                maybeClearTimeoutDelete();
                addEmojiToUser(selectedEmoji);
            } else {
                emojiSequence.push(emoji);
                emojiSequence = emojiSequence.slice(0, MAX_EMOJI_SEQUENCE);

                Settings.setValue("avimoji/emojiSequence", emojiSequence);
            }
        } else {
            if (selectedEmoji && selectedEmoji.code[0] === emoji.code[0]) {
                maybePlayPop("off");
                selectedEmoji = null;
                return;
            } else {
                selectedEmoji = emoji;
                lastEmoji = emoji;
                maybeClearTimeoutDelete();
                addEmojiToUser(selectedEmoji);
            }
        }
        addToFavorites(emoji);
        ui.sendMessage({
            app: "avimoji",
            method: "updateEmojiPicks",
            selectedEmoji: selectedEmoji,
            emojiSequence: emojiSequence
        });
    }
// NOTE: Changing the size of an emoji
    // dynamically update the emoji size
    function handleEmojiSize(data) {
        emojiSize = data.emojiSize;
        if (currentEmoji && currentEmoji.id) {
            addEmojiToUser(selectedEmoji);
        }
        Settings.setValue("avimoji/emojiSize", emojiSize);
        maybeRedrawAnimation();
    }
// NOTE: Change the distance traveled in an emoji sequence
        // handle a user changing the start and end distance of an emoji animation
        var DEFAULT_ANIMATION_DISTANCE = 0.5;
        var animationDistance = Settings.getValue("avimoji/animationDistance", DEFAULT_ANIMATION_DISTANCE);
        function handleAnimationDistance(data) {
            animationDistance = data.animationDistance;
            Settings.setValue("avimoji/animationDistance", animationDistance);
            maybeRedrawAnimation();
        }
// NOTE: Change the speed of emoji sequence
    // handle a user changing the speed of the emoji sequence animation
    var DEFAULT_ANIMATION_SPEED = 1.2;
    var animationSpeed = Settings.getValue("avimoji/animationSpeed", DEFAULT_ANIMATION_SPEED);
    function handleAnimationSpeed(data) {
        animationSpeed = data.animationSpeed;
        Settings.setValue("avimoji/animationDistance", animationSpeed);
        maybeRedrawAnimation();
    }
// NOTE: Update the play state 
    // Update the play state 
        var isPlaying = false;
        function handleUpdateIsPlaying(data) {
            isPlaying = data.isPlaying;
            if (isPlaying) {
                playEmojiSequence();
            } else {
                stopEmojiSequence();
            }
            ui.sendMessage({
                app: "avimoji",
                method: "updatePlay",
                isPlaying: isPlaying
            });
        }
// NOTE: Delete an Emoji from a sequence
    // remove an emoji from the sequence
    function deleteSequenceEmoji(data) {
        emojiSequence.splice(data.index, 1);
        ui.sendMessage({
            app: "avimoji",
            method: "updateEmojiPicks",
            selectedEmoji: selectedEmoji,
            emojiSequence: emojiSequence
        });
        if (emojiSequence.length === 0) {
            maybeClearPlayEmojiSequenceInterval();
        }
    }
// NOTE: Handle emoji advanced selected
    // handle if you change to move to advanced mode
    var advanced = Settings.getValue("avimoji/advanced", false);
    function handleAdvanced(data) {
        advanced = data.advanced;
        Settings.setValue("avimoji/advanced", advanced);
        if (isPlaying) {
            maybeClearPlayEmojiSequenceInterval();
            isPlaying = false;
        }
    }
// NOTE: Reset the current emoji sequence to build a new one
        function handleResetSequenceList() {
            maybeClearPlayEmojiSequenceInterval();
            emojiSequence = [];
            Settings.setValue("avimoji/emojiSequence", emojiSequence);
            ui.sendMessage({
                app: "avimoji",
                method: "updateEmojiPicks",
                selectedEmoji: selectedEmoji,
                emojiSequence: emojiSequence
            });
            ui.sendMessage({
                app: "avimoji",
                method: "updatePlay",
                isPlaying: isPlaying
            });
        }
//
// NOTE: Empty out the favorites object to start tracking them again
    function handleResetFavoritesList() {
        favorites = {};

        Settings.setValue("avimoji/favorites", favorites);
        
        ui.sendMessage({
            app: "avimoji",
            method: "updateFavorites",
            favorites: makeFavoritesArray(favorites)
        });
        deleteEZFavoritesOverlays();
    }
//
// NOTE: toggles the emoji deleting by default
    var shouldTimeoutDelete = Settings.getValue("avimoji/shouldTimeoutDelete", true);
    function handleShouldTimeoutDelete(data) {
        shouldTimeoutDelete = data.shouldTimeoutDelete;
        Settings.setValue("avimoji/shouldTimeoutDelete", shouldTimeoutDelete);
        if (shouldTimeoutDelete && currentEmoji) {
            startTimeoutDelete();
        } else {
            maybeClearTimeoutDelete();
        }
    }
//
// NOTE: play the emoji again when we hit the ";" key
    var lastEmoji = null;
    function playEmojiAgain(){
        if (currentEmoji && currentEmoji.id) {
            maybePlayPop("offThenOn");
        } else {
            createEmoji(lastEmoji);
        }
    }
//
// NOTE: see if we need to clear the timeout delete that is currently there
    function maybeClearTimeoutDelete() {
        if (defaultTimeout) {
            Script.clearTimeout(defaultTimeout);
            defaultTimeout = null;
        }
    }
//
// NOTE: add a new emoji to your favorites list
    var favorites = Settings.getValue("avimoji/favorites", {});
    function addToFavorites(emoji) {
        if (!favorites[emoji.code[UTF_CODE]]) {
            favorites[emoji.code[UTF_CODE]] = { count: 1, code: emoji.code[UTF_CODE] };
        } else {
            favorites[emoji.code[UTF_CODE]].count++;
        }
        var newFavoritesArray = makeFavoritesArray(favorites);
        Settings.setValue("avimoji/favorites", favorites);
        ui.sendMessage({
            app: "avimoji",
            method: "updateFavorites",
            favorites: newFavoritesArray
        });
        if (newFavoritesArray.length === 1 && ezFavorites) {
            var data = {ezFavorites: ezFavorites};
            handleEZFavorites(data);
            // renderEZFavoritesOverlays();
        }
    }


    // dynammically create the above the head properties in case of resize
    var ABOVE_NECK_DEFAULT = 0.2;
    var setupAboveNeck = ABOVE_NECK_DEFAULT;
    setupAvatarScale = MyAvatar.scale;
    function setupAboveHeadAnimationProperties() {
        billboardMode = "full";
        setupNeckPosition = Vec3.subtract(MyAvatar.getNeckPosition(), MyAvatar.position);
        setupEmojiPosition1 = 
            Vec3.sum(setupNeckPosition, [(END_X + START_X) / 2,
            setupAvatarScale * setupAboveNeck * (1 + emojiSize * EMOJI_CONST_SCALER), 0]);
        setupEmojiPosition1 = Vec3.sum(setupEmojiPosition1, [nextPostionXOffset, nextPostionYOffset, nextPostionZOffset]);
        setupEmojiPosition2 = 
        Vec3.sum(setupNeckPosition, 
            [START_X, setupAvatarScale * setupAboveNeck * (1 + emojiSize * EMOJI_CONST_SCALER), 0]);
        setupEmojiPosition2 = Vec3.sum(setupEmojiPosition2, [nextPostionXOffset, nextPostionYOffset, nextPostionZOffset]);
    }


// check to see if we need to redraw an animation
function maybeRedrawAnimation() {
    if (isPlaying) {
        setupAnimationVariables();
        maybeClearPlayEmojiSequenceInterval();
        playEmojiSequence();
    }
}



    // stop playing the emoji sequence
    function stopEmojiSequence() {
        maybeClearPlayEmojiSequenceInterval();
        isPlaying = false;
        currentIndex = 0;
    }

        // play the actual emoji sequence
        var animationEmoji1 = new EntityMaker(entityType);
        var animationEmoji2 = new EntityMaker(entityType);
        var animationInitialDimensions = null;
        function playEmojiSequence(){
            setupAnimationVariables();
            if (animationEmoji1 && animationEmoji1.id) {
                animationEmoji1.destroy();
                animationEmoji1 = new EntityMaker(entityType);
            }
            if (animationEmoji2 && animationEmoji2.id) {
                animationEmoji2.destroy();
                animationEmoji2 = new EntityMaker(entityType);
            }
    
            maybeClearPlayEmojiSequenceInterval();
    
            if (mask) {
                setupMaskAnimationProperties();
            } else {
                setupAboveHeadAnimationProperties();
            }
            animationEmoji1.add("parentJointIndex", MyAvatar.getJointIndex("Head"));
            animationEmoji2.add("parentJointIndex", MyAvatar.getJointIndex("Head"));
            var IMAGE_SIZE = setupAvatarScale * emojiSize;
            var dimensions = { x: IMAGE_SIZE, y: IMAGE_SIZE, z: IMAGE_SIZE };
            animationInitialDimensions = dimensions;
    
            var parentID = MyAvatar.sessionUUID;
            var imageURL1 = "";
            var imageURL2 = "";
            animationEmoji1
                .add('type', "Image")
                .add('name', 'AVIMOJI')
                .add('localPosition', setupEmojiPosition1)
                .add('dimensions', dimensions)
                .add('parentID', parentID)
                .add('emissive', true)
                .add('imageURL', imageURL1)
                .add('ignorePickIntersection', true)
                .add('billboard', billboardMode)
                .add('alpha', 1)
                .add('userData', "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }")
                .create(true);
            animationEmoji2
                .add('type', "Image")
                .add('name', 'AVIMOJI')
                .add('localPosition', setupEmojiPosition2)
                .add('dimensions', dimensions)
                .add('parentID', parentID)
                .add('emissive', true)
                .add('imageURL', imageURL2)
                .add('billboard', billboardMode)
                .add('ignorePickIntersection', true)
                .add('alpha', 1)
                .add('userData', "{ \"grabbableKey\": { \"grabbable\": true, \"kinematic\": false } }")
                .create(true);
            playEmojiInterval = Script.setInterval(onPlayEmojiInterval, DURATION_PER_STEP);
            onPlayEmojiInterval();
            isPlaying = true;
        }
    
    
        // dynamicly setup the animation properties
        var DEFAULT_ANIMATION_DURATION = 1750;
        var ANIMATION_DURATION = DEFAULT_ANIMATION_DURATION * animationSpeed;
        var HOLD_STEPS = 40;
        var ANIMATION_STEPS = 60;
        var DURATION_PER_STEP = ANIMATION_DURATION / (ANIMATION_STEPS + HOLD_STEPS * 2);
        var HALF_POINT = Math.ceil(ANIMATION_STEPS * 0.5);
        var currentStep = HALF_POINT;
        var MAX_SCALE = 1;
        var MIN_SCALE = 0.0;
        var SCALE_DISTANCE = MAX_SCALE - MIN_SCALE;
        var MAX_ALPHA = 1;
        var MIN_ALPHA = 0;
        var ALPHA_DISTANCE = MAX_ALPHA - MIN_ALPHA;
        var THRESHOLD_MIN_ALPHA = 0.001;
        var THRESHHOLD_MAX_ALPHA = 0.999;
        var DISTANCE_0_THRESHHOLD = 0.015;
        var currentScale1 = MIN_SCALE;
        var currentScale2 = MAX_SCALE;
        var currentIndex = 0;
        var playEmojiInterval = null;
        var currentAlpha1 = 1;
        var currentAlpha2 = 1;
        var middleHOLD = 0;
        var lastHOLD = 0;
        var SCALE_INCREASE_PER_STEP = SCALE_DISTANCE / HALF_POINT;
        var ALPHA_PER_STEP = ALPHA_DISTANCE / HALF_POINT;
        function setupAnimationVariables() {
            ANIMATION_DURATION = DEFAULT_ANIMATION_DURATION * animationSpeed;
            DURATION_PER_STEP = ANIMATION_DURATION / (ANIMATION_STEPS + HOLD_STEPS * 2);
            START_X = -1 * animationDistance;
            END_X = 1 * animationDistance;
            POSITION_DISTANCE = END_X - START_X;
            POSITION_PER_STEP = POSITION_DISTANCE / ANIMATION_STEPS;
            currentPosition1 = START_X;
            currentPosition2 = (END_X + START_X) / 2;
        }
    
        
        // interval for playing an emoji animation
        var START_X = -0.25 * animationDistance;
        var END_X = 0.25 * animationDistance;
        var POSITION_DISTANCE = END_X - START_X;
        var POSITION_PER_STEP = POSITION_DISTANCE / ANIMATION_STEPS;
        var currentPosition1 = START_X;
        var currentPosition2 = (END_X + START_X) / 2;
        var lastCurrent1Before0 = 0;
        var lastCurrent2Before0 = 0;
        function onPlayEmojiInterval() {
            var emoji, imageURL;
            if (currentStep === 1) {
                middleHOLD = 0;
                lastHOLD = 0;
                currentPosition1 = 0;
                currentScale1 = MAX_SCALE;
                currentAlpha1 = MAX_ALPHA;
            }
    
            if (currentStep < HALF_POINT) {
                currentScale1 += SCALE_INCREASE_PER_STEP;
                currentScale2 -= SCALE_INCREASE_PER_STEP;
                currentAlpha1 += ALPHA_PER_STEP;
                currentAlpha2 -= ALPHA_PER_STEP;
            }
    
            if (currentStep === HALF_POINT) {
                if (middleHOLD <= HOLD_STEPS) {
                    middleHOLD++;
                    return;
                }
                currentPosition2 = START_X;
                currentScale2 = MIN_SCALE;
                currentAlpha2 = MIN_ALPHA;
                emoji = findValue(currentIndex, emojiSequence);
                imageURL = imageURLBase + emoji.code[UTF_CODE] + ".png";
                animationEmoji2.edit("imageURL", imageURL);
                currentIndex++;
                ui.sendMessage({
                    app: "avimoji",
                    method: "updateCurrentEmoji",
                    selectedEmoji: emoji
                });
            }
    
            if (currentStep === ANIMATION_STEPS) {
                if (lastHOLD <= HOLD_STEPS) {
                    lastHOLD++;
                    return;
                }
                currentStep = 1;
                currentPosition1 = START_X;
                currentScale1 = MIN_SCALE;
                currentAlpha1 = MIN_ALPHA;
                middleHOLD = 0;
                lastHOLD = 0;
                emoji = findValue(currentIndex, emojiSequence);
                imageURL = imageURLBase + emoji.code[UTF_CODE] + ".png";
                animationEmoji1.edit("imageURL", imageURL);
                currentIndex++;
                ui.sendMessage({
                    app: "avimoji",
                    method: "updateCurrentEmoji",
                    selectedEmoji: emoji
                });
            }
    
            if (currentStep > HALF_POINT) {
                currentScale1 -= SCALE_INCREASE_PER_STEP;
                currentScale2 += SCALE_INCREASE_PER_STEP;
                currentAlpha1 -= ALPHA_PER_STEP;
                currentAlpha2 += ALPHA_PER_STEP;
            }
    
            var animationEmoji1Position = animationEmoji1.get("localPosition", true);
            if (currentPosition1 === 0) {
                currentPosition1 = lastCurrent1Before0;
            }
            if (currentPosition2 === 0) {
                currentPosition2 = lastCurrent2Before0;
            }
            currentPosition1 += POSITION_PER_STEP;
            currentPosition2 += POSITION_PER_STEP;
            if (Math.abs(currentPosition1) < DISTANCE_0_THRESHHOLD) {
                lastCurrent1Before0 = currentPosition1;
                currentPosition1 = 0;
            }
            if (Math.abs(currentPosition2) < DISTANCE_0_THRESHHOLD) {
                lastCurrent2Before0 = currentPosition2;
                currentPosition2 = 0;
            }
            animationEmoji1Position.x = currentPosition1;
            var animationEmoji2Position = animationEmoji2.get("localPosition", true);
            animationEmoji2Position.x = currentPosition2;
            var emojiPosition1 = animationEmoji1Position;
            var emojiPosition2 = animationEmoji2Position;
            var newDimensions1 = Vec3.multiply(animationInitialDimensions, EasingFunctions.easeInOutQuad(currentScale1));
            var newDimensions2 = Vec3.multiply(animationInitialDimensions, EasingFunctions.easeInOutQuad(currentScale2));
            var newAlpha1 = EasingFunctions.easeInOutQuint(currentAlpha1);
            var newAlpha2 = EasingFunctions.easeInOutQuint(currentAlpha2);
    
            if (newAlpha1 > THRESHHOLD_MAX_ALPHA) {
                newAlpha1 = 1.0;
            }
    
            if (newAlpha1 < THRESHOLD_MIN_ALPHA) {
                newAlpha1 = 0.0;
            }
    
            if (newAlpha2 > THRESHHOLD_MAX_ALPHA) {
                newAlpha2 = 1.0;
            }
    
            if (newAlpha2 < THRESHOLD_MIN_ALPHA) {
                newAlpha2 = 0.0;
            }
    
            animationEmoji1
                .add("localPosition", emojiPosition1)
                .add("dimensions", newDimensions1)
                .add("alpha", newAlpha1)
                .edit();
    
            animationEmoji2
                .add("localPosition", emojiPosition2)
                .add("dimensions", newDimensions2)
                .add("alpha", newAlpha2)
                .edit();
            currentStep++;
    
        }
    

    // Star the delete process and handle the right animation path for turning off
    var DEFAULT_TIMEOUT_MS = 7000;
    var defaultTimeout = null;
    function startTimeoutDelete() {
        defaultTimeout = Script.setTimeout(function () {
            maybePlayPop("off");
            selectedEmoji = null;
        }, DEFAULT_TIMEOUT_MS);
    }

    

    // check to see if we need to clear the emoji interval
    function maybeClearPlayEmojiSequenceInterval() {
        if (animationEmoji1 && animationEmoji1.id) {
            animationEmoji1.destroy();
        }
        if (animationEmoji2 && animationEmoji2.id) {
            animationEmoji2.destroy();
        }
        if (currentEmoji && currentEmoji.id) {
            currentEmoji.destroy();
            currentEmoji = new EntityMaker("avatar");
            deleteEmojiPreviewOverlay();
        }
        if (playEmojiInterval) {
            Script.clearInterval(playEmojiInterval);
            playEmojiInterval = null;
            currentIndex = 0;
            isPlaying = false;
        }
    }

    // custom logging function
    var PREPEND = "\n##Logger:Avimoji:Web::\n";
    var DEBUG = false;
    // var OFF = "off";
    // var ON = "on";
    // var PRINT = "PRINT";
    var PRETTY_SPACES = 4;
    function log(label, data, overrideDebug) {
        if (!DEBUG) {
            if (overrideDebug !== "PRINT") {
                return;
            }
        } else {
            if (overrideDebug === "off") {
                return;
            }
        }

        data = typeof data === "undefined" ? "" : data;
        data = typeof data === "string" ? data : (JSON.stringify(data, null, PRETTY_SPACES) || "");
        data = data + " " || "";
        console.log(PREPEND + label + ": " + data + "\n");
    }

    function playPopAnimationIn() {
        var dimensions;
        if (currentPopStep === 1) {
            isPopPlaying = true;
            currentPopScale = MIN_POP_SCALE;
        }
        currentPopScale += POP_PER_STEP;

        dimensions = Vec3.multiply(currentSelectedDimensions, EasingFunctions.easeInCubic(currentPopScale));
        currentEmoji.edit("dimensions", dimensions);
        currentPopStep++;

        if (currentPopStep === POP_ANIMATION_STEPS) {
            playSound();
            maybeClearPop();
        }
    }

    // play an animation pop out
    function playPopAnimationOut() {
        var dimensions;
        if (currentPopStep === 1) {
            isPopPlaying = true;
            currentPopScale = MAX_POP_SCALE;
            playSound();
        }
        currentPopScale -= POP_PER_STEP;
        dimensions = Vec3.multiply(currentSelectedDimensions, EasingFunctions.easeOutCubic(currentPopScale));
        currentEmoji.edit("dimensions", dimensions);
        currentPopStep++;

        if (currentPopStep === POP_ANIMATION_STEPS) {
            if (currentEmoji && currentEmoji.id) {
                // currentEmoji.destroy();
                // currentEmoji = new EntityMaker("avatar");
                selectedEmoji = null;
                ui.sendMessage({
                    app: "avimoji",
                    method: "updateEmojiPicks",
                    selectedEmoji: selectedEmoji
                });
            }
            maybeClearPop();
        }
    }


    // play an animatino coming in and then going out
    function playPopAnimationInAndOut() {
        var dimensions;
        if (currentPopStep === 1) {
            isPopPlaying = true;
            currentPopScale = MAX_POP_SCALE;
            playSound();
        }

        currentPopScale -= POP_PER_STEP;
        dimensions = Vec3.multiply(currentSelectedDimensions, EasingFunctions.easeOutCubic(currentPopScale));
        currentEmoji.edit("dimensions", dimensions);
        currentPopStep++;

        if (currentPopStep === POP_ANIMATION_STEPS) {
            if (currentEmoji && currentEmoji.id) {
                currentEmoji.destroy();
                currentEmoji = new EntityMaker("avatar");
            }
            maybeClearPop();
            Script.setTimeout(function () {
                createEmoji(selectedEmoji);
            }, DURATION);
        }
    }


// show all of the emojis instead of just the basic set
var allEmojis = Settings.getValue("avimoji/allEmojis", false);
function handleAllEmojis(data) {
    allEmojis = data.allEmojis;
    Settings.setValue("avimoji/allEmojis", allEmojis);
}


