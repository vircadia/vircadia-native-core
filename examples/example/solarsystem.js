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
    'games/satellite.js'
]);

var DAMPING = 0.0;
var LIFETIME = 6000;
var BASE_URL = "https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/planets/";

// Save intiial avatar and camera position
var startingPosition = {
    x: 8000,
    y: 8000,
    z: 8000
};
MyAvatar.position = startingPosition;
var cameraStart = Camera.getOrientation();


// Place the sun
var MAX_RANGE = 80.0;
var center = Vec3.sum(startingPosition, Vec3.multiply(MAX_RANGE, Vec3.normalize(Quat.getFront(Camera.getOrientation()))));
var SUN_SIZE = 7.0;

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
var referenceRadius = 15.0;
var referenceDiameter = 0.6;
var referencePeriod = 3.0;
var LARGE_BODY_MASS = 250.0;
var SMALL_BODY_MASS = LARGE_BODY_MASS * 0.000000333;
var GRAVITY = (Math.pow(referenceRadius, 3.0) / Math.pow((referencePeriod / (2.0 * Math.PI)), 2.0)) / LARGE_BODY_MASS;
var REFERENCE_GRAVITY = GRAVITY;

var planets = [];
var planetCount = 0;

var trailsEnabled = true;
var MAX_POINTS_PER_LINE = 60;
var LINE_DIM = 200;
var LINE_WIDTH = 20;

var VELOCITY_OFFSET_Y = -0.3;
var VELOCITY_OFFSET_Z = 0.9;

var index = 0;

var Planet = function(name, trailColor, radiusScale, sizeScale) {

    this.name = name;
    this.trailColor = trailColor;

    this.trail = [];
    this.lineStack = [];

    this.radius = radiusScale * referenceRadius;
    this.position = Vec3.sum(center, {
        x: this.radius,
        y: 0.0,
        z: 0.0
    });
    this.period = (2.0 * Math.PI) * Math.sqrt(Math.pow(this.radius, 3.0) / (GRAVITY * LARGE_BODY_MASS));
    this.gravity = GRAVITY;
    this.initialVelocity = Math.sqrt((GRAVITY * LARGE_BODY_MASS) / this.radius);
    this.velocity = Vec3.multiply(this.initialVelocity, Vec3.normalize({
        x: 0,
        y: VELOCITY_OFFSET_Y,
        z: VELOCITY_OFFSET_Z
    }));
    this.dimensions = sizeScale * referenceDiameter;
    this.sizeScale = sizeScale;

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

        if (trailsEnabled) {
            this.updateTrail();
        }
    };

    this.resetTrails = function() {
        elapsed = 0.0;

        this.trail = [];
        this.lineStack = [];
        //add the first line to both the line entity stack and the trail
        this.trail.push(newLine(this.lineStack, this.position, this.period, this.trailColor));

    };

    this.updateTrail = function() {
        var point = this.position;
        var linePos = Entities.getEntityProperties(this.lineStack[this.lineStack.length - 1]).position;

        this.trail.push(computeLocalPoint(linePos, point));

        Entities.editEntity(this.lineStack[this.lineStack.length - 1], {
            linePoints: this.trail
        });

        if (this.trail.length == MAX_POINTS_PER_LINE) {
            this.trail = newLine(this.lineStack, point, this.period, this.trailColor);
        }
    };

    this.zoom = function() {
        Script.include('file:///Users/bridget/Desktop/tween.js');
        init(this);
        // var viewingRange = sizeScale * 2.0;
        // var direction = Vec3.subtract(this.position, MyAvatar.position);
        // var dist = Vec3.length(direction);
        // var timer = 0;
        // while (dist > viewingRange && timer < 8000000) {
        //     timer++;
        //     if (timer % 10000 == 0) {
        //         MyAvatar.position = Vec3.sum(MyAvatar.position, Vec3.normalize(direction));
        //         direction = Vec3.subtract(this.position, MyAvatar.position);
        //         dist = Vec3.length(direction);
        //     }    
        // }
    };


    this.adjustPeriod = function(alpha) {
        // Update global G constant, period, poke velocity to new value
        var ratio = this.last_alpha / alpha;
        this.gravity = Math.pow(ratio, 2.0) * GRAVITY;
        this.period = ratio * this.period;
        this.velocity = Vec3.multiply(ratio, this.velocity);

        this.last_alpha = alpha;
    };


    index++;
    this.resetTrails();

}

// rideOrbit = function() {

//         Script.include('file:///Users/bridget/Desktop/tween.js');
//         init();
    
// }


var MERCURY_LINE_COLOR = {
    red: 255,
    green: 255,
    blue: 255
};
var VENUS_LINE_COLOR = {
    red: 255,
    green: 160,
    blue: 110
};
var EARTH_LINE_COLOR = {
    red: 10,
    green: 150,
    blue: 160
};
var MARS_LINE_COLOR = {
    red: 180,
    green: 70,
    blue: 10
};
var JUPITER_LINE_COLOR = {
    red: 250,
    green: 140,
    blue: 0
};
var SATURN_LINE_COLOR = {
    red: 235,
    green: 215,
    blue: 0
};
var URANUS_LINE_COLOR = {
    red: 135,
    green: 205,
    blue: 240
};
var NEPTUNE_LINE_COLOR = {
    red: 30,
    green: 140,
    blue: 255
};
var PLUTO_LINE_COLOR = {
    red: 255,
    green: 255,
    blue: 255
};

planets.push(new Planet("mercury", MERCURY_LINE_COLOR, 0.387, 0.383));
planets.push(new Planet("venus", VENUS_LINE_COLOR, 0.723, 0.949));
planets.push(new Planet("earth", EARTH_LINE_COLOR, 1.0, 1.0));
planets.push(new Planet("mars", MARS_LINE_COLOR, 1.52, 0.532));
planets.push(new Planet("jupiter", JUPITER_LINE_COLOR, 5.20, 11.21));
planets.push(new Planet("saturn", SATURN_LINE_COLOR, 9.58, 9.45));
planets.push(new Planet("uranus", URANUS_LINE_COLOR, 19.20, 4.01));
planets.push(new Planet("neptune", NEPTUNE_LINE_COLOR, 30.05, 3.88));
planets.push(new Planet("pluto", PLUTO_LINE_COLOR, 39.48, 0.186));

var LABEL_X = 8.0;
var LABEL_Y = 3.0;
var LABEL_Z = 1.0;
var LABEL_DIST = 8.0;
var TEXT_HEIGHT = 1.0;

var PlanetLabel = function(name, index) {
    var text = name + "                    Speed: " + Vec3.length(planets[index].velocity).toFixed(2);
    this.labelPos = Vec3.sum(planets[index].position, {
        x: 0.0,
        y: LABEL_DIST,
        z: LABEL_DIST
    });
    this.linePos = planets[index].position;

    this.line = Entities.addEntity({
        type: "Line",
        position: this.linePos,
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
            computeLocalPoint(this.linePos, this.labelPos)
        ],
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
        position: this.labelPos,
        backgroundColor: {
            red: 10,
            green: 10,
            blue: 10
        },
        faceCamera: true,
        visible: false
    });

    this.show = function() {
        this.showing = true;
        Entities.editEntity(this.line, {
            visible: true
        });
        Entities.editEntity(this.label, {
            visible: true
        });
    }

    this.hide = function() {
        this.showing = false;
        Entities.editEntity(this.line, {
            visible: false
        });
        Entities.editEntity(this.label, {
            visible: false
        });
    }
}


var time = 0.0;
var elapsed;
var TIME_STEP = 60.0;
var dt = 1.0 / TIME_STEP;

var planetLines = [];
var trails = [];
var paused = false;

function update(deltaTime) {
    if (paused) {
        return;
    }
    //deltaTime = dt;
    time++;
    if (time % TIME_STEP == 0) {
        elapsed++;
    }

    for (var i = 0; i < planets.length; ++i) {
        planets[i].update(deltaTime);
    }
}

function pause() {
    for (var i = 0; i < planets.length; ++i) {
        Entities.editEntity(planets[i].planet, {
            velocity: {
                x: 0.0,
                y: 0.0,
                z: 0.0
            }
        });
        planets[i].label = new PlanetLabel(planets[i].name, i);
        planets[i].label.show();
    }
    paused = true;
}

function resume() {
    for (var i = 0; i < planets.length; ++i) {
        planets[i].label.hide();
    }
    paused = false;
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



var spacing = 8;
var textWidth = 70;
var buttonWidth = 30;
var buttonHeight = 30;

var ICONS_URL = 'https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/images/';

var UIPanel = function(x, y, orientation) {
    this.visible = false;
    this.buttons = [];
    this.x = x;
    this.y = y;
    this.nextX = x + spacing;
    this.nextY = y + spacing;
    this.width = spacing * 2.0;
    this.height = spacing * 2.0;

    this.background = Overlays.addOverlay("text", {
        backgroundColor: {
            red: 240,
            green: 240,
            blue: 255
        },
        x: this.x,
        y: this.y,
        width: this.width,
        height: this.height,
        alpha: 1.0,
        backgroundAlpha: 0.7,
        visible: false
    });

    this.addIcon = function(iconID) {
        var icon = Overlays.addOverlay("image", {
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            imageURL: ICONS_URL + iconID + '.svg',
            x: this.nextX,
            y: this.nextY,
            width: buttonWidth,
            height: buttonHeight,
            alpha: 1.0,
            visible: false
        });


        if (orientation === 'horizontal') {
            this.nextX += buttonWidth + spacing;
            this.width += buttonWidth;

        } else if (orientation === 'vertical') {
            this.nextY += buttonHeight + spacing;
            this.height += buttonHeight;
        }

        Overlays.editOverlay(this.background, {
            width: buttonWidth + this.width,
            height: buttonHeight + this.height
        });

        this.buttons.push(icon);
        return icon;
    };

    this.addText = function(text) {
        var icon = Overlays.addOverlay("text", {
            color: {
                red: 240,
                green: 240,
                blue: 255
            },
            text: text,
            x: this.nextX,
            y: this.nextY,
            width: textWidth,
            height: buttonHeight,
            visible: false
        });

        if (orientation === 'horizontal') {
            this.nextX += textWidth + spacing;
            this.width += textWidth;

        } else if (orientation === 'vertical') {
            this.nextY += buttonHeight + spacing;
            this.height += buttonHeight;
        }

        Overlays.editOverlay(this.background, {
            width: textWidth + this.width,
            height: buttonHeight + this.height
        });

        this.buttons.push(icon);
        return icon;
    };


    this.show = function() {
        Overlays.editOverlay(this.background, {
            visible: true
        });
        for (var i in this.buttons) {
            Overlays.editOverlay(this.buttons[i], {
                visible: true
            });
        }
        this.visible = true;
    };

    this.hide = function() {
        Overlays.editOverlay(this.background, {
            visible: false
        });
        for (var i in this.buttons) {
            Overlays.editOverlay(this.buttons[i], {
                visible: false
            });
        }
        this.visible = false;
    };

    this.remove = function() {
        Overlays.deleteOverlay(this.background);
        for (var i in this.buttons) {
            Overlays.deleteOverlay(this.buttons[i]);
        }
    };

}

var panelX = 1250;
var panelY = 500;
var panelWidth = 50;
var panelHeight = 210;

var mainPanel = new UIPanel(panelX, panelY, 'vertical');

var systemViewButton = mainPanel.addIcon('solarsystems');
var systemViewPanel = new UIPanel(panelX - 140, panelY, 'horizontal');
var reverseButton = systemViewPanel.addIcon('reverse');
var pauseButton = systemViewPanel.addIcon('playpause');
var forwardButton = systemViewPanel.addIcon('forward');

var zoomButton = mainPanel.addIcon('magnifier');
var zoomPanel = new UIPanel(panelX - 900, panelY + (buttonHeight + spacing) * 4.0, 'horizontal');
for (var i = 0; i < planets.length; ++i) {
    zoomPanel.addText(planets[i].name);
}

var satelliteButton = mainPanel.addIcon('satellite');
var settingsButton = mainPanel.addIcon('settings');
var stopButton = mainPanel.addIcon('close');

mainPanel.show();

Script.include('../utilities/tools/cookies.js');


var satelliteView;
var satelliteGame;
 var settings = new Panel(panelX - 610, panelY - 80);

            var g_multiplier = 1.0;
            settings.newSlider("Gravitational Force ", 0.1, 5.0,
                function(value) {
                    g_multiplier = value;
                    GRAVITY = REFERENCE_GRAVITY * g_multiplier;
                },

                function() {
                    return g_multiplier;
                },
                function(value) {
                    return value.toFixed(1) + "x";
                });


            var subPanel = settings.newSubPanel("Orbital Periods");

            for (var i = 0; i < planets.length; ++i) {
                planets[i].period_multiplier = 1.0;
                planets[i].last_alpha = planets[i].period_multiplier;

                subPanel.newSlider(planets[i].name, 0.1, 3.0,
                    function(value) {
                        planets[i].period_multiplier = value;
                        planets[i].adjustPeriod(value);
                    },
                    function() {
                        return planets[i].period_multiplier;
                    },
                    function(value) {
                        return (value).toFixed(2) + "x";
                    });
            }
            settings.newCheckbox("Leave Trails: ",
                function(value) {
                    trailsEnabled = value;
                    if (trailsEnabled) {
                        for (var i = 0; i < planets.length; ++i) {
                            planets[i].resetTrails();
                        }
                        //if trails are off and we've already created trails, remove existing trails
                    } else {
                        for (var i = 0; i < planets.length; ++i) {
                            for (var j = 0; j < planets[i].lineStack.length; ++j) {
                                Entities.editEntity(planets[i].lineStack[j], {
                                    visible: false
                                });
                            }
                            planets[i].lineStack = [];
                        }
                    }
                },
                function() {
                    return trailsEnabled;
                },
                function(value) {
                    return value;
                });
 settings.hide();


function mousePressEvent(event) {
   
    var clicked = Overlays.getOverlayAtPoint({
        x: event.x,
        y: event.y
    });

    if (clicked == systemViewButton) {
        MyAvatar.position = startingPosition;
        Camera.setOrientation(cameraStart);
        if (paused) {
            resume();
        }
        
        if (!systemViewPanel.visible) {
            systemViewPanel.show();
        } else {
            systemViewPanel.hide();
        }
    }
    var zoomed = false;
    if (clicked == zoomButton && !satelliteView) {
        if (!zoomPanel.visible) {
            zoomPanel.show();
            pause();
        } else {
            zoomPanel.hide();
            if (zoomed) {
                
                MyAvatar.position = startingPosition;
                Camera.setOrientation(cameraStart);
                resume();
            }

        }
    }

    for (var i = 0; i < planets.length; ++i) {
        if (zoomPanel.visible && clicked == zoomPanel.buttons[i]) {
            planets[i].zoom();
            zoomed = true;
            break;
        }
    }

    if (systemViewPanel.visible && clicked == pauseButton) {
        if (!paused) {
            pause();
        } else {
            resume();
        }
    }



    if (clicked == satelliteButton) {
        if (satelliteView) {
            satelliteGame.endGame();
            MyAvatar.position = startingPosition;
            satelliteView = false;
            resume();
        } else {
            pause();
            var confirmed = Window.confirm("Start satellite game?");
            if (!confirmed) {
                resume();
                continue;
            }
            satelliteView = true;
            MyAvatar.position = {
                x: 200,
                y: 200,
                z: 200
            };
            Camera.setPosition({
                x: 200,
                y: 200,
                z: 200
            });
            satelliteGame = new SatelliteGame();
        }
    }

    if (clicked == settingsButton) {
        if (!settings.visible) {
            settings.show();
        } else {
            settings.hide();
        }
    }

    if (clicked == stopButton) {
        Script.stop();
    }
}


// Clean up models, UI panels, lines, and button overlays
function scriptEnding() {
    mainPanel.remove();
    systemViewPanel.remove();
    zoomPanel.remove();
    
    settings.destroy();
    
    Entities.deleteEntity(theSun);
    for (var i = 0; i < planets.length; ++i) {
        Entities.deleteEntity(planets[i].planet);
    }

    var e = Entities.findEntities(MyAvatar.position, 16000);
    for (i = 0; i < e.length; i++) {
        var props = Entities.getEntityProperties(e[i]);
        if (props.type === "Line" || props.type === "Text") {
            Entities.deleteEntity(e[i]);
        }
    }
};

Controller.mousePressEvent.connect(mousePressEvent);

    Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) {
                return settings.mouseMoveEvent(event);
            });
            Controller.mousePressEvent.connect(function panelMousePressEvent(event) {
                return settings.mousePressEvent(event);
            });
            Controller.mouseDoublePressEvent.connect(function panelMouseDoublePressEvent(event) {
                return settings.mouseDoublePressEvent(event);
            });
            Controller.mouseReleaseEvent.connect(function(event) {
                return settings.mouseReleaseEvent(event);
            });
            Controller.keyPressEvent.connect(function(event) {
                return settings.keyPressEvent(event);
            });
          




Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);