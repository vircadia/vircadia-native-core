//
//  createDressingPlatform.js
//
//  Created by James B. Pollack @imgntn on 1/7/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This script shows how to hook up a model entity to your avatar to act as a doppelganger.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var basePlatform;
var basePosition = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: -1,
    z: 0
}), Vec3.multiply(2, Quat.getFront(Camera.getOrientation())));

var loadArea;
var LOAD_AREA_SCRIPT_URL = Script.resolvePath('loadingAreaEntity.js');

function createBasePlatform() {
    var properties = {
        type: 'Box',
        name: 'Hifi-Dressing-Room-Base',
        dimensions: {
            x: 4,
            y: 0.10,
            z: 4
        },
        color: {
            red: 255,
            green: 0,
            blue: 255
        },
        position: basePosition,
        collisionsWillMove: false,
        ignoreForCollisions: false,
        userData: JSON.stringify({
            grabbableKey: {
                grabbable: false
            }
        })
    }
    basePlatform = Entities.addEntity(properties);
}

function createLoadArea() {
    // on enter, load the wearables manager and the doppelganger manager;
    // on exit, stop the scripts (at least call cleanup);
    var properties = {
        type: 'Box',
        shapeType: 'box',
        name: 'Hifi-Dressing-Room-Load-Area',
        dimensions: {
            x: 0.25,
            y: 0.25,
            z: 0.25
        },
        color: {
            red: 0,
            green: 255,
            blue: 0
        },
        visible: true,
        position: basePosition,
        collisionsWillMove: false,
        ignoreForCollisions: true,
        script: LOAD_AREA_SCRIPT_URL,

    }
    loadArea = Entities.addEntity(properties);
}
var lights = [];

function createLightAtPosition(position) {
    var lightProperties = {
        name: 'Hifi-Spotlight',
        type: "Light",
        isSpotlight: true,
        dimensions: {
            x: 2,
            y: 2,
            z: 8
        },
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        intensity: 0.035,
        exponent: 1,
        cutoff: 40,
        lifetime: -1,
        position: position,
        rotation: getLightRotation(position)
    };

    light = Entities.addEntity(lightProperties);
    lights.push(light);
}

function createLights() {
    var lightPosition = {
        x: basePosition.x - 2,
        y: basePosition.y + 3,
        z: basePosition.z
    }
    createLightAtPosition(lightPosition);

    var lightPosition = {
        x: basePosition.x + 2,
        y: basePosition.y + 3,
        z: basePosition.z
    }

    createLightAtPosition(lightPosition);
    var lightPosition = {
        x: basePosition.x,
        y: basePosition.y + 3,
        z: basePosition.z + 2
    }

    createLightAtPosition(lightPosition);
    var lightPosition = {
        x: basePosition.x,
        y: basePosition.y + 3,
        z: basePosition.z - 2
    }

    createLightAtPosition(lightPosition);

}

function getLightRotation(myPosition) {

    var sourceToTargetVec = Vec3.subtract(basePosition, myPosition);
    var emitOrientation = Quat.rotationBetween(Vec3.UNIT_NEG_Z, sourceToTargetVec);

    return emitOrientation
}

function init() {
    createBasePlatform();
    createLights();
    // createLoadArea();
}


function cleanup() {
    Entities.deleteEntity(basePlatform);

    while (lights.length > 0) {
        Entities.deleteEntity(lights.pop());
    }
    //   Entities.deleteEntity(loadArea);
}
init();
Script.scriptEnding.connect(cleanup)