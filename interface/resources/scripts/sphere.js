//
//  sphere.js
//  interface
//
//  Created by Andrzej Kapolka on 12/17/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

function strictIndexOf(array, element) {
    for (var i = 0; i < array.length; i++) {
        if (array[i] == element) {
            return i;
        }
    }
    return -1;
}

var colorIndex;
var normalIndex;
var visitor;
var info;

var MAX_DEPTH = 4;

var sphereCenter = [ 0.5, 0.5, 0.5 ];
var sphereColor = 0xFFFF00FF;
var sphereRadius = 0.25;
var sphereRadiusSquared = sphereRadius * sphereRadius;

function lengthSquared(x, y, z) {
    return x*x + y*y + z*z;
}

function setNormal(vector) {
    if (normalIndex != -1) {
        var length = Math.sqrt(lengthSquared(vector[0], vector[1], vector[2]));
        if (length == 0.0) {
            info.attributeValues[normalIndex] = 0x007F00;
            
        } else {
            var scale = 127.0 / length;
            info.attributeValues[normalIndex] =
                (Math.floor(vector[0] * scale) & 0xFF) << 16 |
                (Math.floor(vector[1] * scale) & 0xFF) << 8 |
                Math.floor(vector[2] * scale) & 0xFF;
        }
    }
}

function guide(minimum, size, depth) {
    info.minimum = minimum;
    info.size = size;
    
    // start with a relative fast bounding volume test to find most non-intersecting states
    var maximum = [ minimum[0] + size, minimum[1] + size, minimum[2] + size ];
    if (minimum[0] >= sphereCenter[0] + sphereRadius ||
            minimum[1] >= sphereCenter[1] + sphereRadius ||
            minimum[2] >= sphereCenter[2] + sphereRadius ||
            maximum[0] <= sphereCenter[0] - sphereRadius ||
            maximum[1] <= sphereCenter[1] - sphereRadius ||
            maximum[2] <= sphereCenter[2] - sphereRadius) {
        info.isLeaf = true;
        if (colorIndex != -1) {
            info.attributeValues[colorIndex] = 0x0;
        }
        visitor.visit(info);
        return;
    } 
    
    var halfSize = size / 2;
    var center = [ minimum[0] + halfSize, minimum[1] + halfSize, minimum[2] + halfSize ];
    var vector = [ center[0] - sphereCenter[0], center[1] - sphereCenter[1], center[2] - sphereCenter[2] ];
    
    // count the number of points inside the sphere
    var inside = 0;
    if (lengthSquared(sphereCenter[0] - minimum[0], sphereCenter[1] - minimum[1], sphereCenter[2] - minimum[2]) <=
            sphereRadiusSquared) {
        inside++;
    }
    if (lengthSquared(sphereCenter[0] - maximum[0], sphereCenter[1] - minimum[1], sphereCenter[2] - minimum[2]) <=
            sphereRadiusSquared) {
        inside++;
    }
    if (lengthSquared(sphereCenter[0] - minimum[0], sphereCenter[1] - maximum[1], sphereCenter[2] - minimum[2]) <=
            sphereRadiusSquared) {
        inside++;
    }
    if (lengthSquared(sphereCenter[0] - maximum[0], sphereCenter[1] - maximum[1], sphereCenter[2] - minimum[2]) <=
            sphereRadiusSquared) {
        inside++;
    }
    if (lengthSquared(sphereCenter[0] - minimum[0], sphereCenter[1] - minimum[1], sphereCenter[2] - maximum[2]) <=
            sphereRadiusSquared) {
        inside++;
    }
    if (lengthSquared(sphereCenter[0] - maximum[0], sphereCenter[1] - minimum[1], sphereCenter[2] - maximum[2]) <=
            sphereRadiusSquared) {
        inside++;
    }
    if (lengthSquared(sphereCenter[0] - minimum[0], sphereCenter[1] - maximum[1], sphereCenter[2] - maximum[2]) <=
            sphereRadiusSquared) {
        inside++;
    }
    if (lengthSquared(sphereCenter[0] - maximum[0], sphereCenter[1] - maximum[1], sphereCenter[2] - maximum[2]) <=
            sphereRadiusSquared) {
        inside++;
    }
    
    // see if all points are in the sphere
    if (inside == 8) {
        info.isLeaf = true;
        if (colorIndex != -1) {
            info.attributeValues[colorIndex] = sphereColor;
        }
        setNormal(vector);
        visitor.visit(info);
        return;
    }
    
    // if we've reached max depth, compute alpha using a volume estimate
    if (depth == MAX_DEPTH) {
        info.isLeaf = true;
        if (inside >= 3) {
            if (colorIndex != -1) {
                info.attributeValues[colorIndex] = sphereColor;
            }
            setNormal(vector);
        
        } else {
            if (colorIndex != -1) {
                info.attributeValues[colorIndex] = 0x0;
            }
        }
        visitor.visit(info);
        return;
    }
    
    // recurse
    info.isLeaf = false;
    if (!visitor.visit(info)) {
        return;
    }
    depth += 1;
    guide(minimum, halfSize, depth);
    guide([ center[0], minimum[1], minimum[2] ], halfSize, depth);
    guide([ minimum[0], center[1], minimum[2] ], halfSize, depth);
    guide([ center[0], center[1], minimum[2] ], halfSize, depth);
    guide([ minimum[0], minimum[1], center[2] ], halfSize, depth);
    guide([ center[0], minimum[1], center[2] ], halfSize, depth);
    guide([ minimum[0], center[1], center[2] ], halfSize, depth);
    guide([ center[0], center[1], center[2] ], halfSize, depth);
}

(function(visitation) {
    var attributes = visitation.visitor.getAttributes();
    colorIndex = strictIndexOf(attributes, AttributeRegistry.colorAttribute);
    normalIndex = strictIndexOf(attributes, AttributeRegistry.normalAttribute);
    visitor = visitation.visitor;
    info = { attributeValues: new Array(attributes.length) };
    
    // have the sphere orbit the center and pulse in size
    var time = new Date().getTime();
    var ROTATE_PERIOD = 400.0;
    sphereCenter[0] = 0.5 + 0.25 * Math.cos(time / ROTATE_PERIOD); 
    sphereCenter[2] = 0.5 + 0.25 * Math.sin(time / ROTATE_PERIOD);
    var PULSE_PERIOD = 300.0;
    sphereRadius = 0.25 + 0.0625 * Math.cos(time / PULSE_PERIOD);  
    sphereRadiusSquared = sphereRadius * sphereRadius;
    
    guide(visitation.info.minimum, visitation.info.size, 0);
})
