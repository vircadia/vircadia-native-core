"use strict";

//  highlightNearbyEntities.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Controller, RIGHT_HAND, LEFT_HAND, MyAvatar, getGrabPointSphereOffset,
   makeRunningValues, Entities, enableDispatcherModule, disableDispatcherModule, makeDispatcherModuleParameters,
   PICK_MAX_DISTANCE, COLORS_GRAB_SEARCHING_HALF_SQUEEZE, COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD,
   DEFAULT_SEARCH_SPHERE_DISTANCE, getGrabbableData, makeLaserParams
*/

(function () {
    Script.include("/~/system/libraries/controllerDispatcherUtils.js");
    Script.include("/~/system/libraries/controllers.js");
    var dispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");
    function HighlightNearbyEntities(hand) {
        this.hand = hand;
        this.highlightedEntities = [];

        this.parameters = dispatcherUtils.makeDispatcherModuleParameters(
            480,
            this.hand === dispatcherUtils.RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);


        this.isGrabable = function(controllerData, props) {
            if (dispatcherUtils.entityIsGrabbable(props) || dispatcherUtils.entityIsCloneable(props)) {
                // if we've attempted to grab a child, roll up to the root of the tree
                var groupRootProps = dispatcherUtils.findGroupParent(controllerData, props);
                if (dispatcherUtils.entityIsGrabbable(groupRootProps)) {
                    return true;
                }
                return true;
            }
            return false;
        };

        this.hasHyperLink = function(props) {
            return (props.href !== "" && props.href !== undefined);
        };

        this.highlightEntities = function(controllerData) {
            if (this.highlightedEntities.length > 0) {
                dispatcherUtils.clearHighlightedEntities();
                this.highlightedEntities = [];
            }

            var nearbyEntitiesProperties = controllerData.nearbyEntityProperties[this.hand];
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            for (var i = 0; i < nearbyEntitiesProperties.length; i++) {
                var props = nearbyEntitiesProperties[i];
                if (props.distance > dispatcherUtils.NEAR_GRAB_RADIUS * sensorScaleFactor) {
                    continue;
                }
                if (this.isGrabable(controllerData, props) || this.hasHyperLink(props)) {
                    dispatcherUtils.highlightTargetEntity(props.id);
                    this.highlightedEntities.push(props.id);
                }
            }
        };

        this.isReady = function(controllerData) {
            this.highlightEntities(controllerData);
            return dispatcherUtils.makeRunningValues(false, [], []);
        };

        this.run = function(controllerData) {
            return this.isReady(controllerData);
        };
    }

    var leftHighlightNearbyEntities = new HighlightNearbyEntities(dispatcherUtils.LEFT_HAND);
    var rightHighlightNearbyEntities = new HighlightNearbyEntities(dispatcherUtils.RIGHT_HAND);

    dispatcherUtils.enableDispatcherModule("LeftHighlightNearbyEntities", leftHighlightNearbyEntities);
    dispatcherUtils.enableDispatcherModule("RightHighlightNearbyEntities", rightHighlightNearbyEntities);

    function cleanup() {
        dispatcherUtils.disableDispatcherModule("LeftHighlightNearbyEntities");
        dispatcherUtils.disableDispatcherModule("RightHighlightNearbyEntities");
    }

    Script.scriptEnding.connect(cleanup);
}());
