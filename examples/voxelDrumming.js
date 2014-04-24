//
//  voxelDrumming.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Overlays, Controller, and Audio classes
//
//  It adds Hydra controller "fingertip on voxels" drumming
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Menu.addMenuItem({ 
                    menuName: "Developer > Hand Options", 
                    menuItemName: "Voxel Drumming", 
                    isCheckable: true, 
                    isChecked: false 
                });

var collisionCenter = new Array();
collisionCenter[0] = { x: 0, y: 0, z: 0};
collisionCenter[1] = { x: 0, y: 0, z: 0};

var collisionAge = new Array();
collisionAge[0] = 0;
collisionAge[1] = 0;

var collisionDuration = new Array();
collisionDuration[0] = 0;
collisionDuration[1] = 0;

var isColliding = new Array();
isColliding[0] = false;
isColliding[1] = false;

var highlightVoxel = Overlays.addOverlay("cube", 
                            { 
                                position: { x: 0, y: 0, z: 0}, 
                                size: 0,
                                color: { red: 0, green: 0, blue: 0 },
                                visible: false,
                                lineWidth: 3,
                                solid: false
                            });

var collisionBubble = new Array();
collisionBubble[0] = Overlays.addOverlay("sphere", 
                            { 
                                position: { x: 0, y: 0, z: 0}, 
                                size: 0,
                                color: { red: 0, green: 0, blue: 0 },
                                alpha: 0.5,
                                visible: false
                            });
collisionBubble[1] = Overlays.addOverlay("sphere", 
                            { 
                                position: { x: 0, y: 0, z: 0}, 
                                size: 0,
                                color: { red: 0, green: 0, blue: 0 },
                                alpha: 0.5,
                                visible: false
                            });

var audioOptions = new AudioInjectionOptions();
audioOptions.position = { x: MyAvatar.position.x, y: MyAvatar.position.y + 1, z: MyAvatar.position.z }; 
audioOptions.volume = 1;


function clamp(valueToClamp, minValue, maxValue) {
    return Math.max(minValue, Math.min(maxValue, valueToClamp));
}

function produceCollisionSound(deltaTime, palm, voxelDetail) {
    //  Collision between finger and a voxel plays sound

    var palmVelocity = Controller.getSpatialControlVelocity(palm * 2);
    var speed = Vec3.length(palmVelocity);
    var fingerTipPosition = Controller.getSpatialControlPosition(palm * 2 + 1);

    var LOWEST_FREQUENCY = 100.0;
    var HERTZ_PER_RGB = 3.0;
    var DECAY_PER_SAMPLE = 0.0005;
    var DURATION_MAX = 2.0;
    var MIN_VOLUME = 0.1;
    var volume = MIN_VOLUME + clamp(speed, 0.0, (1.0 - MIN_VOLUME));
    var duration = volume;

    collisionCenter[palm] = fingerTipPosition;
    collisionAge[palm] = deltaTime;
    collisionDuration[palm] = duration;

    var voxelBrightness = voxelDetail.red + voxelDetail.green + voxelDetail.blue;
    var frequency = LOWEST_FREQUENCY + (voxelBrightness * HERTZ_PER_RGB);

    audioOptions.position = fingerTipPosition; 
    Audio.startDrumSound(volume, frequency, DURATION_MAX, DECAY_PER_SAMPLE, audioOptions);
}

function update(deltaTime) {
    //  Voxel Drumming with fingertips if enabled
    if (Menu.isOptionChecked("Voxel Drumming")) {

        for (var palm = 0; palm < 2; palm++) {
            var fingerTipPosition = Controller.getSpatialControlPosition(palm * 2 + 1);

            var voxel = Voxels.getVoxelEnclosingPoint(fingerTipPosition);
            if (voxel.s > 0) {
                if (!isColliding[palm]) {
                    //  Collision has just started
                    isColliding[palm] = true;
                    produceCollisionSound(deltaTime, palm, voxel);
            
                    //  Set highlight voxel
                    Overlays.editOverlay(highlightVoxel, 
                                { 
                                    position: { x: voxel.x, y: voxel.y, z: voxel.z}, 
                                    size: voxel.s + 0.002,
                                    color: { red: voxel.red + 128, green: voxel.green + 128, blue: voxel.blue + 128 },
                                    visible: true
                                });
                }
            } else {
                if (isColliding[palm]) {
                    //  Collision has just ended
                    isColliding[palm] = false;
                    Overlays.editOverlay(highlightVoxel, { visible: false });
                }
            }
            
            if (collisionAge[palm] > 0) {
                collisionAge[palm] += deltaTime;
            }

            //  If hand/voxel collision has happened, render a little expanding sphere
            if (collisionAge[palm] > 0) {
                var opacity = clamp(1 - (collisionAge[palm] / collisionDuration[palm]), 0, 1);
                var size =  collisionAge[palm] * 0.25;

                Overlays.editOverlay(collisionBubble[palm], 
                            { 
                                position: { x: collisionCenter[palm].x, y: collisionCenter[palm].y, z: collisionCenter[palm].z}, 
                                size: size,
                                color: { red: 255, green: 0, blue: 0 },
                                alpha: 0.5 * opacity,
                                visible: true
                            });
        
                if (collisionAge[palm] > collisionDuration[palm]) {
                    collisionAge[palm] = 0;
                    Overlays.editOverlay(collisionBubble[palm], { visible: false });
                }
            }
        } // palm loop
    } // menu item check
}
Script.update.connect(update);

function scriptEnding() {
    Overlays.deleteOverlay(highlightVoxel);
    Overlays.deleteOverlay(collisionBubble[0]);
    Overlays.deleteOverlay(collisionBubble[1]);
    Menu.removeMenuItem("Developer > Hand Options","Voxel Drumming");
}
Script.scriptEnding.connect(scriptEnding);
