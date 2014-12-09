//
//  growPlants.js 
//  examples
//
//  Created by Benjamin Arnold on May 29, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  This sample script allows the user to grow different types of plants on the voxels
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

var zFightingSizeAdjust = 0.002; // used to adjust preview voxels to prevent z fighting
var previewLineWidth = 2.0;

var voxelSize = 1;
var windowDimensions = Controller.getViewportDimensions();
var toolIconUrl = HIFI_PUBLIC_BUCKET + "images/tools/";

var MAX_VOXEL_SCALE_POWER = 5;
var MIN_VOXEL_SCALE_POWER = -8;
var MAX_VOXEL_SCALE = Math.pow(2.0, MAX_VOXEL_SCALE_POWER);
var MIN_VOXEL_SCALE = Math.pow(2.0, MIN_VOXEL_SCALE_POWER);


var linePreviewTop = Overlays.addOverlay("line3d", {
                                         position: { x: 0, y: 0, z: 0},
                                         end: { x: 0, y: 0, z: 0},
                                         color: { red: 0, green: 255, blue: 0},
                                         alpha: 1,
                                         visible: false,
                                         lineWidth: previewLineWidth
                                         });

var linePreviewBottom = Overlays.addOverlay("line3d", {
                                            position: { x: 0, y: 0, z: 0},
                                            end: { x: 0, y: 0, z: 0},
                                            color: { red: 0, green: 255, blue: 0},
                                            alpha: 1,
                                            visible: false,
                                            lineWidth: previewLineWidth
                                            });

var linePreviewLeft = Overlays.addOverlay("line3d", {
                                          position: { x: 0, y: 0, z: 0},
                                          end: { x: 0, y: 0, z: 0},
                                          color: { red: 0, green: 255, blue: 0},
                                          alpha: 1,
                                          visible: false,
                                          lineWidth: previewLineWidth
                                          });

var linePreviewRight = Overlays.addOverlay("line3d", {
                                           position: { x: 0, y: 0, z: 0},
                                           end: { x: 0, y: 0, z: 0},
                                           color: { red: 0, green: 255, blue: 0},
                                           alpha: 1,
                                           visible: false,
                                           lineWidth: previewLineWidth
                                           });

                              
var UIColor = { red: 0, green: 160, blue: 0};
var activeUIColor = { red: 0, green: 255, blue: 0};
            
var toolHeight = 50;
var toolWidth = 50;

var editToolsOn = true; 

var voxelToolSelected = false;
        
var scaleSelectorWidth = 144;
var scaleSelectorHeight = 37;        

var scaleSelectorX = windowDimensions.x / 5.0;
var scaleSelectorY = windowDimensions.y - scaleSelectorHeight;             
                                     
var voxelTool = Overlays.addOverlay("image", {
                                   x: scaleSelectorX + scaleSelectorWidth + 1, y: windowDimensions.y - toolHeight, width: toolWidth, height: toolHeight,
                                   subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                                   imageURL: toolIconUrl + "voxel-tool.svg",
                                   visible: editToolsOn,
                                   color: UIColor,
                                   alpha: 0.9
                                   });
                                           
var copyScale = true;
function ScaleSelector() {
   this.x = scaleSelectorX;
   this.y = scaleSelectorY;
   this.width = scaleSelectorWidth;
   this.height = scaleSelectorHeight;

   this.displayPower = false;
   this.scale = 1.0;
   this.power = 0;

   this.FIRST_PART = this.width * 40.0 / 100.0;
   this.SECOND_PART = this.width * 37.0 / 100.0;
   
   this.buttonsOverlay = Overlays.addOverlay("image", {
                                             x: this.x, y: this.y,
                                             width: this.width, height: this.height,
                                             //subImage: { x: 0, y: toolHeight, width: toolWidth, height: toolHeight },
                                             imageURL: toolIconUrl + "voxel-size-selector.svg",
                                             alpha: 0.9,
                                             visible: editToolsOn,
                                             color: activeUIColor
                                             });
   this.textOverlay = Overlays.addOverlay("text", {
                                          x: this.x + this.FIRST_PART, y: this.y,
                                          width: this.SECOND_PART, height: this.height,
                                          topMargin: 13,
                                          text: this.scale.toString(),
                                          backgroundAlpha: 0.0,
                                          visible: editToolsOn,
                                          color: activeUIColor
                                          });
   this.powerOverlay = Overlays.addOverlay("text", {
                                           x: this.x + this.FIRST_PART, y: this.y,
                                           width: this.SECOND_PART, height: this.height,
                                           leftMargin: 28,
                                           text: this.power.toString(),
                                           backgroundAlpha: 0.0,
                                           visible: false,
                                           color: activeUIColor
                                           });
   this.setScale = function(scale) {
       if (scale > MAX_VOXEL_SCALE) {
           scale = MAX_VOXEL_SCALE;
       }
       if (scale < MIN_VOXEL_SCALE) {
           scale = MIN_VOXEL_SCALE;
       }

       this.scale = scale;
       this.power = Math.floor(Math.log(scale) / Math.log(2));
       this.update();
   }

   this.show = function(doShow) {
       Overlays.editOverlay(this.buttonsOverlay, {visible: doShow});
       Overlays.editOverlay(this.textOverlay, {visible: doShow});
       Overlays.editOverlay(this.powerOverlay, {visible: doShow && this.displayPower});
   }

   this.move = function() {
       this.x = swatchesX + swatchesWidth;
       this.y = swatchesY;

       Overlays.editOverlay(this.buttonsOverlay, {
                            x: this.x, y: this.y,
                            });
       Overlays.editOverlay(this.textOverlay, {
                            x: this.x + this.FIRST_PART, y: this.y,
                            });
       Overlays.editOverlay(this.powerOverlay, {
                            x: this.x + this.FIRST_PART, y: this.y,
                            });
   }


   this.switchDisplay = function() {
       this.displayPower = !this.displayPower;

       if (this.displayPower) {
           Overlays.editOverlay(this.textOverlay, {
                                leftMargin: 18,
                                text: "2"
                                });
           Overlays.editOverlay(this.powerOverlay, {
                                text: this.power.toString(),
                                visible: editToolsOn
                                });
       } else {
           Overlays.editOverlay(this.textOverlay, {
                                leftMargin: 13,
                                text: this.scale.toString()
                                });
           Overlays.editOverlay(this.powerOverlay, {
                                visible: false
                                });
       }
   }

   this.update = function() {
       if (this.displayPower) {
           Overlays.editOverlay(this.powerOverlay, {text: this.power.toString()});
       } else {
           Overlays.editOverlay(this.textOverlay, {text: this.scale.toString()});
       }
   }

   this.incrementScale = function() {
       copyScale = false;
       if (this.power < MAX_VOXEL_SCALE_POWER) {
           ++this.power;
           this.scale *= 2.0;
           this.update();
       }
   }

   this.decrementScale = function() {
       copyScale = false;
       if (MIN_VOXEL_SCALE_POWER < this.power) {
           --this.power;
           this.scale /= 2.0;
           this.update();
       }
   }

   this.clicked = function(x, y) {
       if (this.x < x  && x < this.x + this.width &&
           this.y < y && y < this.y + this.height) {

           if (x < this.x + this.FIRST_PART) {
               this.decrementScale();
           } else if (x < this.x + this.FIRST_PART + this.SECOND_PART) {
               this.switchDisplay();
           } else {
               this.incrementScale();
           }
           return true;
       }
       return false;
   }

   this.cleanup = function() {
       Overlays.deleteOverlay(this.buttonsOverlay);
       Overlays.deleteOverlay(this.textOverlay);
       Overlays.deleteOverlay(this.powerOverlay);
   }

}
var scaleSelector = new ScaleSelector();

                                           
function calculateVoxelFromIntersection(intersection, operation) {

    var resultVoxel;
   
    var x;
    var y;
    var z;
    
    // if our "target voxel size" is larger than the voxel we intersected with, then we need to find the closest
    // ancestor voxel of our target size that contains our intersected voxel.
    if (voxelSize > intersection.voxel.s) {
        x = Math.floor(intersection.voxel.x / voxelSize) * voxelSize;
        y = Math.floor(intersection.voxel.y / voxelSize) * voxelSize;
        z = Math.floor(intersection.voxel.z / voxelSize) * voxelSize;
    } else {
        // otherwise, calculate the enclosed voxel of size voxelSize that the intersection point falls inside of.
        // if you have a voxelSize that's smaller than the voxel you're intersecting, this calculation will result
        // in the subvoxel that the intersection point falls in, if the target voxelSize matches the intersecting
        // voxel this still works and results in returning the intersecting voxel which is what we want
        var adjustToCenter = Vec3.multiply(Voxels.getFaceVector(intersection.face), (voxelSize * -0.5));
       
        var centerOfIntersectingVoxel = Vec3.sum(intersection.intersection, adjustToCenter);
        x = Math.floor(centerOfIntersectingVoxel.x / voxelSize) * voxelSize;
        y = Math.floor(centerOfIntersectingVoxel.y / voxelSize) * voxelSize;
        z = Math.floor(centerOfIntersectingVoxel.z / voxelSize) * voxelSize;
    }
    resultVoxel = { x: x, y: y, z: z, s: voxelSize };
    var highlightAt = { x: x, y: y, z: z, s: voxelSize };
    


    // we only do the "add to the face we're pointing at" adjustment, if the operation is an add
    // operation, and the target voxel size is equal to or smaller than the intersecting voxel.
    var wantAddAdjust = (operation == "add" && (voxelSize <= intersection.voxel.s));
   
    // now we also want to calculate the "edge square" for the face for this voxel
    if (intersection.face == "MIN_X_FACE") {
        
        highlightAt.x = x - zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.x -= voxelSize;
        }
        
        resultVoxel.bottomLeft = {x: highlightAt.x, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.bottomRight = {x: highlightAt.x, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z + voxelSize - zFightingSizeAdjust };
        resultVoxel.topLeft = {x: highlightAt.x, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.topRight = {x: highlightAt.x, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z + voxelSize - zFightingSizeAdjust };
        
    } else if (intersection.face == "MAX_X_FACE") {
        
        highlightAt.x = x + voxelSize + zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.x += resultVoxel.s;
        }
        
        resultVoxel.bottomRight = {x: highlightAt.x, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.bottomLeft = {x: highlightAt.x, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z + voxelSize - zFightingSizeAdjust };
        resultVoxel.topRight = {x: highlightAt.x, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.topLeft = {x: highlightAt.x, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z + voxelSize - zFightingSizeAdjust };
        
    } else if (intersection.face == "MIN_Y_FACE") {
        
        highlightAt.y = y - zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.y -= voxelSize;
        }
        
        resultVoxel.topRight = {x: highlightAt.x + zFightingSizeAdjust , y: highlightAt.y, z: highlightAt.z + zFightingSizeAdjust  };
        resultVoxel.topLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.bottomRight = {x: highlightAt.x + zFightingSizeAdjust , y: highlightAt.y, z: highlightAt.z  + voxelSize - zFightingSizeAdjust };
        resultVoxel.bottomLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust , y: highlightAt.y, z: highlightAt.z + voxelSize - zFightingSizeAdjust };
        
    } else if (intersection.face == "MAX_Y_FACE") {
        
        highlightAt.y = y + voxelSize + zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.y += voxelSize;
        }
        
        resultVoxel.bottomRight = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z + zFightingSizeAdjust };
        resultVoxel.bottomLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z + zFightingSizeAdjust};
        resultVoxel.topRight = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z  + voxelSize - zFightingSizeAdjust};
        resultVoxel.topLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y, z: highlightAt.z + voxelSize - zFightingSizeAdjust};
        
    } else if (intersection.face == "MIN_Z_FACE") {
        
        highlightAt.z = z - zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.z -= voxelSize;
        }
        
        resultVoxel.bottomRight = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z };
        resultVoxel.bottomLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z};
        resultVoxel.topRight = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z };
        resultVoxel.topLeft = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z};
        
    } else if (intersection.face == "MAX_Z_FACE") {
        
        highlightAt.z = z + voxelSize + zFightingSizeAdjust;
        if (wantAddAdjust) {
            resultVoxel.z += voxelSize;
        }
        
        resultVoxel.bottomLeft = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z };
        resultVoxel.bottomRight = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y + zFightingSizeAdjust, z: highlightAt.z};
        resultVoxel.topLeft = {x: highlightAt.x + zFightingSizeAdjust, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z };
        resultVoxel.topRight = {x: highlightAt.x + voxelSize - zFightingSizeAdjust, y: highlightAt.y + voxelSize - zFightingSizeAdjust, z: highlightAt.z};
        
    }
    return resultVoxel;
}
                                           
var trackLastMouseX = 0;
var trackLastMouseY = 0;

function showPreviewLines() {

    var pickRay = Camera.computePickRay(trackLastMouseX, trackLastMouseY);

    var intersection = Voxels.findRayIntersection(pickRay);

    if (intersection.intersects) {
        var resultVoxel = calculateVoxelFromIntersection(intersection, "");
        Overlays.editOverlay(linePreviewTop, { position: resultVoxel.topLeft, end: resultVoxel.topRight, visible: true });
        Overlays.editOverlay(linePreviewBottom, { position: resultVoxel.bottomLeft, end: resultVoxel.bottomRight, visible: true });
        Overlays.editOverlay(linePreviewLeft, { position: resultVoxel.topLeft, end: resultVoxel.bottomLeft, visible: true });
        Overlays.editOverlay(linePreviewRight, { position: resultVoxel.topRight, end: resultVoxel.bottomRight, visible: true });
   } else {
        Overlays.editOverlay(linePreviewTop, { visible: false });
        Overlays.editOverlay(linePreviewBottom, { visible: false });
        Overlays.editOverlay(linePreviewLeft, { visible: false });
        Overlays.editOverlay(linePreviewRight, { visible: false });
    }
}

function mouseMoveEvent(event) {
    trackLastMouseX = event.x;
    trackLastMouseY = event.y;
    if (!voxelToolSelected) {
        return;
    }
    showPreviewLines();
}


// Array of possible trees, right now there is only one
var treeTypes = [];

treeTypes.push({
    name: "Tall Green",
    // Voxel Colors
    wood: { r: 133, g: 81, b: 53 },
    leaves: { r: 22, g: 83, b: 31 },

    // How tall the tree is
    height: { min: 20, max: 60 },
    middleHeight: 0.3,

    // Chance of making a branch
    branchChance: { min: 0.01, max: 0.1 },
    branchLength: { min: 30, max: 60 },
    branchThickness: { min: 2, max: 7},

    // The width of the core, affects width and shape
    coreWidth: { min: 1, max: 4 },

    //TODO: Make this quadratic splines instead of linear
    bottomThickness: { min: 2, max: 8 },
    middleThickness: { min: 1, max: 4 },
    topThickness: { min: 3, max: 6 },
    
    //Modifies leaves at top
    leafCapSizeOffset: 0
});

// Applies noise to color
var colorNoiseRange = 0.2;


// Useful constants
var LEFT = 0;
var BACK = 1;
var RIGHT = 2;
var FRONT = 3;
var UP = 4;

// Interpolates between min and max of treevar based on b
function interpolate(treeVar, b) {
    return (treeVar.min + (treeVar.max - treeVar.min) * b);
}

function makeBranch(x, y, z, step, length, dir, thickness, wood, leaves) {
    var moveDir;

    var currentThickness;
    //thickness attenuates to thickness - 3
    var finalThickness = thickness - 3;
    if (finalThickness < 1) {
        finalThickness = 1;
    }
    //Iterative branch generation
    while (true) {
   
        //If we are at the end, place a ball of leaves
        if (step == 0) {
            makeSphere(x, y, z, 2 + finalThickness, leaves); 
            return;
        }
        //thickness attenuation
        currentThickness = Math.round((finalThickness + (thickness - finalThickness) * (step/length))) - 1;
		
        
        // If the branch is thick, grow a vertical slice
        if (currentThickness > 0) {
            for (var i = -currentThickness; i <= currentThickness; i++) {
                var len = currentThickness - Math.abs(i);
                switch (dir) {
                case 0: //left
                case 2: //right
                    growInDirection(x, y + i * voxelSize, z, len, len, BACK, wood, false, true);
                    growInDirection(x, y + i * voxelSize, z, len, len, FRONT, wood, false, false)          
                    break;
                case 1: //back
                case 3: //front
                    growInDirection(x, y + i * voxelSize, z, len, len, LEFT, wood, false, true);
                    growInDirection(x, y + i * voxelSize, z, len, len, RIGHT, wood, false, false)         
                    break;
                }
            }
        } else {
            //Otherwise place a single voxel
            var colorNoise = (colorNoiseRange * Math.random() - colorNoiseRange * 0.5) + 1.0;
            Voxels.setVoxel(x, y, z, voxelSize, wood.r * colorNoise, wood.g * colorNoise, wood.b * colorNoise);
        }

        // determines random change in direction for branch
        var r = Math.floor(Math.random() * 9);

         
        if (r >= 6){
            moveDir = dir; //in same direction
        } else if (r >= 4) {
            moveDir = UP; //up
        }
        else if (dir == LEFT){
            if (r >= 2){
                moveDir = FRONT;
            }
            else{
                moveDir = BACK;
            }
        }
        else if (dir == BACK){
            if (r >= 2){
                moveDir = LEFT;
            }
            else{
                moveDir = RIGHT;
            }
        }
        else if (dir == RIGHT){
            if (r >= 2){
                moveDir = BACK;
            }
            else{
                moveDir = FRONT;
            }
        }
        else if (dir == FRONT){
            if (r >= 2){
                moveDir = RIGHT;
            }
            else{
                moveDir = LEFT;
            }
        }

        //Move the branch by moveDir
        switch (moveDir) {
        case 0: //left 
            x = x - voxelSize;
            break;
        case 1: //back
            z = z - voxelSize;
            break;
        case 2: //right
            x = x + voxelSize;
            break;
        case 3: //front
            z = z + voxelSize;
            break;        
        case 4: //up
            y = y + voxelSize;
            break;
        }
        
	step--;
    }
}

// Places a sphere of voxels
function makeSphere(x, y, z, radius, color) {
    if (radius <= 0) {
        return;
    }
    var width = radius * 2 + 1;
    var distance;
    
    for (var i = -radius; i <= radius; i++){
        for (var j = -radius; j <= radius; j++){
            for (var k = -radius; k <= radius; k++){
                distance = Math.sqrt(i * i + j * j + k * k);
                if (distance <= radius){
                     var colorNoise = (colorNoiseRange * Math.random() - colorNoiseRange * 0.5) + 1.0;
                     Voxels.setVoxel(x + i * voxelSize, y + j * voxelSize, z + k * voxelSize, voxelSize, color.r * colorNoise, color.g * colorNoise, color.b * colorNoise);
                }
            }
        }
    }
}

function growInDirection(x, y, z, step, length, dir, color, isSideBranching, addVoxel) {
    

    if (addVoxel == true) {
        var colorNoise = (colorNoiseRange * Math.random() - colorNoiseRange * 0.5) + 1.0; 
        Voxels.setVoxel(x, y, z, voxelSize, color.r * colorNoise, color.g * colorNoise, color.b * colorNoise);
	}

    // If this is a main vein, it will branch outward perpendicular to its motion
	if (isSideBranching == true){
        var step2;
		if (step >= length - 1){
			step2 = length;
		}
		else{
			step2 = step + 1;
		}
        growInDirection(x, y, z, step, length, BACK, color, false, false);
        growInDirection(x, y, z, step, length, FRONT, color, false, false);
	}

	if (step < 1) return;

    // Recursively move in the direction
	if (dir == LEFT) { //left
		growInDirection(x - voxelSize, y, z, step - 1, length, dir, color, isSideBranching, true);
	}
	else if (dir == BACK) { //back
		growInDirection(x, y, z - voxelSize, step - 1, length, dir, color, isSideBranching, true);
    }
	else if (dir == RIGHT) { //right
		growInDirection(x + voxelSize, y, z, step - 1, length, dir, color, isSideBranching, true);
	}
	else if (dir == FRONT) {//front
	    growInDirection(x, y, z + voxelSize, step - 1, length, dir, color, isSideBranching, true);
	}

}

// Grows the thickness of the tree
function growHorizontalSlice(x, y, z, thickness, color, side) {
    // The side variable determines which directions we should grow in
    // it is an optimization that prevents us from visiting voxels multiple
    // times for trees with a coreWidth > 1
    
    // side:
    //  8 == all directions
    //  0  1  2
    //  3 -1  4
    //  5  6  7
    
    Voxels.setVoxel(x, y, z, voxelSize, color.r, color.g, color.b);
    
    //We are done if there is no thickness
    if (thickness == 0) {
        return;
    }
    
    
    switch (side) {
    case 0:
        growInDirection(x, y, z, thickness, thickness, LEFT, color, true, false);
        break;
    case 1:   
        growInDirection(x, y, z, thickness, thickness, BACK, color, false, false); 
        break;
    case 2:
        growInDirection(x, y, z, thickness, thickness, RIGHT, color, true, false);   
        break;
    case 3:
        growInDirection(x, y, z, thickness, thickness, LEFT, color, false, false);
        break;
    case 4:
        growInDirection(x, y, z, thickness, thickness, BACK, color, false, false);
        break;
    case 5:
        growInDirection(x, y, z, thickness, thickness, RIGHT, color, true, false);
        break;
    case 6:
        growInDirection(x, y, z, thickness, thickness, FRONT, color, false, false); 
        break;
    case 7:
        growInDirection(x, y, z, thickness, thickness, RIGHT, color, true, false);
        break;
    case 8:
        if (thickness > 1){
            growInDirection(x, y, z, thickness, thickness, LEFT, color, true, false);
            growInDirection(x, y, z, thickness, thickness, RIGHT, color, true, false)     
        } else if (thickness == 1){
            Voxels.setVoxel(x - voxelSize, y, z, voxelSize, color.r, color.g, color.b);
            Voxels.setVoxel(x + voxelSize, y, z, voxelSize, color.r, color.g, color.b);
            Voxels.setVoxel(x, y, z - voxelSize, voxelSize, color.r, color.g, color.b);
            Voxels.setVoxel(x, y, z + voxelSize, voxelSize, color.r, color.g, color.b);
        }
        break;
    }
}

function computeSide(x, z, coreWidth) {
    // side:
    //  8 == all directions
    //  0  1  2
    //  3 -1  4
    //  5  6  7
   
    // if the core is only a single block, we can grow out in all directions
    if (coreWidth == 1){
        return 8;
    }
    
    // Back face
    if (z == 0) {
        if (x == 0) {
            return 0;
        } else if (x == coreWidth - 1) {
            return 2;
        } else {
            return 1;
        }  
    }
    
    // Front face
    if (z == (coreWidth - 1)) {
        if (x == 0) {
            return 5;
        } else if (x == (coreWidth - 1)) {
            return 7;
        } else {
            return 6;
        }  
    }
    
    // Left face
    if (x == 0) {
        return 3;
    }
    
    // Right face
    if (x == (coreWidth - 1)) {
        return 4;
    }
     
    //Interior
    return -1;
}


function growTree(x, y, z, tree) {

    // The size of the tree, from 0-1
    var treeSize = Math.random();
    
    // Get tree properties by interpolating with the treeSize
    var height = interpolate(tree.height, treeSize);
    var baseHeight = Math.ceil(tree.middleHeight * height);
    var bottomThickness = interpolate(tree.bottomThickness, treeSize);
    var middleThickness = interpolate(tree.middleThickness, treeSize);
    var topThickness = interpolate(tree.topThickness, treeSize);
    var coreWidth = Math.ceil(interpolate(tree.coreWidth, treeSize));

    var thickness;
    var side;
    
    //Loop upwards through each slice of the tree
    for (var i = 0; i < height; i++){
    
        //Branch properties are based on current height as well as the overall tree size
        var branchChance = interpolate(tree.branchChance, i / height);
        var branchLength = Math.ceil(interpolate(tree.branchLength, (i / height) * treeSize));
        var branchThickness = Math.round(interpolate(tree.branchThickness, (i / height) * treeSize));
        
        // Get the "thickness" of the tree by doing linear interpolation between the middle thickness
        // and the top and bottom thickness.
        if (i <= baseHeight && baseHeight != 0){
            thickness = (i / (baseHeight) * (middleThickness - bottomThickness) + bottomThickness);
        } else {
            var denom = ((height - baseHeight)) * (topThickness - middleThickness) + middleThickness;
            if (denom != 0) {
                thickness = (i - baseHeight) / denom;
            } else {
                thickness = 0;
            }
        }
        // The core of the tree is a vertical rectangular prism through the middle of the tree
        
        //Loop through the "core", which helps shape the trunk
        var startX = x - Math.floor(coreWidth / 2) * voxelSize;
        var startZ = z - Math.floor(coreWidth / 2) * voxelSize;
        for (var j = 0; j < coreWidth; j++) {
            for (var k = 0; k < coreWidth; k++) {
                //determine which side of the tree we are on
                side = computeSide(j, k, coreWidth);
                //grow a horizontal slice of the tree
                growHorizontalSlice(startX + j * voxelSize, y + i * voxelSize, startZ + k * voxelSize, Math.floor(thickness), tree.wood, side);
                
                // Branches
                if (side != -1) {
                    var r = Math.random();
                    if (r <= branchChance){
                        var dir = Math.floor((Math.random() * 4)); 
                        makeBranch(startX + j * voxelSize, y + i * voxelSize, startZ + k * voxelSize, branchLength, branchLength, dir, branchThickness, tree.wood, tree.leaves);
                       
                    }
                }
            }
        }
    }
    
    makeSphere(x, y + height * voxelSize, z, topThickness + coreWidth + tree.leafCapSizeOffset, tree.leaves);
}

function mousePressEvent(event) {
    var mouseX = event.x;
    var mouseY = event.y;
    
    var clickedOnSomething = false;
    // Check if we clicked an overlay
    var clickedOverlay = Overlays.getOverlayAtPoint({x: mouseX, y: mouseY});
    
    if (clickedOverlay == voxelTool) {
        voxelToolSelected = !voxelToolSelected;
        
        if (voxelToolSelected == true) {
            Overlays.editOverlay(voxelTool, {
                                 color: activeUIColor
                                 });
        } else { 
            Overlays.editOverlay(voxelTool, {
                                 color: UIColor
                                 });
        }
        
        clickedOnSomething = true;
    } else if (scaleSelector.clicked(event.x, event.y)) {        
        clickedOnSomething = true;
        voxelSize = scaleSelector.scale;
    }
    
    // Return if we clicked on the UI or the voxel tool is disabled
    if (clickedOnSomething || !voxelToolSelected) {
        return;
    }  
    
    // Compute the picking ray for the click
    var pickRay = Camera.computePickRay(event.x, event.y);
    var intersection = Voxels.findRayIntersection(pickRay);
    var resultVoxel = calculateVoxelFromIntersection(intersection, "add");

    // Currently not in use, could randomly select a tree
    var treeIndex = Math.floor(Math.random() * treeTypes.length);
    
    // Grow the first tree type
    growTree(resultVoxel.x, resultVoxel.y, resultVoxel.z, treeTypes[0]);

}


function scriptEnding() {
    Overlays.deleteOverlay(linePreviewTop);
    Overlays.deleteOverlay(linePreviewBottom);
    Overlays.deleteOverlay(linePreviewLeft);
    Overlays.deleteOverlay(linePreviewRight);
    scaleSelector.cleanup();
    Overlays.deleteOverlay(voxelTool);
}

Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);

Script.scriptEnding.connect(scriptEnding);

Voxels.setPacketsPerSecond(10000);
