//
//  puppetFeet3.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2017/04/11
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var MAPPING_NAME = "com.highfidelity.examples.puppetFeet3";
var mapping = Controller.newMapping(MAPPING_NAME);
var puppetOffset = { x: 0, y: -1, z: 0 };

mapping.from(Controller.Standard.LeftHand).peek().translate(puppetOffset).to(Controller.Standard.LeftFoot);

Controller.enableMapping(MAPPING_NAME);


Script.scriptEnding.connect(function(){
    mapping.disable();
});
