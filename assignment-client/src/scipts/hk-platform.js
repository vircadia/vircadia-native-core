var wantDebug = false;

var METER = 1.0 / TREE_SCALE;

var LIVE_CELL_COLOR = {r: 255, g: 255, b: 255};


var towerBaseLocation = { x: 2800 * METER, y: 0 * METER, z: 2240 * METER};
var towerTop = 352 * METER; // the y location of top of tower
var towerBrickSize = 16 * METER;
var towerColor = { r: 128, g: 128, b: 255};
var towerPlatformWidthInBricks = 36; // 9; 
var towerPlatformOffsetInBricks = 16; // 4; 
var towerPlatformBrickSize = 4 * METER;

var towerPlatformCorner = { 
        x: towerBaseLocation.x - (towerPlatformOffsetInBricks * towerPlatformBrickSize), 
        y: towerTop, 
        z: towerBaseLocation.z - (towerPlatformOffsetInBricks * towerPlatformBrickSize) };


var visualCallbacks = 0;
var platformBuilt = false;

// Note: 16 meters, 9x9 = 660 bytes
// Note: 8 meters, 18x18 (324) = 2940 bytes...
// 8 meters = ~9 bytes , 166 per packet

function buildPlatform() {
    // Cut hole in ground for tower
    for (var y = towerBaseLocation.y; y <= towerTop; y += towerBrickSize) {
        Voxels.queueVoxelDelete(towerBaseLocation.x, y, towerBaseLocation.z, towerBrickSize);
    }

    // Build the tower
    for (var y = towerBaseLocation.y; y <= towerTop; y += towerBrickSize) {
        Voxels.queueDestructiveVoxelAdd(towerBaseLocation.x, y, towerBaseLocation.z, 
                                        towerBrickSize, towerColor.r, towerColor.g, towerColor.b);
    }

    /**
    // Build the platform - this is actually handled buy the game of life board
    for (var x = 0; x < towerPlatformWidthInBricks; x++) {
        for (var z = 0; z < towerPlatformWidthInBricks; z++) {
            var platformBrick = { 
                x: towerPlatformCorner.x + (x * towerPlatformBrickSize),
                y: towerPlatformCorner.y,
                z: towerPlatformCorner.z + (z * towerPlatformBrickSize) };
                
            Voxels.queueDestructiveVoxelAdd(platformBrick.x, platformBrick.y, platformBrick.z, 
                                            towerPlatformBrickSize, towerColor.r, towerColor.g, towerColor.b);
        }
    }
    **/
}


// Add your JavaScript for assignment below this line 

// The following is an example of Conway's Game of Life (http://en.wikipedia.org/wiki/Conway's_Game_of_Life)

var NUMBER_OF_CELLS_EACH_DIMENSION = towerPlatformWidthInBricks;
var NUMBER_OF_CELLS = NUMBER_OF_CELLS_EACH_DIMENSION * NUMBER_OF_CELLS_EACH_DIMENSION;

var currentCells = [];
var nextCells = [];

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

  var REANIMATE_PROBABILITY = 0.005;
  
  var population = currentPopulation();
  var totalPossiblePopulation = NUMBER_OF_CELLS_EACH_DIMENSION * NUMBER_OF_CELLS_EACH_DIMENSION;
  // if current population is below 5% of total possible population, increase REANIMATE_PROBABILITY by 2x
  if (population < (totalPossiblePopulation * 0.05)) {
    REANIMATE_PROBABILITY = REANIMATE_PROBABILITY * 2;
  }
  
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

var fadeRatio = 0.0;
var fadeRatioAdjust = 0.25;

function sendNextCells() {
  var cellsSent = 0;
  for (var i = 0; i < NUMBER_OF_CELLS_EACH_DIMENSION; i++) {
    for (var j = 0; j < NUMBER_OF_CELLS_EACH_DIMENSION; j++) {
      if (nextCells[i][j] != -1) {
        // there has been a change to the state of this cell, send it
        
        // find the x and z position for this voxel, y = 0
        var x = towerPlatformCorner.x + j * towerPlatformBrickSize;
        var y = towerPlatformCorner.y;
        var z = towerPlatformCorner.z + i * towerPlatformBrickSize;

        // queue a packet to add a voxel for the new cell
        var ratio = fadeRatio;
        if (ratio > 1) {
            ratio = 1;
        }
        
        var fromColor, toColor;
        var color = LIVE_CELL_COLOR;
        if (nextCells[i][j] == 1) {
            //print("to LIVE from tower\n");

            color.r = Math.round((127 * ratio) + 128);
            color.g = Math.round((127 * ratio) + 128);
            color.b = 255;

        } else {
            //print("to tower from LIVE\n");


            color.r = Math.round((127 * (1-ratio)) + 128);
            color.g = Math.round((127 * (1-ratio)) + 128);
            color.b = 255;
        }


        if (color.r < 100 || color.r > 255) {
            print("ratio: " + ratio + " "
                 + "color: " + color.r + ", "
                     + color.g + ", "
                     + color.b + " "
                 + "\n");
        }
        Voxels.queueDestructiveVoxelAdd(x, y, z, towerPlatformBrickSize, color.r, color.g, color.b);
        cellsSent++;
      }
    }
  } 
  return cellsSent;
}

var visualCallbacks = 0;

function animatePlatform() {
    if (!platformBuilt) {
        buildPlatform();
        platformBuilt = true;
    } else {
        fadeRatio += fadeRatioAdjust;
        var cellsSent = sendNextCells();  

        //print("fadeRatio: " + fadeRatio + "\n")
        if (fadeRatio >= 1.0) {
            updateCells(); // send the initial game of life cells
            fadeRatio = 0.0;
        }
    }
    
    visualCallbacks++;
    
    if (wantDebug) {
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

}

Voxels.setPacketsPerSecond(500);

// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(animatePlatform);