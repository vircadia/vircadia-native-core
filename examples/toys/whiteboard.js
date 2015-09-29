//
//  detectGrabExample.js
//  examples/entityScripts
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to an entity, will detect when the entity is being grabbed by the hydraGrab script
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;
    var MAX_POINTS_PER_LINE = 40;
    var MIN_POINT_DISTANCE = 0.01;
    var STROKE_WIDTH = 0.02;
    var STROKE_COLOR = {red: 200, green: 20, blue: 160};

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    Whiteboard = function() {
        _this = this;

    };

    Whiteboard.prototype = {

        setRightHand: function() {
            this.hand = 'RIGHT';
            this.getHandPosition = MyAvatar.getRightPalmPosition;
            this.getHandRotation = MyAvatar.getRightPalmRotation;
        },
        setLeftHand: function() {
            this.hand = 'LEFT';
            this.getHandPosition = MyAvatar.getLeftPalmPosition;
            this.getHandRotation = MyAvatar.getLeftPalmRotation;
        },

        startNearGrabNonColliding: function() {
            this.whichHand = this.hand;
            this.laserPointer = Entities.addEntity({
                type: "Box",
                dimensions: {
                    x: STROKE_WIDTH,
                    y: STROKE_WIDTH,
                    z: 0.001
                },
                color: STROKE_COLOR,
                rotation: this.rotation
            });

            setEntityCustomData(this.resetKey, this.laserPointer, {
                resetMe: true
            });

        },

        continueNearGrabbingNonColliding: function() {
            var handPosition = this.getHandPosition();
            var pickRay = {
                origin: handPosition,
                direction: Quat.getUp(this.getHandRotation())
            };
            var intersection = Entities.findRayIntersection(pickRay, true);
            if (intersection.intersects) {
                Entities.editEntity(this.laserPointer, {
                    position: intersection.intersection
                });
                this.paint(intersection.intersection, intersection.surfaceNormal);
            }

        },

        releaseGrab: function() {
            Entities.deleteEntity(this.laserPointer);
            this.painting = false;
        },

        paint: function(position, normal) {
            if (!this.painting) {
                this.newStroke(position);
                this.painting = true;
            }

            if (this.strokePoints.length > MAX_POINTS_PER_LINE) {
                this.painting = false;
                return;
            }


            var localPoint = Vec3.subtract(position, this.strokeBasePosition);
            //Move stroke a bit forward along normal so it doesnt zfight with mesh its drawing on 
            localPoint = Vec3.sum(localPoint, Vec3.multiply(normal, .001));

            if (this.strokePoints.length > 0 && Vec3.distance(localPoint, this.strokePoints[this.strokePoints.length - 1]) < MIN_POINT_DISTANCE) {
                //need a minimum distance to avoid binormal NANs
                return;
            }

            this.strokePoints.push(localPoint);
            this.strokeNormals.push(normal);
            this.strokeWidths.push(STROKE_WIDTH);
            Entities.editEntity(this.currentStroke, {
                linePoints: this.strokePoints,
                normals: this.strokeNormals,
                strokeWidths: this.strokeWidths
            });
        },

        newStroke: function(position) {
            print("NEW STROKE")
            this.strokeBasePosition = position;
            this.currentStroke = Entities.addEntity({
                position: position,
                type: "PolyLine",
                color: STROKE_COLOR,
                dimensions: {
                    x: 50,
                    y: 50,
                    z: 50
                },
                lifetime: 100
            });

            setEntityCustomData(this.resetKey, this.currentStroke, {
                resetMe: true
            });
            this.strokePoints = [];
            this.strokeNormals = [];
            this.strokeWidths = [];

            this.strokes.push(this.currentStroke);
        },


        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        preload: function(entityID) {
            this.entityID = entityID;
            var props = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            this.position = props.position;
            this.rotation = props.rotation;
            this.resetKey = "resetMe";
            this.strokes = [];

        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Whiteboard();
})