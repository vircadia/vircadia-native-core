/*
    domainUserCount.js

    Created by Kalila L. on 5 Aug 2020
    Copyright 2020 Vircadia and contributors.
    
    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

// This is an entity-server script.
// This script is meant to be attached to a text entity because it edits 
// the text property of its host entity in order to display the statistic.

(function () {
    "use strict";
    this.entityID = null;
    var _this = this;
    var timer;
    
    var POSITION = Vec3.ZERO; // 0, 0, 0
    var RANGE = 1000; // 1000 meters
    var UPDATE_INTERVAL = 1000; // 1 second
    
    function getAvatarCount() {
        return AvatarList.getAvatarsInRange(POSITION, RANGE);
    }
    
    function updateAvatarCount() {
        var avatarCount = getAvatarCount();
        
        Entities.editEntity(_this.entityID, {
            text: avatarCount.length
        });
    }
    
    // Standard preload and unload, initialize the entity script here.

    this.preload = function (ourID) {
        this.entityID = ourID;
        
        timer = Script.setInterval(function () {
            updateAvatarCount();
        }, UPDATE_INTERVAL);
    };

    this.unload = function(entityID) {
        Script.clearInterval(timer);
    };

});