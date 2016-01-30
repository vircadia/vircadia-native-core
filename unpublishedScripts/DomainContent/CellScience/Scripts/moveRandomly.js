//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {

    var self = this;

    this.preload = function(entityId) {

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

    }

    this.update = function(deltaTime) {

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

            //    print("POS magnitude is " + magnitudeV + " and direction is " + directionV.x);
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
            //     print("ROT magnitude is " + magnitudeAV + " and direction is " + directionAV.x);
            Entities.editEntity(self.entityId, {
                angularVelocity: Vec3.multiply(magnitudeAV, Vec3.normalize(directionAV))

            });

        }


    }

    this.unload = function() {

        Script.update.disconnect(this.update);

    }

    Script.update.connect(this.update);

})