//
//  collidingParticles.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that creates a couple particles, and sends them on a collision course.
//  One of the particles has a script that when it collides with another particle, it swaps colors with that particle.
//  The other particle has a script that when it collides with another particle it set's it's script to a suicide script.
//
//

var currentIteration = 0;
var NUM_ITERATIONS_BEFORE_SEND = 15; // every 1/4th seconds send another
 
var numberParticlesAdded = 0;
var MAX_PARTICLES = 1;

var velocity = {  
  x: 1/TREE_SCALE, 
  y: 0/TREE_SCALE,
  z: 1/TREE_SCALE };

var gravity = {  
  x: 0/TREE_SCALE, 
  y: 0/TREE_SCALE,
  z: 0/TREE_SCALE };

var damping = 0.1;

var scriptA = " " +
             " function collisionWithParticle(other) { " +
             "   print('collisionWithParticle(other.getID()=' + other.getID() + ')...'); " +
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
             " function collisionWithParticle(other) { " +
             "   print('collisionWithParticle(other.getID()=' + other.getID() + ')...'); " +
             "   print('myID=' + Particle.getID() + '\\n'); " +
             "   Particle.setScript('Particle.setShouldDie(true);'); " +
             " } " +
             " Particle.collisionWithParticle.connect(collisionWithParticle); " +
             " ";

var color = {  
  red: 255, 
  green: 255,
  blue: 0 };

function draw() {
    print("hello... draw()... currentIteration=" + currentIteration + "\n");
    
    // on the first iteration, setup a single particle that's slowly moving
    if (currentIteration == 0) {
        var colorGreen = { red: 0, green: 255, blue: 0 };
        var startPosition = {  
            x: 2/TREE_SCALE, 
            y: 0/TREE_SCALE,
            z: 2/TREE_SCALE };
        var largeRadius = 0.5/TREE_SCALE;
        var verySlow = {  
            x: 0.01/TREE_SCALE, 
            y: 0/TREE_SCALE,
            z: 0.01/TREE_SCALE };
        
        Particles.queueParticleAdd(startPosition, largeRadius, colorGreen,  verySlow, gravity, damping, false, scriptA);
        print("hello... added particle... script=\n");
        print(scriptA);
        numberParticlesAdded++;
    }
    
    if (currentIteration++ % NUM_ITERATIONS_BEFORE_SEND === 0) {
        print("draw()... sending another... currentIteration=" +currentIteration + "\n");

        var center = {  
            x: 0/TREE_SCALE, 
            y: 0/TREE_SCALE,
            z: 0/TREE_SCALE };

        var particleSize = 0.1 / TREE_SCALE;

        print("number of particles=" + numberParticlesAdded +"\n");
        
        var velocityStep = 0.1/TREE_SCALE;
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
            Particles.queueParticleAdd(center, particleSize, color,  velocity, gravity, damping, false, scriptB);
            print("hello... added particle... script=\n");
            print(scriptB);
            numberParticlesAdded++;
        } else {
            Agent.stop();
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
Agent.willSendVisualDataCallback.connect(draw);
print("and here...\n");
