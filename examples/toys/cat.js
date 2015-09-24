//
//  doll.js
//  examples/toybox/entityScripts
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This entity script breathes movement and sound- one might even say life- into a doll.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    Cat = function() {
        _this = this;
        this.meowSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Animals/cat_meow.wav");
        this.distanceThreshold = 1;
        this.canMeow = true;
        this.meowBreakTime = 3000;
    };

    Cat.prototype = {

        startTouch: function() {
            this.meow();
        },

        meow: function() {

            Audio.playSound(this.meowSound, {
                position: this.position,
                volume: .1
            });
        },
        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * connecting to the update signal so we can check our grabbed state
        preload: function(entityID) {
            this.entityID = entityID;
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
            Script.update.connect(this.update);
        },

        unload: function() {
            Script.update.disconnect(this.update);
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Cat();
});