//
//  growth.js
//
//  Created by Zachary Pomerantz 11/2/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var RADIUS = 5;
var LIFETIME = 6000;

var size = 1;
var center = MyAvatar.position;
var positions = [
    Vec3.sum(center, Vec3.multiply(RADIUS, Vec3.FRONT)),
    Vec3.sum(center, Vec3.multiply(-RADIUS, Vec3.FRONT)),
    Vec3.sum(center, Vec3.multiply(RADIUS, Vec3.RIGHT)),
    Vec3.sum(center, Vec3.multiply(-RADIUS, Vec3.RIGHT)),
    Vec3.sum(center, Vec3.multiply(RADIUS, Vec3.UP)),
    Vec3.sum(center, Vec3.multiply(-RADIUS, Vec3.UP)),
];

var spheres = map(positions, getSphere);

Script.update.connect(function(delta) {
    size += delta; // grow by 1 unit/s
    each(spheres, function(sphere) {
        Entities.editEntity(sphere, { dimensions: getDimensions(size) });
    });
});
Script.scriptEnding.connect(function() {
    each(spheres, function(sphere) {
        Entities.deleteEntity(sphere);
    });
});

// Entity helpers

function getSphere(position) {
    return Entities.addEntity({
        type: 'Sphere',
        position: position,
        dimensions: getDimensions(size),
        color: getColor(),
        gravity: Vec3.ZERO,
        lifetime: LIFETIME,
        ignoreCollisions: true,
    });
}

function getDimensions(size) {
    return { x: size, y: size, z: size };
}

function getColor() {
    return { red: getValue(), green: getValue(), blue: getValue() };
    function getValue() { return Math.floor(Math.random() * 255); }
}

// Functional helpers

function map(list, iterator) {
    var result = [];
    for (var i = 0; i < list.length; i++) {
        result.push(iterator(list[i], i, list));
    }
    return result;
}

function each(list, iterator) {
    for (var i = 0; i < list.length; i++) {
        iterator(list[i], i, list);
    }
}
