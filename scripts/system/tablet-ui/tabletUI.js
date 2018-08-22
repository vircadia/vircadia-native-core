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
   MyAvatar, Menu, AvatarInputs, Vec3, cleanUpOldMaterialEntities */

(function() { // BEGIN LOCAL_SCOPE
    var tabletRezzed = false;
    var activeHand = null;
    var DEFAULT_WIDTH = 0.4375;
    var DEFAULT_TABLET_SCALE = 70;
    var preMakeTime = Date.now();
    var validCheckTime = Date.now();
    var debugTablet = false;
    var tabletScalePercentage = 70.0;
    var UIWebTablet = null;
    var MSECS_PER_SEC = 1000.0;
    var MUTE_MICROPHONE_MENU_ITEM = "Mute Microphone";
    var gTablet = null;

    Script.include("../libraries/WebTablet.js");

    function cleanupMaterialEntities() {
        if (Window.isPhysicsEnabled()) {
            cleanUpOldMaterialEntities();
            return;
        }
        Script.setTimeout(cleanupMaterialEntities, 100);
    }

    function checkTablet() {
        if (gTablet === null) {
            gTablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        }
    }

    function tabletIsValid() {
        if (!UIWebTablet) {
            return false;
        }
        if (Overlays.getProperty(HMD.tabletID, "type") !== "model") {
            if (debugTablet) {
                print("TABLET is invalid due to frame: " + JSON.stringify(Overlays.getProperty(HMD.tabletID, "type")));
            }
            return false;
        }
        if (Overlays.getProperty(HMD.homeButtonID, "type") !== "circle3d" ||
                Overlays.getProperty(HMD.tabletScreenID, "type") !== "web3d") {
            if (debugTablet) {
                print("TABLET is invalid due to other");
            }
            return false;
        }
        return true;
    }

    function getTabletScalePercentageFromSettings() {
        checkTablet()
        var toolbarMode = gTablet.toolbarMode;
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

    function updateTabletWidthFromSettings(force) {
        var newTabletScalePercentage = getTabletScalePercentageFromSettings();
        if ((force || (newTabletScalePercentage !== tabletScalePercentage)) && UIWebTablet) {
            tabletScalePercentage = newTabletScalePercentage;
            UIWebTablet.setWidth(DEFAULT_WIDTH * (tabletScalePercentage / 100));
        }
    }

    function onHmdChanged() {
        updateTabletWidthFromSettings();
    }

    function onSensorToWorldScaleChanged(sensorScaleFactor) {
        if (HMD.active) {
            var newTabletScalePercentage = getTabletScalePercentageFromSettings();
            resizeTablet(DEFAULT_WIDTH * (newTabletScalePercentage / 100), undefined, sensorScaleFactor);
        }
    }

    function rezTablet() {
        if (debugTablet) {
            print("TABLET rezzing");
        }
        checkTablet()

        tabletScalePercentage = getTabletScalePercentageFromSettings();
        UIWebTablet = new WebTablet("hifi/tablet/TabletRoot.qml",
                                    DEFAULT_WIDTH * (tabletScalePercentage / 100),
                                    null, activeHand, true, null, false);
        UIWebTablet.register();
        HMD.tabletID = UIWebTablet.tabletEntityID;
        HMD.homeButtonID = UIWebTablet.homeButtonID;
        HMD.tabletScreenID = UIWebTablet.webOverlayID;
        HMD.homeButtonHighlightID = UIWebTablet.homeButtonHighlightID;
        HMD.displayModeChanged.connect(onHmdChanged);
        MyAvatar.sensorToWorldScaleChanged.connect(onSensorToWorldScaleChanged);

        tabletRezzed = true;
    }

    function showTabletUI() {
        checkTablet();

        if (!tabletRezzed || !tabletIsValid()) {
            closeTabletUI();
            rezTablet();
        }

        if (UIWebTablet && tabletRezzed) {
            if (debugTablet) {
                print("TABLET in showTabletUI, already rezzed");
            }
            var tabletProperties = {};
            if (!HMD.tabletContextualMode) { // contextual mode forces tablet in place -> don't update attachment
                UIWebTablet.calculateTabletAttachmentProperties(activeHand, true, tabletProperties);
            }
            tabletProperties.visible = true;
            Overlays.editOverlay(HMD.tabletID, tabletProperties);
            Overlays.editOverlay(HMD.homeButtonID, { visible: true });
            Overlays.editOverlay(HMD.homeButtonHighlightID, { visible: true });
            Overlays.editOverlay(HMD.tabletScreenID, { visible: true });
            Overlays.editOverlay(HMD.tabletScreenID, { maxFPS: 90 });
            updateTabletWidthFromSettings(true);
        }
        gTablet.tabletShown = true;
    }

    function hideTabletUI() {
        checkTablet()
        gTablet.tabletShown = false;
        if (!UIWebTablet) {
            return;
        }

        if (debugTablet) {
            print("TABLET hide");
        }

        Overlays.editOverlay(HMD.tabletID, { visible: false });
        Overlays.editOverlay(HMD.homeButtonID, { visible: false });
        Overlays.editOverlay(HMD.homeButtonHighlightID, { visible: false });
        Overlays.editOverlay(HMD.tabletScreenID, { visible: false });
        Overlays.editOverlay(HMD.tabletScreenID, { maxFPS: 1 });
    }

    function closeTabletUI() {
        checkTablet();
        gTablet.tabletShown = false;
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
            HMD.homeButtonHighlightID = null;
            HMD.tabletScreenID = null;
        } else if (debugTablet) {
            print("TABLET closeTabletUI, UIWebTablet is null");
        }
        tabletRezzed = false;
        gTablet = null;
    }


    function updateShowTablet() {
        var now = Date.now();

        checkTablet();

        // close the WebTablet if it we go into toolbar mode.
        var tabletShown = gTablet.tabletShown;
        var toolbarMode = gTablet.toolbarMode;
        var landscape = gTablet.landscape;

        if (tabletShown && toolbarMode) {
            closeTabletUI();
            HMD.closeTablet();
            return;
        }

        var needInstantUpdate = UIWebTablet && UIWebTablet.getLandscape() !== landscape;

        if ((now - validCheckTime > MSECS_PER_SEC) || needInstantUpdate) {
            validCheckTime = now;

            updateTabletWidthFromSettings();

            if (UIWebTablet) {
                UIWebTablet.setLandscape(landscape);
            }
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
                checkTablet();
                gTablet.landscape = false;
            }
        }
    }

    Messages.subscribe("toggleHand");
    Messages.subscribe("home");
    Messages.messageReceived.connect(handleMessage);

    var clickMapping = Controller.newMapping('tabletToggle-click');
    var wantsMenu = 0;
    clickMapping.from(function () { return wantsMenu; }).to(Controller.Actions.ContextMenu);
    clickMapping.from(Controller.Standard.RightSecondaryThumb).peek().to(function (clicked) {
    if (clicked) {
        //activeHudPoint2d(Controller.Standard.RightHand);
        Messages.sendLocalMessage("toggleHand", Controller.Standard.RightHand);
    }
        wantsMenu = clicked;
    });
    
    clickMapping.from(Controller.Standard.LeftSecondaryThumb).peek().to(function (clicked) {
        if (clicked) {
            //activeHudPoint2d(Controller.Standard.LeftHand);
            Messages.sendLocalMessage("toggleHand", Controller.Standard.LeftHand);
        }
        wantsMenu = clicked;
    });
    
    clickMapping.from(Controller.Standard.Start).peek().to(function (clicked) {
    if (clicked) {
        //activeHudPoint2dGamePad();
        var noHands = -1;
        Messages.sendLocalMessage("toggleHand", Controller.Standard.LeftHand);
    }

        wantsMenu = clicked;
    });
    clickMapping.enable();

    Script.setInterval(updateShowTablet, 100);

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
        HMD.homeButtonHighlightID = null;
        HMD.tabletScreenID = null;
    });
    Script.setTimeout(cleanupMaterialEntities, 100);
}()); // END LOCAL_SCOPE
