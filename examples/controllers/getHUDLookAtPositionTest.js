//
//  getHUDLookAtPositionTest.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2015/12/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// This script demonstrates the testing of the HMD.getHUDLookAtPosition--() functions.
// If these functions are working correctly, we'd expect to see a 3D cube and a 2D square
// follow around the center of the HMD view.

var cubePosition = { x: 0, y: 0, z: 0 };
var cubeSize = 0.03;
var cube = Overlays.addOverlay("cube", {
                    position: cubePosition,
                    size: cubeSize,
                    color: { red: 0, green: 255, blue: 0},
                    alpha: 1,
                    solid: false
                });

var SQUARE_SIZE = 20;

var square = Overlays.addOverlay("rectangle", {
                    x: 0,
                    y: 0,
                    width: SQUARE_SIZE,
                    height: SQUARE_SIZE,
                    color: { red: 0, green: 255, blue: 0},
                    backgroundColor: { red: 0, green: 255, blue: 0},
                    alpha: 1
                });


Script.update.connect(function(deltaTime) {
    if (!HMD.active) {
        return;
    }
    var lookAt3D = HMD.getHUDLookAtPosition3D();
    Overlays.editOverlay(cube, { position: lookAt3D });

    var lookAt2D = HMD.getHUDLookAtPosition2D();
    Overlays.editOverlay(square, { x: lookAt2D.x - (SQUARE_SIZE/2), y: lookAt2D.y  - (SQUARE_SIZE/2)});
});

Script.scriptEnding.connect(function(){
    Overlays.deleteOverlay(cube);
    Overlays.deleteOverlay(square);
});


