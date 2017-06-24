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

    Recorder = (function () {
        var IDLE = 0,
            COUNTING_DOWN = 1,
            RECORDING = 2,
            recordingState = IDLE;

        function isRecording() {
            return recordingState === COUNTING_DOWN || recordingState === RECORDING;
        }

        function startRecording() {

            if (recordingState !== IDLE) {
                return;
            }

            // TODO

            recordingState = COUNTING_DOWN;

        }

        function cancelRecording() {

            // TODO

            recordingState = IDLE;
        }

        function finishRecording() {

            // TODO

            recordingState = IDLE;
        }

        function stopRecording() {
            if (recordingState === COUNTING_DOWN) {
                cancelRecording();
            } else {
                finishRecording();
            }
        }

        function setUp() {

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
