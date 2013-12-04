// Ported from Stefan Gustavson's java implementation
// http://staffwww.itn.liu.se/~stegu/simplexnoise/simplexnoise.pdf
// Read Stefan's excellent paper for details on how this code works.
//
// Sean McCullough banksean@gmail.com
 
/**
 * You can pass in a random number generator object if you like.
 * It is assumed to have a random() method.
 */
var SimplexNoise = function(r) {
	if (r == undefined) r = Math;
  this.grad3 = [[1,1,0],[-1,1,0],[1,-1,0],[-1,-1,0], 
                                 [1,0,1],[-1,0,1],[1,0,-1],[-1,0,-1], 
                                 [0,1,1],[0,-1,1],[0,1,-1],[0,-1,-1]]; 
  this.p = [];
  for (var i=0; i<256; i++) {
	  this.p[i] = Math.floor(r.random()*256);
  }
  // To remove the need for index wrapping, double the permutation table length 
  this.perm = []; 
  for(var i=0; i<512; i++) {
		this.perm[i]=this.p[i & 255];
	} 
 
  // A lookup table to traverse the simplex around a given point in 4D. 
  // Details can be found where this table is used, in the 4D noise method. 
  this.simplex = [ 
    [0,1,2,3],[0,1,3,2],[0,0,0,0],[0,2,3,1],[0,0,0,0],[0,0,0,0],[0,0,0,0],[1,2,3,0], 
    [0,2,1,3],[0,0,0,0],[0,3,1,2],[0,3,2,1],[0,0,0,0],[0,0,0,0],[0,0,0,0],[1,3,2,0], 
    [0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0], 
    [1,2,0,3],[0,0,0,0],[1,3,0,2],[0,0,0,0],[0,0,0,0],[0,0,0,0],[2,3,0,1],[2,3,1,0], 
    [1,0,2,3],[1,0,3,2],[0,0,0,0],[0,0,0,0],[0,0,0,0],[2,0,3,1],[0,0,0,0],[2,1,3,0], 
    [0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0], 
    [2,0,1,3],[0,0,0,0],[0,0,0,0],[0,0,0,0],[3,0,1,2],[3,0,2,1],[0,0,0,0],[3,1,2,0], 
    [2,1,0,3],[0,0,0,0],[0,0,0,0],[0,0,0,0],[3,1,0,2],[0,0,0,0],[3,2,0,1],[3,2,1,0]]; 
};
 
SimplexNoise.prototype.dot = function(g, x, y) { 
	return g[0]*x + g[1]*y;
};
 
SimplexNoise.prototype.noise = function(xin, yin) { 
  var n0, n1, n2; // Noise contributions from the three corners 
  // Skew the input space to determine which simplex cell we're in 
  var F2 = 0.5*(Math.sqrt(3.0)-1.0); 
  var s = (xin+yin)*F2; // Hairy factor for 2D 
  var i = Math.floor(xin+s); 
  var j = Math.floor(yin+s); 
  var G2 = (3.0-Math.sqrt(3.0))/6.0; 
  var t = (i+j)*G2; 
  var X0 = i-t; // Unskew the cell origin back to (x,y) space 
  var Y0 = j-t; 
  var x0 = xin-X0; // The x,y distances from the cell origin 
  var y0 = yin-Y0; 
  // For the 2D case, the simplex shape is an equilateral triangle. 
  // Determine which simplex we are in. 
  var i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords 
  if(x0>y0) {i1=1; j1=0;} // lower triangle, XY order: (0,0)->(1,0)->(1,1) 
  else {i1=0; j1=1;}      // upper triangle, YX order: (0,0)->(0,1)->(1,1) 
  // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and 
  // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where 
  // c = (3-sqrt(3))/6 
  var x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords 
  var y1 = y0 - j1 + G2; 
  var x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords 
  var y2 = y0 - 1.0 + 2.0 * G2; 
  // Work out the hashed gradient indices of the three simplex corners 
  var ii = i & 255; 
  var jj = j & 255; 
  var gi0 = this.perm[ii+this.perm[jj]] % 12; 
  var gi1 = this.perm[ii+i1+this.perm[jj+j1]] % 12; 
  var gi2 = this.perm[ii+1+this.perm[jj+1]] % 12; 
  // Calculate the contribution from the three corners 
  var t0 = 0.5 - x0*x0-y0*y0; 
  if(t0<0) n0 = 0.0; 
  else { 
    t0 *= t0; 
    n0 = t0 * t0 * this.dot(this.grad3[gi0], x0, y0);  // (x,y) of grad3 used for 2D gradient 
  } 
  var t1 = 0.5 - x1*x1-y1*y1; 
  if(t1<0) n1 = 0.0; 
  else { 
    t1 *= t1; 
    n1 = t1 * t1 * this.dot(this.grad3[gi1], x1, y1); 
  }
  var t2 = 0.5 - x2*x2-y2*y2; 
  if(t2<0) n2 = 0.0; 
  else { 
    t2 *= t2; 
    n2 = t2 * t2 * this.dot(this.grad3[gi2], x2, y2); 
  } 
  // Add contributions from each corner to get the final noise value. 
  // The result is scaled to return values in the interval [-1,1]. 
  return 70.0 * (n0 + n1 + n2); 
};
 
// 3D simplex noise 
SimplexNoise.prototype.noise3d = function(xin, yin, zin) { 
  var n0, n1, n2, n3; // Noise contributions from the four corners 
  // Skew the input space to determine which simplex cell we're in 
  var F3 = 1.0/3.0; 
  var s = (xin+yin+zin)*F3; // Very nice and simple skew factor for 3D 
  var i = Math.floor(xin+s); 
  var j = Math.floor(yin+s); 
  var k = Math.floor(zin+s); 
  var G3 = 1.0/6.0; // Very nice and simple unskew factor, too 
  var t = (i+j+k)*G3; 
  var X0 = i-t; // Unskew the cell origin back to (x,y,z) space 
  var Y0 = j-t; 
  var Z0 = k-t; 
  var x0 = xin-X0; // The x,y,z distances from the cell origin 
  var y0 = yin-Y0; 
  var z0 = zin-Z0; 
  // For the 3D case, the simplex shape is a slightly irregular tetrahedron. 
  // Determine which simplex we are in. 
  var i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords 
  var i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords 
  if(x0>=y0) { 
    if(y0>=z0) 
      { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; } // X Y Z order 
      else if(x0>=z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; } // X Z Y order 
      else { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; } // Z X Y order 
    } 
  else { // x0<y0 
    if(y0<z0) { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; } // Z Y X order 
    else if(x0<z0) { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; } // Y Z X order 
    else { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; } // Y X Z order 
  } 
  // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z), 
  // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and 
  // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where 
  // c = 1/6.
  var x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords 
  var y1 = y0 - j1 + G3; 
  var z1 = z0 - k1 + G3; 
  var x2 = x0 - i2 + 2.0*G3; // Offsets for third corner in (x,y,z) coords 
  var y2 = y0 - j2 + 2.0*G3; 
  var z2 = z0 - k2 + 2.0*G3; 
  var x3 = x0 - 1.0 + 3.0*G3; // Offsets for last corner in (x,y,z) coords 
  var y3 = y0 - 1.0 + 3.0*G3; 
  var z3 = z0 - 1.0 + 3.0*G3; 
  // Work out the hashed gradient indices of the four simplex corners 
  var ii = i & 255; 
  var jj = j & 255; 
  var kk = k & 255; 
  var gi0 = this.perm[ii+this.perm[jj+this.perm[kk]]] % 12; 
  var gi1 = this.perm[ii+i1+this.perm[jj+j1+this.perm[kk+k1]]] % 12; 
  var gi2 = this.perm[ii+i2+this.perm[jj+j2+this.perm[kk+k2]]] % 12; 
  var gi3 = this.perm[ii+1+this.perm[jj+1+this.perm[kk+1]]] % 12; 
  // Calculate the contribution from the four corners 
  var t0 = 0.6 - x0*x0 - y0*y0 - z0*z0; 
  if(t0<0) n0 = 0.0; 
  else { 
    t0 *= t0; 
    n0 = t0 * t0 * this.dot(this.grad3[gi0], x0, y0, z0); 
  }
  var t1 = 0.6 - x1*x1 - y1*y1 - z1*z1; 
  if(t1<0) n1 = 0.0; 
  else { 
    t1 *= t1; 
    n1 = t1 * t1 * this.dot(this.grad3[gi1], x1, y1, z1); 
  } 
  var t2 = 0.6 - x2*x2 - y2*y2 - z2*z2; 
  if(t2<0) n2 = 0.0; 
  else { 
    t2 *= t2; 
    n2 = t2 * t2 * this.dot(this.grad3[gi2], x2, y2, z2); 
  } 
  var t3 = 0.6 - x3*x3 - y3*y3 - z3*z3; 
  if(t3<0) n3 = 0.0; 
  else { 
    t3 *= t3; 
    n3 = t3 * t3 * this.dot(this.grad3[gi3], x3, y3, z3); 
  } 
  // Add contributions from each corner to get the final noise value. 
  // The result is scaled to stay just inside [-1,1] 
  return 32.0*(n0 + n1 + n2 + n3); 
};

function SeededRandom(){
  var seed = 42;
  this.random = function(){
      var x = Math.sin(seed++) * 10000;
      return x - Math.floor(x);
  };
}

var perlin = new SimplexNoise(new SeededRandom());
var PERLIN_BASE_RANGE = 4;
var PERLIN_DENOMINATOR = 16384 / PERLIN_BASE_RANGE;

var PLANE_MAX_HEIGHT = 500.0;

function getHeight(x, z) {
  
  var scaledX = x / (METER * PERLIN_DENOMINATOR);
  var scaledZ = z / (METER * PERLIN_DENOMINATOR);
  
  var basePerlin = perlin.noise(scaledX, scaledZ); 
  var scaledBasePerlin = (basePerlin + 1) / 2;
  
  var secondPerlin = perlin.noise(10 * scaledX, 5 * scaledZ);
  scaledSecondPerlin = (secondPerlin + 1) / 2;
  
  var thirdPerlin = perlin.noise(30 * scaledX, 15 * scaledZ);
  scaledThirdPerlin = (thirdPerlin + 1) / 2;
  
  var combinedPerlin = scaledBasePerlin + (0.25 * scaledSecondPerlin) + (0.125 * scaledThirdPerlin);
  combinedPerlin /= 1.0 + 0.25 + 0.125;
  
  return combinedPerlin * PLANE_MAX_HEIGHT;
}

var METER = 1.0 / TREE_SCALE;

var MIN_BUILDING_SIDE_METERS = 16;
var MAX_BUILDING_SIDE_METERS = 128;

var CITY_CORNER_METERS = 0;
var CITY_SIZE_METERS = 16384;

var MIN_SECTION_SIZE_BLOCKS = 2;
var MAX_SECTION_SIZE_BLOCKS = 24;
var SECTION_COLOR_MODIFIER = 0.5;

var BUILDING_BLOCK_METERS = 8;
var BUILDING_BLOCK_SIZE = METER * BUILDING_BLOCK_METERS;

var MIN_BUILDING_SIDE_BLOCKS = MIN_BUILDING_SIDE_METERS / BUILDING_BLOCK_METERS;
var MAX_BUILDING_SIDE_BLOCKS = MAX_BUILDING_SIDE_METERS / BUILDING_BLOCK_METERS;

var BUILDING_COLORS = [
  [42, 74, 123],
  [71, 108, 152],
  [140, 140, 136],
  [40, 45, 41]
];

var WINDOW_COLOR = [250, 250, 210];

var BLACK_COLOR = [1, 1, 1];

var windows = [];
var buildingBeacons = [];
var ignoredBeacons = 0;

var MINIMUM_BEACON_HEIGHT = 400; // don't adjust by METER because we only use this in non-adjusted math


var buildingRandom = new SeededRandom();

function getRandomFloat(min, max) {
    return buildingRandom.random() * (max - min) + min;
}

function getRandomInt(min, max) {
    return Math.floor(buildingRandom.random() * (max - min + 1)) + min;
}

function getRandomOddInt(min, max) {
  var random = getRandomInt(min, max);
  
  // if this number is even then add or take off one to make it odd
  if (random % 2 === 0) {
    random +=  Math.random() < 0.5 ? -1 : 1;
  }
  
  return random;
}

function distanceBetween(point1, point2) {
  return Math.sqrt(Math.pow(point1.x - point2.x, 2) + Math.pow(point1.z - point2.z, 2)); 
}

// pick a city center inside of square we're drawing in
var cityCenter = {};
cityCenter.x = 8192;
cityCenter.z = 8192;

var BEACON_SIZE = 32 * METER;
var BEACON_PROBABILITY = 0.10;


function makeBuilding(buildingCorner, minHeight, maxHeight) {
  // we're building this thing - figure out the size
  var buildingSizeXBlocks = getRandomOddInt(MIN_BUILDING_SIDE_BLOCKS, MAX_BUILDING_SIDE_BLOCKS);
  var buildingSizeZBlocks = getRandomOddInt(MIN_BUILDING_SIDE_BLOCKS, MAX_BUILDING_SIDE_BLOCKS);
  
  // pick the height of this building
  var buildingHeightBlocks = getRandomInt(minHeight / BUILDING_BLOCK_METERS, maxHeight / BUILDING_BLOCK_METERS);
  
  // pick the size of each section (uniform)
  var sectionSizeBlocks = getRandomInt(MIN_SECTION_SIZE_BLOCKS, MAX_SECTION_SIZE_BLOCKS);
  
  // pick the base color for this building
  var buildingColor = BUILDING_COLORS[Math.floor(buildingRandom.random() * BUILDING_COLORS.length)];
  
  // get the height corner (in metres) for this building
  var buildingCornerY = getHeight(buildingCorner.x, buildingCorner.z) - 1;
  
  // enumerate each block of the building and give it the right color
  for (var x = 0; x < buildingSizeXBlocks; x++) {
    for (var z = 0; z < buildingSizeZBlocks; z ++) {
      for (var y = 0; y < buildingHeightBlocks; y++) {
        // copy the base building color
        var blockColor = [];
        blockColor[0] = buildingColor[0];
        blockColor[1] = buildingColor[1];
        blockColor[2] = buildingColor[2];
        
        // queue the voxel add command for this voxel
        var voxelPosition = {};
        voxelPosition.x = (buildingCorner.x + (x * BUILDING_BLOCK_METERS)) * METER;
        voxelPosition.y = (buildingCornerY + (y * BUILDING_BLOCK_METERS)) * METER;
        voxelPosition.z = (buildingCorner.z + (z * BUILDING_BLOCK_METERS)) * METER;
        
        // are we a section - if so color is darker and no windows
        if (y !== 0 && y !== buildingHeightBlocks - 1 && y % sectionSizeBlocks === 0) {
          blockColor[0] = Math.round(blockColor[0] * SECTION_COLOR_MODIFIER);
          blockColor[1] = Math.round(blockColor[1] * SECTION_COLOR_MODIFIER);
          blockColor[2] = Math.round(blockColor[2] * SECTION_COLOR_MODIFIER);
        } else if ((y === 0 || y == 1)
                  && (((x === 0 || x == buildingSizeXBlocks - 1) && (z !== 0 && z != buildingSizeZBlocks - 1)) 
                      || ((z === 0 || z == buildingSizeZBlocks - 1) && (x !== 0 && x != buildingSizeXBlocks -1)))) {
          // this is an edge on the bottom and not a corner
          // light it up - this is the lobby!
          blockColor = WINDOW_COLOR;
        } else if (((x === 0 || x == buildingSizeXBlocks - 1) && z % 2 !== 0) 
                    || ((z === 0 || z == buildingSizeZBlocks - 1) && x % 2 !== 0)) {
          // this is an odd block on an edge - possibly make it the window color
          var lit = Math.random() < 0.25;
          blockColor = lit ? WINDOW_COLOR : BLACK_COLOR; 
          
          windows.push([voxelPosition, lit]);
        }

        Voxels.queueDestructiveVoxelAdd(voxelPosition.x, voxelPosition.y, voxelPosition.z, 
                                        BUILDING_BLOCK_SIZE, blockColor[0], blockColor[1], blockColor[2]);
      }
    }
  }
  
  // innner city buildings might have beacons
  if (minHeight == INNER_CITY_MIN_BUILDING_HEIGHT_METERS && buildingRandom.random() < BEACON_PROBABILITY) {
    // five percent of our buildings should get beacons on them
    var beaconCorner = {};
    beaconCorner.x = buildingCorner.x + (((buildingSizeXBlocks + 1) / 2) * BUILDING_BLOCK_METERS);
    beaconCorner.z = buildingCorner.z + (((buildingSizeZBlocks + 1) / 2) * BUILDING_BLOCK_METERS);
    beaconCorner.y = buildingCornerY + (buildingHeightBlocks * BUILDING_BLOCK_METERS) + (BEACON_SIZE / (2 * METER));
    
    // only add a beacon if it would be above the minimum beacon height.
    if (beaconCorner.y > MINIMUM_BEACON_HEIGHT) {
        buildingBeacons.push([beaconCorner, 255, getRandomInt(0, 2), getRandomInt(4, 16)]);
    } else {
        ignoredBeacons++;
    }
  } 
}



var visualCallbacks = 0;

function glowBeacons() {
  for (var i = 0; i < buildingBeacons.length; i++) {
    // place the block for the beacon
    // and fade the beacon from black to red
    var beaconPosition = buildingBeacons[i][0];
    var beaconColor = { r: 0, g: 0, b: 0};
    
    if (buildingBeacons[i][2] === 0) {
      beaconColor.r = buildingBeacons[i][1];
    } else if (buildingBeacons[i][2] == 1) {
      beaconColor.g = buildingBeacons[i][1];
    } else {
      beaconColor.b = buildingBeacons[i][1];
    }
    
    Voxels.queueDestructiveVoxelAdd(beaconPosition.x * METER, beaconPosition.y * METER, beaconPosition.z * METER, 
                                    BEACON_SIZE, beaconColor.r, beaconColor.g, beaconColor.b
    );
    
    buildingBeacons[i][1] += buildingBeacons[i][3];
    if (buildingBeacons[i][1] < 1 ) {
      buildingBeacons[i][1] = 1;
      buildingBeacons[i][3] *= -1;
    } else if (buildingBeacons[i][1] > 255) {
      buildingBeacons[i][1] = 255;
      buildingBeacons[i][3] *= -1;
    }
  }  
}

// flicker lights every 100 visual callbacks
var NUM_LIGHT_FLICKER_ITERATIONS = 100;
var LIGHT_FLICKER_PROBABILITY = 0.01;

var lightsThisCycle = 0;
function cityLights() {
  lightsThisCycle = 0;
  if (visualCallbacks % NUM_LIGHT_FLICKER_ITERATIONS === 0) {
    for (var i = 0; i < windows.length; i++) {
      // check if we change the state of this window
      if (Math.random() < LIGHT_FLICKER_PROBABILITY) {
        var thisWindow = windows[i];
        
        // flicker this window to the other state
        var newColor = thisWindow[1] ? BLACK_COLOR : WINDOW_COLOR;
        Voxels.queueDestructiveVoxelAdd(thisWindow[0].x, thisWindow[0].y, thisWindow[0].z, 
                                        BUILDING_BLOCK_SIZE, newColor[0], newColor[1], newColor[2]
        );
        lightsThisCycle++;
        
        // change the state of this window in the array
        thisWindow[1] = !thisWindow[1];
        windows[i] = thisWindow;
      }
    }
  }
}

var NUM_BUILDINGS = 300;
var createdBuildings = 0;

var INNER_CITY_RADIUS = CITY_SIZE_METERS / 8;
var SUBURB_RADIUS = CITY_SIZE_METERS / 4;
var SUBURB_LEVEL = 0.4;
var OUTSKIRT_RADIUS = CITY_SIZE_METERS - (INNER_CITY_RADIUS + SUBURB_RADIUS); 
var OUTSKIRT_LEVEL = 0.05;

var INNER_CITY_MAX_BUILDING_HEIGHT_METERS = 512;
var INNER_CITY_MIN_BUILDING_HEIGHT_METERS = 64;

var SUBURB_MAX_BUILDING_HEIGHT_METERS = 64;
var SUBURB_MIN_BUILDING_HEIGHT_METERS = 16;

var OUTSKIRT_MAX_BUILDING_HEIGHT_METERS = 32;
var OUTSKIRT_MIN_BUILDING_HEIGHT_METERS = 16;

var doneBuilding = "no";
function makeBuildings() {
  if (createdBuildings < NUM_BUILDINGS) {
    var randomPlacement = buildingRandom.random();
    var buildingCorner = {x: -1, z: -1};
    
    var minRadiusPush = 0;
    var maxRadiusPush = 0;
    var minHeight = 0;
    var maxHeight = 0;
    
    // pick a corner point for a new building, loop until it is inside the city limits
    while (buildingCorner.x < 0 || buildingCorner.x > CITY_SIZE_METERS || buildingCorner.z < 0 || buildingCorner.z > CITY_SIZE_METERS) {
      if (randomPlacement < OUTSKIRT_LEVEL) {
        minRadiusPush = INNER_CITY_RADIUS + SUBURB_RADIUS;
        maxRadiusPush = CITY_SIZE_METERS;
        
        minHeight = OUTSKIRT_MIN_BUILDING_HEIGHT_METERS;
        maxHeight = OUTSKIRT_MAX_BUILDING_HEIGHT_METERS;
      } else if (randomPlacement < SUBURB_LEVEL) {
        minRadiusPush = INNER_CITY_RADIUS;
        maxRadiusPush = SUBURB_RADIUS;
        
        minHeight = SUBURB_MIN_BUILDING_HEIGHT_METERS;
        maxHeight = SUBURB_MAX_BUILDING_HEIGHT_METERS;
      } else {
        minRadiusPush = 0;
        maxRadiusPush = INNER_CITY_RADIUS;
        
        minHeight = INNER_CITY_MIN_BUILDING_HEIGHT_METERS;
        maxHeight = INNER_CITY_MAX_BUILDING_HEIGHT_METERS;
      }
      
      var radiusPush = getRandomFloat(minRadiusPush, maxRadiusPush);
      var randomAngle = getRandomFloat(0, 360);
    
      buildingCorner.x = cityCenter.x + (radiusPush * Math.cos(randomAngle));
      buildingCorner.z = cityCenter.z + (radiusPush * Math.sin(randomAngle));
    }
    
    makeBuilding(buildingCorner, minHeight, maxHeight);
    
    createdBuildings++;
  } else {
    doneBuilding = "yes";
    glowBeacons();
    cityLights();
  }
  
  visualCallbacks++;
  var wantDebug = false;
  if (wantDebug){
    print("Voxel Stats: " + Voxels.getLifetimeInSeconds() + " seconds," + 
        " visualCallbacks:" + visualCallbacks +
        " doneBuilding:" + doneBuilding + 
        " buildingBeacons.length:" + buildingBeacons.length +
        " ignored beacons:" + ignoredBeacons +
        " lightsThisCycle:" + lightsThisCycle + 
        " Queued packets:" + Voxels.getLifetimePacketsQueued() + "," +
        " PPS:" + Voxels.getLifetimePPSQueued() + "," +
        //" BPS:" + Voxels.getLifetimeBPSQueued() + "," +
        " Sent packets:" + Voxels.getLifetimePacketsSent() + "," +
        " PPS:" + Voxels.getLifetimePPS() + "," +
        //" BPS:" + Voxels.getLifetimeBPS() + 
        "\n");
  }

}

Voxels.setPacketsPerSecond(500);

// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(makeBuildings);