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
var ACTIONS = ["Off", "Diffuse", "Alpha", "Specular", "Roughness", "Normal", "Depth", "Lighting", "Custom"];
var SETTINGS_KEY = "EngineDebugScript.DebugMode";

Number.prototype.clamp = function(min, max) {
    return Math.min(Math.max(this, min), max);
};

var panel = new Panel(10, 100);

function CounterWidget(parentPanel, name, feedGetter, drawGetter, capSetter, capGetter) {
    this.subPanel = panel.newSubPanel(name);

    this.subPanel.newSlider("Num Feed", 0, 1, 
        function(value) { },
        feedGetter,
        function(value) { return (value); });
    this.subPanel.newSlider("Num Drawn", 0, 1, 
        function(value) { },
        drawGetter,
        function(value) { return (value); });
    this.subPanel.newSlider("Max Drawn", -1, 1, 
        capSetter,
        capGetter,
        function(value) { return (value); });

   this.update = function () {
        var numFeed = this.subPanel.get("Num Feed");
        this.subPanel.set("Num Feed", numFeed);
        this.subPanel.set("Num Drawn", this.subPanel.get("Num Drawn"));

        var numMax = Math.max(numFeed, 1);
        this.subPanel.getWidget("Num Feed").setMaxValue(numMax);
        this.subPanel.getWidget("Num Drawn").setMaxValue(numMax);
        this.subPanel.getWidget("Max Drawn").setMaxValue(numMax);
    };
};

var opaquesCounter = new CounterWidget(panel, "Opaques",
    function () { return Scene.getEngineNumFeedOpaqueItems(); },
    function () { return Scene.getEngineNumDrawnOpaqueItems(); },
    function(value) {    Scene.setEngineMaxDrawnOpaqueItems(value); },
    function () { return Scene.getEngineMaxDrawnOpaqueItems(); }
);

var transparentsCounter = new CounterWidget(panel, "Transparents",
    function () { return Scene.getEngineNumFeedTransparentItems(); },
    function () { return Scene.getEngineNumDrawnTransparentItems(); },
    function(value) {    Scene.setEngineMaxDrawnTransparentItems(value); },
    function () { return Scene.getEngineMaxDrawnTransparentItems(); }
);

var overlaysCounter = new CounterWidget(panel, "Overlays",
    function () { return Scene.getEngineNumFeedOverlay3DItems(); },
    function () { return Scene.getEngineNumDrawnOverlay3DItems(); },
    function(value) {    Scene.setEngineMaxDrawnOverlay3DItems(value); },
    function () { return Scene.getEngineMaxDrawnOverlay3DItems(); }
);

var resizing = false;
var previousMode = Settings.getValue(SETTINGS_KEY, -1);
Menu.addActionGroup(MENU, ACTIONS, ACTIONS[previousMode + 1]);
Scene.setEngineDeferredDebugMode(previousMode);
Scene.setEngineDeferredDebugSize({ x: 0.0, y: -1.0, z: 1.0, w: 1.0 }); // Reset to default size

function setEngineDeferredDebugSize(eventX) {
    var scaledX = (2.0 * (eventX / Window.innerWidth) - 1.0).clamp(-1.0, 1.0);
    Scene.setEngineDeferredDebugSize({ x: scaledX, y: -1.0, z: 1.0, w: 1.0 });
}
function shouldStartResizing(eventX) {
    var x = Math.abs(eventX - Window.innerWidth * (1.0 + Scene.getEngineDeferredDebugSize().x) / 2.0);
    var mode = Scene.getEngineDeferredDebugMode();
    return mode !== -1 && x < 20;
}

function menuItemEvent(menuItem) {
    var index = ACTIONS.indexOf(menuItem);
    if (index >= 0) {
        Scene.setEngineDeferredDebugMode(index - 1);
    }
}

// see libraries/render/src/render/Engine.h
var showDisplayStatusFlag = 1;
var showNetworkStatusFlag = 2;

panel.newCheckbox("Display status",
    function(value) { Scene.setEngineDisplayItemStatus(value ?
                                                       Scene.doEngineDisplayItemStatus() | showDisplayStatusFlag :
                                                       Scene.doEngineDisplayItemStatus() & ~showDisplayStatusFlag); },
    function() { return (Scene.doEngineDisplayItemStatus() & showDisplayStatusFlag) > 0; },
    function(value) { return (value & showDisplayStatusFlag) > 0; }
);

panel.newCheckbox("Network/Physics status",
    function(value) { Scene.setEngineDisplayItemStatus(value ?
                                                       Scene.doEngineDisplayItemStatus() | showNetworkStatusFlag :
                                                       Scene.doEngineDisplayItemStatus() & ~showNetworkStatusFlag); },
    function() { return (Scene.doEngineDisplayItemStatus() & showNetworkStatusFlag) > 0; },
    function(value) { return (value & showNetworkStatusFlag) > 0; }
);

panel.newSlider("Tone Mapping Exposure", -10, 10,
    function (value) { Scene.setEngineToneMappingExposure(value); },
    function() { return Scene.getEngineToneMappingExposure(); },
    function (value) { return (value); });

var tickTackPeriod = 500;

function updateCounters() {
    opaquesCounter.update();
    transparentsCounter.update();
    overlaysCounter.update();
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
    Settings.setValue(SETTINGS_KEY, Scene.getEngineDeferredDebugMode());
    Scene.setEngineDeferredDebugMode(-1);
    Scene.setEngineDeferredDebugSize({ x: 0.0, y: -1.0, z: 1.0, w: 1.0 }); // Reset to default size
}
Script.scriptEnding.connect(scriptEnding);


// Collapse items
panel.mousePressEvent({ x: panel.x, y: panel.items["Overlays"].y});
panel.mousePressEvent({ x: panel.x, y: panel.items["Transparents"].y});
panel.mousePressEvent({ x: panel.x, y: panel.items["Opaques"].y});
