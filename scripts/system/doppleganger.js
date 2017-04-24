"use strict";

//  doppleganger.js
//
//  Created by Timothy Dedischew on 04/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  This tablet app creates a mirrored projection of your avatar (ie: a "doppleganger") that you can walk around
//  and inspect.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global */

var USE_SCRIPT_UPDATE = false; // if this is true then Script.update will be used to update the doppleganger joints
var TARGET_FPS = 60; // when USE_SCRIPT_UPDATE is false, Script.setInterval will be used and target this FPS

module.exports = Doppleganger;

if (!Function.prototype.bind) {
    // FIXME: this inline version is meant to be temporary, pending either a system-wide version being adopted
    //  or libraries/utils.js becoming a clean .require'able module
    Function.prototype.bind = function(){var fn=this,s=[].slice,a=s.call(arguments),o=a.shift();return function(){return fn.apply(o,a.concat(s.call(arguments)))}};
}

function Doppleganger(options) {
    this.avatar = options.avatar || MyAvatar;
    this.mirrored = 'mirrored' in options ? options.mirrored : true;
    this.debug = 'debug' in options ? options.debug : true;
    this.eyeToEye = 'eyeToEye' in options ? options.eyeToEye : true;

    // instance properties
    this.active = false;
    this.uuid = null;
    this.interval = null;
    this.selectedJoint = null;
    this.positionNeedsUpdate = true;
}

Doppleganger.prototype = {
    toggle: function() {
        if (this.active) {
            print('toggling off');
            this.off();
            this.active = false;
        } else {
            print('toggling on');
            this.on();
            this.active = true;
        }
        return this.active;
    },
    syncJointName: 'LeftEye',
    update: function() {
        try {
            if (!this.uuid) {
                throw new Error('!this.uuid');
            }
            var rotations = this.avatar.getJointRotations();
            var translations = this.avatar.getJointTranslations() || rotations.map(function(_, i) {
                return this.avatar.getJointTranslation(i);
            }.bind(this));

            if (this.mirrored) {
                var mirroredIndexes = this.mirroredIndexes;
                var size = rotations.length;
                var outRotations = new Array(size);
                var outTranslations = new Array(size);
                for (var i=0; i < size; i++) {
                    var index = mirroredIndexes[i] === false ? i : mirroredIndexes[i];
                    var rot = rotations[index];
                    var trans = translations[index];
                    trans.x *= -1;
                    rot.y *= -1;
                    rot.z *= -1;
                    outRotations[i] = rot;
                    outTranslations[i] = trans;
                }
                rotations = outRotations;
                translations = outTranslations;
            }
            Overlays.editOverlay(this.uuid, {
                visible: true,
                jointRotations: rotations,
                jointTranslations: translations,
            });

            if (this.positionNeedsUpdate) {
                this.positionNeedsUpdate = this.eyeToEye; // only continue updating if seeing eye-to-eye
                this.fixVerticalPosition();
            }
            if (this.debug) {
                this._drawDebugOverlays();
            }
        } catch(e) {
            print('update ERROR: '+ e);
            this.off();
        }
    },
    on: function() {
        if (this.uuid) {
            print('doppleganger -- on() called but overlay model already exists', this.uuid);
            return;
        }
        this.position = Vec3.sum(this.avatar.position, Quat.getFront(this.avatar.orientation));
        this.orientation = this.avatar.orientation;
        this.skeletonModelURL = this.avatar.skeletonModelURL;

        this.jointNames = this.avatar.jointNames;
        this.mirroredNames = this.jointNames.map(function(name, i) {
            if (/Left/.test(name)) {
                return name.replace('Left', 'Right');
            }
            if (/Right/.test(name)) {
                return name.replace('Right', 'Left');
            }
        });

        this.mirroredIndexes = this.mirroredNames.map(function(name) {
            return name ? this.avatar.getJointIndex(name) : false;
        }.bind(this));

        this.uuid = Overlays.addOverlay('model', {
            visible: false,
            url: this.skeletonModelURL,
            position: this.position,
            rotation: this.orientation,
        });

        this._onModelReady(function() {
            if (USE_SCRIPT_UPDATE) {
                this.onUpdate = this.update;
                Script.update.connect(this, 'onUpdate');
                print('doppleganger will be updated from Script.update');
            } else {
                print('doppleganger will be updated using Script.setInterval at', TARGET_FPS +'fps');
                this.interval = Script.setInterval(this.update.bind(this), 1000 / TARGET_FPS);
            }
        });

        print('doppleganger created; overlayID =', this.uuid);

        // trigger clean up (and stop updates) if the overlay gets deleted by any means
        this.onDeletedOverlay = this._onDeletedOverlay;
        Overlays.overlayDeleted.connect(this, 'onDeletedOverlay');

        // FIXME: remove this hook after verifying joint modes between mirrored/non-mirrored
        if (true) {
            this.onMousePressEvent = this._onMousePressEvent;
            Controller.mousePressEvent.connect(this, 'onMousePressEvent');
        }
    },

    // execute callback only after the ModelOverlay has finished loading
    _onModelReady: function(callback) {
        const TIMEOUT_MS = 50;
        var waited = 0;
        var waitForJointNames = function() {
            var names = Overlays.getProperty(this.uuid, 'jointNames');
            if (Array.isArray(names) && names.length) {
                print('jointNames', names);
                callback.call(this, names);
            } else {
                print(waited++, 'waiting for doppleganger jointNames...');
                Script.setTimeout(waitForJointNames, TIMEOUT_MS);
            }
        }.bind(this);
        return Script.setTimeout(waitForJointNames, TIMEOUT_MS);
    },
    shutdown: function() {
        this.off();
        this.active = false;
    },
    off: function() {
        if (this.onUpdate) {
            Script.update.disconnect(this, 'onUpdate');
            delete this.onUpdate;
        }
        if (this.onDeletedOverlay) {
            Overlays.overlayDeleted.disconnect(this, 'onDeletedOverlay');
            delete this.onDeletedOverlay;
        }
        if (this.onMousePressEvent) {
            Controller.mousePressEvent.disconnect(this, 'onMousePressEvent');
            delete this.onMousePressEvent;
        }
        if (this.interval) {
            Script.clearInterval(this.interval);
            this.interval = undefined;
        }
        if (this.uuid) {
            Overlays.deleteOverlay(this.uuid);
            this.uuid = undefined;
        }
        if (this.debugOverlayIDs) {
            this.debugOverlayIDs.forEach(function(o) { Overlays.deleteOverlay(o); });
            this.debugOverlayIDs = undefined;
        }
    },

    // ModelOverlays & ModelEntities get positioned slightly differently than rigged Avatars
    // in EYE_TO_EYE mode this helper takes an actual measurement and adjusts the doppleganger
    // so it always sees "eye to eye" with the avatar
    fixVerticalPosition: function() {
        var byJointName = this.syncJointName || 'Hips';

        var names = Overlays.getProperty(this.uuid, 'jointNames'),
            positions = Overlays.getProperty(this.uuid, 'jointPositions'),
            dopplePosition = Overlays.getProperty(this.uuid, 'position'),
            doppleJointIndex = names.indexOf(byJointName),
            doppleJointPosition = positions[doppleJointIndex];

        var avatarPosition = this.avatar.position,
            avatarJointIndex = this.avatar.getJointIndex(byJointName),
            avatarJointPosition = this.avatar.getAbsoluteJointTranslationInObjectFrame(avatarJointIndex);

        var offset = Vec3.subtract(avatarJointPosition, doppleJointPosition);

        dopplePosition.y = avatarPosition.y + offset.y;
        this.position = dopplePosition;
        Overlays.editOverlay(this.uuid, { position: this.position });
    },

    _onDeletedOverlay: function(uuid) {
        print('onDeletedOverlay', uuid);
        if (uuid === this.uuid) {
            this.off();
        }
    },

    // DEBUG methods for verifying mirrored joint behaviors
    _onMousePressEvent: function(evt) {
        if (evt.isRightButton) {
            this.mirrored = !this.mirrored;
        }
        if (!this.debug || !evt.isLeftButton) {
            return;
        }
        var ray = Camera.computePickRay(evt.x, evt.y),
            hit = Overlays.findRayIntersection(ray, true, this.debugOverlayIDs);

        hit.index = this.debugOverlayIDs.indexOf(hit.overlayID);
        hit.jointName = this.jointNames[hit.index];
        hit.mirroredJointName = this.mirroredNames[hit.index];
        this.selectedJoint = hit.jointName;
        print('selected joint:', JSON.stringify(hit,0,2));
    },

    _drawDebugOverlays: function() {
        const COLOR_DEFAULT = { red: 255, blue: 255, green: 255 };
        const COLOR_SELECTED = { red: 0, blue: 255, green: 0 };

        var id = this.uuid,
            jointOrientations = Overlays.getProperty(id, 'jointOrientations'),
            jointPositions = Overlays.getProperty(id, 'jointPositions'),
            position = Overlays.getProperty(id, 'position'),
            orientation = Overlays.getProperty(id, 'orientation'),
            selectedIndex = this.jointNames.indexOf(this.selectedJoint);

        if (!this.debugOverlayIDs) {
            // set up reference shapes per joint
            this.debugOverlayIDs = jointOrientations.map(function(name, i) {
                return Overlays.addOverlay('shape', {
                    shape: 'Icosahedron',
                    scale: .1,
                    drawInFront: false,
                    text: this.jointNames[i],
                    solid: false,
                    alpha: .5,
                });
            }.bind(this));
        }

        // group updates into { id: {props...}, ... } format
        var updatedOverlays = this.debugOverlayIDs.reduce(function(updates, id, i) {
            updates[id] = {
                position: jointPositions[i],//Vec3.sum(position, Vec3.multiplyQbyV(orientation, jointPositions[i])),
                rotation: jointOrientations[i],
                color: i === selectedIndex ? COLOR_SELECTED : COLOR_DEFAULT,
                solid: i === selectedIndex,
            };
            return updates;
        }, {});
        Overlays.editOverlays(updatedOverlays);
    },
};


