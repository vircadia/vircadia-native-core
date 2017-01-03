"use strict";

//
//  tabletUI.js
//
//  scripts/system/tablet-ui/
//
//  Created by Seth Alves 2016-9-29
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Script, HMD, WebTablet, UIWebTablet */

(function() { // BEGIN LOCAL_SCOPE
    var tabletShown = false;
    var tabletLocation = null;

    Script.include("../libraries/WebTablet.js");

    function showTabletUI() {
        tabletShown = true;
        print("show tablet-ui");
        UIWebTablet = new WebTablet("qml/hifi/tablet/TabletRoot.qml", null, null, tabletLocation);
        UIWebTablet.register();
        HMD.tabletID = UIWebTablet.webEntityID;

        var setUpTabletUI = function() {
            var root = UIWebTablet.getRoot();
            if (!root) {
                print("HERE no root yet");
                Script.setTimeout(setUpTabletUI, 100);
                return;
            }
            print("HERE got root", root);
        }

        Script.setTimeout(setUpTabletUI, 100);
    }

    function hideTabletUI() {
        tabletShown = false;
        print("hide tablet-ui");
        if (UIWebTablet) {
            if (UIWebTablet.onClose) {
                UIWebTablet.onClose();
            }

            tabletLocation = UIWebTablet.getLocation();
            UIWebTablet.unregister();
            UIWebTablet.destroy();
            UIWebTablet = null;
            HMD.tabletID = null;
        }
    }

    function updateShowTablet() {
        if (HMD.showTablet && !tabletShown) {
            showTabletUI();
        } else if (!HMD.showTablet && tabletShown) {
            hideTabletUI();
        }
    }

    Script.update.connect(updateShowTablet);
    // Script.setInterval(updateShowTablet, 1000);

    // Initialise variables used to calculate audio level
    var accumulatedLevel = 0.0;
    // Note: Might have to tweak the following two based on the rate we're getting the data
    var AVERAGING_RATIO = 0.05;
    var AUDIO_LEVEL_UPDATE_INTERVAL_MS = 100;

    // Calculate audio level with the same scaling equation (log scale, exponentially averaged) in AvatarInputs and pal.js
    function getAudioLevel() {
        var LOUDNESS_FLOOR = 11.0;
        var LOUDNESS_SCALE = 2.8 / 5.0;
        var LOG2 = Math.log(2.0);
        var audioLevel = 0.0;
        accumulatedLevel = AVERAGING_RATIO * accumulatedLevel + (1 - AVERAGING_RATIO) * (MyAvatar.audioLoudness);
        // Convert to log base 2
        var logLevel = Math.log(accumulatedLevel + 1) / LOG2;
        
        if (logLevel <= LOUDNESS_FLOOR) {
            audioLevel = logLevel / LOUDNESS_FLOOR * LOUDNESS_SCALE;
        } else {
            audioLevel = (logLevel - (LOUDNESS_FLOOR - 1.0)) * LOUDNESS_SCALE;
        }
        if (audioLevel > 1.0) {
            audioLevel = 1.0;
        }
        return audioLevel;
    }

    Script.setInterval(function() {
        var currentAudioLevel = getAudioLevel();
        // TODO: send the audio level to QML

    }, AUDIO_LEVEL_UPDATE_INTERVAL_MS);

}()); // END LOCAL_SCOPE
