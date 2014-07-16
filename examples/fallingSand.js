//
//  fallingSand.js 
//  examples
//
//  Created by Ben Arnold on 7/14/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This sample script allows the user to place sand voxels that will undergo
//  cellular automata physics.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var zFightingSizeAdjust = 0.002; // used to adjust preview voxels to prevent z fighting
var previewLineWidth = 2.0;

var voxelSize = 1;
var windowDimensions = Controller.getViewportDimensions();
var toolIconUrl = "http://highfidelity-public.s3-us-west-1.amazonaws.com/images/tools/";

var MAX_VOXEL_SCALE_POWER = 5;
var MIN_VOXEL_SCALE_POWER = -8;
var MAX_VOXEL_SCALE = Math.pow(2.0, MAX_VOXEL_SCALE_POWER);
var MIN_VOXEL_SCALE = Math.pow(2.0, MIN_VOXEL_SCALE_POWER);


var linePreviewTop = Overlays.addOverlay("line3d", {
                                         position: { x: 0, y: 0, z: 0},
                                         end: { x: 0, y: 0, z: 0},
                                         color: { red: 0, green: 0, blue: 255},
                                         alpha: 1,
                                         visible: false,
                                         lineWidth: previewLineWidth
                                         });

var linePreviewBottom = Overlays.addOverlay("line3d", {
                                            position: { x: 0, y: 0, z: 0},
                                            end: { x: 0, y: 0, z: 0},
                                            color: { red: 0, green: 0, blue: 255},
                                            alpha: 1,
                                            visible: false,
                                            lineWidth: previewLineWidth
                                            });

var linePreviewLeft = Overlays.addOverlay("line3d", {
                                          position: { x: 0, y: 0, z: 0},
                                          end: { x: 0, y: 0, z: 0},
                                          color: { red: 0, green: 0, blue: 255},
                                          alpha: 1,
                                          visible: false,
                                          lineWidth: previewLineWidth
                                          });

var linePreviewRight = Overlays.addOverlay("line3d", {
                                           position: { x: 0, y: 0, z: 0},
                                           end: { x: 0, y: 0, z: 0},
                                           color: { red: 0, green: 0, blue: 255},
                                           alpha: 1,
                                           visible: false,
                                           lineWidth: previewLineWidth
                                           });

                              
var UIColor = { red: 119, green: 103, blue: 53};
var activeUIColor = { red: 234, green: 206, blue: 106};
            
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
                                          alpha: 0.0,
                                          visible: editToolsOn,
                                          color: activeUIColor
                                          });
   this.powerOverlay = Overlays.addOverlay("text", {
                                           x: this.x + this.FIRST_PART, y: this.y,
                                           width: this.SECOND_PART, height: this.height,
                                           leftMargin: 28,
                                           text: this.power.toString(),
                                           alpha: 0.0,
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

var BRUSH_RADIUS = 2;

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

   
    //Add a clump of sand voxels
    makeSphere(resultVoxel.x, resultVoxel.y, resultVoxel.z, voxelSize * BRUSH_RADIUS, voxelSize);
}

var sandArray = [];
var numSand = 0;


//These arrays are used to buffer add/remove operations so they can be batched together
var addArray = [];
var addArraySize = 0;
var removeArray = [];
var removeArraySize = 0;

//The colors must be different
var activeSandColor = { r: 234, g: 206, b: 106};
var inactiveSandColor = { r: 233, g: 206, b: 106};

//This is used as an optimization, so that we
//will check our 6 neighbors at most once.
var adjacentVoxels = [];
var numAdjacentVoxels = 0;
//Stores a list of voxels we need to activate
var activateMap = {};

var UPDATES_PER_SECOND = 12.0; // frames per second
var frameIndex = 0.0;
var oldFrameIndex = 0;

function update(deltaTime) {
    frameIndex += deltaTime * UPDATES_PER_SECOND;
    if (Math.floor(frameIndex) == oldFrameIndex) {
        return;
    }
    oldFrameIndex++;
    
    //Clear the activate map each frame
    activateMap = {};
    
    //Update all sand in our sandArray
    var i = 0;
    while (i < numSand) {
        //Update the sand voxel and if it doesn't move, deactivate it
        if (updateSand(i) == false) {
            deactivateSand(i);
        } else {
            i++;
        }
    }
    
    for (var i = 0; i < removeArraySize; i++) {
        var voxel = removeArray[i];
        Voxels.eraseVoxel(voxel.x, voxel.y, voxel.z, voxel.s);
    }
    removeArraySize = 0;
     
    //Add all voxels that have moved
    for (var i = 0; i < addArraySize; i++) {
        var voxel = addArray[i];
        Voxels.setVoxel(voxel.x, voxel.y, voxel.z, voxel.s, voxel.r, voxel.g, voxel.b);
    }
    addArraySize = 0;
    
    for (var key in activateMap) {
        var voxel = activateMap[key];
        Voxels.setVoxel(voxel.x, voxel.y, voxel.z, voxel.s, activeSandColor.r, activeSandColor.g, activeSandColor.b);
        sandArray[numSand++] = { x: voxel.x, y: voxel.y, z: voxel.z, s: voxel.s, r: activeSandColor.r, g: activeSandColor.g, b: activeSandColor.b };
    }
}

//Adds a sphere of sand at the center cx,cy,cz
function makeSphere(cx, cy, cz, r, voxelSize) {

    var r2 = r * r;
    var distance2;
    var dx;
    var dy;
    var dz;
     
    for (var x = cx - r; x <= cx + r; x += voxelSize) {
        for (var y = cy - r; y <= cy + r; y += voxelSize) {
            for (var z = cz - r; z <= cz + r; z += voxelSize) {
                dx = Math.abs(x - cx);
                dy = Math.abs(y - cy);
                dz = Math.abs(z - cz);
                distance2 = dx * dx + dy * dy + dz * dz;
                if (distance2 <= r2 && isVoxelEmpty(x, y, z, voxelSize)) {
                    Voxels.setVoxel(x, y, z, voxelSize, activeSandColor.r, activeSandColor.g, activeSandColor.b);
                    sandArray[numSand++] = { x: x, y: y, z: z, s: voxelSize, r: activeSandColor.r, g: activeSandColor.g, b: activeSandColor.b };
                }
            }
        }
    }
}

//Check if a given voxel is empty
function isVoxelEmpty(x, y, z, s, isAdjacent) {
    var halfSize = s / 2;
    var point = {x: x + halfSize, y: y + halfSize, z: z + halfSize };
   
    var adjacent = Voxels.getVoxelEnclosingPointBlocking(point);
    //If color is all 0, we assume its air.
   
    if (adjacent.red == 0 && adjacent.green == 0 && adjacent.blue == 0) {
        return true;
    }
    
    if (isAdjacent) {
        adjacentVoxels[numAdjacentVoxels++] = adjacent;
    }

    return false;
}

//Moves voxel to x,y,z if the space is empty
function tryMoveVoxel(voxel, x, y, z) {
    //If the adjacent voxel is empty, we will move to it.
    if (isVoxelEmpty(x, y, z, voxel.s, false)) {
        var hsize = voxel.s / 2;
        for (var i = 0; i < 5; i++) {
            var point = {x: voxel.x + directionVecs[i].x * voxel.s + hsize, y: voxel.y + directionVecs[i].y * voxel.s + hsize, z: voxel.z + directionVecs[i].z * voxel.s + hsize };
            adjacentVoxels[numAdjacentVoxels++] = Voxels.getVoxelEnclosingPointBlocking(point);
        }
        moveVoxel(voxel, x, y, z);
        
        //Get all adjacent voxels for activation
        return true;
    }
    return false;
}

//Moves voxel to x,y,z
function moveVoxel(voxel, x, y, z) {
    activateNeighbors();
    removeArray[removeArraySize++] = {x: voxel.x, y: voxel.y, z: voxel.z, s: voxel.s};
    addArray[addArraySize++] = {x: x, y: y, z: z, s: voxel.s, r: activeSandColor.r, g: activeSandColor.g, b: activeSandColor.b};
    voxel.x = x;
    voxel.y = y;
    voxel.z = z;
}

var LEFT = 0;
var BACK = 1;
var RIGHT = 2;
var FRONT = 3;
var TOP = 4;

//These indicate the different directions to neighbor voxels, so we can iterate them
var directionVecs = [];
directionVecs[LEFT] = {x: -1, y: 0, z: 0}; //Left
directionVecs[BACK] = {x: 0, y: 0, z: -1}; //Back
directionVecs[RIGHT] = {x: 1, y: 0, z: 0}; //Right
directionVecs[FRONT] = {x: 0, y: 0, z: 1}; //Front
directionVecs[TOP] = {x: 0, y: 1, z: 0}; //Top

function updateSand(i) {
    var voxel = sandArray[i];
    var size = voxel.s;
    var hsize = size / 2;
    numAdjacentVoxels = 0;

    //Down
    if (tryMoveVoxel(voxel, voxel.x, voxel.y - size, voxel.z)) {
        return true;
    }
    
    //Left, back, right, front
    for (var i = 0; i < 4; i++) {
        if (isVoxelEmpty(voxel.x + directionVecs[i].x * size, voxel.y + directionVecs[i].y * size, voxel.z + directionVecs[i].z * size, size, true)
         && isVoxelEmpty(voxel.x + directionVecs[i].x * size, (voxel.y - size) + directionVecs[i].y * size, voxel.z + directionVecs[i].z * size, size, false)) {
            //get the rest of the adjacent voxels
            for (var j = i + 1; j < 5; j++) {
                var point = {x: voxel.x + directionVecs[j].x * size + hsize, y: voxel.y + directionVecs[j].y * size + hsize, z: voxel.z + directionVecs[j].z * size + hsize }; 
                adjacentVoxels[numAdjacentVoxels++] = Voxels.getVoxelEnclosingPointBlocking(point);
            }
            moveVoxel(voxel, voxel.x + directionVecs[i].x * size, voxel.y + directionVecs[i].y * size, voxel.z + directionVecs[i].z * size);
            return true;
        }
    }
    
    return false;
}

function activateNeighbors() {
    for (var i = 0; i < numAdjacentVoxels; i++) {
        var voxel = adjacentVoxels[i];
        //Check if this neighbor is inactive, if so, activate it
        if (voxel.red == inactiveSandColor.r && voxel.green == inactiveSandColor.g && voxel.blue == inactiveSandColor.b) {
            activateMap[voxel.x.toString() + "," + voxel.y.toString() + ',' + voxel.z.toString()] = voxel;
        }
    }
}

//Deactivates a sand voxel to save processing power
function deactivateSand(i) {
    var voxel = sandArray[i];
    addArray[addArraySize++] = {x: voxel.x, y: voxel.y, z: voxel.z, s: voxel.s, r: inactiveSandColor.r, g: inactiveSandColor.g, b: inactiveSandColor.b};
    sandArray[i] = sandArray[numSand-1];
    numSand--;
}

//Cleanup
function scriptEnding() {
    for (var i = 0; i < numSand; i++) {
        var voxel = sandArray[i];
        Voxels.eraseVoxel(voxel.x, voxel.y, voxel.z, voxel.s);
    }
    Overlays.deleteOverlay(linePreviewTop);
    Overlays.deleteOverlay(linePreviewBottom);
    Overlays.deleteOverlay(linePreviewLeft);
    Overlays.deleteOverlay(linePreviewRight);
    scaleSelector.cleanup();
    Overlays.deleteOverlay(voxelTool);
}

Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

Voxels.setMaxPacketSize(1); //this is needed or a bug occurs :(
Voxels.setPacketsPerSecond(10000);