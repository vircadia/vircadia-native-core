//  doll.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 9/21/15.
//  Additions by James B. Pollack @imgntn on 9/24/15
//  Copyright 2015 High Fidelity, Inc.
//
//  This entity script plays an animation and a sound while you hold it.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Known issues:  If you pass the doll between hands, animation can get into a weird state.  We want to prevent the animation from starting again when you switch hands, but when you switch mid-animation your hand at release is still the first hand and not the current hand.
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

(function() {
    Script.include("../../utilities.js");
    Script.include("../../libraries/utils.js");

    // this is the "constructor" for the entity as a JS object we don't do much here
    var Doll = function() {
        this.screamSounds = [SoundCache.getSound("https://hifi-public.s3.amazonaws.com/sounds/KenDoll_1%2303.wav")];
    };

    Doll.prototype = {
        startAnimationSetting: JSON.stringify({
            running: true,
            fps: 30,
            startFrame: 0,
            lastFrame: 128,
            startAutomatically: true
        }),
        stopAnimationSetting: JSON.stringify({running: false}),
        audioInjector: null,
        isGrabbed: false,
        setLeftHand: function() {
            this.hand = 'left';
        },

        setRightHand: function() {
            this.hand = 'right';
        },

        startNearGrab: function() {
            if (this.isGrabbed === false) {

                Entities.editEntity(this.entityID, {
                    animationURL: "https://hifi-public.s3.amazonaws.com/models/Bboys/zombie_scream.fbx",
                    animationSettings: this.startAnimationSetting
                });

                var position = Entities.getEntityProperties(this.entityID, "position").position;
                this.audioInjector = Audio.playSound(this.screamSounds[randInt(0, this.screamSounds.length)], {
                    position: position,
                    volume: 0.1
                });

                this.isGrabbed = true;
                this.initialHand = this.hand;
            }
        },

        continueNearGrab: function() {
            var props = Entities.getEntityProperties(this.entityID, "position");
            var audioOptions = {
                position: props.position
            };
            this.audioInjector.options = audioOptions;
        },

        releaseGrab: function() {
            if (this.isGrabbed === true && this.hand === this.initialHand) {

                this.audioInjector.stop();

                Entities.editEntity(this.entityID, {
                    animationSettings: this.stopAnimationSetting,
                    animationURL: "http://hifi-public.s3.amazonaws.com/models/Bboys/bboy2/bboy2.fbx",
                });
                this.isGrabbed = false;
            }
        },

        preload: function(entityID) {
            // preload() will be called when the entity has become visible (or known) to the interface
            // it gives us a chance to set our local JavaScript object up. In this case it means:
            //   * remembering our entityID, so we can access it in cases where we're called without an entityID
            this.entityID = entityID;
        },
    };
    // entity scripts always need to return a newly constructed object of our type
    return new Doll();
});