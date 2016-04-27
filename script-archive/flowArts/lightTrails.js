//
//  lightTrails.js
//  examples
//
//  Created by Eric Levin on 5/14/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates light trails as you move your hydra hands
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../libraries/utils.js");

var eraseTrail = true;
var ugLSD = 25;
// var eraseTrail = false;
var LEFT = 0;
var RIGHT = 1;
var MAX_POINTS_PER_LINE = 50;

var LIFETIME = 6000;
var DRAWING_DEPTH = 0.8;
var LINE_DIMENSIONS = 100;


var MIN_POINT_DISTANCE = 0.02;


var colorPalette = [{
    red: 250,
    green: 137,
    blue: 162
}, {
    red: 204,
    green: 244,
    blue: 249
}, {
    red: 146,
    green: 206,
    blue: 116
}, {
    red: 240,
    green: 87,
    blue: 129
}];

var STROKE_WIDTH = 0.04;

function controller(side, triggerAction) {
    this.triggerHeld = false;
    this.triggerThreshold = 0.9;
    this.side = side;
    this.triggerAction = triggerAction;
    var texture = "https://s3.amazonaws.com/hifi-public/eric/textures/paintStrokes/trails.png";

    this.light = Entities.addEntity({
        type: 'Light',
        position: MyAvatar.position,
        dimensions: {
            x: 30,
            y: 30,
            z: 30
        },
        color: colorPalette[randInt(0, colorPalette.length)],
        intensity: 5
    });

    this.trail = Entities.addEntity({
        type: "PolyLine",
        dimensions: {
            x: LINE_DIMENSIONS,
            y: LINE_DIMENSIONS,
            z: LINE_DIMENSIONS
        },
        color: {red: 255, green: 255, blue: 255},
        textures: texture,
        lifetime: LIFETIME
    });
    this.points = [];
    this.normals = [];
    this.strokeWidths = [];
    var self = this;

    this.trailEraseInterval = Script.setInterval(function() {
        if (self.points.length > 0 && eraseTrail) {
            self.points.shift();
            self.normals.shift();
            self.strokeWidths.shift();
            Entities.editEntity(self.trail, {
                linePoints: self.points,
                strokeWidths: self.strokeWidths,
                normals: self.normals
            });
        }
    }, ugLSD);


    this.setTrailPosition = function(position) {
        this.trailPosition = position;
        Entities.editEntity(this.trail, {
            position: this.trailPosition
        });
    }


    this.update = function(deltaTime) {
        this.updateControllerState();
        var newTrailPosOffset = Vec3.multiply(Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition)), DRAWING_DEPTH);
        var newTrailPos = Vec3.sum(this.palmPosition, newTrailPosOffset);
        Entities.editEntity(this.light, {
            position: newTrailPos
        });


        if (!this.drawing) {
            this.setTrailPosition(newTrailPos);
            this.drawing = true;
        }

        if (this.drawing) {
            var localPoint = Vec3.subtract(newTrailPos, this.trailPosition);
            if (Vec3.distance(localPoint, this.points[this.points.length - 1]) < MIN_POINT_DISTANCE) {
                //Need a minimum distance to avoid binormal NANs
                return;
            }

            if (this.points.length === MAX_POINTS_PER_LINE) {
                this.points.shift();
                this.normals.shift();
                this.strokeWidths.shift();
            }

            this.points.push(localPoint);
            var normal = computeNormal(newTrailPos, Camera.getPosition());

            this.normals.push(normal);
            this.strokeWidths.push(STROKE_WIDTH + Math.random() * 0.01);
            Entities.editEntity(this.trail, {
                linePoints: this.points,
                normals: this.normals,
                strokeWidths: this.strokeWidths,
            });

        }
    }


    this.updateControllerState = function() {
        this.palmPosition = this.side == RIGHT ? MyAvatar.rightHandPose.translation : MyAvatar.leftHandPose.translation;
        this.tipPosition = this.side == RIGHT ? MyAvatar.rightHandTipPose.translation : MyAvatar.leftHandTipPose.translation;
        this.triggerValue = Controller.getActionValue(this.triggerAction);

    }

    this.cleanup = function() {
        Entities.deleteEntity(this.trail);
        Entities.deleteEntity(this.light);
        Script.clearInterval(this.trailEraseInterval);
    }
}

function computeNormal(p1, p2) {
    return Vec3.normalize(Vec3.subtract(p2, p1));
}

function update(deltaTime) {
    leftController.update(deltaTime);
    rightController.update(deltaTime);
}

function scriptEnding() {
    leftController.cleanup();
    rightController.cleanup();
}

function vectorIsZero(v) {
    return v.x === 0 && v.y === 0 && v.z === 0;
}


var rightController = new controller(RIGHT, Controller.findAction("RIGHT_HAND_CLICK"));
var leftController = new controller(LEFT, Controller.findAction("LEFT_HAND_CLICK"));
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

function map(value, min1, max1, min2, max2) {
    return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
}