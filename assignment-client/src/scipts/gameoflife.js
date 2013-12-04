// Add your JavaScript for assignment below this line 

// The following is an example of Conway's Game of Life (http://en.wikipedia.org/wiki/Conway's_Game_of_Life)

var NUMBER_OF_CELLS_EACH_DIMENSION = 36;
var NUMBER_OF_CELLS = NUMBER_OF_CELLS_EACH_DIMENSION * NUMBER_OF_CELLS_EACH_DIMENSION;

var currentCells = [];
var nextCells = [];

var METER_LENGTH = 1 / TREE_SCALE;
var cellScale = 4 * METER_LENGTH; 

print("TREE_SCALE = " + TREE_SCALE + "\n");

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

function currentPopulation() {
  var i = 0;
  var j = 0;
  var population = 0;
  
  for (i = 0; i < NUMBER_OF_CELLS_EACH_DIMENSION; i++) {
    for (j = 0; j < NUMBER_OF_CELLS_EACH_DIMENSION; j++) {
        if (currentCells[i][j]) {
            population++;
        }
    }
  }
  return population;
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
    
          // here's another random mutation...  0.5% of all dead cells reanimate
          var REANIMATE_PROBABILITY = 0.005;
          if (Math.random() < REANIMATE_PROBABILITY) {
              // mark it one to mark the change
              nextCells[i][j] = 1;
          } else {
              // this cell stays dead
              // mark it -1 for no change
              nextCells[i][j] = -1;
          }
        }
      }    
      
      /*
      if (Math.random() < 0.001) {
        //  Random mutation to keep things interesting in there. 
        nextCells[i][j] = 1;
      }
      */
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
  var cellsSent = 0;
  for (var i = 0; i < NUMBER_OF_CELLS_EACH_DIMENSION; i++) {
    for (var j = 0; j < NUMBER_OF_CELLS_EACH_DIMENSION; j++) {
      if (nextCells[i][j] != -1) {
        // there has been a change to the state of this cell, send it
        
        // find the x and z position for this voxel, y = 0
        var x = towerPlatformCorner.x + j * cellScale;
        var y = towerPlatformCorner.y;
        var z = towerPlatformCorner.z + i * cellScale;

        // queue a packet to add a voxel for the new cell
        var color;
        if (nextCells[i][j] == 1) {
            color = {r: 255, g: 255, b: 255};
        } else {
            color = {r: 128, g: 128, b: 128};
        }
        Voxels.queueDestructiveVoxelAdd(x, 0, z, cellScale, color.r, color.g, color.b);
        cellsSent++;
      }
    }
  } 
  return cellsSent;
}

var sentFirstBoard = false;

var visualCallbacks = 0;
function step() {
  if (sentFirstBoard) {
    // we've already sent the first full board, perform a step in time
    updateCells();
  } else {
    // this will be our first board send
    sentFirstBoard = true;
  }
  
  var cellsSent = sendNextCells();  

    visualCallbacks++;
    print("Voxel Stats: " + Voxels.getLifetimeInSeconds() + " seconds," + 
        " visualCallbacks:" + visualCallbacks +
        " currentPopulation:" + currentPopulation() +
        " cellsSent:" + cellsSent +
        " Queued packets:" + Voxels.getLifetimePacketsQueued() + "," +
        " PPS:" + Voxels.getLifetimePPSQueued() + "," +
        " Bytes:" + Voxels.getLifetimeBytesQueued() + "," +
        //" Sent packets:" + Voxels.getLifetimePacketsSent() + "," +
        //" PPS:" + Voxels.getLifetimePPS() + "," +
        //" Bytes:" + Voxels.getLifetimeBytesSent() + 
        "\n");

}

Voxels.setPacketsPerSecond(500);
Agent.willSendVisualDataCallback.connect(step);