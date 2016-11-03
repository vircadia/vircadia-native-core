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

HomeMusicBox = function(spawnPosition, spawnRotation) {

    var SHOULD_CLEANUP = false;

    var WHITE = {
        red: 255,
        green: 255,
        blue: 255
    };

    var RED = {
        red: 255,
        green: 0,
        blue: 0
    };

    var GREEN = {
        red: 0,
        green: 255,
        blue: 0
    };

    var BLUE = {
        red: 0,
        green: 0,
        blue: 255
    };

    var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
        x: 0,
        y: 0.5,
        z: 0
    }), Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));

    var BASE_DIMENSIONS = {
        x: 0.1661,
        y: 0.1010,
        z: 0.2256
    };

    var BASE_POSITION = center;

    var LID_DIMENSIONS = {
        x: 0.1435,
        y: 0.0246,
        z: 0.1772
    };

    var LID_OFFSET = {
        x: 0,
        y: BASE_DIMENSIONS.y / 2,
        z: 0
    };

    var LID_REGISTRATION_POINT = {
        x: 0,
        y: 0.5,
        z: 0.5
    }

    var LID_SCRIPT_URL = Script.resolvePath('atp:/musicBox/lid.js');

    var BASE_SCRIPT_URL = Script.resolvePath('atp:/musicBox/baseBox.js');
    var base, lid, hat, key;

    function createLid(baseID) {

        var baseProps = Entities.getEntityProperties(baseID);
        var VERTICAL_OFFSET = 0.05;
        var FORWARD_OFFSET = 0;
        var LATERAL_OFFSET = -0.070;

        var startPosition = getOffsetFromCenter(VERTICAL_OFFSET, FORWARD_OFFSET, LATERAL_OFFSET);

        var lidProperties = {
            name: 'home_music_box_lid',
            type: 'Model',
            modelURL: 'atp:/musicBox/MB_Lid.fbx',
            dimensions: LID_DIMENSIONS,
            position: startPosition,
            parentID: baseID,
            registrationPoint: LID_REGISTRATION_POINT,
            dynamic: true,
            script: LID_SCRIPT_URL,
            collidesWith: 'myAvatar,otherAvatar',
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                },
                grabbableKey: {
                    wantsTrigger: true,
                    disableReleaseVelocity: true
                }
            })
        };

        lid = Entities.addEntity(lidProperties);
        createKey(baseID);
        createHat(baseID);

    };

    function createHat(baseID) {
        var VERTICAL_OFFSET = 0.025;
        var FORWARD_OFFSET = 0.0;
        var LATERAL_OFFSET = 0.0;

        var properties = {
            modelURL: "atp:/musicBox/MB_Hat.fbx",
            name: 'home_music_box_hat',
            type: 'Model',
            position: getOffsetFromCenter(VERTICAL_OFFSET, FORWARD_OFFSET, LATERAL_OFFSET),
            parentID: baseID,
            dimensions: {
                x: 0.0786,
                y: 0.0549,
                z: 0.0810
            },
            angularDamping: 1,
            angularVelocity: {
                x: 0,
                y: 0.785398,
                z: 0,
            },
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            })
        };

        hat = Entities.addEntity(properties);
    };

    function createKey(baseID) {
        var VERTICAL_OFFSET = 0.0;
        var FORWARD_OFFSET = 0.11;
        var LATERAL_OFFSET = 0.0;

        var properties = {
            modelURL: "atp:/musicBox/MB_Key.fbx",
            name: 'home_music_box_key',
            type: 'Model',
            parentID: baseID,
            angularDamping: 1,
            angularVelocity: {
                x: 0,
                y: 0,
                z: 0.785398,
            },
            position: getOffsetFromCenter(VERTICAL_OFFSET, FORWARD_OFFSET, LATERAL_OFFSET),
            dimensions: {
                x: 0.0057,
                y: 0.0482,
                z: 0.0435
            },
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            })
        };

        key = Entities.addEntity(properties);
    };

    function createBaseBox() {

        var properties = {
            modelURL: "atp:/musicBox/MB_Box.fbx",
            name: 'home_music_box_base',
            type: 'Model',
            position: BASE_POSITION,
            dynamic: true,
            shapeType: 'compound',
            compoundShapeURL: 'atp:/musicBox/boxHull3.obj',
            dimensions: {
                x: 0.1661,
                y: 0.0928,
                z: 0.2022
            },
            script: BASE_SCRIPT_URL,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                },
                grabbableKey: {
                    disableReleaseVelocity: true
                }
            })
        }

        base = Entities.addEntity(properties);
        createLid(base);
    };

    function cleanup() {
        Entities.deleteEntity(base);
        Entities.deleteEntity(lid);
        Entities.deleteEntity(key);
        Entities.deleteEntity(hat);
    };

    function getOffsetFromCenter(VERTICAL_OFFSET, FORWARD_OFFSET, LATERAL_OFFSET) {

        var properties = Entities.getEntityProperties(base);

        var upVector = Quat.getUp(properties.rotation);
        var frontVector = Quat.getFront(properties.rotation);
        var rightVector = Quat.getRight(properties.rotation);

        var upOffset = Vec3.multiply(upVector, VERTICAL_OFFSET);
        var frontOffset = Vec3.multiply(frontVector, FORWARD_OFFSET);
        var rightOffset = Vec3.multiply(rightVector, LATERAL_OFFSET);

        var finalOffset = Vec3.sum(properties.position, upOffset);
        finalOffset = Vec3.sum(finalOffset, frontOffset);
        finalOffset = Vec3.sum(finalOffset, rightOffset);

        return finalOffset
    };

    this.cleanup = cleanup;

    createBaseBox();

    return this;
}