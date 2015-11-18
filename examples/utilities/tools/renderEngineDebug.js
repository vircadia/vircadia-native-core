//
//  SunLightExample.js
//  examples
//  Sam Gateau
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("cookies.js");

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

var tickTackPeriod = 500;

function updateCounters() {
    opaquesCounter.update();
    transparentsCounter.update();
    overlaysCounter.update();
}
Script.setInterval(updateCounters, tickTackPeriod);

Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) { return panel.mouseMoveEvent(event); });
Controller.mousePressEvent.connect( function panelMousePressEvent(event) { return panel.mousePressEvent(event); });
Controller.mouseReleaseEvent.connect(function(event) { return panel.mouseReleaseEvent(event); });

function scriptEnding() {
    panel.destroy();
}
Script.scriptEnding.connect(scriptEnding);
