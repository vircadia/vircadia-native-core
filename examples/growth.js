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
var LIFETIME = 60; // only keep these entities around for a minute

var SIZE = 1; // starting size
var SIZE_MAX = 3;
var SIZE_MIN = 0.5;

var center = MyAvatar.position;
var positions = [
    Vec3.sum(center, Vec3.multiply(RADIUS, Vec3.FRONT)),
    Vec3.sum(center, Vec3.multiply(-RADIUS, Vec3.FRONT)),
    Vec3.sum(center, Vec3.multiply(RADIUS, Vec3.RIGHT)),
    Vec3.sum(center, Vec3.multiply(-RADIUS, Vec3.RIGHT)),
];

// Create spheres around the avatar
var spheres = map(positions, getSphere);

// Toggle sphere growth
each(spheres, function(sphere) {
    var state = {
        size: SIZE,
        grow: true, // grow/shrink
        state: false, // mutate/pause
    };

    // Grow or shrink on click
    Script.addEventHandler(sphere, 'clickReleaseOnEntity', toggleGrowth);
    // TODO: Toggle on collisions as well

    function toggleGrowth() {
        if (state.state) {
            stop();
        } else {
            start();
        }
    }

    function start() {
        var verb = state.grow ? 'grow' : 'shrink';
        print('starting to '+verb+' '+sphere);

        Script.update.connect(grow);
        state.state = true;
    }

    function stop() {
        print('stopping '+sphere);

        Script.update.disconnect(grow);
        state.state = false;
        state.grow = !state.grow;
    }

    function grow(delta) {
        if ((state.grow && state.size > SIZE_MAX) || // cannot grow more
            (!state.grow && state.size < SIZE_MIN)) { // cannot shrink more
            stop();
            return;
        }

        state.size += (state.grow ? 1 : -1) * delta; // grow/shrink
        Entities.editEntity(sphere, { dimensions: getDimensions(state.size) });
    }
});

Script.scriptEnding.connect(function() { // cleanup
    each(spheres, function(sphere) {
        Entities.deleteEntity(sphere);
    });
});

// NOTE: Everything below this line is a helper.
//       The basic logic of this example is entirely above this note.

// Entity helpers

function getSphere(position) {
    return Entities.addEntity({
        type: 'Sphere',
        position: position,
        dimensions: getDimensions(SIZE),
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
