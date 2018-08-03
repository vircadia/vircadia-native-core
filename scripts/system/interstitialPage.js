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
   Camera, HMD*/

(function() {
    var MAX_X_SIZE = 3;
    var isVisible = true;
    var defaultOffset = 1.5;
    var hifi = "HighFidelity";
    var VOLUME = 0.4;
    var tune = SoundCache.getSound("http://hifi-content.s3.amazonaws.com/alexia/LoadingScreens/crystals_and_voices.wav");
    var sample = null;
    var MAX_LEFT_MARGIN = 1.9;
    var INNER_CIRCLE_WIDTH = 4.7;
    var DESTINATION_CARD_Y_OFFSET = 2;
    var DEFAULT_TONE_MAPPING_EXPOSURE = 0.0;
    var MIN_TONE_MAPPING_EXPOSURE = -5.0;
    var SYSTEM_TOOL_BAR = "com.highfidelity.interface.toolbar.system";
    var MAX_ELAPSED_TIME = 5 * 1000; // time in ms
    function isInFirstPerson() {
        return (Camera.mode === "first person");
    }

    var toolbar = Toolbars.getToolbar(SYSTEM_TOOL_BAR);
    var renderViewTask = Render.getConfig("RenderMainView");

    var domainHostnameMap = {
        eschatology: "Seth Alves",
        blue: "Sam Cake",
        thepines: "Roxie",
        "dev-mobile": "HighFidelity",
        "dev-mobile-master": "HighFidelity",
        portalarium: "Bijou",
        porange: "Caitlyn",
        rust: hifi,
        start: hifi,
        miimusic: "Madysyn",
        codex: "FluffyJenkins",
        zaru: hifi,
        help: hifi,
        therealoasis: "Caitlyn",
        vrmacy: "budgiebeats",
        niccage: "OneLisa",
        impromedia: "GeorgeDeac",
        nest: "budgiebeats",
        gabworld: "LeeGab",
        vrtv: "GeoorgeDeac",
        burrow: "budgiebeats",
        leftcoast: "Lurks",
        lazybones: "LazybonesJurassic",
        skyriver: "Chamberlain",
        chapel: "www.livin.today",
        "hi-studio": hifi,
        luskan: "jyoum",
        arcadiabay: "Aitolda",
        chime: hifi,
        standupnow: "diva",
        avreng: "GeorgeDeac",
        atlas: "rocklin_guy",
        steamedhams: "Alan_",
        banff: hifi,
        operahouse: hifi,
        bankofhighfidelity: hifi,
        tutorial: "WadeWatts",
        nightsky: hifi,
        garageband: hifi,
        painting: hifi,
        windwaker: "bijou",
        fumbleland: "Lpasca",
        monolith: "Nik",
        bijou: "bijou",
        morty: "bijou",
        "hifiqa-rc-bots": hifi,
        fightnight: hifi,
        spirited: "Alan_",
        "desert-oasis": "ryan",
        springfield: "Alan_",
        hall: "ryan",
        "national-park": "ryan",
        vector: "Nik",
        bodymart: hifi,
        "medievil-village": "ryan",
        "villains-lair": "ryan",
        "island-breeze": "ryan",
        "classy-apartment": "ryan",
        voxel: "FlameSoulis",
        virtuoso: "noahglaseruc",
        avatarisland: hifi,
        ioab: "rocklin_guy",
        tamait: "rocklin_guy",
        konshulabs: "Konshu",
        epic: "philip",
        poopsburg: "Caitlyn",
        east: hifi,
        glitched: hifi,
        calartsim: hifi,
        calarts: hifi,
        livin: "rocklin_guy",
        fightclub: "philip",
        thefactory: "whyroc",
        wothal: "Alezia.Kurdis",
        udacity: hifi,
        json: "WadeWatts",
        anonymous: "darlingnotin",
        maker: hifi,
        elisa: "elisahifi",
        volxeltopia: hifi,
        cupcake: hifi,
        minigolf: hifi,
        workshop: hifi,
        vankh: "Alezia.Kurdis",
        "the-crash-site": "WolfGang",
        jjv360: "jjv3600",
        distributed2: hifi,
        anny: hifi,
        university: hifi,
        ludus: hifi,
        stepford: "darlingnotin",
        thespot: hifi
    };

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

    var loadingSphereID = Overlays.addOverlay("model", {
        name: "Loading-Sphere",
        position: Vec3.sum(Vec3.sum(MyAvatar.position, {x: 0.0, y: -1.0, z: 0.0}), Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.95, z: 0})),
        orientation: Quat.multiply(Quat.fromVec3Degrees({x: 0, y: 180, z: 0}), MyAvatar.orientation),
        url: "http://hifi-content.s3.amazonaws.com/alexia/LoadingScreens/black-sphere.fbx",
        dimensions: { x: 20, y: 20, z: 20 },
        alpha: 1,
        visible: isVisible,
        ignoreRayIntersection: true,
        drawInFront: true,
        grabbable: false
    });


    var domainName = "";
    var domainNameTextID = Overlays.addOverlay("text3d", {
        name: "Loading-Destination-Card-Text",
        localPosition: { x: 0.0, y: DESTINATION_CARD_Y_OFFSET + 0.8, z: 5.45 },
        text: domainName,
        textAlpha: 1,
        backgroundAlpha: 0,
        lineHeight: 0.42,
        visible: isVisible,
        ignoreRayIntersection: true,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
        parentID: loadingSphereID
    });

    var hostName = "";

    var domainHostname = Overlays.addOverlay("text3d", {
        name: "Loading-Hostname",
        localPosition: { x: 0.0, y: DESTINATION_CARD_Y_OFFSET + 0.32, z: 5.45 },
        text: hostName,
        textAlpha: 1,
        backgroundAlpha: 0,
        lineHeight: 0.13,
        visible: isVisible,
        ignoreRayIntersection: true,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
        parentID: loadingSphereID
    });

    var toolTip = "";

    var domainToolTip = Overlays.addOverlay("text3d", {
        name: "Loading-Tooltip",
        localPosition: { x: 0.0 , y: DESTINATION_CARD_Y_OFFSET - 1.6, z: 5.45 },
        text: toolTip,
        textAlpha: 1,
        backgroundAlpha: 0,
        lineHeight: 0.13,
        visible: isVisible,
        ignoreRayIntersection: true,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 }),
        parentID: loadingSphereID
    });

    var loadingToTheSpotID = Overlays.addOverlay("image3d", {
        name: "Loading-Destination-Card-Text",
        localPosition: { x: 0.0 , y: DESTINATION_CARD_Y_OFFSET - 1.8, z: 5.45 },
        url: "http://hifi-content.s3.amazonaws.com/alexia/LoadingScreens/goTo_button.png",
        alpha: 1,
        dimensions: { x: 1.2, y: 0.6},
        visible: isVisible,
        emissive: true,
        ignoreRayIntersection: false,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0.0, y: 180.0, z: 0.0 }),
        parentID: loadingSphereID
    });

    var loadingBarPlacard = Overlays.addOverlay("image3d", {
        name: "Loading-Bar-Placard",
        localPosition: { x: 0.0 , y: DESTINATION_CARD_Y_OFFSET - 0.99, z: 5.45 },
        url: "http://hifi-content.s3.amazonaws.com/alexia/LoadingScreens/loadingBar_placard.png",
        alpha: 1,
        dimensions: { x: 4, y: 2.8},
        visible: isVisible,
        emissive: true,
        ignoreRayIntersection: false,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0.0, y: 180.0, z: 0.0 }),
        parentID: loadingSphereID
    });

    var loadingBarProgress = Overlays.addOverlay("image3d", {
        name: "Loading-Bar-Progress",
        localPosition: { x: 0.0 , y: DESTINATION_CARD_Y_OFFSET - 0.99, z: 5.45 },
        url: "http://hifi-content.s3.amazonaws.com/alexia/LoadingScreens/loadingBar_progress.png",
        alpha: 1,
        dimensions: { x: 4, y: 2.8},
        visible: isVisible,
        emissive: true,
        ignoreRayIntersection: false,
        drawInFront: true,
        grabbable: false,
        localOrientation: Quat.fromVec3Degrees({ x: 0.0, y: 180.0, z: 0.0 }),
        parentID: loadingSphereID
    });

    var TARGET_UPDATE_HZ = 60; // 50hz good enough, but we're using update
    var BASIC_TIMER_INTERVAL_MS = 1000 / TARGET_UPDATE_HZ;
    var timerset = false;
    var lastInterval = Date.now();
    var timeElapsed = 0;


    function getLeftMargin(overlayID, text) {
        var textSize = Overlays.textSize(overlayID, text);
        var sizeDifference = ((INNER_CIRCLE_WIDTH - textSize.width) / 2);
        var leftMargin = -(MAX_LEFT_MARGIN - sizeDifference);
        return leftMargin;
    }


    function domainChanged(domain) {
        var name = AddressManager.placename;
        domainName = name.charAt(0).toUpperCase() + name.slice(1);
        var domainNameLeftMargin = getLeftMargin(domainNameTextID, domainName);
        var textProperties = {
            text: domainName,
            leftMargin: domainNameLeftMargin
        };

        var BY = "by ";
        var host = domainHostnameMap[location.placename];
        var text = BY + host;
        var hostLeftMargin = getLeftMargin(domainHostname, text);
        var hostnameProperties = {
            text: BY + host,
            leftMargin: hostLeftMargin
        };

        var randomIndex = Math.floor(Math.random() * userTips.length);
        var tip = userTips[randomIndex];
        var tipLeftMargin = getLeftMargin(domainToolTip, tip);
        var toolTipProperties = {
            text: tip,
            leftMargin: tipLeftMargin
        };

        var myAvatarDirection = Vec3.UNIT_NEG_Z;
        var cardDirectionPrime = {x: 0 , y: 0, z: 5.5};
        var rotationDelta = Quat.rotationBetween(cardDirectionPrime, myAvatarDirection);
        var overlayRotation = Quat.multiply(MyAvatar.orientation, rotationDelta);
        var mainSphereProperties = {
            orientation: overlayRotation
        };

        Overlays.editOverlay(loadingSphereID, mainSphereProperties);
        Overlays.editOverlay(domainNameTextID, textProperties);
        Overlays.editOverlay(domainHostname, hostnameProperties);
        Overlays.editOverlay(domainToolTip, toolTipProperties);
    }

    var THE_PLACE = "hifi://TheSpot";
    function clickedOnOverlay(overlayID, event) {
        print(overlayID + " other: " + loadingToTheSpotID);
        print(event.button === "Primary");
        if (loadingToTheSpotID === overlayID) {
            if (timerset) {
                timeElapsed = 0;
            }
            AddressManager.handleLookupString(THE_PLACE);
        }
    }
    var previousCameraMode = Camera.mode;
    var previousPhysicsStatus = 99999;

    function updateOverlays(physicsEnabled) {
        var properties = {
            visible: !physicsEnabled
        };

        var myAvatarDirection = Vec3.UNIT_NEG_Z;
        var cardDirectionPrime = {x: 0 , y: 0, z: 5.5};
        var rotationDelta = Quat.rotationBetween(cardDirectionPrime, myAvatarDirection);
        var overlayRotation = Quat.multiply(MyAvatar.orientation, rotationDelta);
        var mainSphereProperties = {
            visible: !physicsEnabled,
            orientation: overlayRotation
        };

        if (!HMD.active) {
            toolbar.writeProperty("visible", physicsEnabled);
            MyAvatar.headOrientation = Quat.multiply(Quat.cancelOutRollAndPitch(MyAvatar.headOrientation), Quat.fromPitchYawRollDegrees(2.5, 0, 0));
        }

        renderViewTask.getConfig("LightingModel")["enableAmbientLight"] = physicsEnabled;
        renderViewTask.getConfig("LightingModel")["enableDirectionalLight"] = physicsEnabled;
        renderViewTask.getConfig("LightingModel")["enablePointLight"] = physicsEnabled;
        Overlays.editOverlay(loadingSphereID, mainSphereProperties);
        Overlays.editOverlay(loadingToTheSpotID, properties);
        Overlays.editOverlay(domainNameTextID, properties);
        Overlays.editOverlay(domainHostname, properties);
        Overlays.editOverlay(domainToolTip, properties);
        Overlays.editOverlay(loadingBarPlacard, properties);
        Overlays.editOverlay(loadingBarProgress, properties);
    }

    function update() {
        var physicsEnabled = Window.isPhysicsEnabled();
        var thisInterval = Date.now();
        var deltaTime = (thisInterval - lastInterval);
        lastInterval = thisInterval;
        if (physicsEnabled !== previousPhysicsStatus) {
            if (!physicsEnabled && !timerset) {
                updateOverlays(physicsEnabled);
                sample = Audio.playSound(tune, {
                    localOnly: true,
                    position: MyAvatar.headPosition,
                    volume: VOLUME
                });
                timeElapsed = 0;
                timerset = true;
            }
            previousPhysicsStatus = physicsEnabled;
        }

        if (timerset) {
            timeElapsed += deltaTime;
            if (timeElapsed >= MAX_ELAPSED_TIME) {
                updateOverlays(true);
                sample.stop();
                sample = null;
                timerset = false;
            }

        }

        Overlays.editOverlay(loadingSphereID, {
            position: Vec3.sum(Vec3.sum(MyAvatar.position, {x: 0.0, y: -1.7, z: 0.0}), Vec3.multiplyQbyV(MyAvatar.orientation, {x: 0, y: 0.95, z: 0}))
        });
        Script.setTimeout(update, BASIC_TIMER_INTERVAL_MS);
    }

    Script.setTimeout(update, BASIC_TIMER_INTERVAL_MS);
    Overlays.mouseReleaseOnOverlay.connect(clickedOnOverlay);
    Window.domainChanged.connect(domainChanged);

    function cleanup() {
        Overlays.deleteOverlay(loadingSphereID);
        Overlays.deleteOverlay(loadingToTheSpotID);
        Overlays.deleteOverlay(domainNameTextID);
        Overlays.deleteOverlay(domainHostname);
        Overlays.deleteOverlay(domainToolTip);
        Overlays.deleteOverlay(loadingBarPlacard);
        Overlays.deleteOverlay(loadingBarProgress);
        try {
        }  catch (e) {
        }
    }

    Script.scriptEnding.connect(cleanup);
}());
