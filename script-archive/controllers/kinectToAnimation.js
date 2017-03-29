var handlerId = 0;

var ikTypes = {
    RotationAndPosition: 0,
    RotationOnly: 1,
    HmdHead: 2,
    HipsRelativeRotationAndPosition: 3,
    Off: 4
};

var MAPPING_NAME = "com.highfidelity.examples.kinectToAnimation";
var mapping = Controller.newMapping(MAPPING_NAME);

var recentLeftHand;
var recentRightHand;
var recentLeftFoot;
var recentRightFoot;

mapping.from(Controller.Hardware.Kinect.LeftHand).debug(true).to(function(pose) { recentLeftHand = pose; });
mapping.from(Controller.Hardware.Kinect.RightHand).debug(true).to(function(pose) { recentRightHand = pose; });
mapping.from(Controller.Hardware.Kinect.LeftFoot).debug(true).to(function(pose) { recentLeftFoot = pose; });
mapping.from(Controller.Hardware.Kinect.RightFoot).debug(true).to(function(pose) { recentRightFoot = pose; });

function init() {
    var t = 0;
    var propList = [
        "leftHandType", "leftHandPosition", "leftHandRotation", "rightHandType", "rightHandPosition", "rightHandPosition",
        "leftFootType", "leftFootPosition", "leftFootRotation", "rightFootType", "rightFootPosition", "rightFootPosition"
    ];
    handlerId = MyAvatar.addAnimationStateHandler(function (props) {
        return {
            rightHandType: ikTypes["HipsRelativeRotationAndPosition"],
            rightHandPosition: recentRightHand.translation,
            rightHandRotation: recentRightHand.rotation,
            leftHandType: ikTypes["HipsRelativeRotationAndPosition"],
            leftHandPosition: recentLeftHand.translation,
            leftHandRotation: recentLeftHand.rotation,

            rightFootType: ikTypes["HipsRelativeRotationAndPosition"],
            rightFootPosition: recentRightFoot.translation,
            rightFootRotation: recentRightFoot.rotation,
            leftFootType: ikTypes["HipsRelativeRotationAndPosition"],
            leftFootPosition: recentLeftFoot.translation,
            leftFootRotation: recentLeftFoot.rotation,

        };
    }, propList);

    Controller.enableMapping(MAPPING_NAME);
}

init();
Script.scriptEnding.connect(function(){
    MyAvatar.removeAnimationStateHandler(handlerId);
    mapping.disable();
});



