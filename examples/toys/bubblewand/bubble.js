//  bubble.js
//  part of bubblewand
//
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  example of a nested entity. it doesn't do much now besides delete itself if it collides with something (bubbles are fragile!  it would be cool if it sometimes merged with other bubbbles it hit)
//  todo: play bubble sounds from the bubble itself instead of the wand. 
//  blocker: needs some sound fixes and a way to find its own position before unload for spatialization
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


(function() {
    // Script.include("https://raw.githubusercontent.com/highfidelity/hifi/master/examples/utilities.js");
    // Script.include("https://raw.githubusercontent.com/highfidelity/hifi/master/examples/libraries/utils.js");

    //var popSound;
    this.preload = function(entityID) {
        //  print('bubble preload')
        this.entityID = entityID;
      //  popSound = SoundCache.getSound("http://hifi-public.s3.amazonaws.com/james/bubblewand/sounds/pop.wav");

    }

    this.collisionWithEntity = function(myID, otherID, collision) {
        //if(Entites.getEntityProperties(otherID).userData.objectType==='') { merge bubbles?}
      // Entities.deleteEntity(myID);
      //  this.burstBubbleSound(collision.contactPoint)

    };

    this.unload = function(entityID) {
        // this.properties = Entities.getEntityProperties(entityID);
        //var location = this.properties.position;
        //this.burstBubbleSound();
    };



    this.burstBubbleSound = function(location) {

        // var audioOptions = {
        //     volume: 0.5,
        //     position: location
        // }

        //Audio.playSound(popSound, audioOptions);

    }


})