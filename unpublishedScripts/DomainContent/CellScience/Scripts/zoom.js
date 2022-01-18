//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var teleport;
    var portalDestination;
    var animationURL;
    var self = this;

    this.entered = true;

    this.preload = function(entityID) {
        this.entityId = entityID;
        this.initialize(entityID);
        this.initTimeout = null;
    }

    this.initialize = function(entityID) {
        // print(' should initialize')
        var properties = Entities.getEntityProperties(entityID);
        if (properties.hasOwnProperty('userData') === false) {
            self.initTimeout = Script.setTimeout(function() {
                // print(' no user data yet, try again in one second')
                self.initialize(entityID);
            }, 1000)
        } else if (properties.userData.length === 0) {
            self.initTimeout = Script.setTimeout(function() {
                // print(' no user data yet, try again in one second')
                self.initialize(entityID);
            }, 1000)
        } else {
            // print(' has userData')
            self.portalDestination = properties.userData;
            animationURL = properties.modelURL;
            self.soundOptions = {
                stereo: true,
                loop: false,
                localOnly: false,
                position: properties.position,
                volume: 0.5
            };

            self.teleportSound = SoundCache.getSound("https://cdn-1.vircadia.com/us-e-1/DomainContent/CellScience/Audio/whoosh.wav");
            // print(" portal destination is " + self.portalDestination);
        }
    }

    this.enterEntity = function(entityID) {
        //print('ENTERED A BOUNDARY ENTITY, SHOULD ZOOM', entityID)
        var data = JSON.parse(Entities.getEntityProperties(this.entityId).userData);
        //print('DATA IS::' + data)
        if (data != null) {
            print("Teleporting to (" + data.location.x + ", " + data.location.y + ", " + data.location.z + ")");

            MyAvatar.position = data.location;

        }

    }

    this.lookAtTarget = function(entryPoint, target) {
        //print('SHOULD LOOK AT TARGET')
        var direction = Vec3.normalize(Vec3.subtract(entryPoint, target));
        var pitch = Quat.angleAxis(Math.asin(-direction.y) * 180.0 / Math.PI, {
            x: 1,
            y: 0,
            z: 0
        });
        var yaw = Quat.angleAxis(Math.atan2(direction.x, direction.z) * 180.0 / Math.PI, {
            x: 0,
            y: 1,
            z: 0
        });

        MyAvatar.goToLocation(entryPoint, true, yaw);

        MyAvatar.headYaw = 0;

    }



    this.leaveEntity = function(entityID) {
        Entities.editEntity(entityID, {
            animationURL: animationURL,
            animationSettings: '{ "frameIndex": 1, "running": false }'
        });
        this.entered = false;
        if (this.initTimeout !== null) {
            Script.clearTimeout(this.initTimeout);
        }
        //playSound();
    }

    this.unload = function() {
        if (this.initTimeout !== null) {
            Script.clearTimeout(this.initTimeout);
        }
    }

})