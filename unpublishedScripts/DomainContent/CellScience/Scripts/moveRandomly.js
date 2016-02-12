//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {

    var self = this;

    this.preload=function(entityId){
        this.isConnected = false;
        this.entityId = entityId;
        this.minVelocity = 1;
        this.maxVelocity = 5;
        this.minAngularVelocity = 0.01;
        this.maxAngularVelocity = 0.03;
        Script.setTimeout(self.move,self.getTotalWait())
    }


    this.getTotalWait = function() {
        var avatars = AvatarList.getAvatarIdentifiers();
        var avatarCount = avatars.length;
        var random = Math.random() * 2000;
        var totalWait = random * (avatarCount * 2);

        return totalWait
    }


    this.move = function() {
            print('jbp im the owner so move it')
          
 


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

 

        Script.setTimeout(self.move,self.getTotalWait())
    }


    this.unload = function() {

    }



})