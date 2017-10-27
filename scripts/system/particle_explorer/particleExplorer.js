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
/* global HifiEntityUI, openEventBridge, console, EventBridge, document, window */
/* eslint no-console: 0, no-global-assign: 0 */

(function () {

    var root = document.getElementById("particle-explorer");

    window.onload = function () {
        var ui = new HifiEntityUI(root);
        var textarea = document.createElement("textarea");
        var properties = "";
        var menuStructure = {
            General: [
                {
                    type: "Row",
                    id: "export-import-field"
                },
                {
                    id: "show-properties-button",
                    name: "Show Properties",
                    type: "Button",
                    class: "blue",
                    disabled: true,
                    callback: function (event) {
                        var insertZone = document.getElementById("export-import-field");
                        var json = ui.getSettings();
                        properties = JSON.stringify(json);
                        textarea.value = properties;
                        if (!insertZone.contains(textarea)) {
                            insertZone.appendChild(textarea);
                            insertZone.parentNode.parentNode.style.maxHeight =
                                insertZone.parentNode.clientHeight + "px";
                            document.getElementById("export-properties-button").removeAttribute("disabled");
                            textarea.onchange = function (e) {
                                if (e.target.value !== properties) {
                                    document.getElementById("import-properties-button").removeAttribute("disabled");
                                }
                            };
                            textarea.oninput = textarea.onchange;
                        } else {
                            textarea.onchange = function () {};
                            textarea.oninput = textarea.onchange;
                            textarea.value = "";
                            textarea.remove();
                            insertZone.parentNode.parentNode.style.maxHeight =
                                insertZone.parentNode.clientHeight + "px";
                            document.getElementById("export-properties-button").setAttribute("disabled", true);
                            document.getElementById("import-properties-button").setAttribute("disabled", true);
                        }
                    }
                },
                {
                    id: "import-properties-button",
                    name: "Import",
                    type: "Button",
                    class: "blue",
                    disabled: true,
                    callback: function (event) {
                        ui.fillFields(JSON.parse(textarea.value));
                        ui.submitChanges(JSON.parse(textarea.value));
                    }
                },
                {
                    id: "export-properties-button",
                    name: "Export",
                    type: "Button",
                    class: "red",
                    disabled: true,
                    callback: function (event) {
                        textarea.select();
                        try {
                            var success = document.execCommand('copy');
                            if (!success) {
                                throw "Not success :(";
                            }
                        } catch (e) {
                            print("couldnt copy field");
                        }
                    }
                },
                {
                    type: "Row"
                },
                {
                    id: "isEmitting",
                    name: "Is Emitting",
                    type: "Boolean"
                },
                {
                    type: "Row"
                },
                {
                    id: "lifespan",
                    name: "Lifespan",
                    type: "SliderFloat",
                    min: 0.01,
                    max: 10
                },
                {
                    type: "Row"
                },
                {
                    id: "maxParticles",
                    name: "Max Particles",
                    type: "SliderInteger",
                    min: 1,
                    max: 10000
                },
                {
                    type: "Row"
                },
                {
                    id: "textures",
                    name: "Textures",
                    type: "Texture"
                },
                {
                    type: "Row"
                }
            ],
            Emit: [
                {
                    id: "emitRate",
                    name: "Emit Rate",
                    type: "SliderInteger",
                    max: 1000,
                    min: 1
                },
                {
                    type: "Row"
                },
                {
                    id: "emitSpeed",
                    name: "Emit Speed",
                    type: "SliderFloat",
                    max: 5
                },
                {
                    type: "Row"
                },
                {
                    id: "emitDimensions",
                    name: "Emit Dimension",
                    type: "Vector",
                    defaultRange: {
                        min: 0,
                        step: 0.01
                    }
                },
                {
                    type: "Row"
                },
                {
                    id: "emitOrientation",
                    unit: "deg",
                    name: "Emit Orientation",
                    type: "VectorQuaternion",
                    defaultRange: {
                        min: 0,
                        step: 0.01
                    }
                },
                {
                    type: "Row"
                },
                {
                    id: "emitterShouldTrail",
                    name: "Emitter Should Trail",
                    type: "Boolean"
                },
                {
                    type: "Row"
                }
            ],
            Radius: [
                {
                    id: "particleRadius",
                    name: "Particle Radius",
                    type: "SliderFloat",
                    max: 4.0
                },
                {
                    type: "Row"
                },
                {
                    id: "radiusSpread",
                    name: "Radius Spread",
                    type: "SliderFloat",
                    max: 4.0
                },
                {
                    type: "Row"
                },
                {
                    id: "radiusStart",
                    name: "Radius Start",
                    type: "SliderFloat",
                    max: 4.0
                },
                {
                    type: "Row"
                },
                {
                    id: "radiusFinish",
                    name: "Radius Finish",
                    type: "SliderFloat",
                    max: 4.0
                },
                {
                    type: "Row"
                }
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
                {
                    type: "Row"
                },
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
                {
                    type: "Row"
                },
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
                {
                    type: "Row"
                },
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
                {
                    type: "Row"
                }
            ],
            Acceleration: [
                {
                    id: "emitAcceleration",
                    name: "Emit Acceleration",
                    type: "Vector",
                    defaultRange: {
                        step: 0.01
                    }
                },
                {
                    type: "Row"
                },
                {
                    id: "accelerationSpread",
                    name: "Acceleration Spread",
                    type: "Vector",
                    defaultRange: {
                        step: 0.01
                    }
                },
                {
                    type: "Row"
                }
            ],
            Alpha: [
                {
                    id: "alpha",
                    name: "Alpha",
                    type: "SliderFloat"
                },
                {
                    type: "Row"
                },
                {
                    id: "alphaSpread",
                    name: "Alpha Spread",
                    type: "SliderFloat"
                },
                {
                    type: "Row"
                },
                {
                    id: "alphaStart",
                    name: "Alpha Start",
                    type: "SliderFloat"
                },
                {
                    type: "Row"
                },
                {
                    id: "alphaFinish",
                    name: "Alpha Finish",
                    type: "SliderFloat"
                },
                {
                    type: "Row"
                }
            ],
            Polar: [
                {
                    id: "polarStart",
                    name: "Polar Start",
                    unit: "deg",
                    type: "SliderRadian"
                },
                {
                    type: "Row"
                },
                {
                    id: "polarFinish",
                    name: "Polar Finish",
                    unit: "deg",
                    type: "SliderRadian"
                },
                {
                    type: "Row"
                }
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
                {
                    type: "Row"
                },
                {
                    id: "azimuthFinish",
                    name: "Azimuth Finish",
                    unit: "deg",
                    type: "SliderRadian"
                },
                {
                    type: "Row"
                }
            ]
        };
        ui.setUI(menuStructure);
        ui.setOnSelect(function () {
            document.getElementById("show-properties-button").removeAttribute("disabled");
            document.getElementById("export-properties-button").setAttribute("disabled", true);
            document.getElementById("import-properties-button").setAttribute("disabled", true);
        });
        ui.build();
        var overrideLoad = false;
        if (openEventBridge === undefined) {
            overrideLoad = true,
                openEventBridge = function (callback) {
                    callback({
                        emitWebEvent: function () {},
                        submitChanges: function () {},
                        scriptEventReceived: {
                            connect: function () {

                            }
                        }
                    });
                };
        }
        openEventBridge(function (EventBridge) {
            ui.connect(EventBridge);
        });
        if (overrideLoad) {
            openEventBridge();
        }
    };
})();
