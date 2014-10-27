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

CameraManager = function() {
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

        cameraTool.setVisible(true);
    }

    that.disable = function(ignoreCamera) {
        if (!that.enabled) return;
        that.enabled = false;
        that.mode = MODE_INACTIVE;

        if (!ignoreCamera) {
            Camera.setMode(that.previousCameraMode);
        }
        cameraTool.setVisible(false);
    }

    that.focus = function(entityProperties) {
        var dim = SelectionManager.worldDimensions;
        var size = Math.max(dim.x, Math.max(dim.y, dim.z));

        that.targetZoomDistance = Math.max(size * FOCUS_ZOOM_SCALE, FOCUS_MIN_ZOOM);

        that.setFocalPoint(SelectionManager.worldPosition);

        that.updateCamera();
    }

    that.moveFocalPoint = function(dPos) {
        that.setFocalPoint(Vec3.sum(that.focalPoint, dPos));
    }

    that.setFocalPoint = function(pos) {
        that.targetFocalPoint = pos
        that.updateCamera();
    }

    that.addYaw = function(yaw) {
        that.targetYaw += yaw;
        that.updateCamera();
    }

    that.addPitch = function(pitch) {
        that.targetPitch += pitch;
        that.updateCamera();
    }

    that.addZoom = function(zoom) {
        zoom *= that.targetZoomDistance * ZOOM_SCALING;
        that.targetZoomDistance = Math.min(Math.max(that.targetZoomDistance + zoom, MIN_ZOOM_DISTANCE), MAX_ZOOM_DISTANCE);
        that.updateCamera();
    }

    that.getZoomPercentage = function() {
        return (that.zoomDistance - MIN_ZOOM_DISTANCE) / MAX_ZOOM_DISTANCE;
    }

    that.setZoomPercentage = function(pct) {
        that.targetZoomDistance = pct * (MAX_ZOOM_DISTANCE - MIN_ZOOM_DISTANCE);
    }

    that.pan = function(offset) {
        var up = Quat.getUp(Camera.getOrientation());
        var right = Quat.getRight(Camera.getOrientation());

        up = Vec3.multiply(up, offset.y * 0.01 * PAN_ZOOM_SCALE_RATIO * that.zoomDistance);
        right = Vec3.multiply(right, offset.x * 0.01 * PAN_ZOOM_SCALE_RATIO * that.zoomDistance);

        var dPosition = Vec3.sum(up, right);

        that.moveFocalPoint(dPosition);
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

        return cameraTool.mousePressEvent(event);
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

        that.targetZoomDistance = Math.min(Math.max(that.targetZoomDistance + dZoom, MIN_ZOOM_DISTANCE), MAX_ZOOM_DISTANCE);

        that.updateCamera();
    }

    that.updateCamera = function() {
        if (!that.enabled || Camera.getMode() != "independent") return;

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
        if (Camera.getMode() != "independent") {
            return;
        }

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

    // Last mode that was first or third person
    var lastAvatarCameraMode = "first person";
    Camera.modeUpdated.connect(function(newMode) {
        print("Camera mode has been updated: " + newMode);
        if (newMode == "first person" || newMode == "third person") {
            lastAvatarCameraMode = newMode;
            that.disable(true);
        } else {
            that.enable();
        }
    });

    Controller.keyReleaseEvent.connect(function (event) {
        if (event.text == "ESC" && that.enabled) {
            Camera.setMode(lastAvatarCameraMode);
            cameraManager.disable(true);
        }
    });

    Script.update.connect(that.update);

    Controller.wheelEvent.connect(that.wheelEvent);

    var cameraTool = new CameraTool(that);

    return that;
}

var ZoomTool = function(opts) {
    var that = {};

    var position = opts.position || { x: 0, y: 0 };
    var height = opts.height || 200;
    var color = opts.color || { red: 255, green: 0, blue: 0 };
    var arrowButtonSize = opts.buttonSize || 20;
    var arrowButtonBackground = opts.arrowBackground || { red: 255, green: 255, blue: 255 };
    var zoomBackground = { red: 128, green: 0, blue: 0 };
    var zoomHeight = height - (arrowButtonSize * 2);
    var zoomBarY = position.y + arrowButtonSize,

    var onIncreasePressed = opts.onIncreasePressed;
    var onDecreasePressed = opts.onDecreasePressed;
    var onPercentageSet = opts.onPercentageSet;

    var increaseButton = Overlays.addOverlay("text", {
        x: position.x,
        y: position.y,
        width: arrowButtonSize,
        height: arrowButtonSize,
        color: color,
        backgroundColor: arrowButtonBackground,
        topMargin: 4,
        leftMargin: 4,
        text: "+",
        alpha: 1.0,
        visible: true,
    });
    var decreaseButton = Overlays.addOverlay("text", {
        x: position.x,
        y: position.y + arrowButtonSize + zoomHeight,
        width: arrowButtonSize,
        height: arrowButtonSize,
        color: color,
        backgroundColor: arrowButtonBackground,
        topMargin: 4,
        leftMargin: 4,
        text: "-",
        alpha: 1.0,
        visible: true,
    });
    var zoomBar = Overlays.addOverlay("text", {
        x: position.x + 5,
        y: zoomBarY,
        width: 10,
        height: zoomHeight,
        color: { red: 0, green: 255, blue: 0 },
        backgroundColor: zoomBackground,
        topMargin: 4,
        leftMargin: 4,
        text: "",
        alpha: 1.0,
        visible: true,
    });
    var zoomHandle = Overlays.addOverlay("text", {
        x: position.x,
        y: position.y + arrowButtonSize,
        width: arrowButtonSize,
        height: 10,
        backgroundColor: { red: 0, green: 255, blue: 0 },
        topMargin: 4,
        leftMargin: 4,
        text: "",
        alpha: 1.0,
        visible: true,
    });

    var allOverlays = [
        increaseButton,
        decreaseButton,
        zoomBar,
        zoomHandle,
    ];

    that.destroy = function() {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.deleteOverlay(allOverlays[i]);
        }
    };

    that.setVisible = function(visible) {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.editOverlay(allOverlays[i], { visible: visible });
        }
    }

    that.setZoomPercentage = function(pct) {
        var yOffset = (zoomHeight - 10) * pct;
        Overlays.editOverlay(zoomHandle, {
            y: position.y + arrowButtonSize + yOffset,
        });
    }

    that.mouseReleaseEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
        var clicked = false;
        if (clickedOverlay == increaseButton) {
            if (onIncreasePressed) onIncreasePressed();
            clicked = true;
        } else if (clickedOverlay == decreaseButton) {
            if (onDecreasePressed) onDecreasePressed();
            clicked = true;
        } else if (clickedOverlay == zoomBar) {
            if (onPercentageSet) onPercentageSet((event.y - zoomBarY) / zoomHeight);
            clicked = true;
        }
        return clicked;
    }

    return that;
};

var ArrowTool = function(opts) {
    var that = {};

    var position = opts.position || { x: 0, y: 0 };
    var arrowButtonSize = opts.buttonSize || 20;
    var color = opts.color || { red: 255, green: 0, blue: 0 };
    var arrowButtonBackground = opts.arrowBackground || { red: 255, green: 255, blue: 255 };
    var centerButtonBackground = opts.centerBackground || { red: 255, green: 255, blue: 255 };
    var onUpPressed = opts.onUpPressed;
    var onDownPressed = opts.onDownPressed;
    var onLeftPressed = opts.onLeftPressed;
    var onRightPressed = opts.onRightPressed;
    var onCenterPressed = opts.onCenterPressed;

    var upButton = Overlays.addOverlay("text", {
        x: position.x + arrowButtonSize,
        y: position.y,
        width: arrowButtonSize,
        height: arrowButtonSize,
        color: color,
        backgroundColor: arrowButtonBackground,
        topMargin: 4,
        leftMargin: 4,
        text: "^",
        alpha: 1.0,
        visible: true,
    });
    var leftButton = Overlays.addOverlay("text", {
        x: position.x,
        y: position.y + arrowButtonSize,
        width: arrowButtonSize,
        height: arrowButtonSize,
        color: color,
        backgroundColor: arrowButtonBackground,
        topMargin: 4,
        leftMargin: 4,
        text: "<",
        alpha: 1.0,
        visible: true,
    });
    var rightButton = Overlays.addOverlay("text", {
        x: position.x + (arrowButtonSize * 2),
        y: position.y + arrowButtonSize,
        width: arrowButtonSize,
        height: arrowButtonSize,
        color: color,
        backgroundColor: arrowButtonBackground,
        topMargin: 4,
        leftMargin: 4,
        text: ">",
        alpha: 1.0,
        visible: true,
    });
    var downButton = Overlays.addOverlay("text", {
        x: position.x + arrowButtonSize,
        y: position.y + (arrowButtonSize * 2),
        width: arrowButtonSize,
        height: arrowButtonSize,
        color: color,
        backgroundColor: arrowButtonBackground,
        topMargin: 4,
        leftMargin: 4,
        text: "v",
        alpha: 1.0,
        visible: true,
    });
    var centerButton = Overlays.addOverlay("text", {
        x: position.x + arrowButtonSize,
        y: position.y + arrowButtonSize,
        width: arrowButtonSize,
        height: arrowButtonSize,
        color: color,
        backgroundColor: centerButtonBackground,
        topMargin: 4,
        leftMargin: 4,
        text: "",
        alpha: 1.0,
        visible: true,
    });

    var allOverlays = [
        upButton,
        downButton,
        leftButton,
        rightButton,
        centerButton,
    ];

    that.destroy = function() {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.deleteOverlay(allOverlays[i]);
        }
    };

    that.setVisible = function(visible) {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.editOverlay(allOverlays[i], { visible: visible });
        }
    }

    that.mouseReleaseEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
        var clicked = false;
        if (clickedOverlay == leftButton) {
            if (onLeftPressed) onLeftPressed();
            clicked = true;
        } else if (clickedOverlay == rightButton) {
            if (onRightPressed) onRightPressed();
            clicked = true;
        } else if (clickedOverlay == upButton) {
            if (onUpPressed) onUpPressed();
            clicked = true;
        } else if (clickedOverlay == downButton) {
            if (onDownPressed) onDownPressed();
            clicked = true;
        } else if (clickedOverlay == centerButton) {
            if (onCenterPressed) onCenterPressed();
            clicked = true;
        }
        return clicked;
    }

    return that;
}


CameraTool = function(cameraManager) {
    var that = {};

    var toolsPosition = { x: 20, y: 280 };
    var orbitToolPosition = toolsPosition;
    var panToolPosition = { x: toolsPosition.x + 80, y: toolsPosition.y };
    var zoomToolPosition = { x: toolsPosition.x + 20, y: toolsPosition.y + 80 };

    var orbitIncrement = 15;
    orbitTool = ArrowTool({
        position: orbitToolPosition,
        arrowBackground: { red: 192, green: 192, blue: 192 },
        centerBackground: { red: 128, green: 128, blue: 255 },
        color: { red: 0, green: 0, blue: 0 },
        onUpPressed: function() { cameraManager.addPitch(orbitIncrement); },
        onDownPressed: function() { cameraManager.addPitch(-orbitIncrement); },
        onLeftPressed: function() { cameraManager.addYaw(-orbitIncrement); },
        onRightPressed: function() { cameraManager.addYaw(orbitIncrement); },
        onCenterPressed: function() { cameraManager.focus(); },
    });
    panTool = ArrowTool({
        position: panToolPosition,
        arrowBackground: { red: 192, green: 192, blue: 192 },
        centerBackground: { red: 128, green: 128, blue: 255 },
        color: { red: 0, green: 0, blue: 0 },
        onUpPressed: function() { cameraManager.pan({ x: 0, y: 15 }); },
        onDownPressed: function() { cameraManager.pan({ x: 0, y: -15 }); },
        onLeftPressed: function() { cameraManager.pan({ x: -15, y: 0 }); },
        onRightPressed: function() { cameraManager.pan({ x: 15, y: 0 }); },
    });
    zoomTool = ZoomTool({
        position: zoomToolPosition,
        arrowBackground: { red: 192, green: 192, blue: 192 },
        color: { red: 0, green: 0, blue: 0 },
        onIncreasePressed: function() { cameraManager.addZoom(-10); },
        onDecreasePressed: function() { cameraManager.addZoom(10); },
        onPercentageSet: function(pct) { cameraManager.setZoomPercentage(pct); }
    });

    Script.scriptEnding.connect(function() {
        orbitTool.destroy();
        panTool.destroy();
        zoomTool.destroy();
    });

    that.mousePressEvent = function(event) {
        return orbitTool.mouseReleaseEvent(event)
            || panTool.mouseReleaseEvent(event)
            || zoomTool.mouseReleaseEvent(event);
    };

    that.setVisible = function(visible) {
        orbitTool.setVisible(visible);
        panTool.setVisible(visible);
        zoomTool.setVisible(visible);
    };

    Script.update.connect(function() {
        cameraManager.getZoomPercentage();
        zoomTool.setZoomPercentage(cameraManager.getZoomPercentage());
    });

    that.setVisible(false);

    return that;
};
