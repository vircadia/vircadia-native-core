//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {

    var self = this;

    this.preload = function(entityId) {
        //print('preload move randomly')
        this.isConnected = false;
        this.entityId = entityId;
        this.updateInterval = 100;
        this.posFrame = 0;
        this.rotFrame = 0;
        this.posInterval = 100;
        this.rotInterval = 100;
        this.minVelocity = 1;
        this.maxVelocity = 5;
        this.minAngularVelocity = 0.01;
        this.maxAngularVelocity = 0.03;

        this.initialize(entityId);
        this.initTimeout = null;


        var userData = {
            ownershipKey: {
                owner: MyAvatar.sessionUUID
            },
            grabbableKey: {
                grabbable: false
            }
        };

        Entities.editEntity(entityId, {
            userData: JSON.stringify(userData)
        })
    }

    this.initialize = function(entityId) {
        //print('move randomly  should initialize' + entityId)
        var properties = Entities.getEntityProperties(entityId);
        if (properties.userData.length === 0 || properties.hasOwnProperty('userData') === false) {
            self.initTimeout = Script.setTimeout(function() {
                //print('no user data yet, try again in one second')
                self.initialize(entityId);
            }, 1000)

        } else {
            //print('userdata before parse attempt' + properties.userData)
            self.userData = null;
            try {
                self.userData = JSON.parse(properties.userData);
            } catch (err) {
                //print('error parsing json');
                //print('properties are:' + properties.userData);
                return;
            }
            Script.update.connect(self.update);
            this.isConnected = true;
        }
    }

    this.update = function(deltaTime) {
        // print('jbp in update')
        var data = Entities.getEntityProperties(self.entityId, 'userData').userData;
        var userData;
        try {
            userData = JSON.parse(data)
        } catch (e) {
            //print('error parsing json' + data)
            return;
        };

        // print('userdata is' + data)
        //if the entity doesnt have an owner set yet
        if (userData.hasOwnProperty('ownershipKey') !== true) {
            //print('no movement owner yet')
            return;
        }

        //print('owner is:::' + userData.ownershipKey.owner)
            //get all the avatars to see if the owner is around
        var avatars = AvatarList.getAvatarIdentifiers();
        var ownerIsAround = false;

        //if the current owner is not me...
        if (userData.ownershipKey.owner !== MyAvatar.sessionUUID) {

            //look to see if the current owner is around anymore
            for (var i = 0; i < avatars.length; i++) {
                if (avatars[i] === userData.ownershipKey.owner) {
                    ownerIsAround = true
                        //the owner is around
                    return;
                };
            }

            //if the owner is not around, then take ownership
            if (ownerIsAround === false) {
                //print('taking ownership')

                var userData = {
                    ownershipKey: {
                        owner: MyAvatar.sessionUUID
                    },
                    grabbableKey: {
                        grabbable: false
                    }
                };
                Entities.editEntity(self.entityId, {
                    userData: JSON.stringify(data)
                })
            }
        }
        //but if the current owner IS me, then move it
        else {
            //print('jbp im the owner so move it')
            self.posFrame++;
            self.rotFrame++;

            if (self.posFrame > self.posInterval) {

                self.posInterval = 100 * Math.random() + 300;
                self.posFrame = 0;

                var magnitudeV = self.maxVelocity;
                var directionV = {
                    x: Math.random() - 0.5,
                    y: Math.random() - 0.5,
                    z: Math.random() - 0.5
                };

                //print("POS magnitude is " + magnitudeV + " and direction is " + directionV.x);
                Entities.editEntity(self.entityId, {
                    velocity: Vec3.multiply(magnitudeV, Vec3.normalize(directionV))

                });

            }

            if (self.rotFrame > self.rotInterval) {

                self.rotInterval = 100 * Math.random() + 250;
                self.rotFrame = 0;

                var magnitudeAV = self.maxAngularVelocity;

                var directionAV = {
                    x: Math.random() - 0.5,
                    y: Math.random() - 0.5,
                    z: Math.random() - 0.5
                };
                //print("ROT magnitude is " + magnitudeAV + " and direction is " + directionAV.x);
                Entities.editEntity(self.entityId, {
                    angularVelocity: Vec3.multiply(magnitudeAV, Vec3.normalize(directionAV))

                });

            }

        }

    }

    this.unload = function() {
        if (this.initTimeout !== null) {
            Script.clearTimeout(this.initTimeout);
        }

        if (this.isConnected === true) {
            Script.update.disconnect(this.update);
        }

    }



})