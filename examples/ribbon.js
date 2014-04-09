//
//  ribbon.js
//  examples
//
//  Created by Andrzej Kapolka on 2/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function vectorMultiply(vector, scalar) {
    return [ vector[0] * scalar, vector[1] * scalar, vector[2] * scalar ];
}

function vectorAdd(firstVector, secondVector) {
    return [ firstVector[0] + secondVector[0], firstVector[1] + secondVector[1], firstVector[2] + secondVector[2] ];
}

function vectorSubtract(firstVector, secondVector) {
    return [ firstVector[0] - secondVector[0], firstVector[1] - secondVector[1], firstVector[2] - secondVector[2] ];
}

function vectorCross(firstVector, secondVector) {
    return [ firstVector[1] * secondVector[2] - firstVector[2] * secondVector[1],
        firstVector[2] * secondVector[0] - firstVector[0] * secondVector[2],
        firstVector[0] * secondVector[1] - firstVector[1] * secondVector[0] ];
}

function vectorLength(vector) {
    return Math.sqrt(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
}

function vectorNormalize(vector) {
    return vectorMultiply(vector, 1.0 / vectorLength(vector));
}

function vectorClone(vector) {
    return [ vector[0], vector[1], vector[2] ];
}

function mix(first, second, amount) {
    return first + (second - first) * amount;
}

function vectorMix(firstVector, secondVector, amount) {
    return vectorAdd(firstVector, vectorMultiply(vectorSubtract(secondVector, firstVector), amount));
}

function randomVector(minVector, maxVector) {
    return [ mix(minVector[0], maxVector[0], Math.random()), mix(minVector[1], maxVector[1], Math.random()),
        mix(minVector[2], maxVector[2], Math.random()) ];
}

function applyToLine(start, end, granularity, fn) {
    // determine the number of steps from the maximum length
    var steps = Math.max(
        Math.abs(Math.floor(start[0] / granularity) - Math.floor(end[0] / granularity)),
        Math.abs(Math.floor(start[1] / granularity) - Math.floor(end[1] / granularity)),
        Math.abs(Math.floor(start[2] / granularity) - Math.floor(end[2] / granularity))); 
    var position = vectorClone(start);
    var increment = vectorMultiply(vectorSubtract(end, start), 1.0 / steps);
    for (var i = 0; i <= steps; i++) {
        fn(granularity * Math.floor(position[0] / granularity),
           granularity * Math.floor(position[1] / granularity),
           granularity * Math.floor(position[2] / granularity),
           granularity);
        position = vectorAdd(position, increment);
    }
}

function drawLine(start, end, color, granularity) {
    applyToLine(start, end, granularity, function(x, y, z, scale) {
        Voxels.setVoxel(x, y, z, scale, color[0], color[1], color[2]);
    });
}

function eraseLine(start, end, granularity) {
    applyToLine(start, end, granularity, function(x, y, z, scale) {
        Voxels.eraseVoxel(x, y, z, scale);
    });
}

function getHueColor(hue) {
    // see http://en.wikipedia.org/wiki/HSL_and_HSV
    var hPrime = hue / 60.0;
    var x = Math.floor(255.0 * (1.0 - Math.abs(hPrime % 2.0 - 1.0)));
    if (hPrime < 1) {
        return [255, x, 0];
        
    } else if (hPrime < 2) {
        return [x, 255, 0];
        
    } else if (hPrime < 3) {
        return [0, 255, x];
        
    } else if (hPrime < 4) {
        return [0, x, 255];
        
    } else if (hPrime < 5) {
        return [x, 0, 255];
        
    } else { // hPrime < 6
        return [255, 0, x];
    }   
}

var UNIT_MIN = [-1.0, -1.0, -1.0];
var UNIT_MAX = [1.0, 1.0, 1.0];

var EPSILON = 0.00001;

var BOUNDS_MIN = [5.0, 0.0, 5.0];
var BOUNDS_MAX = [15.0, 10.0, 15.0];

var GRANULARITY = 1.0 / 16.0;

var WIDTH = 0.5;

var HISTORY_LENGTH = 300;

var stateHistory = [];
var position;
var velocity;
var hueAngle = 0;
var smoothedOffset;

function step(deltaTime) {
    if (stateHistory.length === 0) {
        // start at a random position within the bounds, with a random velocity
        position = randomVector(BOUNDS_MIN, BOUNDS_MAX);
        do {
            velocity = randomVector(UNIT_MIN, UNIT_MAX);
        } while (vectorLength(velocity) < EPSILON);
        velocity = vectorMultiply(velocity, GRANULARITY * 0.5 / vectorLength(velocity));
        smoothedOffset = [0.0, 0.0, 0.0];
    }
    
    var right = vectorCross(velocity, [0.0, 1.0, 0.0]);
    if (vectorLength(right) < EPSILON) {
        right = [1.0, 0.0, 0.0];
    } else {
        right = vectorNormalize(right);
    }
    var up = vectorNormalize(vectorCross(right, velocity));
    var ANGULAR_SPEED = 2.0;
    var radians = hueAngle * Math.PI * ANGULAR_SPEED / 180.0;
    var offset = vectorAdd(vectorMultiply(right, WIDTH * Math.cos(radians)), vectorMultiply(up, WIDTH * Math.sin(radians)));
    var OFFSET_SMOOTHING = 0.9;
    smoothedOffset = vectorMix(offset, smoothedOffset, OFFSET_SMOOTHING);
    
    var state = { start: vectorAdd(position, smoothedOffset), end: vectorSubtract(position, smoothedOffset) };
    drawLine(state.start, state.end, getHueColor(hueAngle), GRANULARITY);
    stateHistory.push(state);
    if (stateHistory.length > HISTORY_LENGTH) {
        var last = stateHistory.shift();
        eraseLine(last.start, last.end, GRANULARITY);
    }
    
    // update position, check against bounds
    position = vectorAdd(position, velocity);
    for (var i = 0; i < 3; i++) {
        if (position[i] < BOUNDS_MIN[i]) {
            velocity[i] = -velocity[i];
            position[i] += 2.0 * (BOUNDS_MIN[i] - position[i]);
        
        } else if (position[i] > BOUNDS_MAX[i]) {
            velocity[i] = -velocity[i];
            position[i] += 2.0 * (BOUNDS_MAX[i] - position[i]);
        }
    }
    var MAX_HUE_ANGLE = 360;
    hueAngle = (hueAngle + 1) % MAX_HUE_ANGLE;
}

Script.update.connect(step);
