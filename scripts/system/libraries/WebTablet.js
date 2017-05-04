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

var VEC3_ZERO = {x: 0, y: 0, z: 0};
var X_AXIS = {x: 1, y: 0, z: 0};
var Y_AXIS = {x: 0, y: 1, z: 0};
var DEFAULT_DPI = 34;
var DEFAULT_WIDTH = 0.4375;
var DEFAULT_VERTICAL_FIELD_OF_VIEW = 45; // degrees
var SENSOR_TO_ROOM_MATRIX = -2;
var CAMERA_MATRIX = -7;
var ROT_Y_180 = {x: 0, y: 1, z: 0, w: 0};
var ROT_IDENT = {x: 0, y: 0, z: 0, w: 1};
var TABLET_TEXTURE_RESOLUTION = { x: 480, y: 706 };
var INCHES_TO_METERS = 1 / 39.3701;
var AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

var NO_HANDS = -1;

// will need to be recaclulated if dimensions of fbx model change.
var TABLET_NATURAL_DIMENSIONS = {x: 33.797, y: 50.129, z: 2.269};

var HOME_BUTTON_TEXTURE = "http://hifi-content.s3.amazonaws.com/alan/dev/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-close.png";
// var HOME_BUTTON_TEXTURE = Script.resourcesPath() + "meshes/tablet-with-home-button.fbx/tablet-with-home-button.fbm/button-close.png";
// var TABLET_MODEL_PATH = "http://hifi-content.s3.amazonaws.com/alan/dev/tablet-with-home-button.fbx";
var LOCAL_TABLET_MODEL_PATH = Script.resourcesPath() + "meshes/tablet-with-home-button.fbx";

// returns object with two fields:
//    * position - position in front of the user
//    * rotation - rotation of entity so it faces the user.
function calcSpawnInfo(hand, height) {
    var finalPosition;

    var headPos = (HMD.active && Camera.mode === "first person") ? HMD.position : Camera.position;
    var headRot = (HMD.active && Camera.mode === "first person") ? HMD.orientation : Camera.orientation;

    if (!hand) {
        hand = NO_HANDS;
    }

    if (HMD.active && hand !== NO_HANDS) {
        var handController = getControllerWorldLocation(hand, true);

        var TABLET_UP_OFFSET = 0.1;
        var TABLET_FORWARD_OFFSET = 0.1;
        var normal = Vec3.multiplyQbyV(handController.rotation, {x: 0, y: -1, z: 0});
        var pitch = Math.asin(normal.y);
        var MAX_PITCH = Math.PI / 4;
        if (pitch < -MAX_PITCH) {
            pitch = -MAX_PITCH;
        } else if (pitch > MAX_PITCH) {
            pitch = MAX_PITCH;
        }

        // rebuild normal from pitch and heading.
        var heading = Math.atan2(normal.z, normal.x);
        normal = {x: Math.cos(heading), y: Math.sin(pitch), z: Math.sin(heading)};

        var position = Vec3.sum(handController.position, {x: 0, y: TABLET_UP_OFFSET, z: 0});
        var rotation = Quat.lookAt({x: 0, y: 0, z: 0}, normal, Y_AXIS);
        var offset = Vec3.multiplyQbyV(rotation, {x: 0, y: height / 2, z: TABLET_FORWARD_OFFSET});

        return {
            position: Vec3.sum(offset, position),
            rotation: rotation
        };
    } else {
        var forward = Quat.getForward(headRot);
        finalPosition = Vec3.sum(headPos, Vec3.multiply(0.6, forward));
        var orientation = Quat.lookAt({x: 0, y: 0, z: 0}, forward, {x: 0, y: 1, z: 0});
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
WebTablet = function (url, width, dpi, hand, clientOnly, location) {

    var _this = this;

    // scale factor of natural tablet dimensions.
    this.width = width || DEFAULT_WIDTH;
    var tabletScaleFactor = this.width / TABLET_NATURAL_DIMENSIONS.x;
    this.height = TABLET_NATURAL_DIMENSIONS.y * tabletScaleFactor;
    this.depth = TABLET_NATURAL_DIMENSIONS.z * tabletScaleFactor;
    this.landscape = false;

    if (dpi) {
        this.dpi = dpi;
    } else {
        this.dpi = DEFAULT_DPI * (DEFAULT_WIDTH / this.width);
    }

    var modelURL = LOCAL_TABLET_MODEL_PATH;
    var tabletProperties = {
        name: "WebTablet Tablet",
        type: "Model",
        modelURL: modelURL,
        url: modelURL, // for overlay
        grabbable: true, // for overlay
        loadPriority: 10.0, // for overlay
        userData: JSON.stringify({
            "grabbableKey": {"grabbable": true}
        }),
        dimensions: this.getDimensions(),
        parentID: AVATAR_SELF_ID
    };

    // compute position, rotation & parentJointIndex of the tablet
    this.calculateTabletAttachmentProperties(hand, true, tabletProperties);
    if (location) {
        tabletProperties.localPosition = location.localPosition;
        tabletProperties.localRotation = location.localRotation;
    }

    this.cleanUpOldTablets();

    this.tabletEntityID = Overlays.addOverlay("model", tabletProperties);

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
        resolution: this.getTabletTextureResolution(),
        dpi: this.dpi,
        color: { red: 255, green: 255, blue: 255 },
        alpha: 1.0,
        parentID: this.tabletEntityID,
        parentJointIndex: -1,
        showKeyboardFocusHighlight: false,
        isAA: HMD.active
    });

    var HOME_BUTTON_Y_OFFSET = (this.height / 2) - (this.height / 20);
    this.homeButtonID = Overlays.addOverlay("sphere", {
        name: "homeButton",
        localPosition: {x: -0.001, y: -HOME_BUTTON_Y_OFFSET, z: 0.0},
        localRotation: {x: 0, y: 1, z: 0, w: 0},
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
            var isMessageOpen;
            if (onHomeScreen) {
                isMessageOpen = tablet.isMessageDialogOpen();
                if (isMessageOpen === false) {
                    HMD.closeTablet();
                }
            } else {
                isMessageOpen = tablet.isMessageDialogOpen();
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

WebTablet.prototype.getDimensions = function() {
    if (this.landscape) {
        return { x: this.width * 2, y: this.height, z: this.depth };
    } else {
        return { x: this.width, y: this.height, z: this.depth };
    }
};

WebTablet.prototype.getTabletTextureResolution = function() {
    if (this.landscape) {
        return { x: TABLET_TEXTURE_RESOLUTION.x * 2, y: TABLET_TEXTURE_RESOLUTION.y };
    } else {
        return TABLET_TEXTURE_RESOLUTION;
    }
};

WebTablet.prototype.setLandscape = function(newLandscapeValue) {
    if (this.landscape == newLandscapeValue) {
        return;
    }
    this.landscape = newLandscapeValue;
    Overlays.editOverlay(this.tabletEntityID, { dimensions: this.getDimensions() });
    Overlays.editOverlay(this.webOverlayID, {
        resolution: this.getTabletTextureResolution()
    });
};

WebTablet.prototype.getLocation = function() {
    var location = Overlays.getProperty(this.tabletEntityID, "localPosition");
    var orientation = Overlays.getProperty(this.tabletEntityID, "localOrientation");
    return {
        localPosition: location,
        localRotation: orientation
    };
};

WebTablet.prototype.setHomeButtonTexture = function() {
    // TODO - is this still needed?
    // Entities.editEntity(this.tabletEntityID, {textures: JSON.stringify({"tex.close": HOME_BUTTON_TEXTURE})});
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

WebTablet.prototype.setWidth = function (width) {

    // scale factor of natural tablet dimensions.
    this.width = width || DEFAULT_WIDTH;
    var tabletScaleFactor = this.width / TABLET_NATURAL_DIMENSIONS.x;
    this.height = TABLET_NATURAL_DIMENSIONS.y * tabletScaleFactor;
    this.depth = TABLET_NATURAL_DIMENSIONS.z * tabletScaleFactor;
    this.dpi = DEFAULT_DPI * (DEFAULT_WIDTH / this.width);

    // update tablet model dimensions
    Overlays.editOverlay(this.tabletEntityID, { dimensions: this.getDimensions() });

    // update webOverlay
    var WEB_ENTITY_Z_OFFSET = (this.depth / 2);
    var WEB_ENTITY_Y_OFFSET = 0.004;
    Overlays.editOverlay(this.webOverlayID, {
        localPosition: { x: 0, y: WEB_ENTITY_Y_OFFSET, z: -WEB_ENTITY_Z_OFFSET },
        dpi: this.dpi
    });

    // update homeButton
    var HOME_BUTTON_Y_OFFSET = (this.height / 2) - (this.height / 20);
    Overlays.editOverlay(this.homeButtonID, {
        localPosition: {x: -0.001, y: -HOME_BUTTON_Y_OFFSET, z: 0.0},
        dimensions: { x: 4 * tabletScaleFactor, y: 4 * tabletScaleFactor, z: 4 * tabletScaleFactor}
    });
};

WebTablet.prototype.destroy = function () {
    Overlays.deleteOverlay(this.webOverlayID);
    Overlays.deleteOverlay(this.tabletEntityID);
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
        // TODO -- is this still needed?
        // Entities.editEntity(this.tabletEntityID, tabletProperties);
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
    var X_CLAMP = (DESKTOP_TABLET_SCALE / 100) * ((this.getTabletTextureResolution().x / 2) + TABLET_TEXEL_PADDING.x);
    var Y_CLAMP = (DESKTOP_TABLET_SCALE / 100) * ((this.getTabletTextureResolution().y / 2) + TABLET_TEXEL_PADDING.y);
    windowPos.x = clamp(windowPos.x, X_CLAMP, Window.innerWidth - X_CLAMP);
    windowPos.y = clamp(windowPos.y, Y_CLAMP, Window.innerHeight - Y_CLAMP);

    var fov = (Settings.getValue('fieldOfView') || DEFAULT_VERTICAL_FIELD_OF_VIEW) * (Math.PI / 180);
    var MAX_PADDING_FACTOR = 2.2;
    var PADDING_FACTOR = Math.min(Window.innerHeight / this.getTabletTextureResolution().y, MAX_PADDING_FACTOR);
    var TABLET_HEIGHT = (this.getTabletTextureResolution().y / this.dpi) * INCHES_TO_METERS;
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

    var tabletProperties = {};
    // compute position, rotation & parentJointIndex of the tablet
    this.calculateTabletAttachmentProperties(NO_HANDS, false, tabletProperties);
    // TODO -- is this still needed?
    // Entities.editEntity(this.tabletEntityID, tabletProperties);

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
    entityPickResults = Overlays.findRayIntersection(pickRay, true, [this.tabletEntityID]);
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
            this.initialLocalPosition = Overlays.getProperty(this.tabletEntityID, "localPosition");
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
        // TODO -- is this still needed?
        // Entities.editEntity(self.tabletEntityID, tabletProperties);
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
            Overlays.editOverlay(this.tabletEntityID, {
                localPosition: localPosition
            });
        }
    }
};

WebTablet.prototype.mouseReleaseEvent = function (event) {
    this.dragging = false;
};
