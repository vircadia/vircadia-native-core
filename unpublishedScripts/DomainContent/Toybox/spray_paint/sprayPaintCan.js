//
//  sprayPaintCan.js
//  examples/entityScripts
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function () {
 Script.include("../libraries/utils.js");

    this.spraySound = SoundCache.getSound("http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/spray_paint/spray_paint.wav");

    var TIP_OFFSET_Z = 0.02;
    var TIP_OFFSET_Y = 0.08;

    var ZERO_VEC = {
        x: 0,
        y: 0,
        z: 0
    };

    // if the trigger value goes below this while held, the can will stop spraying.  if it goes above, it will spray
    var DISABLE_SPRAY_THRESHOLD = 0.5;

    var MAX_POINTS_PER_LINE = 40;
    var MIN_POINT_DISTANCE = 0.01;
    var STROKE_WIDTH = 0.02;

    var TRIGGER_CONTROLS = [
        Controller.Standard.LT,
        Controller.Standard.RT,
    ];
    
    this.startNearGrab = function (entityID, args) {
        this.hand = args[0] == "left" ? 0 : 1;
    }

    this.toggleWithTriggerPressure = function () {
        this.triggerValue = Controller.getValue(TRIGGER_CONTROLS[this.hand]);
        if (this.triggerValue < DISABLE_SPRAY_THRESHOLD && this.spraying === true) {
            this.spraying = false;
            this.disableStream();
        } else if (this.triggerValue >= DISABLE_SPRAY_THRESHOLD && this.spraying === false) {
            this.spraying = true;
            this.enableStream();
        }
    }

    this.enableStream = function () {
        var position = Entities.getEntityProperties(this.entityId, "position").position;
        var PI = 3.141593;
        var DEG_TO_RAD = PI / 180.0;

        this.paintStream = Entities.addEntity({
            type: "ParticleEffect",
            name: "streamEffect",
            isEmitting: true,
            position: position,
            textures: "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/spray_paint/smokeparticle.png",
            emitSpeed: 3,
            speedSpread: 0.02,
            emitAcceleration: ZERO_VEC,
            emitRate: 100,
            particleRadius: 0.01,
            radiusSpread: 0.005,
            polarFinish: 0.05,
            colorStart: {
                red: 50,
                green: 10,
                blue: 150
            },
            color: {
                red: 170,
                green: 20,
                blue: 150
            },
            lifetime: 50, //probably wont be holding longer than this straight,
            emitterShouldTrail: true
        });

        setEntityCustomData(this.resetKey, this.paintStream, {
            resetMe: true
        });

        this.sprayInjector = Audio.playSound(this.spraySound, {
            position: position,
            volume: this.sprayVolume,
            loop: true
        });

    }

    this.releaseGrab = function () {
        this.disableStream();
    }

    this.disableStream = function () {
        Entities.deleteEntity(this.paintStream);
        this.paintStream = null;
        this.spraying = false;
        this.sprayInjector.stop();
    }


    this.continueNearGrab = function () {

        this.toggleWithTriggerPressure();

        if (this.spraying === false) {
            return;
        }

        var props = Entities.getEntityProperties(this.entityId, ["position, rotation"]);
        var forwardVec = Quat.getFront(Quat.multiply(props.rotation, Quat.fromPitchYawRollDegrees(0, 90, 0)));
        forwardVec = Vec3.normalize(forwardVec);
        var forwardQuat = orientationOf(forwardVec);
        var upVec = Quat.getUp(props.rotation);
        var position = Vec3.sum(props.position, Vec3.multiply(forwardVec, TIP_OFFSET_Z));
        position = Vec3.sum(position, Vec3.multiply(upVec, TIP_OFFSET_Y))
        Entities.editEntity(this.paintStream, {
            position: position,
            emitOrientation: forwardQuat,
        });

        this.sprayInjector.setOptions({
            position: position,
            volume: this.sprayVolume
        });
    }

    this.preload = function (entityId) {
        this.sprayVolume = 0.1;
        this.spraying = false;
        this.entityId = entityId;
        this.resetKey = "resetMe";
    }


    this.unload = function () {
        if (this.paintStream) {
            Entities.deleteEntity(this.paintStream);
        }
    }
});
