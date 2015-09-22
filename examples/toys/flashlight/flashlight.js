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

(function() {

    Script.include("../../libraries/utils.js");

    var _this;

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    Flashlight = function() {
        _this = this;
    };

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
    }

    // Evaluate the world light entity positions and orientations from the model ones
    function evalLightWorldTransform(modelPos, modelRot) {

        return {
            p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, MODEL_LIGHT_POSITION)),
            q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
        };
    };

    function glowLightWorldTransform(modelPos, modelRot) {
        return {
            p: Vec3.sum(modelPos, Vec3.multiplyQbyV(modelRot, GLOW_LIGHT_POSITION)),
            q: Quat.multiply(modelRot, MODEL_LIGHT_ROTATION)
        };
    };


    Flashlight.prototype = {
        lightOn: false,
        hand: null,
        whichHand: null,
        hasSpotlight: false,
        spotlight: null,
        setRightHand: function() {
            this.hand = 'RIGHT';
        },
        setLeftHand: function() {
            this.hand = 'LEFT';
        },
        startNearGrab: function() {
            if (!_this.hasSpotlight) {

                //this light casts the beam
                this.spotlight = Entities.addEntity({
                    type: "Light",
                    isSpotlight: true,
                    dimensions: {
                        x: 2,
                        y: 2,
                        z: 20
                    },
                    color: {
                        red: 255,
                        green: 255,
                        blue: 255
                    },
                    intensity: 2,
                    exponent: 0.3,
                    cutoff: 20
                });

                //this light creates the effect of a bulb at the end of the flashlight
                this.glowLight = Entities.addEntity({
                    type: "Light",
                    dimensions: {
                        x: 0.25,
                        y: 0.25,
                        z: 0.25
                    },
                    isSpotlight: false,
                    color: {
                        red: 255,
                        green: 255,
                        blue: 255
                    },
                    exponent: 0,
                    cutoff: 90, // in degrees
                });

                this.debugBox = Entities.addEntity({
                    type: 'Box',
                    color: {
                        red: 255,
                        blue: 0,
                        green: 0
                    },
                    dimensions: {
                        x: 0.25,
                        y: 0.25,
                        z: 0.25
                    },
                    ignoreForCollisions:true
                })
                
                this.hasSpotlight = true;

            }

        },
        setWhichHand: function() {
            this.whichHand = this.hand;
        },
        continueNearGrab: function() {
            if (this.whichHand === null) {
                //only set the active hand once -- if we always read the current hand, our 'holding' hand will get overwritten
                this.setWhichHand();
            } else {
                this.updateLightPositions();
                this.changeLightWithTriggerPressure(this.whichHand);
            }
        },
        releaseGrab: function() {
            //delete the lights and reset state
            if (this.hasSpotlight) {
                Entities.deleteEntity(this.spotlight);
                Entities.deleteEntity(this.glowLight);
                this.hasSpotlight = false;
                this.glowLight = null;
                this.spotlight = null;
                this.whichHand = null;

            }
        },
        updateLightPositions: function() {
            var modelProperties = Entities.getEntityProperties(this.entityID, ['position', 'rotation']);

            //move the two lights along the vectors we set above
            var lightTransform = evalLightWorldTransform(modelProperties.position, modelProperties.rotation);
            var glowLightTransform = glowLightWorldTransform(modelProperties.position, modelProperties.rotation);


            //move them with the entity model
            Entities.editEntity(this.spotlight, {
                position: lightTransform.p,
                rotation: lightTransform.q,
            })


            Entities.editEntity(this.glowLight, {
                position: glowLightTransform.p,
                rotation: glowLightTransform.q,
            })

            // Entities.editEntity(this.debugBox, {
            //     position: lightTransform.p,
            //     rotation: lightTransform.q,
            // })

        },
        changeLightWithTriggerPressure: function(flashLightHand) {

            var handClickString = flashLightHand + "_HAND_CLICK";

            var handClick = Controller.findAction(handClickString);

            this.triggerValue = Controller.getActionValue(handClick);

            if (this.triggerValue < DISABLE_LIGHT_THRESHOLD && this.lightOn === true) {
                this.turnLightOff();
            } else if (this.triggerValue >= DISABLE_LIGHT_THRESHOLD && this.lightOn === false) {
                this.turnLightOn();
            }
            return
        },
        turnLightOff: function() {
            Entities.editEntity(this.spotlight, {
                intensity: 0
            });
            Entities.editEntity(this.glowLight, {
                intensity: 0
            });
            this.lightOn = false
        },
        turnLightOn: function() {
            Entities.editEntity(this.glowLight, {
                intensity: 2
            });
            Entities.editEntity(this.spotlight, {
                intensity: 2
            });
            this.lightOn = true
        },
        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        preload: function(entityID) {
            this.entityID = entityID;
        },

        // unload() will be called when our entity is no longer available. It may be because we were deleted,
        // or because we've left the domain or quit the application.
        unload: function(entityID) {

            if (this.hasSpotlight) {
                Entities.deleteEntity(this.spotlight);
                Entities.deleteEntity(this.glowLight);
                this.hasSpotlight = false;
                this.glowLight = null;
                this.spotlight = null;
                this.whichHand = null;
            }


        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Flashlight();
})