var MOUSE_SENSITIVITY = 0.5;
var SCROLL_SENSITIVITY = 0.05;
var PAN_ZOOM_SCALE_RATIO = 0.1;
var ZOOM_SCALING = 0.02;

var MIN_ZOOM_DISTANCE = 0.01;
var MAX_ZOOM_DISTANCE = 100;

var MODE_INACTIVE = null;
var MODE_ORBIT = 'orbit';
var MODE_PAN = 'pan';

var INITIAL_ZOOM_DISTANCE = 4;

EntityCameraTool = function() {
    var that = {};

    that.enabled = false;
    that.mode = MODE_INACTIVE;

    that.zoomDistance = INITIAL_ZOOM_DISTANCE;
    that.yaw = 0;
    that.pitch = 0;
    that.focalPoint = { x: 0, y: 0, z: 0 };

    that.previousCameraMode = null;

    that.lastMousePosition = { x: 0, y: 0 };

    that.enable = function() {
        if (that.enabled) return;
        that.enabled = true;
        that.mode = MODE_INACTIVE;

        // Pick a point INITIAL_ZOOM_DISTANCE in front of the camera to use
        // as a focal point
        that.zoomDistance = INITIAL_ZOOM_DISTANCE;
        var focalPoint = Vec3.sum(Camera.getPosition(),
                        Vec3.multiply(that.zoomDistance, Quat.getFront(Camera.getOrientation())));

        // Determine the correct yaw and pitch to keep the camera in the same location
        var dPos = Vec3.subtract(focalPoint, Camera.getPosition());
        var xzDist = Math.sqrt(dPos.x * dPos.x + dPos.z * dPos.z);

        that.pitch = -Math.atan2(dPos.y, xzDist) * 180 / Math.PI;
        that.yaw = Math.atan2(dPos.x, dPos.z) * 180 / Math.PI;

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
        that.setFocalPoint(entityProperties.position);

        that.updateCamera();
    }

    that.moveFocalPoint = function(dPos) {
        that.setFocalPoint(Vec3.sum(that.focalPoint, dPos));
    }

    that.setFocalPoint = function(pos) {
        that.focalPoint = pos
        that.updateCamera();
    }

    that.mouseMoveEvent = function(event) {
        if (that.enabled && that.mode != MODE_INACTIVE) {
            if (that.mode == MODE_ORBIT) {
                var diffX = event.x - that.lastMousePosition.x;
                var diffY = event.y - that.lastMousePosition.y;
                that.yaw -= MOUSE_SENSITIVITY * (diffX / 5.0)
                that.pitch += MOUSE_SENSITIVITY * (diffY / 10.0)

                while (that.yaw > 180.0) that.yaw -= 360;
                while (that.yaw < -180.0) that.yaw += 360;

                if (that.pitch > 90) that.pitch = 90;
                if (that.pitch < -90) that.pitch = -90;

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
        dZoom *= that.zoomDistance * ZOOM_SCALING;

        that.zoomDistance = Math.max(Math.min(that.zoomDistance + dZoom, MAX_ZOOM_DISTANCE), MIN_ZOOM_DISTANCE);

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

    Controller.wheelEvent.connect(that.wheelEvent);

    return that;
}
