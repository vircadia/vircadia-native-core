"use strict";

//  cloneEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, RIGHT_HAND, LEFT_HAND,
   enableDispatcherModule, disableDispatcherModule, getGrabbableData, Vec3,
   TRIGGER_ON_VALUE, TRIGGER_OFF_VALUE, makeDispatcherModuleParameters, makeRunningValues, NEAR_GRAB_RADIUS
*/

Script.include("/~/system/controllers/controllerDispatcherUtils.js");

// Object assign  polyfill
if (typeof Object.assign != 'function') {
    Object.assign = function(target, varArgs) {
        if (target === null) {
            throw new TypeError('Cannot convert undefined or null to object');
        }
        var to = Object(target);
        for (var index = 1; index < arguments.length; index++) {
            var nextSource = arguments[index];
            if (nextSource !== null) {
                for (var nextKey in nextSource) {
                    if (Object.prototype.hasOwnProperty.call(nextSource, nextKey)) {
                        to[nextKey] = nextSource[nextKey];
                    }
                }
            }
        }
        return to;
    };
}

(function() {

    function entityIsCloneable(props) {
        var grabbableData = getGrabbableData(props);
        return grabbableData.cloneable;
    }

    function CloneEntity(hand) {
        this.hand = hand;
        this.grabbing = false;
        this.previousParentID = {};
        this.previousParentJointIndex = {};
        this.previouslyUnhooked = {};

        this.parameters = makeDispatcherModuleParameters(
            150,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.getTargetProps = function (controllerData) {
            // nearbyEntityProperties is already sorted by length from controller
            var nearbyEntityProperties = controllerData.nearbyEntityProperties[this.hand];
            for (var i = 0; i < nearbyEntityProperties.length; i++) {
                var props = nearbyEntityProperties[i];
                var handPosition = controllerData.controllerLocations[this.hand].position;
                var distance = Vec3.distance(props.position, handPosition);
                if (distance > NEAR_GRAB_RADIUS) {
                    break;
                }
                if (entityIsCloneable(props)) {
                    return props;
                }
            }
            return null;
        };

        this.isReady = function (controllerData) {
            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE) {
                this.waiting = false;
                return makeRunningValues(false, [], []);
            }

            if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE) {
                if (this.waiting) {
                    return makeRunningValues(false, [], []);
                }
                this.waiting = true;
            }

            var cloneableProps = this.getTargetProps(controllerData);
            if (!cloneableProps) {
                return makeRunningValues(false, [], []);
            }

            // we need all the properties, for this
            cloneableProps = Entities.getEntityProperties(cloneableProps.id);

            var worldEntityProps = controllerData.nearbyEntityProperties[this.hand];
            var count = 0;
            worldEntityProps.forEach(function(itemWE) {
                if (itemWE.name.indexOf('-clone-' + cloneableProps.id) !== -1) {
                    count++;
                }
            });

            var grabInfo = getGrabbableData(cloneableProps);

            var limit = grabInfo.cloneLimit ? grabInfo.cloneLimit : 0;
            if (count >= limit && limit !== 0) {
                return makeRunningValues(false, [], []);
            }

            cloneableProps.name = cloneableProps.name + '-clone-' + cloneableProps.id;
            var lifetime = grabInfo.cloneLifetime ? grabInfo.cloneLifetime : 300;
            var dynamic = grabInfo.cloneDynamic ? grabInfo.cloneDynamic : false;
            var cUserData = Object.assign({}, cloneableProps.userData);
            var cProperties = Object.assign({}, cloneableProps);

            try {
                delete cUserData.grabbableKey.cloneLifetime;
                delete cUserData.grabbableKey.cloneable;
                delete cUserData.grabbableKey.cloneDynamic;
                delete cUserData.grabbableKey.cloneLimit;
                delete cProperties.id;
            } catch(e) {
            }

            cProperties.dynamic = dynamic;
            cProperties.locked = false;
            if (!cUserData.grabbableKey) {
                cUserData.grabbableKey = {};
            }
            cUserData.grabbableKey.triggerable = true;
            cUserData.grabbableKey.grabbable = true;
            cProperties.lifetime = lifetime;
            cProperties.userData = JSON.stringify(cUserData);
            // var cloneID =
            Entities.addEntity(cProperties);

            return makeRunningValues(false, [], []);
        };

        this.run = function (controllerData) {
        };

        this.cleanup = function () {
        };
    }

    var leftNearParentingGrabEntity = new CloneEntity(LEFT_HAND);
    var rightNearParentingGrabEntity = new CloneEntity(RIGHT_HAND);

    enableDispatcherModule("LeftNearParentingGrabEntity", leftNearParentingGrabEntity);
    enableDispatcherModule("RightNearParentingGrabEntity", rightNearParentingGrabEntity);

    this.cleanup = function () {
        leftNearParentingGrabEntity.cleanup();
        rightNearParentingGrabEntity.cleanup();
        disableDispatcherModule("LeftNearParentingGrabEntity");
        disableDispatcherModule("RightNearParentingGrabEntity");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
