//
//  HomeButton.js
//
//  Created by Dante Ruiz on 12/6/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function(){
    _this = this;

    this.preload = function(entityID) {
        print(entityID);
        this.entityID = entityID;
    }

    this.clickDownOnEntity = function(entityID, mouseEvent) {
        Messages.sendLocalMessage("home", _this.entityID);
    }

    this.startNearTrigger = function() {
        Messages.sendLocalMessage("home", _this.entityID);
    }


    this.startFarTrigger = function() {
        Messages.sendLocalMessage("home", _this.entityID);
    }

});
