//
//  lightSwitchHall.js
//  examples/entityScripts
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;


    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    LightSwitchHall = function() {
        _this = this;

        this.lightStateKey = "lightStateKey";
        this.resetKey = "resetMe";

        this.switchSound = SoundCache.getSound("https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_2.wav");
    };

    LightSwitchHall.prototype = {

        clickReleaseOnEntity: function(entityId, mouseEvent) {
            if (!mouseEvent.isLeftButton) {
                return;
            }
            this.toggleLights();
        },

        startNearGrabNonColliding: function() {
            this.toggleLights();
        },

        toggleLights: function() {
            var defaultLightData = {
                on: false
            };
            var lightState = getEntityCustomData(this.lightStateKey, this.entityID, defaultLightData);
            if (lightState.on === true) {
                this.clearLights();
            } else if (lightState.on === false) {
                this.createLights();
            }

            // flip model to give illusion of light switch being flicked
            this.flipLights();

            Audio.playSound(this.switchSound, {
                volume: 0.5,
                position: this.position
            });

        },

        clearLights: function() {
            var entities = Entities.findEntities(MyAvatar.position, 100);
            var self = this;
            entities.forEach(function(entity) {
                var resetData = getEntityCustomData(self.resetKey, entity, {})
                if (resetData.resetMe === true && resetData.lightType === "Sconce Light Hall") {
                    Entities.deleteEntity(entity);
                }
            });

            setEntityCustomData(this.lightStateKey, this.entityID, {
                on: false
            });
        },

        createLights: function() {
            var sconceLight1 = Entities.addEntity({
                type: "Light",
                position: {
                    x: 543.75,
                    y: 496.24,
                    z: 511.13
                },
                name: "Sconce 1 Light",
                dimensions: {
                    x: 2.545,
                    y: 2.545,
                    z: 2.545
                },
                cutoff: 90,
                color: {
                    red: 217,
                    green: 146,
                    blue: 24
                }
            });

            setEntityCustomData(this.resetKey, sconceLight1, {
                resetMe: true,
                lightType: "Sconce Light Hall"
            });

            var sconceLight2 = Entities.addEntity({
                type: "Light",
                position: {
                    x: 540.1,
                    y: 496.24,
                    z: 505.57
                },
                name: "Sconce 2 Light",
                dimensions: {
                    x: 2.545,
                    y: 2.545,
                    z: 2.545
                },
                cutoff: 90,
                color: {
                    red: 217,
                    green: 146,
                    blue: 24
                }
            });

            setEntityCustomData(this.resetKey, sconceLight2, {
                resetMe: true,
                lightType: "Sconce Light Hall"
            });

            setEntityCustomData(this.lightStateKey, this.entityID, {
                on: true
            });

        },

        flipLights: function() {
            // flip model to give illusion of light switch being flicked
            var rotation = Entities.getEntityProperties(this.entityID, "rotation").rotation;
            var axis = {
                x: 0,
                y: 1,
                z: 0
            };
            var dQ = Quat.angleAxis(180, axis);
            rotation = Quat.multiply(rotation, dQ);


            Entities.editEntity(this.entityID, {
                rotation: rotation
            });

        },

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
            preload: function(entityID) {
            this.entityID = entityID;

            //The light switch is static, so just cache its position once
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
            var defaultLightData = {
                on: false
            };
            var lightState = getEntityCustomData(this.lightStateKey, this.entityID, defaultLightData);

            //If light is off, then we create two new lights- at the position of the sconces
            if (lightState.on === false) {
                this.createLights();
                this.flipLights();

            }
            //If lights are on, do nothing!
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new LightSwitchHall();
})
