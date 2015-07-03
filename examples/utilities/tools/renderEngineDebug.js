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

panel.newSlider("Num Feed Opaques", 0, 1000, 
    function(value) { },
    function() { return Scene.getEngineNumFeedOpaqueItems(); },
    function(value) { return (value); }
);

panel.newSlider("Num Drawn Opaques", 0, 1000, 
    function(value) { },
    function() { return Scene.getEngineNumDrawnOpaqueItems(); },
    function(value) { return (value); }
);

panel.newSlider("Max Drawn Opaques", -1, 1000, 
    function(value) { Scene.setEngineMaxDrawnOpaqueItems(value); },
    function() { return Scene.getEngineMaxDrawnOpaqueItems(); },
    function(value) { return (value); }
);

panel.newSlider("Num Feed Transparents", 0, 100, 
    function(value) { },
    function() { return Scene.getEngineNumFeedTransparentItems(); },
    function(value) { return (value); }
);

panel.newSlider("Num Drawn Transparents", 0, 100, 
    function(value) { },
    function() { return Scene.getEngineNumDrawnTransparentItems(); },
    function(value) { return (value); }
);

panel.newSlider("Max Drawn Transparents", -1, 100, 
    function(value) { Scene.setEngineMaxDrawnTransparentItems(value); },
    function() { return Scene.getEngineMaxDrawnTransparentItems(); },
    function(value) { return (value); }
);

panel.newSlider("Num Feed Overlay3Ds", 0, 100, 
    function(value) { },
    function() { return Scene.getEngineNumFeedOverlay3DItems(); },
    function(value) { return (value); }
);

panel.newSlider("Num Drawn Overlay3Ds", 0, 100, 
    function(value) { },
    function() { return Scene.getEngineNumDrawnOverlay3DItems(); },
    function(value) { return (value); }
);

panel.newSlider("Max Drawn Overlay3Ds", -1, 100, 
    function(value) { Scene.setEngineMaxDrawnOverlay3DItems(value); },
    function() { return Scene.getEngineMaxDrawnOverlay3DItems(); },
    function(value) { return (value); }
);

panel.newCheckbox("Display status",  
    function(value) { Scene.setEngineDisplayItemStatus(value); },
    function() { return Scene.doEngineDisplayItemStatus(); },
    function(value) { return (value); }
);

var tickTackPeriod = 500;

function updateCounters() {
    var numFeedOpaques = panel.get("Num Feed Opaques");
    var numFeedTransparents = panel.get("Num Feed Transparents");
    var numFeedOverlay3Ds = panel.get("Num Feed Overlay3Ds");

    panel.set("Num Feed Opaques", numFeedOpaques);
    panel.set("Num Drawn Opaques", panel.get("Num Drawn Opaques"));
    panel.set("Num Feed Transparents", numFeedTransparents);
    panel.set("Num Drawn Transparents", panel.get("Num Drawn Transparents"));
    panel.set("Num Feed Overlay3Ds", numFeedOverlay3Ds);
    panel.set("Num Drawn Overlay3Ds", panel.get("Num Drawn Overlay3Ds"));        

    var numMax = Math.max(numFeedOpaques * 1.2, 1);
    panel.getWidget("Num Feed Opaques").setMaxValue(numMax);
    panel.getWidget("Num Drawn Opaques").setMaxValue(numMax);
    panel.getWidget("Max Drawn Opaques").setMaxValue(numMax);

    numMax = Math.max(numFeedTransparents * 1.2, 1);
    panel.getWidget("Num Feed Transparents").setMaxValue(numMax);
    panel.getWidget("Num Drawn Transparents").setMaxValue(numMax);
    panel.getWidget("Max Drawn Transparents").setMaxValue(numMax);        

    numMax = Math.max(numFeedOverlay3Ds * 1.2, 1);
    panel.getWidget("Num Feed Overlay3Ds").setMaxValue(numMax);
    panel.getWidget("Num Drawn Overlay3Ds").setMaxValue(numMax);
    panel.getWidget("Max Drawn Overlay3Ds").setMaxValue(numMax);
}
Script.setInterval(updateCounters, tickTackPeriod);

Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) { return panel.mouseMoveEvent(event); });
Controller.mousePressEvent.connect( function panelMousePressEvent(event) { return panel.mousePressEvent(event); });
Controller.mouseReleaseEvent.connect(function(event) { return panel.mouseReleaseEvent(event); });

function scriptEnding() {
    panel.destroy();
}
Script.scriptEnding.connect(scriptEnding);
