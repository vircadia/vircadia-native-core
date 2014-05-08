//
//  editModels.js
//  examples
//
//  Created by Clément Brisset on 4/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var windowDimensions = Controller.getViewportDimensions();

var LASER_WIDTH = 4;
var LASER_COLOR = { red: 255, green: 0, blue: 0 };
var LASER_LENGTH_FACTOR = 1.5;

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

var tools = [];
var toolIconUrl = "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/tools/";
var numberOfTools = 1;
var toolHeight = 50;
var toolWidth = 50;
var toolVerticalSpacing = 4;
var toolsHeight = toolHeight * numberOfTools + toolVerticalSpacing * (numberOfTools - 1);
var toolsX = windowDimensions.x - 8 - toolWidth;
var toolsY = (windowDimensions.y - toolsHeight) / 2;


var firstModel = Overlays.addOverlay("image", {
                                     subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                                     imageURL: toolIconUrl + "voxel-tool.svg",
                                     x: toolsX, y: toolsY + ((toolHeight + toolVerticalSpacing) * 0), width: toolWidth, height: toolHeight,
                                     visible: true,
                                     alpha: 0.9
                                     });
function Tool(iconURL) {
    this.overlay = Overlays.addOverlay("image", {
                                       subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                                       imageURL: iconURL,
                                       x: toolsX,
                                       y: toolsY + ((toolHeight + toolVerticalSpacing) * tools.length),
                                       width: toolWidth,
                                       height: toolHeight,
                                       visible: true,
                                       alpha: 0.9
                                       });
    
    
    this.cleanup = function() {
        Ovelays.deleteOverlay(this.overlay);
    }
    tools[tools.length] = this;
    return tools.length - 1;
}
Tool.ICON_URL = "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/tools/";
Tool.HEIGHT = 50;
Tool.WIDTH = 50;

function ToolBar(direction, x, y) {
    this.tools = [];
    
    this.numberOfTools = function() {
        return this.tools.length;
    }
}
ToolBar.SPACING = 4;


function controller(wichSide) {
    this.side = wichSide;
    this.palm = 2 * wichSide;
    this.tip = 2 * wichSide + 1;
    this.trigger = wichSide;
    
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
    
    this.pressed = false; // is trigger pressed
    this.pressing = false; // is trigger being pressed (is pressed now but wasn't previously)
    
    this.grabbing = false;
    this.modelID = { isKnownID: false };
    this.oldModelRotation;
    this.oldModelPosition;
    this.oldModelRadius;
    
    this.laser = Overlays.addOverlay("line3d", {
                                     position: this.palmPosition,
                                     end: this.tipPosition,
                                     color: LASER_COLOR,
                                     alpha: 1,
                                     visible: false,
                                     lineWidth: LASER_WIDTH
                                     });
    
    this.guideScale = 0.02;
    this.ball = Overlays.addOverlay("sphere", {
                                    position: this.palmPosition,
                                    size: this.guideScale,
                                    solid: true,
                                    color: { red: 0, green: 255, blue: 0 },
                                    alpha: 1,
                                    visible: false,
                                    });
    this.leftRight = Overlays.addOverlay("line3d", {
                                         position: this.palmPosition,
                                         end: this.tipPosition,
                                         color: { red: 0, green: 0, blue: 255 },
                                         alpha: 1,
                                         visible: false,
                                         lineWidth: LASER_WIDTH
                                         });
    this.topDown = Overlays.addOverlay("line3d", {
                                       position: this.palmPosition,
                                       end: this.tipPosition,
                                       color: { red: 0, green: 0, blue: 255 },
                                       alpha: 1,
                                       visible: false,
                                       lineWidth: LASER_WIDTH
                                       });
    

    
    this.grab = function (modelID, properties) {
        print("Grabbing " + modelID.id);
        this.grabbing = true;
        this.modelID = modelID;
        
        this.oldModelPosition = properties.position;
        this.oldModelRotation = properties.modelRotation;
        this.oldModelRadius = properties.radius;
    }
    
    this.release = function () {
        this.grabbing = false;
        this.modelID.isKnownID = false;
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
        
        if (d < properties.radius && 0 < x && x < LASER_LENGTH_FACTOR) {
            return { valid: true, x: x, y: y, z: z };
        }
        return { valid: false };
    }
    
    this.moveLaser = function () {
        var endPosition = Vec3.sum(this.palmPosition, Vec3.multiply(this.front, LASER_LENGTH_FACTOR));
        
        Overlays.editOverlay(this.laser, {
                             position: this.palmPosition,
                             end: endPosition,
                             visible: true
                             });
        
        
        Overlays.editOverlay(this.ball, {
                             position: endPosition,
                             visible: true
                             });
        Overlays.editOverlay(this.leftRight, {
                             position: Vec3.sum(endPosition, Vec3.multiply(this.right, 2 * this.guideScale)),
                             end: Vec3.sum(endPosition, Vec3.multiply(this.right, -2 * this.guideScale)),
                             visible: true
                             });
        Overlays.editOverlay(this.topDown, {position: Vec3.sum(endPosition, Vec3.multiply(this.up, 2 * this.guideScale)),
                             end: Vec3.sum(endPosition, Vec3.multiply(this.up, -2 * this.guideScale)),
                             visible: true
                             });
    }
    
    this.moveModel = function () {
        if (this.grabbing) {
            var newPosition = Vec3.sum(this.palmPosition,
                                       Vec3.multiply(this.front, this.x));
            newPosition = Vec3.sum(newPosition,
                                   Vec3.multiply(this.up, this.y));
            newPosition = Vec3.sum(newPosition,
                                   Vec3.multiply(this.right, this.z));
            
            var newRotation = Quat.multiply(this.rotation,
                                            Quat.inverse(this.oldRotation));
            newRotation = Quat.multiply(newRotation,
                                        this.oldModelRotation);
            
            Models.editModel(this.modelID, {
                             position: newPosition,
                             modelRotation: newRotation
                             });
            print("Moving " + this.modelID.id);
//            Vec3.print("Old Position: ", this.oldModelPosition);
//            Vec3.print("Sav Position: ", newPosition);
            Quat.print("Old Rotation: ", this.oldModelRotation);
            Quat.print("New Rotation: ", newRotation);
            
            this.oldModelRotation = newRotation;
            this.oldModelPosition = newPosition;
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
        
        this.checkTrigger();
        
        this.moveLaser();
        
        if (!this.pressed && this.grabbing) {
            // release if trigger not pressed anymore.
            this.release();
        }
        
        if (this.pressing) {
            Vec3.print("Looking at: ", this.palmPosition);
            var foundModels = Models.findModels(this.palmPosition, LASER_LENGTH_FACTOR);
            for (var i = 0; i < foundModels.length; i++) {
                
                if (!foundModels[i].isKnownID) {
                    var identify = Models.identifyModel(foundModels[i]);
                    if (!identify.isKnownID) {
                        print("Unknown ID " + identify.id + "(update loop)");
                        return;
                    }
                    foundModels[i] = identify;
                }
                
                var properties = Models.getModelProperties(foundModels[i]);
                print("Checking properties: " + properties.id + " " + properties.isKnownID);
                
                var check = this.checkModel(properties);
                if (check.valid) {
                    this.grab(foundModels[i], properties);
                    this.x = check.x;
                    this.y = check.y;
                    this.z = check.z;
                    return;
                }
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
        print("Both controllers");
        var oldLeftPoint = Vec3.sum(leftController.oldPalmPosition, Vec3.multiply(leftController.oldFront, leftController.x));
        var oldRightPoint = Vec3.sum(rightController.oldPalmPosition, Vec3.multiply(rightController.oldFront, rightController.x));
        
        var oldMiddle = Vec3.multiply(Vec3.sum(oldLeftPoint, oldRightPoint), 0.5);
        var oldLength = Vec3.length(Vec3.subtract(oldLeftPoint, oldRightPoint));
        
        
        var leftPoint = Vec3.sum(leftController.palmPosition, Vec3.multiply(leftController.front, leftController.x));
        var rightPoint = Vec3.sum(rightController.palmPosition, Vec3.multiply(rightController.front, rightController.x));
        
        var middle = Vec3.multiply(Vec3.sum(leftPoint, rightPoint), 0.5);
        var length = Vec3.length(Vec3.subtract(leftPoint, rightPoint));
        
        var ratio = length / oldLength;
        
        var newPosition = Vec3.sum(middle,
                                   Vec3.multiply(Vec3.subtract(leftController.oldModelPosition, oldMiddle), ratio));
        Vec3.print("Ratio : " + ratio + " New position: ", newPosition);
        var rotation = Quat.multiply(leftController.rotation,
                                     Quat.inverse(leftController.oldRotation));
        rotation = Quat.multiply(rotation, leftController.oldModelRotation);
        
        Models.editModel(leftController.modelID, {
                         position: newPosition,
                         //modelRotation: rotation,
                         radius: leftController.oldModelRadius * ratio
                         });
        
        leftController.oldModelPosition = newPosition;
        leftController.oldModelRotation = rotation;
        leftController.oldModelRadius *= ratio;
        return;
    }
    
    leftController.moveModel();
    rightController.moveModel();
}

function checkController(deltaTime) {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    moveOverlays();
    
    // this is expected for hydras
    if (!(numberOfButtons==12 && numberOfTriggers == 2 && controllersPerTrigger == 2)) {
        //print("no hydra connected?");
        return; // bail if no hydra
    }
    
    leftController.update();
    rightController.update();
    moveModels();
}

function moveOverlays() {
    windowDimensions = Controller.getViewportDimensions();
    
    toolsX = windowDimensions.x - 8 - toolWidth;
    toolsY = (windowDimensions.y - toolsHeight) / 2;
    
    Overlays.addOverlay(firstModel, {
                        x: toolsX, y: toolsY + ((toolHeight + toolVerticalSpacing) * 0), width: toolWidth, height: toolHeight,
                        });
}

function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    var url;
    
    if (clickedOverlay == firstModel) {
        url = Window.prompt("Model url", modelURLs[Math.floor(Math.random() * modelURLs.length)]);
        if (url == null) {
            return;        }
    } else {
        print("Didn't click on anything");
        return;
    }
    
    var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));
    Models.addModel({ position: position,
                    radius: radiusDefault,
                    modelURL: url
                    });
}

function scriptEnding() {
    leftController.cleanup();
    rightController.cleanup();
    
    Overlays.deleteOverlay(firstModel);
}
Script.scriptEnding.connect(scriptEnding);

// register the call back so it fires before each data send
Script.update.connect(checkController);
Controller.mousePressEvent.connect(mousePressEvent);



