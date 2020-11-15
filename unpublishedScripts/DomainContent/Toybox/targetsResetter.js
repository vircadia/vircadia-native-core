//
//
//  Created by James B. Pollack @imgntn on 10/26/2015
//  Copyright 2015 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {
    var targetsScriptURL = Script.resolvePath('ping_pong_gun/wallTarget.js');

    var _this;
    Resetter = function() {
        _this = this;
    };

    Resetter.prototype = {

        startNearTrigger: function() {
            this.resetObjects();
        },

        clickReleaseOnEntity: function() {
            this.resetObjects();
        },

        resetObjects: function() {
            var ids = Entities.findEntities(this.initialProperties.position, 50);
            for (var i = 0; i < ids.length; i++) {
                var id = ids[i];
                var properties = Entities.getEntityProperties(id, "name");
                if (properties.name === "Hifi-Target") {
                    Entities.deleteEntity(id);
                }
            }
            this.createTargets();
        },

        preload: function(entityID) {
            this.initialProperties = Entities.getEntityProperties(entityID);
            this.entityID = entityID;
        },

        createTargets: function() {

            var MODEL_URL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/ping_pong_gun/target.fbx';
            var COLLISION_HULL_URL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/ping_pong_gun/target_collision_hull.obj';

            var MINIMUM_MOVE_LENGTH = 0.05;
            var RESET_DISTANCE = 0.5;
            var TARGET_USER_DATA_KEY = 'hifi-ping_pong_target';
            var NUMBER_OF_TARGETS = 6;
            var TARGETS_PER_ROW = 3;

            var TARGET_DIMENSIONS = {
                x: 0.06,
                y: 0.42,
                z: 0.42
            };

            var VERTICAL_SPACING = TARGET_DIMENSIONS.y + 0.5;
            var HORIZONTAL_SPACING = TARGET_DIMENSIONS.z + 0.5;


            var startPosition = {
                x: 548.68,
                y: 497.30,
                z: 509.74
            };

            var rotation = Quat.fromPitchYawRollDegrees(0, -55.25, 0);

            var targets = [];

            function addTargets() {
                var i;
                var row = -1;
                for (i = 0; i < NUMBER_OF_TARGETS; i++) {

                    if (i % TARGETS_PER_ROW === 0) {
                        row++;
                    }

                    var vHat = Quat.getFront(rotation);
                    var spacer = HORIZONTAL_SPACING * (i % TARGETS_PER_ROW) + (row * HORIZONTAL_SPACING / 2);
                    var multiplier = Vec3.multiply(spacer, vHat);
                    var position = Vec3.sum(startPosition, multiplier);
                    position.y = startPosition.y - (row * VERTICAL_SPACING);

                    var targetProperties = {
                        name: 'Hifi-Target',
                        type: 'Model',
                        modelURL: MODEL_URL,
                        shapeType: 'compound',
                        dynamic: true,
                        dimensions: TARGET_DIMENSIONS,
                        compoundShapeURL: COLLISION_HULL_URL,
                        position: position,
                        rotation: rotation,
                        script: targetsScriptURL,
                        userData: JSON.stringify({
                            originalPositionKey: {
                                originalPosition: position
                            },
                            resetMe: {
                                resetMe: true
                            },
                            grabbableKey: {
                                grabbable: false
                            }
                        })
                    };

                    var target = Entities.addEntity(targetProperties);
                    targets.push(target);

                }
            }

            addTargets();

        }

    };

    return new Resetter();
});
