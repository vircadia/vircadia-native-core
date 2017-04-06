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
        isRecordingEnabled = false,
        IDLE = 0,
        COUNTING_DOWN = 1,
        RECORDING = 2,
        recordingState = IDLE,
        tablet,
        button,
        EVENT_BRIDGE_TYPE = "record",
        BODY_LOADED_ACTION = "bodyLoaded",
        USING_TOOLBAR_ACTION = "usingToolbar",
        ENABLE_RECORDING_ACTION = "enableRecording",

        CountdownTimer;

    CountdownTimer = (function () {
        var countdownTimer,
            countdownSeconds,
            NUMBER_OF_SECONDS = 3,
            finishCallback;

        function start(onFinishCallback) {
            finishCallback = onFinishCallback;
            countdownSeconds = NUMBER_OF_SECONDS;
            print(countdownSeconds);
            countdownTimer = Script.setInterval(function () {
                countdownSeconds -= 1;
                if (countdownSeconds <= 0) {
                    Script.clearInterval(countdownTimer);
                    finishCallback();
                } else {
                    print(countdownSeconds);
                }
            }, 1000);
        }

        function cancel() {
            Script.clearInterval(countdownTimer);
        }

        return {
            start: start,
            cancel: cancel
        };
    }());


    function usingToolbar() {
        return ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar"))
            || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar")));
    }

    function updateButtonState() {
        button.editProperties({ isActive: isRecordingEnabled || recordingState !== IDLE });
    }

    function startRecording() {
        recordingState = RECORDING;
        updateButtonState();
        print(APP_NAME + ": Start recording");
    }

    function finishRecording() {
        recordingState = IDLE;
        updateButtonState();
        print(APP_NAME + ": Finish recording");
    }

    function cancelRecording() {
        recordingState = IDLE;
        updateButtonState();
        print(APP_NAME + ": Cancel recording");
    }

    function finishCountdown() {
        recordingState = RECORDING;
        updateButtonState();
        startRecording();
    }

    function cancelCountdown() {
        recordingState = IDLE;
        updateButtonState();
        CountdownTimer.cancel();
        print(APP_NAME + ": Cancel countdown");
    }

    function startCountdown() {
        recordingState = COUNTING_DOWN;
        updateButtonState();
        print(APP_NAME + ": Start countdown");
        CountdownTimer.start(finishCountdown);
    }

    function onTabletScreenChanged(type, url) {
        // Open/close dialog in tablet or window.

        var RECORD_URL = "/scripts/system/html/record.html",
            HOME_URL = "Tablet.qml";

        if (type === "Home" && url === HOME_URL) {
            // Start countdown if recording is enabled.
            if (isRecordingEnabled && recordingState === IDLE) {
                startCountdown();
            }
            isDialogDisplayed = false;
        } else if (type === "Web" && url.slice(-RECORD_URL.length) === RECORD_URL) {
            // Cancel countdown or finish recording.
            if (recordingState === COUNTING_DOWN) {
                cancelCountdown();
            } else if (recordingState === RECORDING) {
                finishRecording();
            }
        }
    }

    function onTabletShownChanged() {
        // Open/close tablet.
        isDialogDisplayed = false;

        if (!tablet.tabletShown) {
            // Start countdown if recording is enabled.
            if (isRecordingEnabled && recordingState === IDLE) {
                startCountdown();
            }
        } else {
            // Cancel countdown or finish recording.
            if (recordingState === COUNTING_DOWN) {
                cancelCountdown();
            } else if (recordingState === RECORDING) {
                finishRecording();
            }
        }
    }

    function onWebEventReceived(data) {
        var message = JSON.parse(data);
        if (message.type === EVENT_BRIDGE_TYPE) {
            if (message.action === BODY_LOADED_ACTION) {
                // Dialog's ready; initialize its state.
                tablet.emitScriptEvent(JSON.stringify({
                    type: EVENT_BRIDGE_TYPE,
                    action: ENABLE_RECORDING_ACTION,
                    value: isRecordingEnabled
                }));
                tablet.emitScriptEvent(JSON.stringify({
                    type: EVENT_BRIDGE_TYPE,
                    action: USING_TOOLBAR_ACTION,
                    value: usingToolbar()
                }));
            } else if (message.action === ENABLE_RECORDING_ACTION) {
                // User update "enable recording" checkbox.
                // The recording state must be idle because the dialog is open.
                isRecordingEnabled = message.value;
                updateButtonState();
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

        // UI communications.
        tablet.webEventReceived.connect(onWebEventReceived);

        // Track showing/hiding tablet/dialog.
        tablet.screenChanged.connect(onTabletScreenChanged);
        tablet.tabletShownChanged.connect(onTabletShownChanged);
    }

    function tearDown() {
        if (recordingState === COUNTING_DOWN) {
            cancelCountdown();
        } else if (recordingState === RECORDING) {
            cancelRecording();
        }

        if (isDialogDisplayed) {
            tablet.gotoHomeScreen();
        }

        if (!tablet) {
            return;
        }
        tablet.webEventReceived.disconnect(onWebEventReceived);
        tablet.tabletShownChanged.disconnect(onTabletShownChanged);
        tablet.screenChanged.disconnect(onTabletScreenChanged);
        button.clicked.disconnect(onButtonClicked);
        tablet.removeButton(button);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
