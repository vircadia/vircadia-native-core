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

var LASER_WIDTH = 4;
var LASER_COLOR = { red: 255, green: 0, blue: 0 };
var LASER_LENGTH_FACTOR = 1.5;

var LEFT = 0;
var RIGHT = 1;

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
    this.modelID;
    
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
    

    
    this.grab = function (modelID) {
        if (!modelID.isKnownID) {
            var identify = Models.identifyModel(modelID);
            if (!identify.isKnownID) {
                print("Unknown ID " + identify.id + "(grab)");
                return;
            }
            modelID = identify;
        }
        print("Grabbing " + modelID.id);
        this.grabbing = true;
        this.modelID = modelID;
    }
    
    this.release = function () {
        this.grabbing = false;
        this.modelID = 0;
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
    
    this.checkModel = function (modelID) {
        if (!modelID.isKnownID) {
            var identify = Models.identifyModel(modelID);
            if (!identify.isKnownID) {
                print("Unknown ID " + identify.id + "(checkModel)");
                return;
            }
            modelID = identify;
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
        var P = Models.getModelProperties(modelID).position;
        
        this.x = Vec3.dot(Vec3.subtract(P, A), B);
        this.y = Vec3.dot(Vec3.subtract(P, A), this.up);
        this.z = Vec3.dot(Vec3.subtract(P, A), this.right);
        var X = Vec3.sum(A, Vec3.multiply(B, this.x));
        var d = Vec3.length(Vec3.subtract(P, X));
        
//        Vec3.print("A: ", A);
//        Vec3.print("B: ", B);
//        Vec3.print("Particle pos: ", P);
//        print("d: " + d + ", x: " + this.x);
        if (d < Models.getModelProperties(modelID).radius && 0 < this.x && this.x < LASER_LENGTH_FACTOR) {
            return true;
        }
        return false;
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
        
        if (this.pressing) {
            Vec3.print("Looking at: ", this.palmPosition);
            var foundModels = Models.findModels(this.palmPosition, LASER_LENGTH_FACTOR);
            for (var i = 0; i < foundModels.length; i++) {
                print("Model found ID (" + foundModels[i].id + ")");
                if (this.checkModel(foundModels[i])) {
                    if (this.grab(foundModels[i])) {
                        return;
                    }
                }
            }
        }
        
        if (!this.pressed && this.grabbing) {
            // release if trigger not pressed anymore.
            this.release();
        }
    
        this.moveLaser();
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
    if (leftController.grabbing) {
        if (rightController.grabbing) {
            var properties = Models.getModelProperties(leftController.modelID);
            
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
                                       Vec3.multiply(Vec3.subtract(properties.position, oldMiddle), ratio));
            Vec3.print("Ratio : " + ratio + " New position: ", newPosition);
            var rotation = Quat.multiply(leftController.rotation,
                                         Quat.inverse(leftController.oldRotation));
            rotation = Quat.multiply(rotation, properties.modelRotation);
            
            Models.editModel(leftController.modelID, {
                                   position: newPosition,
                                   //modelRotation: rotation,
                                   radius: properties.radius * ratio
                                   });
            
            return;
        } else {
            var newPosition = Vec3.sum(leftController.palmPosition,
                                       Vec3.multiply(leftController.front, leftController.x));
            newPosition = Vec3.sum(newPosition,
                                   Vec3.multiply(leftController.up, leftController.y));
            newPosition = Vec3.sum(newPosition,
                                   Vec3.multiply(leftController.right, leftController.z));
            
            var rotation = Quat.multiply(leftController.rotation,
                                         Quat.inverse(leftController.oldRotation));
            rotation = Quat.multiply(rotation,
                                     Models.getModelProperties(leftController.modelID).modelRotation);
            
            Models.editModel(leftController.modelID, {
                             position: newPosition,
                             modelRotation: rotation
                             });
        }
    }
    
    
    if (rightController.grabbing) {
        var newPosition = Vec3.sum(rightController.palmPosition,
                                   Vec3.multiply(rightController.front, rightController.x));
        newPosition = Vec3.sum(newPosition,
                               Vec3.multiply(rightController.up, rightController.y));
        newPosition = Vec3.sum(newPosition,
                               Vec3.multiply(rightController.right, rightController.z));
        
        var rotation = Quat.multiply(rightController.rotation,
                                     Quat.inverse(rightController.oldRotation));
        rotation = Quat.multiply(rotation,
                                 Models.getModelProperties(rightController.modelID).modelRotation);
        
        Models.editModel(rightController.modelID, {
                         position: newPosition,
                         modelRotation: rotation
                         });
    }
}

function checkController(deltaTime) {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    // this is expected for hydras
    if (!(numberOfButtons==12 && numberOfTriggers == 2 && controllersPerTrigger == 2)) {
        //print("no hydra connected?");
        return; // bail if no hydra
    }
    
    leftController.update();
    rightController.update();
    moveModels();
}

function scriptEnding() {
    leftController.cleanup();
    rightController.cleanup();
}
Script.scriptEnding.connect(scriptEnding);

// register the call back so it fires before each data send
Script.update.connect(checkController);



