//
//  createParamsEntity.js
//
//  Created by James B. Pollack @imgntn on 11/6/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script demonstrates creating an entity and sending it a method call with parameters.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var PARAMS_SCRIPT_URL = Script.resolvePath('paramsEntity.js');

var testEntity = Entities.addEntity({
    name: 'paramsTestEntity',
    dimensions: {
        x: 1,
        y: 1,
        z: 1
    },
    type: 'Box',
    position: {
        x: 0,
        y: 0,
        z: 0
    },
    visible: false,
    script: PARAMS_SCRIPT_URL
});


var subData1 = ['apple', 'banana', 'orange'];
var subData2 = {
    thing: 1,
    otherThing: 2
};
var data = [subData1, JSON.stringify(subData2), 'third'];
Script.setTimeout(function() {
    print('sending data to entity')
    Entities.callEntityMethod(testEntity, 'testParams', data);
}, 1500)