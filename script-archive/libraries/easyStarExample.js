//
//  easyStarExample.js
//
//  Created by James B. Pollack @imgntn on 11/9/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This is a script that sets up a grid of obstacles and passable tiles, finds a path, and then moves an entity along the path.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

//  To-Do:
//  Abstract start position and make tiles, spheres, etc. relative
//  Handle dynamically changing grids

Script.include('easyStar.js');
var easystar = loadEasyStar();
Script.include('tween.js');
var TWEEN = loadTween();

var ANIMATION_DURATION = 350;
var grid = [
    [0, 0, 1, 0, 0, 0, 0, 0, 0],
    [0, 1, 1, 0, 1, 0, 1, 0, 0],
    [0, 0, 1, 0, 0, 0, 0, 1, 1],
    [0, 0, 1, 1, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 1, 1, 1, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0, 0]
];


easystar.setGrid(grid);

easystar.setAcceptableTiles([0]);
easystar.enableCornerCutting();
easystar.findPath(0, 0, 8, 0, function(path) {
    if (path === null) {
        print("Path was not found.");
        Script.update.disconnect(tickEasyStar);
    } else {
        print('path' + JSON.stringify(path));

        convertPath(path);
        Script.update.disconnect(tickEasyStar);
    }
});

var tickEasyStar = function() {
    easystar.calculate();
}

Script.update.connect(tickEasyStar);

//a sphere that will get moved
var playerSphere = Entities.addEntity({
    type: 'Sphere',
    shape: 'Sphere',
    color: {
        red: 255,
        green: 0,
        blue: 0
    },
    dimensions: {
        x: 0.5,
        y: 0.5,
        z: 0.5
    },
    position: {
        x: 0,
        y: 0,
        z: 0
    },
    gravity: {
        x: 0,
        y: -9.8,
        z: 0
    },
    dynamic: true,
    damping: 0.2
});

Script.setInterval(function(){
    Entities.editEntity(playerSphere,{
        velocity:{
            x:0,
            y:4,
            z:0
        }
    })
},1000)

var sphereProperties;

//for keeping track of entities
var obstacles = [];
var passables = [];

function createPassableAtTilePosition(posX, posY) {
    var properties = {
        type: 'Box',
        shapeType: 'Box',
        dimensions: {
            x: 1,
            y: 1,
            z: 1
        },
        position: {
            x: posY,
            y: -1,
            z: posX
        },
        color: {
            red: 0,
            green: 0,
            blue: 255
        }
    };
    var passable = Entities.addEntity(properties);
    passables.push(passable);
}



function createObstacleAtTilePosition(posX, posY) {
    var properties = {
        type: 'Box',
        shapeType: 'Box',
        dimensions: {
            x: 1,
            y: 2,
            z: 1
        },
        position: {
            x: posY,
            y: 0,
            z: posX
        },
        color: {
            red: 0,
            green: 255,
            blue: 0
        }
    };
    var obstacle = Entities.addEntity(properties);
    obstacles.push(obstacle);
}

function createObstacles(grid) {
    grid.forEach(function(row, rowIndex) {
        row.forEach(function(v, index) {
            if (v === 1) {
                createObstacleAtTilePosition(rowIndex, index);
            }
            if (v === 0) {
                createPassableAtTilePosition(rowIndex, index);
            }
        })

    })
}



createObstacles(grid);


var currentSpherePosition = {
    x: 0,
    y: 0,
    z: 0
};


function convertPathPointToCoordinates(x, y) {
    return {
        x: y,
        z: x
    };
}

var convertedPath = [];

//convert our path to Vec3s
function convertPath(path) {
    path.forEach(function(point) {
        var convertedPoint = convertPathPointToCoordinates(point.x, point.y);
        convertedPath.push(convertedPoint);
    });
    createTweenPath(convertedPath);
}

function updatePosition() {
    sphereProperties = Entities.getEntityProperties(playerSphere, "position");

    Entities.editEntity(playerSphere, {
        position: {
            x: currentSpherePosition.z,
            y: sphereProperties.position.y,
            z: currentSpherePosition.x
        }

    });
}

var upVelocity = {
    x: 0,
    y: 2.5,
    z: 0
}

var noVelocity = {
    x: 0,
    y: -3.5,
    z: 0
}

function createTweenPath(convertedPath) {
    var i;
    var stepTweens = [];
    var velocityTweens = [];

    //create the tweens
    for (i = 0; i < convertedPath.length - 1; i++) {
        var stepTween = new TWEEN.Tween(currentSpherePosition).to(convertedPath[i + 1], ANIMATION_DURATION).onUpdate(updatePosition).onComplete(tweenStep);


        stepTweens.push(stepTween);
    }

    var j;
    //chain one tween to the next
    for (j = 0; j < stepTweens.length - 1; j++) {
        stepTweens[j].chain(stepTweens[j + 1]);
    }

    //start the tween
    stepTweens[0].start();

}

var velocityShaper = {
    x: 0,
    y: 0,
    z: 0
}

function tweenStep() {
    // print('step between tweens')
}

function updateTweens() {
    //hook tween updates into our update loop
    TWEEN.update();
}

Script.update.connect(updateTweens);

function cleanup() {
    while (obstacles.length > 0) {
        Entities.deleteEntity(obstacles.pop());
    }
    while (passables.length > 0) {
        Entities.deleteEntity(passables.pop());
    }
    Entities.deleteEntity(playerSphere);
    Script.update.disconnect(updateTweens);
}


Script.scriptEnding.connect(cleanup);
