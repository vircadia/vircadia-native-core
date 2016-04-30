//
//  boombox.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 9/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example of a boom box entity script which when assigned to an entity, will detect when the entity is being touched by the avatars hands
//  and start to play a boom box song
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../libraries/utils.js");

    var _this;
    var BOOMBOX_USER_DATA_KEY = "boombox";
    var THE_SONG = "http://hifi-public.s3.amazonaws.com/ryan/freaks7.wav";

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    DetectTouched = function() { 
         _this = this;
         _this.beingTouched = false;
         _this.isPlaying = false;
         _this.injector = null;
    };

    DetectTouched.prototype = {

        // update() will be called regulary, because we've hooked the update signal in our preload() function.
        // we will check the avatars hand positions and if either hand is in our bounding box, we will notice that
        update: function() {
            // because the update() signal doesn't have a valid this, we need to use our memorized _this to access our entityID
            var entityID = _this.entityID;

            var leftHandPosition = MyAvatar.getLeftPalmPosition();
            var rightHandPosition = MyAvatar.getRightPalmPosition();
            var props = Entities.getEntityProperties(entityID);
            var entityMinPoint = props.boundingBox.brn;
            var entityMaxPoint = props.boundingBox.tfl;

            // this is our assumed boombox data if it's not known
            var defaultBoomboxData = { isPlaying: false, avatarID: null };

            // this handy function getEntityCustomData() is available in utils.js and it will return just the specific section
            // of user data we asked for. If it's not available it returns our default data.
            var boomboxData = getEntityCustomData(BOOMBOX_USER_DATA_KEY, entityID, defaultBoomboxData);

            // Only allow interaction if no one else is interacting with the boombox or if we are...
            if (!boomboxData.isPlaying || boomboxData.avatarID == MyAvatar.sessionUUID) {

                // If we were not previously being touched, and we just got touched, then we do our touching action.
                if (pointInExtents(leftHandPosition, entityMinPoint, entityMaxPoint) || pointInExtents(rightHandPosition, entityMinPoint, entityMaxPoint)) {

                    if (!_this.beingTouched) {
                        print("I was just touched...");

                        // remember we're being grabbed so we can detect being released
                        _this.beingTouched = true;

                        // determine what to do based on if we are currently playing
                        if (!_this.isPlaying) {
                            // if we are not currently playing, then start playing.
                            print("Start playing...");

                            _this.isPlaying = true;

                            if (_this.injector == null) {
                                _this.injector = Audio.playSound(_this.song,  {
                                    position: props.position, // position of boombox entity
                                    volume: 0.1,
                                    loop: true
                                });
                            } else {
                                _this.injector.restart();
                            }
                            setEntityCustomData(BOOMBOX_USER_DATA_KEY, entityID, { isPlaying: true, avatarID: MyAvatar.sessionUUID });

                        } else { 
                            // if we are currently playing, then stop playing
                            print("Stop playing...");
                            _this.isPlaying = false;
                            _this.injector.stop();

                            setEntityCustomData(BOOMBOX_USER_DATA_KEY, entityID, { isPlaying: false, avatarID: null });
                        } // endif (!isPlaying)

                    } // endif !beingTouched
                } else if (_this.beingTouched) {
                    // if we are not being grabbed, and we previously were, then we were just released, remember that
                    // and print out a message
                    _this.beingTouched = false;
                    print("I'm am no longer being touched...");
                }

            } else {
                // end if for -- only interact if no one else is...
                print("someone else is playing the boombox...");
            }
        },

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * connecting to the update signal so we can check our grabbed state
        preload: function(entityID) {
            print("preload!");
            this.entityID = entityID;
            Script.update.connect(this.update);
            this.song = SoundCache.getSound(THE_SONG);
        },

        // unload() will be called when our entity is no longer available. It may be because we were deleted,
        // or because we've left the domain or quit the application. In all cases we want to unhook our connection
        // to the update signal
        unload: function(entityID) {
            Script.update.disconnect(this.update);
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new DetectTouched();
})
