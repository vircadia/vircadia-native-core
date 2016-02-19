//
//  markerTipEntityScript.js
//  examples/homeContent/markerTipEntityScript
//
//  Created by Eric Levin on 2/17/15.
//  Copyright 2016 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html



(function() {
    Script.include("../../libraries/utils.js");

    var MAX_POINTS_PER_STROKE = 40;

    MarkerTip = function() {
        _this = this;
        _this.MARKER_TEXTURE_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/textures/markerStroke.png";
        this.strokeForwardOffset = 0.0005;
        this.STROKE_FORWARD_OFFSET_INCRERMENT = 0.00001;
        this.STROKE_WIDTH = 0.003
        _this.MAX_MARKER_TO_BOARD_DISTANCE = 0.2;
        _this.MIN_DISTANCE_BETWEEN_POINTS = 0.002;
        _this.MAX_DISTANCE_BETWEEN_POINTS = 0.1;
        _this.strokes = [];
    };

    MarkerTip.prototype = {

        continueNearGrab: function() {

            _this.continueHolding();
        },

        continueEquip: function() {
            _this.continueHolding();
        },

        continueHolding: function() {
            // cast a ray from marker and see if it hits anything

            var markerProps = Entities.getEntityProperties(_this.entityID, ["position", "rotation"]);


            var pickRay = {
                origin: markerProps.position,
                direction: Quat.getFront(markerProps.rotation)
            }

            var intersection = Entities.findRayIntersection(pickRay, true, [_this.whiteboard]);

            if (intersection.intersects && Vec3.distance(intersection.intersection, markerProps.position) < _this.MAX_MARKER_TO_BOARD_DISTANCE) {
                this.paint(intersection.intersection)
            } else {
                _this.currentStroke = null;
                _this.oldPosition = null;
            }
        },

        newStroke: function(position) {
            _this.strokeBasePosition = position;
            _this.currentStroke = Entities.addEntity({
                type: "PolyLine",
                name: "marker stroke",
                dimensions: {
                    x: 10,
                    y: 10,
                    z: 10
                },
                position: position,
                textures: _this.MARKER_TEXTURE_URL,
                color: _this.markerColor
            });

            _this.linePoints = [];
            _this.normals = [];
            _this.strokeWidths = [];

            _this.strokes.push(_this.currentStroke);
        },

        paint: function(position) {
            var basePosition = position;
            if (!_this.currentStroke) {
                if (_this.oldPosition) {
                    basePosition = _this.oldPosition;
                    print("EBL USE OLD POSITION FOR NEW STROKE!")
                }
               _this.newStroke(basePosition);
            }

            var localPoint = Vec3.subtract(basePosition, this.strokeBasePosition);
            localPoint = Vec3.sum(localPoint, Vec3.multiply(_this.whiteboardNormal, _this.strokeForwardOffset));
            // _this.strokeForwardOffset += _this.STROKE_FORWARD_OFFSET_INCRERMENT;

            if (_this.linePoints.length > 0) {
                var distance  = Vec3.distance(localPoint, _this.linePoints[_this.linePoints.length-1]);
                if (distance < _this.MIN_DISTANCE_BETWEEN_POINTS) {
                    return;
                }
            }

            _this.linePoints.push(localPoint);
            _this.normals.push(_this.whiteboardNormal);
            this.strokeWidths.push(_this.STROKE_WIDTH);

            Entities.editEntity(_this.currentStroke, {
                linePoints: _this.linePoints,
                normals: _this.normals,
                strokeWidths: _this.strokeWidths
            });

            if (_this.linePoints.length > MAX_POINTS_PER_STROKE) {
                _this.currentStroke = null;
                _this.oldPosition = position;
            }
        },

        preload: function(entityID) {
            this.entityID = entityID;
           
            print("EBL PRELOAD");
        },

        setProperties: function(myId, data) {
            var data = JSON.parse(data);
            _this.whiteboard = data.whiteboard;
            var whiteboardProps = Entities.getEntityProperties(_this.whiteboard, ["rotation"]);
            print("EBL: MARKER COLOR " + JSON.stringify(data.markerColor));
            _this.whiteboardNormal = Quat.getRight(whiteboardProps.rotation);
            _this.markerColor = data.markerColor;
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new MarkerTip();
});