//
//  changeColorOnHover.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example of an entity script which when assigned to a non-model entity like a box or sphere, will
//  change the color of the entity when you hover over it. This script uses the JavaScript prototype/class functionality
//  to construct an object that has methods for hoverEnterEntity and hoverLeaveEntity;
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


ChangeColorOnHover = function(){
  this.oldColor = {};
  this.oldColorKnown = false;
};

ChangeColorOnHover.prototype = {

  storeOldColor: function(entityID) {
    var oldProperties = Entities.getEntityProperties(entityID);
    this.oldColor = oldProperties.color;
    this.oldColorKnown = true;
    print("storing old color... this.oldColor=" + this.oldColor.red + "," + this.oldColor.green + "," + this.oldColor.blue);
  },
  
  preload: function(entityID) {
    print("preload");
    this.storeOldColor(entityID);
  },
  
  hoverEnterEntity: function(entityID, mouseEvent) { 
    print("hoverEnterEntity");
    if (!this.oldColorKnown) {
        this.storeOldColor(entityID);
    }
    Entities.editEntity(entityID, { color: { red: 0, green: 255, blue: 255} });
  },
  
  
  hoverLeaveEntity: function(entityID, mouseEvent) { 
    print("hoverLeaveEntity");
    if (this.oldColorKnown) {
        print("leave restoring old color... this.oldColor=" 
                    + this.oldColor.red + "," + this.oldColor.green + "," + this.oldColor.blue);
        Entities.editEntity(entityID, { color: this.oldColor });
    }
  }
};


