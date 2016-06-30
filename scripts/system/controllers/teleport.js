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

// peek at the trigger and thumbs to store their values
teleportMapping.from(Controller.Standard.RT).peek().to(rightTrigger.buttonPress);
teleportMapping.from(Controller.Standard.LT).peek().to(leftTrigger.buttonPress);
teleportMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(rightPad.buttonPress);
teleportMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(leftPad.buttonPress);


function ThumbPad() {

}

ThumbPad.prototype = {
    buttonPress: function(value) {
        print('pad press: ' + value)
        this.buttonValue = value;
    },
    down: function() {
        return this.buttonValue === 0 ? true : false;
    }
}

function Trigger() {

}

Trigger.prototype = {
    buttonPress: function(value) {
        print('trigger press: ' + value)
        this.buttonValue = value;
    },
    down: function() {
        return this.buttonValue === 0 ? true : false;
    }
}

teleportMapping.from(leftPad.down).when(leftTrigger.down).to(teleporter.enterTeleportMode);
teleportMapping.from(rightPad.down).when(rightTrigger.down).to(teleporter.enterTeleportMode);


function Teleporter() {

}

Teleporter.prototype = {
    enterTeleportMode: function(value) {
        print('value on enter: ' + value);
    },
    exitTeleportMode: function(value) {
        print('value on exit: ' + value);
    },
    teleport: function(value) {
        //todo
        //get the y position of the teleport landing spot
        print('value on teleport: ' + value)
        var properties = Entities.getEntityProperties(_this.entityID);
        var offset = getAvatarFootOffset();
        properties.position.y += offset;

        print('OFFSET IS::: ' + JSON.stringify(offset))
        print('TELEPORT POSITION IS:: ' + JSON.stringify(properties.position));
        MyAvatar.position = properties.position;
    }
}

function getAvatarFootOffset() {
    var data = getJointData();
    var upperLeg, lowerLeg, foot, toe, toeTop;
    data.forEach(function(d) {

        var jointName = d.joint;
        if (jointName === "RightUpLeg") {
            upperLeg = d.translation.y;
        }
        if (jointName === "RightLeg") {
            lowerLeg = d.translation.y;
        }
        if (jointName === "RightFoot") {
            foot = d.translation.y;
        }
        if (jointName === "RightToeBase") {
            toe = d.translation.y;
        }
        if (jointName === "RightToe_End") {
            toeTop = d.translation.y
        }
    })

    var myPosition = MyAvatar.position;
    var offset = upperLeg + lowerLeg + foot + toe + toeTop;
    offset = offset / 100;
    return offset
};

function getJointData() {
    var allJointData = [];
    var jointNames = MyAvatar.jointNames;
    jointNames.forEach(function(joint, index) {
        var translation = MyAvatar.getJointTranslation(index);
        var rotation = MyAvatar.getJointRotation(index)
        allJointData.push({
            joint: joint,
            index: index,
            translation: translation,
            rotation: rotation
        });
    });

    return allJointData;
}


var leftPad = new ThumbPad();
var rightPad = new ThumbPad();
var leftTrigger = new Trigger();
var rightTrigger = new Trigger();
var teleporter = new Teleporter();