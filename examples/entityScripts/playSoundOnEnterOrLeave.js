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
    var bird = new Sound("http://s3.amazonaws.com/hifi-public/sounds/Animals/bushtit_1.raw");

    function playSound(entityID) { 
        var options = new AudioInjectionOptions();
        var position = MyAvatar.position; 
        options.position = position;
        options.volume = 0.5;
        Audio.playSound(bird, options);
    }; 

    this.enterEntity = function(entityID) { 
        playSound();
    }; 

    this.leaveEntity = function(entityID) { 
        playSound();
    }; 
})
