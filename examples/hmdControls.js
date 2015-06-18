//
//  hmdControls.js
//  examples
//
//  Created by Sam Gondelman on 6/17/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var MOVE_DISTANCE = 0.5;
var PITCH_INCREMENT = Math.PI / 8;
var YAW_INCREMENT = Math.PI / 8;
var BOOM_SPEED = 0.5;
var THRESHOLD = 0.2;

var hmdControls = (function () {

	function onActionEvent(action, state) {
        if (state < THRESHOLD) {
            return;
        }
        switch (action) {
            case 0: // backward
                var direction = Quat.getFront(Camera.getOrientation());
                direction = Vec3.multiply(-1, direction);
                direction = Vec3.multiply(Vec3.normalize(direction), MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 1: // forward
                var direction = Quat.getFront(Camera.getOrientation());
                direction = Vec3.multiply(Vec3.normalize(direction), MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 2: // left
                var direction = Quat.getRight(Camera.getOrientation());
                direction = Vec3.multiply(-1, direction);
                direction = Vec3.multiply(Vec3.normalize(direction), MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 3: // right
                var direction = Quat.getRight(Camera.getOrientation());
                direction = Vec3.multiply(Vec3.normalize(direction), MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 4: // down
                var direction = Quat.getUp(Camera.getOrientation());
                direction = Vec3.multiply(-1, direction);
                direction = Vec3.multiply(Vec3.normalize(direction), MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 5: // up
                var direction = Quat.getUp(Camera.getOrientation());
                direction = Vec3.multiply(Vec3.normalize(direction), MOVE_DISTANCE);
                MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
                break;
            case 6: // yaw left
                MyAvatar.bodyYaw = MyAvatar.bodyYaw + YAW_INCREMENT;
                break;
            case 7: // yaw right
                MyAvatar.bodyYaw = MyAvatar.bodyYaw - YAW_INCREMENT;
                break;
            case 8: // pitch down
                MyAvatar.headPitch = Math.max(-180, Math.min(180, MyAvatar.headPitch - PITCH_INCREMENT));
                break;
            case 9: // pitch up
                MyAvatar.headPitch = Math.max(-180, Math.min(180, MyAvatar.headPitch + PITCH_INCREMENT));
                break;
            default:
                break;
        }
    }

	function setUp() {
		Controller.captureActionEvents();

        Controller.actionEvent.connect(onActionEvent);
    }

    function tearDown() {
    	Controller.releaseActionEvents();
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());