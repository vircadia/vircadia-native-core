//
//  particleExplorer.js
//
//  Created by James B. Pollack @imgntn on 9/26/2015
//  Copyright 2017 High Fidelity, Inc.
//
//  Reworked by Menithal on 20/5/2017
//
//  Web app side of the App - contains GUI.
//  This is an example of a new, easy way to do two way bindings between dynamically created GUI and in-world entities.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* global HifiEntityUI, openEventBridge, EventBridge, document, window */

(function(){
    var menuStructure = {
        General: [
            {
                id: "importSettings",
                name: "Import Settings",
                type: "Button",
                class: "blue",
                callback: function(){}
            },
            {
                id: "exportSettings",
                name: "Export Settings",
                type: "Button",
                class: "red",
                callback: function(){}
            },
            { type: "Row" },
            {
                id: "isEmitting",
                name: "Is Emitting",
                type: "Boolean"
            },
            { type: "Row" },
            {
                id: "lifespan",
                name: "Lifespan",
                type: "SliderFloat",
                min: 0.01,
                max: 10
            },
            { type: "Row" },
            {
                id: "maxParticles",
                name: "Max Particles",
                type: "SliderInteger",
                min: 1,
                max: 10000
            },
            { type: "Row" },
            {
                id: "textures",
                name: "Textures",
                type: "Texture"
            },
            { type: "Row" }
        ],
        Emit: [
            {
                id: "emitRate",
                name: "Emit Rate",
                type: "SliderInteger",
                max: 1000
            },
            { type: "Row" },
            {
                id: "emitSpeed",
                name: "Emit Speed",
                type: "SliderFloat",
                max: 5
            },

            { type: "Row" },
            {
                id: "emitOrientation",
                unit: "deg",
                name: "Emit Orientation",
                type: "VectorQuaternion"
            },
            { type: "Row" },
            {
                id: "emitShouldTrail",
                name: "Emit Should Trail",
                type: "Boolean"
            },
            { type: "Row" }
        ],
        Color: [
            {
                id: "color",
                name: "Color",
                type: "Color",
                defaultColor: {
                    red: 255,
                    green: 255,
                    blue: 255
                }
            },
            { type: "Row" },
            {
                id: "colorSpread",
                name: "Color Spread",
                type: "Color",
                defaultColor: {
                    red: 0,
                    green: 0,
                    blue: 0
                }
            },
            { type: "Row" },
            {
                id: "colorStart",
                name: "Color Start",
                type: "Color",
                defaultColor: {
                    red: 255,
                    green: 255,
                    blue: 255
                }
            },
            { type: "Row" },
            {
                id: "colorFinish",
                name: "Color Finish",
                type: "Color",
                defaultColor: {
                    red: 255,
                    green: 255,
                    blue: 255
                }
            },
            { type: "Row" }
        ],
        Acceleration: [
            {
                id: "emitAcceleration",
                name: "Emit Acceleration",
                type: "Vector"
            },
            { type: "Row" },
            {
                id: "accelerationSpread",
                name: "Acceleration Spread",
                type: "Vector"
            },
            { type: "Row" }
        ],
        Alpha: [
            {
                id: "alpha",
                name: "Alpha",
                type: "SliderFloat"
            },
            { type: "Row" },
            {
                id: "alphaSpread",
                name: "Alpha Spread",
                type: "SliderFloat"
            },
            { type: "Row" },
            {
                id: "alphaStart",
                name: "Alpha Start",
                type: "SliderFloat"
            },
            { type: "Row" },
            {
                id: "alphaFinish",
                name: "Alpha Finish",
                type: "SliderFloat"
            },
            { type: "Row" }
        ],
        Polar: [
            {
                id: "polarStart",
                name: "Polar Start",
                unit: "deg",
                type: "SliderRadian"
            },
            { type: "Row" },
            {
                id: "polarFinish",
                name: "Polar Finish",
                unit: "deg",
                type: "SliderRadian"
            },
            { type: "Row" }
        ],
        Azimuth: [
            {
                id: "azimuthStart",
                name: "Azimuth Start",
                unit: "deg",
                type: "SliderRadian",
                min: -180,
                max: 0
            },
            { type: "Row" },
            {
                id: "azimuthFinish",
                name: "Azimuth Finish",
                unit: "deg",
                type: "SliderRadian"
            },
            { type: "Row" }
        ],
        Radius: [
            {
                id: "particleRadius",
                name: "Particle Radius",
                type: "SliderFloat",
                max: 4.0
            },
            { type: "Row" },
            {
                id: "radiusSpread",
                name: "Radius Spread",
                type: "SliderFloat",
                max: 4.0
            },
            { type: "Row" },
            {
                id: "radiusStart",
                name: "Radius Start",
                type: "SliderFloat",
                max: 4.0
            },
            { type: "Row" },
            {
                id: "radiusFinish",
                name: "Radius Finish",
                type: "SliderFloat",
                max: 4.0
            },
            { type: "Row" }
        ]
    };
    // Web Bridge Binding here!

    var root = document.getElementById("particle-explorer");

    window.onload = function(){
        var ui = new HifiEntityUI(root, menuStructure);
        openEventBridge( function(EventBridge) {
            ui.build();
            ui.connect(EventBridge);
        });
    };
})();
