// 
//  solarsystem.js
//  games 
//  
//  Created by Bridget Went, 5/28/15.
//  Copyright 2015 High Fidelity, Inc.
//  
//  The start to a project to build a virtual physics classroom to simulate the solar system, gravity, and orbital physics. 
//      - A sun with oribiting planets is created in front of the user
//      - UI elements allow for adjusting the period, gravity, trails, and energy recalculations
//      - Click "PAUSE" to pause the animation and show planet labels
//      - In this mode, double-click a planet label to zoom in on that planet
//              -Double-clicking on earth label initiates satellite orbiter game
//              -Press "TAB" to toggle back to solar system view
//
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include([
    '../utilities/tools/cookies.js',
    'games/satellite.js'
]);

var DAMPING = 0.0;
var LIFETIME = 6000;
var BASE_URL = "https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/planets/";

// Save intiial avatar and camera position
var startingPosition = {x: 800, y: 800, z: 800};
MyAvatar.position = startingPosition;
var startFrame = Window.location.href;


var panelPosition = {x: 300, y: 300};
var panelWidth = 100;
var panelHeight = 300;

var panelBackground = Overlays.addOverlay("text", {
        backgroundColor: {
            red: 200,
            green: 200,
            blue: 255
        },
        x: panelPosition.x,
        y: panelPosition.y,
        width: panelWidth,
        height: panelHeight,
        alpha: 1.0,
        backgroundAlpha: 0.5, visible: true
});



// Place the sun
var MAX_RANGE = 80.0;
var SUN_SIZE = 8.0;
var center = Vec3.sum(startingPosition, Vec3.multiply(MAX_RANGE, Quat.getFront(Camera.getOrientation())));

var theSun = Entities.addEntity({
    type: "Model",
    modelURL: BASE_URL + "sun.fbx",
    position: center,
    dimensions: {
        x: SUN_SIZE,
        y: SUN_SIZE,
        z: SUN_SIZE
    },
    angularVelocity: {
        x: 0.0,
        y: 0.1,
        z: 0.0
    },
    angularDamping: DAMPING,
    damping: DAMPING,
    ignoreCollisions: false,
    lifetime: LIFETIME,
    collisionsWillMove: false
});



// Reference values for physical constants
var referenceRadius = 7.0;
var referencePeriod = 1.0;
var LARGE_BODY_MASS = 250.0;
var SMALL_BODY_MASS = LARGE_BODY_MASS * 0.000000333;
var GRAVITY = (Math.pow(referenceRadius, 3.0) / Math.pow((referencePeriod / (2.0 * Math.PI)), 2.0)) / LARGE_BODY_MASS;
var REFERENCE_GRAVITY = GRAVITY;

var planets = [];
var planetCount = 0;

var TRAILS_ENABLED = true;
var MAX_POINTS_PER_LINE = 20;

var Planet = function(name, trailColor, radius, size) {
    this.index = 0;
    this.name = name;
    this.trailColor = trailColor;
    
    this.trail = [];
    this.lineStack = [];

    this.radius = radius;
    this.position = Vec3.sum(center, { x: this.radius, y: 0.0, z: 0.0 });
    this.period = (2.0 * Math.PI) * Math.sqrt(Math.pow(this.radius, 3.0) / (GRAVITY * LARGE_BODY_MASS));
    this.gravity = GRAVITY;
    this.initialVelocity = Math.sqrt((GRAVITY * LARGE_BODY_MASS) / radius);
    this.velocity = Vec3.multiply(this.initialVelocity, Vec3.normalize({
                x: 0,
                y: -0.2,
                z: 0.9
            }));
    this.dimensions = size;

    this.planet = Entities.addEntity({
            type: "Model",
            modelURL: BASE_URL + name + ".fbx",
            position: this.position,
            dimensions: {
                x: this.dimensions,
                y: this.dimensions,
                z: this.dimensions
            },
            velocity: this.velocity,
            angularDamping: DAMPING,
            damping: DAMPING,
            ignoreCollisions: false,
            lifetime: LIFETIME,
            collisionsWillMove: true,
    });
    
    this.computeAcceleration = function() {
        var acc = -(this.gravity * LARGE_BODY_MASS) * Math.pow(this.radius, (-2.0));
        return acc;
    };

    this.update = function(timeStep) {
        var between = Vec3.subtract(this.position, center);
        var speed = this.computeAcceleration(this.radius) * timeStep;
        var vel = Vec3.multiply(speed, Vec3.normalize(between));

        // Update velocity and position
        this.velocity = Vec3.sum(this.velocity, vel);
        this.position = Vec3.sum(this.position, Vec3.multiply(timeStep, this.velocity));
        Entities.editEntity(this.planet, {
            velocity: this.velocity,
            position: this.position
        });

        if (TRAILS_ENABLED) {
            this.updateTrail();
        }    
    };

    this.resetTrails = function() {
        elapsed = 0.0;

        this.trail = [];
        this.lineStack = [];
        //add the first line to both the line entity stack and the trail
        this.trail.push(newLine(this.lineStack, this.position, this.period, this.lineColor));

    };

    this.updateTrail = function() {
        var point = this.position;

        var prop = Entities.getEntityProperties(this.lineStack[this.lineStack.length - 1]);
        var linePos = prop.position;

        this.trail.push(computeLocalPoint(linePos, point));

        Entities.editEntity(this.lineStack[this.lineStack.length - 1], {
                linePoints: this.trail
        });
        if (this.trail.length === MAX_POINTS_PER_LINE) {
            this.trail = newLine(this.lineStack, point, this.period, this.lineColor);
        }
    };

    this.adjustPeriod = function(alpha) {
        // Update global G constant, period, poke velocity to new value
        var ratio = this.last_alpha / alpha;
        this.gravity = Math.pow(ratio, 2.0) * GRAVITY;
        this.period = ratio * this.period;
        this.velocity = Vec3.multiply(ratio, this.velocity);
        
        this.last_alpha = alpha;
    }

    this.index++;
}

planets.push(new Planet("Mercury", {red: 255, green: 255, blue: 255}, 7.0, 1.0));
planets.push(new Planet("Venus", {red: 255, green: 160, blue: 110}, 8.0, 1.2));
planets.push(new Planet("Earth", {red: 10, green: 150, blue: 160}, 9.2, 1.6));
planets.push(new Planet("Mars", {red: 180, green: 70, blue: 10}, 11.0, 2.0));
planets.push(new Planet("Jupiter", {red: 250, green: 140, blue: 0}, 14.5, 4.3));
planets.push(new Planet("Saturn", {red: 235, green: 215, blue: 0}, 21.0, 3.7));
planets.push(new Planet("Uranus", {red: 135, green: 205, blue: 240}, 29.0, 4.0));
planets.push(new Planet("Neptune", {red: 30, green: 140, blue: 255}, 35.0, 4.2));
planets.push(new Planet("Pluto", {red: 255, green: 255, blue: 255}, 58.0, 3.2));


var labels = [];
var labelLines = [];
var labelsShowing = false;
var LABEL_X = 8.0;
var LABEL_Y = 3.0;
var LABEL_Z = 1.0;
var LABEL_DIST = 8.0;
var TEXT_HEIGHT = 1.0;

var PlanetLabel = function(name, index) {
    var text = name + "                    Speed: " + Vec3.length(planets[index].velocity).toFixed(2);
    var labelPos = Vec3.sum(planets[index].position, { x: 0.0, y: LABEL_DIST, z: LABEL_DIST });
    var linePos = planets[i].position;
    
    this.line = Entities.addEntity({
            type: "Line",
            position: linePos,
            dimensions: {
                x: 20,
                y: 20,
                z: 20
            },
            lineWidth: LINE_WIDTH,
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            linePoints: [{
                x: 0,
                y: 0,
                z: 0
            }, 
            computeLocalPoint(linePos, labelPos)], 
            visible: false
        });

    this.label = Entities.addEntity({
            type: "Text",
            text: text,
            lineHeight: TEXT_HEIGHT,
            dimensions: {
                x: LABEL_X,
                y: LABEL_Y,
                z: LABEL_Z
            },
            position: labelPos,
            backgroundColor: {
                red: 10,
                green: 10,
                blue: 10
            },
            faceCamera: true,
            visible: false
        }); 
    labelLines.push(line);
    labels.push(label); 

    this.show = function() {
        this.showing = true;
        Entities.editEntity(this.line, {visible: true});
        Entities.editEntity(this.label, {visible: true});
    }

    this.hide = function() {
        this.showing = false;
        Entities.editEntity(this.line, {visible: false});
        Entities.editEntity(this.label, {visible: false});
    }

}
    
var time = 0.0;
var elapsed;
var TIME_STEP = 60.0;
var dt = 1.0 / TIME_STEP;

var planetLines = [];
var trails = [];


function update(deltaTime) {
    // if (paused) {
    //     return;
    // }
    deltaTime = dt;
    time++;

    for (var i = 0; i < planets.length; ++i) {
        planets[i].update(deltaTime);
    }
    if (time % TIME_STEP == 0) {
        elapsed++;
    }
}

function computeLocalPoint(linePos, worldPoint) {
    var localPoint = Vec3.subtract(worldPoint, linePos);
    return localPoint;
}

// Create a new line
function newLine(lineStack, point, period, color) {
    if (elapsed < period) {
        var line = Entities.addEntity({
            position: point,
            type: "Line",
            color: color,
            dimensions: {
                x: LINE_DIM,
                y: LINE_DIM,
                z: LINE_DIM
            },
            lifetime: LIFETIME,
            lineWidth: LINE_WIDTH
        });
        lineStack.push(line);
    } else {
        // Begin overwriting first lines after one full revolution (one period)
        var firstLine = lineStack.shift();
        Entities.editEntity(firstLine, {
            position: point,
            linePoints: [{
                x: 0.0,
                y: 0.0,
                z: 0.0
            }]
        });
        lineStack.push(firstLine);

    }
    var points = [];
    points.push(computeLocalPoint(point, point));
    return points;
}

// function mousePressEvent(event) {
//     var clickedOverlay = Overlays.getOverlayAtPoint({
//         x: event.x,
//         y: event.y
//     });
//     if (clickedOverlay == pauseButton) {
//         paused = !paused;
//         for (var i = 0; i < NUM_PLANETS; ++i) {
//             Entities.editEntity(planets[i], {
//                 velocity: {
//                     x: 0.0,
//                     y: 0.0,
//                     z: 0.0
//                 }
//             });
//         }
//         if (paused && !labelsShowing) {
//             Overlays.editOverlay(pauseButton, {
//                 text: "Paused",
//                 backgroundColor: {
//                     red: 255,
//                     green: 50,
//                     blue: 50
//                 }
//             });
//             showLabels();
//         }
//         if (paused == false && labelsShowing) {
//             Overlays.editOverlay(pauseButton, {
//                 text: "Pause",
//                 backgroundColor: {
//                     red: 200,
//                     green: 200,
//                     blue: 255
//                 }
//             });
//             hideLabels();
//         }
//         planetView = false;
//     }
// }

// function keyPressEvent(event) {
//     // Jump back to solar system view
//     if (event.text == "TAB" && planetView) {
//         if (earthView) {
//             satelliteGame.endGame();
//             earthView = false;
//         }
//         MyAvatar.position = startingPosition;
//     }
// }

// //switch to planet view
// function mouseDoublePressEvent(event) {
//     if (earthView) {
//         return;
//     }
//     var pickRay = Camera.computePickRay(event.x, event.y)
//     var rayPickResult = Entities.findRayIntersection(pickRay, true);

//     for (var i = 0; i < NUM_PLANETS; ++i) {
//         if (rayPickResult.entityID === labels[i]) {
//             planetView = true;
//             if (i == 2) {
//                 MyAvatar.position = Vec3.sum(center, {
//                     x: 200,
//                     y: 200,
//                     z: 200
//                 });
//                 Camera.setPosition(Vec3.sum(center, {
//                     x: 200,
//                     y: 200,
//                     z: 200
//                 }));
//                 earthView = true;
//                 satelliteGame = new SatelliteGame();

//             } else {
//                 MyAvatar.position = Vec3.sum({
//                     x: 0.0,
//                     y: 0.0,
//                     z: 3.0
//                 }, planet_properties[i].position);
//                 Camera.lookAt(planet_properties[i].position);
//             }
//             break;
//         }
//     }
// }

// // UI ELEMENTS




// // USE FLOATING UI PANEL TO IMPROVE UI EXPERIENCE

// var paused = false;

// // Create UI panel
// var panel = new Panel(PANEL_X, PANEL_Y);

// var g_multiplier = 1.0;
// panel.newSlider("Adjust Gravitational Force: ", 0.1, 5.0,
//     function(value) {
//         g_multiplier = value;
//         G = G_ref * g_multiplier;
//     },

//     function() {
//         return g_multiplier;
//     },
//     function(value) {
//         return value.toFixed(1) + "x";
//     });

// var period_multiplier = 1.0;
// var last_alpha = period_multiplier;

// panel.newSubPanel("Adjust Orbital Periods");
// /*

//     TODO: Loop through all planets, creating new sliders and adjusting their respective orbital periods


// */

// for (var i = 0; i < planets.length; ++i) 
// panel.newSlider("Adjust Orbital Period: ", 0.1, 3.0,
//     function(value) {
//         period_multiplier = value;
//         changePeriod(period_multiplier);
//     },
//     function() {
//         return period_multiplier;
//     },
//     function(value) {
//         return (value).toFixed(2) + "x";
//     }));

// panel.newCheckbox("Leave Trails: ",
//     function(value) {
//         trailsEnabled = value;
//         if (trailsEnabled) {
//             for (var i = 0; i < NUM_PLANETS; ++i) {
//                 resetTrails(i);
//             }
//             //if trails are off and we've already created trails, remove existing trails
//         } else if (planetLines.length != 0) {
//             for (var i = 0; i < NUM_PLANETS; ++i) {
//                 for (var j = 0; j < planetLines[i].length; ++j) {
//                     Entities.editEntity(planetLines[i][j], {visible: false});
//                 }
//                 planetLines[i] = [];
//             }
//         }
//     },
//     function() {
//         return trailsEnabled;
//     },
//     function(value) {
//         return value;
//     }));




// Clean up models, UI panels, lines, and button overlays
function scriptEnding() {
    satelliteGame.endGame();

    Entities.deleteEntity(theSun);
    for (var i = 0; i < NUM_PLANETS; ++i) {
        Entities.deleteEntity(planets[i]);
    }

    Menu.removeMenu("Developer > Scene");
    panel.destroy();
    Overlays.deleteOverlay(pauseButton);

    var e = Entities.findEntities(MyAvatar.position, 16000);
    for (i = 0; i < e.length; i++) {
        var props = Entities.getEntityProperties(e[i]);
        if (props.type === "Line" || props.type === "Text") {
            Entities.deleteEntity(e[i]);
        }
    }
};


// Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) {
//     return panel.mouseMoveEvent(event);
// });
// Controller.mousePressEvent.connect(function panelMousePressEvent(event) {
//     return panel.mousePressEvent(event);
// });
// Controller.mouseDoublePressEvent.connect(function panelMouseDoublePressEvent(event) {
//     return panel.mouseDoublePressEvent(event);
// });
// Controller.mouseReleaseEvent.connect(function(event) {
//     return panel.mouseReleaseEvent(event);
// });
// Controller.mousePressEvent.connect(mousePressEvent);
// Controller.mouseDoublePressEvent.connect(mouseDoublePressEvent);
// Controller.keyPressEvent.connect(keyPressEvent);

Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);