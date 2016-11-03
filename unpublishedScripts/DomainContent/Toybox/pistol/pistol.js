//
//  pistol.js
//
//  Created by Eric Levin on11/11/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */


(function() {
    Script.include("../libraries/utils.js");

    var _this;
    var DISABLE_LASER_THRESHOLD = 0.2;
    var TRIGGER_CONTROLS = [
        Controller.Standard.LT,
        Controller.Standard.RT,
    ];
    var RELOAD_THRESHOLD = 0.95;

    Pistol = function() {
        _this = this;
        this.equipped = false;
        this.forceMultiplier = 1;
        this.laserLength = 100;

        this.fireSound = SoundCache.getSound("http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/pistol/GUN-SHOT2.raw");
        this.ricochetSound = SoundCache.getSound("http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/pistol/Ricochet.L.wav");
        this.playRichochetSoundChance = 0.1;
        this.fireVolume = 0.2;
        this.bulletForce = 10;
        this.showLaser = false;

        this.laserOffsets = {
            y: 0.095
        };
        this.firingOffsets = {
            z: 0.16
        }

    };

    Pistol.prototype = {
        canShoot: false,

        startEquip: function(id, params) {
            this.equipped = true;
            this.hand = params[0] == "left" ? 0 : 1;
        },

        continueEquip: function(id, params) {
            if (!this.equipped) {
                return;
            }
            this.updateProps();
            if (this.showLaser) {
                this.updateLaser();
            }
            this.toggleWithTriggerPressure();
        },

        updateProps: function() {
            var gunProps = Entities.getEntityProperties(this.entityID, ['position', 'rotation']);
            this.position = gunProps.position;
            this.rotation = gunProps.rotation;
            this.firingDirection = Quat.getFront(this.rotation);
            var upVec = Quat.getUp(this.rotation);
            this.barrelPoint = Vec3.sum(this.position, Vec3.multiply(upVec, this.laserOffsets.y));
            this.laserTip = Vec3.sum(this.barrelPoint, Vec3.multiply(this.firingDirection, this.laserLength));
            this.barrelPoint = Vec3.sum(this.barrelPoint, Vec3.multiply(this.firingDirection, this.firingOffsets.z))
            var pickRay = {
                origin: this.barrelPoint,
                direction: this.firingDirection
            };
        },
        toggleWithTriggerPressure: function() {
            this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[this.hand]);

            if (this.triggerValue < RELOAD_THRESHOLD) {
                this.canShoot = true;
            }
            if (this.canShoot === true && this.triggerValue === 1) {
                this.fire();
                this.canShoot = false;
            }

            if (this.triggerValue < DISABLE_LASER_THRESHOLD && this.showLaser === true) {
                this.showLaser = false;
                Overlays.editOverlay(this.laser, {
                    visible: false
                });
            } else if (this.triggerValue >= DISABLE_LASER_THRESHOLD && this.showLaser === false) {
                this.showLaser = true
                Overlays.editOverlay(this.laser, {
                    visible: true
                });
            }


        },
        updateLaser: function() {

            Overlays.editOverlay(this.laser, {
                start: this.barrelPoint,
                end: this.laserTip,
                alpha: 1
            });
        },

        releaseEquip: function(id, params) {
            this.hand = null;
            this.equipped = false;
            Overlays.editOverlay(this.laser, {
                visible: false
            });
        },

        triggerPress: function(hand, value) {
            if (this.hand === hand && value === 1) {
                //We are pulling trigger on the hand we have the gun in, so fire
                this.fire();
            }
        },

        fire: function() {

            Audio.playSound(this.fireSound, {
                position: this.barrelPoint,
                volume: this.fireVolume
            });

            var pickRay = {
                origin: this.barrelPoint,
                direction: this.firingDirection
            };
            this.createGunFireEffect(this.barrelPoint)
            var intersection = Entities.findRayIntersectionBlocking(pickRay, true);
            if (intersection.intersects) {
                this.createEntityHitEffect(intersection.intersection);
                if (Math.random() < this.playRichochetSoundChance) {
                    Script.setTimeout(function() {
                        Audio.playSound(_this.ricochetSound, {
                            position: intersection.intersection,
                            volume: _this.fireVolume
                        });
                    }, randFloat(10, 200));
                }
                if (intersection.properties.dynamic === 1) {
                    // Any dynaic entity can be shot
                    Entities.editEntity(intersection.entityID, {
                        velocity: Vec3.multiply(this.firingDirection, this.bulletForce)
                    });
                }
            }
        },

        unload: function() {
            Overlays.deleteOverlay(this.laser);
        },

        createEntityHitEffect: function(position) {
            var sparks = Entities.addEntity({
                type: "ParticleEffect",
                position: position,
                lifetime: 4,
                "name": "Sparks Emitter",
                "color": {
                    red: 228,
                    green: 128,
                    blue: 12
                },
                "maxParticles": 1000,
                "lifespan": 0.15,
                "emitRate": 1000,
                "emitSpeed": 1,
                "speedSpread": 0,
                "emitOrientation": {
                    "x": -0.4,
                    "y": 1,
                    "z": -0.2,
                    "w": 0.7071068286895752
                },
                "emitDimensions": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "polarStart": 0,
                "polarFinish": Math.PI,
                "azimuthStart": -3.1415927410125732,
                "azimuthFinish": 2,
                "emitAcceleration": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "accelerationSpread": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "particleRadius": 0.03,
                "radiusSpread": 0.02,
                "radiusStart": 0.02,
                "radiusFinish": 0.03,
                "colorSpread": {
                    red: 100,
                    green: 100,
                    blue: 20
                },
                "alpha": 1,
                "alphaSpread": 0,
                "alphaStart": 0,
                "alphaFinish": 0,
                "additiveBlending": true,
                "textures": "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/pistol/star.png"
            });

            Script.setTimeout(function() {
                Entities.editEntity(sparks, {
                    isEmitting: false
                });
            }, 100);

        },

        createGunFireEffect: function(position) {
            var smoke = Entities.addEntity({
                type: "ParticleEffect",
                position: position,
                lifetime: 1,
                "name": "Smoke Hit Emitter",
                "maxParticles": 1000,
                "lifespan": 4,
                "emitRate": 20,
                emitSpeed: 0,
                "speedSpread": 0,
                "emitDimensions": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "polarStart": 0,
                "polarFinish": 0,
                "azimuthStart": -3.1415927410125732,
                "azimuthFinish": 3.14,
                "emitAcceleration": {
                    "x": 0,
                    "y": 0.5,
                    "z": 0
                },
                "accelerationSpread": {
                    "x": 0.2,
                    "y": 0,
                    "z": 0.2
                },
                "radiusSpread": 0.04,
                "particleRadius": 0.07,
                "radiusStart": 0.07,
                "radiusFinish": 0.07,
                "alpha": 0.7,
                "alphaSpread": 0,
                "alphaStart": 0,
                "alphaFinish": 0,
                "additiveBlending": 0,
                "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png"
            });
            Script.setTimeout(function() {
                Entities.editEntity(smoke, {
                    isEmitting: false
                });
            }, 100);

            var flash = Entities.addEntity({
                type: "ParticleEffect",
                position: this.barrelPoint,
                "name": "Muzzle Flash",
                lifetime: 4,
                parentID: this.entityID,
                "color": {
                    red: 228,
                    green: 128,
                    blue: 12
                },
                "maxParticles": 1000,
                "lifespan": 0.1,
                "emitRate": 1000,
                "emitSpeed": 0.5,
                "speedSpread": 0,
                "emitOrientation": {
                    "x": -0.4,
                    "y": 1,
                    "z": -0.2,
                    "w": 0.7071068286895752
                },
                "emitDimensions": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "polarStart": 0,
                "polarFinish": Math.PI,
                "azimuthStart": -3.1415927410125732,
                "azimuthFinish": 2,
                "emitAcceleration": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "accelerationSpread": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                },
                "particleRadius": 0.05,
                "radiusSpread": 0.01,
                "radiusStart": 0.05,
                "radiusFinish": 0.05,
                "colorSpread": {
                    red: 100,
                    green: 100,
                    blue: 20
                },
                "alpha": 1,
                "alphaSpread": 0,
                "alphaStart": 0,
                "alphaFinish": 0,
                "additiveBlending": true,
                "textures": "http://ericrius1.github.io/PartiArt/assets/star.png"
            });
            Script.setTimeout(function() {
                Entities.editEntity(flash, {
                    isEmitting: false
                });
            }, 100)

        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.laser = Overlays.addOverlay("line3d", {
                start:  { x: 0, y: 0, z: 0 },
                end:  { x: 0, y: 0, z: 0 },
                color: COLORS.RED,
                alpha: 1,
                visible: true,
                lineWidth: 2
            });
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Pistol();
});
