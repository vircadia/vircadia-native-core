//  immediateClientReset.js
//  Created by Eric Levin on 9/23/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


Script.include('libraries/utils.js');
var masterResetScript = Script.resolvePath("masterReset.js");
var hiddenEntityScriptURL = Script.resolvePath("hiddenEntityReset.js");

Script.include(masterResetScript);

function createHiddenMasterSwitch() {

    var resetKey = "resetMe";
    var masterSwitch = Entities.addEntity({
        type: "Box",
        name: "Master Switch",
        script: hiddenEntityScriptURL,
        dimensions: {
            x: 0.7,
            y: 0.2,
            z: 0.1
        },
        position: {
            x: 543.9,
            y: 496.05,
            z: 502.43
        },
        rotation: Quat.fromPitchYawRollDegrees(0, 33, 0),
        visible: false,
        userData: JSON.stringify({
            grabbableKey: {
                wantsTrigger: true
            }
        })
    });
}

var entities = Entities.findEntities(MyAvatar.position, 100);

entities.forEach(function(entity) {
    var name = Entities.getEntityProperties(entity, "name").name
    if (name === "Master Switch") {
        Entities.deleteEntity(entity);
    }
});

createHiddenMasterSwitch();

MasterReset();