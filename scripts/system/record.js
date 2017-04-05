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
        isRecordingEnabled = false,
        isRecording = false,
        tablet,
        button,
        EVENT_BRIDGE_TYPE = "record",
        BODY_LOADED_ACTION = "bodyLoaded",
        ENABLE_RECORDING_ACTION = "enableRecording";

    function startRecording() {
        isRecording = true;
        print("Start recording");
    }

    function finishRecording() {
        isRecording = false;
        print("Finish recording");
    }

    function abandonRecording() {
        isRecording = false;
        print("Abandon recording");
    }

    function onTabletScreenChanged(type, url) {
        // Open/close dialog in tablet or window.

        var RECORD_URL = "/scripts/system/html/record.html",
            HOME_URL = "Tablet.qml";

        if (type === "Home" && url === HOME_URL) {
            // Start recording if recording is enabled.
            if (!isRecording && isRecordingEnabled) {
                startRecording();
                button.editProperties({ isActive: isRecordingEnabled || isRecording });
            }
        } else if (type === "Web" && url.slice(-RECORD_URL.length) === RECORD_URL) {
            // Finish recording if is recording.
            if (isRecording) {
                finishRecording();
                button.editProperties({ isActive: isRecordingEnabled || isRecording });
            }
        }
    }

    function onTabletShownChanged() {
        // Open/close tablet.

        if (!tablet.tabletShown) {
            // Start recording if recording is enabled.
            if (!isRecording && isRecordingEnabled) {
                startRecording();
                button.editProperties({ isActive: isRecordingEnabled || isRecording });
            }
        } else {
            // Finish recording if is recording.
            if (isRecording) {
                finishRecording();
                button.editProperties({ isActive: isRecordingEnabled || isRecording });
            }
        }
    }

    function onWebEventReceived(data) {
        var message = JSON.parse(data);
        if (message.type === EVENT_BRIDGE_TYPE) {
            if (message.action === BODY_LOADED_ACTION) {
                tablet.emitScriptEvent(JSON.stringify({
                    type: EVENT_BRIDGE_TYPE,
                    action: ENABLE_RECORDING_ACTION,
                    value: isRecordingEnabled
                }));
            } else if (message.action === ENABLE_RECORDING_ACTION) {
                isRecordingEnabled = message.value;
                button.editProperties({ isActive: isRecordingEnabled || isRecording });
            }
        }
    }

    function onButtonClicked() {
        tablet.gotoWebScreen(APP_URL);
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
            isActive: isRecordingEnabled || isRecording
        });
        button.clicked.connect(onButtonClicked);

        // UI communications.
        tablet.webEventReceived.connect(onWebEventReceived);

        // Track showing/hiding tablet/dialog.
        tablet.screenChanged.connect(onTabletScreenChanged);
        tablet.tabletShownChanged.connect(onTabletShownChanged);
    }

    function tearDown() {
        if (isRecording) {
            abandonRecording();
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
