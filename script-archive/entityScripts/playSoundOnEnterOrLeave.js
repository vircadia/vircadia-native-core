//
//  playSoundOnEnterOrLeave.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to an entity, that entity will play a sound in world when
//  your avatar enters or leaves the bounds of the entity.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function(){ 
    var bird;

    function playSound(entityID) {
        Audio.playSound(bird, {
          position: MyAvatar.position,
          volume: 0.5
        });
    } 

    this.preload = function(entityID) { 
        print("preload("+entityID+")");
        bird = SoundCache.getSound("http://s3.amazonaws.com/hifi-public/sounds/Animals/bushtit_1.raw");
    }; 

    this.enterEntity = function(entityID) { 
        print("enterEntity("+entityID+")");
        playSound();
    }; 

    this.leaveEntity = function(entityID) {
        print("leaveEntity("+entityID+")");
        playSound();
    }; 
})
