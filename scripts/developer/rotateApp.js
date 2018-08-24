/* global Script, Vec3, MyAvatar, Tablet, Messages, Quat, 
DebugDraw, Mat4, Entities, Xform, Controller, Camera, console, document*/

Script.registerValue("ROTATEAPP", true);

var TABLET_BUTTON_NAME = "ROTATE";
var CHANGE_OF_BASIS_ROTATION = { x: 0, y: 1, z: 0, w: 0 };
var DEFAULT_HEAD_TURN_THRESHOLD = 0.5333;
var DEFAULT_HEAD_TURN_FILTER_LENGTH = 4.0;
var LOADING_DELAY = 2000;
var AVERAGING_RATE = 0.03;
var INCREASING = 1.0;
var DECREASING = -1.0;
var DEGREES_PER_PI_RADIANS = 180.0;
var FILTER_FUDGE_RANGE = 0.9;

var activated = false;
var documentLoaded = false;
var sciptLoaded = false;
var headPoseAverageOrientation = { x: 0, y: 0, z: 0, w: 1 };
var hipToLeftHandAverage = 0.0; // { x: 0, y: 0, z: 0, w: 1 };
var hipToRightHandAverage = 0.0; // { x: 0, y: 0, z: 0, w: 1 };
var averageAzimuth = 0.0;
var hipsPositionRigSpace = { x: 0, y: 0, z: 0 };
var spine2PositionRigSpace = { x: 0, y: 0, z: 0 };
var hipsRotationRigSpace = { x: 0, y: 0, z: 0, w: 1 };
var spine2RotationRigSpace = { x: 0, y: 0, z: 0, w: 1 };
var spine2Rotation = { x: 0, y: 0, z: 0, w: 1 };
var hipToLHandAverage = { x: 0, y: 0, z: 0, w: 1 };
var hipToRHandAverage = { x: 0, y: 0, z: 0, w: 1 };

/*
var ikTypes = {
    RotationAndPosition: 0,
    RotationOnly: 1,
    HmdHead: 2,
    HipsRelativeRotationAndPosition: 3,
    Spline: 4,
    Unknown: 5
};


var ANIM_VARS = [
    //"headType",
    "spine2Type",
    //"hipsType",
    "spine2Position",
    "spine2Rotation"
    //"hipsPosition",
    //"hipsRotation"
];

var handlerId = MyAvatar.addAnimationStateHandler(function (props) {
    //print("in callback");
    //print("props spine2 pos: " + props.spine2Position.x + " " + props.spine2Position.y + " " + props.spine2Position.z);
    //print("props hip pos: " + props.hipsPosition.x + " " + props.hipsPosition.y + " " + props.hipsPosition.z);
    var result = {};
    //{x:0,y:0,z:0}
    //result.headType = ikTypes.HmdHead;
    //result.hipsType = ikTypes.RotationAndPosition;
    //result.hipsPosition = hipsPositionRigSpace; // { x: 0, y: 0, z: 0 };
    //result.hipsRotation = hipsRotationRigSpace;//{ x: 0, y: 0, z: 0, w: 1 }; //
    result.spine2Type = ikTypes.Spline;
    result.spine2Position = spine2PositionRigSpace; // { x: 0, y: 1.3, z: 0 }; 
    result.spine2Rotation = spine2Rotation;
    
    return result;
}, ANIM_VARS);
*/
// define state readings constructor
function StateReading(headPose, rhandPose, lhandPose, diffFromAverageEulers) {
    this.headPose = headPose;
    this.rhandPose = rhandPose;
    this.lhandPose = lhandPose;
    this.diffFromAverageEulers = diffFromAverageEulers;
}

// define current state readings object for holding tracker readings and current differences from averages
var currentStateReadings = new StateReading(Controller.getPoseValue(Controller.Standard.Head),
    Controller.getPoseValue(Controller.Standard.RightHand), 
    Controller.getPoseValue(Controller.Standard.LeftHand),
    { x: 0, y: 0, z: 0 });
    
// declare the checkbox constructor
function AppCheckbox(type,id,eventType,isChecked) {
    this.type = type;
    this.id = id;
    this.eventType = eventType;
    this.data = {value: isChecked};
}

var usingStepResetRotationDirection = new AppCheckbox("checkboxtick", "stepReset", "onStepResetCheckBox", false);
var usingDrawAverageFacing = new AppCheckbox("checkboxtick", "drawAverage", "onDrawAverageFacingCheckBox", false);
var checkBoxArray = new Array(usingStepResetRotationDirection, usingDrawAverageFacing);

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
}

// var HTML_URL = Script.resolvePath("file:///c:/dev/hifi_fork/hifi/scripts/developer/rotateRecenterApp.html");
var HTML_URL = Script.resolvePath("http://hifi-content.s3.amazonaws.com/angus/stepApp/rotateRecenterApp.html");
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

// define the sliders
var filterLengthProperty = new AppProperty("#filterLength-slider", "slider", "onFilterLengthSlider", "filterSignal",
    setFilterLength, DEFAULT_HEAD_TURN_FILTER_LENGTH, function (num) {
        var base = 5;
        var shift = 0;
        return convertExponential(base, num, INCREASING, shift);
    }, function (num) {
        var base = 5;
        var shift = 0;
        return convertLog(base, num, INCREASING, shift);
    }, true);
var angleThresholdProperty = new AppProperty("#angleThreshold-slider", "slider", "onAngleThresholdSlider", "angleSignal",
    setAngleThreshold, DEFAULT_HEAD_TURN_THRESHOLD, function (num) {
        return convertToRadians(num);
    }, function (num) {
        return convertToDegrees(num);
    }, true);

var propArray = new Array(filterLengthProperty, angleThresholdProperty);

function setFilterLength(num) {
    filterLengthProperty.value = num;
    MyAvatar.rotationRecenterFilterLength = filterLengthProperty.value;
    
}

function setAngleThreshold(num) {
    angleThresholdProperty.value = num;
    MyAvatar.rotationThreshold = angleThresholdProperty.value;
}

function convertToRadians(num) {
    return (num / DEGREES_PER_PI_RADIANS) * Math.PI;
}

function convertToDegrees(num) {
    return (num / Math.PI) * DEGREES_PER_PI_RADIANS;
}

function getLog(x, y) {
    return Math.log(y) / Math.log(x);
}

function convertLog(base, num, direction, shift) {
    return direction * getLog(base, (num + FILTER_FUDGE_RANGE)) + shift;
}

function convertExponential(base, num, direction, shift) {
    return Math.pow(base, (direction * num + shift)) - FILTER_FUDGE_RANGE;
}

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

function onKeyPress(event) {
    if (event.text === "'") {
        // when the sensors are reset, then reset the mode.
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
            if (cbox.id === "stepReset") {
                MyAvatar.enableStepResetRotation = cbox.data.value;
            }
            if (cbox.id === "drawAverage") {
                MyAvatar.enableDrawAverageFacing = cbox.data.value;
            }
            // break;
        }

    });
    if (message.type === "onCreateRotateApp") {
        print("document loaded");
        documentLoaded = true;
        Script.setTimeout(initAppForm, LOADING_DELAY);
    }
}

function initAppForm() {
    print("step app is loaded: " + documentLoaded);
    if (documentLoaded === true) {
        propArray.forEach(function (prop) {
            print(prop.name);
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
                "data": { value: cbox.data.value }
            }));
        });
    }
 
}


function onScreenChanged(type, url) {     
    print("Screen changed");
    if (type === "Web" && url === HTML_URL) {
        if (!activated) {
            // hook up to event bridge
            tablet.webEventReceived.connect(onWebEventReceived);
            print("after connect web event");
            MyAvatar.hmdLeanRecenterEnabled = true;
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

function limitAngle(angle) {
    return (angle + 180) % 360 - 180;
}

function computeHandAzimuths(timeElapsed) {

   // var leftHandPositionRigSpace = Vec3.multiplyQbyV(Quat.inverse(CHANGE_OF_BASIS_ROTATION), currentStateReadings.lhandPose.translation);
   // var rightHandPositionRigSpace = Vec3.multiplyQbyV(Quat.inverse(CHANGE_OF_BASIS_ROTATION), currentStateReadings.rhandPose.translation);


   // var hipToLeftHand = Quat.lookAtSimple({ x: 0, y: 0, z: 0 }, { x: leftHandPositionRigSpace.x, y: 0, z: leftHandPositionRigSpace.z });
   // var hipToRightHand = Quat.lookAtSimple({ x: 0, y: 0, z: 0 }, { x: rightHandPositionRigSpace.x, y: 0, z: rightHandPositionRigSpace.z });
   // var hipToHandHalfway = Quat.slerp(hipToLeftHand, hipToRightHand, 0.5);

    var leftHand = currentStateReadings.lhandPose.translation;
    var rightHand = currentStateReadings.rhandPose.translation;
    var head = currentStateReadings.headPose.translation;
    var lHandMinusHead = Vec3.subtract(leftHand, head);
    lHandMinusHead.y = 0.0;
    var rHandMinusHead = Vec3.subtract(rightHand, head);
    rHandMinusHead.y = 0.0;
    //print(JSON.stringify(leftHand));
    //print(JSON.stringify(head));
    var avatarZAxis = { x: 0.0, y: 0.0, z: 1.0 };
    var hipToLHand = Quat.lookAtSimple({ x: 0, y: 0, z: 0 }, lHandMinusHead);
    var hipToRHand = Quat.lookAtSimple({ x: 0, y: 0, z: 0 }, rHandMinusHead);
    var tau = timeElapsed / filterLengthProperty.value;
    hipToLHandAverage = Quat.slerp(hipToLHandAverage, hipToLHand, tau);
    hipToRHandAverage = Quat.slerp(hipToRHandAverage, hipToRHand, tau);

    // var angleToLeft = limitAngle(Quat.safeEulerAngles(hipToLHandAverage).y);
    // var angleToRight = limitAngle(Quat.safeEulerAngles(hipToRHandAverage).y);
    var leftRightMidpoint = (Quat.safeEulerAngles(hipToLHand).y + Quat.safeEulerAngles(hipToRHand).y) / 2.0;
    var leftRightMidpointAverage = (Quat.safeEulerAngles(hipToLHandAverage).y + Quat.safeEulerAngles(hipToRHandAverage).y) / 2.0;
    
    // limit the angle because we are flipped by 180, fix this tomorrow.
    // print(leftRightMidpointAverage/180.0);
    // print("threshold value " + angleThresholdProperty.value);
    

    var raySpineRotation = Quat.fromVec3Degrees({ x: 0, y: leftRightMidpointAverage, z: 0 });
    var zAxisSpineRotation = Vec3.multiplyQbyV(raySpineRotation, { x: 0, y: 0, z: -1 });
    var zAxisWorldSpace = Vec3.multiplyQbyV(MyAvatar.orientation, zAxisSpineRotation);
    // DebugDraw.drawRay(MyAvatar.position, Vec3.sum(MyAvatar.position, zAxisWorldSpace), { x: 1, y: 0, z: 0, w: 1 });

    //print(leftRightMidpoint);

    var headPoseRigSpace = Quat.multiply(CHANGE_OF_BASIS_ROTATION, currentStateReadings.head.rotation);
    headPoseAverageOrientation = Quat.slerp(headPoseAverageOrientation, headPoseRigSpace, tau);
    var headPoseAverageEulers = Quat.safeEulerAngles(headPoseAverageOrientation);

    // get it into radians too!!
    // average head and hands 50/50
    // print("hands azimuth " + leftRightMidpointAverage + " head azimuth " + headPoseAverageEulers.y);
    var headPlusHands = (leftRightMidpointAverage + headPoseAverageEulers.y) / 2.0;
    if ((Math.abs(headPlusHands / 180.0) * Math.PI) > angleThresholdProperty.value) {
        //print("recenter the feet under the head");
        // MyAvatar.triggerRotationRecenter();
        hipToLHandAverage = { x: 0, y: 0, z: 0, w: 1 };
        hipToRHandAverage = { x: 0, y: 0, z: 0, w: 1 };
        headPoseAverageOrientation = { x: 0, y: 0, z: 0, w: 1 };
    }
    
    // put a hard max on this easing function.  
    var rotateAngle = ((Math.cos((leftRightMidpoint / 180.0) * Math.PI) + 2.0)/3.0) * leftRightMidpoint;
    //print("rotate angle " + rotateAngle);

    return Quat.fromVec3Degrees({ x: 0, y: rotateAngle, z: 0 });


}

function update(dt) {
    //update state readings
    currentStateReadings.head = Controller.getPoseValue(Controller.Standard.Head);
    currentStateReadings.rhandPose = Controller.getPoseValue(Controller.Standard.RightHand);
    currentStateReadings.lhandPose = Controller.getPoseValue(Controller.Standard.LeftHand);

    //print(JSON.stringify(currentStateReadings.head));



    var latestSpineRotation = computeHandAzimuths(dt);
    
    spine2Rotation = latestSpineRotation;
    var spine2Pos = MyAvatar.getAbsoluteJointTranslationInObjectFrame(MyAvatar.getJointIndex("Spine2"));
    spine2PositionRigSpace = Vec3.multiplyQbyV(CHANGE_OF_BASIS_ROTATION, spine2Pos);
    /*
    if (HMD.active && !scriptLoaded) {
        //Script.load("rotateApp.js");
        scriptLoaded = true;
    }

    if (!HMD.active) {
        scriptLoaded = false;
    }
    */

    // handle the azimuth of the arms

}

function shutdownTabletApp() {
    tablet.removeButton(tabletButton);
    if (activated) {
        tablet.webEventReceived.disconnect(onWebEventReceived);
        tablet.gotoHomeScreen();
    }
    tablet.screenChanged.disconnect(onScreenChanged);
}

Script.setTimeout(function () {
    tabletButton.clicked.connect(manageClick);
    tablet.screenChanged.connect(onScreenChanged);
    Script.update.connect(update);
    Controller.keyPressEvent.connect(onKeyPress);
}, (LOADING_DELAY));

Script.scriptEnding.connect(function () {
    // if (handlerId) {
    // print("removing animation state handler");
    // handlerId = MyAvatar.removeAnimationStateHandler(handlerId);
    // }
    MyAvatar.hmdLeanRecenterEnabled = true;
    Script.update.disconnect(update);
    shutdownTabletApp();
});