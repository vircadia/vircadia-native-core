//  makePlanets.js
//
//  Created by Philip Rosedale on March 29, 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Make an earth and moon, where you can grab and throw moon into orbit.  Entity
//  script attached to moon gives it gravitation behavior and will also make it attracted to
//  other spheres placed nearby.
//

var SCALE = 3.0;
var EARTH_SIZE = 3.959 / SCALE;
var MOON_SIZE = 1.079 / SCALE;

var BUTTON_SIZE = 32;
var PADDING = 3;

var earth = null;
var moon = null;

var SCRIPT_URL = Script.resolvePath("gravity.js");

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
Script.include(["/~/system/libraries/toolBars.js"]);
var toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL, "highfidelity.makePlanets.js");

var makePlanetsIconURL = Script.resolvePath("gravity.svg");
var button = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: makePlanetsIconURL,
    color: {
        red: 255,
        green: 255,
        blue: 255
    },
    alpha: 1
});

var deleteButton = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: HIFI_PUBLIC_BUCKET + "images/delete.png",
    color: {
        red: 255,
        green: 255,
        blue: 255
    },
    alpha: 1
});

function inFrontOfMe(distance) {
    return Vec3.sum(Camera.getPosition(), Vec3.multiply(distance, Quat.getFront(Camera.getOrientation())));
}

function onButtonClick() {
    earth = Entities.addEntity({
        type: "Model",
        name: "Earth",
        modelURL: "https://s3-us-west-1.amazonaws.com/hifi-content/seth/production/NBody/earth.fbx",
        position: inFrontOfMe(2 * EARTH_SIZE),
        dimensions: { x: EARTH_SIZE, y: EARTH_SIZE, z: EARTH_SIZE },
        shapeType: "sphere",
        lifetime: 86400, // 1 day
        angularDamping: 0,
        angularVelocity: { x: 0, y: 0.1, z: 0 },
    });
    moon = Entities.addEntity({
        type: "Model",
        name: "Moon",
        modelURL: "https://s3-us-west-1.amazonaws.com/hifi-content/seth/production/NBody/moon.fbx",
        position: inFrontOfMe(EARTH_SIZE - MOON_SIZE),
        dimensions: { x: MOON_SIZE, y: MOON_SIZE, z: MOON_SIZE },
        dynamic: true,
        damping: 0, // 0.01,
        angularDamping: 0, // 0.01,
        script: SCRIPT_URL,
        shapeType: "sphere"
    });
    Entities.addEntity({
        "accelerationSpread": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "alpha": 1,
        "alphaFinish": 0,
        "alphaStart": 1,
        "azimuthFinish": 0,
        "azimuthStart": 0,
        "color": {
            "blue": 255,
            "green": 255,
            "red": 255
        },
        "colorFinish": {
            "blue": 255,
            "green": 255,
            "red": 255
        },
        "colorStart": {
            "blue": 255,
            "green": 255,
            "red": 255
        },
        "dimensions": {
            "x": 0.10890001058578491,
            "y": 0.10890001058578491,
            "z": 0.10890001058578491
        },
        "emitAcceleration": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "emitOrientation": {
            "w": 0.99999994039535522,
            "x": 0,
            "y": 0,
            "z": 0
        },
        "emitRate": 300,
        "emitSpeed": 0,
        "emitterShouldTrail": 1,
        "maxParticles": 10000,
        "name": "moon trail",
        "parentID": moon,
        "particleRadius": 0.005,
        "radiusFinish": 0.005,
        "radiusSpread": 0.005,
        "radiusStart": 0.005,
        "speedSpread": 0,
        "lifespan": 20,
        "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
        "type": "ParticleEffect",
        "userData": "{\"grabbableKey\":{\"grabbable\":false}}"
    });
}

function onDeleteButton() {
	Entities.deleteEntity(earth);
	Entities.deleteEntity(moon);
}

function mousePressEvent(event) {
  var clickedText = false;
  var clickedOverlay = Overlays.getOverlayAtPoint({
    x: event.x,
    y: event.y
  });
  if (clickedOverlay == button) {
    onButtonClick();
  } else if (clickedOverlay == deleteButton) {
  	onDeleteButton();
  }
}

function scriptEnding() {
  toolBar.cleanup();
}

Controller.mousePressEvent.connect(mousePressEvent);
Script.scriptEnding.connect(scriptEnding);
