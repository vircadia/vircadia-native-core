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
    const MAX_RGB_COMPONENT_VALUE = 256 / 2; // Limit the values to half the maximum.
    const MIN_COLOR_VALUE = 127;
    const SIZE = 0.3;
    const LIFETIME = 600;

    // Random Pastel Generator based on Piper's script
    function newColor() {
        color = {
            red: randomPastelRGBComponent(),
            green: randomPastelRGBComponent(),
            blue: randomPastelRGBComponent()
        };
        return color;
    }
    // Helper functions.
    function randomPastelRGBComponent() {
        return Math.floor(Math.random() * MAX_RGB_COMPONENT_VALUE) + MIN_COLOR_VALUE;
    }

    var SCRIPT_URL = Script.resolvePath("./entity_scripts/magneticBlock.js");

    var frontVector = Quat.getFront(MyAvatar.orientation);
    frontVector.y -=.25;
    for(var x =0; x < 3; x++) {
        for (var y = 0; y < 3; y++) {

            var frontOffset = {
                x: 0,
                y: SIZE * y + SIZE,
                z: SIZE * x + SIZE
            };

            Entities.addEntity({
                type: "Box",
                name: "MagneticBlock-" + y +'-' + x,
                dimensions: {
                    x: SIZE,
                    y: SIZE,
                    z: SIZE
                },
                userData: JSON.stringify({grabbableKey: { cloneable: true, grabbable: true, cloneLifetime : LIFETIME, cloneLimit: 9999}}),
                position: Vec3.sum(MyAvatar.position, Vec3.sum(frontOffset, frontVector)),
                color: newColor(),
                script: SCRIPT_URL
            });
        }
    }

    Script.stop();
})();
