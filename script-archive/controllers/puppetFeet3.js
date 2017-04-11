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

var rotation = Quat.fromPitchYawRollDegrees(0, 0, -90); 
var noTranslation = { x: 0, y: 0, z: 0 };
var transformMatrix = Mat4.createFromRotAndTrans(rotation, noTranslation);
var rotateAndTranslate = Mat4.createFromRotAndTrans(rotation, puppetOffset);


mapping.from(Controller.Standard.LeftHand).peek().rotate(rotation).translate(puppetOffset).to(Controller.Standard.LeftFoot);

//mapping.from(Controller.Standard.LeftHand).peek().translate(puppetOffset).to(Controller.Standard.LeftFoot);
//mapping.from(Controller.Standard.LeftHand).peek().transform(transformMatrix).translate(puppetOffset).to(Controller.Standard.LeftFoot);
//mapping.from(Controller.Standard.LeftHand).peek().transform(rotateAndTranslate).to(Controller.Standard.LeftFoot);

Controller.enableMapping(MAPPING_NAME);


Script.scriptEnding.connect(function(){
    mapping.disable();
});
