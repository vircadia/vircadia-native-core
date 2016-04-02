(function() {
  var _this = this;
​
  Script.include("http://public.highfidelity.io/scripts/utilities/tools/cookies.js");
print("EBL PRELOADING NEW VERSION ")
 var audioOptions = new AudioEffectOptions({
    bandwidth: 7000,
    preDelay: 20,
    lateDelay: 0,
    reverbTime: 1.5,
    earlyDiffusion: 100,
    lateDiffusion: 100,
    roomSize: 50,
    density: 100,
    bassMult: 1.5,
    bassFreq: 250,
    highGain: -12,
    highFreq: 3000,
    modRate: 2.3,
    modDepth: 50,
    earlyGain: -24,
    lateGain: -24,
    earlyMixLeft: 20,
    earlyMixRight: 20,
    lateMixLeft: 90,
    lateMixRight: 90,
    wetDryMix: 70,
});
​
​
​
  var panel = new Panel(10, 160);
​
var parameters = [
//    { name: "bandwidth", min: 1000, max: 12000, units: " Hz" },
//    { name: "preDelay", min: 0, max: 333, units: " ms" },
//    { name: "lateDelay", min: 0, max: 166, units: " ms" },
//    { name: "reverbTime", min: 0.1, max: 10, units: " seconds" },
//    { name: "earlyDiffusion", min: 0, max: 100, units: " percent" },
//    { name: "lateDiffusion", min: 0, max: 100, units: " percent" },
//    { name: "roomSize", min: 0, max: 100, units: " percent" },
//    { name: "density", min: 0, max: 100, units: " percent" },
//    { name: "bassMult", min: 0.1, max: 4, units: " ratio" },
//    { name: "bassFreq", min: 10, max: 500, units: " Hz" },
//    { name: "highGain", min: -24, max: 0, units: " dB" },
//    { name: "highFreq", min: 1000, max: 12000, units: " Hz" },
//    { name: "modRate", min: 0.1, max: 10, units: " Hz" },
//    { name: "modDepth", min: 0, max: 100, units: " percent" }, 
//    { name: "earlyGain", min: -96, max: 24, units: " dB" },
//    { name: "lateGain", min: -96, max: 24, units: " dB" },
//    { name: "earlyMixLeft", min: 0, max: 100, units: " percent" },
//    { name: "earlyMixRight", min: 0, max: 100, units: " percent" },
//    { name: "lateMixLeft", min: 0, max: 100, units: " percent" },
//    { name: "lateMixRight", min: 0, max: 100, units: " percent" },
//    { name: "wetDryMix", min: 0, max: 100, units: " percent" },
]
​
​
  function setter(name) {
    return function(value) {
      audioOptions[name] = value;
      AudioDevice.setReverbOptions(audioOptions);
    }
  }
​
  function getter(name) {
    return function() {
      return audioOptions[name];
    }
  }
​
  function displayer(units) {
    return function(value) {
      return (value).toFixed(1) + units;
    }
  }
​
  
  function scriptEnding() {
    panel.destroy();
    AudioDevice.setReverb(false);
    print("Reverb is OFF.");
  }
​
  _this.enterEntity = function(entityID) {
    print('EBL I am insiude');
    // create a slider for each parameter
  for (var i = 0; i < parameters.length; i++) {
    var p = parameters[i];
    panel.newSlider(p.name, p.min, p.max, setter(p.name), getter(p.name), displayer(p.units));
  }
​
  Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) {
    return panel.mouseMoveEvent(event);
  });
  Controller.mousePressEvent.connect(function panelMousePressEvent(event) {
    return panel.mousePressEvent(event);
  });
  Controller.mouseReleaseEvent.connect(function(event) {
    return panel.mouseReleaseEvent(event);
  });
​
    AudioDevice.setReverbOptions(audioOptions);
    AudioDevice.setReverb(true);
    print("Reverb is ON.");
​
​
    // Messages.sendMessage('PlayBackOnAssignment', 'BowShootingGameWelcome');
  };
  
  _this.leaveEntity = function(entityID) {
    print('EBL I am outsidee');
  panel.destroy();
    AudioDevice.setReverb(false);
    print("Reverb is OFF.");
​
​
    // Messages.sendMessage('PlayBackOnAssignment', 'BowShootingGameWelcome');
  };
});