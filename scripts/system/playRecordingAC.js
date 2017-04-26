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
        HEARTBEAT_INTERVAL = 3000,
        TIMESTAMP_UPDATE_INTERVAL = 2500,
        AUTOPLAY_SEARCH_INTERVAL = 5000,
        AUTOPLAY_ERROR_INTERVAL = 30000,  // 30s
        scriptUUID,

        Entity,
        Player;

    function log(message) {
        print(APP_NAME + " " + scriptUUID + ": " + message);
    }

    Entity = (function () {
        // Persistence of playback via invisible entity.
        var entityID = null,
            userData,
            updateTimestampTimer = null,
            ENTITY_NAME = "Recording",
            ENTITY_DESCRIPTION = "Avatar recording to play back",
            ENTITIY_POSITION = { x: -16382, y: -16382, z: -16382 },  // Near but not right on domain corner.
            ENTITY_SEARCH_DELTA = { x: 1, y: 1, z: 1 },  // Allow for position imprecision.
            SEARCH_IDLE = 0,
            SEARCH_SEARCHING = 1,
            SEARCH_CLAIMING = 2,
            SEARCH_PAUSING = 3,
            searchState = SEARCH_IDLE,
            otherPlayersPlaying,
            otherPlayersPlayingCounts,
            pauseCount;

        function onUpdateTimestamp() {
            userData.timestamp = Date.now();
            Entities.editEntity(entityID, { userData: JSON.stringify(userData) });
            EntityViewer.queryOctree();  // Keep up to date ready for find().
        }

        function id() {
            return entityID;
        }

        function randomInt(min, max) {
            return Math.floor(Math.random() * (max - min + 1)) + min;
        }


        function onMessageReceived(channel, message, sender) {
            var index;

            if (sender !== scriptUUID) {
                message = JSON.parse(message);
                index = otherPlayersPlaying.indexOf(message.entity);
                if (index !== -1) {
                    otherPlayersPlayingCounts[index] += 1;
                } else {
                    otherPlayersPlaying.push(message.entity);
                    otherPlayersPlayingCounts.push(1);
                }
            }
        }

        function create(filename, position, orientation) {
            // Create a new persistence entity (even if already have one but that should never occur).
            var properties;

            log("Create recording " + filename);

            if (updateTimestampTimer !== null) {
                Script.clearInterval(updateTimestampTimer);  // Just in case.
            }

            searchState = SEARCH_IDLE;

            userData = {
                recording: filename,
                position: position,
                orientation: orientation,
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
            if (!Uuid.isNull(entityID)) {
                updateTimestampTimer = Script.setInterval(onUpdateTimestamp, TIMESTAMP_UPDATE_INTERVAL);
                return true;
            }

            return false;
        }

        function find() {
            // Find a persistence entity that isn't being played.
            // AC scripts may simultaneously find the same entity to play because octree updates aren't instantaneously 
            // propagated. Additionally, messages are not instantaneous. To address these issues the "find" progresses through 
            // the following search states:
            // - SEARCH_IDLE
            //      No searching is being performed.
            //      Return null.
            // - SEARCH_SEARCHING
            //      Looking for an entity that isn't being played (as reported in entity properties) and isn't being claimed (as
            //      reported by heartbeat messages. If one is found transition to SEARCH_CLAIMING and start reporting the entity
            //      in heartbeat messages.
            //      Return null.
            // - SEARCH_CLAIMING
            //      An entity has been found and is reported in heartbeat messages but isn't being played yet. After a period of
            //      time, if no other players report they're playing that entity then transition to SEARCH_IDLE otherwise 
            //      transition to SEARCH_PAUSING.
            //      If transitioning to SEARCH_IDLE update the entity userData and return the recording details, otherwise 
            //      return null;
            // - SEARCH_PAUSING
            //      Two or more players have tried to play the same entity. Wait for a randomized period of time before 
            //      transitioning to SEARCH_SEARCHING.
            //      Return null.
            // One of these states is processed each find() call.
            var entityIDs,
                index,
                found = false,
                properties,
                numberOfClaims,
                result = null;

            switch (searchState) {

            case SEARCH_IDLE:
                log("Start searching");
                otherPlayersPlaying = [];
                otherPlayersPlayingCounts = [];
                Messages.subscribe(HIFI_RECORDER_CHANNEL);
                Messages.messageReceived.connect(onMessageReceived);
                searchState = SEARCH_SEARCHING;
                break;

            case SEARCH_SEARCHING:
                // Find an entity that isn't being played or claimed.
                entityIDs = Entities.findEntities(ENTITIY_POSITION, ENTITY_SEARCH_DELTA.x);
                if (entityIDs.length > 0) {
                    index = -1;
                    while (!found && index < entityIDs.length - 1) {
                        index += 1;
                        if (otherPlayersPlaying.indexOf(entityIDs[index]) === -1) {
                            properties = Entities.getEntityProperties(entityIDs[index], ["name", "userData"]);
                            userData = JSON.parse(properties.userData);
                            found = properties.name === ENTITY_NAME && userData.recording !== undefined;
                        }
                    }
                }

                // Claim entity if found.
                if (found) {
                    log("Claim entity " + entityIDs[index]);
                    entityID = entityIDs[index];
                    searchState = SEARCH_CLAIMING;
                }
                break;

            case SEARCH_CLAIMING:
                // How many other players are claiming (or playing) this entity?
                index = otherPlayersPlaying.indexOf(entityID);
                numberOfClaims = index !== -1 ? otherPlayersPlayingCounts[index] : 0;

                // Have found an entity to play if no other players are also claiming it.
                if (numberOfClaims === 0) {
                    log("Complete claim " + entityID);
                    Messages.messageReceived.disconnect(onMessageReceived);
                    Messages.unsubscribe(HIFI_RECORDER_CHANNEL);
                    searchState = SEARCH_IDLE;
                    userData.scriptUUID = scriptUUID;
                    userData.timestamp = Date.now();
                    Entities.editEntity(entityID, { userData: JSON.stringify(userData) });
                    updateTimestampTimer = Script.setInterval(onUpdateTimestamp, TIMESTAMP_UPDATE_INTERVAL);
                    result = { recording: userData.recording, position: userData.position, orientation: userData.orientation };
                    break;
                }

                // Otherwise back off for a bit before resuming search.
                log("Release claim " + entityID + " and pause searching");
                entityID = null;
                pauseCount = randomInt(0, otherPlayersPlaying.length);
                searchState = SEARCH_PAUSING;
                break;

            case SEARCH_PAUSING:
                // Resume searching if have paused long enough.
                pauseCount -= 1;
                if (pauseCount < 0) {
                    log("Resume searching");
                    otherPlayersPlaying = [];
                    otherPlayersPlayingCounts = [];
                    searchState = SEARCH_SEARCHING;
                }
                break;
            }

            EntityViewer.queryOctree();
            return result;
        }

        function destroy() {
            // Delete current persistence entity.
            if (entityID !== null) {  // Just in case.
                Entities.deleteEntity(entityID);
                entityID = null;
                searchState = SEARCH_IDLE;
            }
            if (updateTimestampTimer !== null) {  // Just in case.
                Script.clearInterval(updateTimestampTimer);
            }
        }

        function setUp() {
            // Set up EntityViewer so that can do Entities.findEntities().
            // Position and orientation set so that viewing entities only in corner of domain.
            var entityViewerPosition = Vec3.sum(ENTITIY_POSITION, ENTITY_SEARCH_DELTA);
            EntityViewer.setPosition(entityViewerPosition);
            EntityViewer.setOrientation(Quat.lookAtSimple(entityViewerPosition, ENTITIY_POSITION));
            EntityViewer.queryOctree();
        }

        function tearDown() {
            // Nothing to do.
        }

        return {
            id: id,
            create: create,
            find: find,
            destroy: destroy,
            setUp: setUp,
            tearDown: tearDown
        };
    }());

    Player = (function () {
        // Recording playback functions.
        var isPlayingRecording = false,
            recordingFilename = "",
            autoPlayTimer = null,

            playRecording;

        function play(recording, position, orientation) {
            if (Entity.create(recording, position, orientation)) {
                log("Play new recording " + recordingFilename);
                isPlayingRecording = true;
                recordingFilename = recording;
                playRecording(recordingFilename, position, orientation);
            } else {
                log("Could not create entity to play new recording " + recordingFilename);
            }
        }

        function autoPlay() {
            var recording,
                AUTOPLAY_SEARCH_DELTA = 1000;

            // Random delay to help reduce collisions between AC scripts.
            Script.setTimeout(function () {
                recording = Entity.find();
                if (recording) {
                    log("Play persisted recording " + recordingFilename);
                    playRecording(recording.recording, recording.position, recording.orientation);
                } else {
                    autoPlayTimer = Script.setTimeout(autoPlay, AUTOPLAY_SEARCH_INTERVAL);  // Try again soon.
                }
            }, Math.random() * AUTOPLAY_SEARCH_DELTA);
        }

        playRecording = function (recording, position, orientation) {
            Recording.loadRecording(recording, function (success) {
                if (success) {
                    Users.disableIgnoreRadius();

                    Agent.isAvatar = true;
                    Avatar.position = position;
                    Avatar.orientation = orientation;

                    Recording.setPlayFromCurrentLocation(true);
                    Recording.setPlayerUseDisplayName(true);
                    Recording.setPlayerUseHeadModel(false);
                    Recording.setPlayerUseAttachments(true);
                    Recording.setPlayerLoop(true);
                    Recording.setPlayerUseSkeletonModel(true);

                    isPlayingRecording = true;
                    recordingFilename = recording;

                    Recording.setPlayerTime(0.0);
                    Recording.startPlaying();

                    UserActivityLogger.logAction("playRecordingAC_play_recording");
                } else {
                    log("Failed to load recording " + recording);
                    autoPlayTimer = Script.setTimeout(autoPlay, AUTOPLAY_ERROR_INTERVAL);  // Try again later.
                }
            });
        };

        function stop() {
            log("Stop playing " + recordingFilename);

            Entity.destroy();

            if (Recording.isPlaying()) {
                Recording.stopPlaying();
                Agent.isAvatar = false;
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
            Entity.setUp();
        }

        function tearDown() {
            if (autoPlayTimer) {
                Script.clearTimeout(autoPlayTimer);
                autoPlayTimer = null;
            }
            Entity.tearDown();
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
            recording: Player.recording(),
            entity: Entity.id()
        }));
        heartbeatTimer = Script.setTimeout(sendHeartbeat, HEARTBEAT_INTERVAL);
    }

    function stopHeartbeat() {
        if (heartbeatTimer) {
            Script.clearTimeout(heartbeatTimer);
            heartbeatTimer = null;
        }
    }

    function onMessageReceived(channel, message, sender) {
        if (channel !== HIFI_PLAYER_CHANNEL) {
            return;
        }

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
                Player.autoPlay();  // There may be another recording to play.
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

        UserActivityLogger.logAction("playRecordingAC_script_load");
    }

    function tearDown() {
        stopHeartbeat();
        Player.stop();

        Messages.messageReceived.disconnect(onMessageReceived);
        Messages.unsubscribe(HIFI_PLAYER_CHANNEL);

        Player.tearDown();
    }

    setUp();
    Script.scriptEnding.connect(tearDown);

}());
