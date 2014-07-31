//
//  editModels.js
//  examples
//
//  Created by Clément Brisset on 4/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script allows you to edit models either with the razor hydras or with your mouse
//
//  If using the hydras :
//  grab grab models with the triggers, you can then move the models around or scale them with both hands.
//  You can switch mode using the bumpers so that you can move models roud more easily.
//
//  If using the mouse :
//  - left click lets you move the model in the plane facing you.
//    If pressing shift, it will move on the horizontale plane it's in.
//  - right click lets you rotate the model. z and x give you access to more axix of rotation while shift allows for finer control.
//  - left + right click lets you scale the model.
//  - you can press r while holding the model to reset its rotation
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("toolBars.js");

var windowDimensions = Controller.getViewportDimensions();
var toolIconUrl = "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/tools/";
var toolHeight = 50;
var toolWidth = 50;

var LASER_WIDTH = 4;
var LASER_COLOR = { red: 255, green: 0, blue: 0 };
var LASER_LENGTH_FACTOR = 500;

var MIN_ANGULAR_SIZE = 2;
var MAX_ANGULAR_SIZE = 45;

var LEFT = 0;
var RIGHT = 1;


var SPAWN_DISTANCE = 1;
var radiusDefault = 0.10;

var modelURLs = [
                 "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Feisar_Ship.FBX",
                 "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/birarda/birarda_head.fbx",
                 "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/pug.fbx",
                 "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/newInvader16x16-large-purple.svo",
                 "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/minotaur/mino_full.fbx",
                 "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Combat_tank_V01.FBX",
                 "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/orc.fbx",
                 "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/slimer.fbx",
                 ];

var toolBar;

var jointList = MyAvatar.getJointNames();

var mode = 0;

var exportMenu = null;

var ExportMenu = function(opts) {
    var self = this;

    var windowDimensions = Controller.getViewportDimensions();
    var pos = { x: windowDimensions.x / 2, y: windowDimensions.y - 100 };

    this._onClose = opts.onClose || function() {};
    this._position = { x: 0.0, y: 0.0, z: 0.0 };
    this._scale = 1.0;

    var minScale = 1;
    var maxScale = 32768;
    var titleWidth = 120;
    var locationWidth = 100;
    var scaleWidth = 144;
    var exportWidth = 100;
    var cancelWidth = 100;
    var margin = 4;
    var height = 30;
    var outerHeight = height + (2 * margin);
    var buttonColor = { red: 128, green: 128, blue: 128};

    var SCALE_MINUS = scaleWidth * 40.0 / 100.0;
    var SCALE_PLUS = scaleWidth * 63.0 / 100.0;

    var fullWidth = locationWidth + scaleWidth + exportWidth + cancelWidth + (2 * margin);
    var offset = fullWidth / 2;
    pos.x -= offset;

    var background= Overlays.addOverlay("text", {
        x: pos.x,
        y: pos.y,
        opacity: 1,
        width: fullWidth,
        height: outerHeight,
        backgroundColor: { red: 200, green: 200, blue: 200 },
        text: "",
    });

    var titleText = Overlays.addOverlay("text", {
        x: pos.x,
        y: pos.y - height,
        font: { size: 14 },
        width: titleWidth,
        height: height,
        backgroundColor: { red: 255, green: 255, blue: 255 },
        color: { red: 255, green: 255, blue: 255 },
        text: "Export Models"
    });

    var locationButton = Overlays.addOverlay("text", {
        x: pos.x + margin,
        y: pos.y + margin,
        width: locationWidth,
        height: height,
        color: { red: 255, green: 255, blue: 255 },
        text: "0, 0, 0",
    });
    var scaleOverlay = Overlays.addOverlay("image", {
        x: pos.x + margin + locationWidth,
        y: pos.y + margin,
        width: scaleWidth,
        height: height,
        subImage: { x: 0, y: 3, width: 144, height: height},
        imageURL: toolIconUrl + "voxel-size-selector.svg",
        alpha: 0.9,
    });
    var scaleViewWidth = 40;
    var scaleView = Overlays.addOverlay("text", {
        x: pos.x + margin + locationWidth + SCALE_MINUS,
        y: pos.y + margin,
        width: scaleViewWidth,
        height: height,
        alpha: 0.0,
        color: { red: 255, green: 255, blue: 255 },
        text: "1"
    });
    var exportButton = Overlays.addOverlay("text", {
        x: pos.x + margin + locationWidth + scaleWidth,
        y: pos.y + margin,
        width: exportWidth,
        height: height,
        color: { red: 0, green: 255, blue: 255 },
        text: "Export"
    });
    var cancelButton = Overlays.addOverlay("text", {
        x: pos.x + margin + locationWidth + scaleWidth + exportWidth,
        y: pos.y + margin,
        width: cancelWidth,
        height: height,
        color: { red: 255, green: 255, blue: 255 },
        text: "Cancel"
    });

    var voxelPreview = Overlays.addOverlay("cube", {
        position: { x: 0, y: 0, z: 0},
        size: this._scale,
        color: { red: 255, green: 255, blue: 0},
        alpha: 1,
        solid: false,
        visible: true,
        lineWidth: 4
    });

    this.parsePosition = function(str) {
        var parts = str.split(',');
        if (parts.length == 3) {
            var x = parseFloat(parts[0]);
            var y = parseFloat(parts[1]);
            var z = parseFloat(parts[2]);
            if (isFinite(x) && isFinite(y) && isFinite(z)) {
                return { x: x, y: y, z: z };
            }
        }
        return null;
    };

    this.showPositionPrompt = function() {
        var positionStr = self._position.x + ", " + self._position.y + ", " + self._position.z;
        while (1) {
            positionStr = Window.prompt("Position to export form:", positionStr);
            if (positionStr == null) {
                break;
            }
            var position = self.parsePosition(positionStr);
            if (position != null) {
                self.setPosition(position.x, position.y, position.z);
                break;
            }
            Window.alert("The position you entered was invalid.");
        }
    };

    this.setScale = function(scale) {
        self._scale = Math.min(maxScale, Math.max(minScale, scale));
        Overlays.editOverlay(scaleView, { text: self._scale });
        Overlays.editOverlay(voxelPreview, { size: self._scale });
    }

    this.decreaseScale = function() {
        self.setScale(self._scale /= 2);
    }

    this.increaseScale = function() {
        self.setScale(self._scale *= 2);
    }

    this.exportModels = function() {
        var x = self._position.x;
        var y = self._position.y;
        var z = self._position.z;
        var s = self._scale;
        var filename = "models__" + Window.location.hostname + "__" + x + "_" + y + "_" + z + "_" + s + "__.svo";
        filename = Window.save("Select where to save", filename, "*.svo")
        if (filename) {
            var success = Clipboard.exportModels(filename, x, y, z, s);
            if (!succcess) {
                WIndow.alert("Export failed: no models found in selected area.");
            }
        }
        self.close();
    };

    this.getPosition = function() {
        return self._position;
    };

    this.setPosition = function(x, y, z) {
        self._position = { x: x, y: y, z: z };
        var positionStr = x + ", " + y + ", " + z;
        Overlays.editOverlay(locationButton, { text: positionStr });
        Overlays.editOverlay(voxelPreview, { position: self._position });

    };

    this.mouseReleaseEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

        if (clickedOverlay == locationButton) {
            self.showPositionPrompt();
        } else if (clickedOverlay == exportButton) {
            self.exportModels();
        } else if (clickedOverlay == cancelButton) {
            self.close();
        } else if (clickedOverlay == scaleOverlay) {
            var x = event.x - pos.x - margin - locationWidth;
            print(x);
            if (x < SCALE_MINUS) {
                self.decreaseScale();
            } else if (x > SCALE_PLUS) {
                self.increaseScale();
            }
        }
    };

    this.close = function() {
        this.cleanup();
        this._onClose();
    };

    this.cleanup = function() {
        Overlays.deleteOverlay(background);
        Overlays.deleteOverlay(titleText);
        Overlays.deleteOverlay(locationButton);
        Overlays.deleteOverlay(exportButton);
        Overlays.deleteOverlay(cancelButton);
        Overlays.deleteOverlay(voxelPreview);
        Overlays.deleteOverlay(scaleOverlay);
        Overlays.deleteOverlay(scaleView);
    };

    print("CONNECTING!");
    Controller.mouseReleaseEvent.connect(this.mouseReleaseEvent);
};

var ModelImporter = function(opts) {
    var self = this;

    var height = 30;
    var margin = 4;
    var outerHeight = height + (2 * margin);
    var titleWidth = 120;
    var cancelWidth = 100;
    var fullWidth = titleWidth + cancelWidth + (2 * margin);

    // TODO: Show import preview
    var localModels = Overlays.addOverlay("localmodels", {
        position: { x: 1, y: 1, z: 1 },
        scale: 1,
        visible: false
    });
    var importScale = 1;
    var importBoundaries = Overlays.addOverlay("cube", {
                                           position: { x: 0, y: 0, z: 0 },
                                           size: 1,
                                           color: { red: 128, blue: 128, green: 128 },
                                           lineWidth: 4,
                                           solid: false,
                                           visible: false
                                           });

    var pos = { x: windowDimensions.x / 2 - (fullWidth / 2), y: windowDimensions.y - 100 };

    var background = Overlays.addOverlay("text", {
        x: pos.x,
        y: pos.y,
        opacity: 1,
        width: fullWidth,
        height: outerHeight,
        backgroundColor: { red: 200, green: 200, blue: 200 },
        visible: false,
        text: "",
    });

    var titleText = Overlays.addOverlay("text", {
        x: pos.x + margin,
        y: pos.y + margin,
        font: { size: 14 },
        width: titleWidth,
        height: height,
        backgroundColor: { red: 255, green: 255, blue: 255 },
        color: { red: 255, green: 255, blue: 255 },
        visible: false,
        text: "Import Models"
    });
    var cancelButton = Overlays.addOverlay("text", {
        x: pos.x + margin + titleWidth,
        y: pos.y + margin,
        width: cancelWidth,
        height: height,
        color: { red: 255, green: 255, blue: 255 },
        visible: false,
        text: "Close"
    });
    this._importing = false;

    this.setImportVisible = function(visible) {
        Overlays.editOverlay(importBoundaries, { visible: visible });
        Overlays.editOverlay(localModels, { visible: visible });
        Overlays.editOverlay(cancelButton, { visible: visible });
        Overlays.editOverlay(titleText, { visible: visible });
        Overlays.editOverlay(background, { visible: visible });
    };

    var importPosition = { x: 0, y: 0, z: 0 };
    this.moveImport = function(position) {
        importPosition = position;
        // TODO: Show import preview
        Overlays.editOverlay(localModels, {
                             position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
                             });
        Overlays.editOverlay(importBoundaries, {
                             position: { x: importPosition.x, y: importPosition.y, z: importPosition.z }
                             });
    }

    this.mouseMoveEvent = function(event) {
        if (self._importing) {
            var pickRay = Camera.computePickRay(event.x, event.y);
            var intersection = Voxels.findRayIntersection(pickRay);

            var distance = 2;// * self._scale;

            if (false) {//intersection.intersects) {
                var intersectionDistance = Vec3.length(Vec3.subtract(pickRay.origin, intersection.intersection));
                if (intersectionDistance < distance) {
                    distance = intersectionDistance * 0.99;
                }

            }

            var targetPosition = {
                x: pickRay.origin.x + (pickRay.direction.x * distance),
                y: pickRay.origin.y + (pickRay.direction.y * distance),
                z: pickRay.origin.z + (pickRay.direction.z * distance)
            };

            if (targetPosition.x < 0) targetPosition.x = 0;
            if (targetPosition.y < 0) targetPosition.y = 0;
            if (targetPosition.z < 0) targetPosition.z = 0;

            var nudgeFactor = 1;
            var newPosition = {
                x: Math.floor(targetPosition.x / nudgeFactor) * nudgeFactor,
                y: Math.floor(targetPosition.y / nudgeFactor) * nudgeFactor,
                z: Math.floor(targetPosition.z / nudgeFactor) * nudgeFactor
            }

            self.moveImport(newPosition);
        }
    }

    this.mouseReleaseEvent = function(event) {
        var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});

        if (clickedOverlay == cancelButton) {
            self._importing = false;
            self.setImportVisible(false);
        }
    };

    // Would prefer to use {4} for the coords, but it would only capture the last digit.
    var fileRegex = /__(.+)__(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)_(\d+(?:\.\d+)?)__/;
    this.doImport = function() {
        if (!self._importing) {
            var filename = Window.browse("Select models to import", "", "*.svo")
            if (filename) {
                parts = fileRegex.exec(filename);
                if (parts == null) {
                    Window.alert("The file you selected does not contain source domain or location information");
                } else {
                    var hostname = parts[1];
                    var x = parts[2];
                    var y = parts[3];
                    var z = parts[4];
                    var s = parts[5];
                    importScale = s;
                    if (hostname != location.hostname) {
                        if (!Window.confirm(("These models were not originally exported from this domain. Continue?"))) {
                            return;
                        }
                    } else {
                        if (Window.confirm(("Would you like to import back to the source location?"))) {
                            var success = Clipboard.importModels(filename);
                            if (success) {
                                Clipboard.pasteModels(x, y, z, 1);
                            } else {
                                Window.alert("There was an error importing the model file.");
                            }
                            return;
                        }
                    }
                }
                var success = Clipboard.importModels(filename);
                if (success) {
                    self._importing = true;
                    self.setImportVisible(true);
                    Overlays.editOverlay(importBoundaries, { size: s });
                } else {
                    Window.alert("There was an error importing the model file.");
                }
            }
        }
    }

    this.paste = function() {
        if (self._importing) {
            // self._importing = false;
            // self.setImportVisible(false);
            Clipboard.pasteModels(importPosition.x, importPosition.y, importPosition.z, 1);
        }
    }

    this.cleanup = function() {
        Overlays.deleteOverlay(localModels);
        Overlays.deleteOverlay(importBoundaries);
        Overlays.deleteOverlay(cancelButton);
        Overlays.deleteOverlay(titleText);
        Overlays.deleteOverlay(background);
    }

    Controller.mouseReleaseEvent.connect(this.mouseReleaseEvent);
    Controller.mouseMoveEvent.connect(this.mouseMoveEvent);
};

var modelImporter = new ModelImporter();

function isLocked(properties) {
    // special case to lock the ground plane model in hq.
    if (location.hostname == "hq.highfidelity.io" && 
        properties.modelURL == "https://s3-us-west-1.amazonaws.com/highfidelity-public/ozan/Terrain_Reduce_forAlpha.fbx") {
        return true;
    }
    return false;
}


function controller(wichSide) {
    this.side = wichSide;
    this.palm = 2 * wichSide;
    this.tip = 2 * wichSide + 1;
    this.trigger = wichSide;
    this.bumper = 6 * wichSide + 5;
    
    this.oldPalmPosition = Controller.getSpatialControlPosition(this.palm);
    this.palmPosition = Controller.getSpatialControlPosition(this.palm);
    
    this.oldTipPosition = Controller.getSpatialControlPosition(this.tip);
    this.tipPosition = Controller.getSpatialControlPosition(this.tip);
    
    this.oldUp = Controller.getSpatialControlNormal(this.palm);
    this.up = this.oldUp;
    
    this.oldFront = Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition));
    this.front = this.oldFront;
    
    this.oldRight = Vec3.cross(this.front, this.up);
    this.right = this.oldRight;
    
    this.oldRotation = Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(this.palm));
    this.rotation = this.oldRotation;
    
    this.triggerValue = Controller.getTriggerValue(this.trigger);
    this.bumperValue = Controller.isButtonPressed(this.bumper);
    
    this.pressed = false; // is trigger pressed
    this.pressing = false; // is trigger being pressed (is pressed now but wasn't previously)
    
    this.grabbing = false;
    this.modelID = { isKnownID: false };
    this.modelURL = "";
    this.oldModelRotation;
    this.oldModelPosition;
    this.oldModelRadius;
    
    this.positionAtGrab;
    this.rotationAtGrab;
    this.modelPositionAtGrab;
    this.modelRotationAtGrab;
    
    this.jointsIntersectingFromStart = [];
    
    this.laser = Overlays.addOverlay("line3d", {
                                     position: { x: 0, y: 0, z: 0 },
                                     end: { x: 0, y: 0, z: 0 },
                                     color: LASER_COLOR,
                                     alpha: 1,
                                     visible: false,
                                     lineWidth: LASER_WIDTH,
                                     anchor: "MyAvatar"
                                     });
    
    this.guideScale = 0.02;
    this.ball = Overlays.addOverlay("sphere", {
                                    position: { x: 0, y: 0, z: 0 },
                                    size: this.guideScale,
                                    solid: true,
                                    color: { red: 0, green: 255, blue: 0 },
                                    alpha: 1,
                                    visible: false,
                                    anchor: "MyAvatar"
                                    });
    this.leftRight = Overlays.addOverlay("line3d", {
                                         position: { x: 0, y: 0, z: 0 },
                                         end: { x: 0, y: 0, z: 0 },
                                         color: { red: 0, green: 0, blue: 255 },
                                         alpha: 1,
                                         visible: false,
                                         lineWidth: LASER_WIDTH,
                                         anchor: "MyAvatar"
                                         });
    this.topDown = Overlays.addOverlay("line3d", {
                                       position: { x: 0, y: 0, z: 0 },
                                       end: { x: 0, y: 0, z: 0 },
                                       color: { red: 0, green: 0, blue: 255 },
                                       alpha: 1,
                                       visible: false,
                                       lineWidth: LASER_WIDTH,
                                       anchor: "MyAvatar"
                                       });
    

    
    this.grab = function (modelID, properties) {
        if (isLocked(properties)) {
            print("Model locked " + modelID.id);
        } else {
            print("Grabbing " + modelID.id);
        
            this.grabbing = true;
            this.modelID = modelID;
            this.modelURL = properties.modelURL;
        
            this.oldModelPosition = properties.position;
            this.oldModelRotation = properties.modelRotation;
            this.oldModelRadius = properties.radius;
            
            this.positionAtGrab = this.palmPosition;
            this.rotationAtGrab = this.rotation;
            this.modelPositionAtGrab = properties.position;
            this.modelRotationAtGrab = properties.modelRotation;
            
            this.jointsIntersectingFromStart = [];
            for (var i = 0; i < jointList.length; i++) {
                var distance = Vec3.distance(MyAvatar.getJointPosition(jointList[i]), this.oldModelPosition);
                if (distance < this.oldModelRadius) {
                    this.jointsIntersectingFromStart.push(i);
                }
            }
            this.showLaser(false);
        }
    }
    
    this.release = function () {
        if (this.grabbing) {
            jointList = MyAvatar.getJointNames();
            
            var closestJointIndex = -1;
            var closestJointDistance = 10;
            for (var i = 0; i < jointList.length; i++) {
                var distance = Vec3.distance(MyAvatar.getJointPosition(jointList[i]), this.oldModelPosition);
                if (distance < closestJointDistance) {
                    closestJointDistance = distance;
                    closestJointIndex = i;
                }
            }
            
            if (closestJointIndex != -1) {
                print("closestJoint: " + jointList[closestJointIndex]);
                print("closestJointDistance (attach max distance): " + closestJointDistance + " (" + this.oldModelRadius + ")");
            }
            
            if (closestJointDistance < this.oldModelRadius) {
                
                if (this.jointsIntersectingFromStart.indexOf(closestJointIndex) != -1 ||
                    (leftController.grabbing && rightController.grabbing &&
                    leftController.modelID.id == rightController.modelID.id)) {
                    // Do nothing
                } else {
                    print("Attaching to " + jointList[closestJointIndex]);
                    var jointPosition = MyAvatar.getJointPosition(jointList[closestJointIndex]);
                    var jointRotation = MyAvatar.getJointCombinedRotation(jointList[closestJointIndex]);
                    
                    var attachmentOffset = Vec3.subtract(this.oldModelPosition, jointPosition);
                    attachmentOffset = Vec3.multiplyQbyV(Quat.inverse(jointRotation), attachmentOffset);
                    var attachmentRotation = Quat.multiply(Quat.inverse(jointRotation), this.oldModelRotation);
                    
                    MyAvatar.attach(this.modelURL, jointList[closestJointIndex],
                                    attachmentOffset, attachmentRotation, 2.0 * this.oldModelRadius,
                                    true, false);
                    
                    Models.deleteModel(this.modelID);
                }
            }
        }
        
        this.grabbing = false;
        this.modelID.isKnownID = false;
        this.jointsIntersectingFromStart = [];
        this.showLaser(true);
    }
    
    this.checkTrigger = function () {
        if (this.triggerValue > 0.9) {
            if (this.pressed) {
                this.pressing = false;
            } else {
                this.pressing = true;
            }
            this.pressed = true;
        } else {
            this.pressing = false;
            this.pressed = false;
        }
    }

    this.checkModel = function (properties) {
        // special case to lock the ground plane model in hq.
        if (isLocked(properties)) {
            return { valid: false };
        }
   
    
        //                P         P - Model
        //               /|         A - Palm
        //              / | d       B - unit vector toward tip
        //             /  |         X - base of the perpendicular line
        //            A---X----->B  d - distance fom axis
        //              x           x - distance from A
        //
        //            |X-A| = (P-A).B
        //            X == A + ((P-A).B)B
        //            d = |P-X|
        
        var A = this.palmPosition;
        var B = this.front;
        var P = properties.position;
        
        var x = Vec3.dot(Vec3.subtract(P, A), B);
        var y = Vec3.dot(Vec3.subtract(P, A), this.up);
        var z = Vec3.dot(Vec3.subtract(P, A), this.right);
        var X = Vec3.sum(A, Vec3.multiply(B, x));
        var d = Vec3.length(Vec3.subtract(P, X));
        
        var angularSize = 2 * Math.atan(properties.radius / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14;
        if (0 < x && angularSize > MIN_ANGULAR_SIZE) {
            if (angularSize > MAX_ANGULAR_SIZE) {
                print("Angular size too big: " + 2 * Math.atan(properties.radius / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14);
                return { valid: false };
            }
            
            return { valid: true, x: x, y: y, z: z };
        }
        return { valid: false };
    }
    
    this.glowedIntersectingModel = { isKnownID: false };
    this.moveLaser = function () {
        // the overlays here are anchored to the avatar, which means they are specified in the avatar's local frame
       
        var inverseRotation = Quat.inverse(MyAvatar.orientation);
        var startPosition = Vec3.multiplyQbyV(inverseRotation, Vec3.subtract(this.palmPosition, MyAvatar.position));
        var direction = Vec3.multiplyQbyV(inverseRotation, Vec3.subtract(this.tipPosition, this.palmPosition));
        var distance = Vec3.length(direction);
        direction = Vec3.multiply(direction, LASER_LENGTH_FACTOR / distance);
        var endPosition = Vec3.sum(startPosition, direction);
        
        Overlays.editOverlay(this.laser, {
                             position: startPosition,
                             end: endPosition
                             });
        
        
        Overlays.editOverlay(this.ball, {
                             position: endPosition
                             });
        Overlays.editOverlay(this.leftRight, {
                             position: Vec3.sum(endPosition, Vec3.multiply(this.right, 2 * this.guideScale)),
                             end: Vec3.sum(endPosition, Vec3.multiply(this.right, -2 * this.guideScale))
                             });
        Overlays.editOverlay(this.topDown, {position: Vec3.sum(endPosition, Vec3.multiply(this.up, 2 * this.guideScale)),
                             end: Vec3.sum(endPosition, Vec3.multiply(this.up, -2 * this.guideScale))
                             });
        this.showLaser(!this.grabbing || mode == 0);
        
        if (this.glowedIntersectingModel.isKnownID) {
            Models.editModel(this.glowedIntersectingModel, { glowLevel: 0.0 });
            this.glowedIntersectingModel.isKnownID = false;
        }
        if (!this.grabbing) {
            var intersection = Models.findRayIntersection({
                                                          origin: this.palmPosition,
                                                          direction: this.front
                                                          });
            var angularSize = 2 * Math.atan(intersection.modelProperties.radius / Vec3.distance(Camera.getPosition(), intersection.modelProperties.position)) * 180 / 3.14;
            if (intersection.accurate && intersection.modelID.isKnownID && angularSize > MIN_ANGULAR_SIZE && angularSize < MAX_ANGULAR_SIZE) {
                this.glowedIntersectingModel = intersection.modelID;
                Models.editModel(this.glowedIntersectingModel, { glowLevel: 0.25 });
            }
        }
    }
    
    this.showLaser = function(show) {
        Overlays.editOverlay(this.laser, { visible: show });
        Overlays.editOverlay(this.ball, { visible: show });
        Overlays.editOverlay(this.leftRight, { visible: show });
        Overlays.editOverlay(this.topDown, { visible: show });
    }
    
    this.moveModel = function () {
        if (this.grabbing) {
            if (!this.modelID.isKnownID) {
                print("Unknown grabbed ID " + this.modelID.id + ", isKnown: " + this.modelID.isKnownID);
                this.modelID =  Models.findRayIntersection({
                                                        origin: this.palmPosition,
                                                        direction: this.front
                                                           }).modelID;
                print("Identified ID " + this.modelID.id + ", isKnown: " + this.modelID.isKnownID);
            }
            var newPosition;
            var newRotation;
            
            switch (mode) {
                case 0:
                    newPosition = Vec3.sum(this.palmPosition,
                                           Vec3.multiply(this.front, this.x));
                    newPosition = Vec3.sum(newPosition,
                                           Vec3.multiply(this.up, this.y));
                    newPosition = Vec3.sum(newPosition,
                                           Vec3.multiply(this.right, this.z));
                    
                    
                    newRotation = Quat.multiply(this.rotation,
                                                Quat.inverse(this.oldRotation));
                    newRotation = Quat.multiply(newRotation,
                                                this.oldModelRotation);
                    break;
                case 1:
                    var forward = Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -1 });
                    var d = Vec3.dot(forward, MyAvatar.position);
                    
                    var factor1 = Vec3.dot(forward, this.positionAtGrab) - d;
                    var factor2 = Vec3.dot(forward, this.modelPositionAtGrab) - d;
                    var vector = Vec3.subtract(this.palmPosition, this.positionAtGrab);
                    
                    if (factor2 < 0) {
                        factor2 = 0;
                    }
                    if (factor1 <= 0) {
                        factor1 = 1;
                        factor2 = 1;
                    }
                    
                    newPosition = Vec3.sum(this.modelPositionAtGrab,
                                           Vec3.multiply(vector,
                                                         factor2 / factor1));
                    
                    newRotation = Quat.multiply(this.rotation,
                                                Quat.inverse(this.rotationAtGrab));
                    newRotation = Quat.multiply(newRotation,
                                                this.modelRotationAtGrab);
                    break;
            }
            
            Models.editModel(this.modelID, {
                             position: newPosition,
                             modelRotation: newRotation
                             });
            
            this.oldModelRotation = newRotation;
            this.oldModelPosition = newPosition;
            
            var indicesToRemove = [];
            for (var i = 0; i < this.jointsIntersectingFromStart.length; ++i) {
                var distance = Vec3.distance(MyAvatar.getJointPosition(this.jointsIntersectingFromStart[i]), this.oldModelPosition);
                if (distance >= this.oldModelRadius) {
                    indicesToRemove.push(this.jointsIntersectingFromStart[i]);
                }

            }
            for (var i = 0; i < indicesToRemove.length; ++i) {
                this.jointsIntersectingFromStart.splice(this.jointsIntersectingFromStart.indexOf(indicesToRemove[i], 1));
            }
        }
    }
    
    this.update = function () {
        this.oldPalmPosition = this.palmPosition;
        this.oldTipPosition = this.tipPosition;
        this.palmPosition = Controller.getSpatialControlPosition(this.palm);
        this.tipPosition = Controller.getSpatialControlPosition(this.tip);
        
        this.oldUp = this.up;
        this.up = Vec3.normalize(Controller.getSpatialControlNormal(this.palm));
        
        this.oldFront = this.front;
        this.front = Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition));
        
        this.oldRight = this.right;
        this.right = Vec3.normalize(Vec3.cross(this.front, this.up));
        
        this.oldRotation = this.rotation;
        this.rotation = Quat.multiply(MyAvatar.orientation, Controller.getSpatialControlRawRotation(this.palm));
        
        this.triggerValue = Controller.getTriggerValue(this.trigger);
        
        var bumperValue = Controller.isButtonPressed(this.bumper);
        if (bumperValue && !this.bumperValue) {
            if (mode == 0) {
                mode = 1;
                Overlays.editOverlay(leftController.laser, { color: { red: 0, green: 0, blue: 255 } });
                Overlays.editOverlay(rightController.laser, { color: { red: 0, green: 0, blue: 255 } });
            } else {
                mode = 0;
                Overlays.editOverlay(leftController.laser, { color: { red: 255, green: 0, blue: 0 } });
                Overlays.editOverlay(rightController.laser, { color: { red: 255, green: 0, blue: 0 } });
            }
        }
        this.bumperValue = bumperValue;
        
        
        this.checkTrigger();
        
        this.moveLaser();
        
        if (!this.pressed && this.grabbing) {
            // release if trigger not pressed anymore.
            this.release();
        }
        
        if (this.pressing) {
            // Checking for attachments intersecting
            var attachments = MyAvatar.getAttachmentData();
            var attachmentIndex = -1;
            var attachmentX = LASER_LENGTH_FACTOR;
            
            var newModel;
            var newProperties;
            
            for (var i = 0; i < attachments.length; ++i) {
                var position = Vec3.sum(MyAvatar.getJointPosition(attachments[i].jointName),
                                        Vec3.multiplyQbyV(MyAvatar.getJointCombinedRotation(attachments[i].jointName), attachments[i].translation));
                var scale = attachments[i].scale;
                
                var A = this.palmPosition;
                var B = this.front;
                var P = position;
                
                var x = Vec3.dot(Vec3.subtract(P, A), B);
                var X = Vec3.sum(A, Vec3.multiply(B, x));
                var d = Vec3.length(Vec3.subtract(P, X));
                
                if (d < scale / 2.0 && 0 < x && x < attachmentX) {
                    attachmentIndex = i;
                    attachmentX = d;
                }
            }
            
            if (attachmentIndex != -1) {
                print("Detaching: " + attachments[attachmentIndex].modelURL);
                MyAvatar.detachOne(attachments[attachmentIndex].modelURL, attachments[attachmentIndex].jointName);
                
                newProperties = {
                position: Vec3.sum(MyAvatar.getJointPosition(attachments[attachmentIndex].jointName),
                                   Vec3.multiplyQbyV(MyAvatar.getJointCombinedRotation(attachments[attachmentIndex].jointName), attachments[attachmentIndex].translation)),
                modelRotation: Quat.multiply(MyAvatar.getJointCombinedRotation(attachments[attachmentIndex].jointName),
                                             attachments[attachmentIndex].rotation),
                radius: attachments[attachmentIndex].scale / 2.0,
                modelURL: attachments[attachmentIndex].modelURL
                };
                newModel = Models.addModel(newProperties);
            } else {
                // There is none so ...
                // Checking model tree
                Vec3.print("Looking at: ", this.palmPosition);
                var pickRay = { origin: this.palmPosition,
                    direction: Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition)) };
                var foundIntersection = Models.findRayIntersection(pickRay);
                
                if(!foundIntersection.accurate) {
                    print("No accurate intersection");
                    return;
                }
                newModel = foundIntersection.modelID;
                
                if (!newModel.isKnownID) {
                    var identify = Models.identifyModel(newModel);
                    if (!identify.isKnownID) {
                        print("Unknown ID " + identify.id + " (update loop " + newModel.id + ")");
                        return;
                    }
                    newModel = identify;
                }
                newProperties = Models.getModelProperties(newModel);
            }
            
            
            print("foundModel.modelURL=" + newProperties.modelURL);
            
            if (isLocked(newProperties)) {
                print("Model locked " + newProperties.id);
            } else {
                var check = this.checkModel(newProperties);
                if (!check.valid) {
                    return;
                }
                
                this.grab(newModel, newProperties);
                
                this.x = check.x;
                this.y = check.y;
                this.z = check.z;
                return;
            }
        }
    }

    this.cleanup = function () {
        Overlays.deleteOverlay(this.laser);
        Overlays.deleteOverlay(this.ball);
        Overlays.deleteOverlay(this.leftRight);
        Overlays.deleteOverlay(this.topDown);
    }
}

var leftController = new controller(LEFT);
var rightController = new controller(RIGHT);

function moveModels() {
    if (leftController.grabbing && rightController.grabbing && rightController.modelID.id == leftController.modelID.id) {
        var newPosition = leftController.oldModelPosition;
        var rotation = leftController.oldModelRotation;
        var ratio = 1;
        
        
        switch (mode) {
            case 0:
                var oldLeftPoint = Vec3.sum(leftController.oldPalmPosition, Vec3.multiply(leftController.oldFront, leftController.x));
                var oldRightPoint = Vec3.sum(rightController.oldPalmPosition, Vec3.multiply(rightController.oldFront, rightController.x));
                
                var oldMiddle = Vec3.multiply(Vec3.sum(oldLeftPoint, oldRightPoint), 0.5);
                var oldLength = Vec3.length(Vec3.subtract(oldLeftPoint, oldRightPoint));
                
                
                var leftPoint = Vec3.sum(leftController.palmPosition, Vec3.multiply(leftController.front, leftController.x));
                var rightPoint = Vec3.sum(rightController.palmPosition, Vec3.multiply(rightController.front, rightController.x));
                
                var middle = Vec3.multiply(Vec3.sum(leftPoint, rightPoint), 0.5);
                var length = Vec3.length(Vec3.subtract(leftPoint, rightPoint));
                
                
                ratio = length / oldLength;
                newPosition = Vec3.sum(middle,
                                       Vec3.multiply(Vec3.subtract(leftController.oldModelPosition, oldMiddle), ratio));
                 break;
            case 1:
                var u = Vec3.normalize(Vec3.subtract(rightController.oldPalmPosition, leftController.oldPalmPosition));
                var v = Vec3.normalize(Vec3.subtract(rightController.palmPosition, leftController.palmPosition));
                
                var cos_theta = Vec3.dot(u, v);
                if (cos_theta > 1) {
                    cos_theta = 1;
                }
                var angle = Math.acos(cos_theta) / Math.PI * 180;
                if (angle < 0.1) {
                    return;
                    
                }
                var w = Vec3.normalize(Vec3.cross(u, v));
                
                rotation = Quat.multiply(Quat.angleAxis(angle, w), leftController.oldModelRotation);
                
                
                leftController.positionAtGrab = leftController.palmPosition;
                leftController.rotationAtGrab = leftController.rotation;
                leftController.modelPositionAtGrab = leftController.oldModelPosition;
                leftController.modelRotationAtGrab = rotation;
                
                rightController.positionAtGrab = rightController.palmPosition;
                rightController.rotationAtGrab = rightController.rotation;
                rightController.modelPositionAtGrab = rightController.oldModelPosition;
                rightController.modelRotationAtGrab = rotation;
                break;
        }
        
        Models.editModel(leftController.modelID, {
                         position: newPosition,
                         modelRotation: rotation,
                         radius: leftController.oldModelRadius * ratio
                         });
        
        leftController.oldModelPosition = newPosition;
        leftController.oldModelRotation = rotation;
        leftController.oldModelRadius *= ratio;
        
        rightController.oldModelPosition = newPosition;
        rightController.oldModelRotation = rotation;
        rightController.oldModelRadius *= ratio;
        return;
    }
    
    leftController.moveModel();
    rightController.moveModel();
}

var hydraConnected = false;
function checkController(deltaTime) {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;
    
    // this is expected for hydras
    if (numberOfButtons==12 && numberOfTriggers == 2 && controllersPerTrigger == 2) {
        if (!hydraConnected) {
            hydraConnected = true;
        }
        
        leftController.update();
        rightController.update();
        moveModels();
    } else {
        if (hydraConnected) {
            hydraConnected = false;
            
            leftController.showLaser(false);
            rightController.showLaser(false);
        }
    }
    
    moveOverlays();
}
var newModel;
var browser;
function initToolBar() {
    toolBar = new ToolBar(0, 0, ToolBar.VERTICAL);
    // New Model
    newModel = toolBar.addTool({
                               imageURL: toolIconUrl + "add-model-tool.svg",
                               subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                               width: toolWidth, height: toolHeight,
                               visible: true,
                               alpha: 0.9
                               });
    browser = toolBar.addTool({
                               imageURL: toolIconUrl + "list-icon.png",
                               width: toolWidth, height: toolHeight,
                               visible: true,
                               alpha: 0.7
                               });
}

function moveOverlays() {
    var newViewPort = Controller.getViewportDimensions();
    
    if (typeof(toolBar) === 'undefined') {
        initToolBar();
        
    } else if (windowDimensions.x == newViewPort.x &&
               windowDimensions.y == newViewPort.y) {
        return;
    }
    
    
    windowDimensions = newViewPort;
    var toolsX = windowDimensions.x - 8 - toolBar.width;
    var toolsY = (windowDimensions.y - toolBar.height) / 2;
    
    toolBar.move(toolsX, toolsY);
}



var modelSelected = false;
var selectedModelID;
var selectedModelProperties;
var mouseLastPosition;
var orientation;
var intersection;


var SCALE_FACTOR = 200.0;

function rayPlaneIntersection(pickRay, point, normal) {
    var d = -Vec3.dot(point, normal);
    var t = -(Vec3.dot(pickRay.origin, normal) + d) / Vec3.dot(pickRay.direction, normal);
    
    return Vec3.sum(pickRay.origin, Vec3.multiply(pickRay.direction, t));
}

function Tooltip() {
    this.x = 285;
    this.y = 115;
    this.width = 500;
    this.height = 145 ;
    this.margin = 5;
    this.decimals = 3;
    
    this.textOverlay = Overlays.addOverlay("text", {
                                           x: this.x,
                                           y: this.y,
                                           width: this.width,
                                           height: this.height,
                                           margin: this.margin,
                                           text: "",
                                           color: { red: 128, green: 128, blue: 128 },
                                           alpha: 0.2,
                                           visible: false
                                           });
    this.show = function(doShow) {
        Overlays.editOverlay(this.textOverlay, { visible: doShow });
    }
    this.updateText = function(properties) {
        var angles = Quat.safeEulerAngles(properties.modelRotation);
        var text = "Model Properties:\n"
        text += "x: " + properties.position.x.toFixed(this.decimals) + "\n"
        text += "y: " + properties.position.y.toFixed(this.decimals) + "\n"
        text += "z: " + properties.position.z.toFixed(this.decimals) + "\n"
        text += "pitch: " + angles.x.toFixed(this.decimals) + "\n"
        text += "yaw:  " + angles.y.toFixed(this.decimals) + "\n"
        text += "roll:    " + angles.z.toFixed(this.decimals) + "\n"
        text += "Scale: " + 2 * properties.radius.toFixed(this.decimals) + "\n"
        text += "ID: " + properties.id + "\n"
        text += "model url: " + properties.modelURL + "\n"
        text += "animation url: " + properties.animationURL + "\n"
        if (properties.sittingPoints.length > 0) {
            text += properties.sittingPoints.length + " sitting points: "
            for (var i = 0; i < properties.sittingPoints.length; ++i) {
                text += properties.sittingPoints[i].name + " "
            }
        } else {
            text += "No sitting points"
        }


        Overlays.editOverlay(this.textOverlay, { text: text });
    }
    
    this.cleanup = function() {
        Overlays.deleteOverlay(this.textOverlay);
    }
}
var tooltip = new Tooltip();

function mousePressEvent(event) {
    if (event.isAlt) {
        return;
    }
    
    mouseLastPosition = { x: event.x, y: event.y };
    modelSelected = false;
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    
    if (newModel == toolBar.clicked(clickedOverlay)) {
        var url = Window.prompt("Model URL", modelURLs[Math.floor(Math.random() * modelURLs.length)]);
        if (url == null || url == "") {
            return;
        }
        
        var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));
        
        if (position.x > 0 && position.y > 0 && position.z > 0) {
            Models.addModel({ position: position,
                            radius: radiusDefault,
                            modelURL: url
                            });
        } else {
            print("Can't create model: Model would be out of bounds.");
        }
        
    } else if (browser == toolBar.clicked(clickedOverlay)) {
        var url = Window.s3Browse();
        if (url == null || url == "") {
            return;
        }
        
        var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));
        
        if (position.x > 0 && position.y > 0 && position.z > 0) {
            Models.addModel({ position: position,
                            radius: radiusDefault,
                            modelURL: url
                            });
        } else {
            print("Can't create model: Model would be out of bounds.");
        }
        
    } else {
        var pickRay = Camera.computePickRay(event.x, event.y);
        Vec3.print("[Mouse] Looking at: ", pickRay.origin);
        var foundIntersection = Models.findRayIntersection(pickRay);
        
        if(!foundIntersection.accurate) {
            return;
        }
        var foundModel = foundIntersection.modelID;
        
        if (!foundModel.isKnownID) {
            var identify = Models.identifyModel(foundModel);
            if (!identify.isKnownID) {
                print("Unknown ID " + identify.id + " (update loop " + foundModel.id + ")");
                return;
            }
            foundModel = identify;
        }
        
        var properties = Models.getModelProperties(foundModel);
        if (isLocked(properties)) {
            print("Model locked " + properties.id);
        } else {
            print("Checking properties: " + properties.id + " " + properties.isKnownID);
            //                P         P - Model
            //               /|         A - Palm
            //              / | d       B - unit vector toward tip
            //             /  |         X - base of the perpendicular line
            //            A---X----->B  d - distance fom axis
            //              x           x - distance from A
            //
            //            |X-A| = (P-A).B
            //            X == A + ((P-A).B)B
            //            d = |P-X|
            
            var A = pickRay.origin;
            var B = Vec3.normalize(pickRay.direction);
            var P = properties.position;
            
            var x = Vec3.dot(Vec3.subtract(P, A), B);
            var X = Vec3.sum(A, Vec3.multiply(B, x));
            var d = Vec3.length(Vec3.subtract(P, X));
            
            var angularSize = 2 * Math.atan(properties.radius / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14;
            if (0 < x && angularSize > MIN_ANGULAR_SIZE) {
                if (angularSize < MAX_ANGULAR_SIZE) {
                    modelSelected = true;
                    selectedModelID = foundModel;
                    selectedModelProperties = properties;
                    
                    orientation = MyAvatar.orientation;
                    intersection = rayPlaneIntersection(pickRay, P, Quat.getFront(orientation));
                } else {
                    print("Angular size too big: " + 2 * Math.atan(properties.radius / Vec3.distance(Camera.getPosition(), properties.position)) * 180 / 3.14);
                }
            }
        }
    }
    
    if (modelSelected) {
        selectedModelProperties.oldRadius = selectedModelProperties.radius;
        selectedModelProperties.oldPosition = {
        x: selectedModelProperties.position.x,
        y: selectedModelProperties.position.y,
        z: selectedModelProperties.position.z,
        };
        selectedModelProperties.oldRotation = {
        x: selectedModelProperties.modelRotation.x,
        y: selectedModelProperties.modelRotation.y,
        z: selectedModelProperties.modelRotation.z,
        w: selectedModelProperties.modelRotation.w,
        };
        selectedModelProperties.glowLevel = 0.0;
        
        print("Clicked on " + selectedModelID.id + " " +  modelSelected);
        tooltip.updateText(selectedModelProperties);
        tooltip.show(true);
    }
}

var glowedModelID = { id: -1, isKnownID: false };
var oldModifier = 0;
var modifier = 0;
var wasShifted = false;
function mouseMoveEvent(event)  {
    if (event.isAlt) {
        return;
    }
    
    var pickRay = Camera.computePickRay(event.x, event.y);
    
    if (!modelSelected) {
        var modelIntersection = Models.findRayIntersection(pickRay);
        if (modelIntersection.accurate) {
            if(glowedModelID.isKnownID && glowedModelID.id != modelIntersection.modelID.id) {
                Models.editModel(glowedModelID, { glowLevel: 0.0 });
                glowedModelID.id = -1;
                glowedModelID.isKnownID = false;
            }
            
            var angularSize = 2 * Math.atan(modelIntersection.modelProperties.radius / Vec3.distance(Camera.getPosition(), modelIntersection.modelProperties.position)) * 180 / 3.14;
            if (modelIntersection.modelID.isKnownID && angularSize > MIN_ANGULAR_SIZE && angularSize < MAX_ANGULAR_SIZE) {
                Models.editModel(modelIntersection.modelID, { glowLevel: 0.25 });
                glowedModelID = modelIntersection.modelID;
            }
        }
        return;
    }
    
    if (event.isLeftButton) {
        if (event.isRightButton) {
            modifier = 1; // Scale
        } else {
            modifier = 2; // Translate
        }
    } else if (event.isRightButton) {
        modifier = 3; // rotate
    } else {
        modifier = 0;
    }
    pickRay = Camera.computePickRay(event.x, event.y);
    if (wasShifted != event.isShifted || modifier != oldModifier) {
        selectedModelProperties.oldRadius = selectedModelProperties.radius;
        
        selectedModelProperties.oldPosition = {
        x: selectedModelProperties.position.x,
        y: selectedModelProperties.position.y,
        z: selectedModelProperties.position.z,
        };
        selectedModelProperties.oldRotation = {
        x: selectedModelProperties.modelRotation.x,
        y: selectedModelProperties.modelRotation.y,
        z: selectedModelProperties.modelRotation.z,
        w: selectedModelProperties.modelRotation.w,
        };
        orientation = MyAvatar.orientation;
        intersection = rayPlaneIntersection(pickRay,
                                            selectedModelProperties.oldPosition,
                                            Quat.getFront(orientation));
        
        mouseLastPosition = { x: event.x, y: event.y };
        wasShifted = event.isShifted;
        oldModifier = modifier;
        return;
    }
    
    
    switch (modifier) {
        case 0:
            return;
        case 1:
            // Let's Scale
            selectedModelProperties.radius = (selectedModelProperties.oldRadius *
                                              (1.0 + (mouseLastPosition.y - event.y) / SCALE_FACTOR));
            
            if (selectedModelProperties.radius < 0.01) {
                print("Scale too small ... bailling.");
                return;
            }
            break;
            
        case 2:
            // Let's translate
            var newIntersection = rayPlaneIntersection(pickRay,
                                                       selectedModelProperties.oldPosition,
                                                       Quat.getFront(orientation));
            var vector = Vec3.subtract(newIntersection, intersection)
            if (event.isShifted) {
                var i = Vec3.dot(vector, Quat.getRight(orientation));
                var j = Vec3.dot(vector, Quat.getUp(orientation));
                vector = Vec3.sum(Vec3.multiply(Quat.getRight(orientation), i),
                                  Vec3.multiply(Quat.getFront(orientation), j));
            }
            
            selectedModelProperties.position = Vec3.sum(selectedModelProperties.oldPosition, vector);
            break;
        case 3:
            // Let's rotate
            if (somethingChanged) {
                selectedModelProperties.oldRotation.x = selectedModelProperties.modelRotation.x;
                selectedModelProperties.oldRotation.y = selectedModelProperties.modelRotation.y;
                selectedModelProperties.oldRotation.z = selectedModelProperties.modelRotation.z;
                selectedModelProperties.oldRotation.w = selectedModelProperties.modelRotation.w;
                mouseLastPosition.x = event.x;
                mouseLastPosition.y = event.y;
                somethingChanged = false;
            }
            
            
            var pixelPerDegrees = windowDimensions.y / (1 * 360); // the entire height of the window allow you to make 2 full rotations
            
            //compute delta in pixel
            var cameraForward = Quat.getFront(Camera.getOrientation());
            var rotationAxis = (!zIsPressed && xIsPressed) ? { x: 1, y: 0, z: 0 } :
                               (!zIsPressed && !xIsPressed) ? { x: 0, y: 1, z: 0 } :
                                                              { x: 0, y: 0, z: 1 };
            rotationAxis = Vec3.multiplyQbyV(selectedModelProperties.modelRotation, rotationAxis);
            var orthogonalAxis = Vec3.cross(cameraForward, rotationAxis);
            var mouseDelta = { x: event.x - mouseLastPosition
                .x, y: mouseLastPosition.y - event.y, z: 0 };
            var transformedMouseDelta = Vec3.multiplyQbyV(Camera.getOrientation(), mouseDelta);
            var delta = Math.floor(Vec3.dot(transformedMouseDelta, Vec3.normalize(orthogonalAxis)) / pixelPerDegrees);
            
            var STEP = 15;
            if (!event.isShifted) {
                delta = Math.round(delta / STEP) * STEP;
            }
                
            var rotation = Quat.fromVec3Degrees({
                                                x: (!zIsPressed && xIsPressed) ? delta : 0,   // x is pressed
                                                y: (!zIsPressed && !xIsPressed) ? delta : 0,   // neither is pressed
                                                z: (zIsPressed && !xIsPressed) ? delta : 0   // z is pressed
                                                });
            rotation = Quat.multiply(selectedModelProperties.oldRotation, rotation);
            
            selectedModelProperties.modelRotation.x = rotation.x;
            selectedModelProperties.modelRotation.y = rotation.y;
            selectedModelProperties.modelRotation.z = rotation.z;
            selectedModelProperties.modelRotation.w = rotation.w;
            break;
    }
    
    Models.editModel(selectedModelID, selectedModelProperties);
    tooltip.updateText(selectedModelProperties);
}


function mouseReleaseEvent(event) {
    if (event.isAlt) {
        return;
    }
    
    if (modelSelected) {
        tooltip.show(false);
    }
    
    modelSelected = false;
    
    glowedModelID.id = -1;
    glowedModelID.isKnownID = false;
}

// In order for editVoxels and editModels to play nice together, they each check to see if a "delete" menu item already
// exists. If it doesn't they add it. If it does they don't. They also only delete the menu item if they were the one that
// added it.
var modelMenuAddedDelete = false;
function setupModelMenus() {
    print("setupModelMenus()");
    // adj our menuitems
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Models", isSeparator: true, beforeItem: "Physics" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Edit Properties...", 
        shortcutKeyEvent: { text: "`" }, afterItem: "Models" });
    if (!Menu.menuItemExists("Edit","Delete")) {
        print("no delete... adding ours");
        Menu.addMenuItem({ menuName: "Edit", menuItemName: "Delete", 
            shortcutKeyEvent: { text: "backspace" }, afterItem: "Models" });
        modelMenuAddedDelete = true;
    } else {
        print("delete exists... don't add ours");
    }

    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Paste Models", shortcutKey: "CTRL+META+V", afterItem: "Edit Properties..." });

    Menu.addMenuItem({ menuName: "File", menuItemName: "Models", isSeparator: true, beforeItem: "Settings" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Export Models", shortcutKey: "CTRL+META+E", afterItem: "Models" });
    Menu.addMenuItem({ menuName: "File", menuItemName: "Import Models", shortcutKey: "CTRL+META+I", afterItem: "Export Models" });
}

function cleanupModelMenus() {
    Menu.removeSeparator("Edit", "Models");
    Menu.removeMenuItem("Edit", "Edit Properties...");
    if (modelMenuAddedDelete) {
        // delete our menuitems
        Menu.removeMenuItem("Edit", "Delete");
    }

    Menu.removeMenuItem("Edit", "Paste Models");

    Menu.removeSeparator("File", "Models");
    Menu.removeMenuItem("File", "Export Models");
    Menu.removeMenuItem("File", "Import Models");
}

function scriptEnding() {
    leftController.cleanup();
    rightController.cleanup();
    toolBar.cleanup();
    cleanupModelMenus();
    tooltip.cleanup();
    modelImporter.cleanup();
    if (exportMenu) {
        exportMenu.close();
    }
}
Script.scriptEnding.connect(scriptEnding);

// register the call back so it fires before each data send
Script.update.connect(checkController);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

setupModelMenus();

function handeMenuEvent(menuItem){
    print("menuItemEvent() in JS... menuItem=" + menuItem);
    if (menuItem == "Delete") {
        if (leftController.grabbing) {
            print("  Delete Model.... leftController.modelID="+ leftController.modelID);
            Models.deleteModel(leftController.modelID);
            leftController.grabbing = false;
        } else if (rightController.grabbing) {
            print("  Delete Model.... rightController.modelID="+ rightController.modelID);
            Models.deleteModel(rightController.modelID);
            rightController.grabbing = false;
        } else if (modelSelected) {
            print("  Delete Model.... selectedModelID="+ selectedModelID);
            Models.deleteModel(selectedModelID);
            modelSelected = false;
        } else {
            print("  Delete Model.... not holding...");
        }
    } else if (menuItem == "Edit Properties...") {
        var editModelID = -1;
        if (leftController.grabbing) {
            print("  Edit Properties.... leftController.modelID="+ leftController.modelID);
            editModelID = leftController.modelID;
        } else if (rightController.grabbing) {
            print("  Edit Properties.... rightController.modelID="+ rightController.modelID);
            editModelID = rightController.modelID;
        } else if (modelSelected) {
            print("  Edit Properties.... selectedModelID="+ selectedModelID);
            editModelID = selectedModelID;
        } else {
            print("  Edit Properties.... not holding...");
        }
        if (editModelID != -1) {
            print("  Edit Properties.... about to edit properties...");
            var array = new Array();
            var decimals = 3;
            array.push({ label: "Model URL:", value: selectedModelProperties.modelURL });
            array.push({ label: "Animation URL:", value: selectedModelProperties.animationURL });
            array.push({ label: "X:", value: selectedModelProperties.position.x.toFixed(decimals) });
            array.push({ label: "Y:", value: selectedModelProperties.position.y.toFixed(decimals) });
            array.push({ label: "Z:", value: selectedModelProperties.position.z.toFixed(decimals) });
            var angles = Quat.safeEulerAngles(selectedModelProperties.modelRotation);
            array.push({ label: "Pitch:", value: angles.x.toFixed(decimals) });
            array.push({ label: "Yaw:", value: angles.y.toFixed(decimals) });
            array.push({ label: "Roll:", value: angles.z.toFixed(decimals) });
            array.push({ label: "Scale:", value: 2 * selectedModelProperties.radius.toFixed(decimals) });
            
            var propertyName = Window.form("Edit Properties", array);
            modelSelected = false;
            
            selectedModelProperties.modelURL = array[0].value;
            selectedModelProperties.animationURL = array[1].value;
            selectedModelProperties.position.x = array[2].value;
            selectedModelProperties.position.y = array[3].value;
            selectedModelProperties.position.z = array[4].value;
            angles.x = array[5].value;
            angles.y = array[6].value;
            angles.z = array[7].value;
            selectedModelProperties.modelRotation = Quat.fromVec3Degrees(angles);
            selectedModelProperties.radius = array[8].value / 2;

            Models.editModel(selectedModelID, selectedModelProperties);
        }
    } else if (menuItem == "Paste Models") {
        modelImporter.paste();
    } else if (menuItem == "Export Models") {
        if (!exportMenu) {
            exportMenu = new ExportMenu({
                onClose: function() {
                    exportMenu = null;
                }
            });
        }
    } else if (menuItem == "Import Models") {
        modelImporter.doImport();
    }
    tooltip.show(false);
}
Menu.menuItemEvent.connect(handeMenuEvent);



// handling of inspect.js concurrence
var zIsPressed = false;
var xIsPressed = false;
var somethingChanged = false;
Controller.keyPressEvent.connect(function(event) {
    if ((event.text == "z" || event.text == "Z") && !zIsPressed) {
        zIsPressed = true;
        somethingChanged = true;
    }
    if ((event.text == "x" || event.text == "X") && !xIsPressed) {
        xIsPressed = true;
        somethingChanged = true;
    }
                                 
    // resets model orientation when holding with mouse
    if (event.text == "r" && modelSelected) {
        selectedModelProperties.modelRotation = Quat.fromVec3Degrees({ x: 0, y: 0, z: 0 });
        Models.editModel(selectedModelID, selectedModelProperties);
        tooltip.updateText(selectedModelProperties);
        somethingChanged = true;
    }
});
Controller.keyReleaseEvent.connect(function(event) {
    if (event.text == "z" || event.text == "Z") {
        zIsPressed = false;
        somethingChanged = true;
    }
    if (event.text == "x" || event.text == "X") {
        xIsPressed = false;
        somethingChanged = true;
    }
    // since sometimes our menu shortcut keys don't work, trap our menu items here also and fire the appropriate menu items
    if (event.text == "`") {
        handeMenuEvent("Edit Properties...");
    }
    if (event.text == "BACKSPACE") {
        handeMenuEvent("Delete");
    }
});
