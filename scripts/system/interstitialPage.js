//
//  interstitialPage.js
//  scripts/system
//
//  Created by Dante Ruiz on 08/02/2018.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Script, Controller, Overlays, Quat, MyAvatar, Entities, print, Vec3, AddressManager, Render, Window, Toolbars,
   Camera, HMD, location, Account*/

(function() {
    Script.include("/~/system/libraries/Xform.js");
    var DEBUG = true;
    var MAX_X_SIZE = 4;
    var EPSILON = 0.25;
    var isVisible = false;
    var STABILITY = 3.0;
    var VOLUME = 0.4;
    var tune = SoundCache.getSound("http://hifi-content.s3.amazonaws.com/alexia/LoadingScreens/crystals_and_voices.wav");
    var sample = null;
    var MAX_LEFT_MARGIN = 1.9;
    var INNER_CIRCLE_WIDTH = 4.7;
    var DESTINATION_CARD_Y_OFFSET = 2;
    var DEFAULT_Z_OFFSET = 5.45;

    var renderViewTask = Render.getConfig("RenderMainView");
    var request = Script.require('request').request;
    var BUTTON_PROPERTIES = {
        text: "Interstitial"
    };
    var tablet = null;
    var button = null;

    // Tips have a character limit of 69
    var userTips = [
        "Tip: Visit TheSpot to explore featured domains!",
        "Tip: Visit our docs online to learn more about scripting!",
        "Tip: Don't want others invading your personal space? Turn on the Bubble!",
        "Tip: Want to make a friend? Shake hands with them in VR!",
        "Tip: Enjoy live music? Visit Rust to dance your heart out!",
        "Tip: Have you visited BodyMart to check out the new avatars recently?",
        "Tip: Use the Create app to import models and create custom entities.",
        "Tip: We're open source! Feel free to contribute to our code on GitHub!",
        "Tip: What emotes have you used in the Emote app?",
        "Tip: Take and share your snapshots with the everyone using the Snap app.",
        "Tip: Did you know you can show websites in-world by creating a web entity?",
        "Tip: Find out more information about domains by visiting our website!",
        "Tip: Did you know you can get cool new apps from the Marketplace?",
        "Tip: Print your snapshots from the Snap app to share with others!",
        "Tip: Log in to make friends, visit new domains, and save avatars!"
    ];

    var DEFAULT_DIMENSIONS = { x: 20, y: 20, z: 20 };

    var loadingSphereID = Overlays.addOverlay("model", {
        name: "Loading-Sphere",
        position: Vec3.sum(Vec3.sum(MyAvatar.position, {x: 0.0, y: -1.0, z: 0.0}), Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.95, z: 0})),
        orientation: Quat.multiply(Quat.fromVec3Degrees({x: 0, y: 180, z: 0}), MyAvatar.orientation),
        url: "http://hifi-content.s3.amazonaws.com/alexia/LoadingScreens/black-sphere.fbx",
        dimensions: DEFAULT_DIMENSIONS,
        alpha: 1,
        visible: isVisible,
        ignoreRayIntersection: true,
        drawInFront: true,
        grabbable: false,
    });

    var anchorOverlay = Overlays.addOverlay("cube", {
        dimensions: {x: 0.2, y: 0.2, z: 0.2},
        visible: true,
        grabbable: false,
        ignoreRayIntersection: true,
        localPosition: {x: 0.0, y: getAnchorLocalYOffset(), z: DEFAULT_Z_OFFSET },
        orientation: Quat.multiply(Quat.fromVec3Degrees({x: 0, y: 180, z: 0}), MyAvatar.orientation),
        solid: true,
        drawInFront: true,
        parentID: loadingSphereID
    });


    var domainName = "Test";
    var domainNameTextID = Overlays.addOverlay("text3d", {
        name: "Loading-Destination-Card-Text",
        localPosition: { x: 0.0, y: 0.8, z: 0.0 },
        text: domainName,
        textAlpha: 1,
        backgroundAlpha: 0,
        lineHeight: 0.42,
        visible: isVisible,
        ignoreRayIntersection: true,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
        parentID: anchorOverlay
    });

    var domainText = "";
    var domainDescription = Overlays.addOverlay("text3d", {
        name: "Loading-Hostname",
        localPosition: { x: 0.0, y: 0.32, z: 0.0 },
        text: domainText,
        textAlpha: 1,
        backgroundAlpha: 0,
        lineHeight: 0.13,
        visible: isVisible,
        ignoreRayIntersection: true,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
        parentID: anchorOverlay
    });

    var toolTip = "";

    var domainToolTip = Overlays.addOverlay("text3d", {
        name: "Loading-Tooltip",
        localPosition: { x: 0.0 , y: -1.6, z: 0.0 },
        text: toolTip,
        textAlpha: 1,
        backgroundAlpha: 0,
        lineHeight: 0.13,
        visible: isVisible,
        ignoreRayIntersection: true,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
        parentID: anchorOverlay
    });

    var loadingToTheSpotID = Overlays.addOverlay("image3d", {
        name: "Loading-Destination-Card-Text",
        localPosition: { x: 0.0 , y: -1.8, z: 0.0 },
        url: "http://hifi-content.s3.amazonaws.com/alexia/LoadingScreens/goTo_button.png",
        alpha: 1,
        dimensions: { x: 1.2, y: 0.6},
        visible: isVisible,
        emissive: true,
        ignoreRayIntersection: false,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0.0, y: 180.0, z: 0.0 }),
        parentID: anchorOverlay
    });

    var loadingBarPlacard = Overlays.addOverlay("image3d", {
        name: "Loading-Bar-Placard",
        localPosition: { x: 0.0, y: -0.99, z: 0.0 },
        url: "http://hifi-content.s3.amazonaws.com/alexia/LoadingScreens/loadingBar_placard.png",
        alpha: 1,
        dimensions: { x: 4, y: 2.8},
        visible: isVisible,
        emissive: true,
        ignoreRayIntersection: false,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0.0, y: 180.0, z: 0.0 }),
        parentID: anchorOverlay
    });

    var loadingBarProgress = Overlays.addOverlay("image3d", {
        name: "Loading-Bar-Progress",
        localPosition: { x: 0.0, y: -0.99, z: 0.0 },
        url: Script.resourcesPath() + "images/loadingBar_v1.png",
        alpha: 1,
        dimensions: {x: 4, y: 2.8},
        visible: isVisible,
        emissive: true,
        ignoreRayIntersection: false,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0.0, y: 180.0, z: 0.0 }),
        parentID: anchorOverlay
    });

    var TARGET_UPDATE_HZ = 60; // 50hz good enough, but we're using update
    var BASIC_TIMER_INTERVAL_MS = 1000 / TARGET_UPDATE_HZ;
    var timerset = false;
    var lastInterval = Date.now();
    var timeElapsed = 0;
    var currentDomain = "";
    var timer = null;
    var target = 0;

    var connectionToDomainFailed = false;


    function getAnchorLocalYOffset() {
        var loadingSpherePosition = Overlays.getProperty(loadingSphereID, "position");
        var loadingSphereOrientation = Overlays.getProperty(loadingSphereID, "rotation");
        var overlayXform = new Xform(loadingSphereOrientation, loadingSpherePosition);
        var worldToOverlayXform = overlayXform.inv();
        var headPosition = MyAvatar.getHeadPosition();
        var headPositionInOverlaySpace = worldToOverlayXform.xformPoint(headPosition);
        return headPositionInOverlaySpace.y;
    }

    function getLeftMargin(overlayID, text) {
        var textSize = Overlays.textSize(overlayID, text);
        var sizeDifference = ((INNER_CIRCLE_WIDTH - textSize.width) / 2);
        var leftMargin = -(MAX_LEFT_MARGIN - sizeDifference);
        return leftMargin;
    }

    function resetValues() {
    }

    function lerp(a, b, t) {
        return ((1 - t) * a + t * b);
    }

    function startInterstitialPage() {
        if (timer === null) {
            print("----------> starting <----------");
            updateOverlays(Window.isPhysicsEnabled());
            startAudio();
            target = 0;
            currentProgress = 0.1;
            connectionToDomainFailed = false;
            timer = Script.setTimeout(update, BASIC_TIMER_INTERVAL_MS);
        }
    }

    function startAudio() {
        sample = Audio.playSound(tune, {
            localOnly: true,
            position: MyAvatar.getHeadPosition(),
            volume: VOLUME
        });
    }

    function endAudio() {
        sample.stop();
        sample = null;
    }

    function domainChanged(domain) {
        print("domain changed: " + domain);
        if (domain !== currentDomain) {
            MyAvatar.restoreAnimation();
            var name = AddressManager.placename;
            domainName = name.charAt(0).toUpperCase() + name.slice(1);
            var domainNameLeftMargin = getLeftMargin(domainNameTextID, domainName);
            var textProperties = {
                text: domainName,
                leftMargin: domainNameLeftMargin
            };

            var url = Account.metaverseServerURL + '/api/v1/places/' + domain;
            request({
                uri: url
            }, function(error, data) {
                if (data.status === "success") {
                    print("-----------> settings domain description <----------");
                    var domainInfo = data.data;
                    var domainDescriptionText = domainInfo.place.description;
                    print("domainText: " + domainDescriptionText);
                    var leftMargin = getLeftMargin(domainDescription, domainDescriptionText);
                    var domainDescriptionProperties = {
                        text: domainDescriptionText,
                        leftMargin: leftMargin
                    };
                    Overlays.editOverlay(domainDescription, domainDescriptionProperties);
                }
            });

            var randomIndex = Math.floor(Math.random() * userTips.length);
            var tip = userTips[randomIndex];
            var tipLeftMargin = getLeftMargin(domainToolTip, tip);
            var toolTipProperties = {
                text: tip,
                leftMargin: tipLeftMargin
            };

            Overlays.editOverlay(domainNameTextID, textProperties);
            Overlays.editOverlay(domainToolTip, toolTipProperties);


            startInterstitialPage();
            currentDomain = domain;
        }
    }

    var THE_PLACE = "hifi://TheSpot";
    function clickedOnOverlay(overlayID, event) {
        print(overlayID + " other: " + loadingToTheSpotID);
        if (loadingToTheSpotID === overlayID) {
            location.handleLookupString(THE_PLACE);
        }
    }

    var currentProgress = 0.1;

    function updateOverlays(physicsEnabled) {
        var properties = {
            visible: !physicsEnabled
        };

        var mainSphereProperties = {
            visible: !physicsEnabled
        };

        var domainTextProperties = {
            text: domainText,
            visible: !physicsEnabled
        };

        // Menu.setIsOptionChecked("Show Overlays", physicsEnabled);

        if (!HMD.active) {
            MyAvatar.headOrientation = Quat.multiply(Quat.cancelOutRollAndPitch(MyAvatar.headOrientation), Quat.fromPitchYawRollDegrees(-3.0, 0, 0));
        }

        //renderViewTask.getConfig("LightingModel")["enableAmbientLight"] = physicsEnabled;
        //renderViewTask.getConfig("LightingModel")["enableDirectionalLight"] = physicsEnabled;
        //renderViewTask.getConfig("LightingModel")["enablePointLight"] = physicsEnabled;
        Overlays.editOverlay(loadingSphereID, mainSphereProperties);
        Overlays.editOverlay(loadingToTheSpotID, properties);
        Overlays.editOverlay(domainNameTextID, properties);
        Overlays.editOverlay(domainDescription, domainTextProperties);
        Overlays.editOverlay(domainToolTip, properties);
        Overlays.editOverlay(loadingBarPlacard, properties);
        Overlays.editOverlay(loadingBarProgress, properties);

        Camera.mode = "first person";
    }


    function scaleInterstitialPage(sensorToWorldScale) {
        var yOffset = getAnchorLocalYOffset();
        var localPosition = {
            x: 0.0,
            y: yOffset,
            z: 5.45
        };

        Overlays.editOverlay(anchorOverlay, { localPosition: localPosition });
    }

    var progress = 0;
    function updateProgress() {
        print("updateProgress");
        var thisInterval = Date.now();
        var deltaTime = (thisInterval - lastInterval);
        lastInterval = thisInterval;
        timeElapsed += deltaTime;

        progress += MAX_X_SIZE * (deltaTime / 1000);
        print(progress);
        var properties = {
            localPosition: { x: -(progress / 2) + 2, y: -0.99, z: 0.0 },
            dimensions: {
                x: progress,
                y: 2.8
            }
        };

        if (progress > MAX_X_SIZE) {
            progress = 0;
        }

        Overlays.editOverlay(loadingBarProgress, properties);

        if (!toggle) {
            Script.setTimeout(updateProgress, BASIC_TIMER_INTERVAL_MS);
        }
    }

    function update() {
        var physicsEnabled = Window.isPhysicsEnabled();
        var thisInterval = Date.now();
        var deltaTime = (thisInterval - lastInterval);
        lastInterval = thisInterval;
        timeElapsed += deltaTime;

        var nearbyEntitiesReadyCount = Window.getPhysicsNearbyEntitiesReadyCount();
        var stabilityCount = Window.getPhysicsNearbyEntitiesStabilityCount();
        var nearbyEntitiesCount = Window.getPhysicsNearbyEntitiesCount();

        var stabilityPercentage = (stabilityCount / STABILITY);
        if (stabilityPercentage > 1) {
            stabilityPercentage = 1;
        }

        var stabilityProgress = (MAX_X_SIZE * 0.75) * stabilityPercentage;

        var entitiesLoadedPercentage = 1;
        if (nearbyEntitiesCount > 0) {
            entitiesLoadedPercentage = nearbyEntitiesReadyCount / nearbyEntitiesCount;
        }
        var entitiesLoadedProgress = (MAX_X_SIZE * 0.25) * entitiesLoadedPercentage;
        var progress = stabilityProgress + entitiesLoadedProgress;
        if (progress >= target) {
            target = progress;
        }
        currentProgress = lerp(currentProgress, target, 0.2);
        var properties = {
            localPosition: { x: -(currentProgress / 2) + 2, y: 0.99, z: 5.45 },
            dimensions: {
                x: currentProgress,
                y: 2.8
            }
        };
        print("progress: " + currentProgress);
        Overlays.editOverlay(loadingBarProgress, properties);
        if (((physicsEnabled && (currentProgress >= (MAX_X_SIZE - EPSILON))) || connectionToDomainFailed)) {
            print("----------> ending <--------");
            updateOverlays((physicsEnabled || connectionToDomainFailed));
            endAudio();
            timer = null;
            return;
        }
        timer = Script.setTimeout(update, BASIC_TIMER_INTERVAL_MS);
    }

    Overlays.mouseReleaseOnOverlay.connect(clickedOnOverlay);
    location.hostChanged.connect(domainChanged);
    location.lookupResultsFinished.connect(function() {
        Script.setTimeout(function() {
            print("location connected: " + location.isConnected);
            connectionToDomainFailed = !location.isConnected;
        }, 300);
    });

    MyAvatar.sensorToWorldScaleChanged.connect(scaleInterstitialPage);

    var toggle = true;
    if (DEBUG) {
        tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
        button = tablet.addButton(BUTTON_PROPERTIES);

        button.clicked.connect(function() {
            toggle = !toggle;
            updateOverlays(toggle);

            if (!toggle) {
                //Script.setTimeout(updateProgress, BASIC_TIMER_INTERVAL_MS);
            }
        });
    }

    function cleanup() {
        Overlays.deleteOverlay(loadingSphereID);
        Overlays.deleteOverlay(loadingToTheSpotID);
        Overlays.deleteOverlay(domainNameTextID);
        Overlays.deleteOverlay(domainDescription);
        Overlays.deleteOverlay(domainToolTip);
        Overlays.deleteOverlay(loadingBarPlacard);
        Overlays.deleteOverlay(loadingBarProgress);
        Overlays.deleteOverlay(anchorOverlay);

        if (DEBUG) {
            tablet.removeButton(button);
        }
    }

    Script.scriptEnding.connect(cleanup);
}());
