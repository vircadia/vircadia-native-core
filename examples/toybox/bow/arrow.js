//
//  arrow.js 
//
//  This script attaches to an arrow to make it stop and stick when it hits something.  Could use this to make it a fire arrow or really give any kind of property to itself or the entity it hits.
//  
//  Created by James B. Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    Script.include("../../libraries/utils.js");

    var NOTCH_DETECTOR_SEARCH_RADIUS = 0.25;
    var FIRE_DETECTOR_SEARCH_RADIUS = 0.25;

    var _this;

    function Arrow() {
        _this = this;
        return;
    }

    Arrow.prototype = {
        stickOnCollision: false,
        notched: false,
        isBurning: false,
        fire: null,
        preload: function(entityID) {
            this.entityID = entityID;
        },

        releaseGrab: function() {
            if (this.fire !== null) {
                this.fire = null;
                this.isBurning = false;
                Entities.deleteEntity(this.fire);
            }
        },

        continueNearGrab: function() {
            this.currentProperties = Entities.getEntityProperties(this.entityID, "position");

            if (this.isBurning !== true) {
                this.searchForFires();
            } else {
                this.updateFirePosition();
            }

            if (this.notched !== true) {
                this.searchForNotchDetectors();
            }
        },

        searchForNotchDetectors: function() {
            if (this.notched === true) {
                return
            };

            var ids = Entities.findEntities(this.currentProperties.position, NOTCH_DETECTOR_SEARCH_RADIUS);
            var i, properties;
            for (i = 0; i < ids.length; i++) {
                id = ids[i];
                properties = Entities.getEntityProperties(id, 'name');
                if (properties.name === "Hifi-NotchDetector") {
                    print('NEAR THE NOTCH!!!')
                    this.notched = true;
                    this.tellBowArrowIsNotched(this.getBowID(id));
                }
            }

        },

        searchForFires: function() {
            print('SEARCHING FOR FIRES!')
            if (this.notched === true) {
                return
            };
            if (this.isBurning === true) {
                return
            };

            var ids = Entities.findEntities(this.currentProperties.position, FIRE_DETECTOR_SEARCH_RADIUS);
            var i, properties;
            for (i = 0; i < ids.length; i++) {
                id = ids[i];
                properties = Entities.getEntityProperties(id, 'name');
                if (properties.name === "Hifi-Arrow-Fire-Source") {
                    print('NEAR A FIRE SOURCE!!!');
                    this.isBurning = true;
                    this.createFireParticleSystem();


                }
            }

        },
        updateFirePosition: function() {
            print('updating fire position' + this.fire)
            Entities.editEntity(this.fire, {
                position: this.currentProperties.position
            })
        },
        createFireParticleSystem: function() {
            print('CREATING FIRE PARTICLE SYSTEM')

            var myOrientation = Quat.fromPitchYawRollDegrees(-90, 0, 0.0);

            var animationSettings = JSON.stringify({
                fps: 30,
                running: true,
                loop: true,
                firstFrame: 1,
                lastFrame: 10000
            });

            this.fire = Entities.addEntity({
                type: "ParticleEffect",
                name: "Hifi-Arrow-Fire-Source",
                animationSettings: animationSettings,
                textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
                emitRate: 100,
                position: this.currentProperties.position,
                colorStart: {
                    red: 70,
                    green: 70,
                    blue: 137
                },
                color: {
                    red: 200,
                    green: 99,
                    blue: 42
                },
                colorFinish: {
                    red: 255,
                    green: 99,
                    blue: 32
                },
                radiusSpread: 0.01,
                radiusStart: 0.02,
                radiusEnd: 0.001,
                particleRadius: 0.15,
                radiusFinish: 0.0,
                emitOrientation: myOrientation,
                emitSpeed: 0.3,
                speedSpread: 0.1,
                alphaStart: 0.05,
                alpha: 0.1,
                alphaFinish: 0.05,
                emitDimensions: {
                    x: 1,
                    y: 1,
                    z: 0.1
                },
                polarFinish: 0.1,
                emitAcceleration: {
                    x: 0.0,
                    y: 0.0,
                    z: 0.0
                },
                accelerationSpread: {
                    x: 0.1,
                    y: 0.01,
                    z: 0.1
                },
                lifespan: 1
            });
        },
        getBowID: function(notchDetectorID) {
            var properties = Entities.getEntityProperties(notchDetectorID, "userData");
            var userData = JSON.parse(properties.userData);
            if (userData.hasOwnProperty('hifiBowKey')) {
                return userData.hifiBowKey.bowID;
            }
        },
        getActionID: function() {
            var properties = Entities.getEntityProperties(this.entityID, "userData");
            var userData = JSON.parse(properties.userData);
            if (userData.hasOwnProperty('hifiHoldActionKey')) {
                return userData.hifiHoldActionKey.holdActionID;
            }
        },

        disableGrab: function() {
            var actionID = this.getActionID();
            var success = Entities.deleteAction(this.entityID, actionID);
        },

        tellBowArrowIsNotched: function(bowID) {
            this.disableGrab();

            setEntityCustomData('grabbableKey', this.entityID, {
                grabbable: false,
                invertSolidWhileHeld: true
            });

            Entities.editEntity(this.entityID, {
                collisionsWillMove: false,
                ignoreForCollisions: true

            })

            setEntityCustomData('hifiBowKey', bowID, {
                hasArrowNotched: true,
                arrowIsBurning: this.isBurning,
                arrowID: this.entityID
            });

            if (this.isBurning === true) {
                this.isBurning = false;
                this.fire = null;
                Entities.deleteEntity(this.fire);
            }

        },

    }

    function deleteEntity(entityID) {
        if (entityID === _this.entityID) {
            if (_this.isBurning === true) {
                Entities.deleteEntity(_this.fire);
            }
        }
    }

    Entities.deletingEntity.connect(deleteEntity);

    return new Arrow;
})