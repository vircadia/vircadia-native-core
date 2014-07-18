//  rockPaperScissorsCells.js 
//  examples
//
//  Created by Ben Arnold on 7/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This sample script creates a voxel wall that simulates the Rock Paper Scissors cellular
//  automata. http://www.gamedev.net/blog/844/entry-2249737-another-cellular-automaton-video/
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var NUMBER_OF_CELLS_EACH_DIMENSION = 48;
var NUMBER_OF_CELLS_REGION_EACH_DIMESION = 16;
var REGIONS_EACH_DIMENSION = NUMBER_OF_CELLS_EACH_DIMENSION / NUMBER_OF_CELLS_REGION_EACH_DIMESION;

var isLocal = false;

var currentCells = [];
var nextCells = [];

var cornerPosition = {x: 100, y: 0, z: 0 }
var position = {x: 0, y: 0, z: 0 };

var METER_LENGTH = 1;
var cellScale = (NUMBER_OF_CELLS_EACH_DIMENSION * METER_LENGTH) / NUMBER_OF_CELLS_EACH_DIMENSION; 

var viewerPosition = {x: cornerPosition.x + (NUMBER_OF_CELLS_EACH_DIMENSION / 2) * cellScale, y: cornerPosition.y + (NUMBER_OF_CELLS_EACH_DIMENSION / 2) * cellScale, z: cornerPosition.z };

viewerPosition.z += 50;
var yaw = 0;
var orientation = Quat.fromPitchYawRollDegrees(0, yaw, 0);

//Feel free to add new cell types here. It can be more than three.
var cellTypes = [];
cellTypes[0] = { r: 255, g: 0, b: 0 };
cellTypes[1] = { r: 0, g: 255, b: 0 };
cellTypes[2] = { r: 0, g:0, b: 255 };
cellTypes[3] = { r: 0, g:255, b: 255 };
cellTypes[4] = { r: 255, g:0, b: 255 };
//cellTypes[5] = { r: 255, g:255, b: 255 };


//Check for free region for AC
var regionMarkerX = -1;
var regionMarkerY = -1;
var regionMarkerI = -1;
var regionMarkerJ = -1;

var regionMarkerColor = {r: 255, g: 0, b: 255};

for (var i = 0; i < REGIONS_EACH_DIMENSION; i++) {
    for (var j = 0; j < REGIONS_EACH_DIMENSION; j++) {
        var x = cornerPosition.x + (j) * cellScale;
        var y = cornerPosition.y + (i + NUMBER_OF_CELLS_EACH_DIMENSION) * cellScale;
        var z = cornerPosition.z;
        var voxel = Voxels.getVoxelAt(x, y, z, cellScale);
        if (voxel.x != x || voxel.y != y || voxel.z != z || voxel.s != cellScale ||
            voxel.red != regionMarkerColor.r || voxel.green != regionMarkerColor.g || voxel.blue != regionMarkerColor.b) {
            regionMarkerX = x;
            regionMarkerY = y;
            regionMarkerI = i;
            regionMarkerJ = j;
            i = REGIONS_EACH_DIMENSION; //force quit loop
            break;
        }
    }
}

if (regionMarkerX == -1) {
    print("No available Cellular Automata regions found!")
    Script.stop();
}

position.x = cornerPosition.x + regionMarkerJ * NUMBER_OF_CELLS_REGION_EACH_DIMESION * cellScale;
position.y = cornerPosition.y + regionMarkerI * NUMBER_OF_CELLS_REGION_EACH_DIMESION * cellScale;
position.z = cornerPosition.z;

Voxels.setVoxel(regionMarkerX, regionMarkerY, position.z, cellScale, regionMarkerColor.r, regionMarkerColor.g, regionMarkerColor.b);

// randomly populate the cell start values
for (var i = 0; i < NUMBER_OF_CELLS_REGION_EACH_DIMESION; i++) {
  // create the array to hold this row
  currentCells[i] = [];
  
  // create the array to hold this row in the nextCells array
  nextCells[i] = [];
  
  var randomColor = Math.floor(Math.random() * cellTypes.length);
  
  for (var j = 0; j < NUMBER_OF_CELLS_REGION_EACH_DIMESION; j++) {
     currentCells[i][j] = { changed: true, type: randomColor };
     
     // put the same value in the nextCells array for first board draw
     nextCells[i][j] = currentCells[i][j];
  }
}

function updateCells() {
    var i = 0;
    var j = 0;
    var cell;
    var y = 0;
    var x = 0;
    
    for (i = 0; i < NUMBER_OF_CELLS_REGION_EACH_DIMESION; i++) {
        for (j = 0; j < NUMBER_OF_CELLS_REGION_EACH_DIMESION; j++) {
          
            cell = currentCells[i][j];
          
            var r = Math.floor(Math.random() * 8);
            
            switch (r){
                case 0:
                    y = i - 1;
                    x = j - 1;
                    break;
                case 1:
                    y = i;
                    x = j-1;
                    break;
                case 2:
                    y = i + 1;
                    x = j - 1;
                    break;
                case 3:
                    y = i + 1;
                    x = j;
                    break;
                case 4:
                    y = i + 1;
                    x = j + 1;
                    break;
                case 5:
                    y = i;
                    x = j + 1;
                    break;
                case 6:
                    y = i - 1;
                    x = j + 1;
                    break;
                case 7:
                    y = i - 1;
                    x = j;
                    break;
                default:
                    continue;
                
            }
            
            //check the voxel grid instead of local array when on the edge
            if (x == -1 || x == NUMBER_OF_CELLS_REGION_EACH_DIMESION ||
                y == -1 || y == NUMBER_OF_CELLS_REGION_EACH_DIMESION) {
          
                var voxel = Voxels.getVoxelAt(position.x + x * cellScale, position.y + y * cellScale, position.z, cellScale);
                var predatorCellType = ((cell.type + 1) % cellTypes.length);
                var predatorCellColor = cellTypes[predatorCellType];
                if (voxel.red == predatorCellColor.r && voxel.green == predatorCellColor.g && voxel.blue == predatorCellColor.b) {
                    nextCells[i][j].type = predatorCellType;
                    nextCells[i][j].changed = true;
                }
            } else {
          
                if (currentCells[y][x].type == ((cell.type + 1) % cellTypes.length)) {
                    nextCells[i][j].type = currentCells[y][x].type;
                    nextCells[i][j].changed = true;
                } else {
                    //indicate no update
                    nextCells[i][j].changed = false;
                }
            }
        }
    }
  
    for (i = 0; i < NUMBER_OF_CELLS_REGION_EACH_DIMESION; i++) {
        for (j = 0; j < NUMBER_OF_CELLS_REGION_EACH_DIMESION; j++) {
            if (nextCells[i][j].changed == true) {
                // there has been a change to this cell, change the value in the currentCells array
                currentCells[i][j] = nextCells[i][j];
            }
        }
    }
}

function sendNextCells() {
    for (var i = 0; i < NUMBER_OF_CELLS_REGION_EACH_DIMESION; i++) {
        for (var j = 0; j < NUMBER_OF_CELLS_REGION_EACH_DIMESION; j++) {
            if (nextCells[i][j].changed == true) {
                // there has been a change to the state of this cell, send it

                // find the x and y position for this voxel, z = 0
                var x = j * cellScale;
                var y = i * cellScale;
                var type = nextCells[i][j].type;

                // queue a packet to add a voxel for the new cell
                Voxels.setVoxel(position.x + x, position.y + y, position.z, cellScale, cellTypes[type].r, cellTypes[type].g, cellTypes[type].b);
            }
        }
    } 
}

var sentFirstBoard = false;

var UPDATES_PER_SECOND = 6.0;
var frameIndex = 1.0;
var oldFrameIndex = 0;

function step(deltaTime) {
    frameIndex += deltaTime * UPDATES_PER_SECOND;
    if (Math.floor(frameIndex) == oldFrameIndex) {
        return;
    }
    oldFrameIndex++;

    if (sentFirstBoard) {
        // we've already sent the first full board, perform a step in time
        updateCells();
    } else {
        // this will be our first board send
        sentFirstBoard = true;
        
        print("AHHHH");
        print(viewerPosition.x + " " + viewerPosition.y + " " + viewerPosition.z);
        
        if (isLocal == false) {
            VoxelViewer.setPosition(viewerPosition);
            VoxelViewer.setOrientation(orientation);
            VoxelViewer.queryOctree();
        }
    }

    sendNextCells();  
}

function scriptEnding() {
    Voxels.eraseVoxel(regionMarkerX, regionMarkerY, position.z, cellScale);
}

Script.scriptEnding.connect(scriptEnding);

Script.update.connect(step);
Voxels.setPacketsPerSecond(2000);

// test for local...
Menu.isOptionChecked("Voxels");
isLocal = true; // will only get here on local client