//  gravity.js
//
//  Created by Philip Rosedale on March 29, 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  This entity script causes the object to move with gravitational force and be attracted to other spheres nearby.
//  The force is scaled by GRAVITY_STRENGTH, and only entities of type "Sphere" within GRAVITY_RANGE will affect it.
//  The person who has most recently grabbed this object will simulate it.
//

function Timer() {
    var time;
    var count = 0;
    var totalTime = 0;
    this.reset = function() {
        count = 0;
        totalTime = 0;
    }
    this.start = function() {
        time = new Date().getTime();
    }
    this.record = function() {
        var elapsed =  new Date().getTime() - time;
        totalTime += elapsed;
        count++;
        return elapsed;
    }
    this.count = function() {
        return count;
    }
    this.average = function() {
        return (count == 0) ? 0 : totalTime / count;
    }
    this.elapsed = function() {
        return new Date().getTime() - time;
    }
}

(function () {
    var entityID,
        wantDebug = true,
        CHECK_INTERVAL = 10.00,
        SEARCH_INTERVAL = 1000,
        GRAVITY_RANGE = 20.0,
        GRAVITY_STRENGTH = 1.0,
        MIN_VELOCITY = 0.01,
        timeoutID = null,
        timeSinceLastSearch = 0,
        timer = new Timer(),
        simulate = false,
        spheres = [];

    var printDebug = function(message) {
        if (wantDebug) {
            print(message);
        }
    }

    var greatestDimension = function(dimensions) {
        return Math.max(Math.max(dimensions.x, dimensions.y), dimensions.z);
    }

    var mass2 = function(dimensions) {
        return dimensions.x * dimensions.y * dimensions.z;
    }

    var findSpheres = function(position) {
        var entities = Entities.findEntities(position, GRAVITY_RANGE);
        spheres = [];
        for (var i = 0; i < entities.length; i++) {
            if (entityID == spheres[i]) {
                // this entity doesn't experience its own gravity.
                continue;
            }
            var props = Entities.getEntityProperties(entities[i]);
            if (props && (props.shapeType == "sphere" || props.type == "Sphere")) {
                spheres.push(entities[i]);
            }
        }
        // print("FOUND " + spheres.length + " SPHERES");
    }

    var applyGravity = function() {
        if (!simulate) {
            return;
        }

        var properties = Entities.getEntityProperties(entityID);
        if (!properties || !properties.position) {
            return;
        }

        // update the list of nearby spheres
        var deltaTime = timer.elapsed() / 1000.0;
        if (deltaTime == 0.0) {
            return;
        }
        timeSinceLastSearch += CHECK_INTERVAL;
        if (timeSinceLastSearch >= SEARCH_INTERVAL) {
            findSpheres(properties.position);
            timeSinceLastSearch = 0;
        }

        var deltaVelocity = { x: 0, y: 0, z: 0 };
        var otherCount = 0;
        var mass = mass2(properties.dimensions);

        for (var i = 0; i < spheres.length; i++) {
            otherProperties = Entities.getEntityProperties(spheres[i]);
            if (!otherProperties || !otherProperties.position) {
                continue; // sphere was deleted
            }
            otherCount++;
            var radius = Vec3.distance(properties.position, otherProperties.position);
            var otherMass = mass2(otherProperties.dimensions);
            var r = (greatestDimension(properties.dimensions) + greatestDimension(otherProperties.dimensions)) / 2;
            if (radius > r) {
                var n0 = Vec3.normalize(Vec3.subtract(otherProperties.position, properties.position));
                var n1 = Vec3.multiply(deltaTime * GRAVITY_STRENGTH * otherMass / (radius * radius), n0);
                deltaVelocity = Vec3.sum(deltaVelocity, n1);
            }
        }
        Entities.editEntity(entityID, { velocity: Vec3.sum(properties.velocity, deltaVelocity) });
        if (Vec3.length(properties.velocity) < MIN_VELOCITY) {
            print("Gravity simulation stopped due to velocity");
            simulate = false;
        } else {
            timer.start();
            timeoutID = Script.setTimeout(applyGravity, CHECK_INTERVAL);
        }
    }
    this.applyGravity = applyGravity;

    var releaseGrab = function() {
        printDebug("Gravity simulation started.");
        var properties = Entities.getEntityProperties(entityID);
        findSpheres(properties.position);
        timer.start();
        timeoutID = Script.setTimeout(applyGravity, CHECK_INTERVAL);
        simulate = true;
    }
    this.releaseGrab = releaseGrab;

    var preload = function (givenEntityID) {
        printDebug("load gravity...");
        entityID = givenEntityID;
    };
    this.preload = preload;

    var unload = function () {
        printDebug("Unload gravity...");
        if (timeoutID !== undefined) {
            Script.clearTimeout(timeoutID);
        }
        if (simulate) {
            Entities.editEntity(entityID, { velocity: { x: 0, y: 0, z: 0 } });
        }
    };
    this.unload = unload;

    var handleMessages = function(channel, message, sender) {
        if (channel === 'Hifi-Object-Manipulation') {
            try {
                var parsedMessage = JSON.parse(message);
                if (parsedMessage.action === 'grab' && parsedMessage.grabbedEntity == entityID) {
                    print("Gravity simulation stopped due to grab");
                    simulate = false;
                }
            } catch (e) {
                print('error parsing Hifi-Object-Manipulation message: ' + message);
            }
        }
    }
    this.handleMessages = handleMessages;

    Messages.messageReceived.connect(this.handleMessages);
    Messages.subscribe('Hifi-Object-Manipulation');
});
