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

var panel = new Panel(10, 800);

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

var tickTackPeriod = 500;

function updateCounters() {
        panel.set("Num Feed Opaques", panel.get("Num Feed Opaques"));
        panel.set("Num Drawn Opaques", panel.get("Num Drawn Opaques"));
        panel.set("Num Feed Transparents", panel.get("Num Feed Transparents"));
        panel.set("Num Drawn Transparents", panel.get("Num Drawn Transparents"));
        panel.set("Num Feed Overlay3Ds", panel.get("Num Feed Overlay3Ds"));
        panel.set("Num Drawn Overlay3Ds", panel.get("Num Drawn Overlay3Ds"));        
}
Script.setInterval(updateCounters, tickTackPeriod);

Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) { return panel.mouseMoveEvent(event); });
Controller.mousePressEvent.connect( function panelMousePressEvent(event) { return panel.mousePressEvent(event); });
Controller.mouseReleaseEvent.connect(function(event) { return panel.mouseReleaseEvent(event); });

function scriptEnding() {
    panel.destroy();
}
Script.scriptEnding.connect(scriptEnding);
