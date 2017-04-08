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
        PLAYER_COMMAND_STOP = "stop",
        heartbeatTimer = null,
        HEARTBEAT_INTERVAL = 3000,  // TODO: Final value.
        sessionUUID,

        Entity,
        Player;

    function log(message) {
        print(APP_NAME + ": " + message);
    }

    Entity = (function () {
        // Persistence of playback.

        // TODO
    }());

    Player = (function () {
        // Recording playback functions.
        var isPlayingRecording = false,
            recordingFilename = "";

        function play(recording, position, orientation) {
            isPlayingRecording = true;
            recordingFilename = recording;

            log("Play recording " + recordingFilename);

            Agent.isAvatar = true;
            Avatar.position = position;
            Avatar.orientation = orientation;

            Recording.loadRecording(recordingFilename);
            Recording.setPlayFromCurrentLocation(true);
            Recording.setPlayerUseDisplayName(true);
            Recording.setPlayerUseHeadModel(false);
            Recording.setPlayerUseAttachments(true);
            Recording.setPlayerLoop(true);
            Recording.setPlayerUseSkeletonModel(true);

            Recording.setPlayerTime(0.0);
            Recording.startPlaying();
        }

        function autoPlay() {
            // TODO: Automatically play a persisted recording, if any.
        }

        function stop() {
            log("Stop playing " + recordingFilename);

            if (Recording.isPlaying()) {
                Recording.stopPlaying();
            }
            isPlayingRecording = false;
            recordingFilename = "";
        }

        function isPlaying() {
            return isPlayingRecording;
        }

        function recording() {
            return recordingFilename;
        }

        function setUp() {
            // Nothing to do.
        }

        function tearDown() {
            // Nothing to do.
        }

        return {
            autoPlay: autoPlay,
            play: play,
            stop: stop,
            isPlaying: isPlaying,
            recording: recording,
            setUp: setUp,
            tearDown: tearDown
        };
    }());

    function sendHeartbeat() {
        Messages.sendMessage(HIFI_RECORDER_CHANNEL, JSON.stringify({
            playing: Player.isPlaying(),
            recording: Player.recording()
        }));
        heartbeatTimer = Script.setTimeout(sendHeartbeat, HEARTBEAT_INTERVAL);
    }

    function cancelHeartbeat() {
        if (heartbeatTimer) {
            Script.clearTimeout(heartbeatTimer);
            heartbeatTimer = null;
        }
    }

    function onMessageReceived(channel, message, sender) {
        message = JSON.parse(message);
        if (message.player === sessionUUID) {
            switch (message.command) {
            case PLAYER_COMMAND_PLAY:
                if (!Player.isPlaying()) {
                    Player.play(message.recording, message.position, message.orientation);
                } else {
                    log("Didn't start playing " + message.recording + " because already playing " + Player.recording());
                }
                sendHeartbeat();
                break;
            case PLAYER_COMMAND_STOP:
                Player.stop();
                Player.autoPlay();
                sendHeartbeat();
                break;
            }
        }
    }

    function setUp() {
        sessionUUID = Agent.sessionUUID;

        Player.setUp();

        Messages.messageReceived.connect(onMessageReceived);
        Messages.subscribe(HIFI_PLAYER_CHANNEL);

        Player.autoPlay();
        sendHeartbeat();
    }

    function tearDown() {
        cancelHeartbeat();

        Messages.messageReceived.disconnect(onMessageReceived);
        Messages.unsubscribe(HIFI_PLAYER_CHANNEL);

        Player.stop();
        Player.tearDown();
    }

    setUp();
    Script.scriptEnding.connect(tearDown);

}());
