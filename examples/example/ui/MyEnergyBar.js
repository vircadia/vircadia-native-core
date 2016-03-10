Script.include("../../libraries/utils.js");
var energyColor = {red: 0, green: 200, blue: 0};
var lowEnergyColor = {red: 255, green: 0, blue: 0};
var totalWidth = 200;
var paddingRight = 50;
var xPosition = Window.innerWidth - totalWidth - paddingRight;
var lowEnergyThreshold = 0.3;
var currentEnergy = 1.0;
var energyLossRate = 0.003;
var energyChargeRate = 0.003;
var isGrabbing = false;
var refractoryPeriod = 2000;

var lastAvatarVelocity = MyAvatar.getVelocity(); 
var lastAvatarPosition = MyAvatar.position;

var background = Overlays.addOverlay("text", {
    x: xPosition,
    y: 20,
    width: totalWidth,
    height: 10,
    backgroundColor: {red: 255, green: 0, blue: 0}
})

var bar = Overlays.addOverlay("text", {
    x: xPosition,
    y: 20,
    width: totalWidth,
    height: 10,
    backgroundColor: energyColor
});


// Takes an energy value between 0 and 1 and sets energy bar width appropriately
function setEnergy(energy) {
    energy = clamp(energy, 0, 1);
    var barWidth = totalWidth * energy;
    var color = energy <= lowEnergyThreshold ? lowEnergyColor: energyColor;
    Overlays.editOverlay(bar, { width: barWidth, backgroundColor: color});
}

function update() {
    currentEnergy = clamp(MyAvatar.energy, 0, 1);
    setEnergy(currentEnergy);
}

function cleanup() {
    Overlays.deleteOverlay(background);
    Overlays.deleteOverlay(bar);
}

function energyChanged(newValue) {
    Entities.currentAvatarEnergy = newValue;
}

function debitAvatarEnergy(value) {
   MyAvatar.energy = MyAvatar.energy - value;
}
Entities.costMultiplier = 0.02;
Entities.debitEnergySource.connect(debitAvatarEnergy);
MyAvatar.energyChanged.connect(energyChanged);
Script.update.connect(update);
Script.scriptEnding.connect(cleanup);
