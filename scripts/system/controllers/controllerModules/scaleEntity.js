//  scaleEntity.js
//
//  Created by Dante Ruiz on  9/18/17
//
//  Grabs physically moveable entities with hydra-like controllers; it works for either near or far objects.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Vec3, MyAvatar, Entities, RIGHT_HAND */

(function() {
    var dispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");
    function ScaleEntity(hand) {
        this.hand = hand;
        this.grabbedThingID = false;
        this.scalingStartDistance = false;
        this.scalingStartDimensions = false;

        this.parameters = dispatcherUtils.makeDispatcherModuleParameters(
            120,
            this.hand === RIGHT_HAND ? ["rightHandTrigger"] : ["leftHandTrigger"],
            [],
            100
        );

        this.otherHand = function() {
            return this.hand === dispatcherUtils.RIGHT_HAND ? dispatcherUtils.LEFT_HAND : dispatcherUtils.RIGHT_HAND;
        };

        this.otherModule = function() {
            return this.hand === dispatcherUtils.RIGHT_HAND ? leftScaleEntity : rightScaleEntity;
        };

        this.bumperPressed = function(controllerData) {
            return ( controllerData.secondaryValues[this.hand] > dispatcherUtils.BUMPER_ON_VALUE);
        };

        this.getTargetProps = function(controllerData) {
            // nearbyEntityProperties is already sorted by length from controller
            var nearbyEntityProperties = controllerData.nearbyEntityProperties[this.hand];
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            for (var i = 0; i < nearbyEntityProperties.length; i++) {
                var props = nearbyEntityProperties[i];
                var handPosition = controllerData.controllerLocations[this.hand].position;
                var distance = Vec3.distance(props.position, handPosition);
                if (distance > dispatcherUtils.NEAR_GRAB_RADIUS * sensorScaleFactor) {
                    continue;
                }
                if ((dispatcherUtils.entityIsGrabbable(props) ||
                     dispatcherUtils.propsArePhysical(props)) && !props.locked) {
                    return props;
                }
            }
            return null;
        };

        this.isReady = function(controllerData) {
            var otherModule = this.otherModule();
            if (this.bumperPressed(controllerData) && otherModule.bumperPressed(controllerData)) {
                var thisHandTargetProps = this.getTargetProps(controllerData);
                var otherHandTargetProps = otherModule.getTargetProps(controllerData);
                if (thisHandTargetProps && otherHandTargetProps) {
                    if (thisHandTargetProps.id === otherHandTargetProps.id) {
                        this.grabbedThingID = thisHandTargetProps.id;
                        this.scalingStartDistance = Vec3.length(Vec3.subtract(controllerData.controllerLocations[this.hand].position,
                            controllerData.controllerLocations[this.otherHand()].position));
                        this.scalingStartDimensions = thisHandTargetProps.dimensions;
                        return dispatcherUtils.makeRunningValues(true, [], []);
                    }
                }
            }
            return dispatcherUtils.makeRunningValues(false, [], []);
        };

        this.run = function(controllerData) {
            var otherModule = this.otherModule();
            if (this.bumperPressed(controllerData) && otherModule.bumperPressed(controllerData)) {
                if (this.hand === dispatcherUtils.RIGHT_HAND) {
                    var scalingCurrentDistance =
                        Vec3.length(Vec3.subtract(controllerData.controllerLocations[this.hand].position,
                            controllerData.controllerLocations[this.otherHand()].position));
                    var currentRescale = scalingCurrentDistance / this.scalingStartDistance;
                    var newDimensions = Vec3.multiply(currentRescale, this.scalingStartDimensions);
                    Entities.editEntity(this.grabbedThingID, { dimensions: newDimensions });
                }
                return dispatcherUtils.makeRunningValues(true, [], []);
            }
            return dispatcherUtils.makeRunningValues(false, [], []);
        };
    }

    var leftScaleEntity = new ScaleEntity(dispatcherUtils.LEFT_HAND);
    var rightScaleEntity = new ScaleEntity(dispatcherUtils.RIGHT_HAND);

    dispatcherUtils.enableDispatcherModule("LeftScaleEntity", leftScaleEntity);
    dispatcherUtils.enableDispatcherModule("RightScaleEntity", rightScaleEntity);

    function cleanup() {
        dispatcherUtils.disableDispatcherModule("LeftScaleEntity");
        dispatcherUtils.disableDispatcherModule("RightScaleEntity");
    }
    Script.scriptEnding.connect(cleanup);
})();
