"use strict";

//  doppleganger.js
//
//  Created by Timothy Dedischew on 04/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global module */
// @module doppleganger
//
// This module contains the `Doppleganger` class implementation for creating an inspectable replica of
// an Avatar (as a model directly in front of and facing them).  Joint positions and rotations are copied
// over in an update thread, so that the model automatically mirrors the Avatar's joint movements.
// An Avatar can then for example walk around "themselves" and examine from the back, etc.
//
// This should be helpful for inspecting your own look and debugging avatars, etc.
//
// The doppleganger is created as an overlay so that others do not see it -- and this also allows for the
// highest possible update rate when keeping joint data in sync.

module.exports = Doppleganger;

// @property {bool} - when set true, Script.update will be used instead of setInterval for syncing joint data
Doppleganger.USE_SCRIPT_UPDATE = false;

// @property {int} - the frame rate to target when using setInterval for joint updates
Doppleganger.TARGET_FPS = 60;

// @class Doppleganger - Creates a new instance of a Doppleganger.
// @param {Avatar} [options.avatar=MyAvatar] - Avatar used to retrieve position and joint data.
// @param {bool}   [options.mirrored=true]   - Apply "symmetric mirroring" of Left/Right joints.
// @param {bool}   [options.autoUpdate=true] - Automatically sync joint data.
function Doppleganger(options) {
    options = options || {};
    this.avatar = options.avatar || MyAvatar;
    this.mirrored = 'mirrored' in options ? options.mirrored : true;
    this.autoUpdate = 'autoUpdate' in options ? options.autoUpdate : true;

    // @public
    this.active = false; // whether doppleganger is currently being displayed/updated
    this.uuid = null;    // current doppleganger's Overlay id
    this.frame = 0;      // current joint update frame

    // @signal - emitted when .active state changes
    this.activeChanged = signal(function(active) {});
    // @signal - emitted once model overlay is either loaded or times out
    this.modelOverlayLoaded = signal(function(error, result){});
}

Doppleganger.prototype = {
    // @public @method - toggles doppleganger on/off
    toggle: function() {
        if (this.active) {
            log('toggling off');
            this.stop();
        } else {
            log('toggling on');
            this.start();
        }
        return this.active;
    },

    // @public @method - re-initialize model if Avatar changed skeletonModelURLs
    refreshAvatarModel: function(forceRefresh) {
        if (forceRefresh || (this.active && this.skeletonModelURL !== this.avatar.skeletonModelURL)) {
            var currentState = { position: this.position, orientation: this.orientation };
            this.stop();
            // turn back on with next script update tick
            Script.setTimeout(bind(this, function() {
                log('recreating doppleganger with latest model:', this.avatar.skeletonModelURL);
                this.start(currentState);
            }), 0);
            return true;
        }
    },

    // @public @method - synchronize the joint data between Avatar / doppleganger
    update: function() {
        this.frame++;
        try {
            if (!this.uuid) {
                throw new Error('!this.uuid');
            }

            if (this.avatar.skeletonModelURL !== this.skeletonModelURL) {
                return this.refreshAvatarModel();
            }

            var rotations = this.avatar.getJointRotations();
            var translations = this.avatar.getJointTranslations();
            var size = rotations.length;

            // note: this mismatch can happen when the avatar's model is actively changing
            if (size !== translations.length ||
                (this._lastRotationLength && size !== this._lastRotationLength)) {
                log('lengths differ', size, translations.length, this._lastRotationLength);
                this._lastRotationLength = 0;
                this.stop();
                this.skeletonModelURL = null;
                // wait a second before restarting
                Script.setTimeout(bind(this, function() {
                    this.refreshAvatarModel(true);
                }), 1000);
                return;
            }
            this._lastRotationLength = size;

            if (this.mirrored) {
                var mirroredIndexes = this.mirroredIndexes;
                var outRotations = new Array(size);
                var outTranslations = new Array(size);
                for (var i=0; i < size; i++) {
                    var index = mirroredIndexes[i];
                    if (index < 0 || index === false) {
                        index = i;
                    }
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
                jointRotations: rotations,
                jointTranslations: translations
            });

            // debug plumbing
            if (this.$update) {
                this.$update();
            }
        } catch (e) {
            log('.update error: '+ e, index);
            this.stop();
        }
    },

    // @public @method - show the doppleganger (and start the update thread, if options.autoUpdate was specified).
    start: function(transform) {
        if (this.uuid) {
            log('on() called but overlay model already exists', this.uuid);
            return;
        }
        transform = transform || {};
        this.activeChanged(this.active = true);
        this.frame = 0;

        this.position = transform.position || Vec3.sum(this.avatar.position, Quat.getForward(this.avatar.orientation));
        this.orientation = transform.orientation || this.avatar.orientation;
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

        var avatar = this.avatar;
        this.mirroredIndexes = this.mirroredNames.map(function(name) {
            return name ? avatar.getJointIndex(name) : false;
        });

        this.uuid = Overlays.addOverlay('model', {
            visible: false,
            url: this.skeletonModelURL,
            position: this.position,
            rotation: this.orientation
        });

        this._waitForModel();
        this.onModelOverlayLoaded = function(error, result) {
            if (error || this.uuid !== result.uuid) {
                return;
            }
            Overlays.editOverlay(this.uuid, { visible: true });
            if (!transform.position) {
                this.syncVerticalPosition();
            }
            log('ModelOverlay is ready; # joints == ' + result.jointNames.length);
            if (this.autoUpdate) {
                this._createUpdateThread();
            }
        };
        this.modelOverlayLoaded.connect(this, 'onModelOverlayLoaded');

        log('doppleganger created; overlayID =', this.uuid);

        // trigger clean up (and stop updates) if the overlay gets deleted
        this.onDeletedOverlay = this._onDeletedOverlay;
        Overlays.overlayDeleted.connect(this, 'onDeletedOverlay');

        if ('onLoadComplete' in this.avatar) {
            // restart the doppleganger if Avatar loads a different model URL
            this.onLoadComplete = this.refreshAvatarModel;
            this.avatar.onLoadComplete.connect(this, 'onLoadComplete');
        }

        // debug plumbing
        if (this.$start) {
            this.$start();
        }
    },

    // @public @method - hide the doppleganger
    stop: function() {
        if (this.onUpdate) {
            Script.update.disconnect(this, 'onUpdate');
            delete this.onUpdate;
        }
        if (this._interval) {
            Script.clearInterval(this._interval);
            this._interval = undefined;
        }
        if (this.onDeletedOverlay) {
            Overlays.overlayDeleted.disconnect(this, 'onDeletedOverlay');
            delete this.onDeletedOverlay;
        }
        if (this.onLoadComplete) {
            this.avatar.onLoadComplete.disconnect(this, 'onLoadComplete');
            delete this.onLoadComplete;
        }
        if (this.onModelOverlayLoaded) {
            this.modelOverlayLoaded.disconnect(this, 'onModelOverlayLoaded');
        }
        if (this.uuid) {
            Overlays.deleteOverlay(this.uuid);
            this.uuid = undefined;
        }
        this.activeChanged(this.active = false);
        // debug plumbing
        if (this.$stop) {
            this.$stop();
        }
    },

    // @public @method - Reposition the doppleganger so it sees "eye to eye" with the Avatar.
    // @param {String} [byJointName=Hips] - the reference joint that will be used to vertically match positions with Avatar
    // @note This method attempts to make a direct measurement and then calculate where the doppleganger needs to be
    //   in order to line-up vertically with the Avatar.  Otherwise, animations such as "away" mode can
    //   result in the doppleganger floating above or below ground level.
    syncVerticalPosition: function s(byJointName) {
        byJointName = byJointName || 'Hips';
        var names = Overlays.getProperty(this.uuid, 'jointNames'),
            positions = Overlays.getProperty(this.uuid, 'jointPositions'),
            dopplePosition = Overlays.getProperty(this.uuid, 'position'),
            doppleJointIndex = names.indexOf(byJointName),
            doppleJointPosition = positions[doppleJointIndex];

        var avatarPosition = this.avatar.position,
            avatarJointIndex = this.avatar.getJointIndex(byJointName),
            avatarJointPosition = this.avatar.getJointPosition(avatarJointIndex);

        var offset = avatarJointPosition.y - doppleJointPosition.y;
        log('adjusting for offset', offset);
        dopplePosition.y = avatarPosition.y + offset;
        this.position = dopplePosition;
        Overlays.editOverlay(this.uuid, { position: this.position });
    },

    // @private @method - signal handler for Overlays.overlayDeleted
    _onDeletedOverlay: function(uuid) {
        if (uuid === this.uuid) {
            log('onDeletedOverlay', uuid);
            this.stop();
        }
    },

    // @private @method - creates the update thread to synchronize joint data
    _createUpdateThread: function() {
        if (!this.autoUpdate) {
            log('options.autoUpdate == false -- call .update() manually to sync joint data');
            return;
        }
        if (Doppleganger.USE_SCRIPT_UPDATE) {
            log('creating Script.update thread');
            this.onUpdate = this.update;
            Script.update.connect(this, 'onUpdate');
        } else {
            log('creating Script.setInterval thread @ ~', Doppleganger.TARGET_FPS +'fps');
            var timeout = 1000 / Doppleganger.TARGET_FPS;
            this._interval = Script.setInterval(bind(this, 'update'), timeout);
        }
    },

    // @private @method - waits for model to load and handles timeouts
    // @note This is needed because sometimes it takes a few frames for the underlying model
    //   to become loaded even when already cached locally.
    _waitForModel: function(callback) {
        var RECHECK_MS = 50, MAX_WAIT_MS = 10000;
        var id = this.uuid,
            watchdogTimer = null;

        function waitForJointNames() {
            if (!watchdogTimer) {
                log('timeout waiting for ModelOverlay jointNames');
                Script.clearInterval(this._interval);
                this._interval = null;
                return this.modelOverlayLoaded(new Error('could not retrieve jointNames'), null);
            }
            var names = Overlays.getProperty(id, 'jointNames');
            if (Array.isArray(names) && names.length) {
                Script.clearInterval(this._interval);
                this._interval = null;
                return this.modelOverlayLoaded(null, { uuid: id, jointNames: names });
            }
        }
        watchdogTimer = Script.setTimeout(function() {
            watchdogTimer = null;
        }, MAX_WAIT_MS);
        this._interval = Script.setInterval(bind(this, waitForJointNames), RECHECK_MS);
    }
};

// @function - bind a function to a `this` context
// @param {Object} - the `this` context
// @param {Function|String} - function or method name
function bind(thiz, method) {
    method = thiz[method] || method;
    return function() {
        return method.apply(thiz, arguments);
    };
}

// @function - Qt signal polyfill
function signal(template) {
    var callbacks = [];
    return Object.defineProperties(function() {
        var args = [].slice.call(arguments);
        callbacks.forEach(function(obj) {
            obj.handler.apply(obj.scope, args);
        });
    }, {
        connect: { value: function(scope, handler) {
            callbacks.push({scope: scope, handler: scope[handler] || handler || scope});
        }},
        disconnect: { value: function(scope, handler) {
            var match = {scope: scope, handler: scope[handler] || handler || scope};
            callbacks = callbacks.filter(function(obj) {
                return !(obj.scope === match.scope && obj.handler === match.handler);
            });
        }}
    });
}

// @function - debug logging
function log() {
    print('doppleganger | ' + [].slice.call(arguments).join(' '));
}

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
//    var doppleganger = new Doppleganger();
//    Doppleganger.addDebugControls(doppleganger);
Doppleganger.addDebugControls = function(doppleganger) {
    var onMousePressEvent,
        debugOverlayIDs,
        selectedJointName;

    if ('$update' in doppleganger) {
        throw new Error('only one set of debug controls can be added per doppleganger');
    }

    function $start() {
        onMousePressEvent = _onMousePressEvent;
        Controller.mousePressEvent.connect(this, _onMousePressEvent);
    }

    function createOverlays(jointNames) {
        return jointNames.map(function(name, i) {
            return Overlays.addOverlay('shape', {
                shape: 'Icosahedron',
                scale: 0.1,
                solid: false,
                alpha: 0.5
            });
        });
    }

    function cleanupOverlays() {
        if (debugOverlayIDs) {
            debugOverlayIDs.forEach(Overlays.deleteOverlay);
            debugOverlayIDs = undefined;
        }
    }

    function $stop() {
        if (onMousePressEvent) {
            Controller.mousePressEvent.disconnect(this, onMousePressEvent);
            onMousePressEvent = undefined;
        }
        cleanupOverlays();
    }

    function $update() {
        var COLOR_DEFAULT = { red: 255, blue: 255, green: 255 };
        var COLOR_SELECTED = { red: 0, blue: 255, green: 0 };

        var id = this.uuid,
            jointOrientations = Overlays.getProperty(id, 'jointOrientations'),
            jointPositions = Overlays.getProperty(id, 'jointPositions'),
            selectedIndex = this.jointNames.indexOf(selectedJointName);

        if (!debugOverlayIDs) {
            debugOverlayIDs = createOverlays(this.jointNames);
        }

        // batch all updates into a single call (using the editOverlays({ id: {props...}, ... }) API)
        var updatedOverlays = debugOverlayIDs.reduce(function(updates, id, i) {
            updates[id] = {
                position: jointPositions[i],
                rotation: jointOrientations[i],
                color: i === selectedIndex ? COLOR_SELECTED : COLOR_DEFAULT,
                solid: i === selectedIndex
            };
            return updates;
        }, {});
        Overlays.editOverlays(updatedOverlays);
    }

    function _onMousePressEvent(evt) {
        if (evt.isRightButton) {
            if (evt.isShifted) {
                if (this.$update) {
                    // toggle debug overlays off
                    cleanupOverlays();
                    delete this.$update;
                } else {
                    this.$update = $update;
                }
            } else {
                this.mirrored = !this.mirrored;
            }
            return;
        }
        if (!evt.isLeftButton || !debugOverlayIDs) {
            return;
        }

        var ray = Camera.computePickRay(evt.x, evt.y),
            hit = Overlays.findRayIntersection(ray, true, debugOverlayIDs);

        hit.jointIndex = debugOverlayIDs.indexOf(hit.overlayID);
        if (hit.jointIndex < 0) {
            return;
        }
        hit.jointName = this.jointNames[hit.jointIndex];
        hit.mirroredJointName = this.mirroredNames[hit.jointIndex];
        selectedJointName = hit.jointName;
        log('selected joint:', JSON.stringify(hit, 0, 2));
    }

    doppleganger.$start = $start;
    doppleganger.$stop = $stop;
    doppleganger.$update = $update;

    return doppleganger;
};
