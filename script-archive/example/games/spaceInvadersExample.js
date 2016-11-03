//
//  spaceInvadersExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates a simple space invaders style of game
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var iteration = 0;

var gameOver = false;

// horizontal movement of invaders
var invaderStepsPerCycle = 120; // the number of update steps it takes then invaders to move one column to the right
var invaderStepOfCycle = 0; // current iteration in the cycle
var invaderMoveDirection = 1; // 1 for moving to right, -1 for moving to left
var LEFT = ",";
var RIGHT = ".";
var FIRE = "SPACE";
var QUIT = "q";

print("Use:");
print(LEFT + " to move left");
print(RIGHT + " to move right");
print(FIRE + " to fire");
print(QUIT + " to quit");

// game length...
var itemLifetimes = 60; // 1 minute


// position the game to be basically near the avatar running the game...
var gameSize = { x: 10, y: 20, z: 1 };
var positionFromAvatarZ = 10;

var avatarX = MyAvatar.position.x;
var avatarY = MyAvatar.position.y;
var avatarZ = MyAvatar.position.z;
var gameAtX = avatarX;
var gameAtY = avatarY;
var gameAtZ = avatarZ;

// move the game to be "centered" on our X
if (gameAtX > (gameSize.x/2)) {
    gameAtX -= (gameSize.x/2);
}

// move the game to be "offset slightly" on our Y
if (gameAtY > (gameSize.y/4)) {
    gameAtY -= (gameSize.y/4);
}


// move the game to be positioned away on our Z
if (gameAtZ > positionFromAvatarZ) {
    gameAtZ -= positionFromAvatarZ;
}

var gameAt = { x: gameAtX, y: gameAtY, z: gameAtZ };
var middleX = gameAt.x + (gameSize.x/2);
var middleY = gameAt.y + (gameSize.y/2);

var invaderSize = 0.4;
var shipSize = 0.25;
var missileSize = 1.0;
var myShip;
var myShipProperties;

// create the rows of space invaders
var invaders = new Array();
var numberOfRows = 3 // FIXME 5;
var invadersPerRow = 3 // FIXME 8;
var emptyColumns = 2; // number of invader width columns not filled with invaders
var invadersBottomCorner = { x: gameAt.x, y: middleY , z: gameAt.z };
var rowHeight = ((gameAt.y + gameSize.y) - invadersBottomCorner.y) / numberOfRows;
var columnWidth = gameSize.x / (invadersPerRow + emptyColumns);

// vertical movement of invaders
var invaderRowOffset = 0;
var stepsPerRow = 20; // number of steps before invaders really move a whole row down.
var yPerStep = rowHeight/stepsPerRow;
var stepsToGround = (middleY - gameAt.y) / yPerStep;
var maxInvaderRowOffset=stepsToGround;

// missile related items
var myMissile;

// sounds
var hitSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/hit.raw");
var shootSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/shoot.raw");
var moveSounds = new Array();
moveSounds[0] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/Lo1.raw");
moveSounds[1] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/Lo2.raw");
moveSounds[2] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/Lo3.raw");
moveSounds[3] = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/Lo4.raw");
var currentMoveSound = 0;
var numberOfSounds = 4;
var stepsPerSound = invaderStepsPerCycle / numberOfSounds;

// if you set this to false, sounds will come from the location of entities instead of the player's head
var soundInMyHead = true; 

// models...
var invaderModels = new Array();
invaderModels[0] = {
        modelURL: HIFI_PUBLIC_BUCKET + "meshes/space_invader.fbx",
        modelScale: 450,
        modelTranslation: { x: -1.3, y: -1.3, z: -1.3 },
    };
invaderModels[1] = {
        modelURL: HIFI_PUBLIC_BUCKET + "meshes/space_invader.fbx",
        modelScale: 450,
        modelTranslation: { x: -1.3, y: -1.3, z: -1.3 },
    };
invaderModels[2] = {
        modelURL: HIFI_PUBLIC_BUCKET + "meshes/space_invader.fbx",
        modelScale: 450,
        modelTranslation: { x: -1.3, y: -1.3, z: -1.3 },
    };
invaderModels[3] = {
        modelURL: HIFI_PUBLIC_BUCKET + "meshes/space_invader.fbx",
        modelScale: 450,
        modelTranslation: { x: -1.3, y: -1.3, z: -1.3 },
    };
invaderModels[4] = {
        modelURL: HIFI_PUBLIC_BUCKET + "meshes/space_invader.fbx",
        modelScale: 450,
        modelTranslation: { x: -1.3, y: -1.3, z: -1.3 },
    };
    
    

//modelURL: HIFI_PUBLIC_BUCKET + "meshes/Feisar_Ship.FBX",
//modelURL: HIFI_PUBLIC_BUCKET + "meshes/invader.svo",
// HIFI_PUBLIC_BUCKET + "meshes/spaceInvader3.fbx"

function initializeMyShip() {
    myShipProperties = {
            type: "Model",
            position: { x: middleX , y: gameAt.y, z: gameAt.z },
            velocity: { x: 0, y: 0, z: 0 },
            gravity: { x: 0, y: 0, z: 0 },
            damping: 0,
            dimensions: { x: shipSize * 2, y: shipSize * 2, z: shipSize * 2 },
            color: { red: 0, green: 255, blue: 0 },
            modelURL: HIFI_PUBLIC_BUCKET + "meshes/space_invader.fbx",
            lifetime: itemLifetimes
        };
    myShip = Entities.addEntity(myShipProperties);
}

// calculate the correct invaderPosition for an column row
function getInvaderPosition(row, column) {
    var xBasePart = invadersBottomCorner.x + (column * columnWidth);
    var xMovePart = 0;
    if (invaderMoveDirection > 0) {
        xMovePart = (invaderMoveDirection * columnWidth * (invaderStepOfCycle/invaderStepsPerCycle));
    } else {
        xMovePart = columnWidth + (invaderMoveDirection * columnWidth * (invaderStepOfCycle/invaderStepsPerCycle));
    }

    var y = invadersBottomCorner.y + (row * rowHeight) - (invaderRowOffset * rowHeight/stepsPerRow);

    var invaderPosition = { 
                    x:  xBasePart + xMovePart, 
                    y: y,
                    z: invadersBottomCorner.z };
    
    return invaderPosition;
}

function initializeInvaders() {
    for (var row = 0; row < numberOfRows; row++) {
        invaders[row] = new Array();
        for (var column = 0; column < invadersPerRow; column++) {
            invaderPosition = getInvaderPosition(row, column);
            invaders[row][column] = Entities.addEntity({
                        type: "Model",
                        shapeType: "box",
                        position: invaderPosition,
                        velocity: { x: 0, y: 0, z: 0 },
                        gravity: { x: 0, y: 0, z: 0 },
                        damping: 0,
                        dimensions: { x: invaderSize * 2, y: invaderSize * 2, z: invaderSize * 2 },
                        color: { red: 255, green: 0, blue: 0 },
                        modelURL: invaderModels[row].modelURL,
                        dynamic: true,
                        lifetime: itemLifetimes
                    });
        }
    }
}

function moveInvaders() {
    for (var row = 0; row < numberOfRows; row++) {
        for (var column = 0; column < invadersPerRow; column++) {
            props = Entities.getEntityProperties(invaders[row][column]);
            if (props.id) {
                invaderPosition = getInvaderPosition(row, column);
                Entities.editEntity(invaders[row][column], 
                        { 
                            position: invaderPosition,
                            velocity: { x: 0, y: 0, z: 0 } // always reset this, incase they got collided with
                        });
            }
        }
    }
}

function displayGameOver() {
    gameOver = true;
    print("Game over...");
}

function update(deltaTime) {
    if (!gameOver) {
        //print("updating space invaders... iteration="+iteration);
        iteration++;

        if (invaderStepOfCycle % stepsPerSound == 0) {
            // play the move sound
            var options = {};
            
            if (soundInMyHead) {
                options.position = { x: MyAvatar.position.x + 0.0, 
                                     y: MyAvatar.position.y + 0.1, 
                                     z: MyAvatar.position.z + 0.0 };
            } else {
                options.position = getInvaderPosition(invadersPerRow / 2, numberOfRows / 2);
            }
            
            Audio.playSound(moveSounds[currentMoveSound], options);
            
            // get ready for next move sound
            currentMoveSound = (currentMoveSound+1) % numberOfSounds;
        }

        invaderStepOfCycle++;
        
        
        if (invaderStepOfCycle > invaderStepsPerCycle) {
            // handle left/right movement
            invaderStepOfCycle = 0;
            if (invaderMoveDirection > 0) {
                invaderMoveDirection = -1;
            } else {
                invaderMoveDirection = 1;
            }

            // handle downward movement
            invaderRowOffset++; // move down one row
            //print("invaderRowOffset="+invaderRowOffset);
        
            // check to see if invaders have reached "ground"...
            if (invaderRowOffset > maxInvaderRowOffset) {
                displayGameOver();
                return;
            }
        
        }
        moveInvaders();
    }
}

// register the call back so it fires before each data send
Script.update.connect(update);

function cleanupGame() {
    print("cleaning up game...");
    Entities.deleteEntity(myShip);
    print("cleanupGame() ... Entities.deleteEntity(myShip)... myShip="+myShip);
    for (var row = 0; row < numberOfRows; row++) {
        for (var column = 0; column < invadersPerRow; column++) {
            Entities.deleteEntity(invaders[row][column]);
            print("cleanupGame() ... Entities.deleteEntity(invaders[row][column])... invaders[row][column]="
                    +invaders[row][column]);
        }
    }
    
    // clean up our missile
    if (myMissile) {
        Entities.deleteEntity(myMissile);
    }
    
    Controller.releaseKeyEvents({text: " "});
    
    Script.stop();
}
Script.scriptEnding.connect(cleanupGame);

function endGame() {
    print("ending game...");
    Script.stop();
}

function moveShipTo(position) {
    Entities.editEntity(myShip, { position: position });
}

function entityCollisionWithEntity(entityA, entityB, collision) {
    print("entityCollisionWithEntity() a="+entityA + " b=" + entityB);
    Vec3.print('entityCollisionWithEntity() penetration=', collision.penetration);
    Vec3.print('entityCollisionWithEntity() contactPoint=', collision.contactPoint);

    deleteIfInvader(entityB);
}

function fireMissile() {
    // we only allow one missile at a time...
    var canFire = false;

    // If we've fired a missile, then check to see if it's still alive
    if (myMissile) {
        var missileProperties = Entities.getEntityProperties(myMissile);
    
        if (!missileProperties || (missileProperties.type === 'Unknown')) {
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
                            
        myMissile = Entities.addEntity(
                        { 
                            type: "Sphere",
                            position: missilePosition,
                            velocity: { x: 0, y: 5, z: 0},
                            gravity: { x: 0, y: 0, z: 0 },
                            damping: 0,
                            dynamic: true,
                            dimensions: { x: missileSize, y: missileSize, z: missileSize },
                            color: { red: 0, green: 0, blue: 255 },
                            lifetime: 5
                        });
    Script.addEventHandler(myMissile, "collisionWithEntity", entityCollisionWithEntity);
        var options = {}
        if (soundInMyHead) {
            options.position = { x: MyAvatar.position.x + 0.0, 
                                 y: MyAvatar.position.y + 0.1, 
                                 z: MyAvatar.position.z + 0.0 };
        } else {
            options.position = missilePosition;
        }

        Audio.playSound(shootSound, options);

    }
}

function keyPressEvent(key) {
    //print("keyPressEvent key.text="+key.text);

    if (key.text == LEFT) {
        myShipProperties.position.x -= 0.1;
        if (myShipProperties.position.x < gameAt.x) {
            myShipProperties.position.x = gameAt.x;
        }
        moveShipTo(myShipProperties.position);
    } else if (key.text == RIGHT) {
        myShipProperties.position.x += 0.1;
        if (myShipProperties.position.x > gameAt.x + gameSize.x) {
            myShipProperties.position.x = gameAt.x + gameSize.x;
        }
        moveShipTo(myShipProperties.position);
    } else if (key.text == FIRE) {
        fireMissile();
    } else if (key.text == QUIT) {
        endGame();
    }
}

// remap the keys...
Controller.keyPressEvent.connect(keyPressEvent);
Controller.captureKeyEvents({text: " "});

function deleteIfInvader(possibleInvaderEntity) {
    for (var row = 0; row < numberOfRows; row++) {
        for (var column = 0; column < invadersPerRow; column++) {
            if (invaders[row][column] == possibleInvaderEntity) {
                Entities.deleteEntity(possibleInvaderEntity);
                Entities.deleteEntity(myMissile);

                // play the hit sound
                var options = {};
                if (soundInMyHead) {
                    options.position = { x: MyAvatar.position.x + 0.0, 
                                         y: MyAvatar.position.y + 0.1, 
                                         z: MyAvatar.position.z + 0.0 };
                } else {
                    options.position = getInvaderPosition(row, column);
                }
                
                Audio.playSound(hitSound, options);
            }
        }
    }
}


// initialize the game...
initializeMyShip();
initializeInvaders();

// shut down the game after 1 minute
var gameTimer = Script.setTimeout(endGame, itemLifetimes * 1000);
