//
//  controllerDisplay.js
//
//  Created by Anthony J. Thibault on 10/20/16
//  Originally created by Ryan Huffman on 9/21/2016
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* globals createControllerDisplay:true, deleteControllerDisplay:true, Controller, Overlays, Vec3, MyAvatar, Quat */

function clamp(value, min, max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    }
    return value;
}

function resolveHardware(path) {
    if (typeof path === 'string') {
        var parts = path.split(".");
        function resolveInner(base, path, i) {
            if (i >= path.length) {
                return base;
            }
            return resolveInner(base[path[i]], path, ++i);
        }
        return resolveInner(Controller.Hardware, parts, 0);
    }
    return path;
}

var DEBUG = true;
function debug() {
    if (DEBUG) {
        var args = Array.prototype.slice.call(arguments);
        args.unshift("controllerDisplay.js | ");
        print.apply(this, args);
    }
}

createControllerDisplay = function(config) {
    var controllerDisplay = {
        overlays: [],
        partOverlays: {},
        parts: {},
        mappingName: "mapping-display-" + Math.random(),
        partValues: {},

        setVisible: function(visible) {
            for (var i = 0; i < this.overlays.length; ++i) {
                Overlays.editOverlay(this.overlays[i], {
                    visible: visible
                });
            }
        },

        setPartVisible: function(partName, visible) {
            // Disabled
            /*
            if (partName in this.partOverlays) {
                for (var i = 0; i < this.partOverlays[partName].length; ++i) {
                    Overlays.editOverlay(this.partOverlays[partName][i], {
                        //visible: visible
                    });
                }
            }
            */
        },

        setLayerForPart: function(partName, layerName) {
            if (partName in this.parts) {
                var part = this.parts[partName];
                if (part.textureLayers && layerName in part.textureLayers) {
                    var layer = part.textureLayers[layerName];
                    var textures = {};
                    if (layer.defaultTextureURL) {
                        textures[part.textureName] = layer.defaultTextureURL;
                    }
                    for (var i = 0; i < this.partOverlays[partName].length; ++i) {
                        Overlays.editOverlay(this.partOverlays[partName][i], {
                            textures: textures
                        });
                    }
                }
            }
        },

        resize: function(sensorScaleFactor) {
            if (this.overlays.length >= 0) {
                var controller = config.controllers[0];
                var position = controller.position;

                // first overlay is main body.
                var overlayID = this.overlays[0];
                var localPosition = Vec3.multiply(sensorScaleFactor, Vec3.sum(Vec3.multiplyQbyV(controller.rotation, controller.naturalPosition), position));
                var dimensions = Vec3.multiply(sensorScaleFactor, controller.dimensions);

                Overlays.editOverlay(overlayID, {
                    dimensions: dimensions,
                    localPosition: localPosition
                });

                if (controller.parts) {
                    var i = 1;
                    for (var partName in controller.parts) {
                        overlayID = this.overlays[i++];
                        var part = controller.parts[partName];
                        localPosition = Vec3.subtract(part.naturalPosition, controller.naturalPosition);
                        var localRotation;
                        var value = this.partValues[partName];
                        var offset, rotation;
                        if (value !== undefined) {
                            if (part.type === "linear") {
                                offset = Vec3.multiply(part.maxTranslation * value, part.axis);
                                localPosition = Vec3.sum(localPosition, offset);
                                localRotation = undefined;
                            } else if (part.type === "joystick") {
                                rotation = Quat.fromPitchYawRollDegrees(value.y * part.xHalfAngle, 0, value.x * part.yHalfAngle);
                                if (part.originOffset) {
                                    offset = Vec3.multiplyQbyV(rotation, part.originOffset);
                                    offset = Vec3.subtract(part.originOffset, offset);
                                } else {
                                    offset = { x: 0, y: 0, z: 0 };
                                }
                                localPosition = Vec3.sum(offset, localPosition);
                                localRotation = rotation;
                            } else if (part.type === "rotational") {
                                value = clamp(value, part.minValue, part.maxValue);
                                var pct = (value - part.minValue) / part.maxValue;
                                var angle = pct * part.maxAngle;
                                rotation = Quat.angleAxis(angle, part.axis);
                                if (part.origin) {
                                    offset = Vec3.multiplyQbyV(rotation, part.origin);
                                    offset = Vec3.subtract(offset, part.origin);
                                } else {
                                    offset = { x: 0, y: 0, z: 0 };
                                }
                                localPosition = Vec3.sum(offset, localPosition);
                                localRotation = rotation;
                            }
                        }
                        if (localRotation !== undefined) {
                            Overlays.editOverlay(overlayID, {
                                dimensions: Vec3.multiply(sensorScaleFactor, part.naturalDimensions),
                                localPosition: Vec3.multiply(sensorScaleFactor, localPosition),
                                localRotation: localRotation
                            });
                        } else {
                            Overlays.editOverlay(overlayID, {
                                dimensions: Vec3.multiply(sensorScaleFactor, part.naturalDimensions),
                                localPosition: Vec3.multiply(sensorScaleFactor, localPosition)
                            });
                        }
                    }
                }
            }
        }
    };

    var mapping = Controller.newMapping(controllerDisplay.mappingName);
    for (var i = 0; i < config.controllers.length; ++i) {
        var controller = config.controllers[i];
        var position = controller.position;
        var sensorScaleFactor = MyAvatar.sensorToWorldScale;

        if (controller.naturalPosition) {
            position = Vec3.sum(Vec3.multiplyQbyV(controller.rotation, controller.naturalPosition), position);
        } else {
            controller.naturalPosition = { x: 0, y: 0, z: 0 };
        }

        var baseOverlayID = Overlays.addOverlay("model", {
            url: controller.modelURL,
            dimensions: Vec3.multiply(sensorScaleFactor, controller.dimensions),
            localRotation: controller.rotation,
            localPosition: Vec3.multiply(sensorScaleFactor, position),
            parentID: MyAvatar.SELF_ID,
            parentJointIndex: controller.jointIndex,
            ignoreRayIntersection: true
        });

        controllerDisplay.overlays.push(baseOverlayID);

        if (controller.parts) {
            for (var partName in controller.parts) {
                var part = controller.parts[partName];
                var localPosition = Vec3.subtract(part.naturalPosition, controller.naturalPosition);
                var localRotation = { x: 0, y: 0, z: 0, w: 1 };

                controllerDisplay.parts[partName] = controller.parts[partName];

                var properties = {
                    url: part.modelURL,
                    localPosition: localPosition,
                    localRotation: localRotation,
                    parentID: baseOverlayID,
                    ignoreRayIntersection: true
                };

                if (part.defaultTextureLayer) {
                    var textures = {};
                    textures[part.textureName] = part.textureLayers[part.defaultTextureLayer].defaultTextureURL;
                    properties.textures = textures;
                }

                var overlayID = Overlays.addOverlay("model", properties);

                if (part.type === "rotational") {
                    var input = resolveHardware(part.input);
                    mapping.from([input]).peek().to(function(partName) {
                        return function(value) {
                            // insert the most recent controller value into controllerDisplay.partValues.
                            controllerDisplay.partValues[partName] = value;
                            controllerDisplay.resize(MyAvatar.sensorToWorldScale);
                        };
                    }(partName));
                } else if (part.type === "touchpad") {
                    var visibleInput = resolveHardware(part.visibleInput);
                    var xInput = resolveHardware(part.xInput);
                    var yInput = resolveHardware(part.yInput);

                    // TODO: Touchpad inputs are currently only working for half
                    // of the touchpad. When that is fixed, it would be useful
                    // to update these to display the current finger position.
                    mapping.from([visibleInput]).peek().to(function(value) {
                    });
                    mapping.from([xInput]).peek().to(function(value) {
                    });
                    mapping.from([yInput]).peek().invert().to(function(value) {
                    });
                } else if (part.type === "joystick") {
                    (function(part, partName) {
                        var xInput = resolveHardware(part.xInput);
                        var yInput = resolveHardware(part.yInput);
                        mapping.from([xInput]).peek().to(function(value) {
                            // insert the most recent controller value into controllerDisplay.partValues.
                            if (controllerDisplay.partValues[partName]) {
                                controllerDisplay.partValues[partName].x = value;
                            } else {
                                controllerDisplay.partValues[partName] = {x: value, y: 0};
                            }
                            controllerDisplay.resize(MyAvatar.sensorToWorldScale);
                        });
                        mapping.from([yInput]).peek().to(function(value) {
                            // insert the most recent controller value into controllerDisplay.partValues.
                            if (controllerDisplay.partValues[partName]) {
                                controllerDisplay.partValues[partName].y = value;
                            } else {
                                controllerDisplay.partValues[partName] = {x: 0, y: value};
                            }
                            controllerDisplay.resize(MyAvatar.sensorToWorldScale);
                        });
                    })(part, partName);

                } else if (part.type === "linear") {
                    (function(part, partName) {
                        var input = resolveHardware(part.input);
                        mapping.from([input]).peek().to(function(value) {
                            // insert the most recent controller value into controllerDisplay.partValues.
                            controllerDisplay.partValues[partName] = value;
                            controllerDisplay.resize(MyAvatar.sensorToWorldScale);
                        });
                    })(part, partName);

                } else if (part.type === "static") {
                    // do nothing
                } else {
                    debug("TYPE NOT SUPPORTED: ", part.type);
                }

                controllerDisplay.overlays.push(overlayID);
                if (!(partName in controllerDisplay.partOverlays)) {
                    controllerDisplay.partOverlays[partName] = [];
                }
                controllerDisplay.partOverlays[partName].push(overlayID);
            }
        }
    }
    Controller.enableMapping(controllerDisplay.mappingName);
    controllerDisplay.resize(MyAvatar.sensorToWorldScale);
    return controllerDisplay;
};

deleteControllerDisplay = function(controllerDisplay) {
    for (var i = 0; i < controllerDisplay.overlays.length; ++i) {
        Overlays.deleteOverlay(controllerDisplay.overlays[i]);
    }
    Controller.disableMapping(controllerDisplay.mappingName);
};
