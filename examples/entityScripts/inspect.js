//
//  inspect.js
//  examples/entityScripts
//
//  If you attach this script to an object, you can pick it up and look at it by clicking
//  and holding/dragging.  When you release, the object will return to it's original location.
//
//  Created by Philip Rosedale on November 21, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function(){ 
    this.entityID = null;
    this.properties = null;
    this.originalPosition = null;
    this.originalRotation = null;
    this.radius = null;
    this.lastMouseX = null;
    this.lastMouseY = null;
    this.sound = null;
    this.isHolding = null;

    this.preload = function(entityID) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.sound = SoundCache.getSound("http://public.highfidelity.io/sounds/Footsteps/FootstepW3Left-12db.wav");
    }

    // Play drop sound
    this.playSound = function() {
        if (this.sound && this.sound.downloaded) {
            Audio.playSound(this.sound, { position: this.properties.position });
        }
    }
    // All callbacks start by updating the properties
    this.updateProperties = function(entityID) {
        if (this.entityID === null) {
            this.entityID = entityID;
        }
        this.properties = Entities.getEntityProperties(this.entityID);
    };

    this.clickDownOnEntity = function(entityID, mouseEvent) {
        this.updateProperties(entityID);  

        // record the old location of this object
        this.originalPosition = this.properties.position;
        this.originalRotation = this.properties.rotation;
        this.radius = Vec3.length(this.properties.dimensions) / 2.0;
        this.lastMouseX = mouseEvent.x; 
        this.lastMouseY = mouseEvent.y;

        var newPosition = Vec3.sum(Camera.position, Vec3.multiply(Quat.getFront(Camera.getOrientation()), this.radius * 3.0));
        // Place object in front of me
        Entities.editEntity(this.entityID, { position: newPosition }); 
        this.isHolding = true;
        this.playSound();
    };
    this.holdingClickOnEntity = function(entityID, mouseEvent) {
        this.updateProperties(entityID); 
        if (mouseEvent.x != this.lastMouseX) {
            var newRotation = Quat.multiply(Quat.fromPitchYawRollDegrees(0, mouseEvent.x - this.lastMouseX, 0), this.properties.rotation);
     
            this.lastMouseX = mouseEvent.x;
            Entities.editEntity(this.entityID, { rotation: newRotation });
        }
        if (mouseEvent.y != this.lastMouseY) {
            var newRotation = Quat.multiply(Quat.fromPitchYawRollDegrees(mouseEvent.y - this.lastMouseY, 0, 0), this.properties.rotation);
            this.lastMouseY = mouseEvent.y;
            Entities.editEntity(this.entityID, { rotation: newRotation });
        }
    };

    this.returnItem = function(entityID) {
        this.updateProperties(entityID); 
        Entities.editEntity(this.entityID, { position: this.originalPosition, 
                                             rotation: this.originalRotation });
        this.isHolding = false;
        this.playSound();
    };

    this.clickReleaseOnEntity = function(entityID, mouseEvent) {
        this.returnItem(entityID);
    };

    this.unload = function(entityID) {
        if (this.isHolding) {
            this.returnItem(entityID);
        }
    };

})
