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
        notched: false,
        preload: function(entityID) {
            this.entityID = entityID;

        },

        unload: function() {},

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
                arrowID: this.entityID
            });

        },

        // collisionWithEntity: function(me, otherEntity, collision) {
        //     print('ARROW HAD COLLISION')
        //         //     if (this.stickOnCollision === true) {
        //         //         print('ARROW SHOULD STICK')
        //         //         Vec3.print('penetration = ', collision.penetration);
        //         //         Vec3.print('collision contact point = ', collision.contactPoint);
        //         //         Entities.editEntity(this.entityID, {
        //         //             velocity: {
        //         //                 x: 0,
        //         //                 y: 0,
        //         //                 z: 0
        //         //             },
        //         //             gravity: {
        //         //                 x: 0,
        //         //                 y: 0,
        //         //                 z: 0
        //         //             },
        //         //             collisionsWillMove: false

        //     //         })
        //     //     }

        // },
        playCollisionSound: function() {

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