//
//  entityCameraTool.js
//  examples
//
//  Created by Ryan Huffman on 10/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var MOUSE_SENSITIVITY = 0.9;
var SCROLL_SENSITIVITY = 0.05;
var PAN_ZOOM_SCALE_RATIO = 0.4;

// Scaling applied based on the size of the object being focused
var FOCUS_ZOOM_SCALE = 1.3;

// Minimum zoom level when focusing on an object
var FOCUS_MIN_ZOOM = 0.5;

// Scaling applied based on the current zoom level
var ZOOM_SCALING = 0.02;

var MIN_ZOOM_DISTANCE = 0.01;
var MAX_ZOOM_DISTANCE = 200;

var MODE_INACTIVE = null;
var MODE_ORBIT = 'orbit';
var MODE_PAN = 'pan';

var EASING_MULTIPLIER = 8;

var INITIAL_ZOOM_DISTANCE = 2;
var INITIAL_ZOOM_DISTANCE_FIRST_PERSON = 3;

EntityCameraTool = function() {
    var that = {};

    that.enabled = false;
    that.mode = MODE_INACTIVE;

    that.zoomDistance = INITIAL_ZOOM_DISTANCE;
    that.targetZoomDistance = INITIAL_ZOOM_DISTANCE;

    that.yaw = 0;
    that.pitch = 0;
    that.targetYaw = 0;
    that.targetPitch = 0;

    that.focalPoint = { x: 0, y: 0, z: 0 };
    that.targetFocalPoint = { x: 0, y: 0, z: 0 };

    that.previousCameraMode = null;

    that.lastMousePosition = { x: 0, y: 0 };

    that.enable = function() {
        if (that.enabled) return;
        that.enabled = true;
        that.mode = MODE_INACTIVE;

        // Pick a point INITIAL_ZOOM_DISTANCE in front of the camera to use as a focal point
        that.zoomDistance = INITIAL_ZOOM_DISTANCE;
        that.targetZoomDistance = that.zoomDistance;
        var focalPoint = Vec3.sum(Camera.getPosition(),
                        Vec3.multiply(that.zoomDistance, Quat.getFront(Camera.getOrientation())));

        if (Camera.getMode() == 'first person') {
            that.targetZoomDistance = INITIAL_ZOOM_DISTANCE_FIRST_PERSON;
        }

        // Determine the correct yaw and pitch to keep the camera in the same location
        var dPos = Vec3.subtract(focalPoint, Camera.getPosition());
        var xzDist = Math.sqrt(dPos.x * dPos.x + dPos.z * dPos.z);

        that.targetPitch = -Math.atan2(dPos.y, xzDist) * 180 / Math.PI;
        that.targetYaw = Math.atan2(dPos.x, dPos.z) * 180 / Math.PI;
        that.pitch = that.targetPitch;
        that.yaw = that.targetYaw;

        that.focalPoint = focalPoint;
        that.setFocalPoint(focalPoint);
        that.previousCameraMode = Camera.getMode();
        Camera.setMode("independent");

        that.updateCamera();
    }

    that.disable = function() {
        if (!that.enabled) return;
        that.enabled = false;
        that.mode = MODE_INACTIVE;

        Camera.setMode(that.previousCameraMode);
    }

    that.focus = function(entityProperties) {
        var dim = entityProperties.dimensions;
        dim = SelectionManager.worldDimensions;
        var size = Math.max(dim.x, Math.max(dim.y, dim.z));

        that.targetZoomDistance = Math.max(size * FOCUS_ZOOM_SCALE, FOCUS_MIN_ZOOM);

        that.setFocalPoint(SelectionManager.worldPosition);//entityProperties.position);

        that.updateCamera();
    }

    that.moveFocalPoint = function(dPos) {
        that.setFocalPoint(Vec3.sum(that.focalPoint, dPos));
    }

    that.setFocalPoint = function(pos) {
        that.targetFocalPoint = pos
        that.updateCamera();
    }

    that.mouseMoveEvent = function(event) {
        if (that.enabled && that.mode != MODE_INACTIVE) {
            if (that.mode == MODE_ORBIT) {
                var diffX = event.x - that.lastMousePosition.x;
                var diffY = event.y - that.lastMousePosition.y;
                that.targetYaw -= MOUSE_SENSITIVITY * (diffX / 5.0)
                that.targetPitch += MOUSE_SENSITIVITY * (diffY / 10.0)

                while (that.targetYaw > 180.0) that.targetYaw -= 360;
                while (that.targetYaw < -180.0) that.targetYaw += 360;

                if (that.targetPitch > 90) that.targetPitch = 90;
                if (that.targetPitch < -90) that.targetPitch = -90;

                that.updateCamera();
            } else if (that.mode == MODE_PAN) {
                var diffX = event.x - that.lastMousePosition.x;
                var diffY = event.y - that.lastMousePosition.y;

                var up = Quat.getUp(Camera.getOrientation());
                var right = Quat.getRight(Camera.getOrientation());

                up = Vec3.multiply(up, diffY * 0.01 * PAN_ZOOM_SCALE_RATIO * that.zoomDistance);
                right = Vec3.multiply(right, -diffX * 0.01 * PAN_ZOOM_SCALE_RATIO * that.zoomDistance);

                var dPosition = Vec3.sum(up, right);

                that.moveFocalPoint(dPosition);
            }
            that.lastMousePosition.x = event.x;
            that.lastMousePosition.y = event.y;

            return true;
        }
        return false;
    }

    that.mousePressEvent = function(event) {
        if (!that.enabled) return;

        if (event.isRightButton || (event.isLeftButton && event.isControl && !event.isShifted)) {
            that.mode = MODE_ORBIT;
            that.lastMousePosition.x = event.x;
            that.lastMousePosition.y = event.y;
            return true;
        } else if (event.isMiddleButton || (event.isLeftButton && event.isControl && event.isShifted)) {
            that.mode = MODE_PAN;
            that.lastMousePosition.x = event.x;
            that.lastMousePosition.y = event.y;
            return true;
        }

        return false;
    }

    that.mouseReleaseEvent = function(event) {
        if (!that.enabled) return;

        that.mode = MODE_INACTIVE;
    }

    that.wheelEvent = function(event) {
        if (!that.enabled) return;

        var dZoom = -event.delta * SCROLL_SENSITIVITY;

        // Scale based on current zoom level
        dZoom *= that.targetZoomDistance * ZOOM_SCALING;

        that.targetZoomDistance = Math.max(that.targetZoomDistance + dZoom, MIN_ZOOM_DISTANCE);

        that.updateCamera();
    }

    that.updateCamera = function() {
        if (!that.enabled) return;

        var yRot = Quat.angleAxis(that.yaw, { x: 0, y: 1, z: 0 });
        var xRot = Quat.angleAxis(that.pitch, { x: 1, y: 0, z: 0 });
        var q = Quat.multiply(yRot, xRot);

        var pos = Vec3.multiply(Quat.getFront(q), that.zoomDistance);
        Camera.setPosition(Vec3.sum(that.focalPoint, pos));

        yRot = Quat.angleAxis(that.yaw - 180, { x: 0, y: 1, z: 0 });
        xRot = Quat.angleAxis(-that.pitch, { x: 1, y: 0, z: 0 });
        q = Quat.multiply(yRot, xRot);

        Camera.setOrientation(q);
    }

    function normalizeDegrees(degrees) {
        while (degrees > 180) degrees -= 360;
        while (degrees < -180) degrees += 360;
        return degrees;
    }

    // Ease the position and orbit of the camera
    that.update = function(dt) {
        var scale = Math.min(dt * EASING_MULTIPLIER, 1.0);

        var dYaw = that.targetYaw - that.yaw;
        if (dYaw > 180) dYaw -= 360;
        if (dYaw < -180) dYaw += 360;

        var dPitch = that.targetPitch - that.pitch;

        that.yaw += scale * dYaw;
        that.pitch += scale * dPitch;

        // Normalize between [-180, 180]
        that.yaw = normalizeDegrees(that.yaw);
        that.pitch = normalizeDegrees(that.pitch);

        var dFocal = Vec3.subtract(that.targetFocalPoint, that.focalPoint);
        that.focalPoint = Vec3.sum(that.focalPoint, Vec3.multiply(scale, dFocal));

        var dZoom = that.targetZoomDistance - that.zoomDistance;
        that.zoomDistance += scale * dZoom;

        that.updateCamera();
    }

    Script.update.connect(that.update);

    Controller.wheelEvent.connect(that.wheelEvent);

    return that;
}
