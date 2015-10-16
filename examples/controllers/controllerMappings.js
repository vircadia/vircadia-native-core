
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

// Assumes you only have the default keyboard connected

myFirstMapping = function() {
return {
    "name": "example",
    "channels": [
        { "from": "Keyboard.W", "to": "Actions.LONGITUDINAL_FORWARD" },
        { "from": "Keyboard.S", "to": "Actions.LONGITUDINAL_BACKWARD" },

        { "from": "Keyboard.Left", "to": "Actions.LATERAL_LEFT" },
        { "from": "Keyboard.Right", "to": "Actions.LATERAL_RIGHT" },

        { "from": "Keyboard.A", "to": "Actions.YAW_LEFT" },
        { "from": "Keyboard.D", "to": "Actions.YAW_RIGHT" },

        { "from": "Keyboard.C", "to": "Actions.VERTICAL_DOWN" }, 
        { "from": "Keyboard.E", "to": "Actions.VERTICAL_UP" },
        {
            "from": "Standard.LX",
            "filters": [ {
                    "type": "scale",
                    "params": [2.0],
                }
            ],
            "to": "Actions.LATERAL_LEFT",
        }, {
            "from": "Keyboard.B",
            "to": "Actions.Yaw"
        }
    ]
}
}


//Script.include('mapping-test0.json');
var myFirstMappingJSON = myFirstMapping();
print('myFirstMappingJSON' + JSON.stringify(myFirstMappingJSON));

var mapping = Controller.parseMapping(JSON.stringify(myFirstMappingJSON));

Controller.enableMapping("example");
/*
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