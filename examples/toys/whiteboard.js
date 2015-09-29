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

        setRightHand: function() {
            this.hand = 'RIGHT';
            this.getHandPosition = MyAvatar.getRightPalmPosition;
            this.getHandRotation = MyAvatar.getRightPalmRotation;
        },
        setLeftHand: function() {
            this.hand = 'LEFT';
            this.getHandPosition = MyAvatar.getLeftPalmPosition;
            this.getHandRotation = MyAvatar.getLeftPalmRotation;
        },

        startNearGrabNonColliding: function() {
            print("START")
            this.whichHand = this.hand;
            this.laserPointer = Entities.addEntity({
                type: "Box",
                dimensions: {x: .02, y: .02, z: 0.001},
                color: {red: 200, green: 10, blue: 10},
                rotation: this.rotation
            });

        setEntityCustomData(this.resetKey, this.laserPointer, {
            resetMe: true
        });

        },

        continueNearGrabbingNonColliding: function() {
            var handPosition = this.getHandPosition();
            var pickRay = {
                origin: handPosition,
                direction: Quat.getUp(this.getHandRotation())
            };
            var intersection = Entities.findRayIntersection(pickRay, true);
            if(intersection.intersects) {
                Entities.editEntity(this.laserPointer, {
                    position: intersection.intersection
                });
            }

        },

        releaseGrab: function() {
            Entities.deleteEntity(this.laserPointer);
        },


        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        preload: function(entityID) {
            this.entityID = entityID;
            var props = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
            this.position = props.position;
            this.rotation = props.rotation;
            this.resetKey = "resetMe";

        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Whiteboard();
})