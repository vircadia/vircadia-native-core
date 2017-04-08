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
        scriptUUID,

        Entity,
        Player;

    function log(message) {
        print(APP_NAME + ": " + message);
    }

    Entity = (function () {
        // Persistence of playback via invisible entity.
        var entityID = null,
            userData,
            updateTimestampTimer = null,
            TIMESTAMP_UPDATE_INTERVAL = 2000,  // TODO: Final value.
            ENTITY_NAME = "Recording",
            ENTITY_DESCRIPTION = "Avatar recording to play back",
            ENTITIY_POSITION = { x: -16382, y: -16382, z: -16382 };  // Near but not right on corner boundary.

        function onUpdateTimestamp() {
            userData.timestamp = Date.now();
            Entities.editEntity(entityID, { userData: JSON.stringify(userData) });
        }

        function create(filename, scriptUUID) {
            // Create a new persistence entity (even if already have one but that should never occur).
            var properties;

            if (updateTimestampTimer !== null) {
                Script.clearInterval(updateTimestampTimer);  // Just in case.
            }

            userData = {
                recording: filename,
                scriptUUID: scriptUUID,
                timestamp: Date.now()
            };

            properties = {
                type: "Box",
                name: ENTITY_NAME,
                description: ENTITY_DESCRIPTION,
                position: ENTITIY_POSITION,
                visible: false,
                userData: JSON.stringify(userData)
            };

            entityID = Entities.addEntity(properties);

            updateTimestampTimer = Script.setInterval(onUpdateTimestamp, TIMESTAMP_UPDATE_INTERVAL);
        }

        function find() {
            // TODO
        }

        function destroy() {
            // Delete current persistence entity.
            if (entityID !== null) {  // Just in case.
                Entities.deleteEntity(entityID);
                entityID = null;
            }
            if (updateTimestampTimer !== null) {  // Just in case.
                Script.clearInterval(updateTimestampTimer);
            }
        }

        return {
            create: create,
            find: find,
            destroy: destroy
        };

    }());

    Player = (function () {
        // Recording playback functions.
        var isPlayingRecording = false,
            recordingFilename = "";

        function play(recording, position, orientation) {
            isPlayingRecording = true;
            recordingFilename = recording;

            log("Play new recording " + recordingFilename);

            Entity.create(recordingFilename, scriptUUID);

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

            Entity.destroy();

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
        if (message.player === scriptUUID) {
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
        scriptUUID = Agent.sessionUUID;

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
