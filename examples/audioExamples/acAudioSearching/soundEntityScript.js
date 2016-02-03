//  soundEntityScript.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 2/2/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script sends a message meant for a running AC script on its preload
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
            Script.setTimeout(function() {
                var soundData = getEntityCustomData("soundKey", _this.entityID);
                // Just a hack for now until bug https://app.asana.com/0/26225263936266/86767805352289 is fixed
                print("EBL SEND MESSAGE");
                Messages.sendMessage(_this.MESSAGE_CHANNEL, JSON.stringify({
                    id: _this.entityID
                }));
            }, 2000);
        },
    };
    // entity scripts always need to return a newly constructed object of our type
    return new SoundEntity();
});