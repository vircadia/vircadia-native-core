// -- ADVANCED DEBUGGING --
// @function - Add debug joint indicators / extra debugging info.
// @param {Doppleganger} - existing Doppleganger instance to add controls to
//
// @note:
//   * rightclick toggles mirror mode on/off
//   * shift-rightclick toggles the debug indicators on/off
//   * clicking on an indicator displays the joint name and mirrored joint name in the debug log.
//
// Example use:
//    var doppleganger = new DopplegangerClass();
//    DopplegangerClass.addDebugControls(doppleganger);

"use strict";
/* eslint-env commonjs */
/* eslint-disable comma-dangle */
/* global console */

var DopplegangerClass = require('./doppleganger.js'),
    modelHelper = require('./model-helper.js').modelHelper,
    utils = require('./utils.js');

module.exports = DebugControls;
// mixin addDebugControls to DopplegangerClass for backwards-compatibility
DopplegangerClass.addDebugControls = function(doppleganger) {
    new DebugControls(doppleganger);
    return doppleganger;
};

DebugControls.version = '0.0.0';
DebugControls.COLOR_DEFAULT = { red: 255, blue: 255, green: 255 };
DebugControls.COLOR_SELECTED = { red: 0, blue: 255, green: 0 };

function log() {
    // eslint-disable-next-line no-console
    (typeof console === 'object' ? console.log : print)('doppleganger-debug | ' + [].slice.call(arguments).join(' '));
}

function DebugControls(doppleganger) {
    this.enableIndicators = true;
    this.selectedJointName = null;
    this.debugOverlayIDs = undefined;
    this.jointSelected = utils.signal(function(result) {});
    this.doppleganger = doppleganger;
    this._initialize();
}
DebugControls.prototype = {
    start: function() {
        if (!this.onMousePressEvent) {
            this.onMousePressEvent = this._onMousePressEvent;
            Controller.mousePressEvent.connect(this, 'onMousePressEvent');
            this.doppleganger.jointsUpdated.connect(this, 'onJointsUpdated');
        }
    },

    stop: function() {
        this.removeIndicators();
        if (this.onMousePressEvent) {
            this.doppleganger.jointsUpdated.disconnect(this, 'onJointsUpdated');
            Controller.mousePressEvent.disconnect(this, 'onMousePressEvent');
            delete this.onMousePressEvent;
        }
    },

    createIndicators: function(jointNames) {
        this.jointNames = jointNames;
        return jointNames.map(function(name, i) {
            return Overlays.addOverlay('shape', {
                shape: 'Icosahedron',
                scale: 0.1,
                solid: false,
                alpha: 0.5
            });
        });
    },

    removeIndicators: function() {
        if (this.debugOverlayIDs) {
            this.debugOverlayIDs.forEach(Overlays.deleteOverlay);
            this.debugOverlayIDs = undefined;
        }
    },

    onJointsUpdated: function(overlayID) {
        if (!this.enableIndicators) {
            return;
        }
        var jointNames = Overlays.getProperty(overlayID, 'jointNames'),
            jointOrientations = Overlays.getProperty(overlayID, 'jointOrientations'),
            jointPositions = Overlays.getProperty(overlayID, 'jointPositions'),
            selectedIndex = jointNames.indexOf(this.selectedJointName);

        if (!this.debugOverlayIDs) {
            this.debugOverlayIDs = this.createIndicators(jointNames);
        }

        // batch all updates into a single call (using the editOverlays({ id: {props...}, ... }) API)
        var updatedOverlays = this.debugOverlayIDs.reduce(function(updates, id, i) {
            updates[id] = {
                position: jointPositions[i],
                rotation: jointOrientations[i],
                color: i === selectedIndex ? DebugControls.COLOR_SELECTED : DebugControls.COLOR_DEFAULT,
                solid: i === selectedIndex
            };
            return updates;
        }, {});
        Overlays.editOverlays(updatedOverlays);
    },

    _onMousePressEvent: function(evt) {
        if (evt.isLeftButton) {
            if (!this.enableIndicators || !this.debugOverlayIDs) {
                return;
            }
            var ray = Camera.computePickRay(evt.x, evt.y),
                hit = Overlays.findRayIntersection(ray, true, this.debugOverlayIDs);

            hit.jointIndex = this.debugOverlayIDs.indexOf(hit.overlayID);
            hit.jointName = this.jointNames[hit.jointIndex];
            this.jointSelected(hit);
        } else if (evt.isRightButton) {
            if (evt.isShifted) {
                this.enableIndicators = !this.enableIndicators;
                if (!this.enableIndicators) {
                    this.removeIndicators();
                }
            } else {
                this.doppleganger.mirrored = !this.doppleganger.mirrored;
            }
        }
    },

    _initialize: function() {
        if ('$debugControls' in this.doppleganger) {
            throw new Error('only one set of debug controls can be added per doppleganger');
        }
        this.doppleganger.$debugControls = this;

        this.doppleganger.activeChanged.connect(this, function(active) {
            if (active) {
                this.start();
            } else {
                this.stop();
            }
        });

        this.jointSelected.connect(this, function(hit) {
            this.selectedJointName = hit.jointName;
            if (hit.jointIndex < 0) {
                return;
            }
            hit.mirroredJointName = modelHelper.deriveMirroredJointNames([hit.jointName])[0];
            log('selected joint:', JSON.stringify(hit, 0, 2));
        });

        Script.scriptEnding.connect(this, 'removeIndicators');
    },
}; // DebugControls.prototype
