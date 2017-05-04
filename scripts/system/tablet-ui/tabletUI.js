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

/* global Script, HMD, WebTablet, UIWebTablet, UserActivityLogger, Settings, Entities, Messages, Tablet, Overlays,
   MyAvatar, Menu, Vec3 */

(function() { // BEGIN LOCAL_SCOPE
    var tabletRezzed = false;
    var activeHand = null;
    var DEFAULT_WIDTH = 0.4375;
    var DEFAULT_TABLET_SCALE = 100;
    var preMakeTime = Date.now();
    var validCheckTime = Date.now();
    var debugTablet = false;
    var tabletScalePercentage = 100.0;
    UIWebTablet = null;

    Script.include("../libraries/WebTablet.js");

    function tabletIsValid() {
        if (!UIWebTablet) {
            return false;
        }
        if (Overlays.getProperty(HMD.tabletID, "type") != "model") {
            if (debugTablet) {
                print("TABLET is invalid due to frame: " + JSON.stringify(Overlays.getProperty(HMD.tabletID, "type")));
            }
            return false;
        }
        if (Overlays.getProperty(HMD.homeButtonID, "type") != "sphere" ||
            Overlays.getProperty(HMD.tabletScreenID, "type") != "web3d") {
            if (debugTablet) {
                print("TABLET is invalid due to other");
            }
            return false;
        }
        return true;
    }

    function getTabletScalePercentageFromSettings() {
        var toolbarMode = Tablet.getTablet("com.highfidelity.interface.tablet.system").toolbarMode;
        var tabletScalePercentage = DEFAULT_TABLET_SCALE;
        if (!toolbarMode) {
            if (HMD.active) {
                tabletScalePercentage = Settings.getValue("hmdTabletScale") || DEFAULT_TABLET_SCALE;
            } else {
                tabletScalePercentage = Settings.getValue("desktopTabletScale") || DEFAULT_TABLET_SCALE;
            }
        }
        return tabletScalePercentage;
    }

    function updateTabletWidthFromSettings() {
        var newTabletScalePercentage = getTabletScalePercentageFromSettings();
        if (newTabletScalePercentage !== tabletScalePercentage && UIWebTablet) {
            tabletScalePercentage = newTabletScalePercentage;
            UIWebTablet.setWidth(DEFAULT_WIDTH * (tabletScalePercentage / 100));
        }
    }

    function onHmdChanged() {
        updateTabletWidthFromSettings();
    }

    function rezTablet() {
        if (debugTablet) {
            print("TABLET rezzing");
        }

        tabletScalePercentage = getTabletScalePercentageFromSettings();
        UIWebTablet = new WebTablet("qml/hifi/tablet/TabletRoot.qml",
                                    DEFAULT_WIDTH * (tabletScalePercentage / 100),
                                    null, activeHand, true);
        UIWebTablet.register();
        HMD.tabletID = UIWebTablet.tabletEntityID;
        HMD.homeButtonID = UIWebTablet.homeButtonID;
        HMD.tabletScreenID = UIWebTablet.webOverlayID;
        HMD.displayModeChanged.connect(onHmdChanged);

        tabletRezzed = true;
    }

    function showTabletUI() {
        Tablet.getTablet("com.highfidelity.interface.tablet.system").tabletShown = true;

        if (!tabletRezzed || !tabletIsValid()) {
            closeTabletUI();
            rezTablet();
        }

        if (UIWebTablet && tabletRezzed) {
            if (debugTablet) {
                print("TABLET in showTabletUI, already rezzed");
            }
            var tabletProperties = {};
            UIWebTablet.calculateTabletAttachmentProperties(activeHand, true, tabletProperties);
            tabletProperties.visible = true;
            Overlays.editOverlay(HMD.tabletID, tabletProperties);
            Overlays.editOverlay(HMD.homeButtonID, { visible: true });
            Overlays.editOverlay(HMD.tabletScreenID, { visible: true });
            Overlays.editOverlay(HMD.tabletScreenID, { maxFPS: 90 });
        }
    }

    function hideTabletUI() {
        Tablet.getTablet("com.highfidelity.interface.tablet.system").tabletShown = false;
        if (!UIWebTablet) {
            return;
        }

        if (debugTablet) {
            print("TABLET hide");
        }

        Overlays.editOverlay(HMD.tabletID, { visible: false });
        Overlays.editOverlay(HMD.homeButtonID, { visible: false });
        Overlays.editOverlay(HMD.tabletScreenID, { visible: false });
        Overlays.editOverlay(HMD.tabletScreenID, { maxFPS: 1 });
    }

    function closeTabletUI() {
        Tablet.getTablet("com.highfidelity.interface.tablet.system").tabletShown = false;
        if (UIWebTablet) {
            if (UIWebTablet.onClose) {
                UIWebTablet.onClose();
            }

            if (debugTablet) {
                print("TABLET close");
            }
            UIWebTablet.unregister();
            UIWebTablet.destroy();
            UIWebTablet = null;
            HMD.tabletID = null;
            HMD.homeButtonID = null;
            HMD.tabletScreenID = null;
        } else if (debugTablet) {
            print("TABLET closeTabletUI, UIWebTablet is null");
        }
        tabletRezzed = false;
    }


    function updateShowTablet() {
        var MSECS_PER_SEC = 1000.0;
        var now = Date.now();

        // close the WebTablet if it we go into toolbar mode.
        var tabletShown = Tablet.getTablet("com.highfidelity.interface.tablet.system").tabletShown;
        var toolbarMode = Tablet.getTablet("com.highfidelity.interface.tablet.system").toolbarMode;
        var landscape = Tablet.getTablet("com.highfidelity.interface.tablet.system").landscape;

        if (tabletShown && toolbarMode) {
            closeTabletUI();
            HMD.closeTablet();
            return;
        }

        if (tabletShown) {
            var MUTE_MICROPHONE_MENU_ITEM = "Mute Microphone";
            var currentMicEnabled = !Menu.isOptionChecked(MUTE_MICROPHONE_MENU_ITEM);
            var currentMicLevel = getMicLevel();
            var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
            tablet.updateMicEnabled(currentMicEnabled);
            tablet.updateAudioBar(currentMicLevel);
        }

        updateTabletWidthFromSettings();
        if (UIWebTablet) {
            UIWebTablet.setLandscape(landscape);
        }

        if (validCheckTime - now > MSECS_PER_SEC) {
            validCheckTime = now;
            if (tabletRezzed && UIWebTablet && !tabletIsValid()) {
                // when we switch domains, the tablet entity gets destroyed and recreated.  this causes
                // the overlay to be deleted, but not recreated.  If the overlay is deleted for this or any
                // other reason, close the tablet.
                closeTabletUI();
                HMD.closeTablet();
                if (debugTablet) {
                    print("TABLET autodestroying");
                }
            }
        }

        // check for change in tablet scale.

        if (HMD.showTablet && !tabletShown && !toolbarMode) {
            UserActivityLogger.openedTablet(false);
            showTabletUI();
        } else if (!HMD.showTablet && tabletShown) {
            UserActivityLogger.closedTablet();
            hideTabletUI();
        }

        // if the tablet is an overlay, attempt to pre-create it and then hide it so that when it's
        // summoned, it will appear quickly.
        if (!toolbarMode) {
            if (now - preMakeTime > MSECS_PER_SEC) {
                preMakeTime = now;
                if (!tabletIsValid()) {
                    closeTabletUI();
                    rezTablet();
                    tabletShown = false;

                    // also cause the stylus model to be loaded
                    var tmpStylusID = Overlays.addOverlay("model", {
                        name: "stylus",
                        url: Script.resourcesPath() + "meshes/tablet-stylus-fat.fbx",
                        loadPriority: 10.0,
                        position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.1, z: -2})),
                        dimensions: { x: 0.01, y: 0.01, z: 0.2 },
                        solid: true,
                        visible: true,
                        ignoreRayIntersection: true,
                        drawInFront: false,
                        lifetime: 3
                    });
                    Script.setTimeout(function() {
                        Overlays.deleteOverlay(tmpStylusID);
                    }, 300);

                } else if (!tabletShown) {
                    hideTabletUI();
                }
            }
        }

    }

    function handleMessage(channel, hand, senderUUID, localOnly) {
        if (channel === "toggleHand") {
            activeHand = JSON.parse(hand);
        }
        if (channel === "home") {
            if (UIWebTablet) {
                Tablet.getTablet("com.highfidelity.interface.tablet.system").landscape = false;
            }
        }
    }

    Messages.subscribe("toggleHand");
    Messages.subscribe("home");
    Messages.messageReceived.connect(handleMessage);

    Script.setInterval(updateShowTablet, 100);

    // Initialise variables used to calculate audio level
    var accumulatedLevel = 0.0;
    // Note: Might have to tweak the following two based on the rate we're getting the data
    var AVERAGING_RATIO = 0.05;

    // Calculate microphone level with the same scaling equation (log scale, exponentially averaged) in AvatarInputs and pal.js
    function getMicLevel() {
        var LOUDNESS_FLOOR = 11.0;
        var LOUDNESS_SCALE = 2.8 / 5.0;
        var LOG2 = Math.log(2.0);
        var micLevel = 0.0;
        accumulatedLevel = AVERAGING_RATIO * accumulatedLevel + (1 - AVERAGING_RATIO) * (MyAvatar.audioLoudness);
        // Convert to log base 2
        var logLevel = Math.log(accumulatedLevel + 1) / LOG2;

        if (logLevel <= LOUDNESS_FLOOR) {
            micLevel = logLevel / LOUDNESS_FLOOR * LOUDNESS_SCALE;
        } else {
            micLevel = (logLevel - (LOUDNESS_FLOOR - 1.0)) * LOUDNESS_SCALE;
        }
        if (micLevel > 1.0) {
            micLevel = 1.0;
        }
        return micLevel;
    }

    Script.scriptEnding.connect(function () {

        // if we reload scripts in tablet mode make sure we close the currently open window, by calling gotoHomeScreen
        var tabletProxy = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        if (tabletProxy && tabletProxy.toolbarMode) {
            tabletProxy.gotoHomeScreen();
        }

        var tabletID = HMD.tabletID;
        Entities.deleteEntity(tabletID);
        Overlays.deleteOverlay(tabletID);
        HMD.tabletID = null;
        HMD.homeButtonID = null;
        HMD.tabletScreenID = null;
    });
}()); // END LOCAL_SCOPE
