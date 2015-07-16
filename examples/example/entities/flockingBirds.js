//
//  flockingBirds.js
//  examples
//
//  Created by Brad Hefta-Gaub on 3/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that generates entities that act like flocking birds
//
// All birds, even flying solo...
//    birds don't like to fall too fast
//       if they fall to fast, they will go into a state of thrusting up, until they reach some max upward velocity, then
//       go back to gliding
//    birds don't like to be below a certain altitude
//       if they are below that altitude they will keep thrusting up, until they get ove
//
// flocking
//    try to align your velocity with velocity of other birds
//    try to fly toward center of flock
//    but dont get too close
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var birdsInFlock = 20;

var birdLifetime = 300; // 1 minute
var count=0; // iterations

var enableFlyTowardPoints = true; // some birds have a point they want to fly to
var enabledClustedFlyTowardPoints = true; // the flyToward points will be generally near each other
var flyToFrames = 100; // number of frames the bird would like to attempt to fly to it's flyTo point
var PROBABILITY_OF_FLY_TOWARD_CHANGE = 0.01; // chance the bird will decide to change its flyTo point
var PROBABILITY_EACH_BIRD_WILL_FLY_TOWARD = 0.2; // chance the bird will decide to flyTo, otherwise it follows
var flyingToCount = 0; // count of birds currently flying to someplace
var flyToCluster = { }; // the point that the cluster of flyTo points is based on when in enabledClustedFlyTowardPoints

// Bird behaviors
var enableAvoidDropping = true; // birds will resist falling too fast, and will thrust up if falling
var enableAvoidMinHeight = true; // birds will resist being below a certain height and thrust up to get above it
var enableAvoidMaxHeight = true; // birds will resist being above a certain height and will glide to get below it
var enableMatchFlockVelocity = true; // birds will thrust to match the flocks average velocity
var enableThrustTowardCenter = true; // birds will thrust to try to move toward the center of the flock
var enableAvoidOtherBirds = true; // birds will thrust away from all other birds
var startWithVelocity = true;
var flockGravity = { x: 0, y: -1, z: 0};

// NOTE: these two features don't seem to be very interesting, they cause odd behaviors
var enableRandomXZThrust = false; // leading birds randomly decide to thrust in some random direction.
var enableSomeBirdsLead = false; // birds randomly decide not fly toward flock, causing other birds to follow
var leaders = 0; // number of birds leading
var PROBABILITY_TO_LEAD = 0.1; // probability a bird will choose to lead

var birds = new Array(); // array of bird state data


var flockStartPosition = MyAvatar.position;
var flockStartVelocity = { x: 0, y: 0, z: 0};
var flockStartThrust =  { x: 0, y: 0, z: 0}; // slightly upward against gravity
var INITIAL_XY_VELOCITY_SCALE = 2;
var birdRadius = 0.2;
var baseBirdColor = { red: 0, green: 255, blue: 255 };
var glidingColor = { red: 255, green: 0, blue: 0 };
var thrustUpwardColor = { red: 0, green: 255, blue: 0 };
var thrustXYColor = { red: 128, green: 0, blue: 128 }; // will be added to any other color
var leadingOrflyToColor = { red: 200, green: 200, blue: 255 };

var tooClose = birdRadius * 3; // how close birds are willing to be to each other
var droppingTooFast = -1; // birds don't like to fall too fast
var risingTooFast = 1; // birds don't like to climb too fast
var upwardThrustAgainstGravity = -10; // how hard a bird will attempt to thrust up to avoid downward motion
var droppingAdjustFrames = 5; // birds try to correct their min height in only a couple frames
var minHeight = 10; // meters off the ground
var maxHeight = 50; // meters off the ground
var adjustFrames = 10; // typical number of frames a bird will assume for it's adjustments
var PROBABILITY_OF_RANDOM_XZ_THRUST = 0.25;
var RANDOM_XZ_THRUST_SCALE = 5; // random -SCALE...to...+SCALE in X or Z direction
var MAX_XY_VELOCITY = 10;

// These are multiplied by every frame since a change. so after 50 frames, there's a 50% probability of change
var PROBABILITY_OF_STARTING_XZ_THRUST = 0.0025;
var PROBABILITY_OF_STOPPING_XZ_THRUST = 0.01;

var FLY_TOWARD_XZ_DISTANCE = 100;
var FLY_TOWARD_Y_DISTANCE = 0;
var FLY_TOWARD_XZ_CLUSTER_DELTA = 5;
var FLY_TOWARD_Y_CLUSTER_DELTA = 0;

function createBirds() {
    if (enabledClustedFlyTowardPoints) {
        flyToCluster = { x: flockStartPosition.x + Math.random() * FLY_TOWARD_XZ_DISTANCE, 
                         y: flockStartPosition.y + Math.random() * FLY_TOWARD_Y_DISTANCE, 
                         z: flockStartPosition.z + Math.random() * FLY_TOWARD_XZ_DISTANCE};
    }

    for(var i =0; i < birdsInFlock; i++) {
        var velocity;

        position = Vec3.sum(flockStartPosition, { x: Math.random(), y: Math.random(), z: Math.random() }); // add random 
        
        var flyingToward = position;
        var isFlyingToward = false;
        if (enableFlyTowardPoints) {
            if (Math.random() < PROBABILITY_EACH_BIRD_WILL_FLY_TOWARD) {
                flyingToCount++;
                isFlyingToward = true;
                if (enabledClustedFlyTowardPoints) {
                    flyingToward = { x: flyToCluster.x + Math.random() * FLY_TOWARD_XZ_CLUSTER_DELTA, 
                                     y: flyToCluster.y + Math.random() * FLY_TOWARD_Y_CLUSTER_DELTA, 
                                     z: flyToCluster.z + Math.random() * FLY_TOWARD_XZ_CLUSTER_DELTA};
                } else {
                    flyingToward = { x: flockStartPosition.x + Math.random() * FLY_TOWARD_XZ_DISTANCE, 
                                     y: flockStartPosition.y + Math.random() * FLY_TOWARD_Y_DISTANCE, 
                                     z: flockStartPosition.z + Math.random() * FLY_TOWARD_XZ_DISTANCE};
                }
            }
                             
            Vec3.print("birds["+i+"].flyingToward=",flyingToward);
        }
        
        birds[i] =  {
                        particle: {},
                        properties: {},
                        thrust: Vec3.sum(flockStartThrust, { x:0, y: 0, z: 0 }),
                        gliding: true,
                        xzThrust: { x:0, y: 0, z: 0},
                        xzthrustCount: 0,
                        isLeading: false,
                        flyingToward: flyingToward,
                        isFlyingToward: isFlyingToward,
                    };

        if (enableSomeBirdsLead) {
            if (Math.random() < PROBABILITY_TO_LEAD) {
                birds[i].isLeading = true;
            }
            if (leaders == 0 && i == (birdsInFlock-1)) {
                birds[i].isLeading = true;
            }
            if (birds[i].isLeading) {
                leaders++;
                velocity = { x: 2, y: 0, z: 2};
                print(">>>>>>THIS BIRD LEADS!!!! i="+i);
            }
        }

        if (startWithVelocity) {
            velocity = Vec3.sum(flockStartVelocity, { x: (Math.random() * INITIAL_XY_VELOCITY_SCALE), 
                                                      y: 0, 
                                                      z: (Math.random() * INITIAL_XY_VELOCITY_SCALE) }); // add random 
        } else {
            velocity = { x: 0, y: 0, z: 0};
        }
        birds[i].particle = Entities.addEntity({
                                type: "Sphere",
                                position: position,
                                velocity: velocity,
                                gravity: flockGravity,
                                damping: 0,
                                dimensions: { x: birdRadius, y: birdRadius, z: birdRadius},
                                color: baseBirdColor,
                                lifetime: birdLifetime
                            });

    }
    print("flyingToCount=" + flyingToCount);
}

var wantDebug = false;
function updateBirds(deltaTime) {
    count++;

    // get all our bird properties, and calculate the current flock velocity
    var averageVelocity = { x: 0, y: 0, z: 0};
    var averagePosition = { x: 0, y: 0, z: 0};
    var knownBirds = 0;
    for(var i =0; i < birdsInFlock; i++) {
        birds[i].properties = Entities.getEntityProperties(birds[i].particle);
        if (birds[i].properties) {
            knownBirds++;
            averageVelocity = Vec3.sum(averageVelocity, birds[i].properties.velocity);
            averagePosition = Vec3.sum(averagePosition, birds[i].properties.position);
        }
    }
    
    if (knownBirds == 0 && count > 100) {
        Script.stop();
        return;
    }
    averageVelocity = Vec3.multiply(averageVelocity, (1 / Math.max(1, knownBirds)));
    averagePosition = Vec3.multiply(averagePosition, (1 / Math.max(1, knownBirds)));
    
    if (wantDebug) {
        Vec3.print("averagePosition=",averagePosition);
        Vec3.print("averageVelocity=",averageVelocity);
        print("knownBirds="+knownBirds);
    }

    var flyToClusterChanged = false;
    if (enabledClustedFlyTowardPoints) {
        if (Math.random() < PROBABILITY_OF_FLY_TOWARD_CHANGE) {
            flyToClusterChanged = true;
            flyToCluster = { x: averagePosition.x + (Math.random() * FLY_TOWARD_XZ_DISTANCE) - FLY_TOWARD_XZ_DISTANCE/2, 
                             y: averagePosition.y + (Math.random() * FLY_TOWARD_Y_DISTANCE) - FLY_TOWARD_Y_DISTANCE/2, 
                             z: averagePosition.z + (Math.random() * FLY_TOWARD_XZ_DISTANCE) - FLY_TOWARD_XZ_DISTANCE/2};
        }
    }
    
    // iterate all birds again, adjust their thrust for various goals
    for(var i =0; i < birdsInFlock; i++) {
    
        birds[i].thrust = { x: 0, y: 0, z: 0 }; // assume no thrust...

        if (birds[i].particle) {
        
            if (enableFlyTowardPoints) {
                // if we're flying toward clusters, and the cluster changed, and this bird is flyingToward
                // then we need to update it's flyingToward
                if (enabledClustedFlyTowardPoints && flyToClusterChanged && birds[i].isFlyingToward) {
                    flyingToward = { x: flyToCluster.x + (Math.random() * FLY_TOWARD_XZ_CLUSTER_DELTA) - FLY_TOWARD_XZ_CLUSTER_DELTA/2, 
                                     y: flyToCluster.y + (Math.random() * FLY_TOWARD_Y_CLUSTER_DELTA) - FLY_TOWARD_Y_CLUSTER_DELTA/2, 
                                     z: flyToCluster.z + (Math.random() * FLY_TOWARD_XZ_CLUSTER_DELTA) - FLY_TOWARD_XZ_CLUSTER_DELTA/2};
                    birds[i].flyingToward = flyingToward;
                }
                
                // there a random chance this bird will decide to change it's flying toward state
                if (Math.random() < PROBABILITY_OF_FLY_TOWARD_CHANGE) {
                    var wasFlyingTo = birds[i].isFlyingToward;
                    
                    // there's some chance it will decide it should be flying toward
                    if (Math.random() < PROBABILITY_EACH_BIRD_WILL_FLY_TOWARD) {
                    
                        // if we're flying toward clustered points, then we randomize from the cluster, otherwise we pick
                        // completely random places based on flocks current averagePosition
                        if (enabledClustedFlyTowardPoints) {
                            flyingToward = { x: flyToCluster.x + (Math.random() * FLY_TOWARD_XZ_CLUSTER_DELTA) - FLY_TOWARD_XZ_CLUSTER_DELTA/2, 
                                             y: flyToCluster.y + (Math.random() * FLY_TOWARD_Y_CLUSTER_DELTA) - FLY_TOWARD_Y_CLUSTER_DELTA/2, 
                                             z: flyToCluster.z + (Math.random() * FLY_TOWARD_XZ_CLUSTER_DELTA) - FLY_TOWARD_XZ_CLUSTER_DELTA/2};
                        } else {
                            flyingToward = { x: averagePosition.x + (Math.random() * FLY_TOWARD_XZ_DISTANCE) - FLY_TOWARD_XZ_DISTANCE/2, 
                                             y: averagePosition.y + (Math.random() * FLY_TOWARD_Y_DISTANCE) - FLY_TOWARD_Y_DISTANCE/2, 
                                             z: averagePosition.z + (Math.random() * FLY_TOWARD_XZ_DISTANCE) - FLY_TOWARD_XZ_DISTANCE/2};
                        }
                        birds[i].flyingToward = flyingToward;
                        birds[i].isFlyingToward = true;
                    } else {
                        birds[i].flyingToward = {};
                        birds[i].isFlyingToward = false;
                    }
                    
                    // keep track of our bookkeeping
                    if (!wasFlyingTo && birds[i].isFlyingToward) {
                        flyingToCount++;
                    }
                    if (wasFlyingTo && !birds[i].isFlyingToward) {
                        flyingToCount--;
                    }
                    print(">>>> CHANGING flyingToCount="+flyingToCount);
                    if (birds[i].isFlyingToward) {
                        Vec3.print("... now birds["+i+"].flyingToward=", birds[i].flyingToward);
                    }
                }

                // actually apply the thrust after all that
                if (birds[i].isFlyingToward) {
                    var flyTowardDelta = Vec3.subtract(birds[i].flyingToward, birds[i].properties.position);
                    var thrustTowardFlyTo = Vec3.multiply(flyTowardDelta, 1/flyToFrames);
                    birds[i].thrust = Vec3.sum(birds[i].thrust, thrustTowardFlyTo);
                }
            }
        
        
            // adjust thrust to avoid dropping to fast
            if (enableAvoidDropping) {
                if (birds[i].gliding) {
                    if (birds[i].properties.velocity.y < droppingTooFast) {
                        birds[i].gliding = false; // leave thrusting against gravity till it gets too high
                        //print("birdGliding["+i+"]=false <<<< try to conteract gravity <<<<<<<<<<<<<<<<<<<<");
                    }
                }
            }
            
            // if the bird is currently not gliding, check to see if it's rising too fast
            if (!birds[i].gliding && birds[i].properties.velocity.y > risingTooFast) {
                //Vec3.print("bird rising too fast will glide bird["+i+"]=",birds[i].properties.velocity.y);
                birds[i].gliding = true;
            }
        
            // adjust thrust to avoid minHeight, we don't care about rising too fast in this case, so we do it 
            // after the rising too fast check
            if (enableAvoidMinHeight) {
                if (birds[i].properties.position.y < minHeight) {
                    //Vec3.print("**** enableAvoidMinHeight... enable thrust against gravity... bird["+i+"].position=",birds[i].properties.position);
                    birds[i].gliding = false;
                }
            }

            // adjust thrust to avoid maxHeight
            if (enableAvoidMaxHeight) {
                if (birds[i].properties.position.y > maxHeight) {
                    //Vec3.print("********************* bird above max height will glide bird["+i+"].position=",birds[i].properties.position);
                    birds[i].gliding = true;
                }
            }

            // if the bird is currently not gliding, then it is applying a thrust upward against gravity
            if (!birds[i].gliding) {
                // as long as we're not rising too fast, keep thrusting...
                if (birds[i].properties.velocity.y < risingTooFast) {
                    var thrustAdjust = {x: 0, y: (flockGravity.y * upwardThrustAgainstGravity), z: 0};
                    //Vec3.print("bird fighting gravity thrustAdjust for bird["+i+"]=",thrustAdjust);
                    birds[i].thrust = Vec3.sum(birds[i].thrust, thrustAdjust);
                } else {
                    //print("%%% non-gliding bird, thrusting too much...");
                }
            }
            
            if (enableRandomXZThrust && birds[i].isLeading) {
                birds[i].xzThrustCount++;

                // we will randomly decide to enable XY thrust, in which case we will set the thrust and leave it
                // that way till we randomly shut it off.
                
                // if we don't have a thrust, check against probability of starting it, and create a random thrust if
                // probability occurs
                if (Vec3.length(birds[i].xzThrust) == 0) {
                    var probabilityToStart = (PROBABILITY_OF_STARTING_XZ_THRUST * birds[i].xzThrustCount);
                    //print("probabilityToStart=" + probabilityToStart);
                    if (Math.random() < probabilityToStart) {
                        var xThrust = (Math.random() * (RANDOM_XZ_THRUST_SCALE * 2)) - RANDOM_XZ_THRUST_SCALE;
                        var zThrust = (Math.random() * (RANDOM_XZ_THRUST_SCALE * 2)) - RANDOM_XZ_THRUST_SCALE;
                    
                        birds[i].xzThrust = { x: zThrust, y: 0, z: zThrust };
                        birds[i].xzThrustCount = 0;
                        //Vec3.print(">>>>>>>>>> STARTING XY THRUST birdXYthrust["+i+"]=", birds[i].xzThrust);
                    }
                }
                
                // if we're thrusting... then check for probability of stopping
                if (Vec3.length(birds[i].xzThrust)) {
                    var probabilityToStop = (PROBABILITY_OF_STOPPING_XZ_THRUST * birds[i].xzThrustCount);
                    //print("probabilityToStop=" + probabilityToStop);
                    if (Math.random() < probabilityToStop) {
                        birds[i].xzThrust = { x: 0, y: 0, z: 0};
                        //Vec3.print(">>>>>>>>>> STOPPING XY THRUST birdXYthrust["+i+"]=", birds[i].xzThrust);
                        birds[i].xzThrustCount = 0;
                    }
                    
                    if (birds[i].properties.velocity.x > MAX_XY_VELOCITY) {
                        birds[i].xzThrust.x = 0;
                        //Vec3.print(">>>>>>>>>> CLEARING X THRUST birdXYthrust["+i+"]=", birds[i].xzThrust);
                    }
                    if (birds[i].properties.velocity.z > MAX_XY_VELOCITY) {
                        birds[i].xzThrust.z = 0;
                        //Vec3.print(">>>>>>>>>> CLEARING Y THRUST birdXYthrust["+i+"]=", birds[i].xzThrust);
                    }

                    if (Vec3.length(birds[i].xzThrust)) {
                        birds[i].thrust = Vec3.sum(birds[i].thrust, birds[i].xzThrust);
                    }
                }
            }

            // adjust thrust to move their velocity toward average flock velocity
            if (enableMatchFlockVelocity) {
                if (birds[i].isLeading) {
                    print("this bird is leading... i="+i);
                } else {
                    var velocityDelta = Vec3.subtract(averageVelocity, birds[i].properties.velocity);
                    var thrustAdjust = velocityDelta;
                    birds[i].thrust = Vec3.sum(birds[i].thrust, thrustAdjust);
                }
            }

            // adjust thrust to move their velocity toward average flock position
            if (enableThrustTowardCenter) {
                if (birds[i].isLeading) {
                    print("this bird is leading... i="+i);
                } else {
                    var positionDelta = Vec3.subtract(averagePosition, birds[i].properties.position);
                    var thrustTowardCenter = Vec3.multiply(positionDelta, 1/adjustFrames);
                    birds[i].thrust = Vec3.sum(birds[i].thrust, thrustTowardCenter);
                }
            }
            

            // adjust thrust to avoid other birds
            if (enableAvoidOtherBirds) {
                var sumThrustThisBird = { x: 0, y: 0, z: 0 };

                for(var j =0; j < birdsInFlock; j++) {

                    // if this is not me, and a known bird, then check our position
                    if (birds[i].properties && j != i) {
                        var positionMe = birds[i].properties.position;
                        var positionYou = birds[j].properties.position;
                        var awayFromYou = Vec3.subtract(positionMe, positionYou); // vector pointing away from "you"
                        var distance = Vec3.length(awayFromYou);
                        if (distance < tooClose) {
                            // NOTE: this was Philip's recommendation for "avoiding" other birds...
                            //    Vme -= Vec3.multiply(Vme, normalize(PositionMe - PositionYou))
                            //
                            // But it doesn't seem to work... Here's my JS implementation...
                            //
                            //  var velocityMe = birds[i].properties.velocity;
                            //  var thrustAdjust = Vec3.cross(velocityMe, Vec3.normalize(awayFromYou));
                            //  sumThrustThisBird = Vec3.sum(sumThrustThisBird, thrustAdjust);
                            //
                            // Instead I just apply a thrust equal to the vector away from all the birds
                            sumThrustThisBird = Vec3.sum(sumThrustThisBird, awayFromYou);
                        }
                    }
                }
                birds[i].thrust = Vec3.sum(birds[i].thrust, sumThrustThisBird);
            }
        }
    }
    
    
    // iterate all birds again, apply their thrust
    for(var i =0; i < birdsInFlock; i++) {
        if (birds[i].particle) {

            var color;
            if (birds[i].gliding) {
                color = glidingColor;
            } else {
                color = thrustUpwardColor;
            }
            if (Vec3.length(birds[i].xzThrust)) {
                color = Vec3.sum(color, thrustXYColor);
            }
        
            var velocityMe = birds[i].properties.velocity;
            // add thrust to velocity
            var newVelocity = Vec3.sum(velocityMe, Vec3.multiply(birds[i].thrust, deltaTime));
            
            if (birds[i].isLeading || birds[i].isFlyingToward) {
                Vec3.print("this bird is leading/flying toward... i="+i+" velocity=",newVelocity);
                color = leadingOrflyToColor;
            }
            
            if (wantDebug) {
                Vec3.print("birds["+i+"].position=", birds[i].properties.position);
                Vec3.print("birds["+i+"].oldVelocity=", velocityMe);
                Vec3.print("birdThrusts["+i+"]=", birds[i].thrust);
                Vec3.print("birds["+i+"].newVelocity=", newVelocity);
            }
            
            birds[i].particle = Entities.editEntity(birds[i].particle,{ velocity: newVelocity, color: color });
            
        }
    }    
}


createBirds();
// register the call back for simulation loop
Script.update.connect(updateBirds);

