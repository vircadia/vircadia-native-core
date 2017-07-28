"use strict";

//  controllerDispatcher.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, controllerDispatcherPlugins, Controller, Vec3, getControllerWorldLocation,
   LEFT_HAND, RIGHT_HAND */

controllerDispatcherPlugins = {};

Script.include("/~/system/libraries/utils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/controllers/controllerDispatcherUtils.js");

(function() {
    var _this = this;

    var NEAR_MIN_RADIUS = 0.1;
    var NEAR_MAX_RADIUS = 1.0;


    var DISPATCHER_PROPERTIES = [
        "position",
        "registrationPoint",
        "rotation",
        "gravity",
        "collidesWith",
        "dynamic",
        "collisionless",
        "locked",
        "name",
        "shapeType",
        "parentID",
        "parentJointIndex",
        "density",
        "dimensions",
        "userData"
    ];

    this.runningPluginName = null;
    this.leftTriggerValue = 0;
    this.leftTriggerClicked = 0;
    this.rightTriggerValue = 0;
    this.rightTriggerClicked = 0;


    this.leftTriggerPress = function(value) {
        _this.leftTriggerValue = value;
    };

    this.leftTriggerClick = function(value) {
        _this.leftTriggerClicked = value;
    };

    this.rightTriggerPress = function(value) {
        _this.rightTriggerValue = value;
    };

    this.rightTriggerClick = function(value) {
        _this.rightTriggerClicked = value;
    };

    this.update = function () {

        var controllerLocations = [getControllerWorldLocation(Controller.Standard.LeftHand, true),
                                   getControllerWorldLocation(Controller.Standard.RightHand, true)];

        var nearbyEntityProperties = [[], []];
        for (var i = LEFT_HAND; i <= RIGHT_HAND; i ++) {
            // todo: check controllerLocations[i].valid
            var controllerPosition = controllerLocations[i].position;
            var nearbyEntityIDs = Entities.findEntities(controllerPosition, NEAR_MIN_RADIUS);
            for (var j = 0; j < nearbyEntityIDs.length; j++) {
                var entityID = nearbyEntityIDs[j];
                var props = Entities.getEntityProperties(entityID, DISPATCHER_PROPERTIES);
                props.id = entityID;
                props.distanceFromController = Vec3.length(Vec3.subtract(controllerPosition, props.position));
                if (props.distanceFromController < NEAR_MAX_RADIUS) {
                    nearbyEntityProperties[i].push(props);
                }
            }
        }

        var controllerData = {
            triggerValues: [_this.leftTriggerValue, _this.rightTriggerValue],
            triggerClicks: [_this.leftTriggerClicked, _this.rightTriggerClicked],
            controllerLocations: controllerLocations,
            nearbyEntityProperties: nearbyEntityProperties,
        };

        if (_this.runningPluginName) {
            var plugin = controllerDispatcherPlugins[_this.runningPluginName];
            if (!plugin || !plugin.run(controllerData)) {
                _this.runningPluginName = null;
            }
        } else if (controllerDispatcherPlugins) {
            for (var pluginName in controllerDispatcherPlugins) {
                // TODO sort names by plugin.priority
                if (controllerDispatcherPlugins.hasOwnProperty(pluginName)) {
                    var candidatePlugin = controllerDispatcherPlugins[pluginName];
                    if (candidatePlugin.isReady(controllerData)) {
                        _this.runningPluginName = pluginName;
                        break;
                    }
                }
            }
        }
    };

    var MAPPING_NAME = "com.highfidelity.controllerDispatcher";
    var mapping = Controller.newMapping(MAPPING_NAME);
    mapping.from([Controller.Standard.RT]).peek().to(this.rightTriggerPress);
    mapping.from([Controller.Standard.RTClick]).peek().to(this.rightTriggerClick);
    mapping.from([Controller.Standard.LT]).peek().to(this.leftTriggerPress);
    mapping.from([Controller.Standard.LTClick]).peek().to(this.leftTriggerClick);
    Controller.enableMapping(MAPPING_NAME);

    this.cleanup = function () {
        Script.update.disconnect(this.update);
        Controller.disableMapping(MAPPING_NAME);
    };

    Script.scriptEnding.connect(this.cleanup);
    Script.update.connect(this.update);
}());
