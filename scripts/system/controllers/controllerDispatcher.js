"use strict";

//  controllerDispatcher.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/*jslint bitwise: true */

/* global Script, Entities, Overlays, Controller, Vec3, Quat, getControllerWorldLocation, RayPick,
   controllerDispatcherPlugins, controllerDispatcherPluginsNeedSort, entityIsGrabbable,
   LEFT_HAND, RIGHT_HAND, NEAR_GRAB_PICK_RADIUS, DEFAULT_SEARCH_SPHERE_DISTANCE, DISPATCHER_PROPERTIES
*/

controllerDispatcherPlugins = {};
controllerDispatcherPluginsNeedSort = false;

Script.include("/~/system/libraries/utils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/controllers/controllerDispatcherUtils.js");

(function() {
    var _this = this;

    var NEAR_MIN_RADIUS = 0.1;
    var NEAR_MAX_RADIUS = 1.0;

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
        rightHand: false,
        mouse: false
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
    this.leftSecondaryValue = 0;
    this.rightSecondaryValue = 0;

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
    this.leftSecondaryPress = function (value) {
        _this.leftSecondaryValue = value;
    };
    this.rightSecondaryPress = function (value) {
        _this.rightSecondaryValue = value;
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

        if (controllerDispatcherPluginsNeedSort) {
            this.orderedPluginNames = [];
            for (var pluginName in controllerDispatcherPlugins) {
                if (controllerDispatcherPlugins.hasOwnProperty(pluginName)) {
                    this.orderedPluginNames.push(pluginName);
                }
            }
            this.orderedPluginNames.sort(function (a, b) {
                return controllerDispatcherPlugins[a].parameters.priority -
                    controllerDispatcherPlugins[b].parameters.priority;
            });

            // print("controllerDispatcher -- new plugin order: " + JSON.stringify(this.orderedPluginNames));
            var output = "controllerDispatcher -- new plugin order: ";
            for (var k = 0; k < this.orderedPluginNames.length; k++) {
                var dbgPluginName = this.orderedPluginNames[k];
                var priority = controllerDispatcherPlugins[dbgPluginName].parameters.priority;
                output += dbgPluginName + ":" + priority;
                if (k + 1 < this.orderedPluginNames.length) {
                    output += ", ";
                }
            }
            print(output);

            controllerDispatcherPluginsNeedSort = false;
        }

        var controllerLocations = [_this.dataGatherers.leftControllerLocation(),
                                   _this.dataGatherers.rightControllerLocation()];

        // find 3d overlays near each hand
        var nearbyOverlayIDs = [];
        var h;
        for (h = LEFT_HAND; h <= RIGHT_HAND; h++) {
            // todo: check controllerLocations[h].valid
            var nearbyOverlays = Overlays.findOverlays(controllerLocations[h].position, NEAR_MAX_RADIUS);
            nearbyOverlays.sort(function (a, b) {
                var aPosition = Overlays.getProperty(a, "position");
                var aDistance = Vec3.distance(aPosition, controllerLocations[h].position);
                var bPosition = Overlays.getProperty(b, "position");
                var bDistance = Vec3.distance(bPosition, controllerLocations[h].position);
                return aDistance - bDistance;
            });
            nearbyOverlayIDs.push(nearbyOverlays);
        }

        // find entities near each hand
        var nearbyEntityProperties = [[], []];
        var nearbyEntityPropertiesByID = {};
        for (h = LEFT_HAND; h <= RIGHT_HAND; h++) {
            // todo: check controllerLocations[h].valid
            var controllerPosition = controllerLocations[h].position;
            var nearbyEntityIDs = Entities.findEntities(controllerPosition, NEAR_MAX_RADIUS);
            for (var j = 0; j < nearbyEntityIDs.length; j++) {
                var entityID = nearbyEntityIDs[j];
                var props = Entities.getEntityProperties(entityID, DISPATCHER_PROPERTIES);
                props.id = entityID;
                nearbyEntityPropertiesByID[entityID] = props;
                nearbyEntityProperties[h].push(props);
            }
        }

        // raypick for each controller
        var rayPicks = [
            RayPick.getPrevRayPickResult(_this.leftControllerRayPick),
            RayPick.getPrevRayPickResult(_this.rightControllerRayPick)
        ];
        var hudRayPicks = [
            RayPick.getPrevRayPickResult(_this.leftControllerHudRayPick),
            RayPick.getPrevRayPickResult(_this.rightControllerHudRayPick)
        ];
        // if the pickray hit something very nearby, put it into the nearby entities list
        for (h = LEFT_HAND; h <= RIGHT_HAND; h++) {

            // XXX find a way to extract searchRay from samuel's stuff
            rayPicks[h].searchRay = {
                origin: controllerLocations[h].position,
                direction: Quat.getUp(controllerLocations[h].orientation),
                length: 1000
            };

            if (rayPicks[h].type == RayPick.INTERSECTED_ENTITY) {
                // XXX check to make sure this one isn't already in nearbyEntityProperties?
                if (rayPicks[h].distance < NEAR_GRAB_PICK_RADIUS) {
                    var nearEntityID = rayPicks[h].objectID;
                    var nearbyProps = Entities.getEntityProperties(nearEntityID, DISPATCHER_PROPERTIES);
                    nearbyProps.id = nearEntityID;
                    nearbyEntityPropertiesByID[nearEntityID] = nearbyProps;
                    nearbyEntityProperties[h].push(nearbyProps);
                }
            }

            // sort by distance from each hand
            nearbyEntityProperties[h].sort(function (a, b) {
                var aDistance = Vec3.distance(a.position, controllerLocations[h].position);
                var bDistance = Vec3.distance(b.position, controllerLocations[h].position);
                return aDistance - bDistance;
            });
        }

        // bundle up all the data about the current situation
        var controllerData = {
            triggerValues: [_this.leftTriggerValue, _this.rightTriggerValue],
            triggerClicks: [_this.leftTriggerClicked, _this.rightTriggerClicked],
            secondaryValues: [_this.leftSecondaryValue, _this.rightSecondaryValue],
            controllerLocations: controllerLocations,
            nearbyEntityProperties: nearbyEntityProperties,
            nearbyEntityPropertiesByID: nearbyEntityPropertiesByID,
            nearbyOverlayIDs: nearbyOverlayIDs,
            rayPicks: rayPicks,
            hudRayPicks: hudRayPicks
        };

        // check for plugins that would like to start.  ask in order of increasing priority value
        for (var pluginIndex = 0; pluginIndex < this.orderedPluginNames.length; pluginIndex++) {
            var orderedPluginName = this.orderedPluginNames[pluginIndex];
            var candidatePlugin = controllerDispatcherPlugins[orderedPluginName];

            if (_this.slotsAreAvailableForPlugin(candidatePlugin)) {
                //print(orderedPluginName);
                var readiness = candidatePlugin.isReady(controllerData, deltaTime);
                if (readiness.active) {
                    // this plugin will start.  add it to the list of running plugins and mark the
                    // activity-slots which this plugin consumes as "in use"
                    _this.runningPluginNames[orderedPluginName] = true;
                    _this.markSlots(candidatePlugin, orderedPluginName);
                }
            }
        }

        // print("QQQ running plugins: " + JSON.stringify(_this.runningPluginNames));

        // give time to running plugins
        for (var runningPluginName in _this.runningPluginNames) {
            if (_this.runningPluginNames.hasOwnProperty(runningPluginName)) {
                var plugin = controllerDispatcherPlugins[runningPluginName];
                if (!plugin) {
                    // plugin was deactivated while running.  find the activity-slots it was using and make
                    // them available.
                    delete _this.runningPluginNames[runningPluginName];
                    _this.unmarkSlotsForPluginName(runningPluginName);
                } else {
                    var runningness = plugin.run(controllerData, deltaTime);
                    if (!runningness.active) {
                        // plugin is finished running, for now.  remove it from the list
                        // of running plugins and mark its activity-slots as "not in use"
                        delete _this.runningPluginNames[runningPluginName];
                        _this.markSlots(plugin, false);
                    }
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

    mapping.from([Controller.Standard.RB]).peek().to(_this.rightSecondaryPress);
    mapping.from([Controller.Standard.LB]).peek().to(_this.leftSecondaryPress);
    mapping.from([Controller.Standard.LeftGrip]).peek().to(_this.leftSecondaryPress);
    mapping.from([Controller.Standard.RightGrip]).peek().to(_this.rightSecondaryPress);

    Controller.enableMapping(MAPPING_NAME);


    this.mouseRayPick = RayPick.createRayPick({
        joint: "Mouse",
        filter: RayPick.PICK_ENTITIES | RayPick.PICK_OVERLAYS,
        enabled: true
    });

    this.mouseHudRayPick = RayPick.createRayPick({
        joint: "Mouse",
        filter: RayPick.PICK_HUD,
        enabled: true
    });

    this.leftControllerRayPick = RayPick.createRayPick({
        joint: "_CONTROLLER_LEFTHAND",
        filter: RayPick.PICK_ENTITIES | RayPick.PICK_OVERLAYS,
        enabled: true,
        maxDistance: DEFAULT_SEARCH_SPHERE_DISTANCE
    });
    this.leftControllerHudRayPick = RayPick.createRayPick({
        joint: "_CONTROLLER_LEFTHAND",
        filter: RayPick.PICK_HUD,
        enabled: true,
        maxDistance: DEFAULT_SEARCH_SPHERE_DISTANCE
    });
    this.rightControllerRayPick = RayPick.createRayPick({
        joint: "_CONTROLLER_RIGHTHAND",
        filter: RayPick.PICK_ENTITIES | RayPick.PICK_OVERLAYS,
        enabled: true,
        maxDistance: DEFAULT_SEARCH_SPHERE_DISTANCE
    });
    this.rightControllerHudRayPick = RayPick.createRayPick({
        joint: "_CONTROLLER_RIGHTHAND",
        filter: RayPick.PICK_HUD,
        enabled: true,
        maxDistance: DEFAULT_SEARCH_SPHERE_DISTANCE
    });


    this.cleanup = function () {
        Script.update.disconnect(_this.update);
        Controller.disableMapping(MAPPING_NAME);
        // RayPick.removeRayPick(_this.mouseRayPick);
        RayPick.removeRayPick(_this.leftControllerRayPick);
        RayPick.removeRayPick(_this.rightControllerRayPick);
        RayPick.removeRayPick(_this.rightControllerHudRayPick);
        RayPick.removeRayPick(_this.leftControllerHudRayPick);
    };

    Script.scriptEnding.connect(this.cleanup);
    Script.update.connect(this.update);
}());
