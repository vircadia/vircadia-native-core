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

Script.include("toolBars.js");

var windowDimensions = Controller.getViewportDimensions();
var toolIconUrl = "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/tools/";
var toolHeight = 50;
var toolWidth = 50;

var LASER_WIDTH = 4;
var LASER_COLOR = { red: 255, green: 0, blue: 0 };
var LASER_LENGTH_FACTOR = 5;

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
        
            this.oldModelPosition = properties.position;
            this.oldModelRotation = properties.modelRotation;
            this.oldModelRadius = properties.radius;
        }
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
        
        if (0 < x && x < LASER_LENGTH_FACTOR) {
            return { valid: true, x: x, y: y, z: z };
        }
        return { valid: false };
    }
    
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
    
    this.hideLaser = function() {
        Overlays.editOverlay(this.laser, { visible: false });
        Overlays.editOverlay(this.ball, { visible: false });
        Overlays.editOverlay(this.leftRight, { visible: false });
        Overlays.editOverlay(this.topDown, { visible: false });
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
//            print("Moving " + this.modelID.id);
//            Vec3.print("Old Position: ", this.oldModelPosition);
//            Vec3.print("Sav Position: ", newPosition);
//            Quat.print("Old Rotation: ", this.oldModelRotation);
//            Quat.print("New Rotation: ", newRotation);
            
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
            var pickRay = { origin: this.palmPosition,
                            direction: Vec3.normalize(Vec3.subtract(this.tipPosition, this.palmPosition)) };
            var foundIntersection = Models.findRayIntersection(pickRay);
            
            if(!foundIntersection.accurate) {
                return;
            }
            var foundModel = foundIntersection.modelID;
            
            if (!foundModel.isKnownID) {
                var identify = Models.identifyModel(foundModel);
                if (!identify.isKnownID) {
                    print("Unknown ID " + identify.id + " (update loop " + foundModel.id + ")");
                    continue;
                }
                foundModel = identify;
            }
            
            var properties = Models.getModelProperties(foundModel);
            print("foundModel.modelURL=" + properties.modelURL);
            
            if (isLocked(properties)) {
                print("Model locked " + properties.id);
            } else {
                print("Checking properties: " + properties.id + " " + properties.isKnownID);
                var check = this.checkModel(properties);
                if (check.valid) {
                    this.grab(foundModel, properties);
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
        //print("Both controllers");
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
        //Vec3.print("Ratio : " + ratio + " New position: ", newPosition);
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
            
            leftController.hideLaser();
            rightController.hideLaser();
        }
    }
    
    moveOverlays();
}



function initToolBar() {
    toolBar = new ToolBar(0, 0, ToolBar.VERTICAL);
    // New Model
    newModel = toolBar.addTool({
                               imageURL: toolIconUrl + "voxel-tool.svg",
                               subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                               width: toolWidth, height: toolHeight,
                               visible: true,
                               alpha: 0.9
                               });
}

function moveOverlays() {
    if (typeof(toolBar) === 'undefined') {
        initToolBar();
        
    } else if (windowDimensions.x == Controller.getViewportDimensions().x &&
               windowDimensions.y == Controller.getViewportDimensions().y) {
        return;
    }
    
    
    windowDimensions = Controller.getViewportDimensions();
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
var TRANSLATION_FACTOR = 100.0;
var ROTATION_FACTOR = 100.0;

function rayPlaneIntersection(pickRay, point, normal) {
    var d = -Vec3.dot(point, normal);
    var t = -(Vec3.dot(pickRay.origin, normal) + d) / Vec3.dot(pickRay.direction, normal);
    
    return Vec3.sum(pickRay.origin, Vec3.multiply(pickRay.direction, t));
}

function mousePressEvent(event) {
    mouseLastPosition = { x: event.x, y: event.y };
    modelSelected = false;
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    
    if (newModel == toolBar.clicked(clickedOverlay)) {
        var url = Window.prompt("Model url", modelURLs[Math.floor(Math.random() * modelURLs.length)]);
        if (url == null) {
            return;
        }
        
        var position = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), SPAWN_DISTANCE));
        Models.addModel({ position: position,
                        radius: radiusDefault,
                        modelURL: url
                        });
        
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
                continue;
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
            
            if (0 < x && x < LASER_LENGTH_FACTOR) {
                modelSelected = true;
                selectedModelID = foundModel;
                selectedModelProperties = properties;
                
                orientation = MyAvatar.orientation;
                intersection = rayPlaneIntersection(pickRay, P, Quat.getFront(orientation));
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
    }
}

var glowedModelID = { id: -1, isKnownID: false };
var oldModifier = 0;
var modifier = 0;
var wasShifted = false;
function mouseMoveEvent(event)  {
    var pickRay = Camera.computePickRay(event.x, event.y);
    
    if (!modelSelected) {
        var modelIntersection = Models.findRayIntersection(pickRay);
        if (modelIntersection.accurate) {
            if(glowedModelID.isKnownID && glowedModelID.id != modelIntersection.modelID.id) {
                Models.editModel(glowedModelID, { glowLevel: 0.0 });
                glowedModelID.id = -1;
                glowedModelID.isKnownID = false;
            }
            
            if (modelIntersection.modelID.isKnownID) {
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
            var rotation = Quat.fromVec3Degrees({ x: event.y - mouseLastPosition.y, y: event.x - mouseLastPosition.x, z: 0 });
            if (event.isShifted) {
                rotation = Quat.fromVec3Degrees({ x: event.y - mouseLastPosition.y, y: 0, z: mouseLastPosition.x - event.x });
            }
            
            var newRotation = Quat.multiply(orientation, rotation);
            newRotation = Quat.multiply(newRotation, Quat.inverse(orientation));
            
            selectedModelProperties.modelRotation = Quat.multiply(newRotation, selectedModelProperties.oldRotation);
            break;
    }
    
    Models.editModel(selectedModelID, selectedModelProperties);
}

function mouseReleaseEvent(event) {
    modelSelected = false;
    
    glowedModelID.id = -1;
    glowedModelID.isKnownID = false;
}



function setupModelMenus() {
    // add our menuitems
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Models", isSeparator: true, beforeItem: "Physics" });
    Menu.addMenuItem({ menuName: "Edit", menuItemName: "Delete Model", shortcutKeyEvent: { text: "backspace" }, afterItem: "Models" });
}

function cleanupModelMenus() {
    // delete our menuitems
    Menu.removeSeparator("Edit", "Models");
    Menu.removeMenuItem("Edit", "Delete Model");
}

function scriptEnding() {
    leftController.cleanup();
    rightController.cleanup();
    toolBar.cleanup();
    cleanupModelMenus();
}
Script.scriptEnding.connect(scriptEnding);

// register the call back so it fires before each data send
Script.update.connect(checkController);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

setupModelMenus();
Menu.menuItemEvent.connect(function(menuItem){
    print("menuItemEvent() in JS... menuItem=" + menuItem);
    if (menuItem == "Delete Model") {
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
    }
});


