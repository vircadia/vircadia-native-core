//
//  WebTablet.js
//
//  Created by Anthony J. Thibault on 8/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* global getControllerWorldLocation, Tablet, WebTablet:true, HMD, Settings, Script,
   Vec3, Quat, MyAvatar, Entities, Overlays, Camera, Messages, Xform, clamp, Controller, Mat4 */

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
var TABLET_TEXTURE_RESOLUTION = { x: 480, y: 706 };
var INCHES_TO_METERS = 1 / 39.3701;
var AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

var NO_HANDS = -1;

// will need to be recaclulated if dimensions of fbx model change.
var TABLET_NATURAL_DIMENSIONS = {x: 33.797, y: 50.129, z: 2.269};

var HOME_BUTTON_TEXTURE = "http://hifi-content.s3.amazonaws.com/alan/dev/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-close.png";
// var HOME_BUTTON_TEXTURE = Script.resourcesPath() + "meshes/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-close.png";
var TABLET_MODEL_PATH = "http://hifi-content.s3.amazonaws.com/alan/dev/tablet-with-home-button.fbx";
var LOCAL_TABLET_MODEL_PATH = Script.resourcesPath() + "meshes/tablet-with-home-button.fbx";

// returns object with two fields:
//    * position - position in front of the user
//    * rotation - rotation of entity so it faces the user.
function calcSpawnInfo(hand, height) {
    var finalPosition;

    var headPos = (HMD.active && Camera.mode === "first person") ? HMD.position : Camera.position;
    var headRot = (HMD.active && Camera.mode === "first person") ? HMD.orientation : Camera.orientation;

    if (HMD.active && hand !== NO_HANDS) {
        var handController = getControllerWorldLocation(hand, true);
        var controllerPosition = handController.position;

        // base of the tablet is slightly above controller position
        var TABLET_BASE_DISPLACEMENT = {x: 0, y: 0.1, z: 0};
        var tabletBase = Vec3.sum(controllerPosition, TABLET_BASE_DISPLACEMENT);

        var d = Vec3.subtract(headPos, tabletBase);
        var theta = Math.acos(d.y / Vec3.length(d));
        d.y = 0;
        if (Vec3.length(d) < 0.0001) {
            d = {x: 1, y: 0, z: 0};
        } else {
            d = Vec3.normalize(d);
        }
        var w = Vec3.normalize(Vec3.cross(Y_AXIS, d));
        var ANGLE_OFFSET = 25;
        var q = Quat.angleAxis(theta * (180 / Math.PI) - (90 - ANGLE_OFFSET), w);
        var u = Vec3.multiplyQbyV(q, d);

        // use u to compute a full lookAt quaternion.
        var lookAtRot = Quat.lookAt(tabletBase, Vec3.sum(tabletBase, u), Y_AXIS);
        var yDisplacement = (height / 2);
        var zDisplacement = 0.05;
        var tabletOffset = Vec3.multiplyQbyV(lookAtRot, {x: 0, y: yDisplacement, z: zDisplacement});
        finalPosition = Vec3.sum(tabletBase, tabletOffset);

        return {
            position: finalPosition,
            rotation: lookAtRot
        };
    } else {
        var front = Quat.getFront(headRot);
        finalPosition = Vec3.sum(headPos, Vec3.multiply(0.6, front));
        var orientation = Quat.lookAt({x: 0, y: 0, z: 0}, front, {x: 0, y: 1, z: 0});
        return {
            position: finalPosition,
            rotation: Quat.multiply(orientation, {x: 0, y: 1, z: 0, w: 0})
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

    if (dpi) {
        this.dpi = dpi;
    } else {
        this.dpi = DEFAULT_DPI * (DEFAULT_WIDTH / this.width);
    }

    var modelURL;
    if (Settings.getValue("tabletVisibleToOthers")) {
        modelURL = TABLET_MODEL_PATH;
    } else {
        modelURL = LOCAL_TABLET_MODEL_PATH;
    }

    var tabletProperties = {
        name: "WebTablet Tablet",
        type: "Model",
        modelURL: modelURL,
        url: modelURL, // for overlay
        grabbable: true, // for overlay
        userData: JSON.stringify({
            "grabbableKey": {"grabbable": true}
        }),
        dimensions: {x: this.width, y: this.height, z: this.depth},
        parentID: AVATAR_SELF_ID
    };

    // compute position, rotation & parentJointIndex of the tablet
    this.calculateTabletAttachmentProperties(hand, true, tabletProperties);

    this.cleanUpOldTablets();

    if (Settings.getValue("tabletVisibleToOthers")) {
        this.tabletEntityID = Entities.addEntity(tabletProperties, clientOnly);
        this.tabletIsOverlay = false;
    } else {
        this.tabletEntityID = Overlays.addOverlay("model", tabletProperties);
        this.tabletIsOverlay = true;
    }

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
        resolution: TABLET_TEXTURE_RESOLUTION,
        dpi: this.dpi,
        color: { red: 255, green: 255, blue: 255 },
        alpha: 1.0,
        parentID: this.tabletEntityID,
        parentJointIndex: -1,
        showKeyboardFocusHighlight: false,
        isAA: HMD.active
    });

    var HOME_BUTTON_Y_OFFSET = (this.height / 2) - 0.009;
    this.homeButtonID = Overlays.addOverlay("sphere", {
        name: "homeButton",
        localPosition: {x: -0.001, y: -HOME_BUTTON_Y_OFFSET, z: 0.0},
        localRotation: Quat.angleAxis(0, Y_AXIS),
        dimensions: { x: 4 * tabletScaleFactor, y: 4 * tabletScaleFactor, z: 4 * tabletScaleFactor},
        alpha: 0.0,
        visible: true,
        drawInFront: false,
        parentID: this.tabletEntityID,
        parentJointIndex: -1
    });

    this.receive = function (channel, senderID, senderUUID, localOnly) {
        if (_this.homeButtonID == senderID) {
            var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
            var onHomeScreen = tablet.onHomeScreen();
            if (onHomeScreen) {
                var isMessageOpen = tablet.isMessageDialogOpen();
                if (isMessageOpen === false) {
                    HMD.closeTablet();
                }
            } else {
                var isMessageOpen = tablet.isMessageDialogOpen();
                if (isMessageOpen === false) {
                    tablet.gotoHomeScreen();
                    _this.setHomeButtonTexture();
                }
            }
        }
    };

    this.state = "idle";

    this.getRoot = function() {
        return Entities.getWebViewRoot(_this.tabletEntityID);
    };

    this.getLocation = function() {
        if (this.tabletIsOverlay) {
            var location = Overlays.getProperty(this.tabletEntityID, "localPosition");
            var orientation = Overlays.getProperty(this.tabletEntityID, "localOrientation");
            return {
                localPosition: location,
                localRotation: orientation
            };
        } else {
            return Entities.getEntityProperties(_this.tabletEntityID, ["localPosition", "localRotation"]);
        }
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

    Controller.mousePressEvent.connect(this.myMousePressEvent);
    Controller.mouseMoveEvent.connect(this.myMouseMoveEvent);
    Controller.mouseReleaseEvent.connect(this.myMouseReleaseEvent);

    this.dragging = false;
    this.initialLocalIntersectionPoint = {x: 0, y: 0, z: 0};
    this.initialLocalPosition = {x: 0, y: 0, z: 0};

    this.myGeometryChanged = function (geometry) {
        _this.geometryChanged(geometry);
    };
    Window.geometryChanged.connect(this.myGeometryChanged);

    this.myCameraModeChanged = function(newMode) {
        _this.cameraModeChanged(newMode);
    };
    Camera.modeUpdated.connect(this.myCameraModeChanged);
};

WebTablet.prototype.setHomeButtonTexture = function() {
    Entities.editEntity(this.tabletEntityID, {textures: JSON.stringify({"tex.close": HOME_BUTTON_TEXTURE})});
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
    if (this.tabletIsOverlay) {
        Overlays.deleteOverlay(this.tabletEntityID);
    } else {
        Entities.deleteEntity(this.tabletEntityID);
    }
    Overlays.deleteOverlay(this.homeButtonID);
    HMD.displayModeChanged.disconnect(this.myOnHmdChanged);

    Controller.mousePressEvent.disconnect(this.myMousePressEvent);
    Controller.mouseMoveEvent.disconnect(this.myMouseMoveEvent);
    Controller.mouseReleaseEvent.disconnect(this.myMouseReleaseEvent);

    Window.geometryChanged.disconnect(this.myGeometryChanged);
    Camera.modeUpdated.disconnect(this.myCameraModeChanged);
};

WebTablet.prototype.geometryChanged = function (geometry) {
    if (!HMD.active) {
        var tabletProperties = {};

        // compute position, rotation & parentJointIndex of the tablet
        this.calculateTabletAttachmentProperties(NO_HANDS, false, tabletProperties);
        Entities.editEntity(this.tabletEntityID, tabletProperties);
    }
};

function gluPerspective(fovy, aspect, zNear, zFar) {
    var cotan = 1 / Math.tan(fovy / 2);
    var alpha = -(zFar + zNear) / (zFar - zNear);
    var beta = -(2 * zFar * zNear) / (zFar - zNear);
    var col0 = {x: cotan / aspect, y: 0, z: 0, w: 0};
    var col1 = {x: 0, y: cotan, z: 0, w: 0};
    var col2 = {x: 0, y: 0, z: alpha, w: -1};
    var col3 = {x: 0, y: 0, z: beta, w: 0};
    return Mat4.createFromColumns(col0, col1, col2, col3);
}

// calclulate the appropriate position of the tablet in world space, such that it fits in the center of the screen.
// with a bit of padding on the top and bottom.
// windowPos is used to position the center of the tablet at the given position.
WebTablet.prototype.calculateWorldAttitudeRelativeToCamera = function (windowPos) {

    var DEFAULT_DESKTOP_TABLET_SCALE = 75;
    var DESKTOP_TABLET_SCALE = Settings.getValue("desktopTabletScale") || DEFAULT_DESKTOP_TABLET_SCALE;

    // clamp window pos so 2d tablet is not off-screen.
    var TABLET_TEXEL_PADDING = {x: 60, y: 90};
    var X_CLAMP = (DESKTOP_TABLET_SCALE / 100) * ((TABLET_TEXTURE_RESOLUTION.x / 2) + TABLET_TEXEL_PADDING.x);
    var Y_CLAMP = (DESKTOP_TABLET_SCALE / 100) * ((TABLET_TEXTURE_RESOLUTION.y / 2) + TABLET_TEXEL_PADDING.y);
    windowPos.x = clamp(windowPos.x, X_CLAMP, Window.innerWidth - X_CLAMP);
    windowPos.y = clamp(windowPos.y, Y_CLAMP, Window.innerHeight - Y_CLAMP);

    var fov = (Settings.getValue('fieldOfView') || DEFAULT_VERTICAL_FIELD_OF_VIEW) * (Math.PI / 180);
    var MAX_PADDING_FACTOR = 2.2;
    var PADDING_FACTOR = Math.min(Window.innerHeight / TABLET_TEXTURE_RESOLUTION.y, MAX_PADDING_FACTOR);
    var TABLET_HEIGHT = (TABLET_TEXTURE_RESOLUTION.y / this.dpi) * INCHES_TO_METERS;
    var WEB_ENTITY_Z_OFFSET = (this.depth / 2);

    // calcualte distance from camera
    var dist = (PADDING_FACTOR * TABLET_HEIGHT) / (2 * Math.tan(fov / 2) * (DESKTOP_TABLET_SCALE / 100)) - WEB_ENTITY_Z_OFFSET;

    var Z_NEAR = 0.01;
    var Z_FAR = 100.0;

    // calculate mouse position in clip space
    var alpha = -(Z_FAR + Z_NEAR) / (Z_FAR - Z_NEAR);
    var beta = -(2 * Z_FAR * Z_NEAR) / (Z_FAR - Z_NEAR);
    var clipZ = (beta / dist) - alpha;
    var clipMousePosition = {x: (2 * windowPos.x / Window.innerWidth) - 1,
                             y: (2 * ((Window.innerHeight - windowPos.y) / Window.innerHeight)) - 1,
                             z: clipZ};

    // calculate projection matrix
    var aspect = Window.innerWidth / Window.innerHeight;
    var projMatrix = gluPerspective(fov, aspect, Z_NEAR, Z_FAR);

    // transform mouse clip position into view coordinates.
    var viewMousePosition = Mat4.transformPoint(Mat4.inverse(projMatrix), clipMousePosition);

    // transform view mouse position into world coordinates.
    var viewToWorldMatrix = Mat4.createFromRotAndTrans(Camera.orientation, Camera.position);
    var worldMousePosition = Mat4.transformPoint(viewToWorldMatrix, viewMousePosition);

    return {
        position: worldMousePosition,
        rotation: Quat.multiply(Camera.orientation, ROT_Y_180)
    };
};

// compute position, rotation & parentJointIndex of the tablet
WebTablet.prototype.calculateTabletAttachmentProperties = function (hand, useMouse, tabletProperties) {
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

        var windowPos;
        if (useMouse) {
            // compute the appropriate postion of the tablet such that it fits in the center of the screen nicely.
            windowPos = {x: Controller.getValue(Controller.Hardware.Keyboard.MouseX),
                         y: Controller.getValue(Controller.Hardware.Keyboard.MouseY)};
        } else {
            windowPos = {x: Window.innerWidth / 2,
                         y: Window.innerHeight / 2};
        }
        var attitude = this.calculateWorldAttitudeRelativeToCamera(windowPos);
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

    var tabletProperties = {};
    // compute position, rotation & parentJointIndex of the tablet
    this.calculateTabletAttachmentProperties(NO_HANDS, false, tabletProperties);
    Entities.editEntity(this.tabletEntityID, tabletProperties);

    // Full scene FXAA should be disabled on the overlay when the tablet in desktop mode.
    // This should make the text more readable.
    Overlays.editOverlay(this.webOverlayID, { isAA: HMD.active });
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
    children = children.concat(Entities.getChildrenIDsOfJoint(AVATAR_SELF_ID, jointIndex));
    children.forEach(function(childID) {
        var props = Entities.getEntityProperties(childID, ["name"]);
        if (props.name === "WebTablet Tablet") {
            Entities.deleteEntity(childID);
        }
    });
};

WebTablet.prototype.cleanUpOldTablets = function() {
    this.cleanUpOldTabletsOnJoint(-1);
    this.cleanUpOldTabletsOnJoint(SENSOR_TO_ROOM_MATRIX);
    this.cleanUpOldTabletsOnJoint(CAMERA_MATRIX);
    this.cleanUpOldTabletsOnJoint(65529);
    this.cleanUpOldTabletsOnJoint(65534);
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
    var entityPickResults;
    if (this.tabletIsOverlay) {
        entityPickResults = Overlays.findRayIntersection(pickRay, true, [this.tabletEntityID]);
    } else {
        entityPickResults = Entities.findRayIntersection(pickRay, true, [this.tabletEntityID]);
    }
    if (entityPickResults.intersects && (entityPickResults.entityID === this.tabletEntityID ||
                                         entityPickResults.overlayID === this.tabletEntityID)) {
        var overlayPickResults = Overlays.findRayIntersection(pickRay, true, [this.webOverlayID, this.homeButtonID], []);
        if (overlayPickResults.intersects && overlayPickResults.overlayID === this.homeButtonID) {
            var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
            var onHomeScreen = tablet.onHomeScreen();
            var isMessageOpen = tablet.isMessageDialogOpen();
            if (onHomeScreen) {
                if (isMessageOpen === false) {
                    HMD.closeTablet();
                }
            } else {
                if (isMessageOpen === false) {
                    tablet.gotoHomeScreen();
                    this.setHomeButtonTexture();
                }
            }
        } else if (!HMD.active && (!overlayPickResults.intersects || overlayPickResults.overlayID !== this.webOverlayID)) {
            this.dragging = true;
            var invCameraXform = new Xform(Camera.orientation, Camera.position).inv();
            this.initialLocalIntersectionPoint = invCameraXform.xformPoint(entityPickResults.intersection);
            if (this.tabletIsOverlay) {
                this.initialLocalPosition = Overlays.getProperty(this.tabletEntityID, "localPosition");
            } else {
                this.initialLocalPosition = Entities.getEntityProperties(this.tabletEntityID, ["localPosition"]).localPosition;
            }
        }
    }
};

WebTablet.prototype.cameraModeChanged = function (newMode) {
    // reposition the tablet.
    // This allows HMD.position to reflect the new camera mode.
    if (HMD.active) {
        var self = this;
        var tabletProperties = {};
        // compute position, rotation & parentJointIndex of the tablet
        self.calculateTabletAttachmentProperties(NO_HANDS, false, tabletProperties);
        Entities.editEntity(self.tabletEntityID, tabletProperties);
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
            if (this.tabletIsOverlay) {
                Overlays.editOverlay(this.tabletEntityID, {
                    localPosition: localPosition
                });
            } else {
                Entities.editEntity(this.tabletEntityID, {
                    localPosition: localPosition
                });
            }
        }
    }
};

WebTablet.prototype.mouseReleaseEvent = function (event) {
    this.dragging = false;
};
