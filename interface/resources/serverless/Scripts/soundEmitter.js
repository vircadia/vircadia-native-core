/* eslint-disable no-magic-numbers */
//
// soundEmitter.js
// 
// Created by Zach Fox on 2019-07-05
// Copyright High Fidelity 2019
//
// Licensed under the Apache 2.0 License
// See accompanying license file or http://apache.org/
//

(function() {
    var that;
    var SOUND_EMITTER_UPDATE_INTERVAL_MS = 500;
    var DEFAULT_AUDIO_INJECTOR_OPTIONS = {
        "position": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "volume": 0.5,
        "loop": false,
        "localOnly": false
    };


    var SoundEmitter = function() {
        that = this;
        that.entityID = false;
        that.soundObjectURL = false;
        that.soundObject = false;
        that.audioInjector = false;
        that.audioInjectorOptions = DEFAULT_AUDIO_INJECTOR_OPTIONS;
        that.signalsConnected = {};
        that.soundEmitterUpdateInterval = false;
    };


    SoundEmitter.prototype = {
        preload: function(entityID) {
            that.entityID = entityID;

            var properties = Entities.getEntityProperties(that.entityID, ["userData"]);

            var userData;

            try {
                userData = JSON.parse(properties.userData);
            } catch (e) {
                console.error("Error parsing userData: ", e);
            }

            if (userData) {
                if (userData.soundURL && userData.soundURL.length > 0) {
                    that.handleNewSoundURL(userData.soundURL);
                } else {
                    console.log("Please specify this entity's `userData`! See README.md for instructions.");
                    return;
                }
            } else {
                console.log("Please specify this entity's `userData`! See README.md for instructions.");
                return;
            }
        },


        clearCurrentSoundData: function() {
            that.soundObjectURL = false;

            if (that.signalsConnected["soundObjectReady"]) {
                that.soundObject.ready.disconnect(that.updateSoundEmitter);
                that.signalsConnected["soundObjectReady"] = false;
            }

            that.soundObject = false;

            if (that.audioInjector) {
                if (that.audioInjector.playing) {
                    that.audioInjector.stop();
                }

                that.audioInjector = false;
            }

            that.audioInjectorOptions = DEFAULT_AUDIO_INJECTOR_OPTIONS;

            if (that.soundEmitterUpdateInterval) {
                Script.clearInterval(that.soundEmitterUpdateInterval);
                that.soundEmitterUpdateInterval = false;
            }
        },


        unload: function() {
            that.clearCurrentSoundData();
        },


        handleNewSoundURL: function(newSoundURL) {
            that.clearCurrentSoundData();

            that.soundObjectURL = newSoundURL;
            that.soundObject = SoundCache.getSound(that.soundObjectURL);

            if (that.soundObject.downloaded) {
                that.updateSoundEmitter();
            } else {
                that.soundObject.ready.connect(that.updateSoundEmitter);
                that.signalsConnected["soundObjectReady"] = true;
            }
        },


        positionChanged: function(newPos) {
            return (newPos.x !== that.audioInjectorOptions.position.x ||
                newPos.y !== that.audioInjectorOptions.position.y ||
                newPos.z !== that.audioInjectorOptions.position.z);
        },


        updateSoundEmitter: function() {
            var properties = Entities.getEntityProperties(that.entityID, ["userData", "position", "script", "serverScripts"]);

            var userData;

            try {
                userData = JSON.parse(properties.userData);
            } catch (e) {
                console.error("Error parsing userData: ", e);
            }

            var optionsChanged = false;
            var shouldRestartPlayback = false;
            var newPosition = properties.position;

            if (userData) {
                if (userData.soundURL && userData.soundURL.length > 0 && userData.soundURL !== that.soundObjectURL) {
                    console.log("Sound Emitter: User put a new sound URL into `userData`! Resetting...");
                    that.handleNewSoundURL(userData.soundURL);
                    return;
                }

                if (typeof(userData.volume) !== "undefined" && !isNaN(userData.volume) && userData.volume !== that.audioInjectorOptions.volume) {
                    optionsChanged = true;
                    that.audioInjectorOptions.volume = userData.volume;
                }
                
                if (typeof(userData.shouldLoop) !== "undefined" && userData.shouldLoop !== that.audioInjectorOptions.shouldLoop) {
                    optionsChanged = true;

                    if (!that.audioInjectorOptions.loop && userData.shouldLoop && that.audioInjector && !that.audioInjector.playing) {
                        shouldRestartPlayback = true;
                    }

                    that.audioInjectorOptions.loop = userData.shouldLoop;
                }

                if (typeof(userData.positionOverride) !== "undefined" && !isNaN(userData.positionOverride.x) &&
                    !isNaN(userData.positionOverride.y) && !isNaN(userData.positionOverride.z)) {
                    newPosition = userData.positionOverride;
                }
            } else {
                console.log("Please specify this entity's `userData`! See README.md for instructions.");
                that.clearCurrentSoundData();
                return;
            }            

            var localOnly = that.audioInjectorOptions.localOnly;
            // If this script is attached as a client entity script...
            if (properties.script.indexOf("soundEmitter.js") > -1) {
                // ... Make sure that the audio injector IS local only.
                localOnly = true;
            // Else if this script is attached as a client server script...
            } else if (properties.serverScripts.indexOf("soundEmitter.js") > -1) {
                // ... Make sure that the audio injector IS NOT local only.
                localOnly = false;
            }
            if (localOnly !== that.audioInjectorOptions.localOnly) {
                optionsChanged = true;
                that.audioInjectorOptions.localOnly = localOnly;
            }

            if (that.positionChanged(newPosition)) {
                optionsChanged = true;
                that.audioInjectorOptions["position"] = newPosition;
            }

            if (!that.audioInjector || shouldRestartPlayback) {
                that.audioInjector = Audio.playSound(that.soundObject, that.audioInjectorOptions);
            } else if (optionsChanged) {
                that.audioInjector.setOptions(that.audioInjectorOptions);
            }

            if (!that.soundEmitterUpdateInterval) {
                that.soundEmitterUpdateInterval = Script.setInterval(that.updateSoundEmitter, SOUND_EMITTER_UPDATE_INTERVAL_MS);
            }
        }
    };

    
    return new SoundEmitter();
});
