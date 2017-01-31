//
//  tabletEventBridgeTest.js
//
//  Created by Anthony J. Thibault on 2016-12-15
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Adds a button to the tablet that will switch to a web page.
// This web page contains buttons that will use the event bridge to trigger sounds.

/* globals Tablet */


var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var tabletButton = tablet.addButton({
    text: "SOUNDS"
});

var WEB_BRIDGE_TEST_HTML = "https://s3.amazonaws.com/hifi-public/tony/webBridgeTest.html?2";

var TROMBONE_URL = "https://s3.amazonaws.com/hifi-public/tony/audio/sad-trombone.wav";
var tromboneSound = SoundCache.getSound(TROMBONE_URL);
var tromboneInjector;

var SCREAM_URL = "https://s3.amazonaws.com/hifi-public/tony/audio/wilhelm-scream.wav";
var screamSound = SoundCache.getSound(SCREAM_URL);
var screamInjector;

tabletButton.clicked.connect(function () {
    tablet.gotoWebScreen(WEB_BRIDGE_TEST_HTML);
});

// hook up to the event bridge
tablet.webEventReceived.connect(function (msg) {
    Script.print("HIFI: recv web event = " + JSON.stringify(msg));
    if (msg === "button-1-play") {

        // play sad trombone
        if (tromboneSound.downloaded) {
            if (tromboneInjector) {
                tromboneInjector.restart();
            } else {
                tromboneInjector = Audio.playSound(tromboneSound, { position: MyAvatar.position,
                                                                    volume: 1.0,
                                                                    loop: false });
            }
        }

        // wait until sound is finished then send a done event
        Script.setTimeout(function () {
            tablet.emitScriptEvent("button-1-done");
        }, 3500);
    }

    if (msg === "button-2-play") {

        // play scream
        if (screamSound.downloaded) {
            if (screamInjector) {
                screamInjector.restart();
            } else {
                screamInjector = Audio.playSound(screamSound, { position: MyAvatar.position,
                                                                volume: 1.0,
                                                                loop: false });
            }
        }

        // wait until sound is finished then send a done event
        Script.setTimeout(function () {
            tablet.emitScriptEvent("button-2-done");
        }, 1000);
    }
});

Script.scriptEnding.connect(function () {
    tablet.removeButton(tabletButton);
});

