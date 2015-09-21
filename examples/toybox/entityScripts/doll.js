//
//  detectGrabExample.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 9/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to an entity, will detect when the entity is being grabbed by the hydraGrab script
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;
    HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    Doll = function() {
        _this = this;
        var screamSoundDirectory = HIFI_PUBLIC_BUCKET + "eric/sounds/"
        this.screamSounds = [SoundCache.getSound(screamSoundDirectory + "dollScream2.wav?=v2"), SoundCache.getSound(screamSoundDirectory + "dollScream1.wav?=v2")];
        this.startAnimationSetting = JSON.stringify({
            running: true
        });

        this.stopAnimationSetting = JSON.stringify({
            frameIndex: 0,
            running: false
        });
    };

    Doll.prototype = {


        startNearGrab: function() {
            Entities.editEntity(this.entityID, {
                animationURL: "https://hifi-public.s3.amazonaws.com/models/Bboys/zombie_scream.fbx",
                animationSettings: this.startAnimationSetting
            });

            var position = Entities.getEntityProperties(this.entityID, "position").position;
            Audio.playSound(this.screamSounds[randInt(0, this.screamSounds.length)], {
                position: position,
                volume: 0.01
            });

        },

        releaseGrab: function() {
            Entities.editEntity(this.entityID, {
                animationURL: "http://hifi-public.s3.amazonaws.com/models/Bboys/bboy2/bboy2.fbx",
                // animationSettings: this.stopAnimationSetting
            });
        },
      

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * connecting to the update signal so we can check our grabbed state
        preload: function(entityID) {
            this.entityID = entityID;
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Doll();
})

function randFloat(low, high) {
    return low + Math.random() * (high - low);
}

function randInt(low, high) {
    return Math.floor(randFloat(low, high));
}