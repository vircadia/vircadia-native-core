// shopCartZeroEntityScript.js
//

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function() {
    BALL_ANGULAR_VELOCITY = {x:0, y:5, z:0}
    var _this;
    

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    ShopCartZero = function() { 
         _this = this;
    };
    

    ShopCartZero.prototype = {

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * connecting to the update signal so we can check our grabbed state
        preload: function(entityID) {
            this.entityID = entityID;
            Entities.editEntity(_this.entityID, { angularVelocity: BALL_ANGULAR_VELOCITY });
            Entities.editEntity(_this.entityID, { angularDamping: 0 });
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new ShopCartZero();
})