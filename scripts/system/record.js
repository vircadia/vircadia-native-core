"use strict";

//
//  record.js
//
//  Created by David Rowe on 5 Apr 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    var APP_NAME = "RECORD",
        APP_ICON_INACTIVE = "icons/tablet-icons/edit-i.svg",  // FIXME: Record icon.
        APP_ICON_ACTIVE = "icons/tablet-icons/edit-a.svg",  // FIXME: Record icon.
        APP_URL = Script.resolvePath("html/record.html"),
        isDialogDisplayed = false,
        tablet,
        button,

        CountdownTimer,
        RecordingIndicator,
        Recorder,
        Player,
        Dialog;

    function log(message) {
        print(APP_NAME + ": " + message);
    }

    function error(message, info) {
        print(APP_NAME + ": " + message + (info !== "" ? " - " + info : ""));
        Window.alert(message);

    }

    CountdownTimer = (function () {
        // Displays countdown overlay.

        var countdownTimer,
            countdownSeconds,
            COUNTDOWN_SECONDS = 3,
            finishCallback,
            isHMD = false,
            desktopOverlay,
            desktopOverlayAdjust,
            DESKTOP_FONT_SIZE = 200,
            hmdOverlay,
            HMD_FONT_SIZE = 0.25;

        function displayOverlay() {
            // Create both overlays in case user switches desktop/HMD mode during countdown.
            var screenSize = Controller.getViewportDimensions(),
                AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

            isHMD = HMD.active;

            desktopOverlayAdjust = {  // Adjust overlay position to cater to font rendering.
                x: -DESKTOP_FONT_SIZE / 4,
                y: -DESKTOP_FONT_SIZE / 2 - 50

            };

            desktopOverlay = Overlays.addOverlay("text", {
                text: countdownSeconds,
                width: DESKTOP_FONT_SIZE,
                height: DESKTOP_FONT_SIZE + 20,
                x: screenSize.x / 2 + desktopOverlayAdjust.x,
                y: screenSize.y / 2 + desktopOverlayAdjust.y,
                font: { size: DESKTOP_FONT_SIZE },
                color: { red: 255, green: 255, blue: 255 },
                alpha: 0.9,
                backgroundAlpha: 0,
                visible: !isHMD
            });

            hmdOverlay = Overlays.addOverlay("text3d", {
                text: countdownSeconds,
                dimensions: { x: HMD_FONT_SIZE, y: HMD_FONT_SIZE },
                parentID: AVATAR_SELF_ID,
                localPosition: { x: 0, y: 0.6, z: 2.0 },
                color: { red: 255, green: 255, blue: 255 },
                alpha: 0.9,
                lineHeight: HMD_FONT_SIZE,
                backgroundAlpha: 0,
                ignoreRayIntersection: true,
                isFacingAvatar: true,
                drawInFront: true,
                visible: isHMD
            });
        }

        function updateOverlay() {
            var screenSize;

            if (isHMD !== HMD.active) {
                if (isHMD) {
                    Overlays.editOverlay(hmdOverlay, { visible: false });
                } else {
                    Overlays.editOverlay(desktopOverlay, { visible: false });
                }
                isHMD = HMD.active;
            }

            if (isHMD) {
                Overlays.editOverlay(hmdOverlay, {
                    text: countdownSeconds,
                    visible: true
                });
            } else {
                screenSize = Controller.getViewportDimensions();
                Overlays.editOverlay(desktopOverlay, {
                    x: screenSize.x / 2 + desktopOverlayAdjust.x,
                    y: screenSize.y / 2 + desktopOverlayAdjust.y,
                    text: countdownSeconds,
                    visible: true
                });
            }
        }

        function deleteOverlay() {
            Overlays.deleteOverlay(desktopOverlay);
            Overlays.deleteOverlay(hmdOverlay);
        }

        function start(onFinishCallback) {
            finishCallback = onFinishCallback;
            countdownSeconds = COUNTDOWN_SECONDS;
            displayOverlay();
            countdownTimer = Script.setInterval(function () {
                countdownSeconds -= 1;
                if (countdownSeconds <= 0) {
                    Script.clearInterval(countdownTimer);
                    deleteOverlay();
                    finishCallback();
                } else {
                    updateOverlay();
                }
            }, 1000);
        }

        function cancel() {
            Script.clearInterval(countdownTimer);
            deleteOverlay();
        }

        return {
            start: start,
            cancel: cancel
        };
    }());

    RecordingIndicator = (function () {
        // Displays "recording" overlay.

        var hmdOverlay,
            HMD_FONT_SIZE = 0.08,
            desktopOverlay,
            DESKTOP_FONT_SIZE = 24;

        function show() {
            // Create both overlays in case user switches desktop/HMD mode.
            var screenSize = Controller.getViewportDimensions(),
                recordingText = "REC",  // Unicode circle \u25cf doesn't render in HMD.
                AVATAR_SELF_ID = "{00000000-0000-0000-0000-000000000001}";

            if (HMD.active) {
                // 3D overlay attached to avatar.
                hmdOverlay = Overlays.addOverlay("text3d", {
                    text: recordingText,
                    dimensions: { x: 3 * HMD_FONT_SIZE, y: HMD_FONT_SIZE },
                    parentID: AVATAR_SELF_ID,
                    localPosition: { x: -1.0, y: 1.1, z: 2.0 },
                    color: { red: 255, green: 0, blue: 0 },
                    alpha: 0.9,
                    lineHeight: HMD_FONT_SIZE,
                    backgroundAlpha: 0,
                    ignoreRayIntersection: true,
                    isFacingAvatar: true,
                    drawInFront: true,
                    visible: true
                });
            } else {
                // 2D overlay on desktop.
                desktopOverlay = Overlays.addOverlay("text", {
                    text: recordingText,
                    width: 3 * DESKTOP_FONT_SIZE,
                    height: DESKTOP_FONT_SIZE,
                    x: screenSize.x - 4 * DESKTOP_FONT_SIZE,
                    y: DESKTOP_FONT_SIZE,
                    font: { size: DESKTOP_FONT_SIZE },
                    color: { red: 255, green: 8, blue: 8 },
                    alpha: 1.0,
                    backgroundAlpha: 0,
                    visible: true
                });
            }
        }

        function hide() {
            if (desktopOverlay) {
                Overlays.deleteOverlay(desktopOverlay);
            }
            if (hmdOverlay) {
                Overlays.deleteOverlay(hmdOverlay);
            }
        }

        return {
            show: show,
            hide: hide
        };
    }());

    Recorder = (function () {
        // Makes the recording and uploads it to the domain's Asset Server.

        var IDLE = 0,
            COUNTING_DOWN = 1,
            RECORDING = 2,
            recordingState = IDLE,
            mappingPath,
            startPosition,
            startOrientation,
            play;

        function setMappingCallback(status) {
            if (status !== "") {
                error("Error mapping recording to " + mappingPath + " on Asset Server!", status);
                return;
            }

            log("Recording mapped to " + mappingPath);
            log("Request play recording");

            play("atp:" + mappingPath, startPosition, startOrientation);
        }

        function saveRecordingToAssetCallback(url) {
            var filename,
                hash;

            if (url === "") {
                error("Error saving recording to Asset Server!");
                return;
            }

            log("Recording saved to Asset Server as " + url);

            filename = (new Date()).toISOString();  // yyyy-mm-ddThh:mm:ss.sssZ
            filename = filename.replace(/[\-:]|\.\d*Z$/g, "").replace("T", "-") + ".hfr";  // yyyymmmdd-hhmmss.hfr
            hash = url.slice(4);  // Remove leading "atp:" from url.
            mappingPath = "/recordings/" + filename;
            Assets.setMapping(mappingPath, hash, setMappingCallback);
        }

        function startRecording() {
            recordingState = RECORDING;
            log("Start recording");
            startPosition = MyAvatar.position;
            startOrientation = MyAvatar.orientation;
            Recording.startRecording();
            RecordingIndicator.show();
        }

        function finishRecording() {
            var success,
                error;

            recordingState = IDLE;
            log("Finish recording");
            Recording.stopRecording();
            RecordingIndicator.hide();
            success = Recording.saveRecordingToAsset(saveRecordingToAssetCallback);
            if (!success) {
                error("Error saving recording to Asset Server!");
            }
        }

        function cancelRecording() {
            Recording.stopRecording();
            RecordingIndicator.hide();
            recordingState = IDLE;
            log("Cancel recording");
        }

        function finishCountdown() {
            recordingState = RECORDING;
            startRecording();
        }

        function cancelCountdown() {
            recordingState = IDLE;
            CountdownTimer.cancel();
            log("Cancel countdown");
        }

        function startCountdown() {
            recordingState = COUNTING_DOWN;
            log("Start countdown");
            CountdownTimer.start(finishCountdown);
        }

        function isIdle() {
            return recordingState === IDLE;
        }

        function isCountingDown() {
            return recordingState === COUNTING_DOWN;
        }

        function isRecording() {
            return recordingState === RECORDING;
        }

        function setUp(playerCallback) {
            play = playerCallback;
        }

        function tearDown() {
            // Nothing to do; any cancelling of recording needs to be done by script using this object.
        }

        return {
            startCountdown: startCountdown,
            cancelCountdown: cancelCountdown,
            startRecording: startRecording,
            cancelRecording: cancelRecording,
            finishRecording: finishRecording,
            isIdle: isIdle,
            isCountingDown: isCountingDown,
            isRecording: isRecording,
            setUp: setUp,
            tearDown: tearDown
        };
    }());

    Player = (function () {
        var HIFI_RECORDER_CHANNEL = "HiFi-Recorder-Channel",
            HIFI_PLAYER_CHANNEL = "HiFi-Player-Channel",
            PLAYER_COMMAND_PLAY = "play",
            PLAYER_COMMAND_STOP = "stop",

            playerIDs = [],         // UUIDs of AC player scripts.
            playerIsPlayings = [],  // True if AC player script is playing a recording.
            playerRecordings = [],  // Assignment client mappings of recordings being played.
            playerTimestamps = [],  // Timestamps of last heartbeat update from player script.

            updateTimer,
            UPDATE_INTERVAL = 5000;  // Must be > player's HEARTBEAT_INTERVAL. TODO: Final value.

        function numberOfPlayers() {
            return playerIDs.length;
        }

        function updatePlayers() {
            var now = Date.now(),
                countBefore = playerIDs.length,
                i;

            // Remove players that haven't sent a heartbeat for a while.
            for (i = playerTimestamps.length - 1; i >= 0; i -= 1) {
                if (now - playerTimestamps[i] > UPDATE_INTERVAL) {
                    playerIDs.splice(i, 1);
                    playerIsPlayings.splice(i, 1);
                    playerRecordings.splice(i, 1);
                    playerTimestamps.splice(i, 1);
                }
            }

            // Update UI.
            if (playerIDs.length !== countBefore) {
                Dialog.updatePlayerDetails(playerIsPlayings, playerRecordings, playerIDs);
            }
        }

        function playRecording(recording, position, orientation) {
            var index,
                CHECK_PLAYING_TIMEOUT = 5000;

            // Optional function parameters.
            if (position === undefined) {
                position = MyAvatar.position;
            }
            if (orientation === undefined) {
                orientation = MyAvatar.orientation;
            }

            index = playerIsPlayings.indexOf(false);
            if (index === -1) {
                error("No assignment client player available to play recording "
                    + recording.slice(4) + "!");  // Remove leading "atp:" from recording.
                return;
            }

            Messages.sendMessage(HIFI_PLAYER_CHANNEL, JSON.stringify({
                player: playerIDs[index],
                command: PLAYER_COMMAND_PLAY,
                recording: recording,
                position: position,
                orientation: orientation
            }));

            Script.setTimeout(function () {
                if (!playerIsPlayings[index] || playerRecordings[index] !== recording) {
                    error("Didn't start playing recording "
                        + recording.slice(4) + "!");  // Remove leading "atp:" from recording.
                }
            }, CHECK_PLAYING_TIMEOUT);

        }

        function stopPlayingRecording(playerID) {
            Messages.sendMessage(HIFI_PLAYER_CHANNEL, JSON.stringify({
                player: playerID,
                command: PLAYER_COMMAND_STOP
            }));
        }

        function onMessageReceived(channel, message, sender) {
            // Heartbeat from AC script.
            var index;

            message = JSON.parse(message);

            index = playerIDs.indexOf(sender);
            if (index === -1) {
                index = playerIDs.length;
                playerIDs[index] = sender;
            }
            playerIsPlayings[index] = message.playing;
            playerRecordings[index] = message.recording;
            playerTimestamps[index] = Date.now();
            Dialog.updatePlayerDetails(playerIsPlayings, playerRecordings, playerIDs);
        }

        function setUp() {
            // Messaging with AC scripts.
            Messages.messageReceived.connect(onMessageReceived);
            Messages.subscribe(HIFI_RECORDER_CHANNEL);

            updateTimer = Script.setInterval(updatePlayers, UPDATE_INTERVAL);
        }

        function tearDown() {
            Script.clearInterval(updateTimer);

            Messages.messageReceived.disconnect(onMessageReceived);
            Messages.subscribe(HIFI_RECORDER_CHANNEL);
        }

        return {
            playRecording: playRecording,
            stopPlayingRecording: stopPlayingRecording,
            numberOfPlayers: numberOfPlayers,
            setUp: setUp,
            tearDown: tearDown
        };
    }());

    Dialog = (function () {
        var EVENT_BRIDGE_TYPE = "record",
            BODY_LOADED_ACTION = "bodyLoaded",
            RECORDINGS_BEING_PLAYED_ACTION = "recordingsBeingPlayed",
            NUMBER_OF_PLAYERS_ACTION = "numberOfPlayers",
            STOP_PLAYING_RECORDING_ACTION = "stopPlayingRecording",
            LOAD_RECORDING_ACTION = "loadRecording",
            START_RECORDING_ACTION = "startRecording";

        function onWebEventReceived(data) {
            var message = JSON.parse(data);
            if (message.type === EVENT_BRIDGE_TYPE) {
                switch (message.action) {
                case BODY_LOADED_ACTION:
                    // Dialog's ready; initialize its state.
                    tablet.emitScriptEvent(JSON.stringify({
                        type: EVENT_BRIDGE_TYPE,
                        action: NUMBER_OF_PLAYERS_ACTION,
                        value: Player.numberOfPlayers()
                    }));
                    break;
                case STOP_PLAYING_RECORDING_ACTION:
                    // Stop the specified player.
                    Player.stopPlayingRecording(message.value);
                    break;
                case LOAD_RECORDING_ACTION:
                    // User wants to select an ATP recording to play.
                    log("TODO: Open dialog for user to select ATP recording to play");
                    break;
                case START_RECORDING_ACTION:
                    // Start making a recording.
                    tablet.gotoHomeScreen();  // Closes window dialog.
                    HMD.closeTablet();
                    if (Recorder.isIdle()) {
                        Recorder.startCountdown();
                    }
                    break;
                }
            }
        }

        function updatePlayerDetails(playerIsPlayings, playerRecordings, playerIDs) {
            var recordingsBeingPlayed = [],
                length,
                i;

            for (i = 0, length = playerIsPlayings.length; i < length; i += 1) {
                if (playerIsPlayings[i]) {
                    recordingsBeingPlayed.push({
                        filename: playerRecordings[i],
                        playerID: playerIDs[i]
                    });
                }
            }
            tablet.emitScriptEvent(JSON.stringify({
                type: EVENT_BRIDGE_TYPE,
                action: RECORDINGS_BEING_PLAYED_ACTION,
                value: JSON.stringify(recordingsBeingPlayed)
            }));

            tablet.emitScriptEvent(JSON.stringify({
                type: EVENT_BRIDGE_TYPE,
                action: NUMBER_OF_PLAYERS_ACTION,
                value: playerIsPlayings.length
            }));
        }

        function setUp() {
            tablet.webEventReceived.connect(onWebEventReceived);
        }

        function tearDown() {
            tablet.webEventReceived.disconnect(onWebEventReceived);
        }

        return {
            updatePlayerDetails: updatePlayerDetails,
            setUp: setUp,
            tearDown: tearDown
        };
    }());

    function onTabletScreenChanged(type, url) {
        // Open/close dialog in tablet or window.
        var RECORD_URL = "/scripts/system/html/record.html";

        if (type === "Web" && url.slice(-RECORD_URL.length) === RECORD_URL) {
            // Cancel countdown or finish recording.
            if (Recorder.isCountingDown()) {
                Recorder.cancelCountdown();
            } else if (Recorder.isRecording()) {
                Recorder.finishRecording();
            }
            isDialogDisplayed = true;
        } else {
            isDialogDisplayed = false;
        }
        button.editProperties({ isActive: isDialogDisplayed });
    }

    function onTabletShownChanged() {
        // Open/close tablet.

        if (tablet.tabletShown) {
            // Cancel countdown or finish recording.
            if (Recorder.isCountingDown()) {
                Recorder.cancelCountdown();
            } else if (Recorder.isRecording()) {
                Recorder.finishRecording();
            }
        }
    }

    function onButtonClicked() {
        if (isDialogDisplayed) {
            // Can click icon in toolbar mode; gotoHomeScreen() closes dialog.
            tablet.gotoHomeScreen();
            isDialogDisplayed = false;
        } else {
            tablet.gotoWebScreen(APP_URL);
            isDialogDisplayed = true;
        }
    }

    function setUp() {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (!tablet) {
            return;
        }

        // Tablet/toolbar button.
        button = tablet.addButton({
            icon: APP_ICON_INACTIVE,
            activeIcon: APP_ICON_ACTIVE,
            text: APP_NAME,
            isActive: false
        });
        button.clicked.connect(onButtonClicked);

        // Track showing/hiding tablet/dialog.
        tablet.screenChanged.connect(onTabletScreenChanged);
        tablet.tabletShownChanged.connect(onTabletShownChanged);

        Dialog.setUp();
        Player.setUp();
        Recorder.setUp(Player.playRecording);
    }

    function tearDown() {
        if (!tablet) {
            return;
        }

        Recorder.tearDown();
        Player.tearDown();
        Dialog.tearDown();

        tablet.tabletShownChanged.disconnect(onTabletShownChanged);
        tablet.screenChanged.disconnect(onTabletScreenChanged);
        button.clicked.disconnect(onButtonClicked);

        if (Recorder.isCountingDown()) {
            Recorder.cancelCountdown();
        } else if (Recorder.isRecording()) {
            Recorder.cancelRecording();
        }

        if (isDialogDisplayed) {
            tablet.gotoHomeScreen();
        }

        tablet.removeButton(button);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
