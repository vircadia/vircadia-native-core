"use strict";

//  controllerDispatcher.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, controllerDispatcherPlugins, Controller, Vec3, getControllerWorldLocation */

Script.include("/~/system/libraries/utils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    var _this = this;

    // var LEFT_HAND = 0;
    // var RIGHT_HAND = 1;

    var NEAR_GRAB_RADIUS = 0.1;
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

        var leftControllerLocation = getControllerWorldLocation(Controller.Standard.LeftHand, true);
        var rightControllerLocation = getControllerWorldLocation(Controller.Standard.RightHand, true);

        var leftNearbyEntityIDs = Entities.findEntities(leftControllerLocation, NEAR_GRAB_RADIUS);
        var rightNearbyEntityIDs = Entities.findEntities(rightControllerLocation, NEAR_GRAB_RADIUS);

        var leftNearbyEntityProperties = {};
        leftNearbyEntityIDs.forEach(function (entityID) {
            var props = Entities.getEntityProperties(entityID, DISPATCHER_PROPERTIES);
            props.id = entityID;
            props.distanceFromController = Vec3.length(Vec3.subtract(leftControllerLocation, props.position));
            leftNearbyEntityProperties.push(props);
        });

        var rightNearbyEntityProperties = {};
        rightNearbyEntityIDs.forEach(function (entityID) {
            var props = Entities.getEntityProperties(entityID, DISPATCHER_PROPERTIES);
            props.id = entityID;
            props.distanceFromController = Vec3.length(Vec3.subtract(rightControllerLocation, props.position));
            rightNearbyEntityProperties.push(props);
        });


        var controllerData = {
            triggerValues: [this.leftTriggerValue, this.rightTriggerValue],
            triggerPresses: [this.leftTriggerPress, this.rightTriggerPress],
            controllerLocations: [ leftControllerLocation, rightControllerLocation ],
            nearbyEntityProperties: [ leftNearbyEntityProperties, rightNearbyEntityProperties ],
        };

        if (this.runningPluginName) {
            var plugin = controllerDispatcherPlugins[this.runningPluginName];
            if (!plugin || !plugin.run(controllerData)) {
                this.runningPluginName = null;
            }
        } else if (controllerDispatcherPlugins) {
            for (var pluginName in controllerDispatcherPlugins) {
                // TODO sort names by plugin.priority
                if (controllerDispatcherPlugins.hasOwnProperty(pluginName)) {
                    var candidatePlugin = controllerDispatcherPlugins[pluginName];
                    if (candidatePlugin.isReady(controllerData)) {
                        this.runningPluginName = candidatePlugin;
                        break;
                    }
                }
            }
        }
    };

    var MAPPING_NAME = "com.highfidelity.controllerDispatcher";
    var mapping = Controller.newMapping(MAPPING_NAME);
    mapping.from([Controller.Standard.RT]).peek().to(this.rightTriggerPress);
    mapping.from([Controller.Standard.RTClick]).peek().to(this.rightTriggerClicked);
    mapping.from([Controller.Standard.LT]).peek().to(this.leftTriggerPress);
    mapping.from([Controller.Standard.LTClick]).peek().to(this.leftTriggerClicked);
    Controller.enableMapping(MAPPING_NAME);

    this.cleanup = function () {
        Script.update.disconnect(this.update);
        Controller.disableMapping(MAPPING_NAME);
    };

    Script.scriptEnding.connect(this.cleanup);
    Script.update.connect(this.update);
}());
