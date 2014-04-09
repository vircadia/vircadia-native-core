//
//  gameoflife.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  The following is an example of Conway's Game of Life (http://en.wikipedia.org/wiki/Conway's_Game_of_Life)
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var NUMBER_OF_CELLS_EACH_DIMENSION = 64;
var NUMBER_OF_CELLS = NUMBER_OF_CELLS_EACH_DIMENSION * NUMBER_OF_CELLS_EACH_DIMENSION;

var currentCells = [];
var nextCells = [];

var METER_LENGTH = 1;
var cellScale = (NUMBER_OF_CELLS_EACH_DIMENSION * METER_LENGTH) / NUMBER_OF_CELLS_EACH_DIMENSION; 

// randomly populate the cell start values
for (var i = 0; i < NUMBER_OF_CELLS_EACH_DIMENSION; i++) {
  // create the array to hold this row
  currentCells[i] = [];
  
  // create the array to hold this row in the nextCells array
  nextCells[i] = [];
  
  for (var j = 0; j < NUMBER_OF_CELLS_EACH_DIMENSION; j++) {
     currentCells[i][j] = Math.floor(Math.random() * 2);
     
     // put the same value in the nextCells array for first board draw
     nextCells[i][j] = currentCells[i][j];
  }
}

function isNeighbourAlive(i, j) {
  if (i < 0 || i >= NUMBER_OF_CELLS_EACH_DIMENSION 
      || i < 0 || j >= NUMBER_OF_CELLS_EACH_DIMENSION) {
    return 0;
  } else {
    return currentCells[i][j];
  }
}

function updateCells() {
  var i = 0;
  var j = 0;
  
  for (i = 0; i < NUMBER_OF_CELLS_EACH_DIMENSION; i++) {
    for (j = 0; j < NUMBER_OF_CELLS_EACH_DIMENSION; j++) {
      // figure out the number of live neighbours for the i-j cell
      var liveNeighbours = 
        isNeighbourAlive(i + 1, j - 1) + isNeighbourAlive(i + 1, j) + isNeighbourAlive(i + 1, j + 1) +
        isNeighbourAlive(i, j - 1) + isNeighbourAlive(i, j + 1) +
        isNeighbourAlive(i - 1, j - 1) + isNeighbourAlive(i - 1, j) + isNeighbourAlive(i - 1, j + 1);
      
      if (currentCells[i][j]) {
        // live cell
        
        if (liveNeighbours < 2) {
          // rule #1 - under-population - this cell will die
          // mark it zero to mark the change
          nextCells[i][j] = 0;
        } else if (liveNeighbours < 4) {
          // rule #2 - this cell lives
          // mark it -1 to mark no change
          nextCells[i][j] = -1;
        } else {
          // rule #3 - overcrowding - this cell dies
          // mark it zero to mark the change
          nextCells[i][j] = 0;
        }
      } else {
        // dead cell
        if (liveNeighbours == 3) {
          // rule #4 - reproduction - this cell revives
          // mark it one to mark the change
          nextCells[i][j] = 1;
        } else {
          // this cell stays dead
          // mark it -1 for no change
          nextCells[i][j] = -1;
        }
      }    
      
      if (Math.random() < 0.001) {
        //  Random mutation to keep things interesting in there. 
        nextCells[i][j] = 1;
      }
    }
  }
  
  for (i = 0; i < NUMBER_OF_CELLS_EACH_DIMENSION; i++) {
    for (j = 0; j < NUMBER_OF_CELLS_EACH_DIMENSION; j++) {
      if (nextCells[i][j] != -1) {
        // there has been a change to this cell, change the value in the currentCells array
        currentCells[i][j] = nextCells[i][j];
      }
    }
  }
}

function sendNextCells() {
  for (var i = 0; i < NUMBER_OF_CELLS_EACH_DIMENSION; i++) {
    for (var j = 0; j < NUMBER_OF_CELLS_EACH_DIMENSION; j++) {
      if (nextCells[i][j] != -1) {
        // there has been a change to the state of this cell, send it
        
        // find the x and y position for this voxel, z = 0
        var x = j * cellScale;
        var y = i * cellScale;

        // queue a packet to add a voxel for the new cell
        var color = (nextCells[i][j] == 1) ? 255 : 1;
        Voxels.setVoxel(x, y, 0, cellScale, color, color, color);
      }
    }
  } 
}

var sentFirstBoard = false;

function step(deltaTime) {
  if (sentFirstBoard) {
    // we've already sent the first full board, perform a step in time
    updateCells();
  } else {
    // this will be our first board send
    sentFirstBoard = true;
  }
  
  sendNextCells();  
}

print("here");
Script.update.connect(step);
Voxels.setPacketsPerSecond(200);
print("now here");
