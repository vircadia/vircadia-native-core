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

    var _this;

    function Arrow() {
        _this = this;
        return;
    }

    Arrow.prototype = {
        stickOnCollision: false,
        glow: false,
        glowBox: null,
        isBurning: false,
        notched: false,
        preload: function(entityID) {
            this.entityID = entityID;


            // this.isBurning = this.checkIfBurning();
            // if (this.isBurning === true || this.glow === true) {
            //     Script.update.connect(this.updateArrowProperties);
            // }
            // if (this.isBurning === true) {
            //     Script.update.connect(this.updateFirePosition);

            // }
            // if (this.glow === true && this.glowBow === null) {
            //     this.createGlowBox();
            //     Script.update.connect(this.updateGlowBoxPosition);
            // }
        },

        unload: function() {

            // Script.update.disconnect(this.updateArrowProperties);

            // if (this.isBurning) {
            //     Script.update.disconnect(this.updateFirePosition);

            // }
            // if (this.glowBox !== null) {
            //     Script.update.disconnect(this.updateGlowBoxPosition);
            // }

        },

        continueNearGrab: function() {
            this.currentProperties = Entities.getEntityProperties(this.entityID, "position");
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
                if (properties.name == "Hifi-NotchDetector") {
                    print('NEAR THE NOTCH!!!')
                    this.notched = true;
                    this.tellBowArrowIsNotched(this.getBowID(id));
                }
            }

        },
        getBowID: function(notchDetectorID) {
            var properties = Entities.getEntityProperties(notchDetectorID, "userData");
            var userData = JSON.parse(properties.userData);
            if (userData.hasOwnProperty('hifiBowKey')) {
                return userData.hifiBowKey.bowID;
            }
        },

        tellBowArrowIsNotched: function(bowID) {
            setEntityCustomData('releaseGrabKey', this.entityID, {
                shouldRelease: true,
                didRelease: false
            });

            setEntityCustomData('grabbableKey', this.entityID, {
                grabbable: false
            });

            setEntityCustomData('hifiBowKey', bowID, {
                hasArrowNotched: true,
                arrowID: this.entityID
            });
        },

        checkIfBurning: function() {
            var properties = Entities.getEntityProperties(this.entityID, "userData");
            var userData = JSON.parse(properties.userData);
            var fire = false;

            if (userData.hasOwnProperty('hifiFireArrowKey')) {
                this.fire = userData.hifiFireArrowKey.fire;
                return true

            } else {
                return false;
            }
        },

        createGlowBox: function() {
            var glowBowProperties = {
                name: 'Arrow Glow Box',
                type: 'Box',
                dimensions: {
                    x: 0.02,
                    y: 0.02,
                    z: 0.64
                },
                color: {
                    red: 255,
                    green: 0,
                    blue: 255
                },
            };

            _this.glowBox = Entities.addEntity(glowBowProperties);
        },
        updateArrowProperties: function() {
            _this.arrowProperties = Entities.getEntityProperties(_this.entityID, ["position", "rotation"]);
        },
        updateGlowBoxPosition: function() {
            //once parenting is available, just attach the glowbow to the arrow
            Entities.editEntity(_this.entityID, {
                position: _this.arrowProperties.position,
                rotation: _this.arrowProperties.rotation
            })
        },
        updateFirePosition: function() {
            //once parenting is available, just attach the glowbow to the arrow
            Entities.editEntity(_this.entityID, {
                position: _this.arrowProperties.position
            })
        },

        collisionWithEntity: function(me, otherEntity, collision) {

            if (this.stickOnCollision === true) {
                Vec3.print('penetration = ', collision.penetration);
                Vec3.print('collision contact point = ', collision.contactPoint);
                Entities.editEntity(this.entityID, {
                    velocity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    gravity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    collisionsWillMove: false

                })
            }

        }
    }

    function deleteEntity(entityID) {
        if (entityID === this.entityID) {
            if (_this.isBurning === true) {
                _this.deleteEntity(_this.fire);
            }
        }
    }

    Entities.deletingEntity.connect(deleteEntity);

    return new Arrow;
})