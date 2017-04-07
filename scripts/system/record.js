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
        tablet,
        button,
        EVENT_BRIDGE_TYPE = "record",
        BODY_LOADED_ACTION = "bodyLoaded",
        USING_TOOLBAR_ACTION = "usingToolbar",
        ENABLE_RECORDING_ACTION = "enableRecording",

        CountdownTimer,
        Recorder;

    function updateButtonState() {
        button.editProperties({ isActive: isRecordingEnabled || !Recorder.isIdle() });
    }

    CountdownTimer = (function () {
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

    Recorder = (function () {
        var IDLE = 0,
            COUNTING_DOWN = 1,
            RECORDING = 2,
            recordingState = IDLE,
            mappingPath;

        function setMappingCallback(status) {
            var error;

            if (status !== "") {
                error = "Error mapping recording to " + mappingPath + " on Asset Server!";
                print(APP_NAME + ": " + error + " - " + status);
                Window.alert(error);
                return;
            }

            print(APP_NAME + ": Recording mapped to " + mappingPath);
            print(APP_NAME + ": Request play recording");
            // TODO
        }

        function saveRecordingToAssetCallback(url) {
            var filename,
                hash,
                error;

            if (url === "") {
                error = "Error saving recording to Asset Server!";
                print(APP_NAME + ": " + error);
                Window.alert(error);
                return;
            }

            print(APP_NAME + ": Recording saved to Asset Server as " + url);

            filename = (new Date()).toISOString();  // yyyy-mm-ddThh:mm:ss.sssZ
            filename = filename.replace(/[-:]|\.\d*Z$/g, "").replace("T", "-") + ".hfr";  // yyyymmmdd-hhmmss.hfr
            hash = url.slice(4);  // Remove leading "atp:" from url.
            mappingPath = "/recordings/" + filename;
            Assets.setMapping(mappingPath, hash, setMappingCallback);
        }

        function startRecording() {
            recordingState = RECORDING;
            updateButtonState();
            print(APP_NAME + ": Start recording");
            Recording.startRecording();
        }

        function finishRecording() {
            var success,
                error;

            recordingState = IDLE;
            updateButtonState();
            print(APP_NAME + ": Finish recording");
            Recording.stopRecording();
            success = Recording.saveRecordingToAsset(saveRecordingToAssetCallback);
            if (!success) {
                error = "Error saving recording to Asset Server!";
                print(APP_NAME + ": " + error);
                Window.alert(error);
            }
        }

        function cancelRecording() {
            Recording.stopRecording();
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

        function isIdle() {
            return recordingState === IDLE;
        }

        function isCountingDown() {
            return recordingState === COUNTING_DOWN;
        }

        function isRecording() {
            return recordingState === RECORDING;
        }

        return {
            startCountdown: startCountdown,
            cancelCountdown: cancelCountdown,
            startRecording: startRecording,
            cancelRecording: cancelRecording,
            finishRecording: finishRecording,
            isIdle: isIdle,
            isCountingDown: isCountingDown,
            isRecording: isRecording
        };
    }());

    function usingToolbar() {
        return ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar"))
            || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar")));
    }

    function onTabletScreenChanged(type, url) {
        // Open/close dialog in tablet or window.

        var RECORD_URL = "/scripts/system/html/record.html",
            HOME_URL = "Tablet.qml";

        if (type === "Home" && url === HOME_URL) {
            // Start countdown if using toolbar and recording is enabled.
            if (usingToolbar() && isRecordingEnabled && Recorder.isIdle()) {
                Recorder.startCountdown();
            }
            isDialogDisplayed = false;
        } else if (type === "Web" && url.slice(-RECORD_URL.length) === RECORD_URL) {
            // Cancel countdown or finish recording.
            if (Recorder.isCountingDown()) {
                Recorder.cancelCountdown();
            } else if (Recorder.isRecording()) {
                Recorder.finishRecording();
            }
        }
    }

    function onTabletShownChanged() {
        // Open/close tablet.
        isDialogDisplayed = false;

        if (!tablet.tabletShown) {
            // Start countdown if recording is enabled.
            if (isRecordingEnabled && Recorder.isIdle()) {
                Recorder.startCountdown();
            }
        } else {
            // Cancel countdown or finish recording.
            if (Recorder.isCountingDown()) {
                Recorder.cancelCountdown();
            } else if (Recorder.isRecording()) {
                Recorder.finishRecording();
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
        if (!tablet) {
            return;
        }

        tablet.webEventReceived.disconnect(onWebEventReceived);
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
