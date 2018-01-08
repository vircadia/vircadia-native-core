"use strict";

//
//  EZrecord.js
//
//  Created by David Rowe on 24 Jun 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {

    var APP_NAME = "EZRECORD",
        APP_ICON_INACTIVE = "icons/tablet-icons/avatar-record-i.svg",
        APP_ICON_ACTIVE = "icons/tablet-icons/avatar-record-a.svg",
        SHORTCUT_KEY = "r",  // Alt modifier is assumed.
        tablet,
        button,

        RecordingIndicator,
        Recorder;

    function log(message) {
        print(APP_NAME + ": " + message);
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
        var IDLE = 0,
            COUNTING_DOWN = 1,
            RECORDING = 2,
            recordingState = IDLE,

            countdownTimer,
            countdownSeconds,
            COUNTDOWN_SECONDS = 3,

            tickSound,
            startRecordingSound,
            finishRecordingSound,
            TICK_SOUND = "../system/assets/sounds/countdown-tick.wav",
            START_RECORDING_SOUND = "../system/assets/sounds/start-recording.wav",
            FINISH_RECORDING_SOUND = "../system/assets/sounds/finish-recording.wav",
            START_RECORDING_SOUND_DURATION = 1200,
            SOUND_VOLUME = 0.2;

        function playSound(sound) {
            Audio.playSound(sound, {
                position: MyAvatar.position,
                localOnly: true,
                volume: SOUND_VOLUME
            });
        }

        function isRecording() {
            return recordingState === COUNTING_DOWN || recordingState === RECORDING;
        }

        function startRecording() {
            if (recordingState !== IDLE) {
                return;
            }

            log("Start countdown");
            countdownSeconds = COUNTDOWN_SECONDS;
            playSound(tickSound);
            countdownTimer = Script.setInterval(function () {
                countdownSeconds -= 1;
                if (countdownSeconds <= 0) {
                    Script.clearInterval(countdownTimer);
                    playSound(startRecordingSound);
                    log("Start recording");
                    Script.setTimeout(function () {
                        // Delay start so that start beep is not included in recorded sound.
                        Recording.startRecording();
                        RecordingIndicator.show();
                    }, START_RECORDING_SOUND_DURATION);
                    recordingState = RECORDING;
                } else {
                    playSound(tickSound);
                }
            }, 1000);

            recordingState = COUNTING_DOWN;
        }

        function cancelRecording() {
            log("Cancel recording");
            if (recordingState === COUNTING_DOWN) {
                Script.clearInterval(countdownTimer);
            } else {
                Recording.stopRecording();
                RecordingIndicator.hide();
            }
            recordingState = IDLE;
        }

        function finishRecording() {
            playSound(finishRecordingSound);
            Recording.stopRecording();
            RecordingIndicator.hide();
            var filename = (new Date()).toISOString();  // yyyy-mm-ddThh:mm:ss.sssZ
            filename = filename.replace(/[\-:]|\.\d*Z$/g, "").replace("T", "-") + ".hfr";  // yyyymmmdd-hhmmss.hfr
            filename = Recording.getDefaultRecordingSaveDirectory() + filename;
            log("Finish recording: " + filename);
            Recording.saveRecording(filename);
            recordingState = IDLE;
            UserActivityLogger.logAction("ezrecord_finish_recording", logDetails());
        }

        function stopRecording() {
            if (recordingState === COUNTING_DOWN) {
                cancelRecording();
            } else if (recordingState === RECORDING) {
                finishRecording();
            }
        }

        function setUp() {
            tickSound = SoundCache.getSound(Script.resolvePath(TICK_SOUND));
            startRecordingSound = SoundCache.getSound(Script.resolvePath(START_RECORDING_SOUND));
            finishRecordingSound = SoundCache.getSound(Script.resolvePath(FINISH_RECORDING_SOUND));
        }

        function tearDown() {
            if (recordingState !== IDLE) {
                cancelRecording();
            }
        }

        return {
            isRecording: isRecording,
            startRecording: startRecording,
            stopRecording: stopRecording,
            setUp: setUp,
            tearDown: tearDown
        };
    }());

    function toggleRecording() {
        if (Recorder.isRecording()) {
            Recorder.stopRecording();
        } else {
            Recorder.startRecording();
        }
        button.editProperties({ isActive: Recorder.isRecording() });
    }

    function onKeyPressEvent(event) {
        if (event.isAlt && event.text === SHORTCUT_KEY && !event.isControl && !event.isMeta && !event.isAutoRepeat) {
            toggleRecording();
        }
    }

    function onButtonClicked() {
        toggleRecording();
    }

    function setUp() {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (!tablet) {
            return;
        }

        Recorder.setUp();

        // tablet/toolbar button.
        button = tablet.addButton({
            icon: APP_ICON_INACTIVE,
            activeIcon: APP_ICON_ACTIVE,
            text: APP_NAME,
            isActive: false
        });
        if (button) {
            button.clicked.connect(onButtonClicked);
        }

        Controller.keyPressEvent.connect(onKeyPressEvent);

        UserActivityLogger.logAction("ezrecord_run_script", logDetails());
    }

    function tearDown() {

        Controller.keyPressEvent.disconnect(onKeyPressEvent);

        Recorder.tearDown();

        if (!tablet) {
            return;
        }

        if (button) {
            button.clicked.disconnect(onButtonClicked);
            tablet.removeButton(button);
            button = null;
        }

        tablet = null;

    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
