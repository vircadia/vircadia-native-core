//  bubble.js
//  part of bubblewand
//
//  Script Type: Entity
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  example of a nested entity. plays a particle burst at the location where its deleted.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {
    Script.include("../../utilities.js");
    Script.include("../../libraries/utils.js");

    var BUBBLE_PARTICLE_TEXTURE = "http://hifi-public.s3.amazonaws.com/james/bubblewand/textures/bubble_particle.png"

    var BUBBLE_USER_DATA_KEY = "BubbleKey";

    BUBBLE_PARTICLE_COLOR = {
        red: 0,
        green: 40,
        blue: 255,
    };

    var _this = this;

    var properties;

    this.preload = function(entityID) {
        //  print('bubble preload')
        _this.entityID = entityID;
        Script.update.connect(_this.update);
    };

    this.update = function() {
        // we want the position at unload but for some reason it keeps getting set to 0,0,0 -- so i just exclude that location.  sorry origin bubbles.
        var tmpProperties = Entities.getEntityProperties(_this.entityID);
        if (tmpProperties.position.x !== 0 && tmpProperties.position.y !== 0 && tmpProperties.position.z !== 0) {
            properties = tmpProperties;
        }

    };

    this.unload = function(entityID) {
        Script.update.disconnect(this.update);

        var defaultGrabData = {
            avatarId: null
        };

        var bubbleCreator = getEntityCustomData(BUBBLE_USER_DATA_KEY, entityID, defaultGrabData);

        if (bubbleCreator === MyAvatar.sessionUUID) {
            this.createBurstParticles();
        }

    };


    this.createBurstParticles = function() {
        //get the current position and dimensions of the bubble
        var position = properties.position;
        var dimensions = properties.dimensions;


        var animationSettings = JSON.stringify({
            fps: 30,
            frameIndex: 0,
            running: true,
            firstFrame: 0,
            lastFrame: 30,
            loop: false
        });

        var particleBurst = Entities.addEntity({
            type: "ParticleEffect",
            animationSettings: animationSettings,
            animationIsPlaying: true,
            position: position,
            lifetime: 0.1,
            dimensions: {
                x: 1,
                y: 1,
                z: 1
            },
            emitVelocity: {
                x: 0.35,
                y: 0.35,
                z: 0.35
            },
            velocitySpread: {
                x: 0.45,
                y: 0.45,
                z: 0.45
            },
            emitAcceleration: {
                x: 0,
                y: -0.1,
                z: 0
            },
            particleRadius: 0.1,
            alphaStart: 0.5,
            alpha: 0.5,
            alphaFinish: 0,
            textures: BUBBLE_PARTICLE_TEXTURE,
            //  color: BUBBLE_PARTICLE_COLOR,
            lifespan: 0.1,
            visible: true,
            locked: false
        });

    };


});