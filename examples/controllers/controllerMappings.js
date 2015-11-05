
//
//  controllerScriptingExamples.js
//  examples
//
//  Created by Sam Gondelman on 6/2/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// This allows to change the input mapping making a sense of how the new mapping works


/*
This function returns a JSON body. It's in charge to modify the mouse/keyboard mapping. The keyboard inputs are mapped directly to actions.
If this new mapping contains inputs which are defined in the standard mapping, these will overwrite the old ones(Keyboard.W, Keyboard.RightMouseButton). 
If this new mapping contains inputs which are not defined in the standard, these will be added to the mapping(Keyboard.M).
*/
myFirstMapping = function() {
return {
    "name": "example",
    "channels": [
        { "from": "Keyboard.W", "to": "Actions.YAW_LEFT" },    
        { "from": "Keyboard.M", "to": "Actions.YAW_LEFT" },
        
        { "from": "Keyboard.RightMouseButton", "to": "Actions.YAW_RIGHT" }
    ]
}
}

/*
This JSON is in charge to modify the Standard controller mapping. 
The standard controller is an abstraction: all the actual controllers are mapped to it. (i.e. Hydra --mapped to-> Standard --mapped to-> Action)
These new inputs will overwrite the standard ones.
It's possible to find all the standard inputs (and their mapping) into standard.json
To try the mySecondMapping effect you need a controller, not the keyboard.
*/
mySecondMapping = function() {
return {
    "name": "example2",
    "channels": [
        { "from": "Standard.LY", "to": "Actions.Yaw" },
        { "from": "Standard.LX", "to": "Actions.Yaw" },
    ]
}
}


var tryFirst = true;
var mapping;
if(tryFirst){
    var myFirstMappingJSON = myFirstMapping();
    print('myfirstMappingJSON' + JSON.stringify(myFirstMappingJSON));
    mapping = Controller.parseMapping(JSON.stringify(myFirstMappingJSON));
}else{
    var mySecondMappingJSON = mySecondMapping();
    print('mySecondMappingJSON' + JSON.stringify(mySecondMappingJSON));
    mapping = Controller.parseMapping(JSON.stringify(mySecondMappingJSON));
}
mapping.enable();




/*
//-----------------some info prints-----------------------
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
    mapping.disable();
});