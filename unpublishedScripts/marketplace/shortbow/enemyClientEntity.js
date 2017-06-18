//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals utils */

(function() {
    Script.include('utils.js');

    function Enemy() {
    }
    Enemy.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;

            // To avoid sending extraneous messages and checking entities that we've already
            // seen, we keep track of which entities we've collided with previously.
            this.entityIDsThatHaveCollidedWithMe = [];

            Script.addEventHandler(entityID, "collisionWithEntity", this.onCollide.bind(this));

            var userData = Entities.getEntityProperties(this.entityID, 'userData').userData;
            var data = utils.parseJSON(userData);
            if (data !== undefined && data.gameChannel !== undefined) {
                this.gameChannel = data.gameChannel;
            } else {
                print("enemyEntity.js | ERROR: userData does not contain a game channel and/or team number");
            }
        },
        onCollide: function(entityA, entityB, collision) {
            if (this.entityIDsThatHaveCollidedWithMe.indexOf(entityB) > -1) {
                return;
            }
            this.entityIDsThatHaveCollidedWithMe.push(entityB);

            var colliderName = Entities.getEntityProperties(entityB, 'name').name;
            if (colliderName.indexOf("projectile") > -1) {
				var smokeImage = {
                    accelerationSpread: { x: 0, y: 0, z: 0 },
                    alpha: 1,
                    alphaFinish: 0,
                    alphaSpread: 0,
                    alphaStart: 0.3,
                    azimuthFinish: 3.1,
                    azimuthStart: -3.14159,
                    color: { red: 255, green: 255, blue: 255 },
                    colorFinish: { red: 255, green: 255, blue: 255 },
                    colorSpread: { red: 0, green: 0, blue: 0 },
                    colorStart: { red: 255, green: 255, blue: 255 },
                    emitAcceleration: { x: 0, y: 0, z: 0 },
                    emitDimensions: { x: 0, y: 0, z: 0 },
                    emitOrientation: { x: -0.7, y: 0.0, z: 0.0, w: 0.7 },
                    emitRate: 0.01,
                    emitSpeed: 0,
                    emitterShouldTrail: 0,
                    isEmitting: 1,
                    lifespan: 0.5,
                    lifetime: 0.5,
                    maxParticles: 1000,
                    name: 'ball-hit-smoke',
                    position: Entities.getEntityProperties(entityA, 'position').position,
                    particleRadius: 0.132,
                    polarFinish: 0,
                    polarStart: 0,
                    radiusFinish: 0.35,
                    radiusSpread: 0,
                    radiusStart: 0.132,
                    speedSpread: 0,
                    textures: Script.resolvePath('bow/smoke.png'),
                    type: 'ParticleEffect'
                };

                Entities.addEntity(smokeImage);
				
                Messages.sendMessage(this.gameChannel, JSON.stringify({
                    type: "enemy-killed",
                    entityID: this.entityID,
                    position: Entities.getEntityProperties(this.entityID, 'position').position
                }));
				
                Entities.deleteEntity(this.entityID);
            } else if (colliderName.indexOf("GateCollider") > -1) {
                Messages.sendMessage(this.gameChannel, JSON.stringify({
                    type: "enemy-escaped",
                    entityID: this.entityID,
                    position: Entities.getEntityProperties(this.entityID, 'position').position
                }));
                Entities.deleteEntity(this.entityID);
            }
        }
    };

    return new Enemy();
});
