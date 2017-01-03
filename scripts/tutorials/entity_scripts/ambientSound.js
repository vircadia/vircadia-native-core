//  ambientSound.js
//
//  This entity script will allow you to create an ambient sound that loops when a person is within a given
//  range of this entity.  Great way to add one or more ambisonic soundfields to your environment.  
//
//  In the userData section for the entity, add/edit three values:  
//      userData.soundURL should be a string giving the URL to the sound file.  Defaults to 100 meters if not set.  
//      userData.range should be an integer for the max distance away from the entity where the sound will be audible.
//      userData.volume is the max volume at which the clip should play.  Defaults to 1.0 full volume)
//
//  The rotation of the entity is copied to the ambisonic field, so by rotating the entity you will rotate the 
//  direction in-which a certain sound comes from.  
//  
//  Remember that the entity has to be visible to the user for the sound to play at all, so make sure the entity is
//  large enough to be loaded at the range you set, particularly for large ranges.  
//   
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){ 
    //  This sample clip and range will be used if you don't add userData to the entity (see above)
    var DEFAULT_RANGE = 100;
    var DEFAULT_URL = "http://hifi-content.s3.amazonaws.com/ken/samples/forest_ambiX.wav";
    var DEFAULT_VOLUME = 1.0;

    var soundURL = "";
    var range = DEFAULT_RANGE;
    var maxVolume = DEFAULT_VOLUME;
    var UPDATE_INTERVAL_MSECS = 100;
    var rotation;

    var entity;
    var ambientSound;
    var center;
    var soundPlaying = false;
    var checkTimer = false;
    var _this;

    var WANT_COLOR_CHANGE = false;
    var COLOR_OFF = { red: 128, green: 128, blue: 128 };
    var COLOR_ON = { red: 255, green: 0, blue: 0 };

    var WANT_DEBUG = false;
    function debugPrint(string) {
        if (WANT_DEBUG) {
            print(string);
        }
    }

    this.updateSettings = function() {
        //  Check user data on the entity for any changes
        var oldSoundURL = soundURL;
        var props = Entities.getEntityProperties(entity, [ "userData" ]);
        if (props.userData) {
            var data = JSON.parse(props.userData);
            if (data.soundURL && !(soundURL === data.soundURL)) {
                soundURL = data.soundURL;
                debugPrint("Read ambient sound URL: " + soundURL);    
            } else if (!data.soundURL) {
                soundURL = DEFAULT_URL;
            }
            if (data.range && !(range === data.range)) {
                range = data.range;
                debugPrint("Read ambient sound range: " + range);
            }
            if (data.volume && !(maxVolume === data.volume)) {
                maxVolume = data.volume;
                debugPrint("Read ambient sound volume: " + maxVolume);
            }
        }
        if (!(soundURL === oldSoundURL) || (soundURL === "")) {
            debugPrint("Loading ambient sound into cache");
            ambientSound = SoundCache.getSound(soundURL);
            if (soundPlaying && soundPlaying.playing) {
                soundPlaying.stop();
                soundPlaying = false;
                if (WANT_COLOR_CHANGE) {
                    Entities.editEntity(entity, { color: COLOR_OFF });
                }
                debugPrint("Restarting ambient sound");
            }
        }   
    }

    this.preload = function(entityID) { 
        //  Load the sound and range from the entity userData fields, and note the position of the entity.
        debugPrint("Ambient sound preload");
        entity = entityID;
        _this = this;
        checkTimer = Script.setInterval(this.maybeUpdate, UPDATE_INTERVAL_MSECS);
    }; 

    this.maybeUpdate = function() {
        // Every UPDATE_INTERVAL_MSECS, update the volume of the ambient sound based on distance from my avatar
        _this.updateSettings();
        var HYSTERESIS_FRACTION = 0.1;
        var props = Entities.getEntityProperties(entity, [ "position", "rotation" ]);
        center = props.position;
        rotation = props.rotation;
        var distance = Vec3.length(Vec3.subtract(MyAvatar.position, center));
        if (distance <= range) {
            var volume = (1.0 - distance / range) * maxVolume;
            if (!soundPlaying && ambientSound.downloaded) {
                soundPlaying = Audio.playSound(ambientSound,  { loop: true, 
                                                                localOnly: true, 
                                                                orientation: rotation,
                                                                volume: volume });
                debugPrint("Starting ambient sound, volume: " + volume);
                if (WANT_COLOR_CHANGE) {
                    Entities.editEntity(entity, { color: COLOR_ON });
                }
                
            } else if (soundPlaying && soundPlaying.playing) {
                soundPlaying.setOptions( { volume: volume, orientation: rotation } );
            }

        } else if (soundPlaying && soundPlaying.playing && (distance > range * HYSTERESIS_FRACTION)) {
            soundPlaying.stop();
            soundPlaying = false;
            Entities.editEntity(entity, { color: { red: 128, green: 128, blue: 128 }});
            debugPrint("Out of range, stopping ambient sound");
        }
    }

    this.unload = function(entityID) { 
        debugPrint("Ambient sound unload");
        if (checkTimer) {
            Script.clearInterval(checkTimer);
        }
        if (soundPlaying && soundPlaying.playing) {
            soundPlaying.stop();
        }
    }; 

}) 
