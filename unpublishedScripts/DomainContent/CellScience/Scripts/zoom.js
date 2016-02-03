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

        var properties = Entities.getEntityProperties(entityID);
        portalDestination = properties.userData;
        animationURL = properties.modelURL;
        this.soundOptions = {
            stereo: true,
            loop: false,
            localOnly: false,
            position: this.position,
            volume: 0.035
        };
        
        this.teleportSound = SoundCache.getSound("https://hifi-content.s3.amazonaws.com/DomainContent/CellScience/Audio/whoosh.wav");
        //print('Script.clearTimeout PRELOADING A ZOOM ENTITY')
        print(" portal destination is " + portalDestination);
    }

    this.enterEntity = function(entityID) {
        print('Script.clearTimeout ENTERED A BOUNDARY ENTITY, SHOULD ZOOM', entityID)

        var data = JSON.parse(Entities.getEntityProperties(this.entityId).userData);


        if (data != null) {
            print("Teleporting to (" + data.location.x + ", " + data.location.y + ", " + data.location.z + ")");
            if (self.teleportSound.downloaded) {
                //print("play sound");
                Audio.playSound(self.teleportSound, self.soundOptions);
            } else {
                //print("not downloaded");
            }

            this.lookAt(data.target, data.location);


        }

    }

    this.lookAt = function(targetPosition, avatarPosition) {
        var direction = Vec3.normalize(Vec3.subtract(MyAvatar.position, targetPosition));

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

        MyAvatar.goToLocation(avatarPosition, true, yaw);
        MyAvatar.headYaw = 0;
    }



    this.leaveEntity = function(entityID) {
        Entities.editEntity(entityID, {
            animationURL: animationURL,
            animationSettings: '{ "frameIndex": 1, "running": false }'
        });
        this.entered = false;
        //playSound();
    }

    this.hoverEnterEntity = function(entityID) {
        Entities.editEntity(entityID, {
            animationURL: animationURL,
            animationSettings: '{ "fps": 24, "firstFrame": 1, "lastFrame": 25, "frameIndex": 1, "running": true, "hold": true }'
        });
    }
})