//
//  reticleTest.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2015/12/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var mappingJSON = {
    "name": "com.highfidelity.testing.reticleWithJoystick",
    "channels": [
        { "from": "Standard.RT", "to": "Actions.ReticleClick", "filters": "constrainToInteger" },

        { "from": "Standard.RX", "to": "Actions.ReticleX",
          "filters":
            [
                { "type": "pulse", "interval": 0.05 },
                { "type": "scale", "scale": 20 }
            ]
        },
        { "from": "Standard.RY", "to": "Actions.ReticleY",
          "filters":
            [
                { "type": "pulse", "interval": 0.05 },
                { "type": "scale", "scale": 20 }
            ]
        },
    ]
};

mapping = Controller.parseMapping(JSON.stringify(mappingJSON));
mapping.enable();


var MAPPING_NAME = "com.highfidelity.testing.reticleWithHand";
var mapping2 = Controller.newMapping(MAPPING_NAME);
//mapping2.from(Controller.Standard.RightHand).debug(true).to(Controller.Actions.LeftHand);

/*
mapping2.from(Controller.Standard.RightHand).peek().to(function(pose) {
        print("Controller.Standard.RightHand value:" + JSON.stringify(pose));
    });
*/

var translation = { x: 0, y: 0.1, z: 0 };
var translationDx = 0.01;
var translationDy = 0.01;
var translationDz = 0.01;
var TRANSLATION_LIMIT = 0.5;
var rotation = Quat.fromPitchYawRollDegrees(45, 0, 45);
mapping2.from(function() {
    translation.x = translation.x + translationDx;
    translation.y = translation.y + translationDy;
    translation.z = translation.z + translationDz;
    if ((translation.x > TRANSLATION_LIMIT) || (translation.x < (-1 *TRANSLATION_LIMIT))) {
        translationDx = translationDx * -1;
    }
    if ((translation.y > TRANSLATION_LIMIT) || (translation.y < (-1 *TRANSLATION_LIMIT))) {
        translationDy = translationDy * -1;
    }
    if ((translation.z > TRANSLATION_LIMIT) || (translation.z < (-1 *TRANSLATION_LIMIT))) {
        translationDz = translationDz * -1;
    }
    var pose = {
            translation: translation,
            rotation: rotation,
            velocity: { x: 0, y: 0, z: 0 },
            angularVelocity: { x: 0, y: 0, z: 0, w: 1 }
        };
    return pose;
}).debug(true).to(Controller.Standard.LeftHand);

Controller.enableMapping(MAPPING_NAME);


Script.scriptEnding.connect(function(){
    mapping.disable();
});
