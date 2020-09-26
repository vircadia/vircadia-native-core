//  agentAPITest.js
//  scripts/developer/tests
//
//  Created by Thijs Wenker on 7/23/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var SOUND_DATA = { url: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/howard/sounds/piano1.wav") };

// getSound function from crowd-agent.js
function getSound(data, callback) { // callback(sound) when downloaded (which may be immediate).
    var sound = SoundCache.getSound(data.url);
    if (sound.downloaded) {
        return callback(sound);
    }
    function onDownloaded() {
        sound.ready.disconnect(onDownloaded);
        callback(sound);
    }
    sound.ready.connect(onDownloaded);
}


function agentAPITest() {
    console.warn('Agent.isAvatar =', Agent.isAvatar);

    Agent.isAvatar = true;
    console.warn('Agent.isAvatar =', Agent.isAvatar);

    console.warn('Agent.isListeningToAudioStream =', Agent.isListeningToAudioStream);

    Agent.isListeningToAudioStream = true;
    console.warn('Agent.isListeningToAudioStream =', Agent.isListeningToAudioStream);

    console.warn('Agent.isNoiseGateEnabled =', Agent.isNoiseGateEnabled);

    Agent.isNoiseGateEnabled = true;
    console.warn('Agent.isNoiseGateEnabled =', Agent.isNoiseGateEnabled);
    console.warn('Agent.lastReceivedAudioLoudness =', Agent.lastReceivedAudioLoudness);
    console.warn('Agent.sessionUUID =', Agent.sessionUUID);

    getSound(SOUND_DATA, function (sound) {
        console.warn('Agent.isPlayingAvatarSound =', Agent.isPlayingAvatarSound);
        Agent.playAvatarSound(sound);
        console.warn('Agent.isPlayingAvatarSound =', Agent.isPlayingAvatarSound);
    });
}

if (Script.context === "agent") {
    agentAPITest();
} else {
    console.error('This script should be run as agent script. EXITING.');
}
