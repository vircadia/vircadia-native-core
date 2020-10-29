//
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){ 

    var soundURL ='https://cdn-1.vircadia.com/us-e-1/Developer/Tutorials/soundMaker/bell.wav';
    var ringSound;

    this.preload = function(entityID) { 
        print("preload("+entityID+")");
        ringSound = SoundCache.getSound(soundURL);
    }; 

    this.clickDownOnEntity = function(entityID, mouseEvent) { 
        var bellPosition = Entities.getEntityProperties(entityID).position;
        print("clickDownOnEntity()...");
        Audio.playSound(ringSound,  {
            position: bellPosition,
            volume: 0.5
            });
    }; 

})
