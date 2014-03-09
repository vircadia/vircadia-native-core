//
//  gun.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Modified by Philip on 3/3/14
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that turns the hydra controllers and mouse into a particle gun.
//  It reads the controller, watches for trigger pulls, and launches particles.
//  When particles collide with voxels they blow little holes out of the voxels. 
//
//

var lastX = 0;
var lastY = 0;
var yawFromMouse = 0;
var pitchFromMouse = 0;
var isMouseDown = false; 

var BULLET_VELOCITY = 5.0;

// Load some sound to use for loading and firing 
var fireSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guns/GUN-SHOT2.raw");
var loadSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guns/Gun_Reload_Weapon22.raw");
var impactSound = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Guns/BulletImpact2.raw");
var audioOptions = new AudioInjectionOptions();
audioOptions.volume = 0.9;

// initialize our triggers
var triggerPulled = new Array();
var numberOfTriggers = Controller.getNumberOfTriggers();
for (t = 0; t < numberOfTriggers; t++) {
    triggerPulled[t] = false;
}

//  Create a reticle image in center of screen 
var screenSize = Controller.getViewportDimensions();
var reticle = Overlays.addOverlay("image", {
                    x: screenSize.x / 2 - 16,
                    y: screenSize.y / 2 - 16,
                    width: 32,
                    height: 32,
                    imageURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/images/reticle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

function shootBullet(position, velocity) {
    var BULLET_SIZE = 0.02;
    Particles.addParticle(
        { position: position, 
          radius: BULLET_SIZE, 
          color: {  red: 200, green: 0, blue: 0 },  
          velocity: velocity, 
          gravity: {  x: 0, y: -0.1, z: 0 }, 
          damping: 0 });

    // Play firing sounds 
    audioOptions.position = position;   
    Audio.playSound(fireSound, audioOptions);
}

function particleCollisionWithVoxel(particle, voxel, penetration) {
    Vec3.print('particleCollisionWithVoxel() ... penetration=', penetration);
    var HOLE_SIZE = 0.125;
    var particleProperties = Particles.getParticleProperties(particle);
    var position = particleProperties.position; 
    Particles.deleteParticle(particle);
    //  Make a hole in this voxel 
    Voxels.eraseVoxel(position.x, position.y, position.z, HOLE_SIZE);
    //audioOptions.position = position; 
    audioOptions.position = Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation()));
    Audio.playSound(impactSound, audioOptions); 
}

function update(deltaTime) {

    //  Check for mouseLook movement, update rotation 
       // rotate body yaw for yaw received from mouse
    var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.fromVec3( { x: 0, y: yawFromMouse, z: 0 } ));
    MyAvatar.orientation = newOrientation;
    yawFromMouse = 0;

    // apply pitch from mouse
    var newPitch = MyAvatar.headPitch + pitchFromMouse;
    MyAvatar.headPitch = newPitch;
    pitchFromMouse = 0;

    //  Check hydra controller for trigger press 

    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    // this is expected for hydras
    if (numberOfTriggers == 2 && controllersPerTrigger == 2) {
        for (var t = 0; t < numberOfTriggers; t++) {
            var shootABullet = false;
            var triggerValue = Controller.getTriggerValue(t);

            if (triggerPulled[t]) {
                // must release to at least 0.1
                if (triggerValue < 0.1) {
                    triggerPulled[t] = false; // unpulled
                }
            } else {
                // must pull to at least 0.9
                if (triggerValue > 0.9) {
                    triggerPulled[t] = true; // pulled
                    shootABullet = true;
                }
            }

            if (shootABullet) {
                
                var palmController = t * controllersPerTrigger; 
                var palmPosition = Controller.getSpatialControlPosition(palmController);

                var fingerTipController = palmController + 1; 
                var fingerTipPosition = Controller.getSpatialControlPosition(fingerTipController);
                
                var palmToFingerTipVector = 
                        {   x: (fingerTipPosition.x - palmPosition.x),
                            y: (fingerTipPosition.y - palmPosition.y),
                            z: (fingerTipPosition.z - palmPosition.z)  };
                                    
                // just off the front of the finger tip
                var position = { x: fingerTipPosition.x + palmToFingerTipVector.x/2, 
                                 y: fingerTipPosition.y + palmToFingerTipVector.y/2, 
                                 z: fingerTipPosition.z  + palmToFingerTipVector.z/2};   

                var linearVelocity = 25; 
                                    
                var velocity = { x: palmToFingerTipVector.x * linearVelocity,
                                 y: palmToFingerTipVector.y * linearVelocity,
                                 z: palmToFingerTipVector.z * linearVelocity };

                shootBullet(position, velocity);
            }
        }
    }
}

function mousePressEvent(event) {
    isMouseDown = true;
    lastX = event.x;
    lastY = event.y;
    audioOptions.position = Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation()));
    Audio.playSound(loadSound, audioOptions);
}

function mouseReleaseEvent(event) { 
    //  position 
    var DISTANCE_FROM_CAMERA = 2.0;
    var camera = Camera.getPosition();
    var forwardVector = Quat.getFront(Camera.getOrientation());
    var newPosition = Vec3.sum(camera, Vec3.multiply(forwardVector, DISTANCE_FROM_CAMERA));
    var velocity = Vec3.multiply(forwardVector, BULLET_VELOCITY);
    shootBullet(newPosition, velocity);
    isMouseDown = false;
}

function mouseMoveEvent(event) {
    if (isMouseDown) {
        var MOUSE_YAW_SCALE = -0.25;
        var MOUSE_PITCH_SCALE = -12.5;
        var FIXED_MOUSE_TIMESTEP = 0.016;
        yawFromMouse += ((event.x - lastX) * MOUSE_YAW_SCALE * FIXED_MOUSE_TIMESTEP);
        pitchFromMouse += ((event.y - lastY) * MOUSE_PITCH_SCALE * FIXED_MOUSE_TIMESTEP);
        lastX = event.x;
        lastY = event.y;
    }
}

function scriptEnding() {
    Overlays.deleteOverlay(reticle); 
}

Particles.particleCollisionWithVoxel.connect(particleCollisionWithVoxel);
Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);



