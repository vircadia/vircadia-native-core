//
//  tiltMaze.js
//
//  Script Type: Entity
//
//  Created by James B. Pollack @imgntn on 2/15/2016
//  Copyright 2016 High Fidelity, Inc.
//
//
//  This is the entity script for a tilt maze.  It create a ball when you grab it, which you have a set amount of time to get through the hole
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {

    var COLLISION_HULL = "";
    var MAZE_MODEL = "";
    var LENGTH_OF_GAME = 60;

    var BALL_DETECTOR_SCRIPT = Script.resolvePath('ballDetector.js?' + Math.random())
    var _this;

    function TiltMaze() {
        _this = this;
        return;
    }

    TiltMaze.prototype = {
        hasBall: false,
        hand: null,
        startNearGrab: function(entityID, args) {
            this.hand = args[0];

        },
        releaseGrab: function() {

        },

        preload: function(entityID) {
            this.entityID = entityID;
        },

        createBall: function(position) {
            var properties = {
                type: 'Sphere',
                shapeType: 'sphere',
                dimensions: BALL_DIMENSIONS,
                color: BALL_COLOR,
                position: position
            };
            this.ball = Entities.addEntity(properties);
        },

        createBallDetector: function(position, rotation) {
            var properties = {
                parentID: this.entityID,
                type: 'Box',
                dimensions: BALL_DETECTOR_DIMENSIONS,
                position: position,
                rotiation: rotation,
                collisionless: true,
                script: BALL_DETECTOR_SCRIPT
            };
            this.ballDetector = Entities.addEntity(properties);
        },

        destroyBall: function() {
            Entities.deleteEntity(this.ball);
        }

            unload: function() {


        },

    };

    return new TiltMaze();
});