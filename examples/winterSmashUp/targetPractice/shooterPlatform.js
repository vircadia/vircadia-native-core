//
//  shooterPlatform.js
//  examples/winterSmashUp/targetPractice
//
//  Created by Thijs Wenker on 12/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  The Winter Smash Up Target Practice Game using a bow.
//  This is the platform that spawns the bow and attaches it to the avatars hand,
//  then de-rez the bow on leaving to prevent walking up to the targets with the bow.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this = this;

    const GAME_CHANNEL = 'winterSmashUpGame';
    const SCRIPT_URL = Script.resolvePath('../../toybox/bow/bow.js');
    const MODEL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/new/bow-deadly.fbx";
    const COLLISION_HULL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/new/bow_collision_hull.obj";
    const RIGHT_HAND = 1;
    const LEFT_HAND = 0;

    var bowEntity = undefined;

    _this.enterEntity = function(entityID) {
        print('entered bow game entity');

        // Triggers a recording on an assignment client:
        Messages.sendMessage('PlayBackOnAssignment', 'BowShootingGameExplaination');

        bowEntity = Entities.addEntity({
            name: 'Hifi-Bow-For-Game',
            type: 'Model',
            modelURL: MODEL_URL,
            position: MyAvatar.position,
            dimensions: {x: 0.04, y: 1.3, z: 0.21},
            dynamic: true,
            gravity: {x: 0, y: 0, z: 0},
            shapeType: 'compound',
            compoundShapeURL: COLLISION_HULL_URL,
            script: SCRIPT_URL,
            userData: JSON.stringify({
                grabbableKey: {
                    invertSolidWhileHeld: true,
                    spatialKey: {
                        leftRelativePosition: {
                            x: 0.05,
                            y: 0.06,
                            z: -0.05
                        },
                        rightRelativePosition: {
                            x: -0.05,
                            y: 0.06,
                            z: -0.05
                        },
                        relativeRotation: Quat.fromPitchYawRollDegrees(0, 90, -90)
                    }
                }
            })
        });
        Messages.sendMessage('Hifi-Hand-Grab', JSON.stringify({hand: 'left', entityID: bowEntity}));
    };

    _this.leaveEntity = function(entityID) {
        if (bowEntity !== undefined) {
            Entities.deleteEntity(bowEntity);
            bowEntity = undefined;
        }
    };
});
