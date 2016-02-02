//  soundEntityScript.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 2/2/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script sends a message meant for a running AC script on preload
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    Script.include("../../libraries/utils.js");
    var _this;
    // this is the "constructor" for the entity as a JS object we don't do much here
    var SoundEntity = function() {
        _this = this;
        this.MESSAGE_CHANNEL = "Hifi-Sound-Entity";
    };

    SoundEntity.prototype = {     
        preload: function(entityID) {
            this.entityID = entityID;
            print("EBL SENDING MESSAGE " + this.entityID);
            Messages.sendMessage(this.MESSAGE_CHANNEL, JSON.stringify({id: this.entityID}));
        },
    };
    // entity scripts always need to return a newly constructed object of our type
    return new SoundEntity();
});
