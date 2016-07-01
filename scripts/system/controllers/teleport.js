//v1
//check if trigger is down xxx
//if trigger is down, check if thumb is down xxx
//if both are down, enter 'teleport mode' xxx
//aim controller to change landing spot xxx
//visualize aim + spot (line + circle) xxx
//release trigger to teleport then exit teleport mode xxx
//if thumb is release, exit teleport mode xxx

//v2: show room boundaries when choosing a place to teleport
//v2: smooth fade screen in/out?
//v2: haptic feedback


// alternate notes for philip:
// try just thumb to teleport
// cancel if destination is within ~1m of current location


//try moving to final destination in 4 steps: 50% 75% 90% 100% (arrival)



var inTeleportMode = false;

function ThumbPad(hand) {
    this.hand = hand;
    var _this = this;

    this.buttonPress = function(value) {
        print('jbp pad press: ' + value + " on: " + _this.hand)
        _this.buttonValue = value;
    };

    this.down = function() {
        return _this.buttonValue === 1 ? 1.0 : 0.0;
    };

}

function Trigger(hand) {
    this.hand = hand;
    var _this = this;

    this.buttonPress = function(value) {
        print('jbp trigger press: ' + value + " on: " + _this.hand)
        _this.buttonValue = value;

    };

    this.down = function() {
        return _this.buttonValue === 1 ? 1.0 : 0.0;
    };
}


function Teleporter() {
    var _this = this;
    this.targetProps = null;
    this.rightOverlayLine = null;
    this.leftOverlayLine = null;
    this.initialize = function() {
        print('jbp initialize')
        this.createMappings();
        this.disableGrab();
        _this.targetEntity = Entities.addEntity({
            name: 'Hifi-Teleporter-Target-Entity',
            position: MyAvatar.position,
            type: 'Sphere',
            dimensions: {
                x: 0.2,
                y: 0.2,
                z: 0.2
            },
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            collisionless: true,
            collidesWith: '',
            visible: true
        });
    };


    this.createMappings = function() {
        print('jbp create mappings internal');
        // peek at the trigger and thumbs to store their values
        teleporter.telporterMappingInternalName='Hifi-Teleporter-Internal-Dev-' + Math.random();
       teleporter.teleportMappingInternal =  Controller.newMapping(teleporter.telporterMappingInternalName);
        
        Controller.enableMapping(teleporter.telporterMappingInternalName);
    };

    this.disableMappings = function() {
        print('jbp disable mappings internal')
        Controller.disableMapping(teleporter.telporterMappingInternalName);
    };

    this.enterTeleportMode = function(hand) {
        if (inTeleportMode === true) {
            return
        }
        print('jbp hand on entering teleport mode: ' + hand);
        inTeleportMode = true;
        this.teleportHand = hand;
        this.initialize();
        this.updateConnected = true;
        Script.update.connect(this.update);
    };

    this.exitTeleportMode = function(value) {
        print('jbp value on exit: ' + value);
        this.disableMappings();
        this.rightOverlayOff();
        this.leftOverlayOff();
        Entities.deleteEntity(_this.targetEntity);
        this.enableGrab();
        Script.update.disconnect(this.update);
        this.updateConnected = false;
        inTeleportMode = false;

    };

    this.update = function() {
       //print('in teleporter update')
        if (rightPad.buttonValue === 0 || leftPad.buttonValue === 0) {
            print('JBP THUMB RELEASED SHOULD EXIT')
            _this.exitTeleportMode();
            return;
        }

        if (teleporter.teleportHand === 'left') {
            teleporter.leftRay();
            if (leftTrigger.buttonValue === 0) {
                _this.teleport();
            }
        } else {
            teleporter.rightRay();
            if (rightTrigger.buttonValue === 0) {
                _this.teleport();
            }
        }


    };

    this.rightRay = function() {

        var rightPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.RightHand).translation), MyAvatar.position);

        var rightRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.RightHand).rotation)

        var rightPickRay = {
            origin: rightPosition,
            direction: Quat.getUp(rightRotation),
        };

        this.rightPickRay = rightPickRay;

        var location = Vec3.sum(rightPickRay.origin, Vec3.multiply(rightPickRay.direction, 500));
        this.rightLineOn(rightPickRay.origin, location, {
            red: 255,
            green: 0,
            blue: 0
        });
        var rightIntersection = Entities.findRayIntersection(teleporter.rightPickRay, true, [], [this.targetEntity]);

        if (rightIntersection.intersects) {
            this.updateTargetEntity(rightIntersection);
        };
    };

    this.leftRay = function() {
        var leftPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.LeftHand).translation), MyAvatar.position);


        var leftRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.LeftHand).rotation)
        var leftPickRay = {
            origin: leftPosition,
            direction: Quat.getUp(leftRotation),
        };

        this.leftPickRay = leftPickRay;

        var location = Vec3.sum(leftPickRay.origin, Vec3.multiply(leftPickRay.direction, 500));
        this.leftLineOn(leftPickRay.origin, location, {
            red: 0,
            green: 255,
            blue: 0
        });
        var leftIntersection = Entities.findRayIntersection(teleporter.leftPickRay, true, [], [this.targetEntity]);

        if (leftIntersection.intersects) {
            this.updateTargetEntity(leftIntersection);
        };
    };

    this.rightLineOn = function(closePoint, farPoint, color) {
        // draw a line
        if (this.rightOverlayLine === null) {
            var lineProperties = {
                lineWidth: 5,
                start: closePoint,
                end: farPoint,
                color: color,
                ignoreRayIntersection: true, // always ignore this
                visible: true,
                alpha: 1
            };

            this.rightOverlayLine = Overlays.addOverlay("line3d", lineProperties);

        } else {
            var success = Overlays.editOverlay(this.rightOverlayLine, {
                lineWidth: 5,
                start: closePoint,
                end: farPoint,
                color: color,
                visible: true,
                ignoreRayIntersection: true, // always ignore this
                alpha: 1
            });
        }
    };

    this.leftLineOn = function(closePoint, farPoint, color) {
        // draw a line
        if (this.leftOverlayLine === null) {
            var lineProperties = {
                lineWidth: 5,
                start: closePoint,
                end: farPoint,
                color: color,
                ignoreRayIntersection: true, // always ignore this
                visible: true,
                alpha: 1
            };

            this.leftOverlayLine = Overlays.addOverlay("line3d", lineProperties);

        } else {
            var success = Overlays.editOverlay(this.leftOverlayLine, {
                lineWidth: 5,
                start: closePoint,
                end: farPoint,
                color: color,
                visible: true,
                ignoreRayIntersection: true, // always ignore this
                alpha: 1
            });
        }
    };

    this.rightOverlayOff = function() {
        if (this.rightOverlayLine !== null) {
            Overlays.deleteOverlay(this.rightOverlayLine);
            this.rightOverlayLine=null;
        }
    };

    this.leftOverlayOff = function() {
        if (this.leftOverlayLine !== null) {
            Overlays.deleteOverlay(this.leftOverlayLine);
             this.leftOverlayLine=null;
        }
    };

    this.updateTargetEntity = function(intersection) {
        var targetProps = Entities.getEntityProperties(this.targetEntity);
        var position = {
            x: intersection.intersection.x,
            y: intersection.intersection.y + targetProps.dimensions.y / 2,
            z: intersection.intersection.z
        }
        Entities.editEntity(this.targetEntity, {
            position: position
        });


    };

    this.disableGrab = function() {
        Messages.sendLocalMessage('Hifi-Hand-Disabler', 'both');
    };

    this.enableGrab = function() {
        Messages.sendLocalMessage('Hifi-Hand-Disabler', 'none');
    };

    this.teleport = function(value) {
        //todo
        //get the y position of the teleport landing spot
        print('value on teleport: ' + value)
        var properties = Entities.getEntityProperties(teleporter.targetEntity);
        var offset = getAvatarFootOffset();
        properties.position.y += offset;

        print('OFFSET IS::: ' + JSON.stringify(offset))
        print('TELEPORT POSITION IS:: ' + JSON.stringify(properties.position));
        MyAvatar.position = properties.position;

        this.exitTeleportMode();
    };
}

//related to repositioning the avatar after you teleport
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


var leftPad = new ThumbPad('left');
var rightPad = new ThumbPad('right');
var leftTrigger = new Trigger('left');
var rightTrigger = new Trigger('right');

//create a controller mapping and make sure to disable it when the script is stopped

var mappingName = 'Hifi-Teleporter-Dev-' + Math.random();
var teleportMapping = Controller.newMapping(mappingName);
teleportMapping.from(Controller.Standard.RT).peek().to(rightTrigger.buttonPress);
teleportMapping.from(Controller.Standard.LT).peek().to(leftTrigger.buttonPress);
teleportMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(rightPad.buttonPress);
teleportMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(leftPad.buttonPress);

teleportMapping.from(leftPad.down).when(leftTrigger.down).to(function() {
    teleporter.enterTeleportMode('left')
});
teleportMapping.from(rightPad.down).when(rightTrigger.down).to(function() {
    teleporter.enterTeleportMode('right')
});

var teleporter = new Teleporter();

Controller.enableMapping(mappingName);

Script.scriptEnding.connect(cleanup);

function cleanup() {
    teleportMapping.disable();
    teleporter.disableMappings();
    teleporter.rightOverlayOff();
    teleporter.leftOverlayOff();
    Entities.deleteEntity(teleporter.targetEntity);
    if (teleporter.updateConnected !== null) {
        Script.update.disconnect(teleporter.update);
    }
}