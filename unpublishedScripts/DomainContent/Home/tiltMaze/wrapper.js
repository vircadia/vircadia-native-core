//
//  createTiltMaze.js
//
//  Created by James B. Pollack @imgntn on 2/15/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This script creates a maze with a ball that you can tilt to try to get to the end.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var MAZE_SCRIPT = Script.resolvePath('atp:/tiltMaze/maze.js');

TiltMaze = function(spawnPosition, spawnRotation) {

    var ball, ballSpawningAnchor, ballDetector, tiltMaze, lightAtTheEnd;

    var MAZE_MODEL_URL = "atp:/tiltMaze/newmaze_tex-4.fbx";
    var MAZE_COLLISION_HULL = "atp:/tiltMaze/newmaze_tex-3.obj";

    var SCALE = 0.5;

    var MAZE_DIMENSIONS = Vec3.multiply(SCALE, {
        x: 1,
        y: 0.15,
        z: 1
    });

    var BALL_DIMENSIONS = Vec3.multiply(SCALE, {
        x: 0.035,
        y: 0.035,
        z: 0.035
    });

    var BALL_SPAWNER_DIMENSIONS = Vec3.multiply(SCALE, {
        x: 0.05,
        y: 0.05,
        z: 0.05
    });

    var BALL_DETECTOR_DIMENSIONS = Vec3.multiply(SCALE, {
        x: 0.1,
        y: 0.1,
        z: 0.1
    });

    var BALL_COLOR = {
        red: 255,
        green: 0,
        blue: 0
    };

    var DEBUG_COLOR = {
        red: 0,
        green: 255,
        blue: 0
    };

    var center = spawnPosition;


    var CLEANUP = false;

    var BALL_FORWARD_OFFSET = -0.2 * SCALE;
    var BALL_RIGHT_OFFSET = -0.4 * SCALE;
    var BALL_VERTICAL_OFFSET = 0.02 * SCALE;

    var BALL_FRICTION = 0.7;
    var BALL_RESTITUTION = 0.1;
    var BALL_DAMPING = 0.6;
    var BALL_ANGULAR_DAMPING = 0.2;
    var BALL_DENSITY = 1000;
    var BALL_GRAVITY = {
        x: 0,
        y: -9.8,
        z: 0
    };

    var MAZE_DENSITY = 1000;
    var MAZE_RESTITUTION = 0.1;
    var MAZE_DAMPING = 0.6;
    var MAZE_ANGULAR_DAMPING = 0.6;
    var MAZE_GRAVITY = {
        x: 0,
        y: -10,
        z: 0
    };

    var DETECTOR_VERTICAL_OFFSET = 0.0 * SCALE;
    var DETECTOR_FORWARD_OFFSET = 0.35 * SCALE;
    var DETECTOR_RIGHT_OFFSET = 0.35 * SCALE;

    var END_LIGHT_COLOR = {
        red: 255,
        green: 0,
        blue: 0
    };

    var END_LIGHT_DIMENSIONS = {
        x: 0.2,
        y: 0.2,
        z: 0.8
    };

    var END_LIGHT_INTENSITY = 0.035;
    var END_LIGHT_CUTOFF = 30;
    var END_LIGHT_EXPONENT = 1;

    var getBallStartLocation = function() {
        var mazeProps = Entities.getEntityProperties(tiltMaze);
        var right = Quat.getRight(mazeProps.rotation);
        var front = Quat.getFront(mazeProps.rotation);
        var vertical = {
            x: 0,
            y: BALL_VERTICAL_OFFSET,
            z: 0
        };

        var finalOffset = Vec3.sum(vertical, Vec3.multiply(right, BALL_RIGHT_OFFSET));
        finalOffset = Vec3.sum(finalOffset, Vec3.multiply(front, BALL_FORWARD_OFFSET));
        var location = Vec3.sum(mazeProps.position, finalOffset);
        return location;
    };

    var getBallFinishLocation = function() {
        var mazeProps = Entities.getEntityProperties(tiltMaze);
        var right = Quat.getRight(mazeProps.rotation);
        var forward = Quat.getFront(mazeProps.rotation);
        var up = Quat.getUp(mazeProps.rotation);

        var position = Vec3.sum(mazeProps.position, Vec3.multiply(up, DETECTOR_VERTICAL_OFFSET));
        position = Vec3.sum(position, Vec3.multiply(right, DETECTOR_RIGHT_OFFSET));
        position = Vec3.sum(position, Vec3.multiply(forward, DETECTOR_FORWARD_OFFSET));

        return position;
    };


    var createBall = function(position) {
        var properties = {
            name: 'home_sphere_tiltMazeBall',
            type: 'Sphere',
            position: getBallStartLocation(),
            dynamic: true,
            collisionless: false,
            friction: BALL_FRICTION,
            restitution: BALL_RESTITUTION,
            angularDamping: BALL_ANGULAR_DAMPING,
            damping: BALL_DAMPING,
            gravity: BALL_GRAVITY,
            density: BALL_DENSITY,
            color: BALL_COLOR,
            dimensions: BALL_DIMENSIONS,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                },
                grabbableKey: {
                    grabbable: false
                }
            })

        };

        ball = Entities.addEntity(properties);

    };

    var createBallSpawningAnchor = function() {
        var properties = {
            name: 'home_box_tiltMazeBallSpawningAnchor',
            parentID: tiltMaze,
            type: 'Box',
            color: DEBUG_COLOR,
            dimensions: BALL_SPAWNER_DIMENSIONS,
            position: getBallStartLocation(),
            collisionless: true,
            visible: false,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        };

        ballSpawningAnchor = Entities.addEntity(properties);
    };


    var createBallDetector = function() {

        var properties = {
            name: 'home_box_tiltMazeBallDetector',
            parentID: tiltMaze,
            type: 'Box',
            color: DEBUG_COLOR,
            shapeType: 'none',
            dimensions: BALL_DETECTOR_DIMENSIONS,
            position: getBallFinishLocation(),
            collisionless: true,
            dynamic: false,
            visible: false,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),

        };

        ballDetector = Entities.addEntity(properties);

    };

    var createTiltMaze = function(position) {
        var properties = {
            name: 'home_model_tiltMaze',
            type: 'Model',
            modelURL: MAZE_MODEL_URL,
            gravity: MAZE_GRAVITY,
            compoundShapeURL: MAZE_COLLISION_HULL,
            dimensions: MAZE_DIMENSIONS,
            position: position,
            restitution: MAZE_RESTITUTION,
            damping: MAZE_DAMPING,
            angularDamping: MAZE_ANGULAR_DAMPING,
            dynamic: true,
            density: MAZE_DENSITY,
            script: MAZE_SCRIPT,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        }

        if (spawnRotation !== undefined) {
            properties.rotation = spawnRotation;
        }
        tiltMaze = Entities.addEntity(properties);

    };

    var createLightAtTheEnd = function() {

        var mazeProps = Entities.getEntityProperties(tiltMaze, 'position');

        var up = Quat.getUp(mazeProps.rotation);
        var down = Vec3.multiply(-1, up);

        var emitOrientation = Quat.rotationBetween(Vec3.UNIT_NEG_Z, down);

        var position = getBallFinishLocation();
        var lightProperties = {
            parentID: tiltMaze,
            name: 'home_light_tiltMazeEndLight',
            type: "Light",
            isSpotlight: true,
            dimensions: END_LIGHT_DIMENSIONS,
            color: END_LIGHT_COLOR,
            intensity: END_LIGHT_INTENSITY,
            exponent: END_LIGHT_EXPONENT,
            cutoff: END_LIGHT_CUTOFF,
            lifetime: -1,
            position: position,
            rotation: emitOrientation,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        };

        lightAtTheEnd = Entities.addEntity(lightProperties);
    };

    var createAll = function() {
        createTiltMaze(center);
        createBallSpawningAnchor();
        createBallDetector(center);
        createBall(center);
        createLightAtTheEnd();
        Entities.editEntity(tiltMaze, {
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                },
                tiltMaze: {
                    firstBall: ball,
                    ballSpawner: ballSpawningAnchor,
                    detector: ballDetector,
                    lightAtTheEnd: lightAtTheEnd
                }
            })
        });
    };

    createAll();

    function cleanup() {
        print('MAZE CLEANUP')
        Entities.deleteEntity(tiltMaze);
        Entities.deleteEntity(ball);
        Entities.deleteEntity(ballSpawningAnchor);
        Entities.deleteEntity(lightAtTheEnd);
    }

    this.cleanup = cleanup;
    print('CREATED TILTMAZE')

}