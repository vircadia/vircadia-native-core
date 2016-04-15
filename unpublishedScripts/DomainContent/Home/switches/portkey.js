//
//  portkey.js
//
//
//  Created by Eric Levin on 3/28/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This entity script is designed to teleport a user to an in-world location specified in object's userdata
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function() {

    Script.include("atp:/utils.js");
    var _this = this;


    this.startFarTrigger = function() {
        _this.teleport();
    }

    this.startNearTrigger = function() {
        _this.teleport();
    }

    this.startNearGrab = function() {
        _this.teleport();
    }

    this.teleport = function() {
        Window.location = _this.portkeyLink;
    }

    this.preload = function(entityID) {
        _this.entityID = entityID;
        _this.portkeyLink = Entities.getEntityProperties(_this.entityID, "href").href;
    }

});