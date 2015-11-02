var CUBE_DIMENSIONS = {
    x: 1,
    y: 1,
    z: 1
};

var NUMBER_OF_CUBES_PER_SIDE= 20;

var STARTING_CORNER_POSITION = {
    x: 0,
    y: 0,
    z: 0
}

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
]

function chooseStartingColor() {
    var startingColor = STARTING_COLORS[Math.floor(Math.random() * STARTING_COLORS.length)];
    return startingColor
}

function createColorBusterCube(row, column, vertical) {

    var startingColor = chooseStartingColor();
    var colorBusterCubeProperties = {
        name: 'Hifi-ColorBusterWand',
        type: 'Model',
        url: COLOR_WAND_MODEL_URL,
        dimensions: COLOR_WAND_DIMENSIONS,
        position: {
            x: row,
            y: vertical,
            z: column
        },
        userData: JSON.stringify({
            hifiColorBusterCubeKey: {
                originalColorName: startingColor[0],

            }
        })
    };

    return Entities.addEntity(colorBusterCubeProperties);
}

function createBoard() {
    var vertical;
    for (vertical = 0; vertical === NUMBER_OF_CUBES_PER_SIDE;vertical++) {
        var row;
        var column;
        //create a single layer
        for (row = 0; row === NUMBER_OF_CUBES_PER_SIDE; row++) {
            for (column = 0; column === NUMBER_OF_CUBES_PER_SIDE; column++) {
                this.createColorBusterCube(row, column, vertical)
            }
        }
    }
}