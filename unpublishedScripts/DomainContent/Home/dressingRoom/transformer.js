//
//
//  Created by The Content Team 4/10/216
//  Copyright 2016 High Fidelity, Inc.
//
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// when you drop a doll and it hits the dressing room platform it transforms into a big version. 
// the small doll is destroyed and a new small doll is put on the shelf


(function() {

    //full size dimensions
    var STYLIZED_FEMALE_DIMENSIONS = {
        x: 1.6323,
        y: 1.7705,
        z: 0.2851
    };

    var BEING_OF_LIGHT_DIMENSIONS = {
        x: 1.8838,
        y: 1.7865,
        z: 0.2955
    };

    var ROBOT_DIMENSIONS = {
        //robot
        x: 1.4439,
        y: 0.6224,
        z: 0.4998
    };

    var WILL_DIMENSIONS = {
        x: 1.6326,
        y: 1.6764,
        z: 0.2606
    };

    var PRISCILLA_DIMENSIONS = {
        //priscilla
        x: 1.6448,
        y: 1.6657,
        z: 0.3078
    };

    var MATTHEW_DIMENSIONS = {
        //matthew
        x: 1.8722,
        y: 1.8197,
        z: 0.3666
    };

    var _this;

    function Transformer() {
        _this = this;
        return this;
    };

    Transformer.prototype = {
        locked: false,
        rotatorBlock: null,
        transformationSound: null,
        preload: function(entityID) {
            print('PRELOAD TRANSFORMER SCRIPT')
            this.entityID = entityID;
            this.initialProperties = Entities.getEntityProperties(entityID);
        },

        collisionWithEntity: function(myID, otherID, collisionInfo) {
            var otherProps = Entities.getEntityProperties(otherID);

            if (otherProps.name === "hifi-home-dressing-room-transformer-collider" && _this.locked === false) {
                _this.locked = true;
                _this.findRotatorBlock();
            } else {
                var transformerProps = Entities.getEntityProperties(_this.entityID, ["rotation", "position"]);

                var eulerRotation = Quat.safeEulerAngles(transformerProps.rotation);
                eulerRotation.x = 0;
                eulerRotation.z = 0;
                var newRotation = Quat.fromVec3Degrees(eulerRotation);

                //we zero out the velocity and angular velocity so the cow doesn't change position or spin
                Entities.editEntity(_this.entityID, {
                    rotation: newRotation,
                    velocity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    angularVelocity: {
                        x: 0,
                        y: 0,
                        z: 0
                    }
                });

                return;
            }
        },

        findRotatorBlock: function() {
            var myProps = Entities.getEntityProperties(_this.entityID);
            var results = Entities.findEntities(myProps.position, 10);

            results.forEach(function(result) {
                var resultProps = Entities.getEntityProperties(result);
                if (resultProps.name === "hifi-home-dressing-room-rotator-block") {
                    _this.rotatorBlock = result;
                    _this.removeCurrentBigVersion(result);
                    return;
                }
            });
        },

        removeCurrentBigVersion: function(rotatorBlock) {
            var blacklistKey = 'Hifi-Hand-RayPick-Blacklist';
            var myProps = Entities.getEntityProperties(_this.entityID);
            var results = Entities.findEntities(myProps.position, 10);

            results.forEach(function(result) {
                var resultProps = Entities.getEntityProperties(result);
                if (resultProps.name === "hifi-home-dressing-room-big-transformer") {
                    Messages.sendMessage(blacklistKey, JSON.stringify({
                        action: 'remove',
                        id: result
                    }));

                    Entities.deleteEntity(result);

                    return;
                }
            });

            _this.createBigVersion();
        },

        createBigVersion: function() {
            var smallProps = Entities.getEntityProperties(_this.entityID);
            var rotatorProps = Entities.getEntityProperties(_this.rotatorBlock);

            var dimensions;
            if (smallProps.modelURL.indexOf('will') > -1) {
                // print('TRANSFORMER IS WILL');
                dimensions = WILL_DIMENSIONS;
            } else if (smallProps.modelURL.indexOf('being_of_light') > -1) {
                // print('TRANSFORMER IS BEING OF LIGHT');
                dimensions = BEING_OF_LIGHT_DIMENSIONS;
            } else if (smallProps.modelURL.indexOf('stylized_female') > -1) {
                // print('TRANSFORMER IS ARTEMIS');
                dimensions = STYLIZED_FEMALE_DIMENSIONS;
            } else if (smallProps.modelURL.indexOf('simple_robot') > -1) {
                // print('TRANSFORMER IS A ROBOT');
                dimensions = ROBOT_DIMENSIONS;
            } else if (smallProps.modelURL.indexOf('priscilla') > -1) {
                // print('TRANSFORMER IS PRISCILLA');
                dimensions = PRISCILLA_DIMENSIONS;
            } else if (smallProps.modelURL.indexOf('matthew') > -1) {
                // print('TRANSFORMER IS MATTHEW');
                dimensions = MATTHEW_DIMENSIONS;
            } else {
                // print('TRANSFORMER IS SOME OTHER');
                dimensions = smallProps.naturalDimensions;
            }

            var bigVersionProps = {
                name: "hifi-home-dressing-room-big-transformer",
                type: 'Model',
                dimensions: dimensions,
                parentID: _this.rotatorBlock,
                modelURL: smallProps.modelURL,
                position: _this.putTransformerOnRotatorBlock(rotatorProps.position),
                rotation: rotatorProps.rotation,
                userData: JSON.stringify({
                    'grabbableKey': {
                        'grabbable': false
                    },
                    'hifiHomeKey': {
                        'reset': true
                    }
                }),
            };

            if (bigVersionProps.modelURL.indexOf('simple_robot') > -1) {
                bigVersionProps.position.y += 0.5;
            }

            var bigVersion = Entities.addEntity(bigVersionProps);

            var blacklistKey = 'Hifi-Hand-RayPick-Blacklist';
            Messages.sendMessage(blacklistKey, JSON.stringify({
                action: 'add',
                id: bigVersion
            }));

            _this.putNewVersionOnShelf();
        },

        putTransformerOnRotatorBlock: function(blockPosition) {
            return blockPosition;
        },

        putNewVersionOnShelf: function() {
            var littleVersionProps = Entities.getEntityProperties(_this.entityID);
            delete littleVersionProps.id;
            delete littleVersionProps.created;
            delete littleVersionProps.age;
            delete littleVersionProps.ageAsText;
            delete littleVersionProps.position;
            delete littleVersionProps.rotation;
            delete littleVersionProps.localPosition;
            delete littleVersionProps.localRotation;
            delete littleVersionProps.naturalPosition;
            littleVersionProps.gravity = {
                x: 0,
                y: -10,
                z: 0
            };
            var userData = JSON.parse(littleVersionProps.userData);

            var basePosition = userData["hifiHomeTransformerKey"].basePosition;
            var baseRotation = userData["hifiHomeTransformerKey"].baseRotation;

            littleVersionProps.position = basePosition;
            littleVersionProps.rotation = baseRotation;
            var littleTransformer = Entities.addEntity(littleVersionProps);

            _this.removeSelf();
        },

        removeSelf: function() {
            var success = Entities.deleteEntity(_this.entityID);
        },
    };


    return new Transformer();
})