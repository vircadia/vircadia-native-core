//
//  clownGloveDispenser.js
//
//  Created by Thijs Wenker on 8/2/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Based on examples/winterSmashUp/targetPractice/shooterPlatform.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var _this = this;

    var CHANNEL_PREFIX = 'io.highfidelity.boppo_server_';

    var leftBoxingGlove = undefined;
    var rightBoxingGlove = undefined;

    var inZone = false;

    var wearGloves = function() {
        leftBoxingGlove = Entities.addEntity({
            position: MyAvatar.position,
            collisionsWillMove: true,
            dimensions: {
                x: 0.24890634417533875,
                y: 0.28214839100837708,
                z: 0.21127720177173615
            },
            dynamic: true,
            gravity: {
                x: 0,
                y: -9.8,
                z: 0
            },
            modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/caitlyn/production/elBoppo/LFT_glove_VR3.fbx"),
            name: "Boxing Glove - Left",
            registrationPoint: {
                x: 0.5,
                y: 0,
                z: 0.5
            },
            shapeType: "simple-hull",
            type: "Model",
            userData: JSON.stringify({
                grabbableKey: {
                    invertSolidWhileHeld: true
                },
                wearable: {
                    joints: {
                        LeftHand: [
                            {x: 0, y: 0.0, z: 0.02 },
                            Quat.fromVec3Degrees({x: 0, y: 0, z: 0})
                        ]
                    }
                }
            })
        });
        Messages.sendLocalMessage('Hifi-Hand-Grab', JSON.stringify({hand: 'left', entityID: leftBoxingGlove}));
        // Allows teleporting while glove is wielded
        Messages.sendLocalMessage('Hifi-Teleport-Ignore-Add', leftBoxingGlove);

        rightBoxingGlove = Entities.addEntity({
            position: MyAvatar.position,
            collisionsWillMove: true,
            dimensions: {
                x: 0.24890634417533875,
                y: 0.28214839100837708,
                z: 0.21127720177173615
            },
            dynamic: true,
            gravity: {
                x: 0,
                y: -9.8,
                z: 0
            },
            modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/caitlyn/production/elBoppo/RT_glove_VR2.fbx"),
            name: "Boxing Glove - Right",
            registrationPoint: {
                x: 0.5,
                y: 0,
                z: 0.5
            },
            shapeType: "simple-hull",
            type: "Model",
            userData: JSON.stringify({
                grabbableKey: {
                    invertSolidWhileHeld: true
                },
                wearable: {
                    joints: {
                        RightHand: [
                            {x: 0, y: 0.0, z: 0.02 },
                            Quat.fromVec3Degrees({x: 0, y: 0, z: 0})
                        ]
                    }
                }
            })
        });
        Messages.sendLocalMessage('Hifi-Hand-Grab', JSON.stringify({hand: 'right', entityID: rightBoxingGlove}));
        // Allows teleporting while glove is wielded
        Messages.sendLocalMessage('Hifi-Teleport-Ignore-Add', rightBoxingGlove);
    };

    var cleanUpGloves = function() {
        if (leftBoxingGlove !== undefined) {
            Entities.deleteEntity(leftBoxingGlove);
            leftBoxingGlove = undefined;
        }
        if (rightBoxingGlove !== undefined) {
            Entities.deleteEntity(rightBoxingGlove);
            rightBoxingGlove = undefined;
        }
    };

    var wearGlovesIfHMD = function() {
        // cleanup your old gloves if they're still there (unlikely)
        cleanUpGloves();
        if (HMD.active) {
            wearGloves();
        }
    };

    _this.preload = function(entityID) {
        HMD.displayModeChanged.connect(function() {
            if (inZone) {
                wearGlovesIfHMD();
            }
        });
    };

    _this.unload = function() {
        cleanUpGloves();
    };

    _this.enterEntity = function(entityID) {
        inZone = true;
        print('entered boxing glove dispenser entity');
        wearGlovesIfHMD();

        // Reset boppo if game is not running:
        var parentID = Entities.getEntityProperties(entityID, ['parentID']).parentID;
        Messages.sendMessage(CHANNEL_PREFIX + parentID, 'enter-zone');
    };

    _this.leaveEntity = function(entityID) {
        inZone = false;
        cleanUpGloves();
    };

    _this.unload = _this.leaveEntity;
});
