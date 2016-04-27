//
//  Created by Philip Rosedale on March 7, 2016
//  Copyright 2016 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  A firefly which is animated by passerbys.  It's physical, no gravity, periodic forces applied. 
//  If a firefly is found to 
//  

(function () {
	var entityID,
		timeoutID = null,
		properties,
		shouldSimulate = false,
		ACTIVE_CHECK_INTERVAL = 100,     //  milliseconds
		INACTIVE_CHECK_INTERVAL = 1000,  //  milliseconds
		MAX_DISTANCE_TO_SIMULATE = 20,   //  meters
		LIGHT_LIFETIME = 1400,			 //  milliseconds a firefly light will stay alive
		BUMP_SPEED = 1.5,			     //  average velocity given by a bump
        BUMP_CHANCE = 0.33,
		MIN_SPEED = 0.125,				 //  below this speed, firefly gets a new bump  
        SPIN_SPEED = 3.5,
        BRIGHTNESS = 0.25,
        wantDebug = false

    function randomVector(size) {
        return { x: (Math.random() - 0.5) * size, 
                 y: (Math.random() - 0.5) * size,
                 z: (Math.random() - 0.5) * size };
    }

    function printDebug(message) {
        if (wantDebug) {
            print(message);
        }
    }

	function maybe() {  
        properties = Entities.getEntityProperties(entityID);
        var speed = Vec3.length(properties.velocity);
        var distance = Vec3.distance(MyAvatar.position, properties.position);
        printDebug("maybe: speed: " + speed +  ", distance: " + distance);
        if (shouldSimulate) {
        	//  We are simulating this firefly, so do stuff:
            if (distance > MAX_DISTANCE_TO_SIMULATE) {
                shouldSimulate = false; 
            } else if ((speed < MIN_SPEED) && (Math.random() < BUMP_CHANCE)) {
                bump();
                makeLight();
            } 
        } else if (Vec3.length(properties.velocity) == 0.0) {
        	//  We've found a firefly that is not being simulated, so maybe take it over
        	if (distance < MAX_DISTANCE_TO_SIMULATE) {
        		shouldSimulate = true;
        	} 
        }
        timeoutID = Script.setTimeout(maybe, (shouldSimulate == true) ? ACTIVE_CHECK_INTERVAL : INACTIVE_CHECK_INTERVAL);
    }
 
    function bump() {
    	//  Give the firefly a little brownian hop 
        printDebug("bump!");
        var velocity = randomVector(BUMP_SPEED);
        if (velocity.y < 0.0) { velocity.y *= -1.0 };
    	Entities.editEntity(entityID, { velocity: velocity,
                                        angularVelocity: randomVector(SPIN_SPEED) });
    }

    function makeLight() {
        printDebug("make light!");
    	//  create a light attached to the firefly that lives for a while 
    	Entities.addEntity({
                    type: "Light",
                    name: "firefly light",
                    intensity: 4.0 * BRIGHTNESS,
                    falloffRadius: 8.0 * BRIGHTNESS,
                    dimensions: {
                        x: 30 * BRIGHTNESS,
                        y: 30 * BRIGHTNESS,
                        z: 30 * BRIGHTNESS
                    },
                    position: Vec3.sum(properties.position, { x: 0, y: 0.2, z: 0 }),
                    parentID: entityID,
                    color: {
                        red: 150 + Math.random() * 100,
                        green: 100 + Math.random() * 50,
                        blue: 150 + Math.random() * 100
                    },
                    lifetime:  LIGHT_LIFETIME / 1000
                });
    }

    this.preload = function (givenEntityID) {
        printDebug("preload firefly...");
        entityID = givenEntityID;
        timeoutID = Script.setTimeout(maybe, ACTIVE_CHECK_INTERVAL);
    };
    this.unload = function () {
        printDebug("unload firefly...");
        if (timeoutID !== undefined) {
            Script.clearTimeout(timeoutID);
        }
    };
})