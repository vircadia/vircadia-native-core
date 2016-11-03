//
//  whiteBoardEntityScript.js
//  examples/painting/whiteboard
//
//  Created by Eric Levin on 10/12/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */

/*global Whiteboard */



(function() {
    Script.include("../../libraries/utils.js");
    var _this;
    var RIGHT_HAND = 1;
    var LEFT_HAND = 0;
    var MIN_POINT_DISTANCE = 0.01;
    var MAX_POINT_DISTANCE = 0.5;
    var MAX_POINTS_PER_LINE = 40;
    var MAX_DISTANCE = 5;

    var PAINT_TRIGGER_THRESHOLD = 0.6;
    var MIN_STROKE_WIDTH = 0.0005;
    var MAX_STROKE_WIDTH = 0.03;
    var textureURL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/textures/paintStroke.png";

    var TRIGGER_CONTROLS = [
        Controller.Standard.LT,
        Controller.Standard.RT,
    ];

    Whiteboard = function() {
        _this = this;
    };

    Whiteboard.prototype = {

        setRightHand: function() {
            this.hand = RIGHT_HAND;
        },

        setLeftHand: function() {
            this.hand = LEFT_HAND;
        },

        startFarTrigger: function() {
            if (this.painting) {
                return;
            }
            this.whichHand = this.hand;
            if (this.hand === RIGHT_HAND) {
                this.getHandPosition = MyAvatar.getRightPalmPosition;
                this.getHandRotation = MyAvatar.getRightPalmRotation;
            } else if (this.hand === LEFT_HAND) {
                this.getHandPosition = MyAvatar.getLeftPalmPosition;
                this.getHandRotation = MyAvatar.getLeftPalmRotation;
            }
            Overlays.editOverlay(this.laserPointer, {
                visible: true
            });
        },

        continueFarTrigger: function() {
            var handPosition = this.getHandPosition();
            var pickRay = {
                origin: handPosition,
                direction: Quat.getUp(this.getHandRotation())
            };

            this.intersection = Entities.findRayIntersection(pickRay, true, this.whitelist);
            //Comment out above line and uncomment below line to see difference in performance between using a whitelist, and not using one
            // this.intersection = Entities.findRayIntersection(pickRay, true);

            if (this.intersection.intersects) {
                var distance = Vec3.distance(handPosition, this.intersection.intersection);
                if (distance < MAX_DISTANCE) {
                    this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[this.hand]);
                    this.currentStrokeWidth = map(this.triggerValue, 0, 1, MIN_STROKE_WIDTH, MAX_STROKE_WIDTH);
                    var displayPoint = this.intersection.intersection;
                    displayPoint = Vec3.sum(displayPoint, Vec3.multiply(this.normal, 0.01));
                    Overlays.editOverlay(this.laserPointer, {
                        position: displayPoint,
                        size: {
                            x: this.currentStrokeWidth,
                            y: this.currentStrokeWidth
                        }
                    });
                    if (this.triggerValue > PAINT_TRIGGER_THRESHOLD) {
                        this.paint(this.intersection.intersection, this.intersection.surfaceNormal);
                    } else {
                        this.painting = false;
                        this.oldPosition = null;
                    }
                }
            } else if (this.intersection.properties.type !== "Unknown") {
                //Sometimes ray will pick against an invisible object with type unkown... so if type is unknown, ignore
                this.stopPainting();
            }
        },

        stopPainting: function() {
            this.painting = false;
            Overlays.editOverlay(this.laserPointer, {
                visible: false
            });
            this.oldPosition = null;
        },

        paint: function(position, normal) {
            if (this.painting === false) {
                if (this.oldPosition) {
                    this.newStroke(this.oldPosition);
                } else {
                    this.newStroke(position);
                }
                this.painting = true;
            }


            var localPoint = Vec3.subtract(position, this.strokeBasePosition);
            //Move stroke a bit forward along normal each point so it doesnt zfight with mesh its drawing on, or previous part of stroke(s)
            localPoint = Vec3.sum(localPoint, Vec3.multiply(this.normal, this.forwardOffset));
            this.forwardOffset += 0.00001;
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
            this.strokeNormals.push(this.normal);
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
        },


        newStroke: function(position) {
            this.strokeBasePosition = position;
            this.currentStroke = Entities.addEntity({
                position: position,
                type: "PolyLine",
                name: "paintStroke",
                color: this.strokeColor,
                textures: textureURL,
                dimensions: {
                    x: 50,
                    y: 50,
                    z: 50
                },
                lifetime: 200,
                userData: JSON.stringify({
                    whiteboard: this.entityID
                })
            });
            this.strokePoints = [];
            this.strokeNormals = [];
            this.strokeWidths = [];

            this.strokes.push(this.currentStroke);

        },

        stopFarTrigger: function() {
            if (this.hand !== this.whichHand) {
                return;
            }
            this.stopPainting();

        },

        changeColor: function() {
            var userData = JSON.parse(Entities.getEntityProperties(this.entityID, ["userData"]).userData);
            this.strokeColor = userData.color.currentColor;
            this.colorIndicator = userData.colorIndicator;
            Overlays.editOverlay(this.laserPointer, {
                color: this.strokeColor
            });


            Entities.callEntityMethod(this.colorIndicator, "changeColor");
        },

        eraseBoard: function() {
            var distance = Math.max(this.dimensions.x, this.dimensions.y);
            var entities = Entities.findEntities(this.position, distance);
            entities.forEach(function(entity) {
                var props = Entities.getEntityProperties(entity, ["name, userData"]);
                var name = props.name;
                if (!props.userData) {
                    return;
                }
                var whiteboardID = JSON.parse(props.userData).whiteboard;
                if (name === "paintStroke" && JSON.stringify(whiteboardID) === JSON.stringify(_this.entityID)) {
                    // This entity is a paintstroke and part of this whiteboard so delete it
                    Entities.deleteEntity(entity);
                }
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;
            var props = Entities.getEntityProperties(this.entityID, ["position", "rotation", "userData", "dimensions"]);
            this.position = props.position;
            this.rotation = props.rotation;
            this.dimensions = props.dimensions;
            this.normal = Vec3.multiply(Quat.getFront(this.rotation), -1);
            this.painting = false;
            this.strokes = [];
            this.whitelist = [this.entityID];
            this.laserPointer = Overlays.addOverlay("circle3d", {
                color: this.strokeColor,
                solid: true,
                rotation: this.rotation
            });
            this.forwardOffset = 0.0005;

            this.changeColor();
        },

        unload: function() {

            Overlays.deleteOverlay(this.laserPointer);
            // this.eraseBoard();
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Whiteboard();
});
