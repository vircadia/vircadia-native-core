//
//
//  Created by The Content Team 4/10/216
//  Copyright 2016 High Fidelity, Inc.
//
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include('../utils.js');
    Script.include('../virtualBaton.js');
    var _this;

    var d = new Date();
    var h = d.getHours();
    h = h % 12;

    // Only one person should similate the clock at a time- so we pass a virtual baton
    var baton;
    var iOwn = false;
    var connected = false;
    var CUCKOO_SOUND_URL = "atp:/cuckooClock/cuckoomix.wav";
    CuckooClockMinuteHand = function() {
        _this = this;
        _this.TIME_LOCK_DURATION = 300000;
        _this.checkTime = true;
        _this.cuckooSound = SoundCache.getSound(CUCKOO_SOUND_URL);
        _this.CUCKOO_SOUND_VOLUME = 0.1;

    };

    function startUpdate() {
        // print("EBL Start CLOCK Update")
            //when the baton is claimed;
            //   print('trying to claim the object' + _entityID)
        iOwn = true;
        connected = true;


        _this.updateSecondHandForOwnersLocalTime();

        Script.update.connect(_this.update);
    }



    function stopUpdateAndReclaim() {
        //when the baton is released;
        print('EBL I released the object ' + _this.entityID)
        iOwn = false;
        if (connected === true) {
            connected = false;
            Script.update.disconnect(_this.update);
        }
        //hook up callbacks to the baton
        baton.claim(startUpdate, stopUpdateAndReclaim);
    }

    CuckooClockMinuteHand.prototype = {


        preload: function(entityID) {
            _this.entityID = entityID; // this.animation.isRunning = true;
            _this.userData = getEntityUserData(entityID);
            // print("ANIMATION!!! " + JSON.stringify(_this.animationURL));
            if (!_this.userData || !_this.userData.clockBody) {
                // print("THIS CLOCK HAND IS NOT ATTACHED TO A CLOCK BODY!");
                return;
            }
            _this.clockBody = _this.userData.clockBody;
            _this.secondHand = _this.userData.secondHand;
            // One winner for each entity

            if (Entities.canRez() && Entities.canAdjustLocks) {
                // print("EBL I HAVE EDIT RIGHTS, SO SET UP BATON!");
                baton = virtualBaton({
                    batonName: "io.highfidelity.cuckooClock:" + _this.entityID
                });
                stopUpdateAndReclaim();
            }

        },

        updateSecondHandForOwnersLocalTime: function() {
            // We are taking over so make sure to update our second hand to reflect local time
            if (!_this.secondHand) {
                // print("WE DONT HAVE A SECOND HAND! RETURNING");
                return;
            }
            var clockRotation = Entities.getEntityProperties(_this.clockBody, "rotation").rotation;
            var DEGREES_FOR_SECOND = 6;
            var myDate = new Date();
            var seconds = myDate.getSeconds();
            secondRollDegrees = -seconds * DEGREES_FOR_SECOND;
            var localClockHandRotation = Quat.fromPitchYawRollDegrees(0, 0, secondRollDegrees);
            var worldClockHandRotation = Quat.multiply(clockRotation, localClockHandRotation);
            Entities.editEntity(_this.secondHand, {
                rotation: worldClockHandRotation
            });

        },

        unload: function() {
            baton.unload();
            if (connected === true) {
                connected = false;
                Script.update.disconnect(_this.update);
            }
        },

        update: function() {

            if (iOwn === false) {
                return;
            }

            _this.clockBodyAnimationProps = Entities.getEntityProperties(_this.clockBody, "animation").animation;
            if (!_this.clockBodyAnimationProps) {
                // print("NO CLOCK BODY ANIMATION PROPS! RETURNING");
                return;
            }

            if (_this.checkTime === false) {
                return;
            }
            var date = new Date();
            // Check to see if we are at the top of the hour
            var seconds = date.getSeconds();
            var minutes = date.getMinutes();

            if (minutes % 15 === 0 && seconds === 0) {
                _this.popCuckooOut();
            }

        },

        popCuckooOut: function() {
            _this.checkTime = false;
            // We are at the top of the hour!
            _this.position = Entities.getEntityProperties(_this.entityID, "position").position;
            // print("EBL POP CUCKOO CLOCK!!!!!!!!!!!");
            if (!_this.cuckooSoundInjector) {
                _this.cuckooSoundInjector = Audio.playSound(_this.cuckooSound, {
                    position: _this.position,
                    volume: _this.CUCKOO_SOUND_VOLUME
                });
            } else {
                _this.cuckooSoundInjector.stop();
                _this.cuckooSoundInjector.restart();
            }

            Entities.editEntity(_this.clockBody, {
                animation: {
                    running: true,
                    currentFrame: 0
                }
            });
            Script.setTimeout(function() {
                _this.checkTime = true;
            }, _this.TIME_LOCK_DURATION);
        }


    };

    // entity scripts always need to return a newly constructed object of our type
    return new CuckooClockMinuteHand();
});