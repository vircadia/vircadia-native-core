//
//  drumStick.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  This example musical instrument script plays 'air drums' when you move your hands downward
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

function length(v) {
    return Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}


function printVector(v) {
    print(v.x + ", " + v.y + ", " + v.z + "\n");
}

function vMinus(a, b) { 
    var rval = { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
    return rval;
}

//  First, load two percussion sounds to be used on the sticks

var drum1 = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Drums/RackTomHi.raw");
var drum2 = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Drums/RackTomLo.raw");

//  State Machine:
//  0 = not triggered 
//  1 = triggered, waiting to stop to play sound
var state = new Array();
state[0] = 0; 
state[1] = 0; 
var strokeSpeed = new Array();
strokeSpeed[0] = 0.0; 
strokeSpeed[1] = 0.0; 

function checkSticks(deltaTime) {
    for (var palm = 0; palm < 2; palm++) {
        var handPose = (palm == 0) ? MyAvatar.leftHandPose : MyAvatar.rightHandPose;
        var palmVelocity = handPose.velocity;
        var speed = length(palmVelocity);
        
        const TRIGGER_SPEED = 0.30;            //    Lower this value to let you 'drum' more gently
        const STOP_SPEED = 0.01;             //    Speed below which a sound will trigger 
        const GAIN = 0.5;                    //    Loudness compared to stick velocity
        const AVERAGING = 0.2;                //    How far back to sample trailing velocity

        //   Measure trailing average stroke speed to ultimately set volume
        strokeSpeed[palm] = (1.0 - AVERAGING) * strokeSpeed[palm] + AVERAGING * (speed * GAIN);

        if (state[palm] == 0) {
            //  Waiting for downward speed to indicate stroke
            if ((palmVelocity.y < 0.0) && (strokeSpeed[palm] > TRIGGER_SPEED)) {
                state[palm] = 1;
            }
        } else if (state[palm] == 1) {
            //   Waiting for change in velocity direction or slowing to trigger drum sound
            if ((palmVelocity.y > 0.0) || (speed < STOP_SPEED)) {
                state[palm] = 0;
        
                var options = { position: handPose.translation };
        
                if (strokeSpeed[palm] > 1.0) { strokeSpeed[palm] = 1.0; }
                options.volume = strokeSpeed[palm];

                if (palm == 0) {
                    Audio.playSound(drum1, options);
                } else {
                    Audio.playSound(drum2, options);
                }
            }
        }
    }
}

// Connect a call back that happens every frame
Script.update.connect(checkSticks);