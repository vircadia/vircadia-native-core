//
//  overlayUtils.js
//  examples/libraries
//
//  Modified by Zander Otavka on 7/15/15
//  Copyright 2014 High Fidelity, Inc.
//
//  Manage overlays with object oriented goodness, instead of ugly `Overlays.h` methods.
//  Instead of:
//
//      var billboard = Overlays.addOverlay("billboard", { visible: false });
//      ...
//      Overlays.editOverlay(billboard, { visible: true });
//      ...
//      Overlays.deleteOverlay(billboard);
//
//  You can now do:
//
//      var billboard = new BillboardOverlay({ visible: false });
//      ...
//      billboard.visible = true;
//      ...
//      billboard.destroy();
//
//  See more on usage below.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


/**
 *  DEPRECATION WARNING: Will be deprecated soon in favor of FloatingUIPanel.
 *
 *  OverlayGroup provides a way to create composite overlays and control their
 *  position relative to a settable rootPosition and rootRotation.
 */
OverlayGroup = function(opts) {
    var that = {};

    var overlays = {};

    var rootPosition = opts.position || { x: 0, y: 0, z: 0 };
    var rootRotation = opts.rotation || Quat.fromPitchYawRollRadians(0, 0, 0);
    var visible = opts.visible == true;

    function updateOverlays() {
        for (overlayID in overlays) {
            var overlay = overlays[overlayID];
            var newPosition = Vec3.multiplyQbyV(rootRotation, overlay.position);
            newPosition = Vec3.sum(rootPosition, newPosition);
            Overlays.editOverlay(overlayID, {
                visible: visible,
                position: newPosition,
                rotation: Quat.multiply(rootRotation, overlay.rotation),
            });
        };
    }

    that.createOverlay = function(type, properties) {
        properties.position = properties.position || { x: 0, y: 0, z: 0 };
        properties.rotation = properties.rotation || Quat.fromPitchYawRollRadians(0, 0, 0);

        var overlay = Overlays.addOverlay(type, properties);

        overlays[overlay] = {
            position: properties.position,
            rotation: properties.rotation,
        };

        updateOverlays();

        return overlay;
    }

    that.setProperties = function(properties) {
        if (properties.position !== undefined) {
            rootPosition = properties.position;
        }
        if (properties.rotation !== undefined) {
            rootRotation = properties.rotation;
        }
        if (properties.visible !== undefined) {
            visible = properties.visible;
        }
        updateOverlays();
    };

    that.destroy = function() {
        for (var overlay in overlays) {
            Overlays.deleteOverlay(overlay);
        }
        overlays = {};
    }
    
    return that;
};


/**
 *  Object oriented abstraction layer for overlays.
 *
 *  Usage:
 *      // Create an overlay
 *      var billboard = new BillboardOverlay({
 *          visible: true,
 *          isFacingAvatar: true,
 *          ignoreRayIntersections: false
 *      });
 *
 *      // Get a property
 *      var isVisible = billboard.visible;
 *
 *      // Set a single property
 *      billboard.position = { x: 1, y: 3, z: 2 };
 *
 *      // Set multiple properties at the same time
 *      billboard.setProperties({
 *          url: "http://images.com/overlayImage.jpg",
 *          dimensions: { x: 2, y: 2 }
 *      });
 *
 *      // Clone an overlay
 *      var clonedBillboard = billboard.clone();
 *
 *      // Remove an overlay from the world
 *      billboard.destroy();
 *
 *      // Remember, there is a poor orphaned JavaScript object left behind.  You should remove any
 *      // references to it so you don't accidentally try to modify an overlay that isn't there.
 *      billboard = undefined;
 */
(function() {
    var ABSTRACT = null;

    function generateOverlayClass(superclass, type, properties) {
        var that;
        if (type == ABSTRACT) {
            that = function(type, params) {
                superclass.apply(this, [type, params]);
            };
        } else {
            that = function(params) {
                superclass.apply(this, [type, params]);
            };
        }

        that.prototype = new superclass();
        that.prototype.constructor = that;

        properties.forEach(function(prop) {
            Object.defineProperty(that.prototype, prop, {
                get: function() {
                    return Overlays.getProperty(this._id, prop);
                },
                set: function(newValue) {
                    var keyValuePair = {};
                    keyValuePair[prop] = newValue;
                    this.setProperties(keyValuePair);
                },
                configurable: true
            });
        });

        return that;
    }


    // Supports multiple inheritance of properties.  Just `concat` them onto the end of the
    // properties list.
    var PANEL_ATTACHABLE_FIELDS = ["attachedPanel"];

    // TODO: finish exposing all overlay classes.

    var Overlay = (function() {
        var BaseOverlay = (function() {
            var that = function(type, params) {
                Object.apply(this, []);
                if (type && params) {
                    this._type = type;
                    this._id = Overlays.addOverlay(type, params);
                } else {
                    this._type = "";
                    this._id = 0;
                }
                this._attachedPanelPointer = null;
            };

            that.prototype = new Object();
            that.prototype.constructor = that;

            Object.defineProperty(that.prototype, "overlayType", {
                get: function() {
                    return this._type;
                }
            });

            that.prototype.setProperties = function(properties) {
                Overlays.editOverlay(this._id, properties);
            };

            that.prototype.clone = function() {
                var clone = new this.constructor();
                clone._type = this._type;
                clone._id = Overlays.cloneOverlay(this._id);
                if (this._attachedPanelPointer) {
                    this._attachedPanelPointer.addChild(clone);
                }
                return clone;
            };

            that.prototype.destroy = function() {
                Overlays.deleteOverlay(this._id);
            };

            return that;
        }());

        return generateOverlayClass(BaseOverlay, ABSTRACT, [
            "alpha", "glowLevel", "pulseMax", "pulseMin", "pulsePeriod", "glowLevelPulse",
            "alphaPulse", "colorPulse", "visible", "anchor"
        ]);
    }());

    var Base3DOverlay = generateOverlayClass(Overlay, ABSTRACT, [
        "position", "lineWidth", "rotation", "isSolid", "isFilled", "isWire", "isDashedLine",
        "ignoreRayIntersection", "drawInFront", "drawOnHUD"
    ]);

    var Planar3DOverlay = generateOverlayClass(Base3DOverlay, ABSTRACT, [
        "dimensions"
    ]);

    BillboardOverlay = generateOverlayClass(Planar3DOverlay, "billboard", [
        "url", "subImage", "isFacingAvatar", "offsetPosition"
    ].concat(PANEL_ATTACHABLE_FIELDS));
}());


/**
 *  Object oriented abstraction layer for panels.
 */
FloatingUIPanel = (function() {
    var that = function(params) {
        this._id = Overlays.addPanel(params);
        this._children = [];
    };

    var FIELDS = ["offsetPosition", "offsetRotation", "facingRotation"];
    FIELDS.forEach(function(prop) {
        Object.defineProperty(that.prototype, prop, {
            get: function() {
                return Overlays.getPanelProperty(this._id, prop);
            },
            set: function(newValue) {
                var keyValuePair = {};
                keyValuePair[prop] = newValue;
                this.setProperties(keyValuePair);
            },
            configurable: false
        });
    });

    Object.defineProperty(that.prototype, "children", {
        get: function() {
            return this._children.slice();
        }
    })

    that.prototype.addChild = function(overlay) {
        overlay.attachedPanel = this._id;
        overlay._attachedPanelPointer = this;
        this._children.push(overlay);
        return overlay;
    };

    that.prototype.removeChild = function(overlay) {
        var i = this._children.indexOf(overlay);
        if (i >= 0) {
            overlay.attachedPanel = 0;
            overlay._attachedPanelPointer = null;
            this._children.splice(i, 1);
        }
    };

    that.prototype.setVisible = function(visible) {
        for (var i in this._children) {
            this._children[i].visible = visible;
        }
    };

    that.prototype.setProperties = function(properties) {
        Overlays.editPanel(this._id, properties);
    };

    that.prototype.destroy = function() {
        Overlays.deletePanel(this._id);
        var i = _panels.indexOf(this);
        if (i >= 0) {
            _panels.splice(i, 1);
        }
    };

    that.prototype.findRayIntersection = function(pickRay) {
        var rayPickResult = Overlays.findRayIntersection(pickRay);
        if (rayPickResult.intersects) {
            for (var i in this._children) {
                if (this._children[i]._id == rayPickResult.overlayID) {
                    return this._children[i];
                }
            }
        }
        return null;
    };

    return that;
}());
