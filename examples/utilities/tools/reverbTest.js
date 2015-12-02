//
//  reverbTest.js
//  examples
//
//  Created by Ken Cooke on 11/23/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("cookies.js");

var audioOptions = new AudioEffectOptions({
    maxRoomSize: 50,
    roomSize: 50,
    reverbTime: 4,
    damping: 0.50,
    inputBandwidth: 0.8,
    earlyLevel: 0,
    tailLevel: 0,
    dryLevel: -6,
    wetLevel: -6
});

AudioDevice.setReverbOptions(audioOptions);
AudioDevice.setReverb(true);
print("Reverb is ON.");

var panel = new Panel(10, 200);

var parameters = [
    { name: "roomSize", min: 0, max: 100, units: " feet" },
    { name: "reverbTime", min: 0, max: 10, units: " sec" },
    { name: "damping", min: 0, max: 1, units: " " },
    { name: "inputBandwidth", min: 0, max: 1, units: " " },
    { name: "earlyLevel", min: -48, max: 0, units: " dB" },
    { name: "tailLevel", min: -48, max: 0, units: " dB" },
    { name: "wetLevel", min: -48, max: 0, units: " dB" },    
]

function setter(name) {
    return function(value) { audioOptions[name] = value; AudioDevice.setReverbOptions(audioOptions); }
}

function getter(name) {
    return function() { return audioOptions[name]; }
}

function displayer(units) {
    return function(value) { return (value).toFixed(1) + units; };
}

// create a slider for each parameter
for (var i = 0; i < parameters.length; i++) {
    var p = parameters[i];
    panel.newSlider(p.name, p.min, p.max, setter(p.name), getter(p.name), displayer(p.units));
}

Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) { return panel.mouseMoveEvent(event); });
Controller.mousePressEvent.connect( function panelMousePressEvent(event) { return panel.mousePressEvent(event); });
Controller.mouseReleaseEvent.connect(function(event) { return panel.mouseReleaseEvent(event); });

function scriptEnding() {
    panel.destroy();
    AudioDevice.setReverb(false);
    print("Reverb is OFF.");
}
Script.scriptEnding.connect(scriptEnding);
