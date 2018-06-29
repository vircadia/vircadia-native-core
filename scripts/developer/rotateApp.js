/* global Script, Vec3, MyAvatar, Tablet, Messages, Quat, 
DebugDraw, Mat4, Entities, Xform, Controller, Camera, console, document*/

Script.registerValue("ROTATEAPP", true);

var TABLET_BUTTON_NAME = "ROTATE";
var CHANGE_OF_BASIS_ROTATION = { x: 0, y: 1, z: 0, w: 0 };
var HEAD_TURN_THRESHOLD = .5333;
var HEAD_TURN_FILTER_LENGTH = 4.0;
var LOADING_DELAY = 500;
var AVERAGING_RATE = 0.03;

var activated = false;
var documentLoaded = false;
var headPoseAverageOrientation = { x: 0, y: 0, z: 0, w: 1 };
var hipToLeftHandAverage = 0.0; // { x: 0, y: 0, z: 0, w: 1 };
var hipToRightHandAverage = 0.0; // { x: 0, y: 0, z: 0, w: 1 };
var averageAzimuth = 0.0;
var hipsPositionRigSpace = { x: 0, y: 0, z: 0 };
var spine2PositionRigSpace = { x: 0, y: 0, z: 0 };
var hipsRotationRigSpace = { x: 0, y: 0, z: 0, w: 1 };
var spine2RotationRigSpace = { x: 0, y: 0, z: 0, w: 1 };
var spine2Rotation = { x: 0, y: 0, z: 0, w: 1 };

var ikTypes = {
    RotationAndPosition: 0,
    RotationOnly: 1,
    HmdHead: 2,
    HipsRelativeRotationAndPosition: 3,
    Spline: 4,
    Unknown: 5
};

/*
var ANIM_VARS = [
    //"headType",
    "spine2Type",
    //"hipsType",
    "spine2Position",
    "spine2Rotation",
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

var HTML_URL = Script.resolvePath("file:///c:/dev/hifi_fork/hifi/scripts/developer/rotateApp.html");
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

// define the sliders
var filterLengthProperty = new AppProperty("#filterLength-slider", "slider", "onFilterLengthSlider", "filterSignal",
    setAnteriorDistance, DEFAULT_ANTERIOR, function (num) {
        return convertToMeters(num);
    }, function (num) {
        return convertToCentimeters(num);
    },true);
var angleThresholdProperty = new AppProperty("#angleThreshold-slider", "slider", "onAngleThresholdSlider", "angleSignal",
    setPosteriorDistance, DEFAULT_POSTERIOR, function (num) {
        return convertToMeters(num);
    }, function (num) {
        return convertToCentimeters(num);
    }, true);



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
            // break;
        }
    });
}

function initAppForm() {
    print("step app is loaded: " + documentLoaded);
    propArray.forEach(function (prop) {
        print(prop.name);
        tablet.emitScriptEvent(JSON.stringify({
            "type": "slider",
            "id": prop.name,
            "data": { "value": prop.convertToSlider(prop.value) }
        }));
    });
    /*
    checkBoxArray.forEach(function(cbox) {
        tablet.emitScriptEvent(JSON.stringify({
            "type": "checkboxtick",
            "id": cbox.id,
            "data": { value: cbox.data.value }
        }));
    });
    */
 
}


function onScreenChanged(type, url) {     
    print("Screen changed");
    if (type === "Web" && url === HTML_URL) {
        if (!activated) {
            // hook up to event bridge
            tablet.webEventReceived.connect(onWebEventReceived);
            print("after connect web event");
            MyAvatar.hmdLeanRecenterEnabled = false;
            Script.setTimeout(initAppForm, LOADING_DELAY);
        }
        activated = true;
    } else {
        if (activated) {
            // disconnect from event bridge
            tablet.webEventReceived.disconnect(onWebEventReceived);
        }
        activated = false;
    }
}
*/
function update(dt) {


}

function shutdownTabletApp() {
    tablet.removeButton(tabletButton);
    if (activated) {
        tablet.webEventReceived.disconnect(onWebEventReceived);
        tablet.gotoHomeScreen();
    }
    tablet.screenChanged.disconnect(onScreenChanged);
}

tabletButton.clicked.connect(manageClick);
tablet.screenChanged.connect(onScreenChanged);

Script.update.connect(update);

Controller.keyPressEvent.connect(onKeyPress);

Script.scriptEnding.connect(function () {
    if (handlerId) {
        print("removing animation state handler");
        // handlerId = MyAvatar.removeAnimationStateHandler(handlerId);
    }
    MyAvatar.hmdLeanRecenterEnabled = true;
    Script.update.disconnect(update);
    shutdownTabletApp();
});