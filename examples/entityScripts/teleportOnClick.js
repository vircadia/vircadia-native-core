//
//  teleportOnClick.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/3/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to an entity, that entity will teleport your avatar if you
//  click on it the entity.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function(){ 
    this.clickDownOnEntity = function(entityID, mouseEvent) { 
        MyAvatar.position = Entities.getEntityProperties(entityID).position;
    }; 
})
