"use strict";

//  doppleganger.js
//
//  Created by Timothy Dedischew on 04/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* eslint-env commonjs */
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

var _modelHelper = require('./model-helper.js'),
    modelHelper = _modelHelper.modelHelper,
    ModelReadyWatcher = _modelHelper.ModelReadyWatcher;

// @property {bool} - toggle verbose debug logging on/off
Doppleganger.WANT_DEBUG = false;

// @property {bool} - when set true, Script.update will be used instead of setInterval for syncing joint data
Doppleganger.USE_SCRIPT_UPDATE = false;

// @property {int} - the frame rate to target when using setInterval for joint updates
Doppleganger.TARGET_FPS = 60;

// @class Doppleganger - Creates a new instance of a Doppleganger.
// @param {Avatar} [options.avatar=MyAvatar] - Avatar used to retrieve position and joint data.
// @param {bool}   [options.mirrored=true]   - Apply "symmetric mirroring" of Left/Right joints.
// @param {bool}   [options.autoUpdate=true] - Automatically sync joint data.
function Doppleganger(options) {
    this.options = options = options || {};
    this.avatar = options.avatar || MyAvatar;
    this.mirrored = 'mirrored' in options ? options.mirrored : true;
    this.autoUpdate = 'autoUpdate' in options ? options.autoUpdate : true;

    // @public
    this.active = false;   // whether doppleganger is currently being displayed/updated
    this.objectID = null; // current doppleganger's Overlay or Entity id
    this.frame = 0;        // current joint update frame

    // @signal - emitted when .active state changes
    this.activeChanged = signal(function(active, reason) {});
    // @signal - emitted once model is either loaded or errors out
    this.modelLoaded = signal(function(error, result){});
    // @signal - emitted each time the model's joint data has been synchronized
    this.jointsUpdated = signal(function(objectID){});
}

Doppleganger.prototype = {
    // @public @method - toggles doppleganger on/off
    toggle: function() {
        if (this.active) {
            debugPrint('toggling off');
            this.stop();
        } else {
            debugPrint('toggling on');
            this.start();
        }
        return this.active;
    },

    // @public @method - synchronize the joint data between Avatar / doppleganger
    update: function() {
        this.frame++;
        try {
            if (!this.objectID) {
                throw new Error('!this.objectID');
            }

            if (this.avatar.skeletonModelURL !== this.skeletonModelURL) {
                return this.stop('avatar_changed');
            }

            var rotations = this.avatar.getJointRotations();
            var translations = this.avatar.getJointTranslations();
            var size = rotations.length;

            // note: this mismatch can happen when the avatar's model is actively changing
            if (size !== translations.length ||
                (this.jointStateCount && size !== this.jointStateCount)) {
                debugPrint('mismatched joint counts (avatar model likely changed)', size, translations.length, this.jointStateCount);
                this.stop('avatar_changed_joints');
                return;
            }
            this.jointStateCount = size;

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
            modelHelper.editObject(this.objectID, {
                jointRotations: rotations,
                jointTranslations: translations
            });
            this.jointsUpdated(this.objectID);
        } catch (e) {
            log('.update error: '+ e, index, e.stack);
            this.stop('update_error');
            throw e;
        }
    },

    // @public @method - show the doppleganger (and start the update thread, if options.autoUpdate was specified).
    // @param {vec3} [options.position=(in front of avatar)] - starting position
    // @param {quat} [options.orientation=avatar.orientation] - starting orientation
    start: function(options) {
        options = assign(this.options, options);
        if (this.objectID) {
            log('start() called but object model already exists', this.objectID);
            return;
        }
        var avatar = this.avatar;
        if (!avatar.jointNames.length) {
            return this.stop('joints_unavailable');
        }

        this.frame = 0;
        var localPosition = Vec3.multiply(2, Quat.getForward(avatar.orientation));
        this.position = options.position || Vec3.sum(avatar.position, localPosition);
        this.orientation = options.orientation || avatar.orientation;
        this.skeletonModelURL = avatar.skeletonModelURL;
        this.scale = avatar.scale || 1.0;
        this.jointStateCount = 0;
        this.jointNames = avatar.jointNames;
        this.type = options.type || 'overlay';
        this.mirroredNames = modelHelper.deriveMirroredJointNames(this.jointNames);
        this.mirroredIndexes = this.mirroredNames.map(function(name) {
            return name ? avatar.getJointIndex(name) : false;
        });

        this.objectID = modelHelper.addObject({
            type: this.type === 'overlay' ? 'model' : 'Model',
            modelURL: this.skeletonModelURL,
            position: this.position,
            rotation: this.orientation,
            scale: this.scale
        });
        Script.scriptEnding.connect(this, function() {
            modelHelper.deleteObject(this.objectID);
        });
        debugPrint('doppleganger created; objectID =', this.objectID);

        // trigger clean up (and stop updates) if the object gets deleted
        this.onObjectDeleted = function(uuid) {
            if (uuid === this.objectID) {
                log('onObjectDeleted', uuid);
                this.stop('object_deleted');
            }
        };
        modelHelper.objectDeleted.connect(this, 'onObjectDeleted');

        if ('onLoadComplete' in avatar) {
            // stop the current doppleganger if Avatar loads a different model URL
            this.onLoadComplete = function() {
                if (avatar.skeletonModelURL !== this.skeletonModelURL) {
                    this.stop('avatar_changed_load');
                }
            };
            avatar.onLoadComplete.connect(this, 'onLoadComplete');
        }

        this.onModelLoaded = function(error, result) {
            if (error) {
                return this.stop(error);
            }
            debugPrint('model ('+modelHelper.type(this.objectID)+')' + ' is ready; # joints == ' + result.jointNames.length);
            var naturalDimensions = modelHelper.getProperties(this.objectID, ['naturalDimensions']).naturalDimensions;
            debugPrint('naturalDimensions:', JSON.stringify(naturalDimensions));
            var props = { visible: true };
            if (naturalDimensions) {
                props.dimensions = Vec3.multiply(this.scale, naturalDimensions);
            }
            debugPrint('scaledDimensions:', this.scale, JSON.stringify(props.dimensions));
            modelHelper.editObject(this.objectID, props);
            if (!options.position) {
                this.syncVerticalPosition();
            }
            if (this.autoUpdate) {
                this._createUpdateThread();
            }
        };

        this._resource = ModelCache.prefetch(this.skeletonModelURL);
        this._modelReadier = new ModelReadyWatcher({
            resource: this._resource,
            objectID: this.objectID
        });
        this._modelReadier.modelReady.connect(this, 'onModelLoaded');
        this.activeChanged(this.active = true, 'start');
    },

    // @public @method - hide the doppleganger
    // @param {String} [reason=stop] - the reason stop was called
    stop: function(reason) {
        reason = reason || 'stop';
        if (this.onUpdate) {
            Script.update.disconnect(this, 'onUpdate');
            delete this.onUpdate;
        }
        if (this._interval) {
            Script.clearInterval(this._interval);
            this._interval = undefined;
        }
        if (this.onObjectDeleted) {
            modelHelper.objectDeleted.disconnect(this, 'onObjectDeleted');
            delete this.onObjectDeleted;
        }
        if (this.onLoadComplete) {
            this.avatar.onLoadComplete.disconnect(this, 'onLoadComplete');
            delete this.onLoadComplete;
        }
        if (this.onModelLoaded) {
            this._modelReadier && this._modelReadier.modelReady.disconnect(this, 'onModelLoaded');
            this._modelReadier = this.onModelLoaded = undefined;
        }
        if (this.objectID) {
            modelHelper.deleteObject(this.objectID);
            this.objectID = undefined;
        }
        if (this.active) {
            this.activeChanged(this.active = false, reason);
        } else if (reason) {
            debugPrint('already stopped so not triggering another activeChanged; latest reason was:', reason);
        }
    },
    // @public @method - Reposition the doppleganger so it sees "eye to eye" with the Avatar.
    // @param {String} [byJointName=Hips] - the reference joint used to align the Doppleganger and Avatar
    syncVerticalPosition: function(byJointName) {
        byJointName = byJointName || 'Hips';
        var positions = modelHelper.getJointPositions(this.objectID),
            properties = modelHelper.getProperties(this.objectID),
            dopplePosition = properties.position,
            doppleJointIndex = modelHelper.getJointIndex(this.objectID, byJointName),// names.indexOf(byJointName),
            doppleJointPosition = positions[doppleJointIndex];

        debugPrint('........... doppleJointPosition', JSON.stringify({
            byJointName: byJointName,
            dopplePosition: dopplePosition,
            doppleJointIndex: doppleJointIndex,
            doppleJointPosition: doppleJointPosition,
            properties: properties.type,
            positions: positions[0]
        },0,2));
        var avatarPosition = this.avatar.position,
            avatarJointIndex = this.avatar.getJointIndex(byJointName),
            avatarJointPosition = this.avatar.getJointPosition(avatarJointIndex);

        var offset = (avatarJointPosition.y - doppleJointPosition.y);
        debugPrint('adjusting for offset', offset);
        if (properties.type === 'model') {
            dopplePosition.y = avatarPosition.y + offset;
        } else {
            dopplePosition.y = avatarPosition.y - offset;
        }

        this.position = dopplePosition;
        modelHelper.editObject(this.objectID, { position: this.position });
    },

    // @private @method - creates the update thread to synchronize joint data
    _createUpdateThread: function() {
        if (Doppleganger.USE_SCRIPT_UPDATE) {
            debugPrint('creating Script.update thread');
            this.onUpdate = this.update;
            Script.update.connect(this, 'onUpdate');
        } else {
            debugPrint('creating Script.setInterval thread @ ~', Doppleganger.TARGET_FPS +'fps');
            var timeout = 1000 / Doppleganger.TARGET_FPS;
            this._interval = Script.setInterval(bind(this, 'update'), timeout);
        }
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
            var callback = {scope: scope, handler: scope[handler] || handler || scope};
            if (!callback.handler || !callback.handler.apply) {
                throw new Error('invalid arguments to connect:' + [template, scope, handler]);
            }
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

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/assign#Polyfill
/* eslint-disable */
function assign(target, varArgs) { // .length of function is 2
    'use strict';
    if (target == null) { // TypeError if undefined or null
        throw new TypeError('Cannot convert undefined or null to object');
    }

    var to = Object(target);

    for (var index = 1; index < arguments.length; index++) {
        var nextSource = arguments[index];

        if (nextSource != null) { // Skip over if undefined or null
            for (var nextKey in nextSource) {
                // Avoid bugs when hasOwnProperty is shadowed
                if (Object.prototype.hasOwnProperty.call(nextSource, nextKey)) {
                    to[nextKey] = nextSource[nextKey];
                }
            }
        }
    }
    return to;
}
/* eslint-enable */
// //https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/assign#Polyfill

// @function - debug logging
function log() {
    print('doppleganger | ' + [].slice.call(arguments).join(' '));
}

function debugPrint() {
    Doppleganger.WANT_DEBUG && log.apply(this, arguments);
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
    DebugControls.COLOR_DEFAULT = { red: 255, blue: 255, green: 255 };
    DebugControls.COLOR_SELECTED = { red: 0, blue: 255, green: 0 };

    function DebugControls() {
        this.enableIndicators = true;
        this.selectedJointName = null;
        this.debugOverlayIDs = undefined;
        this.jointSelected = signal(function(result) {});
    }
    DebugControls.prototype = {
        start: function() {
            if (!this.onMousePressEvent) {
                this.onMousePressEvent = this._onMousePressEvent;
                Controller.mousePressEvent.connect(this, 'onMousePressEvent');
            }
        },

        stop: function() {
            this.removeIndicators();
            if (this.onMousePressEvent) {
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
            if (!evt.isLeftButton || !this.enableIndicators || !this.debugOverlayIDs) {
                return;
            }
            var ray = Camera.computePickRay(evt.x, evt.y),
                hit = Overlays.findRayIntersection(ray, true, this.debugOverlayIDs);

            hit.jointIndex = this.debugOverlayIDs.indexOf(hit.overlayID);
            hit.jointName = this.jointNames[hit.jointIndex];
            this.jointSelected(hit);
        }
    };

    if ('$debugControls' in doppleganger) {
        throw new Error('only one set of debug controls can be added per doppleganger');
    }
    var debugControls = new DebugControls();
    doppleganger.$debugControls = debugControls;

    function onMousePressEvent(evt) {
        if (evt.isRightButton) {
            if (evt.isShifted) {
                debugControls.enableIndicators = !debugControls.enableIndicators;
                if (!debugControls.enableIndicators) {
                    debugControls.removeIndicators();
                }
            } else {
                doppleganger.mirrored = !doppleganger.mirrored;
            }
        }
    }

    doppleganger.activeChanged.connect(function(active) {
        if (active) {
            debugControls.start();
            doppleganger.jointsUpdated.connect(debugControls, 'onJointsUpdated');
            Controller.mousePressEvent.connect(onMousePressEvent);
        } else {
            Controller.mousePressEvent.disconnect(onMousePressEvent);
            doppleganger.jointsUpdated.disconnect(debugControls, 'onJointsUpdated');
            debugControls.stop();
        }
    });

    debugControls.jointSelected.connect(function(hit) {
        debugControls.selectedJointName = hit.jointName;
        if (hit.jointIndex < 0) {
            return;
        }
        hit.mirroredJointName = modelHelper.deriveMirroredJointNames([hit.jointName])[0];
        log('selected joint:', JSON.stringify(hit, 0, 2));
    });

    Script.scriptEnding.connect(debugControls, 'removeIndicators');

    return doppleganger;
};
