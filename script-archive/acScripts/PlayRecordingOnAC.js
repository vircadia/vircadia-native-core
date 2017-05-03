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


var recordingFile = "http://your.recording.url";
var playFromCurrentLocation = true;
var loop = true;

// Set position here if playFromCurrentLocation is true
Avatar.position = { x:1, y: 1, z: 1 };
Avatar.orientation = Quat.fromPitchYawRollDegrees(0, 0, 0);
Avatar.scale = 1.0;
Agent.isAvatar = true;

// Disable the privacy bubble
Users.disableIgnoreRadius();

Recording.loadRecording(recordingFile, function(success) {
    if (success) {
        Script.update.connect(update);
    } else {
        print("Failed to load recording from " + recordingFile);
    }
});

count = 300; // This is necessary to wait for the audio mixer to connect
function update(event) {
    if (count > 0) {
        count--;
        return;
    }
    if (count == 0) {
        Recording.setPlayFromCurrentLocation(playFromCurrentLocation);
        Recording.setPlayerLoop(loop);
        Recording.setPlayerUseDisplayName(true);
        Recording.setPlayerUseAttachments(true);
        Recording.setPlayerUseHeadModel(false);
        Recording.setPlayerUseSkeletonModel(true);
        Recording.startPlaying();
        Vec3.print("Playing from ", Avatar.position);
        count--;
    }

    if (!Recording.isPlaying()) {
        Script.update.disconnect(update);
    }
}
