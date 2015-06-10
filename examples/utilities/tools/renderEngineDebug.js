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

var panel = new Panel(10, 400);

panel.newCheckbox("Enable Cull Opaque", 
    function(value) { Scene.setEngineCullOpaque((value != 0)); },
    function() { return Scene.doEngineCullOpaque(); },
    function(value) { return (value); }
);

panel.newCheckbox("Enable Sort Opaque", 
    function(value) { Scene.setEngineSortOpaque((value != 0)); },
    function() { return Scene.doEngineSortOpaque(); },
    function(value) { return (value); }
);

panel.newCheckbox("Enable Render Opaque", 
    function(value) { Scene.setEngineRenderOpaque((value != 0)); },
    function() { return Scene.doEngineRenderOpaque(); },
    function(value) { return (value); }
);

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

panel.newCheckbox("Enable Cull Transparent", 
    function(value) { Scene.setEngineCullTransparent((value != 0)); },
    function() { return Scene.doEngineCullTransparent(); },
    function(value) { return (value); }
);

panel.newCheckbox("Enable Sort Transparent", 
    function(value) { Scene.setEngineSortTransparent((value != 0)); },
    function() { return Scene.doEngineSortTransparent(); },
    function(value) { return (value); }
);

panel.newCheckbox("Enable Render Transparent", 
    function(value) { Scene.setEngineRenderTransparent((value != 0)); },
    function() { return Scene.doEngineRenderTransparent(); },
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

var tickTackPeriod = 500;

function updateCounters() {
        panel.set("Num Feed Opaques", panel.get("Num Feed Opaques"));
        panel.set("Num Drawn Opaques", panel.get("Num Drawn Opaques"));
        panel.set("Num Feed Transparents", panel.get("Num Feed Transparents"));
        panel.set("Num Drawn Transparents", panel.get("Num Drawn Transparents"));
}
Script.setInterval(updateCounters, tickTackPeriod);

Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) { return panel.mouseMoveEvent(event); });
Controller.mousePressEvent.connect( function panelMousePressEvent(event) { return panel.mousePressEvent(event); });
Controller.mouseReleaseEvent.connect(function(event) { return panel.mouseReleaseEvent(event); });

function scriptEnding() {
    panel.destroy();
}
Script.scriptEnding.connect(scriptEnding);
