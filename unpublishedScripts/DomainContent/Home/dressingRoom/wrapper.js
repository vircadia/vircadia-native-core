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

var TRANSFORMER_SCRIPT = Script.resolvePath('transformer.js');

var AVATAR_COLLISION_HULL = 'atp:/dressingRoom/Avatar-Hull-6.obj';
var ROBOT_COLLISION_HULL = 'atp:/dressingRoom/robot_hull.obj';

TransformerDoll = function(modelURL, spawnPosition, spawnRotation, dimensions) {

    var transformerProps = {
        name: 'hifi-home-dressing-room-little-transformer',
        type: 'Model',
        shapeType: 'compound',
        compoundShapeURL: AVATAR_COLLISION_HULL,
        position: spawnPosition,
        rotation: Quat.fromPitchYawRollDegrees(spawnRotation.x, spawnRotation.y, spawnRotation.z),
        modelURL: modelURL,
        dynamic: true,
        gravity: {
            x: 0,
            y: -10,
            z: 0
        },
        visible: true,
        restitution: 0.1,
        damping: 0.9,
        angularDamping: 0.9,
        userData: JSON.stringify({
            'grabbableKey': {
                'grabbable': true
            },
            'hifiHomeTransformerKey': {
                'basePosition': spawnPosition,
                'baseRotation': Quat.fromPitchYawRollDegrees(spawnRotation.x, spawnRotation.y, spawnRotation.z),
            },
            'hifiHomeKey': {
                'reset': true
            }
        }),
        density: 7500,
        dimensions: dimensions,
        script: TRANSFORMER_SCRIPT
    };

    if (modelURL.indexOf('robot') > -1) {
        transformerProps.compoundShapeURL = ROBOT_COLLISION_HULL;
    }

    var transformer = Entities.addEntity(transformerProps);

    return this;
}