//
//  highlightNearbyEntities.js
//
//  Created by Dante Ruiz 2018-4-10
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Controller, RIGHT_HAND, LEFT_HAND, MyAvatar, getGrabPointSphereOffset,
   makeRunningValues, Entities, enableDispatcherModule, disableDispatcherModule, makeDispatcherModuleParameters,
   PICK_MAX_DISTANCE, COLORS_GRAB_SEARCHING_HALF_SQUEEZE, COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD,
   DEFAULT_SEARCH_SPHERE_DISTANCE, getGrabbableData, makeLaserParams, entityIsCloneable, Messages, print
*/

"use strict";

(function () {
    Script.include("/~/system/libraries/controllerDispatcherUtils.js");
    Script.include("/~/system/libraries/controllers.js");
    Script.include("/~/system/libraries/cloneEntityUtils.js");
    var dispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");

    function differenceInArrays(firstArray, secondArray) {
        var differenceArray = firstArray.filter(function(element) {
            return secondArray.indexOf(element) < 0;
        });

        return differenceArray;
    }

    function HighlightNearbyEntities(hand) {
        this.hand = hand;
        this.otherHand = hand === dispatcherUtils.RIGHT_HAND ? dispatcherUtils.LEFT_HAND :
            dispatcherUtils.RIGHT_HAND;
        this.highlightedEntities = [];

        this.parameters = dispatcherUtils.makeDispatcherModuleParameters(
            120,
            this.hand === dispatcherUtils.RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);


        this.isGrabable = function(controllerData, props) {
            var canGrabEntity = false;
            if (dispatcherUtils.entityIsGrabbable(props) || entityIsCloneable(props)) {
                // if we've attempted to grab a child, roll up to the root of the tree
                var groupRootProps = dispatcherUtils.findGroupParent(controllerData, props);
                canGrabEntity = true;
                if (!dispatcherUtils.entityIsGrabbable(groupRootProps)) {
                    canGrabEntity = false;
                }
            }
            return canGrabEntity;
        };

        this.clearAll = function() {
            this.highlightedEntities.forEach(function(entity) {
                dispatcherUtils.unhighlightTargetEntity(entity);
            });
        };

        this.hasHyperLink = function(props) {
            return (props.href !== "" && props.href !== undefined);
        };

        this.removeEntityFromHighlightList = function(entityID) {
            var index = this.highlightedEntities.indexOf(entityID);
            if (index > -1) {
                this.highlightedEntities.splice(index, 1);
            }
        };

        this.getOtherModule = function() {
            var otherModule = this.hand === dispatcherUtils.RIGHT_HAND ? leftHighlightNearbyEntities :
                rightHighlightNearbyEntities;
            return otherModule;
        };

        this.getOtherHandHighlightedEntities = function() {
            return this.getOtherModule().highlightedEntities;
        };

        this.highlightEntities = function(controllerData) {
            var nearbyEntitiesProperties = controllerData.nearbyEntityProperties[this.hand];
            var otherHandHighlightedEntities = this.getOtherHandHighlightedEntities();
            var newHighlightedEntities = [];
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            for (var i = 0; i < nearbyEntitiesProperties.length; i++) {
                var props = nearbyEntitiesProperties[i];
                if (props.distance > dispatcherUtils.NEAR_GRAB_RADIUS * sensorScaleFactor) {
                    continue;
                }
                if (this.isGrabable(controllerData, props) || this.hasHyperLink(props)) {
                    dispatcherUtils.highlightTargetEntity(props.id);
                    if (newHighlightedEntities.indexOf(props.id) < 0) {
                        newHighlightedEntities.push(props.id);
                    }
                }
            }

            var unhighlightEntities = differenceInArrays(this.highlightedEntities, newHighlightedEntities);

            unhighlightEntities.forEach(function(entityID) {
                if (otherHandHighlightedEntities.indexOf(entityID) < 0 ) {
                    dispatcherUtils.unhighlightTargetEntity(entityID);
                }
            });
            this.highlightedEntities = newHighlightedEntities;
        };

        this.isReady = function(controllerData) {
            this.highlightEntities(controllerData);
            return dispatcherUtils.makeRunningValues(false, [], []);
        };

        this.run = function(controllerData) {
            return this.isReady(controllerData);
        };
    }

    var handleMessage = function(channel, message, sender) {
        var data;
        if (sender === MyAvatar.sessionUUID) {
            if (channel === 'Hifi-unhighlight-entity') {
                try {
                    data = JSON.parse(message);
                    var hand = data.hand;
                    if (hand === dispatcherUtils.LEFT_HAND) {
                        leftHighlightNearbyEntities.removeEntityFromHighlightList(data.entityID);
                    } else if (hand === dispatcherUtils.RIGHT_HAND) {
                        rightHighlightNearbyEntities.removeEntityFromHighlightList(data.entityID);
                    }
                } catch (e) {
                    print("Failed to parse message");
                }
            } else if (channel === 'Hifi-unhighlight-all') {
                leftHighlightNearbyEntities.clearAll();
                rightHighlightNearbyEntities.clearAll();
            }
        }
    };
    var leftHighlightNearbyEntities = new HighlightNearbyEntities(dispatcherUtils.LEFT_HAND);
    var rightHighlightNearbyEntities = new HighlightNearbyEntities(dispatcherUtils.RIGHT_HAND);

    dispatcherUtils.enableDispatcherModule("LeftHighlightNearbyEntities", leftHighlightNearbyEntities);
    dispatcherUtils.enableDispatcherModule("RightHighlightNearbyEntities", rightHighlightNearbyEntities);

    function cleanup() {
        dispatcherUtils.disableDispatcherModule("LeftHighlightNearbyEntities");
        dispatcherUtils.disableDispatcherModule("RightHighlightNearbyEntities");
    }
    Messages.subscribe('Hifi-unhighlight-entity');
    Messages.subscribe('Hifi-unhighlight-all');
    Messages.messageReceived.connect(handleMessage);
    Script.scriptEnding.connect(cleanup);
}());
