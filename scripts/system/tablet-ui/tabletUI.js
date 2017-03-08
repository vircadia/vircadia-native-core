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
    var tabletShown = false;
    var tabletRezzed = false;
    var tabletLocation = null;
    var activeHand = null;
    var DEFAULT_WIDTH = 0.4375;
    var DEFAULT_TABLET_SCALE = 100;
    UIWebTablet = null;

    Script.include("../libraries/WebTablet.js");

    function rezTablet(invisible) {
        print("XXX show tablet-ui, not rezzed.  position is " +  JSON.stringify(tabletLocation));
        var toolbarMode = Tablet.getTablet("com.highfidelity.interface.tablet.system").toolbarMode;
        var TABLET_SCALE = DEFAULT_TABLET_SCALE;
        if (toolbarMode) {
            TABLET_SCALE = Settings.getValue("desktopTabletScale") || DEFAULT_TABLET_SCALE;
        } else {
            TABLET_SCALE = Settings.getValue("hmdTabletScale") || DEFAULT_TABLET_SCALE;
        }

        UIWebTablet = new WebTablet("qml/hifi/tablet/TabletRoot.qml",
                                    DEFAULT_WIDTH * (TABLET_SCALE / 100),
                                    null, activeHand, true, tabletLocation, invisible);
        UIWebTablet.register();
        HMD.tabletID = UIWebTablet.tabletEntityID;
        HMD.homeButtonID = UIWebTablet.homeButtonID;
        HMD.tabletScreenID = UIWebTablet.webOverlayID;
        tabletRezzed = true;
    }

    function showTabletUI() {
        tabletShown = true;

        if (!tabletRezzed) {
            rezTablet(false);
        }

        if (UIWebTablet && tabletRezzed) {
            if (tabletLocation) {
                print("XXX show tablet-ui, already rezzed, already position: " + JSON.stringify(tabletLocation));
                Overlays.editOverlay(HMD.tabletID, {
                    localPosition: tabletLocation.localPosition,
                    localRotation: tabletLocation.localRotation,
                    visible: true
                });
            } else {
                print("XXX show tablet-ui, already rezzed, no position");
                var tabletProperties = {};
                UIWebTablet.calculateTabletAttachmentProperties(activeHand, true, tabletProperties);
                tabletProperties.visible = true;
                Overlays.editOverlay(HMD.tabletID, tabletProperties);
            }
            Overlays.editOverlay(HMD.homeButtonID, { visible: true });
            Overlays.editOverlay(HMD.tabletScreenID, { visible: true });
        }
    }

    function hideTabletUI() {
        tabletShown = false;
        if (UIWebTablet) {
            tabletLocation = UIWebTablet.getLocation();
            print("XXX hide tablet-ui, position was " +  JSON.stringify(tabletLocation));
            // Overlays.editOverlay(HMD.tabletID, { localPosition: { x: -1000, y: 0, z:0 } });
            Overlays.editOverlay(HMD.tabletID, { visible: false });
            Overlays.editOverlay(HMD.homeButtonID, { visible: false });
            Overlays.editOverlay(HMD.tabletScreenID, { visible: false });
        } else {
            print("XXX hide tablet-ui, UIWebTablet is null");
        }
    }

    function closeTabletUI() {
        tabletShown = false;
        if (UIWebTablet) {
            if (UIWebTablet.onClose) {
                UIWebTablet.onClose();
            }

            tabletLocation = UIWebTablet.getLocation();
            print("XXX close tablet-ui, position was " +  JSON.stringify(tabletLocation));
            UIWebTablet.unregister();
            UIWebTablet.destroy();
            UIWebTablet = null;
            HMD.tabletID = null;
            HMD.homeButtonID = null;
            HMD.tabletScreenID = null;
        } else {
            print("XXX close tablet-ui, UIWebTablet is null");
        }
        tabletRezzed = false;
    }


    function updateShowTablet() {

        // close the WebTablet if it we go into toolbar mode.
        var toolbarMode = Tablet.getTablet("com.highfidelity.interface.tablet.system").toolbarMode;
        var visibleToOthers = Settings.getValue("tabletVisibleToOthers");

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

        // XXX don't do this check every time?
        if (tabletRezzed && UIWebTablet && Overlays.getOverlayType(UIWebTablet.webOverlayID) != "web3d") {
            // when we switch domains, the tablet entity gets destroyed and recreated.  this causes
            // the overlay to be deleted, but not recreated.  If the overlay is deleted for this or any
            // other reason, close the tablet.
            closeTabletUI();
            HMD.closeTablet();
            print("XXX autodestroying tablet");
        }


        if (HMD.showTablet && !tabletShown && !toolbarMode) {
            UserActivityLogger.openedTablet(visibleToOthers);
            showTabletUI();
        } else if (!HMD.showTablet && tabletShown) {
            UserActivityLogger.closedTablet();
            if (visibleToOthers) {
                closeTabletUI();
            } else {
                hideTabletUI();
            }
        } else if (!toolbarMode && !visibleToOthers && !tabletRezzed) {
            // pre-make the tablet so it will appear quickly
            tabletLocation = {
                localPosition: Vec3.sum(MyAvatar.position, { x: -1000, y: 0, z: 0 }),
                localRotation: { x: 0, y: 0, z: 0, w: 1 }
            };
            print("XXX pre-creating tablet at " + JSON.stringify(tabletLocation));
            rezTablet(true);
            tabletLocation = null;
            tabletShown = false;
        }
    }

    function toggleHand(channel, hand, senderUUID, localOnly) {
        if (channel === "toggleHand") {
            activeHand = JSON.parse(hand);
        }
    }

    Messages.subscribe("toggleHand");
    Messages.messageReceived.connect(toggleHand);

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
        Entities.deleteEntity(HMD.tabletID);
        HMD.tabletID = null;
        HMD.homeButtonID = null;
        HMD.tabletScreenID = null;
    });
}()); // END LOCAL_SCOPE
