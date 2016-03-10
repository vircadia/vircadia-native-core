//
//  closePaint.js
//  examples
//
//  Created by Eric Levina on 9/30/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Run this script to be able to paint on entities you are close to, with hydras.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../libraries/utils.js");


var RIGHT_HAND = 1;
var LEFT_HAND = 0;

var MIN_POINT_DISTANCE = 0.02;
var MAX_POINT_DISTANCE = 0.5;

var SPATIAL_CONTROLLERS_PER_PALM = 2;
var TIP_CONTROLLER_OFFSET = 1;

var TRIGGER_ON_VALUE = 0.3;

var MAX_DISTANCE = 10;

var MAX_POINTS_PER_LINE = 40;

var RIGHT_4_ACTION = 18;
var RIGHT_2_ACTION = 16;

var LEFT_4_ACTION = 17;
var LEFT_2_ACTION = 16;

var HUE_INCREMENT = 0.02;

var MIN_STROKE_WIDTH = 0.002;
var MAX_STROKE_WIDTH = 0.04;


var center = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(Camera.getOrientation())));

var textureURL = "https://s3.amazonaws.com/hifi-public/eric/textures/paintStrokes/paintStroke.png";



function MyController(hand, triggerAction) {
    this.hand = hand;
    this.strokes = [];
    this.painting = false;
    this.currentStrokeWidth = MIN_STROKE_WIDTH;

    if (this.hand === RIGHT_HAND) {
        this.getHandPosition = MyAvatar.getRightPalmPosition;
        this.getHandRotation = MyAvatar.getRightPalmRotation;
    } else {
        this.getHandPosition = MyAvatar.getLeftPalmPosition;
        this.getHandRotation = MyAvatar.getLeftPalmRotation;
    }

    this.triggerAction = triggerAction;
    this.palm = SPATIAL_CONTROLLERS_PER_PALM * hand;
    this.tip = SPATIAL_CONTROLLERS_PER_PALM * hand + TIP_CONTROLLER_OFFSET;


    this.strokeColor = {
        h: 0.8,
        s: 0.8,
        l: 0.4
    };


    this.laserPointer = Overlays.addOverlay("circle3d", {
        color: hslToRgb(this.strokeColor),
        solid: true,
        position: center
    });
    this.triggerValue = 0;
    this.prevTriggerValue = 0;
    var _this = this;

    this.update = function() {
        this.updateControllerState();
        this.search();
        if (this.canPaint === true) {
            this.paint(this.intersection.intersection, this.intersection.surfaceNormal);
        }
    };

    this.paint = function(position, normal) {
        if (this.painting === false) {
            if (this.oldPosition) {
                this.newStroke(this.oldPosition);
            } else {
                this.newStroke(position);
            }
            this.painting = true;
        }



        var localPoint = Vec3.subtract(position, this.strokeBasePosition);
        //Move stroke a bit forward along normal so it doesnt zfight with mesh its drawing on 
        localPoint = Vec3.sum(localPoint, Vec3.multiply(normal, 0.001 + Math.random() * .001)); //rand avoid z fighting

        var distance = Vec3.distance(localPoint, this.strokePoints[this.strokePoints.length - 1]);
        if (this.strokePoints.length > 0 && distance < MIN_POINT_DISTANCE) {
            //need a minimum distance to avoid binormal NANs
            return;
        }
        if (this.strokePoints.length > 0 && distance > MAX_POINT_DISTANCE) {
            //Prevents drawing lines accross models
            this.painting = false;
            return;
        }
        if (this.strokePoints.length === 0) {
            localPoint = {
                x: 0,
                y: 0,
                z: 0
            };
        }

        this.strokePoints.push(localPoint);
        this.strokeNormals.push(normal);
        this.strokeWidths.push(this.currentStrokeWidth);
        Entities.editEntity(this.currentStroke, {
            linePoints: this.strokePoints,
            normals: this.strokeNormals,
            strokeWidths: this.strokeWidths
        });
        if (this.strokePoints.length === MAX_POINTS_PER_LINE) {
            this.painting = false;
            return;
        }
        this.oldPosition = position;
    }

    this.newStroke = function(position) {
        this.strokeBasePosition = position;
        this.currentStroke = Entities.addEntity({
            position: position,
            type: "PolyLine",
            color: hslToRgb(this.strokeColor),
            dimensions: {
                x: 50,
                y: 50,
                z: 50
            },
            lifetime: 200,
            textures: textureURL
        });
        this.strokePoints = [];
        this.strokeNormals = [];
        this.strokeWidths = [];

        this.strokes.push(this.currentStroke);

    }

    this.updateControllerState = function() {
        this.triggerValue = Controller.getValue(this.triggerAction);
        
        if (this.triggerValue > TRIGGER_ON_VALUE && this.prevTriggerValue <= TRIGGER_ON_VALUE) {
            this.squeeze();
        } else if (this.triggerValue < TRIGGER_ON_VALUE && this.prevTriggerValue >= TRIGGER_ON_VALUE) {
            this.release();
        }

        this.prevTriggerValue = this.triggerValue;
    }

    this.squeeze = function() {
        this.tryPainting = true;

    }
    this.release = function() {
        this.painting = false;
        this.tryPainting = false;
        this.canPaint = false;
        this.oldPosition = null;
    }
    this.search = function() {

        // the trigger is being pressed, do a ray test
        var handPosition = this.getHandPosition();
        var pickRay = {
            origin: handPosition,
            direction: Quat.getUp(this.getHandRotation())
        };


        this.intersection = Entities.findRayIntersection(pickRay, true);
        if (this.intersection.intersects) {
            var distance = Vec3.distance(handPosition, this.intersection.intersection);
            if (distance < MAX_DISTANCE) {
                var displayPoint = this.intersection.intersection;
                displayPoint = Vec3.sum(displayPoint, Vec3.multiply(this.intersection.surfaceNormal, .01));
                if (this.tryPainting) {
                    this.canPaint = true;
                }
                this.currentStrokeWidth = map(this.triggerValue, TRIGGER_ON_VALUE, 1, MIN_STROKE_WIDTH, MAX_STROKE_WIDTH);
                var laserSize = map(distance, 1, MAX_DISTANCE, 0.01, 0.1);
                laserSize += this.currentStrokeWidth / 2;
                Overlays.editOverlay(this.laserPointer, {
                    visible: true,
                    position: displayPoint,
                    rotation: orientationOf(this.intersection.surfaceNormal),
                    size: {
                        x: laserSize,
                        y: laserSize
                    }
                });

            } else {
                this.hitFail();
            }
        } else {
            this.hitFail();
        }
    };

    this.hitFail = function() {
        this.canPaint = false;

        Overlays.editOverlay(this.laserPointer, {
            visible: false
        });

    }

    this.cleanup = function() {
        Overlays.deleteOverlay(this.laserPointer);
        this.strokes.forEach(function(stroke) {
            Entities.deleteEntity(stroke);
        });
    }

    this.cycleColorDown = function() {
        this.strokeColor.h -= HUE_INCREMENT;
        if (this.strokeColor.h < 0) {
            this.strokeColor = 1;
        }
        Overlays.editOverlay(this.laserPointer, {
            color: hslToRgb(this.strokeColor)
        });
    }

    this.cycleColorUp = function() {
        this.strokeColor.h += HUE_INCREMENT;
        if (this.strokeColor.h > 1) {
            this.strokeColor.h = 0;
        }
        Overlays.editOverlay(this.laserPointer, {
            color: hslToRgb(this.strokeColor)
        });
    }
}

var rightController = new MyController(RIGHT_HAND, Controller.Standard.RT);
var leftController = new MyController(LEFT_HAND, Controller.Standard.LT);

Controller.actionEvent.connect(function(action, state) {
    if (state === 0) {
        return;
    }
    if (action === RIGHT_4_ACTION) {
        rightController.cycleColorUp();
    } else if (action === RIGHT_2_ACTION) {
        rightController.cycleColorDown();
    }
    if (action === LEFT_4_ACTION) {
        leftController.cycleColorUp();
    } else if (action === LEFT_2_ACTION) {
        leftController.cycleColorDown();
    }
});

function update() {
    rightController.update();
    leftController.update();
}

function cleanup() {
    rightController.cleanup();
    leftController.cleanup();
}

Script.scriptEnding.connect(cleanup);
Script.update.connect(update);
