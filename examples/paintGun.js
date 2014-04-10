//
//  paintGun.js
//  examples
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// initialize our triggers
var triggerPulled = new Array();
var numberOfTriggers = Controller.getNumberOfTriggers();
for (t = 0; t < numberOfTriggers; t++) {
    triggerPulled[t] = false;
}

function checkController(deltaTime) {
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
                         " function collisionWithVoxel(voxel, collision) { " +
                         "   print('collisionWithVoxel(voxel)... '); " +
                         "   Vec3.print('penetration=', collision.penetration); " +
                         "   Vec3.print('contactPoint=', collision.contactPoint); " +
                         "   print('myID=' + Particle.getID() + '\\n'); " +
                         "   var voxelColor = { red: voxel.red, green: voxel.green, blue: voxel.blue };" +
                         "   var voxelAt = { x: voxel.x, y: voxel.y, z: voxel.z };" +
                         "   var voxelScale = voxel.s;" +
                         "   print('voxelColor=' + voxelColor.red + ', ' + voxelColor.green + ', ' + voxelColor.blue + '\\n'); " +
                         "   var myColor = Particle.getColor();" +
                         "   print('myColor=' + myColor.red + ', ' + myColor.green + ', ' + myColor.blue + '\\n'); " +
                         "   Particle.setColor(voxelColor); " +
                         "   Voxels.setVoxel(voxelAt.x, voxelAt.y, voxelAt.z, voxelScale, 255, 255, 0);  " +
                         "   print('Voxels.setVoxel(' + voxelAt.x + ', ' + voxelAt.y + ', ' + voxelAt.z + ', ' + voxelScale + ')... \\n'); " +
                         " } " +
                         " Particle.collisionWithVoxel.connect(collisionWithVoxel); ";
                
                Particles.addParticle(
                    { position: position, 
                      radius: 0.01, 
                      color: {  red: 128, green: 128, blue: 128 },  
                      velocity: velocity, 
                      gravity: {  x: 0, y: -0.1, z: 0 }, 
                      damping: 0, 
                      script: script }
                );
            }
        }
    }
}

 
// register the call back so it fires before each data send
Script.update.connect(checkController);
