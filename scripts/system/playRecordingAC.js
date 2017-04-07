"use strict";

//
//  playRecordingAC.js
//
//  Created by David Rowe on 7 Apr 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    var APP_NAME = "PLAYBACK",
        HIFI_RECORDER_CHANNEL = "HiFi-Recorder-Channel",
        HIFI_PLAYER_CHANNEL = "HiFi-Player-Channel",
        PLAYER_COMMAND_PLAY = "play",
        heartbeatTimer,
        HEARTBEAT_INTERVAL = 3000,  // TODO: Final value.
        sessionUUID,
        isPlaying = false,  // TODO: Just use recording value instead?
        recording = "";

    function log(message) {
        print(APP_NAME + ": " + message);
    }

    function updateRecorder() {
        Messages.sendMessage(HIFI_RECORDER_CHANNEL, JSON.stringify({
            playing: isPlaying,
            recording: recording
        }));
    }

    function onHeartbeatTimer() {
        updateRecorder();
    }

    function onMessageReceived(channel, message, sender) {
        message = JSON.parse(message);
        if (message.player === sessionUUID) {
            switch (message.command) {
            case PLAYER_COMMAND_PLAY:
                isPlaying = true;
                recording = message.recording;

                log("Play recording " + recording);

                Agent.isAvatar = true;
                Avatar.position = message.position;
                Avatar.orientation = message.orientation;

                Recording.loadRecording(recording);
                Recording.setPlayFromCurrentLocation(true);
                Recording.setPlayerUseDisplayName(true);
                Recording.setPlayerUseHeadModel(false);
                Recording.setPlayerUseAttachments(true);
                Recording.setPlayerLoop(true);
                Recording.setPlayerUseSkeletonModel(true);

                Recording.setPlayerTime(0.0);
                Recording.startPlaying();

                break;
            }

            updateRecorder();
        }
    }

    function setUp() {
        sessionUUID = Agent.sessionUUID;

        Messages.messageReceived.connect(onMessageReceived);
        Messages.subscribe(HIFI_PLAYER_CHANNEL);

        heartbeatTimer = Script.setInterval(onHeartbeatTimer, HEARTBEAT_INTERVAL);
    }

    function tearDown() {
        Script.clearInterval(heartbeatTimer);

        Messages.messageReceived.disconnect(onMessageReceived);
        Messages.unsubscribe(HIFI_PLAYER_CHANNEL);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);

}());
