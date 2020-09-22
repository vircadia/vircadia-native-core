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

	var BOOF_SOUND = SoundCache.getSound(Script.resolvePath("sounds/boof.wav"));
	
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
					"alpha": 0.89999997615814209,
					"alphaFinish": 0.89999997615814209,
					"alphaStart": 0.89999997615814209,
					"azimuthFinish": 0.40000000596046448,
					"clientOnly": 0,
					"color": {
						"blue": 57,
						"green": 254,
						"red": 255
					},
					"colorFinish": {
						"blue": 57,
						"green": 254,
						"red": 255
					},
					"colorSpread": {
						"blue": 130,
						"green": 130,
						"red": 130
					},
					"colorStart": {
						"blue": 57,
						"green": 254,
						"red": 255
					},
					"created": "2017-01-10T19:17:07Z",
					"dimensions": {
						"x": 1.7600001096725464,
						"y": 1.7600001096725464,
						"z": 1.7600001096725464
					},
					"emitAcceleration": {
						"x": 0,
						"y": 0,
						"z": 0
					},
					"emitOrientation": {
						"w": 0.7070610523223877,
						"x": -0.70715254545211792,
						"y": -1.5258869098033756e-05,
						"z": -1.5258869098033756e-05
					},
					"emitRate": 25.200000762939453,
					"emitSpeed": 0,
					"id": "{0273087c-a676-4ac2-93ff-f2440517dfb7}",
					"lastEdited": 1485295831550663,
					"lastEditedBy": "{dfe734a0-8289-47f6-a1a6-cf3f6d57adac}",
					"lifespan": 0.5,
                    "lifetime": 0.5,
					"locked": 1,
					"owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
					"particleRadius": 0.30099999904632568,
					"polarFinish": 3,
					"position": Entities.getEntityProperties(entityA, 'position').position,
					"queryAACube": {
						"scale": 3.0484094619750977,
						"x": -1.5242047309875488,
						"y": -1.5242047309875488,
						"z": -1.5242047309875488
					},
					"radiusFinish": 0.30099999904632568,
					"radiusStart": 0.30099999904632568,
					"rotation": {
						"w": 0.086696967482566833,
						"x": -0.53287714719772339,
						"y": 0.80860012769699097,
						"z": -0.23394235968589783
					},
					"textures": Script.getExternalPath(Script.ExternalPaths.HF_Public, "/alan/Particles/Particle-Sprite-Smoke-1.png"),
					"type": "ParticleEffect"
                };

				Audio.playSound(BOOF_SOUND, {
                    volume: 1.0,
                    position: Entities.getEntityProperties(entityA, 'position').position
                });
				
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
