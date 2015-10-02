//
//  cat.js
//  examples/toybox/entityScripts
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;

    Cat = function() {
        _this = this;
        this.meowSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Animals/cat_meow.wav");
    };

    Cat.prototype = {
        isMeowing:false,
        startTouch: function() {
            var _ t=this;
            if(this.isMeowing!==true){
                this.meow();
                this.isMeowing=true;
                    Script.setTimeout(function(){
                    _t.isMeowing=false;
                },2000)
            }

        },

        meow: function() {

            Audio.playSound(this.meowSound, {
                position: this.position,
                volume: .1
            });
        },
      
        preload: function(entityID) {
            this.entityID = entityID;
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
        },

        unload: function() {
            Script.update.disconnect(this.update);
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Cat();
});
