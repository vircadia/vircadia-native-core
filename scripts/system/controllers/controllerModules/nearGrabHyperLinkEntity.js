"use strict";

//  nearGrabHyperLinkEntity.js
//
//  Created by Dante Ruiz on 03/02/2018
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, MyAvatar, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule,
   makeDispatcherModuleParameters, makeRunningValues, TRIGGER_OFF_VALUE, NEAR_GRAB_RADIUS, BUMPER_ON_VALUE, AddressManager
*/

(function() {
    Script.include("/~/system/libraries/controllerDispatcherUtils.js");
    Script.include("/~/system/libraries/controllers.js");

    function NearGrabHyperLinkEntity(hand) {
        this.hand = hand;
        this.targetEntityID = null;
        this.hyperlink = "";

        this.parameters = makeDispatcherModuleParameters(
            125,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);


        this.getTargetProps = function(controllerData) {
            var nearbyEntitiesProperties = controllerData.nearbyEntityProperties[this.hand];
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            for (var i = 0; i < nearbyEntitiesProperties.length; i++) {
                var props = nearbyEntitiesProperties[i];
                if (props.distance > NEAR_GRAB_RADIUS * sensorScaleFactor) {
                    continue;
                }
                if (props.href !== "" && props.href !== undefined) {
                    return props;
                }
            }
            return null;
        };

        this.isReady = function(controllerData) {
            this.targetEntityID = null;
            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE &&
                controllerData.secondaryValues[this.hand] < TRIGGER_OFF_VALUE) {
                return makeRunningValues(false, [], []);
            }

            var targetProps = this.getTargetProps(controllerData);
            if (targetProps) {
                this.hyperlink = targetProps.href;
                this.targetEntityID = targetProps.id;
                return makeRunningValues(true, [], []);
            }

            return makeRunningValues(false, [], []);
        };

        this.run = function(controllerData) {
            if ((controllerData.triggerClicks[this.hand] < TRIGGER_OFF_VALUE &&
                 controllerData.secondaryValues[this.hand] < TRIGGER_OFF_VALUE) || this.hyperlink === "") {
                return makeRunningValues(false, [], []);
            }

            if (controllerData.triggerClicks[this.hand] ||
                controllerData.secondaryValues[this.hand] > BUMPER_ON_VALUE) {
                AddressManager.handleLookupString(this.hyperlink);
                return makeRunningValues(false, [], []);
            }

            return makeRunningValues(true, [], []);
        };
    }

    var leftNearGrabHyperLinkEntity = new NearGrabHyperLinkEntity(LEFT_HAND);
    var rightNearGrabHyperLinkEntity = new NearGrabHyperLinkEntity(RIGHT_HAND);

    enableDispatcherModule("LeftNearGrabHyperLink", leftNearGrabHyperLinkEntity);
    enableDispatcherModule("RightNearGrabHyperLink", rightNearGrabHyperLinkEntity);

    function cleanup() {
        disableDispatcherModule("LeftNearGrabHyperLink");
        disableDispatcherModule("RightNearGrabHyperLink");

    }

    Script.scriptEnding.connect(cleanup);
}());
