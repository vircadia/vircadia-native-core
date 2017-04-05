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
        EVENT_BRIDGE_TYPE = "record";

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

    function onTabletShownChanged() {
        if (tablet.tabletShown) {
            // Finish recording if is recording.
            if (isRecording) {
                finishRecording();
                button.editProperties({ isActive: isRecordingEnabled || isRecording });
            }
        } else {
            // Start recording if recording is enabled.
            if (isRecordingEnabled) {
                startRecording();
                button.editProperties({ isActive: isRecordingEnabled || isRecording });
            }
        }
    }

    function onWebEventReceived(data) {
        var message = JSON.parse(data);
        if (message.type === EVENT_BRIDGE_TYPE) {
            if (message.action === "bodyLoaded") {
                tablet.emitScriptEvent(JSON.stringify({
                    type: EVENT_BRIDGE_TYPE,
                    action: "enableRecording",
                    value: isRecordingEnabled
                }));

            } else if (message.action === "enableRecording") {
                isRecordingEnabled = message.value;
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

        // Track showing/hiding tablet.
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
        button.clicked.disconnect(onButtonClicked);
        tablet.removeButton(button);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());
