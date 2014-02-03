//
//  spaceInvadersExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/30/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates a simple space invaders style of game
//

var iteration = 0;

var itemLifetimes = 60;
var gameAt = { x: 10, y: 0, z: 10 };
var gameSize = { x: 10, y: 20, z: 1 };
var middleX = gameAt.x + (gameSize.x/2);
var middleY = gameAt.y + (gameSize.y/2);

var shipSize = 0.2;
var missileSize = 0.1;
var myShip;
var myShipProperties;

// create the rows of space invaders
var invaders = new Array();
var numberOfRows = 5;
var invadersPerRow = 8;
var emptyColumns = 2; // number of invader width columns not filled with invaders
var invadersBottomCorner = { x: gameAt.x, y: middleY , z: gameAt.z };
var rowHeight = ((gameAt.y + gameSize.y) - invadersBottomCorner.y) / numberOfRows;
var columnWidth = gameSize.x / (invadersPerRow + emptyColumns);

var missileFired = false;
var myMissile;

function initializeMyShip() {
    myShipProperties = {
                                position: { x: middleX , y: gameAt.y, z: gameAt.z },
                                velocity: { x: 0, y: 0, z: 0 },
                                gravity: { x: 0, y: 0, z: 0 },
                                damping: 0,
                                radius: shipSize,
                                color: { red: 0, green: 255, blue: 0 },
                                lifetime: itemLifetimes
                            };
    myShip = Particles.addParticle(myShipProperties);
}

function initializeInvaders() {
    for (var row = 0; row < numberOfRows; row++) {
        invaders[row] = new Array();
        for (var column = 0; column < invadersPerRow; column++) {
            invaderPosition = { 
                    x: invadersBottomCorner.x + (column * columnWidth),
                    y: invadersBottomCorner.y + (row * rowHeight),
                    z: invadersBottomCorner.z };

        
            invaders[row][column] = Particles.addParticle({
                        position: invaderPosition,
                        velocity: { x: 0, y: 0, z: 0 },
                        gravity: { x: 0, y: 0, z: 0 },
                        damping: 0,
                        radius: shipSize,
                        color: { red: 255, green: 0, blue: 0 },
                        lifetime: itemLifetimes
                    });
                
            print("invaders[row][column].creatorTokenID=" + invaders[row][column].creatorTokenID);
        }
    }
}

var invadersMovingRight = true;
function moveInvaders() {
    print("moveInvaders()...");
    if (invadersMovingRight) {
        invaderMoveX = 0.05;
    } else {
        invaderMoveX = -0.05;
    }
    for (var row = 0; row < numberOfRows; row++) {
        for (var column = 0; column < invadersPerRow; column++) {
            props = Particles.getParticleProperties(invaders[row][column]);
            if (props.isKnownID) {
                newPosition = { x: props.position.x + invaderMoveX, 
                                y: props.position.y,
                                z: props.position.z };
                Particles.editParticle(invaders[row][column], { position: newPosition });
            }
        }
    }
}

function update() {
    print("updating space invaders... iteration="+iteration);
    iteration++;
    if ((iteration % 60) == 0) {
        invadersMovingRight = !invadersMovingRight;
    }
    moveInvaders();
}

// register the call back so it fires before each data send
Script.willSendVisualDataCallback.connect(update);

function endGame() {
    print("ending game...");
    Particles.deleteParticle(myShip);
    print("endGame() ... Particles.deleteParticle(myShip)... myShip.id="+myShip.id);
    for (var row = 0; row < numberOfRows; row++) {
        for (var column = 0; column < invadersPerRow; column++) {
            Particles.deleteParticle(invaders[row][column]);
            print("endGame() ... Particles.deleteParticle(invaders[row][column])... invaders[row][column].id="+invaders[row][column].id);
        }
    }
    
    // clean up our missile
    if (missileFired) {
        Particles.deleteParticle(myMissile);
    }
    Script.stop();
}

function moveShipTo(position) {
    myShip = Particles.identifyParticle(myShip);
    Particles.editParticle(myShip, { position: position });
}

function fireMissile() {
    // we only allow one missile at a time...
    var canFire = false;

    // If we've fired a missile, then check to see if it's still alive
    if (missileFired) {
        var missileProperties = Particles.getParticleProperties(myMissile);
        print("missileProperties.isKnownID=" + missileProperties.isKnownID);
    
        if (!missileProperties.isKnownID) {
            print("canFire = true");
            canFire = true;
        }    
    } else {
        canFire = true;
    }
    
    if (canFire) {
        print("firing missile");
        var missilePosition = { x: myShipProperties.position.x, 
                                y: myShipProperties.position.y + (2* shipSize), 
                                z: myShipProperties.position.z };
                            
        myMissile = Particles.addParticle(
                        { 
                            position: missilePosition,
                            velocity: { x: 0, y: 5, z: 0},
                            gravity: { x: 0, y: 0, z: 0 },
                            damping: 0,
                            radius: missileSize,
                            color: { red: 0, green: 0, blue: 255 },
                            lifetime: 5
                        });

        missileFired = true;
    }
}

function keyPressEvent(key) {
    //print("keyPressEvent key.text="+key.text);
    if (key.text == ",") {
        myShipProperties.position.x -= 0.1;
        if (myShipProperties.position.x < gameAt.x) {
            myShipProperties.position.x = gameAt.x;
        }
        moveShipTo(myShipProperties.position);
    } else if (key.text == ".") {
        myShipProperties.position.x += 0.1;
        if (myShipProperties.position.x > gameAt.x + gameSize.x) {
            myShipProperties.position.x = gameAt.x + gameSize.x;
        }
        moveShipTo(myShipProperties.position);
    } else if (key.text == " ") {
        fireMissile();
    } else if (key.text == "q") {
        endGame();
    }
}

// remap the keys...
Controller.keyPressEvent.connect(keyPressEvent);
Controller.captureKeyEvents({text: " "});

function deleteIfInvader(possibleInvaderParticle) {
    for (var row = 0; row < numberOfRows; row++) {
        for (var column = 0; column < invadersPerRow; column++) {
            invaders[row][column] = Particles.identifyParticle(invaders[row][column]);
            if (invaders[row][column].isKnownID) {
                if (invaders[row][column].id == possibleInvaderParticle.id) {
                    Particles.deleteParticle(possibleInvaderParticle);
                    Particles.deleteParticle(myMissile);
                }
            }
        }
    }
}

function particleCollisionWithParticle(particleA, particleB) {
    print("particleCollisionWithParticle() a.id="+particleA.id + " b.id=" + particleB.id);
    if (missileFired) {
        myMissile = Particles.identifyParticle(myMissile);
        if (myMissile.id == particleA.id) {
            deleteIfInvader(particleB);
        } else if (myMissile.id == particleB.id) {
            deleteIfInvader(particleA);
        }
    }
}
Particles.particleCollisionWithParticle.connect(particleCollisionWithParticle);


// initialize the game...
initializeMyShip();
initializeInvaders();


