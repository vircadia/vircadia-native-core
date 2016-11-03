//
//  detectGrabExample.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 9/3/15.
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
    DetectGrabbed = function() { 
         _this = this;
    };

    DetectGrabbed.prototype = {

        setRightHand: function () {
            print("I am being held in a right hand... entity:" + this.entityID);
        },
        setLeftHand: function () {
            print("I am being held in a left hand... entity:" + this.entityID);
        },

        startDistantGrab: function () {
            print("I am being distance held... entity:" + this.entityID);
        },
        continueDistantGrab: function () {
            print("I continue to be distance held... entity:" + this.entityID);
        },

        startNearGrab: function () {
            print("I was just grabbed... entity:" + this.entityID);
        },
        continueNearGrab: function () {
            print("I am still being grabbed... entity:" + this.entityID);
        },

        releaseGrab: function () {
            print("I was released... entity:" + this.entityID);
        },

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * connecting to the update signal so we can check our grabbed state
        preload: function(entityID) {
            this.entityID = entityID;
        },
    };

    // entity scripts always need to return a newly constructed object of our type
    return new DetectGrabbed();
})
