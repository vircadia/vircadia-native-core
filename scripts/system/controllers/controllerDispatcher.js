"use strict";

//  controllerDispatcher.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, Overlays, controllerDispatcherPlugins, Controller, Vec3, getControllerWorldLocation,
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


    var TARGET_UPDATE_HZ = 60; // 50hz good enough, but we're using update
    var BASIC_TIMER_INTERVAL_MS = 1000 / TARGET_UPDATE_HZ;
    var lastInterval = Date.now();
    var intervalCount = 0;
    var totalDelta = 0;
    var totalVariance = 0;
    var highVarianceCount = 0;
    var veryhighVarianceCount = 0;

    // a module can occupy one or more "activity" slots while it's running.  If all the required slots for a module are
    // not set to false (not in use), a module cannot start.  When a module is using a slot, that module's name
    // is stored as the value, rather than false.
    this.activitySlots = {
        leftHand: false,
        rightHand: false
    };

    this.slotsAreAvailableForPlugin = function (plugin) {
        for (var i = 0; i < plugin.parameters.activitySlots.length; i++) {
            if (_this.activitySlots[plugin.parameters.activitySlots[i]]) {
                return false; // something is already using a slot which _this plugin requires
            }
        }
        return true;
    };

    this.markSlots = function (plugin, used) {
        for (var i = 0; i < plugin.parameters.activitySlots.length; i++) {
            _this.activitySlots[plugin.parameters.activitySlots[i]] = used;
        }
    };

    this.unmarkSlotsForPluginName = function (runningPluginName) {
        // this is used to free activity-slots when a plugin is deactivated while it's running.
        for (var activitySlot in _this.activitySlots) {
            if (activitySlot.hasOwnProperty(activitySlot) && _this.activitySlots[activitySlot] == runningPluginName) {
                _this.activitySlots[activitySlot] = false;
            }
        }
    };

    this.runningPluginNames = {};
    this.leftTriggerValue = 0;
    this.leftTriggerClicked = 0;
    this.rightTriggerValue = 0;
    this.rightTriggerClicked = 0;


    this.leftTriggerPress = function (value) {
        _this.leftTriggerValue = value;
    };

    this.leftTriggerClick = function (value) {
        _this.leftTriggerClicked = value;
    };

    this.rightTriggerPress = function (value) {
        _this.rightTriggerValue = value;
    };

    this.rightTriggerClick = function (value) {
        _this.rightTriggerClicked = value;
    };

    this.dataGatherers = {};
    this.dataGatherers.leftControllerLocation = function () {
        return getControllerWorldLocation(Controller.Standard.LeftHand, true);
    };
    this.dataGatherers.rightControllerLocation = function () {
        return getControllerWorldLocation(Controller.Standard.RightHand, true);
    };

    this.updateTimings = function () {
        intervalCount++;
        var thisInterval = Date.now();
        var deltaTimeMsec = thisInterval - lastInterval;
        var deltaTime = deltaTimeMsec / 1000;
        lastInterval = thisInterval;

        totalDelta += deltaTimeMsec;

        var variance = Math.abs(deltaTimeMsec - BASIC_TIMER_INTERVAL_MS);
        totalVariance += variance;

        if (variance > 1) {
            highVarianceCount++;
        }

        if (variance > 5) {
            veryhighVarianceCount++;
        }

        return deltaTime;
    };

    this.update = function () {
        var deltaTime =  this.updateTimings();

        var controllerLocations = [_this.dataGatherers.leftControllerLocation(),
                                   _this.dataGatherers.rightControllerLocation()];

        // find 3d overlays near each hand
        var nearbyOverlayIDs = [];
        var h;
        for (h = LEFT_HAND; h <= RIGHT_HAND; h++) {
            // todo: check controllerLocations[h].valid
            var nearbyOverlays = Overlays.findOverlays(controllerLocations[h].position, NEAR_MIN_RADIUS);
            var makeOverlaySorter = function (handIndex) {
                return function (a, b) {
                    var aPosition = Overlays.getProperty(a, "position");
                    var aDistance = Vec3.distance(aPosition, controllerLocations[handIndex]);
                    var bPosition = Overlays.getProperty(b, "position");
                    var bDistance = Vec3.distance(bPosition, controllerLocations[handIndex]);
                    return aDistance - bDistance;
                };
            };
            nearbyOverlays.sort(makeOverlaySorter(h));
            nearbyOverlayIDs.push(nearbyOverlays);
        }

        // find entities near each hand
        var nearbyEntityProperties = [[], []];
        for (h = LEFT_HAND; h <= RIGHT_HAND; h++) {
            // todo: check controllerLocations[h].valid
            var controllerPosition = controllerLocations[h].position;
            var nearbyEntityIDs = Entities.findEntities(controllerPosition, NEAR_MIN_RADIUS);
            for (var j = 0; j < nearbyEntityIDs.length; j++) {
                var entityID = nearbyEntityIDs[j];
                var props = Entities.getEntityProperties(entityID, DISPATCHER_PROPERTIES);
                props.id = entityID;
                props.distanceFromController = Vec3.length(Vec3.subtract(controllerPosition, props.position));
                if (props.distanceFromController < NEAR_MAX_RADIUS) {
                    nearbyEntityProperties[h].push(props);
                }
            }
            // sort by distance from each hand
            var makeSorter = function (handIndex) {
                return function (a, b) {
                    var aDistance = Vec3.distance(a.position, controllerLocations[handIndex]);
                    var bDistance = Vec3.distance(b.position, controllerLocations[handIndex]);
                    return aDistance - bDistance;
                };
            };
            nearbyEntityProperties[h].sort(makeSorter(h));
        }

        // bundle up all the data about the current situation
        var controllerData = {
            triggerValues: [_this.leftTriggerValue, _this.rightTriggerValue],
            triggerClicks: [_this.leftTriggerClicked, _this.rightTriggerClicked],
            controllerLocations: controllerLocations,
            nearbyEntityProperties: nearbyEntityProperties,
            nearbyOverlayIDs: nearbyOverlayIDs
        };

        // print("QQQ dispatcher " + JSON.stringify(_this.runningPluginNames) + " : " + JSON.stringify(_this.activitySlots));

        // check for plugins that would like to start
        for (var pluginName in controllerDispatcherPlugins) {
            // TODO sort names by plugin.priority
            if (controllerDispatcherPlugins.hasOwnProperty(pluginName)) {
                var candidatePlugin = controllerDispatcherPlugins[pluginName];
                if (_this.slotsAreAvailableForPlugin(candidatePlugin) && candidatePlugin.isReady(controllerData, deltaTime)) {
                    // this plugin will start.  add it to the list of running plugins and mark the
                    // activity-slots which this plugin consumes as "in use"
                    _this.runningPluginNames[pluginName] = true;
                    _this.markSlots(candidatePlugin, pluginName);
                }
            }
        }

        // give time to running plugins
        for (var runningPluginName in _this.runningPluginNames) {
            if (_this.runningPluginNames.hasOwnProperty(runningPluginName)) {
                var plugin = controllerDispatcherPlugins[runningPluginName];
                if (!plugin) {
                    // plugin was deactivated while running.  find the activity-slots it was using and make
                    // them available.
                    delete _this.runningPluginNames[runningPluginName];
                    _this.unmarkSlotsForPluginName(runningPluginName);
                } else if (!plugin.run(controllerData, deltaTime)) {
                    // plugin is finished running, for now.  remove it from the list
                    // of running plugins and mark its activity-slots as "not in use"
                    delete _this.runningPluginNames[runningPluginName];
                    _this.markSlots(plugin, false);
                }
            }
        }
    };

    var MAPPING_NAME = "com.highfidelity.controllerDispatcher";
    var mapping = Controller.newMapping(MAPPING_NAME);
    mapping.from([Controller.Standard.RT]).peek().to(_this.rightTriggerPress);
    mapping.from([Controller.Standard.RTClick]).peek().to(_this.rightTriggerClick);
    mapping.from([Controller.Standard.LT]).peek().to(_this.leftTriggerPress);
    mapping.from([Controller.Standard.LTClick]).peek().to(_this.leftTriggerClick);
    Controller.enableMapping(MAPPING_NAME);

    this.cleanup = function () {
        Script.update.disconnect(_this.update);
        Controller.disableMapping(MAPPING_NAME);
    };

    Script.scriptEnding.connect(this.cleanup);
    Script.update.connect(this.update);
}());
