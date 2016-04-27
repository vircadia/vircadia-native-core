//
//  SunLightExample.js
//  examples
//  Sam Gateau
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../utilities/tools/cookies.js");

var panel = new Panel(10, 400);

panel.newSlider("Origin Longitude", -180, 180,
    function(value) { Scene.setStageLocation(value, Scene.getStageLocationLatitude(), Scene.getStageLocationAltitude()); },
    function() { return Scene.getStageLocationLongitude(); },
    function(value) { return value.toFixed(0) + " deg"; }
);

panel.newSlider("Origin Latitude", -90, 90,
    function(value) { Scene.setStageLocation(Scene.getStageLocationLongitude(), value, Scene.getStageLocationAltitude()); },
    function() { return Scene.getStageLocationLatitude(); },
    function(value) { return value.toFixed(0) + " deg"; }
);

panel.newSlider("Origin Altitude", 0, 1000,
    function(value) { Scene.setStageLocation(Scene.getStageLocationLongitude(), Scene.getStageLocationLatitude(), value); },
    function() { return Scene.getStageLocationAltitude(); },
    function(value) { return (value).toFixed(0) + " km"; }
);

panel.newSlider("Year Time", 0, 364, 
    function(value) { Scene.setStageYearTime(value); },
    function() { return Scene.getStageYearTime(); },
    function(value) {
        var numDaysPerMonth = 365.0 / 12.0;
        var monthly = (value / numDaysPerMonth);
        var month = Math.floor(monthly);
        return (month + 1).toFixed(0) + "/"  + Math.ceil(0.5 + (monthly - month)*Math.ceil(numDaysPerMonth)).toFixed(0); }
);

panel.newSlider("Day Time", 0, 24, 
        
    function(value) { Scene.setStageDayTime(value); panel.update("Light Direction"); },
    function() { return Scene.getStageDayTime(); },
    function(value) {
        var hour = Math.floor(value);
        return (hour).toFixed(0) + ":" + ((value - hour)*60.0).toFixed(0);
    }
);

var tickTackPeriod = 50;
var tickTackSpeed = 0.0;
panel.newSlider("Tick tack time", -1.0, 1.0, 
    function(value) { tickTackSpeed = value; },
    function() { return tickTackSpeed; },
    function(value) { return (value).toFixed(2); }
);

function runStageTime() {
    if (tickTackSpeed != 0.0) {
        var hour = panel.get("Day Time");
        hour += tickTackSpeed;
        panel.set("Day Time", hour);

        if (hour >= 24.0) {
            panel.set("Year Time", panel.get("Year Time") + 1);
        } else if (hour < 0.0) {
            panel.set("Year Time", panel.get("Year Time") - 1);
        }
    }
}
Script.setInterval(runStageTime, tickTackPeriod);

panel.newCheckbox("Enable Sun Model", 
    function(value) { Scene.setStageSunModelEnable((value != 0)); },
    function() { return Scene.isStageSunModelEnabled(); },
    function(value) { return (value); }
);

panel.newDirectionBox("Light Direction", 
    function(value) { Scene.setKeyLightDirection(value); },
    function() { return Scene.getKeyLightDirection(); },
    function(value) { return value.x.toFixed(2) + "," + value.y.toFixed(2) + "," + value.z.toFixed(2); }
);

panel.newSlider("Light Intensity", 0.0, 5, 
    function(value) { Scene.setKeyLightIntensity(value); },
    function() { return Scene.getKeyLightIntensity(); },
    function(value) { return (value).toFixed(2); }
);

panel.newSlider("Ambient Light Intensity", 0.0, 1.0, 
    function(value) { Scene.setKeyLightAmbientIntensity(value); },
    function() { return Scene.getKeyLightAmbientIntensity(); },
    function(value) { return (value).toFixed(2); }
);

panel.newColorBox("Light Color", 
    function(value) { Scene.setKeyLightColor(value); },
    function() { return Scene.getKeyLightColor(); },
    function(value) { return (value); } // "(" + value.x + "," = value.y + "," + value.z + ")"; }
);

Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) { return panel.mouseMoveEvent(event); });
Controller.mousePressEvent.connect( function panelMousePressEvent(event) { return panel.mousePressEvent(event); });
Controller.mouseReleaseEvent.connect(function(event) { return panel.mouseReleaseEvent(event); });

function scriptEnding() {
    Menu.removeMenu("Developer > Scene");
    panel.destroy();
}
Script.scriptEnding.connect(scriptEnding);
