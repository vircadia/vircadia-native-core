"use strict";

//  controllerDispatcher.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Entities, Overlays, Controller, Vec3, Quat, getControllerWorldLocation, RayPick,
   controllerDispatcherPlugins:true, controllerDispatcherPluginsNeedSort:true,
   LEFT_HAND, RIGHT_HAND, NEAR_GRAB_PICK_RADIUS, DEFAULT_SEARCH_SPHERE_DISTANCE, DISPATCHER_PROPERTIES,
   getGrabPointSphereOffset, HMD, MyAvatar, Messages, findHandChildEntities, Pointers, PickType, COLORS_GRAB_SEARCHING_HALF_SQUEEZE
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD, Picks, TRIGGER_ON_VALUE, PointerManager
*/

controllerDispatcherPlugins = {};
controllerDispatcherPluginsNeedSort = false;

Script.include("/~/system/libraries/utils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/libraries/controllerDispatcherUtils.js");

(function() {
    Script.include("/~/system/libraries/pointersUtils.js");
    var NEAR_MAX_RADIUS = 0.1;

    var TARGET_UPDATE_HZ = 60; // 50hz good enough, but we're using update
    var BASIC_TIMER_INTERVAL_MS = 1000 / TARGET_UPDATE_HZ;

    var PROFILE = false;
    var DEBUG = false;

    if (typeof Test !== "undefined") {
        PROFILE = true;
    }

    function ControllerDispatcher() {
        var _this = this;
        this.lastInterval = Date.now();
        this.intervalCount = 0;
        this.totalDelta = 0;
        this.totalVariance = 0;
        this.highVarianceCount = 0;
        this.veryhighVarianceCount = 0;
        this.tabletID = null;
        this.blacklist = [];
        this.pointerManager = new PointerManager();

        // a module can occupy one or more "activity" slots while it's running.  If all the required slots for a module are
        // not set to false (not in use), a module cannot start.  When a module is using a slot, that module's name
        // is stored as the value, rather than false.
        this.activitySlots = {
            leftHand: false,
            rightHand: false,
            rightHandTrigger: false,
            leftHandTrigger: false,
            rightHandEquip: false,
            leftHandEquip: false,
            mouse: false
        };

        this.laserVisibleStatus = [false, false, false, false];
        this.laserLockStatus = [false, false, false, false];

        this.slotsAreAvailableForPlugin = function (plugin) {
            for (var i = 0; i < plugin.parameters.activitySlots.length; i++) {
                if (_this.activitySlots[plugin.parameters.activitySlots[i]]) {
                    return false; // something is already using a slot which _this plugin requires
                }
            }
            return true;
        };

        this.markSlots = function (plugin, pluginName) {
            for (var i = 0; i < plugin.parameters.activitySlots.length; i++) {
                _this.activitySlots[plugin.parameters.activitySlots[i]] = pluginName;
            }
        };

        this.unmarkSlotsForPluginName = function (runningPluginName) {
            // this is used to free activity-slots when a plugin is deactivated while it's running.
            for (var activitySlot in _this.activitySlots) {
                if (activitySlot.hasOwnProperty(activitySlot) && _this.activitySlots[activitySlot] === runningPluginName) {
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
            _this.intervalCount++;
            var thisInterval = Date.now();
            var deltaTimeMsec = thisInterval - _this.lastInterval;
            var deltaTime = deltaTimeMsec / 1000;
            _this.lastInterval = thisInterval;

            _this.totalDelta += deltaTimeMsec;

            var variance = Math.abs(deltaTimeMsec - BASIC_TIMER_INTERVAL_MS);
            _this.totalVariance += variance;

            if (variance > 1) {
                _this.highVarianceCount++;
            }

            if (variance > 5) {
                _this.veryhighVarianceCount++;
            }

            return deltaTime;
        };

        this.setIgnoreTablet = function() {
            if (HMD.tabletID !== this.tabletID) {
                this.tabletID = HMD.tabletID;
                Pointers.setIgnoreItems(_this.leftPointer, _this.blacklist.concat([HMD.tabletID]));
                Pointers.setIgnoreItems(_this.rightPointer, _this.blacklist.concat([HMD.tabletID]));
            }
        };

        this.update = function () {
            try {
                _this.updateInternal();
            }  catch (e) {
                print(e);
            }
            Script.setTimeout(_this.update, BASIC_TIMER_INTERVAL_MS);
        };

        this.updateInternal = function () {
            if (PROFILE) {
                Script.beginProfileRange("dispatch.pre");
            }
            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            var deltaTime = _this.updateTimings();
            _this.setIgnoreTablet();

            if (controllerDispatcherPluginsNeedSort) {
                _this.orderedPluginNames = [];
                for (var pluginName in controllerDispatcherPlugins) {
                    if (controllerDispatcherPlugins.hasOwnProperty(pluginName)) {
                        _this.orderedPluginNames.push(pluginName);
                    }
                }
                _this.orderedPluginNames.sort(function (a, b) {
                    return controllerDispatcherPlugins[a].parameters.priority -
                        controllerDispatcherPlugins[b].parameters.priority;
                });

                var output = "controllerDispatcher -- new plugin order: ";
                for (var k = 0; k < _this.orderedPluginNames.length; k++) {
                    var dbgPluginName = _this.orderedPluginNames[k];
                    var priority = controllerDispatcherPlugins[dbgPluginName].parameters.priority;
                    output += dbgPluginName + ":" + priority;
                    if (k + 1 < _this.orderedPluginNames.length) {
                        output += ", ";
                    }
                }

                controllerDispatcherPluginsNeedSort = false;
            }

            if (PROFILE) {
                Script.endProfileRange("dispatch.pre");
            }

            if (PROFILE) {
                Script.beginProfileRange("dispatch.gather");
            }

            var controllerLocations = [
                _this.dataGatherers.leftControllerLocation(),
                _this.dataGatherers.rightControllerLocation()
            ];

            // find 3d overlays near each hand
            var nearbyOverlayIDs = [];
            var h;
            for (h = LEFT_HAND; h <= RIGHT_HAND; h++) {
                if (controllerLocations[h].valid) {
                    var nearbyOverlays =
                        Overlays.findOverlays(controllerLocations[h].position, NEAR_MAX_RADIUS * sensorScaleFactor);
                    nearbyOverlays.sort(function (a, b) {
                        var aPosition = Overlays.getProperty(a, "position");
                        var aDistance = Vec3.distance(aPosition, controllerLocations[h].position);
                        var bPosition = Overlays.getProperty(b, "position");
                        var bDistance = Vec3.distance(bPosition, controllerLocations[h].position);
                        return aDistance - bDistance;
                    });
                    nearbyOverlayIDs.push(nearbyOverlays);
                } else {
                    nearbyOverlayIDs.push([]);
                }
            }

            // find entities near each hand
            var nearbyEntityProperties = [[], []];
            var nearbyEntityPropertiesByID = {};
            for (h = LEFT_HAND; h <= RIGHT_HAND; h++) {
                if (controllerLocations[h].valid) {
                    var controllerPosition = controllerLocations[h].position;
                    var nearbyEntityIDs = Entities.findEntities(controllerPosition, NEAR_MAX_RADIUS * sensorScaleFactor);
                    for (var j = 0; j < nearbyEntityIDs.length; j++) {
                        var entityID = nearbyEntityIDs[j];
                        var props = Entities.getEntityProperties(entityID, DISPATCHER_PROPERTIES);
                        props.id = entityID;
                        props.distance = Vec3.distance(props.position, controllerLocations[h].position);
                        nearbyEntityPropertiesByID[entityID] = props;
                        nearbyEntityProperties[h].push(props);
                    }
                }
            }

            // raypick for each controller
            var rayPicks = [
                Pointers.getPrevPickResult(_this.leftPointer),
                Pointers.getPrevPickResult(_this.rightPointer)
            ];
            var hudRayPicks = [
                Pointers.getPrevPickResult(_this.leftHudPointer),
                Pointers.getPrevPickResult(_this.rightHudPointer)
            ];
            var mouseRayPick = Pointers.getPrevPickResult(_this.mouseRayPick);
            // if the pickray hit something very nearby, put it into the nearby entities list
            for (h = LEFT_HAND; h <= RIGHT_HAND; h++) {

                // XXX find a way to extract searchRay from samuel's stuff
                rayPicks[h].searchRay = {
                    origin: controllerLocations[h].position,
                    direction: Quat.getUp(controllerLocations[h].orientation),
                    length: 1000
                };

                if (rayPicks[h].type === Picks.INTERSECTED_ENTITY) {
                    // XXX check to make sure this one isn't already in nearbyEntityProperties?
                    if (rayPicks[h].distance < NEAR_GRAB_PICK_RADIUS * sensorScaleFactor) {
                        var nearEntityID = rayPicks[h].objectID;
                        var nearbyProps = Entities.getEntityProperties(nearEntityID, DISPATCHER_PROPERTIES);
                        nearbyProps.id = nearEntityID;
                        nearbyProps.distance = rayPicks[h].distance;
                        nearbyEntityPropertiesByID[nearEntityID] = nearbyProps;
                        nearbyEntityProperties[h].push(nearbyProps);
                    }
                }

                // sort by distance from each hand
                nearbyEntityProperties[h].sort(function (a, b) {
                    return a.distance - b.distance;
                });
            }

            // sometimes, during a HMD snap-turn, an equipped or held item wont be near
            // the hand when the findEntities is done.  Gather up any hand-children here.
            for (h = LEFT_HAND; h <= RIGHT_HAND; h++) {
                var handChildrenIDs = findHandChildEntities(h);
                handChildrenIDs.forEach(function (handChildID) {
                    if (handChildID in nearbyEntityPropertiesByID) {
                        return;
                    }
                    var props = Entities.getEntityProperties(handChildID, DISPATCHER_PROPERTIES);
                    props.id = handChildID;
                    nearbyEntityPropertiesByID[handChildID] = props;
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
                hudRayPicks: hudRayPicks,
                mouseRayPick: mouseRayPick
            };
            if (PROFILE) {
                Script.endProfileRange("dispatch.gather");
            }

            if (PROFILE) {
                Script.beginProfileRange("dispatch.isReady");
            }
            // check for plugins that would like to start.  ask in order of increasing priority value
            for (var pluginIndex = 0; pluginIndex < _this.orderedPluginNames.length; pluginIndex++) {
                var orderedPluginName = _this.orderedPluginNames[pluginIndex];
                var candidatePlugin = controllerDispatcherPlugins[orderedPluginName];

                if (_this.slotsAreAvailableForPlugin(candidatePlugin)) {
                    if (PROFILE) {
                        Script.beginProfileRange("dispatch.isReady." + orderedPluginName);
                    }
                    var readiness = candidatePlugin.isReady(controllerData, deltaTime);
                    if (readiness.active) {
                        // this plugin will start.  add it to the list of running plugins and mark the
                        // activity-slots which this plugin consumes as "in use"
                        _this.runningPluginNames[orderedPluginName] = true;
                        _this.markSlots(candidatePlugin, orderedPluginName);
                        _this.pointerManager.makePointerVisible(candidatePlugin.parameters.handLaser);
                        if (DEBUG) {
                            print("controllerDispatcher running " + orderedPluginName);
                        }
                    }
                    if (PROFILE) {
                        Script.endProfileRange("dispatch.isReady." + orderedPluginName);
                    }
                }
            }
            if (PROFILE) {
                Script.endProfileRange("dispatch.isReady");
            }

            if (PROFILE) {
                Script.beginProfileRange("dispatch.run");
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
                    } else {
                        if (PROFILE) {
                            Script.beginProfileRange("dispatch.run." + runningPluginName);
                        }
                        var runningness = plugin.run(controllerData, deltaTime);
                        if (!runningness.active) {
                            // plugin is finished running, for now.  remove it from the list
                            // of running plugins and mark its activity-slots as "not in use"
                            delete _this.runningPluginNames[runningPluginName];
                            _this.markSlots(plugin, false);
                            _this.pointerManager.makePointerInvisible(plugin.parameters.handLaser);
                            if (DEBUG) {
                                print("controllerDispatcher stopping " + runningPluginName);
                            }
                        }
                        _this.pointerManager.lockPointerEnd(plugin.parameters.handLaser, runningness.laserLockInfo);
                        if (PROFILE) {
                            Script.endProfileRange("dispatch.run." + runningPluginName);
                        }
                    }
                }
            }
            _this.pointerManager.updatePointersRenderState(controllerData.triggerClicks, controllerData.triggerValues);
            if (PROFILE) {
                Script.endProfileRange("dispatch.run");
            }
        };

        this.setBlacklist = function() {
            RayPick.setIgnoreItems(_this.leftControllerRayPick, this.blacklist.concat(HMD.tabletID));
            RayPick.setIgnoreItems(_this.rightControllerRayPick, this.blacklist.concat(HMD.tabletID));
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

        this.leftPointer = this.pointerManager.createPointer(false, PickType.Ray, {
            joint: "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
            filter: Picks.PICK_OVERLAYS | Picks.PICK_ENTITIES,
            triggers: [{action: Controller.Standard.LTClick, button: "Focus"}, {action: Controller.Standard.LTClick, button: "Primary"}],
            posOffset: getGrabPointSphereOffset(Controller.Standard.LeftHand, true),
            hover: true,
            scaleWithAvatar: true,
            distanceScaleEnd: true,
            hand: LEFT_HAND
        });
        this.rightPointer = this.pointerManager.createPointer(false, PickType.Ray, {
            joint: "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND",
            filter: Picks.PICK_OVERLAYS | Picks.PICK_ENTITIES,
            triggers: [{action: Controller.Standard.RTClick, button: "Focus"}, {action: Controller.Standard.RTClick, button: "Primary"}],
            posOffset: getGrabPointSphereOffset(Controller.Standard.RightHand, true),
            hover: true,
            scaleWithAvatar: true,
            distanceScaleEnd: true,
            hand: RIGHT_HAND
        });
        this.leftHudPointer = this.pointerManager.createPointer(true, PickType.Ray, {
            joint: "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
            filter: Picks.PICK_HUD,
            maxDistance: DEFAULT_SEARCH_SPHERE_DISTANCE,
            posOffset: getGrabPointSphereOffset(Controller.Standard.LeftHand, true),
            triggers: [{action: Controller.Standard.LTClick, button: "Focus"}, {action: Controller.Standard.LTClick, button: "Primary"}],
            hover: true,
            scaleWithAvatar: true,
            distanceScaleEnd: true,
            hand: LEFT_HAND
        });
        this.rightHudPointer = this.pointerManager.createPointer(true, PickType.Ray, {
            joint: "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND",
            filter: Picks.PICK_HUD,
            maxDistance: DEFAULT_SEARCH_SPHERE_DISTANCE,
            posOffset: getGrabPointSphereOffset(Controller.Standard.RightHand, true),
            triggers: [{action: Controller.Standard.RTClick, button: "Focus"}, {action: Controller.Standard.RTClick, button: "Primary"}],
            hover: true,
            scaleWithAvatar: true,
            distanceScaleEnd: true,
            hand: RIGHT_HAND
        });
        this.mouseRayPick = Pointers.createPointer(PickType.Ray, {
            joint: "Mouse",
            filter: Picks.PICK_ENTITIES | Picks.PICK_OVERLAYS,
            enabled: true
        });
        this.handleHandMessage = function(channel, message, sender) {
            var data;
            if (sender === MyAvatar.sessionUUID) {
                try {
                    if (channel === 'Hifi-Hand-RayPick-Blacklist') {
                        data = JSON.parse(message);
                        var action = data.action;
                        var id = data.id;
                        var index = _this.blacklis.indexOf(id);

                        if (action === 'add' && index === -1) {
                            _this.blacklist.push(id);
                            _this.setBlacklist();
                        }

                        if (action === 'remove') {
                            if (index > -1) {
                                _this.blacklist.splice(index, 1);
                                _this.setBlacklist();
                            }
                        }
                    }

                } catch (e) {
                    print("WARNING: handControllerGrab.js -- error parsing Hifi-Hand-RayPick-Blacklist message: " + message);
                }
            }
        };

        this.cleanup = function () {
            Controller.disableMapping(MAPPING_NAME);
            _this.pointerManager.removePointers();
            Pointers.removePointer(this.mouseRayPick);
        };
    }
    function mouseReleaseOnOverlay(overlayID, event) {
        if (overlayID === HMD.homeButtonID && event.button === "Primary") {
            Messages.sendLocalMessage("home", overlayID);
        }
    }

    var HAPTIC_STYLUS_STRENGTH = 1.0;
    var HAPTIC_STYLUS_DURATION = 20.0;
    function mousePress(id, event) {
        if (HMD.active) {
            var runningPlugins = controllerDispatcher.runningPluginNames;
            if (event.id === controllerDispatcher.leftPointer && event.button === "Primary" && runningPlugins.LeftWebSurfaceLaserInput) {
                Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, LEFT_HAND);
            } else if (event.id === controllerDispatcher.rightPointer && event.button === "Primary" && runningPlugins.RightWebSurfaceLaserInput) {
                Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, RIGHT_HAND);
            }
        }
    }
    Overlays.mouseReleaseOnOverlay.connect(mouseReleaseOnOverlay);
    Overlays.mousePressOnOverlay.connect(mousePress);
    Entities.mousePressOnEntity.connect(mousePress);
    var controllerDispatcher = new ControllerDispatcher();
    Messages.subscribe('Hifi-Hand-RayPick-Blacklist');
    Messages.messageReceived.connect(controllerDispatcher.handleHandMessage);
    Script.scriptEnding.connect(controllerDispatcher.cleanup);
    Script.setTimeout(controllerDispatcher.update, BASIC_TIMER_INTERVAL_MS);
}());
