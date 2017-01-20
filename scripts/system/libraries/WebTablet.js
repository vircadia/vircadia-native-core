//
//  WebTablet.js
//
//  Created by Anthony J. Thibault on 8/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* global getControllerWorldLocation, setEntityCustomData, Tablet, WebTablet:true, HMD, Settings, Script,
   Vec3, Quat, MyAvatar, Entities, Overlays, Camera, Messages, Xform */

Script.include(Script.resolvePath("../libraries/utils.js"));
Script.include(Script.resolvePath("../libraries/controllers.js"));
Script.include(Script.resolvePath("../libraries/Xform.js"));

var X_AXIS = {x: 1, y: 0, z: 0};
var Y_AXIS = {x: 0, y: 1, z: 0};
var DEFAULT_DPI = 34;
var DEFAULT_WIDTH = 0.4375;
var DEFAULT_VERTICAL_FIELD_OF_VIEW = 45; // degrees
var SENSOR_TO_ROOM_MATRIX = -2;
var CAMERA_MATRIX = -7;
var ROT_Y_180 = {x: 0, y: 1, z: 0, w: 0};

var TABLET_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Tablet-Model-v1-x.fbx";

// will need to be recaclulated if dimensions of fbx model change.
var TABLET_NATURAL_DIMENSIONS = {x: 33.797, y: 50.129, z: 2.269};

var HOME_BUTTON_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/tablet-home-button.fbx";

// returns object with two fields:
//    * position - position in front of the user
//    * rotation - rotation of entity so it faces the user.
function calcSpawnInfo(hand, height) {
    var noHands = -1;
    var finalPosition;
    if (HMD.active && hand !== noHands) {
        var handController = getControllerWorldLocation(hand, true);
        var controllerPosition = handController.position;

        // compute the angle of the chord with length (height / 2)
        var theta = Math.asin(height / (2 * Vec3.distance(HMD.position, controllerPosition)));

        // then we can use this angle to rotate the vector between the HMD position and the center of the tablet.
        // this vector, u, will become our new look at direction.
        var d = Vec3.normalize(Vec3.subtract(HMD.position, controllerPosition));
        var w = Vec3.normalize(Vec3.cross(Y_AXIS, d));
        var q = Quat.angleAxis(theta * (180 / Math.PI), w);
        var u = Vec3.multiplyQbyV(q, d);

        // use u to compute a full lookAt quaternion.
        var lookAtRot = Quat.lookAt(controllerPosition, Vec3.sum(controllerPosition, u), Y_AXIS);

        // adjust the tablet position by a small amount.
        var yDisplacement = (height / 2) + 0.1;
        var zDisplacement = 0.05;
        var tabletOffset = Vec3.multiplyQbyV(lookAtRot, {x: 0, y: yDisplacement, z: zDisplacement});
        finalPosition = Vec3.sum(controllerPosition, tabletOffset);
        return {
            position: finalPosition,
            rotation: lookAtRot
        };
    } else {
        var front = Quat.getFront(Camera.orientation);
        finalPosition = Vec3.sum(Camera.position, Vec3.multiply(0.6, front));
        return {
            position: finalPosition,
            rotation: Quat.multiply(Camera.orientation, {x: 0, y: 1, z: 0, w: 0})
        };
    }
}

/**
 * WebTablet
 * @param url [string] url of content to show on the tablet.
 * @param width [number] width in meters of the tablet model
 * @param dpi [number] dpi of web surface used to show the content.
 * @param hand [number] -1 indicates no hand, Controller.Standard.RightHand or Controller.Standard.LeftHand
 * @param clientOnly [bool] true indicates tablet model is only visible to client.
 */
WebTablet = function (url, width, dpi, hand, clientOnly) {

    var _this = this;

    // scale factor of natural tablet dimensions.
    this.width = width || DEFAULT_WIDTH;
    var tabletScaleFactor = this.width / TABLET_NATURAL_DIMENSIONS.x;
    this.height = TABLET_NATURAL_DIMENSIONS.y * tabletScaleFactor;
    this.depth = TABLET_NATURAL_DIMENSIONS.z * tabletScaleFactor;
    this.dpi = dpi || DEFAULT_DPI;

    var tabletProperties = {
        name: "WebTablet Tablet",
        type: "Model",
        modelURL: TABLET_URL,
        userData: JSON.stringify({
            "grabbableKey": {"grabbable": true}
        }),
        dimensions: {x: this.width, y: this.height, z: this.depth},
        parentID: MyAvatar.sessionUUID
    };

    // compute position, rotation & parentJointIndex of the tablet
    this.calculateTabletAttachmentProperties(hand, tabletProperties);

    this.cleanUpOldTablets();
    this.tabletEntityID = Entities.addEntity(tabletProperties, clientOnly);

    if (this.webOverlayID) {
        Overlays.deleteOverlay(this.webOverlayID);
    }

    var WEB_ENTITY_Z_OFFSET = (this.depth / 2);
    var WEB_ENTITY_Y_OFFSET = 0.004;

    this.webOverlayID = Overlays.addOverlay("web3d", {
        name: "WebTablet Web",
        url: url,
        localPosition: { x: 0, y: WEB_ENTITY_Y_OFFSET, z: -WEB_ENTITY_Z_OFFSET },
        localRotation: Quat.angleAxis(180, Y_AXIS),
        resolution: { x: 480, y: 706 },
        dpi: this.dpi,
        color: { red: 255, green: 255, blue: 255 },
        alpha: 1.0,
        parentID: this.tabletEntityID,
        parentJointIndex: -1,
        showKeyboardFocusHighlight: false
    });

    var HOME_BUTTON_Y_OFFSET = (this.height / 2) - 0.035;
    this.homeButtonEntity = Overlays.addOverlay("model", {
        name: "homeButton",
        url: Script.resourcesPath() + "meshes/tablet-home-button.fbx",
        localPosition: {x: 0.0, y: -HOME_BUTTON_Y_OFFSET, z: -0.01},
        localRotation: Quat.angleAxis(0, Y_AXIS),
        solid: true,
        visible: true,
        dimensions: { x: 0.04, y: 0.04, z: 0.02},
        drawInFront: false,
        parentID: this.tabletEntityID,
        parentJointIndex: -1
    });

    this.receive = function (channel, senderID, senderUUID, localOnly) {
        if (_this.homeButtonEntity === parseInt(senderID)) {
            var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
            tablet.gotoHomeScreen();
        }
    };

    this.state = "idle";

    this.getRoot = function() {
        return Entities.getWebViewRoot(_this.tabletEntityID);
    };

    this.getLocation = function() {
        return Entities.getEntityProperties(_this.tabletEntityID, ["localPosition", "localRotation"]);
    };
    this.clicked = false;

    this.myOnHmdChanged = function () {
        _this.onHmdChanged();
    };
    HMD.displayModeChanged.connect(this.myOnHmdChanged);

    this.myMousePressEvent = function (event) {
        _this.mousePressEvent(event);
    };

    this.myMouseMoveEvent = function (event) {
        _this.mouseMoveEvent(event);
    };

    this.myMouseReleaseEvent = function (event) {
        _this.mouseReleaseEvent(event);
    };

    if (!HMD.active) {
        Controller.mousePressEvent.connect(this.myMousePressEvent);
        Controller.mouseMoveEvent.connect(this.myMouseMoveEvent);
        Controller.mouseReleaseEvent.connect(this.myMouseReleaseEvent);
    }

    this.dragging = false;
    this.initialLocalIntersectionPoint = {x: 0, y: 0, z: 0};
    this.initialLocalPosition = {x: 0, y: 0, z: 0};
};

WebTablet.prototype.setURL = function (url) {
    Overlays.editOverlay(this.webOverlayID, { url: url });
};

WebTablet.prototype.setScriptURL = function (scriptURL) {
    Overlays.editOverlay(this.webOverlayID, { scriptURL: scriptURL });
};

WebTablet.prototype.getOverlayObject = function () {
    return Overlays.getOverlayObject(this.webOverlayID);
};

WebTablet.prototype.destroy = function () {
    Overlays.deleteOverlay(this.webOverlayID);
    Entities.deleteEntity(this.tabletEntityID);
    Overlays.deleteEntity(this.homeButtonEntity);
    HMD.displayModeChanged.disconnect(this.myOnHmdChanged);

    if (!HMD.active) {
        Controller.mousePressEvent.disconnect(this.myMousePressEvent);
        Controller.mouseMoveEvent.disconnect(this.myMouseMoveEvent);
        Controller.mouseReleaseEvent.disconnect(this.myMouseReleaseEvent);
    }
};

// calclulate the appropriate position of the tablet in world space, such that it fits in the center of the screen.
// with a bit of padding on the top and bottom.
WebTablet.prototype.calculateWorldAttitudeRelativeToCamera = function () {
    var PADDING_FACTOR = 2.2;
    var fov = (Settings.getValue('fieldOfView') || DEFAULT_VERTICAL_FIELD_OF_VIEW) * (Math.PI / 180);
    var dist = (PADDING_FACTOR * this.height) / (2 * Math.tan(fov / 2));
    return {
        position: Vec3.sum(Camera.position, Vec3.multiply(dist, Quat.getFront(Camera.orientation))),
        rotation: Quat.multiply(Camera.orientation, ROT_Y_180)
    };
};

// compute position, rotation & parentJointIndex of the tablet
WebTablet.prototype.calculateTabletAttachmentProperties = function (hand, tabletProperties) {
    if (HMD.active) {
        // in HMD mode, the tablet should be relative to the sensor to world matrix.
        tabletProperties.parentJointIndex = SENSOR_TO_ROOM_MATRIX;

        // compute the appropriate position of the tablet, near the hand controller that was used to spawn it.
        var spawnInfo = calcSpawnInfo(hand, this.height);
        tabletProperties.position = spawnInfo.position;
        tabletProperties.rotation = spawnInfo.rotation;
    } else {
        // in desktop mode, the tablet should be relative to the camera
        tabletProperties.parentJointIndex = CAMERA_MATRIX;

        // compute the appropriate postion of the tablet such that it fits in the center of the screen nicely.
        var attitude = this.calculateWorldAttitudeRelativeToCamera();
        tabletProperties.position = attitude.position;
        tabletProperties.rotation = attitude.rotation;
    }
};

WebTablet.prototype.onHmdChanged = function () {

    if (HMD.active) {
        Controller.mousePressEvent.disconnect(this.myMousePressEvent);
        Controller.mouseMoveEvent.disconnect(this.myMouseMoveEvent);
        Controller.mouseReleaseEvent.disconnect(this.myMouseReleaseEvent);
    } else {
        Controller.mousePressEvent.connect(this.myMousePressEvent);
        Controller.mouseMoveEvent.connect(this.myMouseMoveEvent);
        Controller.mouseReleaseEvent.connect(this.myMouseReleaseEvent);
    }

    var NO_HANDS = -1;
    var tabletProperties = {};
    // compute position, rotation & parentJointIndex of the tablet
    this.calculateTabletAttachmentProperties(NO_HANDS, tabletProperties);
    Entities.editEntity(this.tabletEntityID, tabletProperties);
};

WebTablet.prototype.pickle = function () {
    return JSON.stringify({ webOverlayID: this.webOverlayID, tabletEntityID: this.tabletEntityID });
};

WebTablet.prototype.register = function() {
    Messages.subscribe("home");
    Messages.messageReceived.connect(this.receive);
};

WebTablet.prototype.cleanUpOldTabletsOnJoint = function(jointIndex) {
    var children = Entities.getChildrenIDsOfJoint(MyAvatar.sessionUUID, jointIndex);
    print("cleanup " + children);
    children.forEach(function(childID) {
        var props = Entities.getEntityProperties(childID, ["name"]);
        if (props.name === "WebTablet Tablet") {
            print("cleaning up " + props.name);
            Entities.deleteEntity(childID);
        } else {
            print("not cleaning up " + props.name);
        }
    });
};

WebTablet.prototype.cleanUpOldTablets = function() {
    this.cleanUpOldTabletsOnJoint(-1);
    this.cleanUpOldTabletsOnJoint(SENSOR_TO_ROOM_MATRIX);
    this.cleanUpOldTabletsOnJoint(CAMERA_MATRIX);
    this.cleanUpOldTabletsOnJoint(65529);
};

WebTablet.prototype.unregister = function() {
    Messages.unsubscribe("home");
    Messages.messageReceived.disconnect(this.receive);
};

WebTablet.unpickle = function (string) {
    if (!string) {
        return;
    }
    var tablet = JSON.parse(string);
    tablet.__proto__ = WebTablet.prototype;
    return tablet;
};

WebTablet.prototype.getPosition = function () {
    return Overlays.getProperty(this.webOverlayID, "position");
};

WebTablet.prototype.mousePressEvent = function (event) {
    var pickRay = Camera.computePickRay(event.x, event.y);
    var entityPickResults = Entities.findRayIntersection(pickRay, true); // non-accurate picking
    if (entityPickResults.intersects && entityPickResults.entityID === this.tabletEntityID) {
        var overlayPickResults = Overlays.findRayIntersection(pickRay);
        if (!overlayPickResults.intersects || !overlayPickResults.overlayID === this.webOverlayID) {
            this.dragging = true;
            var invCameraXform = new Xform(Camera.orientation, Camera.position).inv();
            this.initialLocalIntersectionPoint = invCameraXform.xformPoint(entityPickResults.intersection);
            this.initialLocalPosition = Entities.getEntityProperties(this.tabletEntityID, ["localPosition"]).localPosition;
        }
    }
};

function rayIntersectPlane(planePosition, planeNormal, rayStart, rayDirection) {
    var rayDirectionDotPlaneNormal = Vec3.dot(rayDirection, planeNormal);
    if (rayDirectionDotPlaneNormal > 0.00001 || rayDirectionDotPlaneNormal < -0.00001) {
        var rayStartDotPlaneNormal = Vec3.dot(Vec3.subtract(planePosition, rayStart), planeNormal);
        var distance = rayStartDotPlaneNormal / rayDirectionDotPlaneNormal;
        return {hit: true, distance: distance};
    } else {
        // ray is parallel to the plane
        return {hit: false, distance: 0};
    }
}

WebTablet.prototype.mouseMoveEvent = function (event) {
    if (this.dragging) {
        var pickRay = Camera.computePickRay(event.x, event.y);

        // transform pickRay into camera local coordinates
        var invCameraXform = new Xform(Camera.orientation, Camera.position).inv();
        var localPickRay = {
            origin: invCameraXform.xformPoint(pickRay.origin),
            direction: invCameraXform.xformVector(pickRay.direction)
        };

        var NORMAL = {x: 0, y: 0, z: -1};
        var result = rayIntersectPlane(this.initialLocalIntersectionPoint, NORMAL, localPickRay.origin, localPickRay.direction);
        if (result.hit) {
            var localIntersectionPoint = Vec3.sum(localPickRay.origin, Vec3.multiply(localPickRay.direction, result.distance));
            var localOffset = Vec3.subtract(localIntersectionPoint, this.initialLocalIntersectionPoint);
            var localPosition = Vec3.sum(this.initialLocalPosition, localOffset);
            Entities.editEntity(this.tabletEntityID, {
                localPosition: localPosition
            });
        }
    }
};

WebTablet.prototype.mouseReleaseEvent = function (event) {
    this.dragging = false;
};
