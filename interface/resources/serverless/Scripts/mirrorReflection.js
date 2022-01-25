//
//  mirrorReflection.js
//
//  Created by Rebecca Stankus on 8/30/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// Attach `mirrorReflection.js` to a zone entity that is parented to
// the box entity whose Script is `mirrorClient.js`.
// When a user enters this zone, the mirror will turn on.
// When a user leaves this zone, the mirror will turn off.

(function () {
    var mirrorID, reflectionAreaID;
    // get id of reflection area and mirror
    this.preload = function(entityID) {
        reflectionAreaID = entityID;
        mirrorID = Entities.getEntityProperties(reflectionAreaID, 'parentID').parentID;
    };

    // when avatar enters reflection area, begin reflecting
    this.enterEntity = function(entityID){
        Entities.callEntityMethod(mirrorID, 'mirrorLocalEntityOn');
    };

    // when avatar leaves reflection area, stop reflecting
    this.leaveEntity = function (entityID) {
        Entities.callEntityMethod(mirrorID, 'mirrorLocalEntityOff');
    };
});
