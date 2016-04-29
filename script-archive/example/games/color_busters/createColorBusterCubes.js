//
//  createColorBusterCubes.js
//
//  Created by James B. Pollack @imgntn on 11/2/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates cubes that can be removed with a Color Buster wand.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var DELETE_AT_ENDING = false;

var CUBE_DIMENSIONS = {
    x: 1,
    y: 1,
    z: 1
};

var NUMBER_OF_CUBES_PER_SIDE = 8;

var STARTING_CORNER_POSITION = {
    x: 100,
    y: 100,
    z: 100
};
var STARTING_COLORS = [
    ['red', {
        red: 255,
        green: 0,
        blue: 0
    }],
    ['yellow', {
        red: 255,
        green: 255,
        blue: 0
    }],
    ['blue', {
        red: 0,
        green: 0,
        blue: 255
    }],
    ['orange', {
        red: 255,
        green: 165,
        blue: 0
    }],
    ['violet', {
        red: 128,
        green: 0,
        blue: 128
    }],
    ['green', {
        red: 0,
        green: 255,
        blue: 0
    }]
];

function chooseStartingColor() {
    var startingColor = STARTING_COLORS[Math.floor(Math.random() * STARTING_COLORS.length)];
    return startingColor;
}

var cubes = [];

function createColorBusterCube(row, column, vertical) {

    print('make cube at ' + row + ':' + column + ":" + vertical);

    var position = {
        x: STARTING_CORNER_POSITION.x + row,
        y: STARTING_CORNER_POSITION.y + vertical,
        z: STARTING_CORNER_POSITION.z + column
    };

    var startingColor = chooseStartingColor();
    var colorBusterCubeProperties = {
        name: 'Hifi-ColorBusterCube',
        type: 'Box',
        dimensions: CUBE_DIMENSIONS,
        dynamic: false,
        collisionless: false,
        color: startingColor[1],
        position: position,
        userData: JSON.stringify({
            hifiColorBusterCubeKey: {
                originalColorName: startingColor[0]
            },
            grabbableKey: {
                grabbable: false
            }
        })
    };
    var cube = Entities.addEntity(colorBusterCubeProperties);
    cubes.push(cube);
    return cube
}

function createBoard() {
    var vertical;
    var row;
    var column;
    for (vertical = 0; vertical < NUMBER_OF_CUBES_PER_SIDE; vertical++) {
        print('vertical:' + vertical)
            //create a single layer
        for (row = 0; row < NUMBER_OF_CUBES_PER_SIDE; row++) {
            print('row:' + row)
            for (column = 0; column < NUMBER_OF_CUBES_PER_SIDE; column++) {
                print('column:' + column)
                createColorBusterCube(row, column, vertical)
            }
        }
    }
}

function deleteCubes() {
    while (cubes.length > 0) {
        Entities.deleteEntity(cubes.pop());
    }
}

if (DELETE_AT_ENDING === true) {
    Script.scriptEnding.connect(deleteCubes);

}

createBoard();
