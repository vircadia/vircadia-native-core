//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {

    Script.include(Script.resolvePath('virtualBaton.js'));

    var self = this;

    var baton;
    var iOwn = false;
    var currentInterval;
    var _entityId;

    function startUpdate() {
        iOwn = true;
        print('i am the owner ' + _entityId)
    }

    function stopUpdateAndReclaim() {
        print('i released the object ' + _entityId)
        iOwn = false;
        baton.claim(startUpdate, stopUpdateAndReclaim);
    }

    this.preload = function(entityId) {
        this.isConnected = false;
        this.entityId = entityId;
        _entityId = entityId;
        this.minVelocity = 1;
        this.maxVelocity = 5;
        this.minAngularVelocity = 0.01;
        this.maxAngularVelocity = 0.03;
        baton = virtualBaton({
            batonName: 'io.highfidelity.vesicles:' + entityId, // One winner for each entity
        });
        stopUpdateAndReclaim();
       currentInterval =  Script.setInterval(self.move, self.getTotalWait())
    }


    this.getTotalWait = function() {
        return (Math.random() * 5000) * 2;
        // var avatars = AvatarList.getAvatarIdentifiers();
        // var avatarCount = avatars.length;
        // var random = Math.random() * 5000;
        // var totalWait = random * (avatarCount * 2);
        // print('cellscience color avatarcount, totalwait: ', avatarCount, totalWait)
        // return totalWait
    }


    this.move = function() {
        if (!iOwn) {
            return;
        }

        var properties = Entities.getEntityProperties(self.entityId);
        var color = properties.color;

        var newColor;
        var red = {
            red: 255,
            green: 0,
            blue: 0
        }
        var green = {
            red: 0,
            green: 255,
            blue: 0
        }
        var blue = {
            red: 0,
            green: 0,
            blue: 255
        }
        if (color.red > 0) {
            newColor = green;
        }
        if (color.green > 0) {
            newColor = blue;
        }
        if (color.blue > 0) {
            newColor = red
        }
        Entities.editEntity(self.entityId, {
            color: newColor
        });


    }


    this.unload = function() {
        baton.release(function() {});
        Script.clearTimeout(currentInterval);
    }



})