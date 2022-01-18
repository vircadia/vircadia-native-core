//
//
//  Created by James B. Pollack @imgntn on 10/26/2015
//  Copyright 2015 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {

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
            var ids = Entities.findEntities(this.initialProperties.position, 75);
            var i;
            for (i = 0; i < ids.length; i++) {
                var id = ids[i];
                var properties = Entities.getEntityProperties(id, "name");
                if (properties.name === "Hifi-Basketball") {
                    Entities.deleteEntity(id);
                }
            }

            this.createBasketballs();
        },

        createBasketballs: function() {
            var NUMBER_OF_BALLS = 4;
            var DIAMETER = 0.30;
            var basketballURL ="https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball2.fbx";
            var basketballCollisionSoundURL ="https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball.wav";

            var position = {
                x: 542.86,
                y: 494.84,
                z: 475.06
            };
            var collidingBalls = [];

            var i;
            for (i = 0; i < NUMBER_OF_BALLS; i++) {
                var ballPosition = {
                    x: position.x,
                    y: position.y + DIAMETER * 2,
                    z: position.z + (DIAMETER) - (DIAMETER * i)
                };
                var newPosition = {
                    x: position.x + (DIAMETER * 2) - (DIAMETER * i),
                    y: position.y + DIAMETER * 2,
                    z: position.z
                };
                var collidingBall = Entities.addEntity({
                    type: "Model",
                    name: 'Hifi-Basketball',
                    shapeType: 'Sphere',
                    position: newPosition,
                    dimensions: {
                        x: DIAMETER,
                        y: DIAMETER,
                        z: DIAMETER
                    },
                    restitution: 1.0,
                    damping: 0.00001,
                    gravity: {
                        x: 0,
                        y: -9.8,
                        z: 0
                    },
                    dynamic: true,
                    collisionSoundURL: basketballCollisionSoundURL,
                    collisionless: false,
                    modelURL: basketballURL,
                    userData: JSON.stringify({
                        resetMe: {
                            resetMe: true
                        },
                        grabbableKey: {
                            invertSolidWhileHeld: true
                        }
                    })
                });

                collidingBalls.push(collidingBall);

            }
        },

        preload: function(entityID) {
            this.initialProperties = Entities.getEntityProperties(entityID);
            this.entityID = entityID;
        },

    };

    return new Resetter();
});
