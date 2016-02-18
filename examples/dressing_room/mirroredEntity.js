//
//  mirroredEntity.js
//
//  Created by James B. Pollack @imgntn on 1/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  when grabbed, this entity relays updates to update the base entity
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;

    MirroredEntity = function() { 
         _this = this;
    };

    MirroredEntity.prototype = {
        startNearGrab: function () {
            print("I was just grabbed... entity:" + this.entityID);
        },
        continueNearGrab: function () {
            print("I am still being grabbed... entity:" + this.entityID);
            var data = {
                action:'updateBase',
                baseEntity:this.userData.doppelgangerKey.baseEntity,
                mirrorEntity:this.entityID,
                doppelganger:this.userData.doppelgangerKey.doppelganger
            }
            Messages.sendMessage('Hifi-Doppelganger-Wearable',data)
        },

        releaseGrab: function () {
            print("I was released... entity:" + this.entityID);
        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.initialProperties = Entities.getEntityProperties(this.entityID);
            this.userData = JSON.parse(this.initialProperties.userData);
        },
    };

    return new MirroredEntity();
})
