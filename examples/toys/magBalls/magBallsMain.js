//
//  Created by Bradley Austin Davis on 2015/08/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/htmlColors.js");
Script.include("constants.js");
Script.include("utils.js");
Script.include("magBalls.js");

Script.include("ballController.js");

var magBalls = new MagBalls();

// Clear any previous balls
// magBalls.clear();

MenuController = function(side) {
    HandController.call(this, side);
}

// FIXME resolve some of the issues with dual controllers before allowing both controllers active
var handControllers = [new BallController(LEFT_CONTROLLER, magBalls)]; //, new HandController(RIGHT) ];
