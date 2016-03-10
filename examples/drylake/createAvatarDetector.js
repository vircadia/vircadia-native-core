//
//  createAvatarDetector.js
//
//  Created by James B. Pollack @imgntn on 12/7/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Run this script if you want the rats to run away from you.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var avatarDetector = null;

function createAvatarDetector() {

    var detectorProperties = {
        name: 'Hifi-Avatar-Detector',
        type: 'Box',
        position: MyAvatar.position,
        dimensions: {
            x: 1,
            y: 2,
            z: 1
        },
        dynamic: false,
        collisionless: true,
        visible: false,
        color: {
            red: 255,
            green: 0,
            blue: 0
        }
    }

    avatarDetector = Entities.addEntity(detectorProperties);
};

var updateAvatarDetector = function() {
    //  print('updating detector position' + JSON.stringify(MyAvatar.position))
    Entities.editEntity(avatarDetector, {
        position: MyAvatar.position
    });

};

var cleanup = function() {
    Script.update.disconnect(updateAvatarDetector);
    Entities.deleteEntity(avatarDetector);
}

createAvatarDetector();
Script.scriptEnding.connect(cleanup);
Script.update.connect(updateAvatarDetector);
