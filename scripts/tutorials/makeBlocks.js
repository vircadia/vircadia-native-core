//
//  makeBlocks.js
//
//  Created by Matti Lahtinen 4/3/2017
//  Copyright 2017 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Creates multiple  "magnetic" blocks with random colors that users clones of and snap together.


(function() {
    var MAX_RGB_COMPONENT_VALUE = 256 / 2; // Limit the values to half the maximum.
    var MIN_COLOR_VALUE = 127;
    var SIZE = 0.3;
    var LIFETIME = 600;
    var VERTICAL_OFFSET = -0.25;
    var ROWS = 3;
    var COLUMNS = 3;
    // Random Pastel Generator based on Piper's script
    function newColor() {
        return {
            red: randomPastelRGBComponent(),
            green: randomPastelRGBComponent(),
            blue: randomPastelRGBComponent()
        };
    }
    // Helper functions.
    function randomPastelRGBComponent() {
        return Math.floor(Math.random() * MAX_RGB_COMPONENT_VALUE) + MIN_COLOR_VALUE;
    }

    var SCRIPT_URL = Script.resolvePath("./entity_scripts/magneticBlock.js");

    var forwardVector = Quat.getForward(MyAvatar.orientation);
    forwardVector.y += VERTICAL_OFFSET;
    for (var x = 0; x < COLUMNS; x++) {
        for (var y = 0; y < ROWS; y++) {

            var forwardOffset = {
                x: 0,
                y: SIZE * y + SIZE,
                z: SIZE * x + SIZE
            };

            Entities.addEntity({
                type: "Box",
                name: "MagneticBlock-" + y + '-' + x,
                dimensions: {
                    x: SIZE,
                    y: SIZE,
                    z: SIZE
                },
                userData: JSON.stringify({
                    grabbableKey: {
                        cloneable: true,
                        grabbable: true,
                        cloneLifetime: LIFETIME,
                        cloneLimit: 9999
                    }
                }),
                position: Vec3.sum(MyAvatar.position, Vec3.sum(forwardOffset, forwardVector)),
                color: newColor(),
                script: SCRIPT_URL
            });
        }
    }

    Script.stop();
})();
