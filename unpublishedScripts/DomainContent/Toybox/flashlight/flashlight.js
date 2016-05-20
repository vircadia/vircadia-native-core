//
//  flashlight.js
//
//  Script Type: Entity
//
//  Created by Sam Gateau on 9/9/15.
//  Additions by James B. Pollack @imgntn on 9/21/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This is a toy script that can be added to the Flashlight model entity:
//  "https://hifi-public.s3.amazonaws.com/models/props/flashlight.fbx"
//  that creates a spotlight attached with the flashlight model while the entity is grabbed
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */
(function() {

    Script.include("../libraries/utils.js");

    var ON_SOUND_URL = 'http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/flashlight/flashlight_on.wav';
    var OFF_SOUND_URL = 'http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/flashlight/flashlight_off.wav';

    //we are creating lights that we don't want to get stranded so lets make sure that we can get rid of them
    var startTime = Date.now();
    //if you're going to be using this in a dungeon or something and holding it for a long time, increase this lifetime value.
    var LIFETIME = 100;
    var MSEC_PER_SEC = 1000.0;

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    function Flashlight() {
        return;
    }

    //if the trigger value goes below this while held, the flashlight will turn off.  if it goes above, it will 
    var DISABLE_LIGHT_THRESHOLD = 0.7;

    // These constants define the Spotlight position and orientation relative to the model 
    var MODEL_LIGHT_POSITION = {
        x: 0,
        y: -0.3,
        z: 0
    };
    var MODEL_LIGHT_ROTATION = Quat.angleAxis(-90, {
        x: 1,
        y: 0,
        z: 0
    });

    var GLOW_LIGHT_POSITION = {
        x: 0,
        y: -0.1,
        z: 0
    };

    // Evaluate the world light entity positions and orientations from the model ones
    function evalLightWorldTransform(modelPos, modelRot) {

        return {
            p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
            q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
        };
    }

    function glowLightWorldTransform(modelPos, modelRot) {
        return {
            p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, GLOW_LIGHT_POSITION)),
            q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
        };
    }

    Flashlight.prototype = {
        lightOn: false,
        hand: null,
        whichHand: null,
        hasSpotlight: false,
        spotlight: null,

        startNearGrab: function(entityID, args) {
            this.hand = args[0];

            print("FLASHLIGHT startNearGrab");
            if (!this.hasSpotlight) {

                var modelProperties = Entities.getEntityProperties(this.entityID, ['position', 'rotation']);
                var lightTransform = evalLightWorldTransform(modelProperties.position, modelProperties.rotation);
                var glowLightTransform = glowLightWorldTransform(modelProperties.position, modelProperties.rotation);


                //this light casts the beam
                this.spotlight = Entities.addEntity({
                    type: "Light",
                    isSpotlight: true,
                    dimensions: {
                        x: 2,
                        y: 2,
                        z: 20
                    },
                    parentID: this.entityID,
                    color: {
                        red: 255,
                        green: 255,
                        blue: 255
                    },
                    intensity:  1,
                    falloffRadius:0.9,
                    exponent: 0.5,
                    cutoff: 20,
                    lifetime: LIFETIME,
                    position: lightTransform.p,
                    rotation: lightTransform.q,
                });

                //this light creates the effect of a bulb at the end of the flashlight
                this.glowLight = Entities.addEntity({
                    type: "Light",
                    dimensions: {
                        x: 0.25,
                        y: 0.25,
                        z: 0.25
                    },
                    parentID: this.entityID,
                    isSpotlight: false,
                    color: {
                        red: 255,
                        green: 255,
                        blue: 255
                    },
                    exponent: 0,
                    intensity:1.0,
                    falloffRadius:0.3,
                    lifetime: LIFETIME,
                    cutoff: 90, // in degrees
                    position: glowLightTransform.p,
                    rotation: glowLightTransform.q,

                });

                this.hasSpotlight = true;

            }

        },
        startEquip: function(id, params) {
            this.startNearGrab(id, params);
        },

        setWhichHand: function() {
            this.whichHand = this.hand;
        },

        continueNearGrab: function() {
            if (this.whichHand === null) {
                //only set the active hand once -- if we always read the current hand, our 'holding' hand will get overwritten
                this.setWhichHand();
            } else {
                this.changeLightWithTriggerPressure(this.whichHand);
            }
        },
        continueEquip: function(entityID, args) {
            this.continueNearGrab(entityID, args);
        },

        releaseGrab: function(entityID, args) {
            //delete the lights and reset state
            if (this.hasSpotlight) {
                Entities.deleteEntity(this.spotlight);
                Entities.deleteEntity(this.glowLight);
                this.hasSpotlight = false;
                this.glowLight = null;
                this.spotlight = null;
                this.whichHand = null;
                this.lightOn = false;
            }
        },
        releaseEquip: function(entityID, args) {
            this.releaseGrab(entityID, args);
        },
        
        changeLightWithTriggerPressure: function(flashLightHand) {

            if (flashLightHand === 'LEFT') {
                this.triggerValue = Controller.getValue(Controller.Standard.LT);
            }
            if (flashLightHand === 'RIGHT') {
                this.triggerValue = Controller.getValue(Controller.Standard.RT);

            }

            if (this.triggerValue < DISABLE_LIGHT_THRESHOLD && this.lightOn === true) {
                this.turnLightOff();
            } else if (this.triggerValue >= DISABLE_LIGHT_THRESHOLD && this.lightOn === false) {
                this.turnLightOn();
            }
            return;
        },

        turnLightOff: function() {
            this.playSoundAtCurrentPosition(false);
            Entities.editEntity(this.spotlight, {
                intensity: 0
            });
            Entities.editEntity(this.glowLight, {
                intensity: 0
            });
            this.lightOn = false;
        },

        turnLightOn: function() {
            this.playSoundAtCurrentPosition(true);

            Entities.editEntity(this.glowLight, {
                intensity: 2
            });
            Entities.editEntity(this.spotlight, {
                intensity: 2
            });
            this.lightOn = true;
        },

        playSoundAtCurrentPosition: function(playOnSound) {
            var position = Entities.getEntityProperties(this.entityID, "position").position;

            var audioProperties = {
                volume: 0.25,
                position: position
            };

            if (playOnSound) {
                Audio.playSound(this.ON_SOUND, audioProperties);
            } else {
                Audio.playSound(this.OFF_SOUND, audioProperties);
            }
        },

        preload: function(entityID) {

            //  preload() will be called when the entity has become visible (or known) to the interface
            //  it gives us a chance to set our local JavaScript object up. In this case it means:
            //  * remembering our entityID, so we can access it in cases where we're called without an entityID
            //  * preloading sounds
            this.entityID = entityID;
            this.ON_SOUND = SoundCache.getSound(ON_SOUND_URL);
            this.OFF_SOUND = SoundCache.getSound(OFF_SOUND_URL);

        },

        unload: function() {
            // unload() will be called when our entity is no longer available. It may be because we were deleted,
            // or because we've left the domain or quit the application.
            if (this.hasSpotlight) {
                Entities.deleteEntity(this.spotlight);
                Entities.deleteEntity(this.glowLight);
                this.hasSpotlight = false;
                this.glowLight = null;
                this.spotlight = null;
                this.whichHand = null;
                this.lightOn = false;
            }

        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Flashlight();
});
