//
//  collidingEntities.js
//  examples
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  This is an example script that creates a couple entities, and sends them on a collision course.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var currentIteration = 0;
var NUM_ITERATIONS_BEFORE_SEND = 15; // every 1/4th seconds send another
 
var numberEntitiesAdded = 0;
var MAX_ENTITIES = 1;

var velocity = {  
  x: 1, 
  y: 0,
  z: 1 };

var damping = 0;

var color = {  
  red: 255, 
  green: 255,
  blue: 0 };

function draw(deltaTime) {
    print("hello... draw()... currentIteration=" + currentIteration + "\n");
    
    // on the first iteration, setup a single entity that's slowly moving
    if (currentIteration == 0) {
        var colorGreen = { red: 0, green: 255, blue: 0 };
        var startPosition = {  
            x: 2, 
            y: 1,
            z: 2 };
        var largeRadius = 0.5;

        var properties = {
            type: "Sphere",
            dynamic: true,
            position: startPosition, 
            dimensions: {x: largeRadius, y: largeRadius, z: largeRadius},
            registrationPoint: { x: 0.5, y: 0.5, z: 0.5 },
            color: colorGreen, 
            damping: damping, 
            lifetime: 20
        };
        
        Entities.addEntity(properties);
        numberEntitiesAdded++;
    }
    
    if (currentIteration++ % NUM_ITERATIONS_BEFORE_SEND === 0) {
        print("draw()... sending another... currentIteration=" +currentIteration + "\n");

        var center = {  
            x: 0, 
            y: 1,
            z: 0 };

        var entitySize = 1.1;

        print("number of entitys=" + numberEntitiesAdded +"\n");
        
        var velocityStep = 0.1;
        if (velocity.x > 0) {
          velocity.x -= velocityStep; 
          velocity.z += velocityStep; 
          color.blue = 0;
          color.green = 255;
        } else {
          velocity.x += velocityStep; 
          velocity.z -= velocityStep; 
          color.blue = 255;
          color.green = 0;
        }
        
        if (numberEntitiesAdded <= MAX_ENTITIES) {
            var properties = {
                type: "Sphere",
                dynamic: true,
                position: center, 
                dimensions: {x: entitySize, y: entitySize, z: entitySize},
                registrationPoint: { x: 0.5, y: 0.5, z: 0.5 },
                color: color, 
                velocity: velocity, 
                damping: damping, 
                lifetime: 20
            };
            Entities.addEntity(properties);
            numberEntitiesAdded++;
        } else {
            Script.stop();
        }
        
        print("Entity Stats: " + Entities.getLifetimeInSeconds() + " seconds," + 
            " Queued packets:" + Entities.getLifetimePacketsQueued() + "," +
            " PPS:" + Entities.getLifetimePPSQueued() + "," +
            " BPS:" + Entities.getLifetimeBPSQueued() + "," +
            " Sent packets:" + Entities.getLifetimePacketsSent() + "," +
            " PPS:" + Entities.getLifetimePPS() + "," +
            " BPS:" + Entities.getLifetimeBPS() + 
            "\n");
    }
}

 
// register the call back so it fires before each data send
print("here...\n");
Entities.setPacketsPerSecond(40000);
Script.update.connect(draw);
print("and here...\n");
