//
//  grenadeLauncher.js
//  examples
//  Created by Ben Arnold on 7/11/14.
//  This is a modified version of gun.js by Brad Hefta-Gaub.
//
//  Copyright 2013 High Fidelity, Inc.
//
//  This is an example script that turns the hydra controllers and mouse into a entity gun.
//  It reads the controller, watches for trigger pulls, and launches entities.
//  When entities collide with voxels they blow big holes out of the voxels. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

function getRandomFloat(min, max) {
    return Math.random() * (max - min) + min;
}

var lastX = 0;
var lastY = 0;
var yawFromMouse = 0;
var pitchFromMouse = 0;
var isMouseDown = false; 

var BULLET_VELOCITY = 3.0;
var MIN_THROWER_DELAY = 1000;
var MAX_THROWER_DELAY = 1000;
var LEFT_BUTTON_1 = 1;
var LEFT_BUTTON_3 = 3;
var RELOAD_INTERVAL = 5;

var showScore = false;

// Load some sound to use for loading and firing 
var fireSound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guns/GUN-SHOT2.raw");
var loadSound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guns/Gun_Reload_Weapon22.raw");
var impactSound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Guns/BulletImpact2.raw");
var targetHitSound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/hit.raw");
var targetLaunchSound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/shoot.raw");

var gunModel = "http://public.highfidelity.io/models/attachments/HaloGun.fst";

var audioOptions {
  volume: 0.9
}

var shotsFired = 0;

var shotTime = new Date(); 

// initialize our triggers
var triggerPulled = new Array();
var numberOfTriggers = Controller.getNumberOfTriggers();
for (t = 0; t < numberOfTriggers; t++) {
    triggerPulled[t] = false;
}

var isLaunchButtonPressed = false; 

var score = 0; 

//  Create a reticle image in center of screen 
var screenSize = Controller.getViewportDimensions();
var reticle = Overlays.addOverlay("image", {
                    x: screenSize.x / 2 - 16,
                    y: screenSize.y / 2 - 16,
                    width: 32,
                    height: 32,
                    imageURL: HIFI_PUBLIC_BUCKET + "images/reticle.png",
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1
                });

if (showScore) {
    var text = Overlays.addOverlay("text", {
                    x: screenSize.x / 2 - 100,
                    y: screenSize.y / 2 - 50,
                    width: 150,
                    height: 50,
                    color: { red: 0, green: 0, blue: 0},
                    textColor: { red: 255, green: 0, blue: 0},
                    topMargin: 4,
                    leftMargin: 4,
                    text: "Score: " + score
                });
}

function printVector(string, vector) {
    print(string + " " + vector.x + ", " + vector.y + ", " + vector.z);
}

function shootBullet(position, velocity) {
    var BULLET_SIZE = 0.1;
    var BULLET_GRAVITY = -3.0;
    //Creates a grenade with a reasonable lifetime so that one is less likely to accidentally blow up
    //far away voxels
    Entities.addEntity(
        { type: "Sphere",
          position: position, 
          collisionsWillMove: true,
          dimensions: { x: BULLET_SIZE, y: BULLET_SIZE, z: BULLET_SIZE },
          color: {  red: 10, green: 10, blue: 10 },  
          velocity: velocity, 
          gravity: {  x: 0, y: BULLET_GRAVITY, z: 0 }, 
          lifetime: 10.0,
          damping: 0 });

    // Play firing sounds 
    audioOptions.position = position;   
    Audio.playSound(fireSound, audioOptions);
    shotsFired++;
    if ((shotsFired % RELOAD_INTERVAL) == 0) {
        Audio.playSound(loadSound, audioOptions);
    }
}

function shootTarget() {
    var TARGET_SIZE = 0.25;
    var TARGET_GRAVITY = -0.6;
    var TARGET_UP_VELOCITY = 3.0;
    var TARGET_FWD_VELOCITY = 5.0;
    var DISTANCE_TO_LAUNCH_FROM = 3.0;
    var camera = Camera.getPosition();
    //printVector("camera", camera);
    var targetDirection = Quat.angleAxis(getRandomFloat(-20.0, 20.0), { x:0, y:1, z:0 });
    targetDirection = Quat.multiply(Camera.getOrientation(), targetDirection);
    var forwardVector = Quat.getFront(targetDirection);
    //printVector("forwardVector", forwardVector);
    var newPosition = Vec3.sum(camera, Vec3.multiply(forwardVector, DISTANCE_TO_LAUNCH_FROM));
    //printVector("newPosition", newPosition);
    var velocity = Vec3.multiply(forwardVector, TARGET_FWD_VELOCITY);
    velocity.y += TARGET_UP_VELOCITY;
    //printVector("velocity", velocity);
    
    Entities.addEntity(
        { type: "Sphere",
          position: newPosition, 
          collisionsWillMove: true,
          dimensions: { x: TARGET_SIZE, y: TARGET_SIZE, z: TARGET_SIZE },
          color: {  red: 0, green: 200, blue: 200 },  
          velocity: velocity, 
          gravity: {  x: 0, y: TARGET_GRAVITY, z: 0 }, 
          lifetime: 1000.0,
          damping: 0.99 });

    // Record start time 
    shotTime = new Date();

    // Play target shoot sound
    audioOptions.position = newPosition;   
    Audio.playSound(targetLaunchSound, audioOptions);
}



function entityCollisionWithVoxel(entity, voxel, collision) {

print("entityCollisionWithVoxel....");

    var VOXEL_SIZE = 0.5;
    // Don't make this big. I mean it.
    var CRATER_RADIUS = 5;
    var entityProperties = Entities.getEntityProperties(entity);
    var position = entityProperties.position; 
    Entities.deleteEntity(entity);
    
    audioOptions.position = collision.contactPoint;
    Audio.playSound(impactSound, audioOptions); 
    
    //  Make a crater
    var center = collision.contactPoint;
    var RADIUS = CRATER_RADIUS * VOXEL_SIZE;
    var RADIUS2 = RADIUS * RADIUS;
    var distance2;
    var dx;
    var dy;
    var dz;
    for (var x = center.x - RADIUS; x <= center.x + RADIUS; x += VOXEL_SIZE) {
        for (var y = center.y - RADIUS; y <= center.y + RADIUS; y += VOXEL_SIZE) {
            for (var z = center.z - RADIUS; z <= center.z + RADIUS; z += VOXEL_SIZE) {
                dx = Math.abs(x - center.x);
                dy = Math.abs(y - center.y);
                dz = Math.abs(z - center.z);
                distance2 = dx * dx + dy * dy + dz * dz;
                if (distance2 <= RADIUS2) {
                    Voxels.eraseVoxel(x, y, z, VOXEL_SIZE);
                }
            }
        }
    }
}

function entityCollisionWithEntity(entity1, entity2, collision) {
    score++;
    if (showScore) {
        Overlays.editOverlay(text, { text: "Score: " + score } );
    }

    //  Record shot time 
    var endTime = new Date(); 
    var msecs = endTime.valueOf() - shotTime.valueOf();
    //print("hit, msecs = " + msecs);
    //Vec3.print("penetration = ", collision.penetration);
    //Vec3.print("contactPoint = ", collision.contactPoint);
    Entities.deleteEntity(entity1);
    Entities.deleteEntity(entity2);
    // play the sound near the camera so the shooter can hear it
    audioOptions.position = Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation()));   
    Audio.playSound(targetHitSound, audioOptions);
}

function keyPressEvent(event) {
    // if our tools are off, then don't do anything
    if (event.text == "t") {
        var time = MIN_THROWER_DELAY + Math.random() * MAX_THROWER_DELAY;
        Script.setTimeout(shootTarget, time); 
    } else if (event.text == ".") {
        shootFromMouse();
    } else if (event.text == "r") {
        playLoadSound();
    }
}

function playLoadSound() {
    audioOptions.position = Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation())); 
    Audio.playSound(loadSound, audioOptions);
}

//MyAvatar.attach(gunModel, "RightHand", {x: -0.02, y: -.14, z: 0.07}, Quat.fromPitchYawRollDegrees(-70, -151, 72), 0.20);
MyAvatar.attach(gunModel, "LeftHand", {x: -0.02, y: -.14, z: 0.07}, Quat.fromPitchYawRollDegrees(-70, -151, 72), 0.20);

//  Give a bit of time to load before playing sound
Script.setTimeout(playLoadSound, 2000); 

function update(deltaTime) {
 
    // Check for mouseLook movement, update rotation 
    // rotate body yaw for yaw received from controller or mouse
    var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.fromVec3Radians( { x: 0, y: yawFromMouse, z: 0 } ));
    MyAvatar.orientation = newOrientation;
    yawFromMouse = 0;

    // apply pitch from controller or mouse
    var newPitch = MyAvatar.headPitch + pitchFromMouse;
    MyAvatar.headPitch = newPitch;
    pitchFromMouse = 0;

    //  Check hydra controller for launch button press 
    if (!isLaunchButtonPressed && Controller.isButtonPressed(LEFT_BUTTON_3)) {
        isLaunchButtonPressed = true; 
        var time = MIN_THROWER_DELAY + Math.random() * MAX_THROWER_DELAY;
        Script.setTimeout(shootTarget, time);
    } else if (isLaunchButtonPressed && !Controller.isButtonPressed(LEFT_BUTTON_3)) {
        isLaunchButtonPressed = false;   
        
    }

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
    //audioOptions.position = Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation()));
    //Audio.playSound(loadSound, audioOptions);
}

function shootFromMouse() {
    var DISTANCE_FROM_CAMERA = 2.0;
    var camera = Camera.getPosition();
    var forwardVector = Quat.getFront(Camera.getOrientation());
    var newPosition = Vec3.sum(camera, Vec3.multiply(forwardVector, DISTANCE_FROM_CAMERA));
    var velocity = Vec3.multiply(forwardVector, BULLET_VELOCITY);
    shootBullet(newPosition, velocity);
}

function mouseReleaseEvent(event) { 
    //  position 
    isMouseDown = false;
}

function mouseMoveEvent(event) {
    //Move the camera if LEFT_BUTTON_1 is pressed
    if (Controller.isButtonPressed(LEFT_BUTTON_1)) {
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
    Overlays.deleteOverlay(text);
    MyAvatar.detachOne(gunModel);
}

Entities.entityCollisionWithVoxel.connect(entityCollisionWithVoxel);
Entities.entityCollisionWithEntity.connect(entityCollisionWithEntity);
Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.keyPressEvent.connect(keyPressEvent);



