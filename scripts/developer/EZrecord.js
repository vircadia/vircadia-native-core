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

        Recorder;

    function log(message) {
        print(APP_NAME + ": " + message);
    }

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

                        // TODO

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

                // TODO

            }
            recordingState = IDLE;
        }

        function finishRecording() {
            log("Finish recording");
            playSound(finishRecordingSound);

            // TODO

            recordingState = IDLE;
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

        Controller.keyPressEvent.connect(onKeyPressEvent);
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
