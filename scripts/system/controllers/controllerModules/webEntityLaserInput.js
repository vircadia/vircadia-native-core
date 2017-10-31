"use strict";

//  webEntityLaserInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Controller, LaserPointers, RayPick, RIGHT_HAND, LEFT_HAND, Vec3, Quat, getGrabPointSphereOffset,
   makeRunningValues, Entities, enableDispatcherModule, disableDispatcherModule, makeDispatcherModuleParameters,
   PICK_MAX_DISTANCE, COLORS_GRAB_SEARCHING_HALF_SQUEEZE, COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD,
   DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_ON_VALUE, ZERO_VEC, Overlays, Picks
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    function WebEntityLaserInput(hand) {
        this.hand = hand;
        this.parameters = makeDispatcherModuleParameters(
            550,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.isPointingAtWebEntity = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];
            var entityProperty = Entities.getEntityProperties(intersection.objectID);
            var entityType = entityProperty.type;

            if ((intersection.type === Picks.INTERSECTED_ENTITY && entityType === "Web")) {
                return true;
            }
            return false;
        };

        this.isReady = function(controllerData) {
            if (this.isPointingAtWebEntity(controllerData)) {
                return makeRunningValues(true, [], []);
            }
            return makeRunningValues(false, [], []);
        };

        this.run = function(controllerData, deltaTime) {
            if (!this.isPointingAtWebEntity(controllerData)) {
                return makeRunningValues(false, [], []);
            }
            return makeRunningValues(true, [], []);
        };
    }

    var leftWebEntityLaserInput = new WebEntityLaserInput(LEFT_HAND);
    var rightWebEntityLaserInput = new WebEntityLaserInput(RIGHT_HAND);

    enableDispatcherModule("LeftWebEntityLaserInput", leftWebEntityLaserInput);
    enableDispatcherModule("RightWebEntityLaserInput", rightWebEntityLaserInput);

    function cleanup() {
        disableDispatcherModule("LeftWebEntityLaserInput");
        disableDispatcherModule("RightWebEntityLaserInput");
    };
    Script.scriptEnding.connect(cleanup);

}());
