/* jslint bitwise: true */

/* global Script, Vec3, MyAvatar, Tablet, Messages, Quat, 
DebugDraw, Mat4, Entities, Xform, Controller, Camera, console, document*/

Script.registerValue("STEPAPP", true);
var CENTIMETERSPERMETER = 100.0;
var LEFT = 0;
var RIGHT = 1;
var INCREASING = 1.0;
var DECREASING = -1.0;
var DEFAULT_AVATAR_HEIGHT = 1.64;
var TABLET_BUTTON_NAME = "STEP";
var CHANGE_OF_BASIS_ROTATION = { x: 0, y: 1, z: 0, w: 0 };
// in meters (mostly)
var DEFAULT_ANTERIOR = 0.04;
var DEFAULT_POSTERIOR = 0.06;
var DEFAULT_LATERAL = 0.10;
var DEFAULT_HEIGHT_DIFFERENCE = 0.02;
var DEFAULT_ANGULAR_VELOCITY = 0.3;
var DEFAULT_HAND_VELOCITY = 0.4;
var DEFAULT_ANGULAR_HAND_VELOCITY = 3.3;
var DEFAULT_HEAD_VELOCITY = 0.14;
var DEFAULT_LEVEL_PITCH = 7;
var DEFAULT_LEVEL_ROLL = 7;
var DEFAULT_DIFF = 0.0;
var DEFAULT_DIFF_EULERS = { x: 0.0, y: 0.0, z: 0.0 };
var DEFAULT_HIPS_POSITION;
var DEFAULT_HEAD_POSITION; 
var DEFAULT_TORSO_LENGTH; 
var SPINE_STRETCH_LIMIT = 0.02;

var VELOCITY_EPSILON = 0.02;
var AVERAGING_RATE = 0.03;
var HEIGHT_AVERAGING_RATE = 0.01;
var STEP_TIME_SECS = 0.2;
var MODE_SAMPLE_LENGTH = 100;
var RESET_MODE = false;
var HEAD_TURN_THRESHOLD = 25.0;
var NO_SHARED_DIRECTION = -0.98;
var LOADING_DELAY = 500;
var FAILSAFE_TIMEOUT = 2.5;

var debugDrawBase = true;
var activated = false;
var documentLoaded = false;
var failsafeFlag = false;
var failsafeSignalTimer = -1.0;
var stepTimer = -1.0;


var modeArray = new Array(MODE_SAMPLE_LENGTH);
var modeHeight = -10.0;

var handPosition;
var handOrientation;
var hands = [];
var hipToHandAverage = [];
var handDotHead = [];
var headAverageOrientation = MyAvatar.orientation;
var headPoseAverageOrientation = { x: 0, y: 0, z: 0, w: 1 };
var averageHeight = 1.0;
var headEulers = { x: 0.0, y: 0.0, z: 0.0 };
var headAverageEulers = { x: 0.0, y: 0.0, z: 0.0 };
var headAveragePosition = { x: 0, y: 0.4, z: 0 };
var frontLeft = { x: -DEFAULT_LATERAL, y: 0, z: -DEFAULT_ANTERIOR };
var frontRight = { x: DEFAULT_LATERAL, y: 0, z: -DEFAULT_ANTERIOR };
var backLeft = { x: -DEFAULT_LATERAL, y: 0, z: DEFAULT_POSTERIOR };
var backRight = { x: DEFAULT_LATERAL, y: 0, z: DEFAULT_POSTERIOR };


// define state readings constructor
function StateReading(headPose, rhandPose, lhandPose, backLength, diffFromMode, diffFromAverageHeight, diffFromAveragePosition,
    diffFromAverageEulers) {
    this.headPose = headPose;
    this.rhandPose = rhandPose;
    this.lhandPose = lhandPose;
    this.backLength = backLength;
    this.diffFromMode = diffFromMode;
    this.diffFromAverageHeight = diffFromAverageHeight;
    this.diffFromAveragePosition = diffFromAveragePosition;
    this.diffFromAverageEulers = diffFromAverageEulers;
}

// define current state readings object for holding tracker readings and current differences from averages
var currentStateReadings = new StateReading(Controller.getPoseValue(Controller.Standard.Head),
    Controller.getPoseValue(Controller.Standard.RightHand), Controller.getPoseValue(Controller.Standard.LeftHand),
    DEFAULT_TORSO_LENGTH, DEFAULT_DIFF, DEFAULT_DIFF, DEFAULT_DIFF, DEFAULT_DIFF_EULERS);

// declare the checkbox constructor
function AppCheckbox(type,id,eventType,isChecked) {
    this.type = type;
    this.id = id;
    this.eventType = eventType;
    this.data = {value: isChecked};
}

// define the checkboxes in the html file
var usingAverageHeight = new AppCheckbox("checkboxtick", "runningAverageHeightCheck", "onRunningAverageHeightCheckBox",
    false);
var usingModeHeight = new AppCheckbox("checkboxtick","modeCheck","onModeCheckBox",true);
var usingBaseOfSupport = new AppCheckbox("checkboxtick","baseOfSupportCheck","onBaseOfSupportCheckBox",true);
var usingAverageHeadPosition = new AppCheckbox("checkboxtick", "headAveragePositionCheck", "onHeadAveragePositionCheckBox",
    false);

var checkBoxArray = new Array(usingAverageHeight,usingModeHeight,usingBaseOfSupport,usingAverageHeadPosition);

// declare the html slider constructor
function AppProperty(name, type, eventType, signalType, setFunction, initValue, convertToThreshold, convertToSlider, signalOn) {
    this.name = name;
    this.type = type;
    this.eventType = eventType;
    this.signalType = signalType;
    this.setValue = setFunction;
    this.value = initValue;
    this.get = function () {
        return this.value;
    };
    this.convertToThreshold = convertToThreshold;
    this.convertToSlider = convertToSlider;
    this.signalOn = signalOn;
}

// define the sliders
var frontBaseProperty = new AppProperty("#anteriorBase-slider", "slider", "onAnteriorBaseSlider", "frontSignal",
    setAnteriorDistance, DEFAULT_ANTERIOR, function (num) {
        return convertToMeters(num);
    }, function (num) {
        return convertToCentimeters(num);
    },true);
var backBaseProperty = new AppProperty("#posteriorBase-slider", "slider", "onPosteriorBaseSlider", "backSignal",
    setPosteriorDistance, DEFAULT_POSTERIOR, function (num) {
        return convertToMeters(num);
    }, function (num) {
        return convertToCentimeters(num);
    }, true);
var lateralBaseProperty = new AppProperty("#lateralBase-slider", "slider", "onLateralBaseSlider", "lateralSignal",
    setLateralDistance, DEFAULT_LATERAL, function (num) {
        return convertToMeters(num);
    }, function (num) {
        return convertToCentimeters(num);
    }, true);
var headAngularVelocityProperty = new AppProperty("#angularVelocityHead-slider", "slider", "onAngularVelocitySlider",
    "angularHeadSignal", setAngularThreshold, DEFAULT_ANGULAR_VELOCITY, function (num) {
        var base = 4;
        var shift = 2;
        return convertExponential(base, num, DECREASING, shift);
    }, function (num) {
        var base = 4;
        var shift = 2;
        return convertLog(base, num, DECREASING, shift);
    }, true);
var heightDifferenceProperty = new AppProperty("#heightDifference-slider", "slider", "onHeightDifferenceSlider", "heightSignal",
    setHeightThreshold, DEFAULT_HEIGHT_DIFFERENCE, function (num) {
        return convertToMeters(-num);
    }, function (num) {
        return convertToCentimeters(-num);
    }, true);
var handsVelocityProperty = new AppProperty("#handsVelocity-slider", "slider", "onHandsVelocitySlider", "handVelocitySignal",
    setHandVelocityThreshold, DEFAULT_HAND_VELOCITY, function (num) {
        return num;
    }, function (num) {
        return num;
    }, true);
var handsAngularVelocityProperty = new AppProperty("#handsAngularVelocity-slider", "slider", "onHandsAngularVelocitySlider",
    "handAngularSignal", setHandAngularVelocityThreshold, DEFAULT_ANGULAR_HAND_VELOCITY, function (num) {
        var base = 7;
        var shift = 2;
        return convertExponential(base, num, DECREASING, shift);
    }, function (num) {
        var base = 7;
        var shift = 2;
        return convertLog(base, num, DECREASING, shift);
    }, true);
var headVelocityProperty = new AppProperty("#headVelocity-slider", "slider", "onHeadVelocitySlider", "headVelocitySignal",
    setHeadVelocityThreshold, DEFAULT_HEAD_VELOCITY, function (num) {
        var base = 2;
        var shift = 0;
        return convertExponential(base, num, INCREASING, shift);
    }, function (num) {
        var base = 2;
        var shift = 0;
        return convertLog(base, num, INCREASING, shift);
    }, true);
var headPitchProperty = new AppProperty("#headPitch-slider", "slider", "onHeadPitchSlider", "headPitchSignal",
    setHeadPitchThreshold, DEFAULT_LEVEL_PITCH, function (num) {
        var base = 2.5;
        var shift = 5;
        return convertExponential(base, num, DECREASING, shift);
    }, function (num) {
        var base = 2.5;
        var shift = 5;
        return convertLog(base, num, DECREASING, shift);
    }, true);
var headRollProperty = new AppProperty("#headRoll-slider", "slider", "onHeadRollSlider", "headRollSignal", setHeadRollThreshold,
    DEFAULT_LEVEL_ROLL, function (num) {
        var base = 2.5;
        var shift = 5;
        return convertExponential(base, num, DECREASING, shift);
    }, function (num) {
        var base = 2.5;
        var shift = 5;
        return convertLog(base, num, DECREASING, shift);
    }, true);

var propArray = new Array(frontBaseProperty, backBaseProperty, lateralBaseProperty, headAngularVelocityProperty,
    heightDifferenceProperty, handsVelocityProperty, handsAngularVelocityProperty, headVelocityProperty, headPitchProperty,
    headRollProperty);

// var HTML_URL = Script.resolvePath("http://hifi-content.s3.amazonaws.com/angus/stepApp/stepApp.html");
var HTML_URL = Script.resolvePath("http://hifi-content.s3.amazonaws.com/angus/stepApp/stepAppExtra.html");
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

function manageClick() {
    if (activated) {
        tablet.gotoHomeScreen();
    } else {
        tablet.gotoWebScreen(HTML_URL);
    }
}

var tabletButton = tablet.addButton({
    text: TABLET_BUTTON_NAME,
    icon: Script.resolvePath("http://hifi-content.s3.amazonaws.com/angus/stepApp/foot.svg"),
    activeIcon: Script.resolvePath("http://hifi-content.s3.amazonaws.com/angus/stepApp/foot.svg")
});

function drawBase() {
    // transform corners into world space, for rendering.
    var worldPointLf = Vec3.sum(MyAvatar.position,Vec3.multiplyQbyV(MyAvatar.orientation, frontLeft));
    var worldPointRf = Vec3.sum(MyAvatar.position,Vec3.multiplyQbyV(MyAvatar.orientation, frontRight));
    var worldPointLb = Vec3.sum(MyAvatar.position,Vec3.multiplyQbyV(MyAvatar.orientation, backLeft));
    var worldPointRb = Vec3.sum(MyAvatar.position,Vec3.multiplyQbyV(MyAvatar.orientation, backRight));

    var GREEN = { r: 0, g: 1, b: 0, a: 1 };
    // draw border
    DebugDraw.drawRay(worldPointLf, worldPointRf, GREEN);
    DebugDraw.drawRay(worldPointRf, worldPointRb, GREEN);
    DebugDraw.drawRay(worldPointRb, worldPointLb, GREEN);
    DebugDraw.drawRay(worldPointLb, worldPointLf, GREEN);
}

function onKeyPress(event) {
    if (event.text === "'") {
        // when the sensors are reset, then reset the mode.
        RESET_MODE = false;
    }
}

function onWebEventReceived(msg) {
    var message = JSON.parse(msg);
    print(" we have a message from html dialog " + message.type);
    propArray.forEach(function (prop) {
        if (prop.eventType === message.type) {
            prop.setValue(prop.convertToThreshold(message.data.value));
            print("message from " + prop.name);
            // break;
        }
    });
    checkBoxArray.forEach(function(cbox) {
        if (cbox.eventType === message.type) {
            cbox.data.value = message.data.value;
            // break;
        }
    });
    if (message.type === "onCreateStepApp") {
        print("document loaded");
        documentLoaded = true;
        Script.setTimeout(initAppForm, LOADING_DELAY);
    }
}

function initAppForm() {
    print("step app is loaded: " + documentLoaded);
    if (documentLoaded === true) {
        propArray.forEach(function (prop) {
            tablet.emitScriptEvent(JSON.stringify({
                "type": "trigger",
                "id": prop.signalType,
                "data": { "value": "green" }
            }));
            tablet.emitScriptEvent(JSON.stringify({
                "type": "slider",
                "id": prop.name,
                "data": { "value": prop.convertToSlider(prop.value) }
            }));
        });
        checkBoxArray.forEach(function (cbox) {
            tablet.emitScriptEvent(JSON.stringify({
                "type": "checkboxtick",
                "id": cbox.id,
                "data": { "value": cbox.data.value }
            }));
        });
    }
}

function updateSignalColors() {

    // force the updates by running the threshold comparisons
    withinBaseOfSupport(currentStateReadings.headPose.translation);
    withinThresholdOfStandingHeightMode(currentStateReadings.diffFromMode);
    headAngularVelocityBelowThreshold(currentStateReadings.headPose.angularVelocity);
    handDirectionMatchesHeadDirection(currentStateReadings.lhandPose, currentStateReadings.rhandPose);
    handAngularVelocityBelowThreshold(currentStateReadings.lhandPose, currentStateReadings.rhandPose);
    headVelocityGreaterThanThreshold(Vec3.length(currentStateReadings.headPose.velocity));
    headMovedAwayFromAveragePosition(currentStateReadings.diffFromAveragePosition);
    headLowerThanHeightAverage(currentStateReadings.diffFromAverageHeight);
    isHeadLevel(currentStateReadings.diffFromAverageEulers);

    propArray.forEach(function (prop) {
        if (prop.signalOn) {
            tablet.emitScriptEvent(JSON.stringify({ "type": "trigger", "id": prop.signalType, "data": { "value": "green" } }));
        } else {
            tablet.emitScriptEvent(JSON.stringify({ "type": "trigger", "id": prop.signalType, "data": { "value": "red" } }));
        }
    });
}

function onScreenChanged(type, url) {     
    print("Screen changed");
    if (type === "Web" && url === HTML_URL) {
        if (!activated) {
            // hook up to event bridge
            tablet.webEventReceived.connect(onWebEventReceived);
            print("after connect web event");
            MyAvatar.hmdLeanRecenterEnabled = false;
            
        }
        activated = true;
    } else {
        if (activated) {
            // disconnect from event bridge
            tablet.webEventReceived.disconnect(onWebEventReceived);
            documentLoaded = false;
        }
        activated = false;
    }
}

function getLog(x, y) {
    return Math.log(y) / Math.log(x);
}

function noConversion(num) {
    return num;
}

function convertLog(base, num, direction, shift) {
    return direction * getLog(base, (num + 1.0)) + shift;
}

function convertExponential(base, num, direction, shift) {
    return Math.pow(base, (direction*num + shift)) - 1.0;
}

function convertToCentimeters(num) {
    return num * CENTIMETERSPERMETER;
}

function convertToMeters(num) {
    print("convert to meters " + num);
    return num / CENTIMETERSPERMETER;
}

function isInsideLine(a, b, c) {
    return (((b.x - a.x)*(c.z - a.z) - (b.z - a.z)*(c.x - a.x)) > 0);
}

function setAngularThreshold(num) {
    headAngularVelocityProperty.value = num;
    print("angular threshold " + headAngularVelocityProperty.get());
}

function setHeadRollThreshold(num) {
    headRollProperty.value = num;
    print("head roll threshold " + headRollProperty.get());
}

function setHeadPitchThreshold(num) {
    headPitchProperty.value = num;
    print("head pitch threshold " + headPitchProperty.get());
}

function setHeightThreshold(num) {
    heightDifferenceProperty.value = num;
    print("height threshold " + heightDifferenceProperty.get());
}

function setLateralDistance(num) {
    lateralBaseProperty.value = num;
    frontLeft.x = -lateralBaseProperty.get();
    frontRight.x = lateralBaseProperty.get();
    backLeft.x = -lateralBaseProperty.get();
    backRight.x = lateralBaseProperty.get();
    print("lateral distance  " + lateralBaseProperty.get());
}

function setAnteriorDistance(num) {
    frontBaseProperty.value = num;
    frontLeft.z = -frontBaseProperty.get();
    frontRight.z = -frontBaseProperty.get();
    print("anterior distance " + frontBaseProperty.get());
}

function setPosteriorDistance(num) {
    backBaseProperty.value = num;
    backLeft.z = backBaseProperty.get();
    backRight.z = backBaseProperty.get();
    print("posterior distance " + backBaseProperty.get());
}

function setHandAngularVelocityThreshold(num) {
    handsAngularVelocityProperty.value = num;
    print("hand angular velocity threshold " + handsAngularVelocityProperty.get());
}

function setHandVelocityThreshold(num) {
    handsVelocityProperty.value = num;
    print("hand velocity threshold " + handsVelocityProperty.get());
}

function setHeadVelocityThreshold(num) {
    headVelocityProperty.value = num;
    print("headvelocity threshold " + headVelocityProperty.get());
}

function withinBaseOfSupport(pos) {
    var userScale = 1.0;
    frontBaseProperty.signalOn = !(isInsideLine(Vec3.multiply(userScale, frontLeft), Vec3.multiply(userScale, frontRight), pos));
    backBaseProperty.signalOn = !(isInsideLine(Vec3.multiply(userScale, backRight), Vec3.multiply(userScale, backLeft), pos));
    lateralBaseProperty.signalOn = !(isInsideLine(Vec3.multiply(userScale, frontRight), Vec3.multiply(userScale, backRight), pos)
        && isInsideLine(Vec3.multiply(userScale, backLeft), Vec3.multiply(userScale, frontLeft), pos));
    return (!frontBaseProperty.signalOn && !backBaseProperty.signalOn && !lateralBaseProperty.signalOn);
}

function withinThresholdOfStandingHeightMode(heightDiff) {
    if (usingModeHeight.data.value) {
        heightDifferenceProperty.signalOn = heightDiff < heightDifferenceProperty.get();
        return heightDifferenceProperty.signalOn;
    } else {
        return true;
    }
}

function headAngularVelocityBelowThreshold(headAngularVelocity) {
    var angVel = Vec3.length({ x: headAngularVelocity.x, y: 0, z: headAngularVelocity.z });
    headAngularVelocityProperty.signalOn = angVel < headAngularVelocityProperty.get();
    return headAngularVelocityProperty.signalOn;
}

function handDirectionMatchesHeadDirection(lhPose, rhPose) {
    handsVelocityProperty.signalOn = ((handsVelocityProperty.get() < NO_SHARED_DIRECTION) ||
        ((!lhPose.valid || ((handDotHead[LEFT] > handsVelocityProperty.get()) &&
        (Vec3.length(lhPose.velocity) > VELOCITY_EPSILON))) &&
        (!rhPose.valid || ((handDotHead[RIGHT] > handsVelocityProperty.get()) &&
        (Vec3.length(rhPose.velocity) > VELOCITY_EPSILON)))));
    return handsVelocityProperty.signalOn;
}

function handAngularVelocityBelowThreshold(lhPose, rhPose) {
    var xzRHandAngularVelocity = Vec3.length({ x: rhPose.angularVelocity.x, y: 0.0, z: rhPose.angularVelocity.z });
    var xzLHandAngularVelocity = Vec3.length({ x: lhPose.angularVelocity.x, y: 0.0, z: lhPose.angularVelocity.z });
    handsAngularVelocityProperty.signalOn = ((!rhPose.valid ||(xzRHandAngularVelocity < handsAngularVelocityProperty.get())) 
        && (!lhPose.valid || (xzLHandAngularVelocity < handsAngularVelocityProperty.get())));
    return handsAngularVelocityProperty.signalOn;
}

function headVelocityGreaterThanThreshold(headVel) {
    headVelocityProperty.signalOn = (headVel > headVelocityProperty.get()) || (headVelocityProperty.get() < VELOCITY_EPSILON);
    return headVelocityProperty.signalOn;
}

function headMovedAwayFromAveragePosition(headDelta) {
    return !withinBaseOfSupport(headDelta) || !usingAverageHeadPosition.data.value;
}

function headLowerThanHeightAverage(heightDiff) {
    if (usingAverageHeight.data.value) {
        print("head lower than height average");
        heightDifferenceProperty.signalOn = heightDiff < heightDifferenceProperty.get();
        return heightDifferenceProperty.signalOn;
    } else {
        return true;
    }
}

function isHeadLevel(diffEulers) {
    headRollProperty.signalOn = Math.abs(diffEulers.z) < headRollProperty.get();
    headPitchProperty.signalOn = Math.abs(diffEulers.x) < headPitchProperty.get();
    return (headRollProperty.signalOn && headPitchProperty.signalOn);
}

function findAverage(arr) {
    var sum = arr.reduce(function (acc, val) {
        return acc + val;
    },0);
    return sum / arr.length;
}

function addToModeArray(arr,num) {
    for (var i = 0 ;i < (arr.length - 1); i++) {
        arr[i] = arr[i+1];
    }
    arr[arr.length - 1] = (Math.floor(num*CENTIMETERSPERMETER))/CENTIMETERSPERMETER;
}

function findMode(ary, currentMode, backLength, defaultBack, currentHeight) {
    var numMapping = {};
    var greatestFreq = 0;
    var mode;
    ary.forEach(function (number) {
        numMapping[number] = (numMapping[number] || 0) + 1;
        if ((greatestFreq < numMapping[number]) || ((numMapping[number] === MODE_SAMPLE_LENGTH) && (number > currentMode) )) {
            greatestFreq = numMapping[number];
            mode = number;
        }
    });
    if (mode > currentMode) {
        return Number(mode);    
    } else {
        if (!RESET_MODE && HMD.active) {
            print("resetting the mode............................................. ");
            print("resetting the mode............................................. ");
            RESET_MODE = true;
            var correction = 0.02;
            return currentHeight - correction;
        } else {
            return currentMode; 
        }
    }    
}

function update(dt) {
    if (debugDrawBase) {
        drawBase();
    }
    //  Update current state information
    currentStateReadings.headPose = Controller.getPoseValue(Controller.Standard.Head);
    currentStateReadings.rhandPose = Controller.getPoseValue(Controller.Standard.RightHand);
    currentStateReadings.lhandPose = Controller.getPoseValue(Controller.Standard.LeftHand);

    // back length
    var headMinusHipLean = Vec3.subtract(currentStateReadings.headPose.translation, DEFAULT_HIPS_POSITION);
    currentStateReadings.backLength = Vec3.length(headMinusHipLean);
    // print("back length and default " + currentStateReadings.backLength + " " + DEFAULT_TORSO_LENGTH);

    // mode height
    addToModeArray(modeArray, currentStateReadings.headPose.translation.y);
    modeHeight = findMode(modeArray, modeHeight, currentStateReadings.backLength, DEFAULT_TORSO_LENGTH,
        currentStateReadings.headPose.translation.y);
    currentStateReadings.diffFromMode = modeHeight - currentStateReadings.headPose.translation.y;

    // hand direction
    var leftHandLateralPoseVelocity = currentStateReadings.lhandPose.velocity;
    leftHandLateralPoseVelocity.y = 0.0;
    var rightHandLateralPoseVelocity = currentStateReadings.rhandPose.velocity;
    rightHandLateralPoseVelocity.y = 0.0;
    var headLateralPoseVelocity = currentStateReadings.headPose.velocity;
    headLateralPoseVelocity.y = 0.0;
    handDotHead[LEFT] = Vec3.dot(Vec3.normalize(leftHandLateralPoseVelocity), Vec3.normalize(headLateralPoseVelocity));
    handDotHead[RIGHT] = Vec3.dot(Vec3.normalize(rightHandLateralPoseVelocity), Vec3.normalize(headLateralPoseVelocity));
    
    // average head position
    headAveragePosition = Vec3.mix(headAveragePosition, currentStateReadings.headPose.translation, AVERAGING_RATE);
    currentStateReadings.diffFromAveragePosition = Vec3.subtract(currentStateReadings.headPose.translation,
        headAveragePosition);

    // average height
    averageHeight = currentStateReadings.headPose.translation.y * HEIGHT_AVERAGING_RATE +
        averageHeight * (1.0 - HEIGHT_AVERAGING_RATE);
    currentStateReadings.diffFromAverageHeight = Math.abs(currentStateReadings.headPose.translation.y - averageHeight);

    // eulers diff
    headEulers = Quat.safeEulerAngles(currentStateReadings.headPose.rotation);
    headAverageOrientation = Quat.slerp(headAverageOrientation, currentStateReadings.headPose.rotation, AVERAGING_RATE);
    headAverageEulers = Quat.safeEulerAngles(headAverageOrientation);
    currentStateReadings.diffFromAverageEulers = Vec3.subtract(headAverageEulers, headEulers);

    // headpose rig space is for determining when to recenter rotation.
    var headPoseRigSpace = Quat.multiply(CHANGE_OF_BASIS_ROTATION, currentStateReadings.headPose.rotation);
    headPoseAverageOrientation = Quat.slerp(headPoseAverageOrientation, headPoseRigSpace, AVERAGING_RATE);
    var headPoseAverageEulers = Quat.safeEulerAngles(headPoseAverageOrientation);

    // make the signal colors reflect the current thresholds that have been crossed
    updateSignalColors();
    print("the current back length is " + currentStateReadings.backLength + " " + DEFAULT_TORSO_LENGTH);
    //  Conditions for taking a step. 
    // 1. off the base of support. front, lateral, back edges.
    // 2. head is not lower than the height mode value by more than the maxHeightChange tolerance
    // 3. the angular velocity of the head is not greater than the threshold value
    //    ie this reflects the speed the head is rotating away from having up = (0,1,0) in Avatar frame..
    // 4. the hands velocity vector has the same direction as the head, within the given tolerance
    //    the tolerance is an acos value, -1 means the hands going in any direction will not block translating
    //    up to 1 where the hands velocity direction must exactly match that of the head. -1 threshold disables this condition.
    // 5. the angular velocity xz magnitude for each hand is below the threshold value
    //    ie here this reflects the speed that each hand is rotating away from having up = (0,1,0) in Avatar frame.
    // 6. head velocity is below step threshold
    // 7. head has moved further than the threshold from the running average position of the head.
    // 8. head height is not lower than the running average head height with a difference of maxHeightChange.
    // 9. head's rotation in avatar space is not pitching or rolling greater than the pitch or roll thresholds
    if (!withinBaseOfSupport(currentStateReadings.headPose.translation) &&
        withinThresholdOfStandingHeightMode(currentStateReadings.diffFromMode) &&
        headAngularVelocityBelowThreshold(currentStateReadings.headPose.angularVelocity) &&
        handDirectionMatchesHeadDirection(currentStateReadings.lhandPose, currentStateReadings.rhandPose) &&
        handAngularVelocityBelowThreshold(currentStateReadings.lhandPose, currentStateReadings.rhandPose) &&  
        headVelocityGreaterThanThreshold(Vec3.length(currentStateReadings.headPose.velocity)) &&
        headMovedAwayFromAveragePosition(currentStateReadings.diffFromAveragePosition) && 
        headLowerThanHeightAverage(currentStateReadings.diffFromAverageHeight) &&
        isHeadLevel(currentStateReadings.diffFromAverageEulers)) {

        if (stepTimer < 0.0) { //!MyAvatar.isRecenteringHorizontally()
            print("trigger recenter========================================================");
            MyAvatar.triggerHorizontalRecenter();
            stepTimer = STEP_TIME_SECS;
        }
    } else if ((currentStateReadings.backLength > (DEFAULT_TORSO_LENGTH + SPINE_STRETCH_LIMIT)) &&
        (failsafeSignalTimer < 0.0) && HMD.active) {
        // do the failsafe recenter.
        // failsafeFlag stops repeated setting of failsafe button color.
        // RESET_MODE false forces a reset of the height
        RESET_MODE = false;
        failsafeFlag = true;
        failsafeSignalTimer = FAILSAFE_TIMEOUT;
        MyAvatar.triggerHorizontalRecenter();
        tablet.emitScriptEvent(JSON.stringify({ "type": "failsafe", "id": "failsafeSignal", "data": { "value": "green" } }));
        // in fail safe we debug print the values that were blocking us.
        print("failsafe debug---------------------------------------------------------------");
        propArray.forEach(function (prop) {
            print(prop.name);
            if (!prop.signalOn) {
                print(prop.signalType + " contributed to failsafe call");
            }
        });
        print("end failsafe debug---------------------------------------------------------------");

    }
    
    if ((failsafeSignalTimer < 0.0) && failsafeFlag) {
        failsafeFlag = false;
        tablet.emitScriptEvent(JSON.stringify({ "type": "failsafe", "id": "failsafeSignal", "data": { "value": "orange" } }));
    }

    stepTimer -= dt;
    failsafeSignalTimer -= dt;

    if (!HMD.active) {
        RESET_MODE = false;
    }

    if (Math.abs(headPoseAverageEulers.y) > HEAD_TURN_THRESHOLD) {
        //  Turn feet
        // MyAvatar.triggerRotationRecenter();
        // headPoseAverageOrientation = { x: 0, y: 0, z: 0, w: 1 };
    }  
}
    
function shutdownTabletApp() {
    // GlobalDebugger.stop();
    tablet.removeButton(tabletButton);
    if (activated) {
        tablet.webEventReceived.disconnect(onWebEventReceived);
        tablet.gotoHomeScreen();
    }
    tablet.screenChanged.disconnect(onScreenChanged);
}

tabletButton.clicked.connect(manageClick);
tablet.screenChanged.connect(onScreenChanged);

Script.setTimeout(function() {
    DEFAULT_HIPS_POSITION = MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(MyAvatar.getJointIndex("Hips"));
    DEFAULT_HEAD_POSITION = MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(MyAvatar.getJointIndex("Head"));
    DEFAULT_TORSO_LENGTH = Vec3.length(Vec3.subtract(DEFAULT_HEAD_POSITION, DEFAULT_HIPS_POSITION));
},(4*LOADING_DELAY));

Script.update.connect(update);
Controller.keyPressEvent.connect(onKeyPress);
Script.scriptEnding.connect(function () {
    MyAvatar.hmdLeanRecenterEnabled = true;
    Script.update.disconnect(update);
    shutdownTabletApp();
});
