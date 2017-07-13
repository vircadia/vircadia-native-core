//
//  lookAtTarget.js
//
//  Created by Thijs Wenker on 3/15/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals LookAtTarget:true */

LookAtTarget = function(sourceEntityID) {
    /* private variables */
    var _this,
        _options,
        _sourceEntityID,
        _sourceEntityProperties,
        REQUIRED_PROPERTIES = ['position', 'rotation', 'userData'],
        LOOK_AT_TAG = 'lookAtTarget';

    LookAtTarget = function(sourceEntityID) {
        _this = this;
        _sourceEntityID = sourceEntityID;
        _this.updateOptions();
    };

    /* private functions */
    var updateEntitySourceProperties = function() {
        _sourceEntityProperties = Entities.getEntityProperties(_sourceEntityID, REQUIRED_PROPERTIES);
    };

    var getUpdatedActionProperties = function() {
        return {
            targetRotation: _this.getLookAtRotation(),
            angularTimeScale: 0.1,
            ttl: 10
        };
    };

    var getNewActionProperties = function() {
        var newActionProperties = getUpdatedActionProperties();
        newActionProperties.tag = LOOK_AT_TAG;
        return newActionProperties;
    };

    LookAtTarget.prototype = {
        /* public functions */
        updateOptions: function() {
            updateEntitySourceProperties();
            _options = JSON.parse(_sourceEntityProperties.userData).lookAt;
        },
        getTargetPosition: function() {
            return Entities.getEntityProperties(_options.targetID).position;
        },
        getLookAtRotation: function() {
            _this.updateOptions();

            var newRotation = Quat.lookAt(_sourceEntityProperties.position, _this.getTargetPosition(), Vec3.UP);
            if (_options.rotationOffset !== undefined) {
                newRotation = Quat.multiply(newRotation, Quat.fromVec3Degrees(_options.rotationOffset));
            }
            if (_options.disablePitch || _options.disableYaw || _options.disablePitch) {
                var disabledAxis = _options.clearDisabledAxis ? Vec3.ZERO :
                    Quat.safeEulerAngles(_sourceEntityProperties.rotation);
                var newEulers = Quat.safeEulerAngles(newRotation);
                newRotation = Quat.fromVec3Degrees({
                    x: _options.disablePitch ? disabledAxis.x : newEulers.x,
                    y: _options.disableYaw ? disabledAxis.y : newEulers.y,
                    z: _options.disableRoll ? disabledAxis.z : newEulers.z
                });
            }
            return newRotation;
        },
        lookAtDirectly: function() {
            Entities.editEntity(_sourceEntityID, {rotation: _this.getLookAtRotation()});
        },
        lookAtByAction: function() {
            var actionIDs = Entities.getActionIDs(_sourceEntityID);
            var actionFound = false;
            actionIDs.forEach(function(actionID) {
                if (actionFound) {
                    return;
                }
                var actionArguments = Entities.getActionArguments(_sourceEntityID, actionID);
                if (actionArguments.tag === LOOK_AT_TAG) {
                    actionFound = true;
                    Entities.updateAction(_sourceEntityID, actionID, getUpdatedActionProperties());
                }
            });
            if (!actionFound) {
                Entities.addAction('tractor', _sourceEntityID, getNewActionProperties());
            }
        }
    };

    return new LookAtTarget(sourceEntityID);
};
