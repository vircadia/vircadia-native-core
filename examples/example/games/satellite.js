// 
//  satellite.js 
//  games
//  
//  Created by Bridget Went 7/1/2015. 
//  Copyright 2015 High Fidelity, Inc.
//  
//  A game to bring a satellite model into orbit around an animated earth model . 
//      - Double click to create a new satellite
//      - Click on the satellite, drag a vector arrow to specify initial velocity
//      - Release mouse to launch the active satellite
//      - Orbital movement is calculated using equations of gravitational physics
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include('../../utilities/tools/vector.js');

var URL = "https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/";

SatelliteCreator = function() {
    print("initializing satellite game");
   
    var MAX_RANGE = 50.0;
    var Y_AXIS = {
        x: 0,
        y: 1,
        z: 0
    }
    var LIFETIME = 6000;
    var ERROR_THRESH = 20.0;

    // Create the spinning earth model
    var EARTH_SIZE = 20.0;
    var CLOUDS_OFFSET = 0.5;
    var SPIN = 0.1;
    var ZONE_DIM = 100.0;
    var LIGHT_INTENSITY = 1.5;

    var center, distance;
    var earth;


    Earth = function(position, size) {
        this.earth = Entities.addEntity({
            type: "Model",
            shapeType: 'sphere',
            modelURL: URL + "earth.fbx",
            position: position,
            dimensions: {
                x: size,
                y: size,
                z: size
            },
            rotation: Quat.angleAxis(180, {
                x: 1,
                y: 0,
                z: 0
            }),
            angularVelocity: {
                x: 0.00,
                y: 0.5 * SPIN,
                z: 0.00
            },
            angularDamping: 0.0,
            damping: 0.0,
            ignoreCollisions: false,
            lifetime: 6000,
            dynamic: false,
            visible: true
        });

        this.clouds = Entities.addEntity({
            type: "Model",
            shapeType: 'sphere',
            modelURL: URL + "clouds.fbx",
            position: position,
            dimensions: {
                x: size + CLOUDS_OFFSET,
                y: size + CLOUDS_OFFSET,
                z: size + CLOUDS_OFFSET
            },
            angularVelocity: {
                x: 0.00,
                y: SPIN,
                z: 0.00
            },
            angularDamping: 0.0,
            damping: 0.0,
            ignoreCollisions: false,
            lifetime: LIFETIME,
            dynamic: false,
            visible: true
        });

        this.zone = Entities.addEntity({
            type: "Zone",
            position: position,
            dimensions: {
                x: ZONE_DIM,
                y: ZONE_DIM,
                z: ZONE_DIM
            },
            keyLightDirection: Vec3.normalize(Vec3.subtract(position, Camera.getPosition())),
            keyLightIntensity: LIGHT_INTENSITY
        });

        this.cleanup = function() {
            print('cleaning up earth models');
            Entities.deleteEntity(this.clouds);
            Entities.deleteEntity(this.earth);
            Entities.deleteEntity(this.zone);
        }
    }

    
    this.init = function() {
        if (this.isActive) {
            this.quitGame();
        }
        var confirmed = Window.confirm("Start satellite game?");
        if (!confirmed) {
           return false;
        }
        
        this.isActive = true;
        MyAvatar.position = {
                x: 1000,
                y: 1000,
                z: 1000
            };
        Camera.setPosition({
                x: 1000,
                y: 1000,
                z: 1000
            });

        // Create earth model
        center = Vec3.sum(Camera.getPosition(), Vec3.multiply(MAX_RANGE, Quat.getFront(Camera.getOrientation())));
        distance = Vec3.length(Vec3.subtract(center, Camera.getPosition()));
        earth = new Earth(center, EARTH_SIZE);
        return true;
    };
    

    var satellites = [];
    var SATELLITE_SIZE = 2.0;
    var launched = false;
    var activeSatellite;

    var PERIOD = 4.0;
    var LARGE_BODY_MASS = 16000.0;
    var SMALL_BODY_MASS = LARGE_BODY_MASS * 0.000000333;

    Satellite = function(position, planetCenter) {
        // The Satellite class

        this.launched = false;
        this.startPosition = position;
        this.readyToLaunch = false;
        this.radius = Vec3.length(Vec3.subtract(position, planetCenter));

        this.satellite = Entities.addEntity({
            type: "Model",
            modelURL: "https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/satellite/satellite.fbx",
            position: this.startPosition,
            dimensions: {
                x: SATELLITE_SIZE,
                y: SATELLITE_SIZE,
                z: SATELLITE_SIZE
            },
            angularDamping: 0.0,
            damping: 0.0,
            ignoreCollisions: false,
            lifetime: LIFETIME,
            dynamic: false,
        });

        this.getProperties = function() {
            return Entities.getEntityProperties(this.satellite);
        }

        this.launch = function() {
            var prop = Entities.getEntityProperties(this.satellite);
            var between = Vec3.subtract(planetCenter, prop.position);
            var radius = Vec3.length(between);
            this.gravity = (4.0 * Math.PI * Math.PI * Math.pow(radius, 3.0)) / (LARGE_BODY_MASS * PERIOD * PERIOD);

            var initialVelocity = Vec3.normalize(Vec3.cross(between, Y_AXIS));
            initialVelocity = Vec3.multiply(Math.sqrt((this.gravity * LARGE_BODY_MASS) / radius), initialVelocity);
            initialVelocity = Vec3.multiply(this.arrow.magnitude, initialVelocity);
            initialVelocity = Vec3.multiply(Vec3.length(initialVelocity), this.arrow.direction);

            Entities.editEntity(this.satellite, {
                velocity: initialVelocity
            });
            this.launched = true;
        };


        this.update = function(deltaTime) {
            var prop = Entities.getEntityProperties(this.satellite);
            var between = Vec3.subtract(prop.position, planetCenter);
            var radius = Vec3.length(between);
            var acceleration = -(this.gravity * LARGE_BODY_MASS) * Math.pow(radius, (-2.0));
            var speed = acceleration * deltaTime;
            var vel = Vec3.multiply(speed, Vec3.normalize(between));

            var newVelocity = Vec3.sum(prop.velocity, vel);
            var newPos = Vec3.sum(prop.position, Vec3.multiply(newVelocity, deltaTime));

            Entities.editEntity(this.satellite, {
                velocity: newVelocity,
                position: newPos
            });
        };
    }

    function mouseDoublePressEvent(event) {
        var pickRay = Camera.computePickRay(event.x, event.y);
        var addVector = Vec3.multiply(pickRay.direction, distance);
        var point = Vec3.sum(Camera.getPosition(), addVector);

        // Create a new satellite 
        activeSatellite = new Satellite(point, center);
        satellites.push(activeSatellite);
    }

    function mousePressEvent(event) {
        if (!activeSatellite) {
            return;
        }
        // Reset label
        if (activeSatellite.arrow) {
            activeSatellite.arrow.deleteLabel();
        }
        var statsPosition = Vec3.sum(Camera.getPosition(), Vec3.multiply(MAX_RANGE * 0.4, Quat.getFront(Camera.getOrientation())));
        var pickRay = Camera.computePickRay(event.x, event.y)
        var rayPickResult = Entities.findRayIntersection(pickRay, true);
        if (rayPickResult.entityID === activeSatellite.satellite) {
            // Create a draggable vector arrow at satellite position
            activeSatellite.arrow = new VectorArrow(distance, true, "INITIAL VELOCITY", statsPosition);
            activeSatellite.arrow.onMousePressEvent(event);
            activeSatellite.arrow.isDragging = true;
        }
    }

    function mouseMoveEvent(event) {
        if (!activeSatellite || !activeSatellite.arrow || !activeSatellite.arrow.isDragging) {
            return;
        }
        activeSatellite.arrow.onMouseMoveEvent(event);
    }

    function mouseReleaseEvent(event) {
        if (!activeSatellite || !activeSatellite.arrow || !activeSatellite.arrow.isDragging) {
            return;
        }
        activeSatellite.arrow.onMouseReleaseEvent(event);
        activeSatellite.launch();
        activeSatellite.arrow.cleanup();
    }

    var counter = 0.0;
    var CHECK_ENERGY_PERIOD = 500;

    function update(deltaTime) {
        if (!activeSatellite) {
            return;
        }
        // Update all satellites
        for (var i = 0; i < satellites.length; i++) {
            if (!satellites[i].launched) {
                continue;
            }
            satellites[i].update(deltaTime);
        }

        counter++;
        if (counter % CHECK_ENERGY_PERIOD == 0) {
            var prop = activeSatellite.getProperties();
            var error = calcEnergyError(prop.position, Vec3.length(prop.velocity));
            if (Math.abs(error) <= ERROR_THRESH) {
                activeSatellite.arrow.editLabel("Nice job! The satellite has reached a stable orbit.");
            } else {
                activeSatellite.arrow.editLabel("Try again! The satellite is in an unstable orbit.");
            }
        }
    }

    this.quitGame = function() {
        print("ending satellite game");
        this.isActive = false;

        for (var i = 0; i < satellites.length; i++) {
            Entities.deleteEntity(satellites[i].satellite);
            satellites[i].arrow.cleanup();
        }
        earth.cleanup();
        
    }


    function calcEnergyError(pos, vel) {
        //Calculate total energy error for active satellite's orbital motion
        var radius = activeSatellite.radius;
        var gravity = (4.0 * Math.PI * Math.PI * Math.pow(radius, 3.0)) / (LARGE_BODY_MASS * PERIOD * PERIOD);
        var initialVelocityCalculated = Math.sqrt((gravity * LARGE_BODY_MASS) / radius);

        var totalEnergy = 0.5 * LARGE_BODY_MASS * Math.pow(initialVelocityCalculated, 2.0) - ((gravity * LARGE_BODY_MASS * SMALL_BODY_MASS) / radius);
        var measuredEnergy = 0.5 * LARGE_BODY_MASS * Math.pow(vel, 2.0) - ((gravity * LARGE_BODY_MASS * SMALL_BODY_MASS) / Vec3.length(Vec3.subtract(pos, center)));
        var error = ((measuredEnergy - totalEnergy) / totalEnergy) * 100;
        return error;
    }

    Controller.mousePressEvent.connect(mousePressEvent);
    Controller.mouseDoublePressEvent.connect(mouseDoublePressEvent);
    Controller.mouseMoveEvent.connect(mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
    Script.update.connect(update);
    Script.scriptEnding.connect(this.quitGame);

}

