//
// PlayRecordingOnAC.js
//  examples
//
//  Created by Clément Brisset on 8/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var filename = "http://your.recording.url";
var playFromCurrentLocation = true;
var loop = true;

Avatar.skeletonModelURL = "https://hifi-public.s3.amazonaws.com/marketplace/contents/e21c0b95-e502-4d15-8c41-ea2fc40f1125/3585ddf674869a67d31d5964f7b52de1.fst?1427169998";

// Set position here if playFromCurrentLocation is true
Avatar.position = { x:1, y: 1, z: 1 };
Avatar.orientation = Quat.fromPitchYawRollDegrees(0, 0, 0);
Avatar.scale = 1.0;

Agent.isAvatar = true;

Avatar.loadRecording(filename);

count = 300; // This is necessary to wait for the audio mixer to connect
function update(event) {
    if (count > 0) {
        count--;
        return;
    }
    if (count == 0) {
        Avatar.setPlayFromCurrentLocation(playFromCurrentLocation);
        Avatar.setPlayerLoop(loop);
        Avatar.setPlayerUseDisplayName(true);
        Avatar.setPlayerUseAttachments(true);
        Avatar.setPlayerUseHeadModel(false);
        Avatar.setPlayerUseSkeletonModel(true);
        Avatar.startPlaying();
        Avatar.play();
        Vec3.print("Playing from ", Avatar.position);
        
        count--;
    }
    
    if (Avatar.isPlaying()) {
        Avatar.play();
    } else {
        Script.update.disconnect(update);
    }
}

Script.update.connect(update);
