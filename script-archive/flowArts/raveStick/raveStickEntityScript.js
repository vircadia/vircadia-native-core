//  raveStickEntityScript.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 12/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This entity script create light trails on a given object as it moves.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../../libraries/utils.js");
    var _this;
    var LIFETIME = 6000;
    var DRAWING_DEPTH = 0.8;
    var LINE_DIMENSIONS = 100;
    var MAX_POINTS_PER_LINE = 50;
    var MIN_POINT_DISTANCE = 0.02;
    var STROKE_WIDTH = 0.05
    var ugLSD = 35;
    var RaveStick = function() {
        _this = this;
        this.colorPalette = [{
            red: 0,
            green: 200,
            blue: 40
        }, {
            red: 200,
            green: 10,
            blue: 40
        }];
        var texture = "https://s3.amazonaws.com/hifi-public/eric/textures/paintStrokes/trails.png";
        this.trail = Entities.addEntity({
            type: "PolyLine",
            dimensions: {
                x: LINE_DIMENSIONS,
                y: LINE_DIMENSIONS,
                z: LINE_DIMENSIONS
            },
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            textures: texture,
            lifetime: LIFETIME
        });

        this.points = [];
        this.normals = [];
        this.strokeWidths = [];
    };

    RaveStick.prototype = {
        isGrabbed: false,

        startNearGrab: function() {
            this.trailBasePosition = Entities.getEntityProperties(this.entityID, "position").position;
            Entities.editEntity(this.trail, {
                position: this.trailBasePosition
            });
            this.points = [];
            this.normals = [];
            this.strokeWidths = [];
            this.setupEraseInterval();
        },

        continueNearGrab: function() {
            var props = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            var forwardVec = Quat.getFront(Quat.multiply(props.rotation, Quat.fromPitchYawRollDegrees(-90, 0, 0)));
            forwardVec = Vec3.normalize(forwardVec);
            var forwardQuat = orientationOf(forwardVec);
            var position = Vec3.sum(props.position, Vec3.multiply(Quat.getFront(props.rotation), 0.04));
            var localPoint = Vec3.subtract(position, this.trailBasePosition);
            if (this.points.length >= 1 && Vec3.distance(localPoint, this.points[this.points.length - 1]) < MIN_POINT_DISTANCE) {
                //Need a minimum distance to avoid binormal NANs
                return;
            }
            if (this.points.length === MAX_POINTS_PER_LINE) {
                this.points.shift();
                this.normals.shift();
                this.strokeWidths.shift();
            }

            this.points.push(localPoint);
            var normal = Quat.getUp(props.rotation);
            this.normals.push(normal);
            this.strokeWidths.push(STROKE_WIDTH);
            Entities.editEntity(this.trail, {
                linePoints: this.points,
                normals: this.normals,
                strokeWidths: this.strokeWidths
            });


        },

        setupEraseInterval: function() {
            this.trailEraseInterval = Script.setInterval(function() {
                if (_this.points.length > 0) {
                    _this.points.shift();
                    _this.normals.shift();
                    _this.strokeWidths.shift();
                    Entities.editEntity(_this.trail, {
                        linePoints: _this.points,
                        strokeWidths: _this.strokeWidths,
                        normals: _this.normals
                    });
                }
            }, ugLSD);
        },

        releaseGrab: function() {
            if(!this.trailEraseInterval) {
                return;
            }
            Script.setTimeout(function() {
              Script.clearInterval(_this.trailEraseInterval);
              _this.trailEraseInterval = null;  
            }, 3000);
        },

        preload: function(entityID) {
            this.entityID = entityID;
        },

        unload: function() {
            Entities.deleteEntity(this.beam);
            Entities.deleteEntity(this.trail);
            if (this.trailEraseInterval) {
                Script.clearInterval(this.trailEraseInterval);
            }
        }
    };
    return new RaveStick();

    function computeNormal(p1, p2) {
        return Vec3.normalize(Vec3.subtract(p2, p1));
    }
});