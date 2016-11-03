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
Script.scriptEnding.connect(function(){
    mapping.disable();
});
