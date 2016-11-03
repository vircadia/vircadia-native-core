//  photo_sphere.js
//
//  Created by James B. Pollack @imgntn on 3/11/2015
//  Copyright 2016 High Fidelity, Inc.
//
//  This script creates a photo sphere around you.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var photoSphere, light;

//equirectangular
var url = 'http://hifi-content.s3.amazonaws.com/james/projection_objects/IMG_9167.JPG';

var MODEL_URL = 'http://hifi-content.s3.amazonaws.com/james/projection_objects/photosphere2.fbx';

function createPhotoSphere() {

    var textureString = 'photo:"' + url + '"'

    var properties = {
        type: 'Model',
        modelURL: MODEL_URL,
        name: 'hifi-photo-sphere',
        dimensions: {
            x: 32,
            y: 32,
            z: 32
        },
        position: MyAvatar.position,
        textures: textureString
    }
    photoSphere = Entities.addEntity(properties);
}

function createLight() {
    var properties = {
        name: 'hifi-photo-sphere-light',
        type: 'Light',
        dimensions: {
            x: 36,
            y: 36,
            z: 36,
        },
        intensity: 4.0,
        falloffRadius: 22,
        position: MyAvatar.position
    }
    light = Entities.addEntity(properties);
}

function cleanup() {
    Entities.deleteEntity(photoSphere);
    Entities.deleteEntity(light);
}

Script.scriptEnding.connect(cleanup);
createPhotoSphere();
createLight();
