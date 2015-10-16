
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
    "channels": [ {
            "from": "Keyboard.A",
            "filters": [ {
                    "type": "clamp",
                    "params": [0, 1],
                }
            ],
            "to": "Actions.LONGITUDINAL_FORWARD",
        }, {
            "from": "Keyboard.Left",
            "filters": [ {
                    "type": "clamp",
                    "params": [0, 1],
                }, {
                    "type": "invert"
                }
            ],
            "to": "Actions.LONGITUDINAL_BACKWARD",
        }, {
            "from": "Keyboard.C",
            "filters": [ {
                    "type": "scale",
                    "params": [2.0],
                }
            ],
            "to": "Actions.Yaw",
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
