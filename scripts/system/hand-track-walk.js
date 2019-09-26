
/* global Script, Controller, Vec3 */
/* jshint loopfunc:true */

(function() {

    var mappingName = 'hand-track-walk-' + Math.random();
    var inputMapping = Controller.newMapping(mappingName);

    var leftIndexPos = null;
    var rightIndexPos = null;

    var pinchOnBelowDistance = 0.016;
    var pinchOffAboveDistance = 0.04;

    var walking = false;

    function updateWalking() {
        if (leftIndexPos && rightIndexPos) {
            var tipDistance = Vec3.distance(leftIndexPos, rightIndexPos);
            if (tipDistance < pinchOnBelowDistance) {
                print("qqqq walking");
                walking = true;
            } else if (walking && tipDistance > pinchOffAboveDistance) {
                print("qqqq stopping");
                walking = false;
            }
        }
    }

    function leftIndexChanged(pose) {
        if (pose.valid) {
            leftIndexPos = pose.translation;
        } else {
            leftIndexPos = null;
        }
        updateWalking();
    }

    function rightIndexChanged(pose) {
        if (pose.valid) {
            rightIndexPos = pose.translation;
        } else {
            rightIndexPos = null;
        }
        updateWalking();
    }

    function cleanUp() {
        inputMapping.disable();
    }

    Script.scriptEnding.connect(function () {
        cleanUp();
    });

    inputMapping.from(Controller.Standard.LeftHandIndex4).peek().to(leftIndexChanged);
    inputMapping.from(Controller.Standard.RightHandIndex4).peek().to(rightIndexChanged);

    inputMapping.from(function() {
        if (walking) {
            return -1;
        } else {
            return Controller.getActionValue(Controller.Standard.TranslateZ);
        }
    }).to(Controller.Actions.TranslateZ);

    Controller.enableMapping(mappingName);
})();
