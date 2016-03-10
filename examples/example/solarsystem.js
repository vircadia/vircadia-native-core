// 
//  solarsystem.js
//  example 
//  
//  Created by Bridget Went, 5/28/15.
//  Copyright 2015 High Fidelity, Inc.
//  
//  A project to build a virtual physics classroom to simulate the solar system, gravity, and orbital physics. 
//
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

CreateSimulation = function() {
    Script.include("https://hifi-public.s3.amazonaws.com/eric/scripts/tween.js");
    Script.include('games/satellite.js');

    var trailsEnabled = this.trailsEnabled = true;

    var DAMPING = this.DAMPING = 0.0;
    var LIFETIME = this.LIFETIME = 6000;
    var BASE_URL = this.BASE_URL = "https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/planets/";

    // Save intiial avatar and camera position
    var startingPosition = this.startingPosition = {
        x: 8000,
        y: 8000,
        z: 8000
    };
    MyAvatar.position = startingPosition;
    var cameraStart = this.cameraStart = Camera.getOrientation();


    // Place the sun
    var MAX_RANGE = this.MAX_RANGE = 100.0;
    var center = this.center = Vec3.sum(startingPosition, Vec3.multiply(MAX_RANGE, Vec3.normalize(Quat.getFront(Camera.getOrientation()))));
    var SUN_SIZE = this.SUN_SIZE = 7.0;

    var theSun = this.theSun = Entities.addEntity({
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
        dynamic: false
    });



    // Reference values for physical constants
    var referenceRadius = this.referenceRadius = 15.0;
    var referenceDiameter = this.referenceDiameter = 0.6;
    var referencePeriod = this.referencePeriod = 3.0;
    var LARGE_BODY_MASS = this.LARGE_BODY_MASS = 250.0;
    var SMALL_BODY_MASS = this.SMALL_BODY_MASS = LARGE_BODY_MASS * 0.000000333;
    var GRAVITY = this.GRAVITY = (Math.pow(referenceRadius, 3.0) / Math.pow((referencePeriod / (2.0 * Math.PI)), 2.0)) / LARGE_BODY_MASS;
    var REFERENCE_GRAVITY = this.REFERENCE_GRAVITY = GRAVITY;

    var planets = this.planets = [];
    var MAX_POINTS_PER_LINE = this.MAX_POINTS_PER_LINE = 40;
    var LINE_DIM = this.LINE_DIM = 200;
    var LINE_WIDTH = this.LINE_WIDTH = 20;

    var VELOCITY_OFFSET_Y = this.VELOCITY_OFFSET_Y = -0.3;
    var VELOCITY_OFFSET_Z = this.VELOCITY_OFFSET_Z = 0.9;

    var index = this.index = 0;

    this.computeLocalPoint = function(linePos, worldPoint) {
        var localPoint = Vec3.subtract(worldPoint, linePos);
        return localPoint;
    }

    // Create a new line
    this.newLine = function(lineStack, point, period, color) {
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

    this.pause = function() {
        if (paused) {
            return;
        }
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

    this.resume = function() {
        if (!paused) {
            return;
        }
        for (var i = 0; i < planets.length; ++i) {
            planets[i].label.hide();
        }
        paused = false;
    }

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
        this.startPosition = this.position;
        this.period = (2.0 * Math.PI) * Math.sqrt(Math.pow(this.radius, 3.0) / (GRAVITY * LARGE_BODY_MASS));

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
            dynamic: false,
        });

        this.computeAcceleration = function() {
            var acc = -(GRAVITY * LARGE_BODY_MASS) * Math.pow(this.radius, (-2.0));
            return acc;
        };

        this.update = function(deltaTime) {
            var between = Vec3.subtract(this.position, center);
            var speed = this.computeAcceleration(this.radius) * deltaTime;
            var vel = Vec3.multiply(speed, Vec3.normalize(between));

            // Update velocity and position
            this.velocity = Vec3.sum(this.velocity, vel);
            this.position = Vec3.sum(this.position, Vec3.multiply(deltaTime, this.velocity));
            Entities.editEntity(this.planet, {
                velocity: this.velocity,
                position: this.position
            });

            if (trailsEnabled) {
                this.updateTrail();
            }
        };

        this.clearTrails = function() {
            elapsed = 0.0;
            for (var j = 0; j < this.lineStack.length; ++j) {
                Entities.editEntity(this.lineStack[j], {
                    visible: false
                });
            }

        }
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
            if (!paused) {
                pause();
            }
            this.tweening = true;
            var tweenTime = 1000;
            var startingPosition = MyAvatar.position;
            var endingPosition = Vec3.sum({
                x: 0,
                y: 0,
                z: this.sizeScale
            }, this.position);
            var currentProps = {
                x: startingPosition.x,
                y: startingPosition.y,
                z: startingPosition.z,
            };
            var endProps = {
                x: endingPosition.x,
                y: endingPosition.y,
                z: endingPosition.z,
            };

            var moveTween = new TWEEN.Tween(currentProps).
            to(endProps, tweenTime).
            easing(TWEEN.Easing.Cubic.InOut).
            onUpdate(function() {
                MyAvatar.position = {
                    x: currentProps.x,
                    y: currentProps.y,
                    z: currentProps.z
                };
            }).start();

            moveTween.onComplete(function() {
                Camera.lookAt(endingPosition);
                this.tweening = false;
            });

        }

        index++;
        this.resetTrails();

    }


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

    this.initPlanets = function() {
        planets.push(new Planet("mercury", MERCURY_LINE_COLOR, 0.387, 0.383));
        planets.push(new Planet("venus", VENUS_LINE_COLOR, 0.723, 0.949));
        planets.push(new Planet("earth", EARTH_LINE_COLOR, 1.0, 1.0));
        planets.push(new Planet("mars", MARS_LINE_COLOR, 1.52, 0.532));
        planets.push(new Planet("jupiter", JUPITER_LINE_COLOR, 5.20, 11.21));
        planets.push(new Planet("saturn", SATURN_LINE_COLOR, 9.58, 9.45));
        planets.push(new Planet("uranus", URANUS_LINE_COLOR, 19.20, 4.01));
        planets.push(new Planet("neptune", NEPTUNE_LINE_COLOR, 30.05, 3.88));
        planets.push(new Planet("pluto", PLUTO_LINE_COLOR, 39.48, 0.186));
    }
    initPlanets();

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

    var planetLines = [];
    var trails = this.trails = [];
    var paused = this.paused = false;

    var time = 0;
    var elapsed;
    var TIME_STEP = 80;

    this.update = function(deltaTime) {
        for (var i = 0; i < planets.length; ++i) {
            if (paused) {
                return;
            }
            planets[i].update(deltaTime);
        }

        time++;
        if (time % TIME_STEP == 0) {
            elapsed++;
        }

    }

    Script.include('planets-ui.js');

    // Clean up models, UI panels, lines, and button overlays
    this.scriptEnding = function() {

        Entities.deleteEntity(theSun);
        for (var i = 0; i < planets.length; ++i) {
            Entities.deleteEntity(planets[i].planet);
        }
        var e = Entities.findEntities(MyAvatar.position, 10000);
        for (i = 0; i < e.length; i++) {
            var props = Entities.getEntityProperties(e[i]);
            if (props.type === "Line" || props.type === "Text") {
                Entities.deleteEntity(e[i]);
            }
        }
    };

    this.restart = function() {
        if (paused) {
            resume();
        }
        for (var i = 0; i < planets.length; ++i) {
            Entities.deleteEntity(planets[i].planet);
            planets[i].clearTrails();
        }
        time = 0.0;
        elapsed = 0.0;
        planets.length = 0;
        initPlanets();


        MyAvatar.position = startingPosition;
        Camera.setPosition(cameraStart);
    };

}
CreateSimulation();

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
