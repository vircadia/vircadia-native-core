//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {

    Script.include('virtualBaton.js');

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
        currentInterval = Script.setInterval(self.move, self.getTotalWait())
    }


    this.getTotalWait = function() {
        return (Math.random() * 5000) * 2;
    }


    this.move = function() {
        if (!iOwn) {
            return;
        }

        var magnitudeV = self.maxVelocity;
        var directionV = {
            x: Math.random() - 0.5,
            y: Math.random() - 0.5,
            z: Math.random() - 0.5
        };

        //print("POS magnitude is " + magnitudeV + " and direction is " + directionV.x);

        var magnitudeAV = self.maxAngularVelocity;

        var directionAV = {
            x: Math.random() - 0.5,
            y: Math.random() - 0.5,
            z: Math.random() - 0.5
        };
        //print("ROT magnitude is " + magnitudeAV + " and direction is " + directionAV.x);
        Entities.editEntity(self.entityId, {
            velocity: Vec3.multiply(magnitudeV, Vec3.normalize(directionV)),
            angularVelocity: Vec3.multiply(magnitudeAV, Vec3.normalize(directionAV))

        });


    }


    this.unload = function() {
        if (baton) {
            baton.release(function() {});
        }
        Script.clearInterval(currentInterval);
    }



})