//
//  particleExplorer.js
//
//  Created by James B. Pollack @imgntn on 9/26/2015
//  includes setup from @ctrlaltdavid's particlesTest.js
//  Copyright 2015 High Fidelity, Inc.
//
//  Interface side of the App.
//  Quickly edit the aesthetics of a particle system.  
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  next version: 2 way bindings, integrate with edit.js
//
/*global print, WebWindow, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

var box,
    sphere,
    sphereDimensions = {
        x: 0.4,
        y: 0.8,
        z: 0.2
    },
    pointDimensions = {
        x: 0.0,
        y: 0.0,
        z: 0.0
    },
    sphereOrientation = Quat.fromPitchYawRollDegrees(-60.0, 30.0, 0.0),
    verticalOrientation = Quat.fromPitchYawRollDegrees(-90.0, 0.0, 0.0),
    particles,
    particleExample = -1,
    PARTICLE_RADIUS = 0.04,
    SLOW_EMIT_RATE = 2.0,
    HALF_EMIT_RATE = 50.0,
    FAST_EMIT_RATE = 100.0,
    SLOW_EMIT_SPEED = 0.025,
    FAST_EMIT_SPEED = 1.0,
    GRAVITY_EMIT_ACCELERATON = {
        x: 0.0,
        y: -0.3,
        z: 0.0
    },
    ZERO_EMIT_ACCELERATON = {
        x: 0.0,
        y: 0.0,
        z: 0.0
    },
    PI = 3.141593,
    DEG_TO_RAD = PI / 180.0,
    NUM_PARTICLE_EXAMPLES = 18;

var particleProperties;

function setUp() {
    var boxPoint,
        spawnPoint,
        animation = {
            fps: 30,
            frameIndex: 0,
            running: true,
            firstFrame: 0,
            lastFrame: 30,
            loop: true
        };

    boxPoint = Vec3.sum(MyAvatar.position, Vec3.multiply(4.0, Quat.getFront(Camera.getOrientation())));
    boxPoint = Vec3.sum(boxPoint, {
        x: 0.0,
        y: -0.5,
        z: 0.0
    });
    spawnPoint = Vec3.sum(boxPoint, {
        x: 0.0,
        y: 1.0,
        z: 0.0
    });

    box = Entities.addEntity({
        type: "Box",
        name: "ParticlesTest Box",
        position: boxPoint,
        rotation: verticalOrientation,
        dimensions: {
            x: 0.3,
            y: 0.3,
            z: 0.3
        },
        color: {
            red: 128,
            green: 128,
            blue: 128
        },
        lifetime: 3600, // 1 hour; just in case
        visible: true
    });

    // Same size and orientation as emitter when ellipsoid.
    sphere = Entities.addEntity({
        type: "Sphere",
        name: "ParticlesTest Sphere",
        position: boxPoint,
        rotation: sphereOrientation,
        dimensions: sphereDimensions,
        color: {
            red: 128,
            green: 128,
            blue: 128
        },
        lifetime: 3600, // 1 hour; just in case
        visible: false
    });

    // 1.0m above the box or ellipsoid.
    particles = Entities.addEntity({
        type: "ParticleEffect",
        name: "ParticlesTest Emitter",
        position: spawnPoint,
        emitOrientation: verticalOrientation,
        particleRadius: PARTICLE_RADIUS,
        radiusSpread: 0.0,
        emitRate: SLOW_EMIT_RATE,
        emitSpeed: FAST_EMIT_SPEED,
        speedSpread: 0.0,
        emitAcceleration: GRAVITY_EMIT_ACCELERATON,
        accelerationSpread: {
            x: 0.0,
            y: 0.0,
            z: 0.0
        },
        textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        lifespan: 5.0,
        visible: false,
        locked: false,
        animationSettings: animation,
        animationIsPlaying: true,
        lifetime: 3600 // 1 hour; just in case
    });

}

SettingsWindow = function() {
    var _this = this;

    this.webWindow = null;

    this.init = function() {
        Script.update.connect(waitForObjectAuthorization);
        _this.webWindow = new WebWindow('Particle Explorer', Script.resolvePath('index.html'), 400, 600, false);
        _this.webWindow.eventBridge.webEventReceived.connect(_this.onWebEventReceived);
    };

    this.sendData = function(data) {
        _this.webWindow.eventBridge.emitScriptEvent(JSON.stringify(data));
    };

    this.onWebEventReceived = function(data) {
        var _data = JSON.parse(data);
        if (_data.messageType === 'page_loaded') {
            _this.webWindow.setVisible(true);
            _this.pageLoaded = true;
            sendInitialSettings(particleProperties);
        }
        if (_data.messageType === 'settings_update') {
            editEntity(_data.updatedSettings);
            return;
        }

    };
};

function waitForObjectAuthorization() {
    var properties = Entities.getEntityProperties(particles, "isKnownID");
    var isKnownID = properties.isKnownID;
    if (isKnownID === false || SettingsWindow.pageLoaded === false) {
        return;
    }
    var currentProperties = Entities.getEntityProperties(particles);
    particleProperties = currentProperties;
    Script.update.connect(sendObjectUpdates);
    Script.update.disconnect(waitForObjectAuthorization);
}

function sendObjectUpdates() {
    var currentProperties = Entities.getEntityProperties(particles);
    sendUpdatedObject(currentProperties);
}

function sendInitialSettings(properties) {
    var settings = {
        messageType: 'initial_settings',
        initialSettings: properties
    };

    settingsWindow.sendData(settings);
}

function sendUpdatedObject(properties) {
    var settings = {
        messageType: 'object_update',
        objectSettings: properties
    };
    settingsWindow.sendData(settings);
}

function editEntity(properties) {
    Entities.editEntity(particles, properties);
}

function tearDown() {
    Entities.deleteEntity(particles);
    Entities.deleteEntity(box);
    Entities.deleteEntity(sphere);
    Script.update.disconnect(sendObjectUpdates);
}

var settingsWindow = new SettingsWindow();
settingsWindow.init();
setUp();
Script.scriptEnding.connect(tearDown);