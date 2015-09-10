//  bubble.js
//  part of bubblewand
//
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  example of a nested entity. it doesn't do much now besides delete itself if it collides with something (bubbles are fragile!  it would be cool if it sometimes merged with other bubbbles it hit)
//  todo: play bubble sounds & particle bursts from the bubble itself instead of the wand. 
//  blocker: needs some sound fixes and a way to find its own position before unload for spatialization
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() {
    Script.include("../../utilities.js");
    Script.include("../../libraries/utils.js");

    var POP_SOUNDS = [
        SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop0.wav"),
        SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop1.wav"),
        SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop2.wav"),
        SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop3.wav")
    ]

    BUBBLE_PARTICLE_COLOR = {
        red: 0,
        green: 40,
        blue: 255,
    }

    var _t = this;

    var properties;
    var checkPositionInterval;
    this.preload = function(entityID) {
        //  print('bubble preload')
        _t.entityID = entityID;
        //properties = Entities.getEntityProperties(entityID);
        // _t.loadShader(entityID);
        Script.update.connect(_t.internalUpdate);
    };

    this.internalUpdate = function() {
        var tmpProperties = Entities.getEntityProperties(_t.entityID);
        if (tmpProperties.position.x !== 0 && tmpProperties.position.y !== 0 && tmpProperties.position.z !== 0) {
            properties = tmpProperties;
        }
    }

    this.loadShader = function(entityID) {
        setEntityUserData(entityID, {
            "ProceduralEntity": {
                "shaderUrl": "http://hifi-public.s3.amazonaws.com/james/bubblewand/shaders/quora.fs",
            }
        })
    };


    this.leaveEntity = function(entityID) {
        //   print('LEAVE ENTITY:' + entityID)
    };

    this.collisionWithEntity = function(myID, otherID, collision) {
        //Entities.deleteEntity(myID);
        // Entities.deleteEntity(otherID);
    };

    this.unload = function(entityID) {
        Script.update.disconnect(this.internalUpdate);
        var position = properties.position;
        _t.endOfBubble(position);
        print('UNLOAD PROPS' + JSON.stringify(position));

    };

    this.endOfBubble = function(position) {
        this.burstBubbleSound(position);
        this.createBurstParticles(position);
    }

    this.burstBubbleSound = function(position) {
        var audioOptions = {
            volume: 0.5,
            position: position
        }
        Audio.playSound(POP_SOUNDS[randInt(0, 4)], audioOptions);

    }

    this.createBurstParticles = function(position) {
        var _t = this;
        //get the current position of the bubble
        var position = properties.position;
        //var orientation = properties.orientation;

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
            lifetime: 1.0,
            dimensions: {
                x: 1,
                y: 1,
                z: 1
            },
            emitVelocity: {
                x: 0,
                y: -1,
                z: 0
            },
            velocitySpread: {
                x: 1,
                y: 0,
                z: 1
            },
            emitAcceleration: {
                x: 0,
                y: -1,
                z: 0
            },
            textures: "https://raw.githubusercontent.com/ericrius1/SantasLair/santa/assets/smokeparticle.png",
            color: BUBBLE_PARTICLE_COLOR,
            lifespan: 1.0,
            visible: true,
            locked: false
        });

    }


})