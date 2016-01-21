//
//  renderEngineDebug.js
//  examples/utilities/tools
//
//  Sam Gateau
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("cookies.js");

var MENU = "Developer>Render>Debug Deferred Buffer";
var ACTIONS = ["Off", "Diffuse", "Metallic", "Roughness", "Normal", "Depth", "Lighting", "Shadow", "PyramidDepth", "AmbientOcclusion", "OcclusionBlurred", "Custom"];
var SETTINGS_KEY = "EngineDebugScript.DebugMode";

Number.prototype.clamp = function(min, max) {
    return Math.min(Math.max(this, min), max);
};

var panel = new Panel(10, 100);

function CounterWidget(parentPanel, name, counter) {
    var subPanel = parentPanel.newSubPanel(name);
    var widget = parentPanel.items[name];
    widget.editTitle({ width: 270 });

    subPanel.newSlider('Max Drawn', -1, 1, 
        function(value) { counter.maxDrawn = value; }, // setter
        function() { return counter.maxDrawn; }, // getter
        function(value) { return value; });

    var slider = subPanel.getWidget('Max Drawn');

    this.update = function () {
        var numDrawn = counter.numDrawn; // avoid double polling
        var numMax = Math.max(numDrawn, 1);
        var title = [
            ' ' + name,
            numDrawn + ' / ' + counter.numFeed
        ].join('\t');

        widget.editTitle({ text: title });
        slider.setMaxValue(numMax);
    };
};

var opaquesCounter = new CounterWidget(panel, "Opaques", Render.opaque);
var transparentsCounter = new CounterWidget(panel, "Transparents", Render.transparent);
var overlaysCounter = new CounterWidget(panel, "Overlays", Render.overlay3D);

var resizing = false;
var previousMode = Settings.getValue(SETTINGS_KEY, -1);
previousMode = 8;
Menu.addActionGroup(MENU, ACTIONS, ACTIONS[previousMode + 1]);
Render.deferredDebugMode = previousMode;
Render.deferredDebugSize = { x: 0.0, y: -1.0, z: 1.0, w: 1.0 }; // Reset to default size

function setEngineDeferredDebugSize(eventX) {
    var scaledX = (2.0 * (eventX / Window.innerWidth) - 1.0).clamp(-1.0, 1.0);
    Render.deferredDebugSize = { x: scaledX, y: -1.0, z: 1.0, w: 1.0 };
}
function shouldStartResizing(eventX) {
    var x = Math.abs(eventX - Window.innerWidth * (1.0 + Render.deferredDebugSize.x) / 2.0);
    var mode = Render.deferredDebugMode;
    return mode !== -1 && x < 20;
}

function menuItemEvent(menuItem) {
    var index = ACTIONS.indexOf(menuItem);
    if (index >= 0) {
        Render.deferredDebugMode = (index - 1);
    }
}

// see libraries/render/src/render/Engine.h
var showDisplayStatusFlag = 1;
var showNetworkStatusFlag = 2;

panel.newCheckbox("Display status",
    function(value) { Render.displayItemStatus = (value ?
                                                       Render.displayItemStatus | showDisplayStatusFlag :
                                                       Render.displayItemStatus & ~showDisplayStatusFlag); },
    function() { return (Render.displayItemStatus & showDisplayStatusFlag) > 0; },
    function(value) { return (value & showDisplayStatusFlag) > 0; }
);

panel.newCheckbox("Network/Physics status",
    function(value) { Render.displayItemStatus = (value ?
                                                       Render.displayItemStatus | showNetworkStatusFlag :
                                                       Render.displayItemStatus & ~showNetworkStatusFlag); },
    function() { return (Render.displayItemStatus & showNetworkStatusFlag) > 0; },
    function(value) { return (value & showNetworkStatusFlag) > 0; }
);

panel.newSlider("Tone Mapping Exposure", -10, 10,
    function (value) { Render.tone.exposure = value; },
    function() { return Render.tone.exposure; },
    function (value) { return (value); });

panel.newSlider("Ambient Occlusion Resolution Level", 0.0, 4.0,
    function (value) { Render.ambientOcclusion.resolutionLevel = value; },
    function() { return Render.ambientOcclusion.resolutionLevel; },
    function (value) { return (value); });

panel.newSlider("Ambient Occlusion Radius", 0.0, 2.0,
    function (value) { Render.ambientOcclusion.radius = value; },
    function() { return Render.ambientOcclusion.radius; },
    function (value) { return (value.toFixed(2)); });

panel.newSlider("Ambient Occlusion Level", 0.0, 1.0,
    function (value) { Render.ambientOcclusion.level = value; },
    function() { return Render.ambientOcclusion.level; },
    function (value) { return (value.toFixed(2)); });

panel.newSlider("Ambient Occlusion Num Samples", 1, 32,
    function (value) { Render.ambientOcclusion.numSamples = value; },
    function() { return Render.ambientOcclusion.numSamples; },
    function (value) { return (value); });

panel.newSlider("Ambient Occlusion Num Spiral Turns", 0.0, 30.0,
    function (value) { Render.ambientOcclusion.numSpiralTurns = value; },
    function() { return Render.ambientOcclusion.numSpiralTurns; },
    function (value) { return (value.toFixed(2)); });

panel.newCheckbox("Ambient Occlusion Dithering",
    function (value) { Render.ambientOcclusion.ditheringEnabled = value; },
    function() { return Render.ambientOcclusion.ditheringEnabled; },
    function (value) { return (value); });

panel.newSlider("Ambient Occlusion Falloff Bias", 0.0, 0.2,
    function (value) { Render.ambientOcclusion.falloffBias = value; },
    function() { return Render.ambientOcclusion.falloffBias; },
    function (value) { return (value.toFixed(2)); });

panel.newSlider("Ambient Occlusion Edge Sharpness", 0.0, 1.0,
    function (value) { Render.ambientOcclusion.edgeSharpness = value; },
    function() { return Render.ambientOcclusion.edgeSharpness; },
    function (value) { return (value.toFixed(2)); });

panel.newSlider("Ambient Occlusion Blur Radius", 0.0, 6.0,
    function (value) { Render.ambientOcclusion.blurRadius = value; },
    function() { return Render.ambientOcclusion.blurRadius; },
    function (value) { return (value); });

panel.newSlider("Ambient Occlusion Blur Deviation", 0.0, 3.0,
    function (value) { Render.ambientOcclusion.blurDeviation = value; },
    function() { return Render.ambientOcclusion.blurDeviation; },
    function (value) { return (value.toFixed(2)); });


panel.newSlider("Ambient Occlusion GPU time", 0.0, 10.0,
    function (value) {},
    function() { return Render.ambientOcclusion.gpuTime; },
    function (value) { return (value.toFixed(2) + " ms"); });


var tickTackPeriod = 500;

function updateCounters() {
    opaquesCounter.update();
    transparentsCounter.update();
    overlaysCounter.update();
    panel.update("Ambient Occlusion GPU time");
}
Script.setInterval(updateCounters, tickTackPeriod);

function mouseMoveEvent(event) {
    if (resizing) {
        setEngineDeferredDebugSize(event.x);
    } else {
        panel.mouseMoveEvent(event);
    }
}

function mousePressEvent(event) {
    if (shouldStartResizing(event.x)) {
        resizing = true;
    } else {
        panel.mousePressEvent(event);
    }
}

function mouseReleaseEvent(event) {
    if (resizing) {
        resizing = false;
    } else {
        panel.mouseReleaseEvent(event);
    }
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

Menu.menuItemEvent.connect(menuItemEvent);

function scriptEnding() {
    panel.destroy();
    Menu.removeActionGroup(MENU);
    // Reset
    Settings.setValue(SETTINGS_KEY, Render.deferredDebugMode);
    Render.deferredDebugMode = -1;
    Render.deferredDebugSize = { x: 0.0, y: -1.0, z: 1.0, w: 1.0 };
    Render.opaque.maxDrawn = -1;
    Render.transparent.maxDrawn = -1;
    Render.overlay3D.maxDrawn = -1;
}
Script.scriptEnding.connect(scriptEnding);


// Collapse items
panel.mousePressEvent({ x: panel.x, y: panel.items["Overlays"].y});
panel.mousePressEvent({ x: panel.x, y: panel.items["Transparents"].y});
panel.mousePressEvent({ x: panel.x, y: panel.items["Opaques"].y});
