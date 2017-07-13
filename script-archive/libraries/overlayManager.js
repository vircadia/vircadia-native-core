//
//  overlayManager.js
//  examples/libraries
//
//  Created by Zander Otavka on 7/24/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Manage overlays with object oriented goodness, instead of ugly `Overlays.h` methods.
//  Instead of:
//
//      var billboard = Overlays.addOverlay("image3d", { visible: false });
//      ...
//      Overlays.editOverlay(billboard, { visible: true });
//      ...
//      Overlays.deleteOverlay(billboard);
//
//  You can now do:
//
//      var billboard = new Image3DOverlay({ visible: false });
//      ...
//      billboard.visible = true;
//      ...
//      billboard.destroy();
//
//  More on usage below.  Examples in `examples/example/overlayPanelExample.js`.
//
//  Note that including this file will delete `Overlays` from the global scope.  All the
//  functionality of `Overlays` is represented here, just better.  If you try to use `Overlays`
//  in tandem, there may be performance problems or nasty surprises.
//

(function() {
    // Delete `Overlays` from the global scope.
    var Overlays = this.Overlays;
    delete this.Overlays;


    var ABSTRACT = null;

    var overlays = {};
    var panels = {};

    var overlayTypes = {};


    function generateOverlayClass(superclass, type, properties) {
        var that;
        if (type == ABSTRACT) {
            that = function(type, params) {
                superclass.call(this, type, params);
            };
        } else {
            that = function(params) {
                superclass.call(this, type, params);
            };
            overlayTypes[type] = that;
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
                configurable: false
            });
        });

        return that;
    }

    //
    //  Create a new JavaScript object for an overlay of given ID.
    //
    function makeOverlayFromId(id) {
        var type = Overlays.getOverlayType(id);
        if (!type) {
            return null;
        }
        var overlay = new overlayTypes[type]();
        overlay._id = id;
        overlays[id] = overlay;
        return overlay;
    }

    //
    //  Get or create an overlay object from the id.
    //
    //  @param knownOverlaysOnly (Optional: Boolean)
    //      If true, a new object will not be created.
    //  @param searchList (Optional: Object)
    //      Map of overlay id's and overlay objects.  Can be generated with
    //      `OverlayManager.makeSearchList`.
    //
    function findOverlay(id, knownOverlaysOnly, searchList) {
        if (id > 0) {
            knownOverlaysOnly = Boolean(knownOverlaysOnly);
            searchList = searchList || overlays;
            var foundOverlay = searchList[id];
            if (foundOverlay) {
                return foundOverlay;
            }
            if (!knownOverlaysOnly) {
                return makeOverlayFromId(id);
            }
        }
        return null;
    }

    //
    //  Create a new JavaScript object for a panel of given ID.
    //
    function makePanelFromId(id) {
        if (!Overlays.isAddedPanel(id)) {
            return null;
        }
        var panel = new OverlayPanel();
        panel._id = id;
        overlays[id] = overlay;
        return overlay;
    }

    //
    //  Get or create a panel object from the id.
    //
    //  @param knownOverlaysOnly (Optional: Boolean)
    //      If true, a new object will not be created.
    //  @param searchList (Optional: Object)
    //      Map of overlay id's and overlay objects.  Can be generated with
    //      `OverlayManager.makeSearchList`.
    //
    function findPanel(id, knownPanelsOnly, searchList) {
        if (id > 0) {
            knownPanelsOnly = Boolean(knownPanelsOnly);
            searchList = searchList || panels;
            var foundPanel = searchList[id];
            if (foundPanel) {
                return foundPanel;
            }
            if (!knownPanelsOnly) {
                return makePanelFromId(id);
            }
        }
        return null;
    }

    function findOverlayOrPanel(id, knownObjectsOnly, searchList) {
        return findOverlay(id, knownObjectsOnly, searchList) ||
               findPanel(id, knownObjectsOnly, searchList);
    }


    var Overlay = (function() {
        var that = function(type, params) {
            if (type && params) {
                this._id = Overlays.addOverlay(type, params);
                overlays[this._id] = this;
            } else {
                this._id = 0;
            }
        };

        that.prototype.constructor = that;

        Object.defineProperty(that.prototype, "isLoaded", {
            get: function() {
                return Overlays.isLoaded(this._id);
            }
        });

        Object.defineProperty(that.prototype, "parentPanel", {
            get: function() {
                return findPanel(Overlays.getParentPanel(this._id));
            }
        });

        that.prototype.getTextSize = function(text) {
            return Overlays.textSize(this._id, text);
        };

        that.prototype.setProperties = function(properties) {
            Overlays.editOverlay(this._id, properties);
        };

        that.prototype.clone = function() {
            return makeOverlayFromId(Overlays.cloneOverlay(this._id));
        };

        that.prototype.destroy = function() {
            Overlays.deleteOverlay(this._id);
        };

        that.prototype.isPanelAttachable = function() {
            return false;
        };

        return generateOverlayClass(that, ABSTRACT, [
            "alpha", "pulseMax", "pulseMin", "pulsePeriod",
            "alphaPulse", "colorPulse", "visible", "anchor"
        ]);
    })();

    // Supports multiple inheritance of properties.  Just `concat` them onto the end of the
    // properties list.
    var PanelAttachable = ["offsetPosition", "offsetRotation", "offsetScale"];
    var Billboardable = ["isFacingAvatar"];

    var Overlay2D = generateOverlayClass(Overlay, ABSTRACT, [
        "bounds", "x", "y", "width", "height"
    ]);

    var Base3DOverlay = generateOverlayClass(Overlay, ABSTRACT, [
        "position", "lineWidth", "rotation", "isSolid", "isFilled", "isWire", "isDashedLine",
        "ignoreRayIntersection", "drawInFront"
    ]);

    var Planar3DOverlay = generateOverlayClass(Base3DOverlay, ABSTRACT, [
        "dimensions"
    ]);

    var Billboard3DOverlay = generateOverlayClass(Planar3DOverlay, ABSTRACT, [
    ].concat(PanelAttachable).concat(Billboardable));
    Billboard3DOverlay.prototype.isPanelAttachable = function() { return true; };

    var Volume3DOverlay = generateOverlayClass(Base3DOverlay, ABSTRACT, [
        "dimensions"
    ]);

    Image3DOverlay = generateOverlayClass(Billboard3DOverlay, "image3d", [
        "url", "subImage"
    ]);

    Text3DOverlay = generateOverlayClass(Billboard3DOverlay, "text3d", [
        "text", "backgroundColor", "backgroundAlpha", "lineHeight", "leftMargin", "topMargin",
        "rightMargin", "bottomMargin"
    ]);

    Cube3DOverlay = generateOverlayClass(Volume3DOverlay, "cube", [
        "borderSize"
    ]);

    Sphere3DOverlay = generateOverlayClass(Volume3DOverlay, "sphere", [
    ]);

    Circle3DOverlay = generateOverlayClass(Planar3DOverlay, "circle3d", [
        "startAt", "endAt", "outerRadius", "innerRadius", "hasTickMarks",
        "majorTickMarksAngle", "minorTickMarksAngle", "majorTickMarksLength",
        "minorTickMarksLength", "majorTickMarksColor", "minorTickMarksColor"
    ]);

    Rectangle3DOverlay = generateOverlayClass(Planar3DOverlay, "rectangle3d", [
    ]);

    Line3DOverlay = generateOverlayClass(Base3DOverlay, "line3d", [
        "start", "end"
    ]);

    Grid3DOverlay = generateOverlayClass(Planar3DOverlay, "grid", [
        "minorGridWidth", "majorGridEvery"
    ]);

    ModelOverlay = generateOverlayClass(Volume3DOverlay, "model", [
        "url", "dimensions", "textures"
    ]);


    OverlayPanel = (function() {
        var that = function(params) {
            this._id = Overlays.addPanel(params);
            panels[this._id] = this;
        };

        that.prototype.constructor = that;

        var props = [
            "anchorPosition", "anchorPositionBinding", "anchorRotation", "anchorRotationBinding", "anchorScale", "visible"
        ].concat(PanelAttachable).concat(Billboardable)

        props.forEach(function(prop) {
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

        Object.defineProperty(that.prototype, "parentPanel", {
            get: function() {
                return findPanel(Overlays.getParentPanel(this._id));
            }
        });

        Object.defineProperty(that.prototype, "children", {
            get: function() {
                var idArray = Overlays.getPanelProperty(this._id, "children");
                var objArray = [];
                for (var i = 0; i < idArray.length; i++) {
                    objArray[i] = findOverlayOrPanel(idArray[i]);
                }
                return objArray;
            }
        });

        that.prototype.addChild = function(child) {
            Overlays.setParentPanel(child._id, this._id);
            return child;
        };

        that.prototype.removeChild = function(child) {
            if (child.parentPanel === this) {
                Overlays.setParentPanel(child._id, 0);
            }
        };

        that.prototype.setProperties = function(properties) {
            Overlays.editPanel(this._id, properties);
        };

        that.prototype.setChildrenVisible = function() {
            this.children.forEach(function(child) {
                child.visible = true;
                if (child.setChildrenVisible !== undefined) {
                    child.setChildrenVisible();
                }
            });
        };

        that.prototype.destroy = function() {
            Overlays.deletePanel(this._id);
        };

        return that;
    })();


    OverlayManager = {
        findOnRay: function(pickRay, knownOverlaysOnly, searchList) {
            var rayPickResult = Overlays.findRayIntersection(pickRay);
            if (rayPickResult.intersects) {
                return findOverlay(rayPickResult.overlayID, knownOverlaysOnly, searchList);
            }
            return null;
        },
        findAtPoint: function(point, knownOverlaysOnly, searchList) {
            var foundID = Overlays.getOverlayAtPoint(point);
            if (foundID) {
                return findOverlay(foundID, knownOverlaysOnly, searchList);
            } else {
                var pickRay = Camera.computePickRay(point.x, point.y);
                return OverlayManager.findOnRay(pickRay, knownOverlaysOnly, searchList);
            }
        },
        makeSearchList: function(array) {
            var searchList = {};
            array.forEach(function(object) {
                searchList[object._id] = object;
            });
            return searchList;
        },
        findRayIntersection: function(pickRay) {
            return Overlays.findRayIntersection(pickRay);
        }
    };


    // Threadsafe cleanup of JavaScript objects.

    function onOverlayDeleted(id) {
        if (id in overlays) {
            if (overlays[id].parentPanel) {
                overlays[id].parentPanel.removeChild(overlays[id]);
            }
            delete overlays[id];
        }
    }

    function onPanelDeleted(id) {
        if (id in panels) {
            if (panels[id].parentPanel) {
                panels[id].parentPanel.removeChild(panels[id]);
            }
            delete panels[id];
        }
    }

    Overlays.overlayDeleted.connect(onOverlayDeleted);
    Overlays.panelDeleted.connect(onPanelDeleted);
})();
