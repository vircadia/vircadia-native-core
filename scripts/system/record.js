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
        APP_ICON_INACTIVE = "icons/tablet-icons/avatar-record-i.svg",
        APP_ICON_ACTIVE = "icons/tablet-icons/avatar-record-a.svg",
        APP_URL = Script.resolvePath("html/record.html"),
        isDialogDisplayed = false,
        tablet,
        button,
        isConnected,

        RecordingIndicator,
        Recorder,
        Player,
        Dialog,

        SCRIPT_STARTUP_DELAY = 3000;  // 3s

    function log(message) {
        print(APP_NAME + ": " + message);
    }

    function error(message, info) {
        print(APP_NAME + ": " + message + (info !== undefined ? " - " + info : ""));
        Window.alert(message);
    }

    function logDetails() {
        return {
            current_domain: location.placename
        };
    }

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
                CAMERA_JOINT_INDEX = -7;

            if (HMD.active) {
                // 3D overlay attached to avatar.
                hmdOverlay = Overlays.addOverlay("text3d", {
                    text: recordingText,
                    dimensions: { x: 3 * HMD_FONT_SIZE, y: HMD_FONT_SIZE },
                    parentID: MyAvatar.sessionUUID,
                    parentJointIndex: CAMERA_JOINT_INDEX,
                    localPosition: { x: 0.95, y: 0.95, z: -2.0 },
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
            play,

            countdownTimer,
            countdownSeconds,
            COUNTDOWN_SECONDS = 3,

            tickSound,
            startRecordingSound,
            finishRecordingSound,
            TICK_SOUND = "assets/sounds/countdown-tick.wav",
            START_RECORDING_SOUND = "assets/sounds/start-recording.wav",
            FINISH_RECORDING_SOUND = "assets/sounds/finish-recording.wav",
            START_RECORDING_SOUND_DURATION = 1200,
            SOUND_VOLUME = 0.2;

        function playSound(sound) {
            Audio.playSound(sound, {
                position: MyAvatar.position,
                localOnly: true,
                volume: SOUND_VOLUME
            });
        }

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
            playSound(startRecordingSound);
            Script.setTimeout(function () {
                // Delay start so that start beep is not included in recorded sound.
                startPosition = MyAvatar.position;
                startOrientation = MyAvatar.orientation;
                Recording.startRecording();
                RecordingIndicator.show();
            }, START_RECORDING_SOUND_DURATION);
        }

        function finishRecording() {
            var success,
                error;

            recordingState = IDLE;
            log("Finish recording");
            UserActivityLogger.logAction("record_finish_recording", logDetails());
            playSound(finishRecordingSound);
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
            Dialog.setCountdownNumber("");
            recordingState = RECORDING;
            startRecording();
        }

        function cancelCountdown() {
            recordingState = IDLE;
            Script.clearInterval(countdownTimer);
            Dialog.setCountdownNumber("");
            log("Cancel countdown");
        }

        function startCountdown() {
            recordingState = COUNTING_DOWN;
            log("Start countdown");
            countdownSeconds = COUNTDOWN_SECONDS;
            Dialog.setCountdownNumber(countdownSeconds);
            playSound(tickSound);
            countdownTimer = Script.setInterval(function () {
                countdownSeconds -= 1;
                if (countdownSeconds <= 0) {
                    Script.clearInterval(countdownTimer);
                    finishCountdown();
                } else {
                    Dialog.setCountdownNumber(countdownSeconds);
                    playSound(tickSound);
                }
            }, 1000);
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

            tickSound = SoundCache.getSound(Script.resolvePath(TICK_SOUND));
            startRecordingSound = SoundCache.getSound(Script.resolvePath(START_RECORDING_SOUND));
            finishRecordingSound = SoundCache.getSound(Script.resolvePath(FINISH_RECORDING_SOUND));
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
            RECORDER_COMMAND_ERROR = "error",
            HIFI_PLAYER_CHANNEL = "HiFi-Player-Channel",
            PLAYER_COMMAND_PLAY = "play",
            PLAYER_COMMAND_STOP = "stop",

            playerIDs = [],             // UUIDs of AC player scripts.
            playerIsPlayings = [],      // True if AC player script is playing a recording.
            playerRecordings = [],      // Assignment client mappings of recordings being played.
            playerTimestamps = [],      // Timestamps of last heartbeat update from player script.

            updateTimer,
            UPDATE_INTERVAL = 5000;  // Must be > player's HEARTBEAT_INTERVAL.

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
            var index;

            // Optional function parameters.
            if (position === undefined) {
                position = MyAvatar.position;
            }
            if (orientation === undefined) {
                orientation = MyAvatar.orientation;
            }

            index = playerIsPlayings.indexOf(false);
            if (index === -1) {
                error("No player instance available to play recording "
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

            if (channel !== HIFI_RECORDER_CHANNEL) {
                return;
            }

            message = JSON.parse(message);

            if (message.command === RECORDER_COMMAND_ERROR) {
                if (message.user === MyAvatar.sessionUUID) {
                    error(message.message);
                }
            } else {
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
        }

        function reset() {
            playerIDs = [];
            playerIsPlayings = [];
            playerRecordings = [];
            playerTimestamps = [];
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
            reset: reset,
            setUp: setUp,
            tearDown: tearDown
        };
    }());

    Dialog = (function () {
        var isFinishOnOpen = false,
            countdownNumber = "",
            EVENT_BRIDGE_TYPE = "record",
            BODY_LOADED_ACTION = "bodyLoaded",
            USING_TOOLBAR_ACTION = "usingToolbar",
            RECORDINGS_BEING_PLAYED_ACTION = "recordingsBeingPlayed",
            NUMBER_OF_PLAYERS_ACTION = "numberOfPlayers",
            STOP_PLAYING_RECORDING_ACTION = "stopPlayingRecording",
            LOAD_RECORDING_ACTION = "loadRecording",
            START_RECORDING_ACTION = "startRecording",
            SET_COUNTDOWN_NUMBER_ACTION = "setCountdownNumber",
            STOP_RECORDING_ACTION = "stopRecording",
            FINISH_ON_OPEN_ACTION = "finishOnOpen",
            SETTINGS_FINISH_ON_OPEN = "record/finishOnOpen";

        function isUsingToolbar() {
            return ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar"))
                || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar")));
        }

        function updateRecordingStatus(isRecording) {
            if (isRecording) {
                tablet.emitScriptEvent(JSON.stringify({
                    type: EVENT_BRIDGE_TYPE,
                    action: START_RECORDING_ACTION
                }));
                tablet.emitScriptEvent(JSON.stringify({
                    type: EVENT_BRIDGE_TYPE,
                    action: SET_COUNTDOWN_NUMBER_ACTION,
                    value: countdownNumber
                }));
            } else {
                tablet.emitScriptEvent(JSON.stringify({
                    type: EVENT_BRIDGE_TYPE,
                    action: STOP_RECORDING_ACTION
                }));
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

        function setCountdownNumber(number) {
            countdownNumber = number;
            tablet.emitScriptEvent(JSON.stringify({
                type: EVENT_BRIDGE_TYPE,
                action: SET_COUNTDOWN_NUMBER_ACTION,
                value: countdownNumber
            }));
        }

        function finishOnOpen() {
            return isFinishOnOpen;
        }

        function onWebEventReceived(data) {
            var message,
                recording;

            message = JSON.parse(data);
            if (message.type === EVENT_BRIDGE_TYPE) {
                switch (message.action) {
                case BODY_LOADED_ACTION:
                    // Dialog's ready; initialize its state.
                    tablet.emitScriptEvent(JSON.stringify({
                        type: EVENT_BRIDGE_TYPE,
                        action: USING_TOOLBAR_ACTION,
                        value: isUsingToolbar()
                    }));
                    tablet.emitScriptEvent(JSON.stringify({
                        type: EVENT_BRIDGE_TYPE,
                        action: FINISH_ON_OPEN_ACTION,
                        value: isFinishOnOpen
                    }));
                    tablet.emitScriptEvent(JSON.stringify({
                        type: EVENT_BRIDGE_TYPE,
                        action: NUMBER_OF_PLAYERS_ACTION,
                        value: Player.numberOfPlayers()
                    }));
                    updateRecordingStatus(!Recorder.isIdle());
                    UserActivityLogger.logAction("record_open_dialog", logDetails());
                    break;
                case STOP_PLAYING_RECORDING_ACTION:
                    // Stop the specified player.
                    log("Unload recording " + message.value);
                    Player.stopPlayingRecording(message.value);
                    break;
                case LOAD_RECORDING_ACTION:
                    // User wants to select an ATP recording to play.
                    recording = Window.browseAssets("Select Recording to Play", "recordings", "*.hfr");
                    if (recording) {
                        log("Load recording " + recording);
                        UserActivityLogger.logAction("record_load_recording", logDetails());
                        Player.playRecording("atp:" + recording, MyAvatar.position, MyAvatar.orientation);
                    }
                    break;
                case START_RECORDING_ACTION:
                    // Start making a recording.
                    if (Recorder.isIdle()) {
                        Recorder.startCountdown();
                    }
                    break;
                case STOP_RECORDING_ACTION:
                    // Cancel or finish a recording.
                    if (Recorder.isCountingDown()) {
                        Recorder.cancelCountdown();
                    } else if (Recorder.isRecording()) {
                        Recorder.finishRecording();
                    }
                    break;
                case FINISH_ON_OPEN_ACTION:
                    // Set behavior on dialog open.
                    isFinishOnOpen = message.value;
                    Settings.setValue(SETTINGS_FINISH_ON_OPEN, isFinishOnOpen);
                    break;
                }
            }
        }

        function setUp() {
            isFinishOnOpen = Settings.getValue(SETTINGS_FINISH_ON_OPEN) === true;
            tablet.webEventReceived.connect(onWebEventReceived);
        }

        function tearDown() {
            tablet.webEventReceived.disconnect(onWebEventReceived);
        }

        return {
            updatePlayerDetails: updatePlayerDetails,
            updateRecordingStatus: updateRecordingStatus,
            setCountdownNumber: setCountdownNumber,
            finishOnOpen: finishOnOpen,
            setUp: setUp,
            tearDown: tearDown
        };
    }());

    function onTabletScreenChanged(type, url) {
        // Opened/closed dialog in tablet or window.
        var RECORD_URL = "/scripts/system/html/record.html";

        if (type === "Web" && url.slice(-RECORD_URL.length) === RECORD_URL) {
            if (Dialog.finishOnOpen()) {
                // Cancel countdown or finish recording.
                if (Recorder.isCountingDown()) {
                    Recorder.cancelCountdown();
                } else if (Recorder.isRecording()) {
                    Recorder.finishRecording();
                }
                Dialog.updateRecordingStatus(false);
            }
            isDialogDisplayed = true;
        } else {
            isDialogDisplayed = false;
        }
        button.editProperties({ isActive: isDialogDisplayed });
    }

    function onTabletShownChanged() {
        // Opened/closed tablet.
        if (tablet.tabletShown && Dialog.finishOnOpen()) {
            // Cancel countdown or finish recording.
            if (Recorder.isCountingDown()) {
                Recorder.cancelCountdown();
            } else if (Recorder.isRecording()) {
                Recorder.finishRecording();
            }
            Dialog.updateRecordingStatus(false);
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

    function onUpdate() {
        if (isConnected !== Window.location.isConnected) {
            // Server restarted or domain changed.
            isConnected = !isConnected;
            if (!isConnected) {
                // Clear dialog.
                Player.reset();
            }
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
        if (button) {
            button.clicked.connect(onButtonClicked);
        }

        // Track showing/hiding tablet/dialog.
        tablet.screenChanged.connect(onTabletScreenChanged);
        tablet.tabletShownChanged.connect(onTabletShownChanged);

        Dialog.setUp();
        Player.setUp();
        Recorder.setUp(Player.playRecording);

        isConnected = Window.location.isConnected;
        Script.update.connect(onUpdate);

        UserActivityLogger.logAction("record_run_script", logDetails());
    }

    function tearDown() {
        if (!tablet) {
            return;
        }

        Script.update.disconnect(onUpdate);

        Recorder.tearDown();
        Player.tearDown();
        Dialog.tearDown();

        tablet.tabletShownChanged.disconnect(onTabletShownChanged);
        tablet.screenChanged.disconnect(onTabletScreenChanged);
        if (button) {
            button.clicked.disconnect(onButtonClicked);
            tablet.removeButton(button);
            button = null;
        }

        if (Recorder.isCountingDown()) {
            Recorder.cancelCountdown();
        } else if (Recorder.isRecording()) {
            Recorder.cancelRecording();
        }

        if (isDialogDisplayed) {
            tablet.gotoHomeScreen();
        }

        tablet = null;
    }

    // FIXME: If setUp() is run immediately at Interface start-up, Interface hangs and crashes because of the line of code:
    //     tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    //setUp();
    //Script.scriptEnding.connect(tearDown);
    Script.setTimeout(function () {
        setUp();
        Script.scriptEnding.connect(tearDown);
    }, SCRIPT_STARTUP_DELAY);
}());
