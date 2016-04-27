
//
//  controllerScriptingExamples.js
//  examples
//
//  Created by Sam Gondelman on 6/2/15
//  Rewritten by Alessandro Signa on 11/05/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// This allows to change the input mapping to easly understand of how the new mapping works.
// Two different ways are presented: the first one uses a JSON file to create the mapping, the second one declares the new routes explicitly one at a time. 
// You shuold prefer the first method if you have a lot of new routes, the second one if you want to express the action as a function.


/*
This function returns a JSON body. It's in charge to modify the standard controller and the mouse/keyboard mapping. 

The Standard controller is an abstraction: all the actual controllers are mapped to it. (i.e. Hydra --mapped to-> Standard --mapped to-> Action)
This example will overwrite the mapping of the left axis (Standard.LY, Standard.LX).
It's possible to find all the standard inputs (and their mapping) into standard.json
To try these changes you need a controller, not the keyboard.

The keyboard/mouse inputs are mapped directly to actions since the keyboard doesn't have its default mapping passing through the Standard controller.
If this new mapping contains inputs which are defined in the standard mapping, these will overwrite the old ones(Keyboard.W, Keyboard.RightMouseButton). 
If this new mapping contains inputs which are not defined in the standard, these will be added to the mapping(Keyboard.M).
*/
myFirstMapping = function() {
return {
    "name": "controllerMapping_First",
    "channels": [

        { "from": "Standard.LY", "to": "Actions.Yaw" },
        { "from": "Standard.LX", "to": "Actions.Yaw" },
        
        { "from": "Keyboard.W", "to": "Actions.YAW_LEFT" },    
        { "from": "Keyboard.M", "to": "Actions.YAW_RIGHT" },

        { "from": "Keyboard.LeftMouseButton", "to": "Actions.Up" }

    ]
}
}



var firstWay = true;
var mapping;
var MAPPING_NAME;


if(firstWay){
    var myFirstMappingJSON = myFirstMapping();
    print('myfirstMappingJSON' + JSON.stringify(myFirstMappingJSON));
    mapping = Controller.parseMapping(JSON.stringify(myFirstMappingJSON));
    mapping.enable();
}else{
    MAPPING_NAME = "controllerMapping_Second";
    var mapping2 = Controller.newMapping(MAPPING_NAME);
    mapping2.from(Controller.Hardware.Keyboard.RightMouseClicked).to(function (value) {
        print("Keyboard.RightMouseClicked");
    });
    mapping2.from(Controller.Standard.LX).to(Controller.Actions.Yaw);
    Controller.enableMapping(MAPPING_NAME);
}




/*
//-----------------some info prints that you would like to enable-----------------------
Object.keys(Controller.Standard).forEach(function (input) {
    print("Controller.Standard." + input + ":" + Controller.Standard[input]);
});

Object.keys(Controller.Hardware).forEach(function (deviceName) {
    Object.keys(Controller.Hardware[deviceName]).forEach(function (input) {
        print("Controller.Hardware." + deviceName + "." + input + ":" + Controller.Hardware[deviceName][input]);
    });
});

Object.keys(Controller.Actions).forEach(function (actionName) {
    print("Controller.Actions." + actionName + ":" + Controller.Actions[actionName]);
});
*/


Controller.hardwareChanged.connect(function () {
    print("hardwareChanged ---------------------------------------------------");
    Object.keys(Controller.Hardware).forEach(function (deviceName) {
        Object.keys(Controller.Hardware[deviceName]).forEach(function (input) {
            print("Controller.Hardware." + deviceName + "." + input + ":" + Controller.Hardware[deviceName][input]);
        });
    });
    print("-------------------------------------------------------------------");
});


Script.scriptEnding.connect(function () {
    if(firstWay){
        mapping.disable();
    } else {
        Controller.disableMapping(MAPPING_NAME);
    }
});