//
//  triggeredRecordingOnAC.js
//  examples/acScripts
//
//  Created by Thijs Wenker on 12/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is the triggered rocording script used in the winterSmashUp target practice game.
//  Change the CLIP_URL to your asset,
//  the RECORDING_CHANNEL and RECORDING_CHANNEL_MESSAGE are used to trigger it i.e.:
//  Messages.sendMessage("PlayBackOnAssignment", "BowShootingGameWelcome");
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

const CLIP_URL = "atp:3fbe82f2153c443f12f9a2b14ce2d7fa2fff81977263746d9e0885ea5aabed62.hfr";

const RECORDING_CHANNEL = 'PlayBackOnAssignment';
const RECORDING_CHANNEL_MESSAGE = 'BowShootingGameWelcome'; // For each different assignment add a different message here.
const PLAY_FROM_CURRENT_LOCATION = true;
const USE_DISPLAY_NAME = true;
const USE_ATTACHMENTS = true;
const USE_AVATAR_MODEL = true;
const AUDIO_OFFSET = 0.0;
const STARTING_TIME = 0.0;
const COOLDOWN_PERIOD = 0; // The period in ms that no animations can be played after one has been played already

var isPlaying = false;
var isPlayable = true;

var playRecording = function() {
    if (!isPlayable || isPlaying) {
        return;
    }
    Agent.isAvatar = true;
    Recording.setPlayFromCurrentLocation(PLAY_FROM_CURRENT_LOCATION);
    Recording.setPlayerUseDisplayName(USE_DISPLAY_NAME);
    Recording.setPlayerUseAttachments(USE_ATTACHMENTS);
    Recording.setPlayerUseHeadModel(false);
    Recording.setPlayerUseSkeletonModel(USE_AVATAR_MODEL);
    Recording.setPlayerLoop(false);
    Recording.setPlayerTime(STARTING_TIME);
    Recording.setPlayerAudioOffset(AUDIO_OFFSET);
    Recording.loadRecording(CLIP_URL);
    Recording.startPlaying();
    isPlaying = true;
    isPlayable = false; // Set this true again after the cooldown period
};

Script.update.connect(function(deltaTime) {
    if (isPlaying && !Recording.isPlaying()) {
        print('Reached the end of the recording. Resetting.');
        isPlaying = false;
        Agent.isAvatar = false;
        if (COOLDOWN_PERIOD === 0) {
            isPlayable = true;
            return;
        }
        Script.setTimeout(function () {
            isPlayable;
        }, COOLDOWN_PERIOD);
    }
});

Messages.subscribe(RECORDING_CHANNEL);

Messages.messageReceived.connect(function (channel, message, senderID) {
    if (channel === RECORDING_CHANNEL && message === RECORDING_CHANNEL_MESSAGE) {
        playRecording();
    }
});
