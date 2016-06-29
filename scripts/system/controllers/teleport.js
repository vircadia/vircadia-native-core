//check if trigger is down
//if trigger is down, check if thumb is down
//if both are down, enter 'teleport mode'
//aim controller to change landing spot
//visualize aim + spot (line + circle)
//release trigger to teleport then exit teleport mode
//if thumb is release, exit teleport mode


//v2: show room boundaries when choosing a place to teleport
//v2: smooth fade screen in/out?
//v2: haptic feedback
//v2: particles for parabolic arc to landing spot

//parabola: point count, point spacing, graphic thickness



//Standard.LeftPrimaryThumb
//Standard.LT


//create a controller mapping and make sure to disable it when the script is stopped
var teleportMapping = Controller.newMapping('Hifi-Teleporter-Dev-' + Math.random());
Script.scriptEnding.connect(teleportMapping.disable);

// Gather the trigger data for smoothing.
clickMapping.from(Controller.Standard.RT).peek().to(rightTrigger.triggerPress);
clickMapping.from(Controller.Standard.LT).peek().to(leftTrigger.triggerPress);



function thumbPad() {

}

thumbPad.prototype = {
    down: function() {
        return this.padValue === 0 ? true : false;
    }
}

function trigger() {

}

trigger.prototype = {
    triggerPress: function(value) {
        this.triggerValue = value;
    },
    down: function() {
        return this.triggerValue === 0 ? true : false;
    }
}


teleportMapping.from(thumbPad.down).when(trigger.down).to(teleport);

function teleport() {

}

function() {}