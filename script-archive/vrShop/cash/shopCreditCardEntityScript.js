// shopCreditCardEntityScript.js
//

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    
    var _this;
    var entityProperties = null;
    
    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    CreditCard = function() { 
         _this = this;
    };
    
    CreditCard.prototype = {
        
        preload: function(entityID) {
            this.entityID = entityID;
            var ownerObj = getEntityCustomData('ownerKey', this.entityID, null);
            if (ownerObj.ownerID === MyAvatar.sessionUUID) {
                myCard = true;
                entityProperties = Entities.getEntityProperties(this.entityID);
            }
        },
        
        releaseGrab: function () {
            //reset the card to its original properties (position, angular velocity, ecc)
            Entities.editEntity(_this.entityID, entityProperties);
        },
        
        unload: function (entityID) {
            Entities.deleteEntity(this.entityID);
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new CreditCard();
})