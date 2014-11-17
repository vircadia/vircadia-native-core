//
//  movable.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function(){ 

    this.entityID = null;
    this.properties = null;
    this.graboffset = null;

    // Pr, Vr are respectively the Ray's Point of origin and Vector director
    // Pp, Np are respectively the Plane's Point of origin and Normal vector
    this.rayPlaneIntersection = function(Pr, Vr, Pp, Np) {
        var d = -Vec3.dot(Pp, Np);
        var t = -(Vec3.dot(Pr, Np) + d) / Vec3.dot(Vr, Np);
        return Vec3.sum(Pr, Vec3.multiply(t, Vr));
    };

    // updates the piece position based on mouse input
    this.updatePosition = function(mouseEvent) {
        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var upVector = { x: 0, y: 1, z: 0 };
        var intersection = this.rayPlaneIntersection(pickRay.origin, pickRay.direction,
                                                     this.properties.position, upVector);
                                                     
        var newPosition = Vec3.sum(intersection, this.graboffset);
        Entities.editEntity(this.entityID, { position: newPosition });
    };

    this.grab = function(mouseEvent) {
        // first calculate the offset
        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var upVector = { x: 0, y: 1, z: 0 };
        var intersection = this.rayPlaneIntersection(pickRay.origin, pickRay.direction,
                                                     this.properties.position, upVector);                                 
        this.graboffset = Vec3.subtract(this.properties.position, intersection);
        this.updatePosition(mouseEvent);
    };
    
    this.move = function(mouseEvent) {
        this.updatePosition(mouseEvent);
    };
    
    this.release = function(mouseEvent) {
        this.updatePosition(mouseEvent);
    };
    
      // All callbacks start by updating the properties
    this.updateProperties = function(entityID) {
        if (this.entityID === null || !this.entityID.isKnownID) {
            this.entityID = Entities.identifyEntity(entityID);
        }
        this.properties = Entities.getEntityProperties(this.entityID);
    };
  
    this.preload = function(entityID) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
    };
    
    this.clickDownOnEntity = function(entityID, mouseEvent) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.grab(mouseEvent);
    };
    
    this.holdingClickOnEntity = function(entityID, mouseEvent) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.move(mouseEvent);
    };
    this.clickReleaseOnEntity = function(entityID, mouseEvent) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.release(mouseEvent);
    };

})
