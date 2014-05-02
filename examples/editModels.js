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
var LASER_LENGTH_FACTOR = 1;

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
    this.particleID = 0;
    this.oldParticleProperties;
    
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
    

    
    this.grab = function (particle) {
        if (!particle.isKnownID) {
            var identify = Particles.identifyParticle(particle);
            if (!identify.isKnownID) {
                return;
            }
        }
        print("Grabbing");
        this.grabbing = true;
        this.particleID = identify;
        
        this.oldParticleProperties = Particles.getParticleProperties(this.particleID);
    }
    
    this.release = function () {
        this.grabbing = false;
        this.particleID = -1;
        this.oldParticleProperties = { };
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
    
    this.checkParticle = function (particle) {
        if (!particle.isKnownID) {
            var identify = Particles.identifyParticle(particle);
            if (!identify.isKnownID) {
                print("Unknown ID (checkParticle)");
                return;
            }
        }
        //                P         P - Particle
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
        var P = Particles.getParticleProperties(particle).position;
        
        this.x = Vec3.dot(Vec3.subtract(P, A), B);
        this.y = Vec3.dot(Vec3.subtract(P, A), this.up);
        this.z = Vec3.dot(Vec3.subtract(P, A), this.right);
        var X = Vec3.sum(A, Vec3.multiply(B, this.x));
        var d = Vec3.length(Vec3.subtract(P, X));
        
//        Vec3.print("A: ", A);
//        Vec3.print("B: ", B);
//        Vec3.print("Particle pos: ", P);
//        print("d: " + d + ", x: " + this.x);
        if (d < 0.5 && 0 < this.x && this.x < LASER_LENGTH_FACTOR) {
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
            if (this.checkParticle(particleTest1)) {
                this.grab(particleTest1);
            }
        }
        
        if (!this.pressed && this.grabbing) {
            this.release();
        }
        
        if(this.grabbing) {
            this.oldParticleProperties = Particles.getParticleProperties(this.particleID);
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


var particleRadius = 0.05;
var particleTest1 = Particles.addParticle({ position: { x: 0, y: 0, z:0 },
                                          velocity: { x: 0, y: 0, z: 0},
                                          gravity: { x: 0, y: 0, z: 0},
                                          radius: particleRadius,
                                          color: { red: 0, green: 0, blue: 255 },
                                          modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/models/heads/defaultAvatar_head.fst",
                                          lifetime: 600,
                                          })
var particleTest2 = Particles.addParticle({ position: { x: 0, y: 0, z:0 },
                                          velocity: { x: 0, y: 0, z: 0},
                                          gravity: { x: 0, y: 0, z: 0},
                                          radius: particleRadius,
                                          color: { red: 0, green: 0, blue: 255 },
                                          alpha: 0.2,
                                          lifetime: 600,
                                          })


var diff = { x: 0, y: 0, z: 0 };
function moveParticles() {
    if (leftController.grabbing) {
        if (rightController.grabbing) {
            
            var newPosition = Vec3.sum(leftController.palmPosition,
                                       Vec3.multiply(leftController.front, leftController.x));
            newPosition = Vec3.sum(newPosition,
                                   Vec3.multiply(leftController.up, leftController.y));
            newPosition = Vec3.sum(newPosition,
                                   Vec3.multiply(leftController.right, leftController.z));
            
            var rotation = Quat.multiply(leftController.rotation,
                                         Quat.inverse(leftController.oldRotation));
            rotation = Quat.multiply(rotation, Particles.getParticleProperties(leftController.particleID).modelRotation);
            
            Particles.editParticle(leftController.particleID, {
                                   position: newPosition,
                                   modelRotation: rotation,
                                   lifetime: 600
                                   });
            Particles.editParticle(particleTest2, {
                                   position: newPosition,
                                   lifetime: 600
                                   });
            
            return;
            var leftVector = Vec3.sum(Vec3.multiply(leftController.front, leftController.x),
                                      Vec3.multiply(leftController.up, leftController.y));
            leftVector = Vec3.sum(leftVector,
                                  Vec3.multiply(leftController.right, leftController.z));
            
            var rightVector = Vec3.sum(Vec3.multiply(rightController.front, rightController.x),
                                       Vec3.multiply(rightController.up, rightController.y));
            rightVector = Vec3.sum(rightVector,
                                   Vec3.multiply(rightController.right, rightController.z));
            
            var newPosition = Vec3.sum(Vec3.sum(leftController.palmPosition, leftVector),
                                       Vec3.sum(rightController.palmPosition, rightVector));
            newPosition = Vec3.multiply(newPosition, 0.5);
            
            
            var rotantion = Quat.multiply(MyAvatar.orientation,
                                          Quat.inverse(leftController.oldRotation));
            rotation = Quat.multiply(rotation, Particles.getParticleProperties(leftController.particleID).modelRotation);
            
            Particles.editParticle(leftController.particleID, {
                                   position: newPosition,
                                   //modelRotation: rotation,
                                   lifetime: 600
                                   });
            Particles.editParticle(particleTest2, {
                                   position: newPosition,
                                   lifetime: 600
                                   });
            leftController.checkParticle(leftController.particleID);
            rightController.checkParticle(rightController.particleID);
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
            rotation = Quat.multiply(rotation, Particles.getParticleProperties(leftController.particleID).modelRotation);
            
            Particles.editParticle(leftController.particleID, {
                                   position: newPosition,
                                   modelRotation: rotation,
                                   lifetime: 600
                                   });
            Particles.editParticle(particleTest2, {
                                   position: newPosition,
                                   lifetime: 600
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
        rotation = Quat.multiply(rotation, Particles.getParticleProperties(rightController.particleID).modelRotation);
        
        Particles.editParticle(rightController.particleID, {
                               position: newPosition,
                               modelRotation: rotation,
                               lifetime: 600
                               });
        Particles.editParticle(particleTest2, {
                               position: newPosition,
                               lifetime: 600
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
    moveParticles();

    
    
    ///// TEMP ///////
    if (!particleTest1.isKnownID) {
        var identify = Particles.identifyParticle(particleTest1);
        if (!identify.isKnownID) {
            print("Unknown ID (temp)");
            return;
        }
    }
    var createButtonPressed = Controller.isButtonPressed(3);
    if (createButtonPressed) {
        var position = MyAvatar.position;
        var forwardVector = Quat.getFront(MyAvatar.orientation);
        position = Vec3.sum(position, Vec3.multiply(forwardVector, 2));
        Particles.editParticle(particleTest1, {
                               position: position,
                               modelRotation: Quat.fromVec3Radians({ x: 0, y: 0 , z: 0 }),
                               lifetime: 600
                               });
        Particles.editParticle(particleTest2, {
                               position: position,
                               lifetime: 600
                               });
    }
    createButtonPressed = Controller.isButtonPressed(9);
    if (createButtonPressed) {
        var position = MyAvatar.position;
        var forwardVector = Quat.getFront(MyAvatar.orientation);
        position = Vec3.sum(position, Vec3.multiply(forwardVector, 2));
        Particles.editParticle(particleTest1, {
                               position: position,
                               modelRotation: Quat.fromVec3Radians({ x: 0, y: 0 , z: 0 }),
                               lifetime: 600
                               });
        Particles.editParticle(particleTest2, {
                               position: position,
                               lifetime: 600
                               });
    }
    //////////////////////////////////
}

function scriptEnding() {
    leftController.cleanup();
    rightController.cleanup();
    Particles.deleteParticle(particleTest1);
    Particles.deleteParticle(particleTest2);
}
Script.scriptEnding.connect(scriptEnding);

// register the call back so it fires before each data send
Script.update.connect(checkController);



