//
//  lightningEntity.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 3/1/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This is an example of an entity script which will randomly create a flash of lightning and a thunder sound
//  effect, as well as a background rain sound effect. It can be applied to any entity, although it works best
//  on a zone entity.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */
(function () {
    "use strict";

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Various configurable settings.
    //
    // You can change these values to change some of the various effects of the rain storm.
    // These values can also be controlled by setting user properties on the entity that you've attached this script to.
    // add a "lightning" section to a JSON encoded portion of the user data... for example:
    //    {
    //      "lightning": {
    //        "flashMax": 20,
    //        "flashMin": 0,
    //        "flashMaxRandomness": 10,
    //        "flashIntensityStepRandomeness": 2,
    //        "averageLighteningStrikeGap": 120,
    //        "extraRandomRangeLightningStrikeGap": 10,
    //        "thunderURL": "atp:1336efe995398f5e0d46b37585785de8ba872fe9a9b718264db03748cd41c758.wav",
    //        "thunderVolume": 0.1,
    //        "rainURL": "atp:e0cc7438aca776636f6e6f731685781d9999b961c945e4e5760d937be5beecdd.wav",
    //        "rainVolume": 0.05
    //      }
    //      // NOTE: you can have other user data here as well, so long as it's JSON encoded, it won't impact the lightning script
    //    }
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    var MAX_FLASH_INTENSITY = 20; // this controls how bright the lightning effect appears
    var MIN_FLASH_INTENSITY = 0;  // this is probably best at 0, but it could be higher, which will make the lightning not fade completely to darkness before going away.
    var MAX_FLASH_INTENSITY_RANDOMNESS = 10; // this will add some randomness to the max brightness of the lightning
    var FLASH_INTENSITY_STEP_RANDOMNESS = 2; // as the lightning goes from min to max back to min, this will make it more random in it's brightness
    var AVERAGE_LIGHTNING_STRIKE_GAP_IN_SECONDS = 120; // how long on average between lighting
    var EXTRA_RANDOM_RANGE_LIGHTNING_STRIKE_GAP_IN_SECONDS = 10; // some randomness to the lightning gap
    var THUNDER_SOUND_URL = "https://s3.amazonaws.com/hifi-public/brad/rainstorm/thunder-48k.wav"; // thunder sound effect, must be 48k 16bit PCM
    var THUNDER_VOLUME = 1; // adjust the volume of the thunder sound effect
    var RAIN_SOUND_URL = "https://s3.amazonaws.com/hifi-public/brad/rainstorm/rain.wav"; // the background rain, this will loop
    var RAIN_VOLUME = 0.05; // adjust the volume of the rain sound effect.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Various constants and variables we need access to in all scopes
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    var MSECS_PER_SECOND = 1000; // number of milliseconds in a second, a universal truth, don't change this. :)

    // This is a little trick for implementing entity scripts. Many of the entity callbacks will have the JavaScript
    // "this" variable set, but if you connect to a signal like update, "this" won't correctly point to the instance
    // of your entity script, and so we set "_this" and use it in cases we need to access "this". We need to define
    // the variable in this scope so that it's not share among all entities.

    var _this; // this is important here... or else the _this will be globally scoped.


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Various helper functions...
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Helper function for returning either a value, or the default value if the value is undefined. This is
    // is handing in parsing JSON where you don't know if the values have been set or not.
    function valueOrDefault(value, defaultValue) {
        if (value !== undefined) {
            return value;
        }
        return defaultValue;
    }

    // return a random float between high and low
    function randFloat(low, high) {
        return low + Math.random() * (high - low);
    }

    // the "constructor" for our class. pretty simple, it just sets our _this, so we can access it later.
    function Lightning() {
        _this = this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The class prototype
    //
    // This is the class definition/prototype for our class. Funtions declared here will be accessible
    // via the instance of the entity
    //
    Lightning.prototype = {

        // preload()
        //    This is called by every viewer/interface instance that "sees" the entity you've attached this script to.
        //    If this is a zone entity, then it will surely be called whenever someone enters the zone.
        preload: function (entityID) {

            // we always make a point of remember our entityID, so that we can access our entity later
            _this.entityID = entityID;

            // set up some of our time related state
            var now = Date.now();
            _this.lastUpdate = now;
            _this.lastStrike = now;

            // some of our other state related items
            _this.lightningID = false; // this will be the entityID for any lightning that we create
            _this.lightningActive = false; // are we actively managing lightning

            // Get the entities userData property, to see if someone has overridden any of our default settings
            var userDataText = Entities.getEntityProperties(entityID, ["userData"]).userData;
            var userData = {};
            if (userDataText !== "") {
                userData = JSON.parse(userDataText);
            }
            var lightningUserData = valueOrDefault(userData.lightning, {});
            _this.flashIntensityStepRandomeness = valueOrDefault(lightningUserData.flashIntensityStepRandomness, FLASH_INTENSITY_STEP_RANDOMNESS);
            _this.flashMax = valueOrDefault(lightningUserData.flashMax, MAX_FLASH_INTENSITY);
            _this.flashMin = valueOrDefault(lightningUserData.flashMin, MIN_FLASH_INTENSITY);
            _this.flashMaxRandomness = valueOrDefault(lightningUserData.flashMaxRandomness, MAX_FLASH_INTENSITY_RANDOMNESS);
            _this.averageLightningStrikeGap = valueOrDefault(lightningUserData.averageLightningStrikeGap, AVERAGE_LIGHTNING_STRIKE_GAP_IN_SECONDS);
            _this.extraRandomRangeLightningStrikeGap = valueOrDefault(lightningUserData.extraRandomRangeLightningStrikeGap, EXTRA_RANDOM_RANGE_LIGHTNING_STRIKE_GAP_IN_SECONDS);

            var thunderURL = valueOrDefault(lightningUserData.thunderURL, THUNDER_SOUND_URL);
            _this.thunderSound = SoundCache.getSound(thunderURL); // start downloading the thunder into the cache in case we need it later
            _this.thunderVolume = valueOrDefault(lightningUserData.thunderVolume, THUNDER_VOLUME);

            var rainURL = valueOrDefault(lightningUserData.rainURL, RAIN_SOUND_URL);
            _this.rainSound = SoundCache.getSound(rainURL); // start downloading the rain, we will be using it for sure
            _this.rainVolume = valueOrDefault(lightningUserData.rainVolume, RAIN_VOLUME);
            _this.rainPlaying = false;

            Script.update.connect(_this.onUpdate); // connect our onUpdate to a regular update signal from the interface
        },

        // unload()
        //    This is called by every viewer/interface instance that "sees" the entity when you "stop knowing about" the
        //    entity. Usually this means the user has left then domain, or maybe the entity has been deleted. We need to
        //    clean up anything transient that we created here. In this case it means disconnecting from the update signal
        //    and stopping the rain sound if we started it.
        unload: function ( /*entityID*/ ) {
            Script.update.disconnect(_this.onUpdate);
            if (_this.rainInjector !== undefined) {
                _this.rainInjector.stop();
            }
        },

        // plays the rain sound effect. This is played locally, which means it doesn't provide spatialization, which
        // we don't really want for the ambient sound effect of rain. Also, since it's playing locally, we don't send
        // the stream to the audio mixer. Another subtle side effect of playing locally is that the sound isn't in sync
        // for all users in the domain, but that's ok for a sound effect like rain which is more of a white noise like
        // sound effect.
        playLocalRain: function () {
            var myPosition = Entities.getEntityProperties(_this.entityID, "position").position;
            _this.rainInjector = Audio.playSound(_this.rainSound, {
                position: myPosition,
                volume: _this.rainVolume,
                loop: true,
                localOnly: true
            });
            _this.rainPlaying = true;
        },

        // this handles a single "step" of the lightning flash effect. It assumes a light entity has already been created,
        // and all it really does is change the intensity of the light based on the settings that are part of this entity's
        // userData.
        flashLightning: function (lightningID) {
            var lightningProperties = Entities.getEntityProperties(lightningID, ["userData", "intensity"]);
            var lightningParameters = JSON.parse(lightningProperties.userData);
            var currentIntensity = lightningProperties.intensity;
            var flashDirection = lightningParameters.flashDirection;
            var flashMax = lightningParameters.flashMax;
            var flashMin = lightningParameters.flashMin;
            var flashIntensityStepRandomeness = lightningParameters.flashIntensityStepRandomeness;
            var newIntensity = currentIntensity + flashDirection + randFloat(-flashIntensityStepRandomeness, flashIntensityStepRandomeness);

            if (flashDirection > 0) {
                if (newIntensity >= flashMax) {
                    flashDirection = -1; // reverse flash
                    newIntensity = flashMax;
                }
            } else {
                if (newIntensity <= flashMin) {
                    flashDirection = 1; // reverse flash
                    newIntensity = flashMin;
                }
            }

            // if we reached 0 intensity, then we're done with this strike...
            if (newIntensity === 0) {
                _this.lightningActive = false;
                Entities.deleteEntity(lightningID);
            }

            // FIXME - we probably don't need to re-edit the userData of the light... we're only
            // changing direction, the rest are the same... we could just store direction in our
            // own local variable state
            var newLightningParameters = JSON.stringify({
                flashDirection: flashDirection,
                flashIntensityStepRandomeness: flashIntensityStepRandomeness,
                flashMax: flashMax,
                flashMin: flashMin
            });

            // this is what actually creates the effect, changing the intensity of the light
            Entities.editEntity(lightningID, {intensity: newIntensity, userData: newLightningParameters});
        },

        // findMyLightning() is designed to make the script more robust. Since we're an open editable platform
        // it's possible that from the time that we started the lightning effect until "now" when we're attempting
        // to change the light, some other client might have deleted our light. Before we proceed in editing
        // the light, we check to see if it exists.
        findMyLightning: function () {
            if (_this.lightningID !== false) {
                var lightningName = Entities.getEntityProperties(_this.lightningID, "name").name;
                if (lightningName !== undefined) {
                    return _this.lightningID;
                }
            }
            return false;
        },

        // findOtherLightning() is designed to allow this script to work in a "multi-user" environment, which we
        // must assume we are in. Since every user/viewer/client that connect to the domain and "sees" our entity
        // is going to run this script, any of them could be in charge of flashing the lightning. So before we
        // start to flash the lightning, we will first check to see if someone else already is.
        //
        // returns true if some other lightning exists... likely because some other viewer is flashing it
        // returns false if no other lightning exists...
        findOtherLightning: function () {
            var myPosition = Entities.getEntityProperties(_this.entityID, "position").position;

            // find all entities near me...
            var entities = Entities.findEntities(myPosition, 1000);
            var checkEntityID;
            var checkProperties;
            var entity;
            for (entity = 0; entity < entities.length; entity++) {
                checkEntityID = entities[entity];
                checkProperties = Entities.getEntityProperties(checkEntityID, ["name", "type"]);

                // check to see if they are lightning
                if (checkProperties.type === "Light" && checkProperties.name === "lightning for creator:" + _this.entityID) {
                    return true;
                }
            }
            return false;
        },

        // createNewLightning() actually creates new lightning and plays the thunder sound
        createNewLightning: function () {
            var myPosition = Entities.getEntityProperties(_this.entityID, "position").position;
            _this.lightningID = Entities.addEntity({
                type: "Light",
                name: "lightning for creator:" + _this.entityID,
                userData: JSON.stringify({
                    flashDirection: 1,
                    flashIntensityStepRandomeness: _this.flashIntensityStepRandomeness,
                    flashMax: _this.flashMax + randFloat(-_this.flashMaxRandomness, _this.flashMaxRandomness),
                    flashMin: _this.flashMin
                }),
                falloffRadius: 1000,
                intensity: 0,
                position: myPosition,
                collisionless: true,
                dimensions: {x: 1000, y: 1000, z: 1000},
                color: {red: 255, green: 255, blue: 255},
                lifetime: 10 // lightning only lasts 10 seconds....
            });

            // play the thunder...
            Audio.playSound(_this.thunderSound, {
                position: myPosition,
                volume: _this.thunderVolume
            });

            return _this.lightningID;
        },

        // onUpdate() this will be called regularly, approximately every frame of the simulation. We will use
        // it to determine if we need to do a lightning/thunder strike
        onUpdate: function () {
            var now = Date.now();

            // check to see if rain is downloaded and not yet playing, if so start playing
            if (!_this.rainPlaying && _this.rainSound.downloaded) {
                _this.playLocalRain();
            }

            // NOTE: _this.lightningActive will only be TRUE if we are the one who created
            //       the lightning and we are in charge of flashing it...
            if (_this.lightningActive) {
                var lightningID = _this.findMyLightning();
                // if for some reason our lightning is gone... then just return to non-active state
                if (lightningID === false) {
                    _this.lightningActive = false;
                    _this.lightningID = false;
                } else {
                    // otherwise, flash our lightning...
                    _this.flashLightning(lightningID);
                }
            } else {
                // whether or not it's time for us to strike, we always keep an eye out for anyone else
                // striking... and if we see someone else striking, we will reset our lastStrike time
                if (_this.findOtherLightning()) {
                    _this.lastStrike = now;
                }

                var sinceLastStrike = now - _this.lastStrike;
                var nextRandomStrikeTime = _this.averageLightningStrikeGap
                        + randFloat(-_this.extraRandomRangeLightningStrikeGap,
                        _this.extraRandomRangeLightningStrikeGap);

                if (sinceLastStrike > nextRandomStrikeTime * MSECS_PER_SECOND) {

                    // so it's time for a strike... let's see if someone else has lightning...
                    // if no one else is flashing lightning... then we create it...
                    if (_this.findOtherLightning()) {
                        _this.lightningActive = false;
                        _this.lightningID = false;
                    } else {
                        _this.createNewLightning();
                        _this.lightningActive = true;
                    }

                    _this.lastStrike = now;
                }
            }
        }
    };

    return new Lightning();
});