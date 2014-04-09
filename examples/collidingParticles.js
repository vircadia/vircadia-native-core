//
//  collidingParticles.js
//  examples
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  This is an example script that creates a couple particles, and sends them on a collision course.
//  One of the particles has a script that when it collides with another particle, it swaps colors with that particle.
//  The other particle has a script that when it collides with another particle it set's it's script to a suicide script.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var currentIteration = 0;
var NUM_ITERATIONS_BEFORE_SEND = 15; // every 1/4th seconds send another
 
var numberParticlesAdded = 0;
var MAX_PARTICLES = 1;

var velocity = {  
  x: 1, 
  y: 0,
  z: 1 };

var gravity = {  
  x: 0, 
  y: 0,
  z: 0 };

var damping = 0.1;

var scriptA = " " +
             " function collisionWithParticle(other, collision) { " +
             "   print('collisionWithParticle(other.getID()=' + other.getID() + ')...'); " +
             "   Vec3.print('penetration=', collision.penetration); " +
             "   Vec3.print('contactPoint=', collision.contactPoint); " +
             "   print('myID=' + Particle.getID() + '\\n'); " +
             "   var colorBlack = { red: 0, green: 0, blue: 0 };" +
             "   var otherColor = other.getColor();" +
             "   print('otherColor=' + otherColor.red + ', ' + otherColor.green + ', ' + otherColor.blue + '\\n'); " +
             "   var myColor = Particle.getColor();" +
             "   print('myColor=' + myColor.red + ', ' + myColor.green + ', ' + myColor.blue + '\\n'); " +
             "   Particle.setColor(otherColor); " +
             "   other.setColor(myColor); " +
             " } " +
             " Particle.collisionWithParticle.connect(collisionWithParticle); " +
             " ";

var scriptB = " " +
             " function collisionWithParticle(other, collision) { " +
             "   print('collisionWithParticle(other.getID()=' + other.getID() + ')...'); " +
             "   Vec3.print('penetration=', collision.penetration); " +
             "   Vec3.print('contactPoint=', collision.contactPoint); " +
             "   print('myID=' + Particle.getID() + '\\n'); " +
             "   Particle.setScript('Particle.setShouldDie(true);'); " +
             " } " +
             " Particle.collisionWithParticle.connect(collisionWithParticle); " +
             " ";

var color = {  
  red: 255, 
  green: 255,
  blue: 0 };

function draw(deltaTime) {
    print("hello... draw()... currentIteration=" + currentIteration + "\n");
    
    // on the first iteration, setup a single particle that's slowly moving
    if (currentIteration == 0) {
        var colorGreen = { red: 0, green: 255, blue: 0 };
        var startPosition = {  
            x: 2, 
            y: 0,
            z: 2 };
        var largeRadius = 0.5;
        var verySlow = {  
            x: 0.01, 
            y: 0,
            z: 0.01 };

        var properties = {
            position: startPosition, 
            radius: largeRadius, 
            color: colorGreen, 
            velocity: verySlow, 
            gravity: gravity, 
            damping: damping, 
            inHand: false, 
            script: scriptA
        };
        
        Particles.addParticle(properties);
        print("hello... added particle... script=\n");
        print(scriptA);
        numberParticlesAdded++;
    }
    
    if (currentIteration++ % NUM_ITERATIONS_BEFORE_SEND === 0) {
        print("draw()... sending another... currentIteration=" +currentIteration + "\n");

        var center = {  
            x: 0, 
            y: 0,
            z: 0 };

        var particleSize = 0.1;

        print("number of particles=" + numberParticlesAdded +"\n");
        
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
        
        if (numberParticlesAdded <= MAX_PARTICLES) {
            var properties = {
                position: center, 
                radius: particleSize, 
                color: color, 
                velocity: velocity, 
                gravity: gravity, 
                damping: damping, 
                inHand: false, 
                script: scriptB
            };
            Particles.addParticle(properties);
            print("hello... added particle... script=\n");
            print(scriptB);
            numberParticlesAdded++;
        } else {
            Script.stop();
        }
        
        print("Particles Stats: " + Particles.getLifetimeInSeconds() + " seconds," + 
            " Queued packets:" + Particles.getLifetimePacketsQueued() + "," +
            " PPS:" + Particles.getLifetimePPSQueued() + "," +
            " BPS:" + Particles.getLifetimeBPSQueued() + "," +
            " Sent packets:" + Particles.getLifetimePacketsSent() + "," +
            " PPS:" + Particles.getLifetimePPS() + "," +
            " BPS:" + Particles.getLifetimeBPS() + 
            "\n");
    }
}

 
// register the call back so it fires before each data send
print("here...\n");
Particles.setPacketsPerSecond(40000);
Script.update.connect(draw);
print("and here...\n");
