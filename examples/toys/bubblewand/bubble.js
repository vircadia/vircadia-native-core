//  bubble.js
//  part of bubblewand
//
//  Script Type: Entity
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  example of a nested entity. it doesn't do much now besides delete itself if it collides with something (bubbles are fragile!  it would be cool if it sometimes merged with other bubbbles it hit)
//  todo: play bubble sounds & particle bursts from the bubble itself instead of the wand. 
//  blocker: needs some sound fixes and a way to find its own position before unload for spatialization
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() { << << << < HEAD
    Script.include("../../utilities.js");
    Script.include("../../libraries/utils.js");

    var BUBBLE_PARTICLE_TEXTURE = "https://raw.githubusercontent.com/ericrius1/SantasLair/santa/assets/smokeparticle.png"


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
        Script.update.connect(_t.internalUpdate);
    };

    this.internalUpdate = function() {
        // we want the position at unload but for some reason it keeps getting set to 0,0,0 -- so i just exclude that location.  sorry origin bubbles.
        var tmpProperties = Entities.getEntityProperties(_this.entityID);
        if (tmpProperties.position.x !== 0 && tmpProperties.position.y !== 0 && tmpProperties.position.z !== 0) {
            properties = tmpProperties;
        }

    };

    this.unload = function(entityID) {
        Script.update.disconnect(this.internalUpdate);
        var position = properties.position;
        _t.endOfBubble(position);
        //  print('UNLOAD PROPS' + JSON.stringify(position));

    };

    this.endOfBubble = function(position) {

        this.createBurstParticles(position);
        this.burstBubbleSound(position);
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
            lifetime: 0.2,
            dimensions: {
                x: 1,
                y: 1,
                z: 1
            },
            emitVelocity: {
                x: 0,
                y: 0,
                z: 0
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
            alphaStart: 1.0,
            alpha: 1,
            alphaFinish: 0.0,
            textures: BUBBLE_PARTICLE_TEXTURE,
            color: BUBBLE_PARTICLE_COLOR,
            lifespan: 0.2,
            visible: true,
            locked: false
        });

    };


})