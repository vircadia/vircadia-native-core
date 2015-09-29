//
//  detectGrabExample.js
//  examples/entityScripts
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to an entity, will detect when the entity is being grabbed by the hydraGrab script
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var _this;


    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    Whiteboard = function() {
        _this = this;

    };

    Whiteboard.prototype = {

   
        startNearGrabNonColliding: function() {
            print("HEY")
        },


        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        preload: function(entityID) {
            this.entityID = entityID;

            this.position = Entities.getEntityProperties(this.entityID, "position").position;
      
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Whiteboard();
})