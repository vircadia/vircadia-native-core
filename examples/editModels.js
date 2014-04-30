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
var LASER_LENGTH_FACTOR = 4;

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
    
    this.triggerValue = Controller.getTriggerValue(this.trigger);
    
    this.pressed = false; // is trigger pressed
    this.pressing = false; // is trigger being pressed (is pressed now but wasn't previously)
    
    this.grabbing = false;
    this.particleID = 0;
    this.oldParticlePosition = { x: 0, y: 0, z: 0 };
    
    this.laser = Overlays.addOverlay("line3d", {
                                     position: this.palmPosition,
                                     end: this.tipPosition,
                                     color: LASER_COLOR,
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
        this.grabbing = true;
        this.particle = identify;
        
        var properties = Particles.getParticleProperties(this.particle);
        
        this.oldParticlePosition = properties.position;
        
        print("Grabbed: " + this.oldParticlePosition);
    }
    
    this.release = function () {
        this.grabbing = false;
        this.particleID = -1;
        this.oldParticlePosition = { x: 0, y: 0, z: 0 };
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
        var vector = Vec3.subtract(this.tipPosition, this.palmPosition);
        var endPosition = Vec3.sum(this.palmPosition, Vec3.multiply(vector, LASER_LENGTH_FACTOR));
        
        Overlays.editOverlay(this.laser, {
                             position: this.palmPosition,
                             end: endPosition,
                             visible: true
                             });
    }
    
    this.moveParticle = function () {
        if (this.grabbing) {
            
            
            
        }
    }
    
    this.update = function () {
        this.oldPalmPosition = this.palmPosition;
        this.oldTipPosition = this.tipPosition;
        this.palmPosition = Controller.getSpatialControlPosition(this.palm);
        this.tipPosition = Controller.getSpatialControlPosition(this.tip);
        this.triggerValue = Controller.getTriggerValue(this.trigger);
        
        this.checkTrigger();
        
        if (this.pressing) {
            var particle = -1;
            
//                P         P - Particle
//               /|         A - Palm
//              / | d       B - unit vector toward tip
//             /  |         X - base of the perpendicular line
//            A---X----->B  d - distance fom axis
//              x           x - distance from A
//
//            |X-A| = (P-A).B, in B unit
//            X == A + ((P-A).B)B
//            d = |P-X| in standard unit
            
            var A = this.palmPosition;
            var B = Vec3.subtract(this.tipPosition, A);
            var P = particlePosition;
            
            var x = Vec3.dot(Vec3.subtract(P, A), B);
            var X = Vec3.sum(A, Vec3.multiply(B, x));
            var d = Vec3.length(Vec3.subtract(P, X));
            
            
            if (d < 0.5 && 0 < x && x < LASER_LENGTH_FACTOR) {
                particle = particleTest;
            }
            Vec3.print("A = ", A);
            Vec3.print("B = ", B);
            Vec3.print("P = ", P);
            Vec3.print("X = ", X);
            print("d = " + d + ", x = " + x);
            this.grab(particle);
        }
        
        if (!this.pressed && this.grabbing) {
            this.release();
        }
    
        
        this.moveLaser();
        this.moveParticle();
    }
    
    this.cleanup = function () {
        Overlays.deleteOverlay(this.laser);
    }
}

var leftController = new controller(LEFT);
var rightController = new controller(RIGHT);



var particlePosition = MyAvatar.position;
var particleRadius = 0.05;
var particleTest = Particles.addParticle({ position: particlePosition,
                                     velocity: { x: 0, y: 0, z: 0},
                                     gravity: { x: 0, y: 0, z: 0},
                                     radius: particleRadius,
                                     damping: 0.999,
                                     color: { red: 255, green: 0, blue: 0 },
                                     lifetime: 100,
                                     modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/models/heads/defaultAvatar_head.fst"
                                     })


function checkController(deltaTime) {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    // this is expected for hydras
    if (!(numberOfButtons==12 && numberOfTriggers == 2 && controllersPerTrigger == 2)) {
        print("no hydra connected?");
        return; // bail if no hydra
    }
    
    leftController.update();
    rightController.update();

    
    
    ///// TEMP ///////
    var createButtonPressed = Controller.isButtonPressed(3) || Controller.isButtonPressed(9);
    if (createButtonPressed) {
        particlePosition = MyAvatar.position;
        var forwardVector = Quat.getFront(MyAvatar.orientation);
        particlePosition = Vec3.sum(particlePosition, Vec3.multiply(forwardVector, 2));
        Particles.editParticle(particleTest, {
                               position: particlePosition,
                               lifetime: 100000
                               });
    }
    //////////////////////////////////
}

function scriptEnding() {
    leftController.cleanup();
    rightController.cleanup();
    Particles.deleteParticle(particleTest);
}
Script.scriptEnding.connect(scriptEnding);

// register the call back so it fires before each data send
Script.update.connect(checkController);



