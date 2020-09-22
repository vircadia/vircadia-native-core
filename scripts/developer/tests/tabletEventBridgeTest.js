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
    text: "SOUNDS",
    icon: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/trombone-i.png"),
    activeIcon: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/trombone-a.png")
});

var WEB_BRIDGE_TEST_HTML = Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/webBridgeTest.html?2");
var TROMBONE_URL = Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/audio/sad-trombone.wav");
var SCREAM_URL = Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/audio/wilhelm-scream.wav");

tabletButton.clicked.connect(function () {
    if (shown) {
        tablet.gotoHomeScreen();
    } else {
        tablet.gotoWebScreen(WEB_BRIDGE_TEST_HTML);
    }
});

var shown = false;

function onScreenChanged(type, url) {
    if (type === "Web" && url === WEB_BRIDGE_TEST_HTML) {
        tabletButton.editProperties({isActive: true});

        if (!shown) {
            // hook up to the event bridge
            tablet.webEventReceived.connect(onWebEventReceived);
        }
        shown = true;
    } else {
        tabletButton.editProperties({isActive: false});

        if (shown) {
            // disconnect from the event bridge
            tablet.webEventReceived.disconnect(onWebEventReceived);
        }
        shown = false;
    }
}

tablet.screenChanged.connect(onScreenChanged);

// ctor
function SoundBuddy(url) {
    this.sound = SoundCache.getSound(url);
    this.injector = null;
}

SoundBuddy.prototype.play = function (options, doneCallback) {
    if (this.sound.downloaded) {
        if (this.injector) {
            this.injector.setOptions(options);
            this.injector.restart();
        } else {
            this.injector = Audio.playSound(this.sound, options);
            this.injector.finished.connect(function () {
                if (doneCallback) {
                    doneCallback();
                }
            });
        }
    }
};

var tromboneSound = new SoundBuddy(TROMBONE_URL);
var screamSound = new SoundBuddy(SCREAM_URL);
var soundOptions = { position: MyAvatar.position, volume: 1.0, loop: false, localOnly: true };

function onWebEventReceived(msg) {
    Script.print("HIFI: recv web event = " + JSON.stringify(msg));
    if (msg === "button-1-play") {
        soundOptions.position = MyAvatar.position;
        tromboneSound.play(soundOptions, function () {
            tablet.emitScriptEvent("button-1-done");
        });
    } else if (msg === "button-2-play") {
        soundOptions.position = MyAvatar.position;
        screamSound.play(soundOptions, function () {
            tablet.emitScriptEvent("button-2-done");
        });
    }
}

Script.scriptEnding.connect(function () {
    tablet.removeButton(tabletButton);
    if (shown) {
        tablet.webEventReceived.disconnect(onWebEventReceived);
    }
});

