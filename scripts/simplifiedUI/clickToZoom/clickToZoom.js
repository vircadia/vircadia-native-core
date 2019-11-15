//
//  Created by Luis Cuenca on 11/14/19
//  Copyright 2019 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var ZoomStatus = {
        "zoomingIn" : 0,
        "zoomingOut" : 1,
        "zoomedIn" : 2,
        "zoomedOut" : 4,
        "consumed" : 5
    }

    var FocusType = {
        "avatar" : 0,
        "entity" : 1
    }

    var EasingFunctions = {
        easeInOutQuad: function (t) { return t<.5 ? 2*t*t : -1+(4-2*t)*t },
        // accelerating from zero velocity
        easeInCubic: function (t) { return t*t*t },
        // decelerating to zero velocity
        easeOutCubic: function (t) { return (--t)*t*t+1 },
        // acceleration until halfway, then deceleration
        easeInOutCubic: function (t) { return t<.5 ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2)+1 },
        // accelerating from zero velocity
        easeInQuart: function (t) { return t*t*t*t },
        // decelerating to zero velocity
        easeOutQuart: function (t) { return 1-(--t)*t*t*t },
        // acceleration until halfway, then deceleration
        easeInOutQuart: function (t) { return t<.5 ? 8*t*t*t*t : 1-8*(--t)*t*t*t },
        // accelerating from zero velocity
        easeInQuint: function (t) { return t*t*t*t*t },
        // decelerating to zero velocity
        easeOutQuint: function (t) { return 1+(--t)*t*t*t*t },
        // acceleration until halfway, then deceleration
        easeInOutQuint: function (t) { return t<.5 ? 16*t*t*t*t*t : 1+16*(--t)*t*t*t*t }
    };

    var ZoomData = function(type, lookAt, focusNormal, focusDimensions, velocity, maxDuration) {
        var self = this;

        this.focusType = type;
        this.lookAt = lookAt;
        this.focusDimensions = focusDimensions;
        this.focusNormal = focusNormal;
        this.velocity = velocity;
        this.maxDuration =  maxDuration;

        this.initialPos = Camera.getPosition();
        this.initialRot = Camera.getOrientation();
        this.interpolatedPos = this.initialPos;
        this.interpolatedRot = this.initialRot;
        this.initialMode = Camera.mode;
        this.initialOffset = Vec3.distance(self.initialPos, MyAvatar.getDefaultEyePosition());

        this.finalPos = Vec3.ZERO;
        this.finalRot = Quat.IDENTITY;
        this.direction = Vec3.ZERO;
        this.distance = Vec3.ZERO;
        this.totalTime = 0.0;
        this.elapsedTime = 0.0;
        this.maxZoomInAmount = 0.6;
        this.maxZoomOutAmount = 0.2;
        this.currentZoomAmount = 0.0;

        this.zoomPanOffset = {x: 0.5 * Window.innerWidth, y: 0.5 * Window.innerHeight};
        this.zoomPanDelta = {x: 0.0, y: 0.0};

        this.status = ZoomStatus.zoomedOut;

        var OWN_CAMERA_CHANGE_MAX_FRAMES = 30;
        this.ownCameraChangeElapseTime = 0.0;
        this.ownCameraChange = false;

        this.applyZoomPan = function() {
            self.zoomPanOffset.x += self.zoomPanDelta.x;
            self.zoomPanOffset.y += self.zoomPanDelta.y;
        }

        this.setZoomPanDelta = function(x, y) {
            self.zoomPanDelta.x = x;
            self.zoomPanDelta.y = y;
            var totalX = self.zoomPanOffset.x + self.zoomPanDelta.x;
            var totalY = self.zoomPanOffset.y + self.zoomPanDelta.y;
            totalX = Math.min(Math.max(totalX, 0.0), Window.innerWidth);
            totalY = Math.min(Math.max(totalY, 0.0), Window.innerHeight);
            self.zoomPanDelta.x = totalX - self.zoomPanOffset.x;
            self.zoomPanDelta.y = totalY - self.zoomPanOffset.y;
            self.updateSuperPan(totalX, totalY);
        }

        this.getFocusDistance = function(zoomDims) {
            var objAspect = zoomDims.x / zoomDims.y;
            var camAspect = Camera.frustum.aspectRatio;
            var m = 0.0;
            if (objAspect < camAspect) {
                m = zoomDims.y;
            } else {
                m = zoomDims.x / camAspect;
            }
            var DEGREE_TO_RADIAN = 0.0174533;
            var fov = DEGREE_TO_RADIAN * Camera.frustum.fieldOfView;
            return (0.5 * m) / Math.tan(0.5 * fov);
        }

        this.finalPos = Vec3.sum(this.lookAt, Vec3.multiply(this.getFocusDistance(this.focusDimensions), this.focusNormal));
        this.finalRot = Quat.lookAtSimple(this.finalPos, this.lookAt);

        this.computeRouteIn = function() {
            var railVector = Vec3.subtract(self.finalPos, self.initialPos);
            self.direction = Vec3.normalize(railVector);
            self.distance = Vec3.length(railVector);
            self.totalTime = self.distance / self.velocity;
            self.totalTime = self.totalTime > self.maxDuration ? self.maxDuration : self.totalTime;
        }

        this.computeRouteOut = function() {
            self.finalPos = Camera.getPosition();
            var camOffset = Vec3.ZERO;
            var myAvatarRotation = MyAvatar.orientation;
            if (self.initialMode.indexOf("first") != -1) {
                self.initialRot = myAvatarRotation;
            } else {
                var lookAtPoint = MyAvatar.getDefaultEyePosition();
                var lookFromSign = self.initialMode.indexOf("selfie") != -1 ? 1 : -1;
                var lookFromPoint = Vec3.sum(lookAtPoint, Vec3.multiply(self.initialOffset * lookFromSign, Quat.getFront(myAvatarRotation)));
                self.initialRot = Quat.lookAtSimple(lookFromPoint, lookAtPoint);
                self.initialPos = lookFromPoint;
            }
            self.computeRouteIn();
        }

        this.initZoomIn = function() {
            if (self.status === ZoomStatus.zoomedOut) {
                self.computeRouteIn();
                self.status = ZoomStatus.zoomingIn;
                self.changeCameraMode("independent");
                self.elapsedTime = 0.0;
            }
        }

        this.initZoomOut = function() {
            if (self.status === ZoomStatus.zoomedIn) {
                self.computeRouteOut();
                self.status = ZoomStatus.zoomingOut;
                self.changeCameraMode("independent");
                self.elapsedTime = 0.0;
            }
        }

        this.needsUpdate = function() {
            return self.status === ZoomStatus.zoomingIn || self.status === ZoomStatus.zoomingOut;
        }

        this.updateZoom = function(deltaTime) {
            if (self.ownCameraChange) {
                self.ownCameraChange = self.ownCameraChangeElapseTime < OWN_CAMERA_CHANGE_MAX_FRAMES * deltaTime;
                self.ownCameraChangeElapseTime += deltaTime;
            }
            if (!self.needsUpdate) {
                return;
            }
            if (self.elapsedTime < self.totalTime) {
                var ratio = EasingFunctions.easeInOutQuart(self.elapsedTime / self.totalTime);

                if (self.status === ZoomStatus.zoomingIn) {
                    var curDist = self.distance * ratio;
                    var addition = Vec3.multiply(curDist, self.direction);
                    self.interpolatedPos = Vec3.sum(self.initialPos, addition);
                    self.interpolatedRot = Quat.mix(self.initialRot, self.finalRot, ratio);
                } else if (self.status === ZoomStatus.zoomingOut) {
                    self.interpolatedPos = Vec3.sum(self.finalPos, Vec3.multiply(self.distance * ratio, Vec3.multiply(-1, self.direction)));
                    self.interpolatedRot = Quat.mix(self.finalRot, self.initialRot, ratio);
                }
                self.elapsedTime += deltaTime;
                Camera.setPosition(self.interpolatedPos);
                Camera.setOrientation(self.interpolatedRot);
            } else {
                Camera.setPosition(self.finalPos);
                Camera.setOrientation(self.finalRot);
                if (self.status === ZoomStatus.zoomingIn) {
                    self.status = ZoomStatus.zoomedIn;
                } else if (self.status === ZoomStatus.zoomingOut) {
                    self.status = ZoomStatus.consumed;
                    self.changeCameraMode(self.initialMode);
                }
            }
        }

        this.resetZoomAspect = function() {
            self.computeRouteIn();
            Camera.setPosition(self.finalPos);
        }

        this.updateSuperZoom = function(delta) {
            var ZOOM_STEP = 0.1;
            self.currentZoomAmount = self.currentZoomAmount + (delta < 0.0 ? -1 : 1) * ZOOM_STEP;
            self.currentZoomAmount = Math.min(Math.max(self.currentZoomAmount, -self.maxZoomOutAmount), self.maxZoomInAmount);
            self.updateSuperPan(self.zoomPanOffset.x, self.zoomPanOffset.y);
        }

        this.updateSuperPan = function(x, y) {
            var zoomOffset = Vec3.multiply(self.currentZoomAmount, Vec3.subtract(self.lookAt, self.finalPos));
            var xRatio = 0.5 - x / Window.innerWidth;
            var yRatio = 0.5 - y / Window.innerHeight;
            var cameraOrientation = Camera.getOrientation();
            var cameraY = Quat.getUp(cameraOrientation);
            var cameraX = Vec3.multiply(-1, Quat.getRight(cameraOrientation));
            var xOffset = Vec3.multiply(xRatio * self.focusDimensions.x, cameraX);
            var yOffset = Vec3.multiply(yRatio * self.focusDimensions.y, cameraY);
            zoomOffset = Vec3.sum(zoomOffset, xOffset);
            zoomOffset = Vec3.sum(zoomOffset, yOffset);
            Camera.setPosition(Vec3.sum(self.finalPos, zoomOffset));
        }

        this.abort = function() {
            self.changeCameraMode(self.initialMode);
        }

        this.changeCameraMode = function(mode) {
            self.ownCameraChange = true;
            self.ownCameraChangeElapseTime = 0.0;
            Camera.mode = mode;
        }
    }

    var ZoomOnAnything = function() {
        var self = this;
        this.zoomEntityID;
        this.zoomEntityData;
        this.zoomCameraPos;
        var ZOOM_MAX_VELOCITY = 15.0; // meters per second
        var ZOOM_MAX_DURATION = 1.0;
        this.zoomDelta = {x: 0.0, y: 0.0};
        this.isPanning = false;
        this.screenPointRef = {x: 0.0, y: 0.0};

        this.getEntityDimsFromNormal = function(dims, rot, normal) {
            var zoomXNormal = Vec3.multiplyQbyV(rot, Vec3.UNIT_X);
            var zoomYNormal = Vec3.multiplyQbyV(rot, Vec3.UNIT_Y);
            var zoomZNormal = Vec3.multiplyQbyV(rot, Vec3.UNIT_Z);
            var affinities = [
                {axis: "x", normal: zoomXNormal, affin: Math.abs(Vec3.dot(zoomXNormal, normal)), dims: {x: dims.z, y: dims.y}},
                {axis: "y", normal: zoomYNormal, affin: Math.abs(Vec3.dot(zoomYNormal, normal)), dims: {x: dims.x, y: dims.z}},
                {axis: "z", normal: zoomZNormal, affin: Math.abs(Vec3.dot(zoomZNormal, normal)), dims: {x: dims.x, y: dims.y}}
            ];
            affinities.sort(function(a, b) {
                return b.affin - a.affin;
            });
            return affinities[0];
        }

        this.getAvatarFocusPoint = function(avatar) {
            var rEyeIndex = avatar.getJointIndex("RightEye");
            var lEyeIndex = avatar.getJointIndex("LeftEye");
            var headIndex = avatar.getJointIndex("Head");
            var focusPoint = Vec3.ZERO;
            var validPoint = false;
            var count = 0;
            if (rEyeIndex != -1) {
                focusPoint = Vec3.sum(focusPoint, avatar.getJointPosition(rEyeIndex));
                validPoint = true;
                count++;
            }
            if (lEyeIndex != -1) {
                var leftEyePos = avatar.getJointPosition(lEyeIndex);
                var NORMAL_EYE_DISTANCE = 0.1;
                focusPoint = Vec3.sum(focusPoint, leftEyePos);
                validPoint = true;
                count++;
            }
            if (headIndex != -1) {
                focusPoint = Vec3.sum(focusPoint, avatar.getJointPosition(headIndex));
                validPoint = true;
                count++;
            }
            if (!validPoint) {
                focusPoint = avatar.getJointPosition("Hips");
                count++;
            }
            return Vec3.multiply(1.0/count, focusPoint);
        }

        this.getZoomDataFromAvatar = function(avatarID, skinToBoneDist, zoomVelocity, maxDuration) {
            var headDiam = 2.0 * skinToBoneDist;
            headDiam = headDiam < 0.5 ? 0.5 : headDiam;
            var avatar = AvatarList.getAvatar(avatarID);
            var focusPoint = self.getAvatarFocusPoint(avatar);
            var focusDims = {x: headDiam, y: headDiam};
            var focusNormal = Quat.getFront(avatar.orientation);
            var zoomData = new ZoomData(FocusType.avatar, focusPoint, focusNormal, focusDims, zoomVelocity, maxDuration);
            return zoomData;
        }

        this.getZoomDataFromEntity = function(intersection, objectProps, zoomVelocity, maxDuration) {
            var position = objectProps.position;
            var dimensions = objectProps.dimensions;
            var rotation = objectProps.rotation;
            var focusNormal = intersection.surfaceNormal;
            var dimsResult = self.getEntityDimsFromNormal(dimensions, rotation, focusNormal);
            var focusDims = dimsResult.dims;
            var focusDepth = Vec3.dot(Vec3.subtract(intersection.intersection, position), dimsResult.normal);
            var newPosition = Vec3.sum(position, Vec3.multiply(focusDepth, dimsResult.normal));
            var zoomData = new ZoomData(FocusType.entity, newPosition, focusNormal, focusDims, zoomVelocity, maxDuration);
            return zoomData;
        }

        this.zoomOnEntity = function(intersection, objectProps) {
            self.zoomEntityData = self.getZoomDataFromEntity(intersection, objectProps, ZOOM_MAX_VELOCITY, ZOOM_MAX_DURATION);
            self.zoomEntityData.initZoomIn();
        }

        this.zoomOnAvatar = function(avatarID, skinToBoneDist) {
            self.zoomEntityData = self.getZoomDataFromAvatar(avatarID, skinToBoneDist, ZOOM_MAX_VELOCITY, ZOOM_MAX_DURATION);
            self.zoomEntityData.initZoomIn();
        }

        this.updateZoom = function(deltaTime) {
            if (self.zoomEntityData && self.zoomEntityData.needsUpdate()) {
                self.zoomEntityData.updateZoom(deltaTime);
                if (self.zoomEntityData.status === ZoomStatus.consumed) {
                    self.zoomEntityData = undefined;
                }
            }
        }

        this.mouseDoublePressEvent = function(event) {
            if (event.isLeftButton) {
                if (!self.zoomEntityData) {
                    var pickRay = Camera.computePickRay(event.x, event.y);
                    var intersection = AvatarManager.findRayIntersection({origin: pickRay.origin, direction: pickRay.direction}, [], [MyAvatar.sessionUUID], false);
                    zoomingAtAvatarID = intersection.intersects ? intersection.avatarID : undefined;
                    if (!zoomingAtAvatarID) {
                        intersection = Entities.findRayIntersection({origin: pickRay.origin, direction: pickRay.direction}, true);
                        self.zoomEntityID = intersection.entityID;
                        var entityProps = Entities.getEntityProperties(intersection.entityID);
                        if (entityProps.type === "Shape") {
                            var FIND_SHAPES_DISTANCE = 10.0;
                            var shapes = Entities.findEntitiesByType("Shape", intersection.intersection, FIND_SHAPES_DISTANCE);
                            intersection = Entities.findRayIntersection({origin: pickRay.origin, direction: pickRay.direction}, true, [], shapes);
                            self.zoomEntityID = intersection.entityID;
                            entityProps = Entities.getEntityProperties(intersection.entityID);
                        }
                        if (!entityProps.dimensions) {
                            return;
                        }
                        self.zoomOnEntity(intersection, entityProps);
                    } else {
                        var avatar = AvatarList.getAvatar(zoomingAtAvatarID);
                        var skinToBoneDist = Vec3.distance(intersection.intersection, avatar.getJointPosition(intersection.jointIndex));
                        self.zoomOnAvatar(zoomingAtAvatarID, skinToBoneDist);
                    }
                } else if (!self.zoomEntityData.needsUpdate()){
                    self.zoomEntityData.initZoomOut();
                }
            }
        }

        this.mousePressEvent = function(event) {
            if (event.isRightButton) {
                self.isPanning = true;
                self.screenPointRef = {x: event.x, y: event.y};
            }
        }

        this.mouseReleaseEvent = function(event) {
            if (event.isRightButton) {
                if (self.zoomEntityData) {
                    self.zoomEntityData.applyZoomPan();
                    self.isPanning = false;
                    self.screenPointRef = {x: 0, y: 0};
                }
            }
        }

        this.mouseMoveEvent = function(event) {
            if (event.isRightButton) {
                if (self.isPanning && self.zoomEntityData) {
                    self.zoomEntityData.setZoomPanDelta(event.x - self.screenPointRef.x, event.y - self.screenPointRef.y);
                }
            }
        }

        this.mouseWheel = function(event) {
            if (self.zoomEntityData) {
                self.zoomEntityData.updateSuperZoom(event.delta);
            }
        }

        this.abort = function() {
            self.zoomEntityData.abort();
            self.zoomEntityData = undefined;
        }
    }

    var zoomOE = new ZoomOnAnything();

    Window.geometryChanged.connect(function() {
        if (zoomOE.zoomEntityData){
            zoomOE.zoomEntityData.resetZoomAspect();
        }
    });

    Camera.modeUpdated.connect(function(mode) {
        if (zoomOE.zoomEntityData && !zoomOE.zoomEntityData.ownCameraChange) {
            zoomOE.abort();
        }
    });

    Controller.mousePressEvent.connect(zoomOE.mousePressEvent);
    Controller.mouseDoublePressEvent.connect(zoomOE.mouseDoublePressEvent);
    Controller.mouseMoveEvent.connect(zoomOE.mouseMoveEvent);
    Controller.mouseReleaseEvent.connect(zoomOE.mouseReleaseEvent);
    Controller.wheelEvent.connect(zoomOE.mouseWheel);
    Script.update.connect(zoomOE.updateZoom);
    Script.scriptEnding.connect(function() {
        if (zoomOE.zoomEntityData) {
            zoomOE.abort();
        }
    });
})();