//
//  markerTipEntityScript.js
//  examples/homeContent/markerTipEntityScript
//
//  Created by Eric Levin on 2/17/15.
//  Copyright 2016 High Fidelity, Inc.
//
//  This script provides the logic for an object to draw marker strokes on its associated whiteboard

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html



(function() {
    Script.include("../../libraries/utils.js");
    var TRIGGER_CONTROLS = [
        Controller.Standard.LT,
        Controller.Standard.RT,
    ];
    var MAX_POINTS_PER_STROKE = 40;
    var _this;
    MarkerTip = function() {
        _this = this;
        _this.MARKER_TEXTURE_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/textures/markerStroke.png";
        _this.strokeForwardOffset = 0.0001;
        _this.STROKE_WIDTH_RANGE = {
            min: 0.002,
            max: 0.01
        };
        _this.MAX_MARKER_TO_BOARD_DISTANCE = 1.4;
        _this.MIN_DISTANCE_BETWEEN_POINTS = 0.002;
        _this.MAX_DISTANCE_BETWEEN_POINTS = 0.1;
        _this.strokes = [];
        _this.PAINTING_TRIGGER_THRESHOLD = 0.2;
        _this.STROKE_NAME = "hifi-marker-stroke";
        _this.WHITEBOARD_SURFACE_NAME = "hifi-whiteboardDrawingSurface";
        _this.MARKER_RESET_WAIT_TIME = 3000;
    };

    MarkerTip.prototype = {

        startEquip: function(id, params) {
            _this.whiteboards = [];
            _this.equipped = true;
            _this.hand = params[0] == "left" ? 0 : 1;
            _this.markerColor = getEntityUserData(_this.entityID).markerColor;
            // search for whiteboards
            var markerPosition = Entities.getEntityProperties(_this.entityID, "position").position;
            var entities = Entities.findEntities(markerPosition, 10);
            entities.forEach(function(entity) {
                var entityName = Entities.getEntityProperties(entity, "name").name;
                if (entityName === _this.WHITEBOARD_SURFACE_NAME) {

                    _this.whiteboards.push(entity);
                }
            });

            print("intersectable entities " + JSON.stringify(_this.whiteboards))
        },

        releaseEquip: function() {
            _this.resetStroke();
            Overlays.editOverlay(_this.laserPointer, {
                visible: false
            });

            // Once user releases marker, wait a bit then put marker back to its original position and rotation
            Script.setTimeout(function() {
                var userData = getEntityUserData(_this.entityID);
                Entities.editEntity(_this.entityID, {
                    position: userData.originalPosition,
                    rotation: userData.originalRotation,
                    velocity: {
                        x: 0,
                        y: -0.01,
                        z: 0
                    }
                });
            }, _this.MARKER_RESET_WAIT_TIME);
        },


        continueEquip: function() {
            // cast a ray from marker and see if it hits anything
            var markerProps = Entities.getEntityProperties(_this.entityID, ["position", "rotation"]);

            var pickRay = {
                origin: markerProps.position,
                direction: Quat.getFront(markerProps.rotation)
            }
            var intersection = Entities.findRayIntersectionBlocking(pickRay, true, _this.whiteboards);

            if (intersection.intersects && Vec3.distance(intersection.intersection, markerProps.position) < _this.MAX_MARKER_TO_BOARD_DISTANCE) {
                _this.currentWhiteboard = intersection.entityID;
                var whiteboardRotation = Entities.getEntityProperties(_this.currentWhiteboard, "rotation").rotation;
                _this.whiteboardNormal = Quat.getFront(whiteboardRotation);
                Overlays.editOverlay(_this.laserPointer, {
                    visible: true,
                    position: intersection.intersection,
                    rotation: whiteboardRotation
                })
                _this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[_this.hand]);
                if (_this.triggerValue > _this.PAINTING_TRIGGER_THRESHOLD) {
                    _this.paint(intersection.intersection)
                } else {
                    _this.resetStroke();
                }
            } else {
                if (_this.currentStroke) {
                    _this.resetStroke();
                }

                Overlays.editOverlay(_this.laserPointer, {
                    visible: false
                });
            }

        },

        newStroke: function(position) {
            _this.strokeBasePosition = position;
            _this.currentStroke = Entities.addEntity({
                type: "PolyLine",
                name: _this.STROKE_NAME,
                dimensions: {
                    x: 10,
                    y: 10,
                    z: 10
                },
                position: position,
                textures: _this.MARKER_TEXTURE_URL,
                color: _this.markerColor,
                lifetime: 5000,
            });

            _this.linePoints = [];
            _this.normals = [];
            _this.strokes.push(_this.currentStroke);
        },

        paint: function(position) {
            var basePosition = position;
            if (!_this.currentStroke) {
                if (_this.oldPosition) {
                    basePosition = _this.oldPosition;
                }
                _this.newStroke(basePosition);
            }

            var localPoint = Vec3.subtract(basePosition, _this.strokeBasePosition);
            localPoint = Vec3.sum(localPoint, Vec3.multiply(_this.whiteboardNormal, _this.strokeForwardOffset));

            if (_this.linePoints.length > 0) {
                var distance = Vec3.distance(localPoint, _this.linePoints[_this.linePoints.length - 1]);
                if (distance < _this.MIN_DISTANCE_BETWEEN_POINTS) {
                    return;
                }
            }
            _this.linePoints.push(localPoint);
            _this.normals.push(_this.whiteboardNormal);

            var strokeWidths = [];
            for (var i = 0; i < _this.linePoints.length; i++) {
                // Create a temp array of stroke widths for calligraphy effect - start and end should be less wide
                var pointsFromCenter = Math.abs(_this.linePoints.length / 2 - i);
                var pointWidth = map(pointsFromCenter, 0, this.linePoints.length / 2, _this.STROKE_WIDTH_RANGE.max, this.STROKE_WIDTH_RANGE.min);
                strokeWidths.push(pointWidth);
            }

            Entities.editEntity(_this.currentStroke, {
                linePoints: _this.linePoints,
                normals: _this.normals,
                strokeWidths: strokeWidths
            });

            if (_this.linePoints.length > MAX_POINTS_PER_STROKE) {
                Entities.editEntity(_this.currentStroke, {
                    parentID: _this.currentWhiteboard
                });
                _this.currentStroke = null;
                _this.oldPosition = position;
            }
        },
        resetStroke: function() {

            Entities.editEntity(_this.currentStroke, {
                parentID: _this.currentWhiteboard
            });
            _this.currentStroke = null;

            _this.oldPosition = null;
        },

        preload: function(entityID) {
            this.entityID = entityID;
            _this.laserPointer = Overlays.addOverlay("circle3d", {
                color: {
                    red: 220,
                    green: 35,
                    blue: 53
                },
                solid: true,
                size: 0.01,
            });

        },

        unload: function() {
            Overlays.deleteOverlay(_this.laserPointer);
            _this.strokes.forEach(function(stroke) {
                Entities.deleteEntity(stroke);
            });
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new MarkerTip();
});
