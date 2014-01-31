//
//  gun.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that turns the hydra controllers into a particle gun.
//  It reads the controller, watches for trigger pulls, and launches particles.
//  The particles it creates have a script that when they collide with Voxels, the
//  particle will change it's color to match the voxel it hits, and then delete the
//  voxel.
//
//

// initialize our triggers
var triggerPulled = new Array();
var numberOfTriggers = Controller.getNumberOfTriggers();
for (t = 0; t < numberOfTriggers; t++) {
    triggerPulled[t] = false;
}

function checkController() {
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

                // This is the script for the particles that this gun shoots.
                var script = 
                         " function collisionWithVoxel(voxel) { " +
                         "   print('collisionWithVoxel(voxel)... '); " +
                         "   print('myID=' + Particle.getID() + '\\n'); " +
                         "   var voxelColor = { red: voxel.red, green: voxel.green, blue: voxel.blue };" +
                         "   var voxelAt = { x: voxel.x, y: voxel.y, z: voxel.z };" +
                         "   var voxelScale = voxel.s;" +
                         "   print('voxelColor=' + voxel.red + ', ' + voxel.green + ', ' + voxel.blue + '\\n'); " +
                         "   var myColor = Particle.getColor();" +
                         "   print('myColor=' + myColor.red + ', ' + myColor.green + ', ' + myColor.blue + '\\n'); " +
                         "   Particle.setColor(voxelColor); " +
                         "   Voxels.eraseVoxel(voxelAt.x, voxelAt.y, voxelAt.z, voxelScale);  " +
                         "   print('Voxels.eraseVoxel(' + voxelAt.x + ', ' + voxelAt.y + ', ' + voxelAt.z + ', ' + voxelScale + ')... \\n'); " +
                         " } " +
                         " Particle.collisionWithVoxel.connect(collisionWithVoxel); ";
                
                Particles.addParticle(
                    { position: position, 
                      radius: 0.05, 
                      color: {  red: 128, green: 128, blue: 128 },  
                      velocity: velocity, 
                      gravity: {  x: 0, y: -0.1, z: 0 }, 
                      damping: 0, 
                      script: script });
            }
        }
    }
}


// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(checkController);
