/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, {
/******/ 				configurable: false,
/******/ 				enumerable: true,
/******/ 				get: getter
/******/ 			});
/******/ 		}
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = 3);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports) {

/* eslint-env commonjs */
/* global console */

module.exports = {
    version: '0.0.1',
    bind: bind,
    signal: signal,
    assign: assign,
    assert: assert
};

function log() {
    // eslint-disable-next-line no-console
    (typeof console === 'object' ? console.log : print)('utils | ' + [].slice.call(arguments).join(' '));
}
log(module.exports.version);

// @function - bind a function to a `this` context
// @param {Object} - the `this` context
// @param {Function|String} - function or method name
// @param {value} varargs... - optional curry-right arguments (passed to method after any explicit arguments)
function bind(thiz, method, varargs) {
    method = thiz[method] || method;
    varargs = [].slice.call(arguments, 2);
    return function() {
        var args = [].slice.call(arguments).concat(varargs);
        return method.apply(thiz, args);
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

// examples:
//     assert(function assertion() { return (conditions === true) }, 'assertion failed!')
//     var neededValue = assert(idString, 'idString not specified!');
//     assert(false, 'unexpected state');
function assert(truthy, message) {
    message = message || 'Assertion Failed:';

    if (typeof truthy === 'function' && truthy.name === 'assertion') {
        // extract function body to display with the assertion message
        var assertion = (truthy+'').replace(/[\r\n]/g, ' ')
            .replace(/^[^{]+\{|\}$|^\s*|\s*$/g, '').trim()
            .replace(/^return /,'').replace(/\s[\r\n\t\s]+/g, ' ');
        message += ' ' + JSON.stringify(assertion);
        try {
            truthy = truthy();
        } catch (e) {
            message += '(exception: ' + e +')';
        }
    }
    if (!truthy) {
        message += ' ('+truthy+')';
        throw new Error(message);
    }
    return truthy;
}


/***/ }),
/* 1 */
/***/ (function(module, exports, __webpack_require__) {

//  model-helper.js
//
//  Created by Timothy Dedischew on 06/01/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* eslint-env commonjs */
/* global console */
// @module model-helper
//
// This module provides ModelReadyWatcher (a helper class for knowing when a model becomes usable inworld) and
// also initial plumbing helpers to eliminate unnecessary API differences when working with Model Overlays and
// Model Entities at a high-level from scripting.

var utils = __webpack_require__(0),
    assert = utils.assert;

module.exports = {
    version: '0.0.1',
    ModelReadyWatcher: ModelReadyWatcher
};

function log() {
    // eslint-disable-next-line no-console
    (typeof console === 'object' ? console.log : print)('model-helper | ' + [].slice.call(arguments).join(' '));
}
log(module.exports.version);

var _objectDeleted = utils.signal(function objectDeleted(objectID){});
// proxy for _objectDeleted that only binds deletion tracking if script actually connects to the unified signal
var objectDeleted = utils.assign(function objectDeleted(objectID){}, {
    connect: function() {
        Overlays.overlayDeleted.connect(_objectDeleted);
        // Entities.deletingEntity.connect(objectDeleted);
        Script.scriptEnding.connect(function() {
            Overlays.overlayDeleted.disconnect(_objectDeleted);
            // Entities.deletingEntity.disconnect(objectDeleted);
        });
        // hereafter _objectDeleted.connect will be used instead
        this.connect = utils.bind(_objectDeleted, 'connect');
        return this.connect.apply(this, arguments);
    },
    disconnect: utils.bind(_objectDeleted, 'disconnect')
});

var modelHelper = module.exports.modelHelper = {
    // Entity <-> Overlay property translations
    _entityFromOverlay: {
        modelURL: function url() {
            return this.url;
        },
        dimensions: function dimensions() {
            return Vec3.multiply(this.scale, this.naturalDimensions);
        }
    },
    _overlayFromEntity: {
        url: function modelURL() {
            return this.modelURL;
        },
        scale: function scale() {
            return this.dimensions && this.naturalDimensions && {
                x: this.dimensions.x / this.naturalDimensions.x,
                y: this.dimensions.y / this.naturalDimensions.y,
                z: this.dimensions.z / this.naturalDimensions.z
            };
        }
    },
    objectDeleted: objectDeleted,
    type: function(objectID) {
        // TODO: support Model Entities (by detecting type from objectID, which is already possible)
        return !Uuid.isNull(objectID) ? 'overlay' : null;
    },
    addObject: function(properties) {
        var type = 'overlay'; // this.resolveType(properties)
        switch (type) {
            case 'overlay': return Overlays.addOverlay(properties.type, this.toOverlayProps(properties));
        }
        return false;
    },
    editObject: function(objectID, properties) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.editOverlay(objectID, this.toOverlayProps(properties));
        }
        return false;
    },
    deleteObject: function(objectID) {
        return this.type(objectID) === 'overlay' && Overlays.deleteOverlay(objectID);
    },
    getProperty: function(objectID, propertyName) {
        return this.type(objectID) === 'overlay' && Overlays.getProperty(objectID, propertyName);
    },
    getProperties: function(objectID, filter) {
        switch (this.type(objectID)) {
            case 'overlay':
                filter = Array.isArray(filter) ? filter : [
                    'position', 'rotation', 'localPosition', 'localRotation', 'parentID',
                    'parentJointIndex', 'scale', 'name', 'visible', 'type', 'url',
                    'dimensions', 'naturalDimensions', 'grabbable'
                ];
                var properties = filter.reduce(function(out, propertyName) {
                    out[propertyName] = Overlays.getProperty(objectID, propertyName);
                    return out;
                }, {});
                return this.toEntityProps(properties);
        }
        return null;
    },
    // adapt Entity conventions to Overlay (eg: { modelURL: ... } -> { url: ... })
    toOverlayProps: function(properties) {
        var result = {};
        for (var from in this._overlayFromEntity) {
            var adapter = this._overlayFromEntity[from];
            result[from] = adapter.call(properties, from, adapter.name);
        }
        return utils.assign(result, properties);
    },
    // adapt Overlay conventions to Entity (eg: { url: ... } -> { modelURL: ... })
    toEntityProps: function(properties) {
        var result = {};
        for (var from in this._entityToOverlay) {
            var adapter = this._entityToOverlay[from];
            result[from] = adapter.call(properties, from, adapter.name);
        }
        return utils.assign(result, properties);
    },
    editObjects: function(updatedObjects) {
        var objectIDs = Object.keys(updatedObjects),
            type = this.type(objectIDs[0]);
        switch (type) {
            case 'overlay':
                var translated = {};
                for (var objectID in updatedObjects) {
                    translated[objectID] = this.toOverlayProps(updatedObjects[objectID]);
                }
                return Overlays.editOverlays(translated);
        }
        return false;
    },
    getJointIndex: function(objectID, name) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointNames').indexOf(name);
        }
        return -1;
    },
    getJointNames: function(objectID) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointNames');
        }
        return [];
    },
    // @function - derives mirrored joint names from a list of regular joint names
    // @param {Array} - list of joint names to mirror
    // @return {Array} - list of mirrored joint names (note: entries for non-mirrored joints will be `undefined`)
    deriveMirroredJointNames: function(jointNames) {
        return jointNames.map(function(name, i) {
            if (/Left/.test(name)) {
                return name.replace('Left', 'Right');
            }
            if (/Right/.test(name)) {
                return name.replace('Right', 'Left');
            }
            return undefined;
        });
    },
    getJointPosition: function(objectID, index) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointPositions')[index];
        }
        return Vec3.ZERO;
    },
    getJointPositions: function(objectID) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointPositions');
        }
        return [];
    },
    getJointOrientation: function(objectID, index) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointOrientations')[index];
        }
        return Quat.normalize({});
    },
    getJointOrientations: function(objectID) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointOrientations');
        }
    },
    getJointTranslation: function(objectID, index) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointTranslations')[index];
        }
        return Vec3.ZERO;
    },
    getJointTranslations: function(objectID) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointTranslations');
        }
        return [];
    },
    getJointRotation: function(objectID, index) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointRotations')[index];
        }
        return Quat.normalize({});
    },
    getJointRotations: function(objectID) {
        switch (this.type(objectID)) {
            case 'overlay': return Overlays.getProperty(objectID, 'jointRotations');
        }
        return [];
    }
}; // modelHelper


// @property {PreconditionFunction} - indicates when the model's jointNames have become available
ModelReadyWatcher.JOINTS = function(state) {
    return Array.isArray(state.jointNames);
};
// @property {PreconditionFunction} - indicates when a model's naturalDimensions have become available
ModelReadyWatcher.DIMENSIONS = function(state) {
    return Vec3.length(state.naturalDimensions) > 0;
};
// @property {PreconditionFunction} - indicates when both a model's naturalDimensions and jointNames have become available
ModelReadyWatcher.JOINTS_AND_DIMENSIONS = function(state) {
    // eslint-disable-next-line new-cap
    return ModelReadyWatcher.JOINTS(state) && ModelReadyWatcher.DIMENSIONS(state);
};
// @property {int} - interval used for continually rechecking model readiness, until ready or a timeout occurs
ModelReadyWatcher.RECHECK_MS = 50;
// @property {int} - default wait time before considering a model unready-able.
ModelReadyWatcher.DEFAULT_TIMEOUT_SECS = 10;

// @private @class - waits for model to become usable inworld and tracks errors/timeouts
// @param [Object} options -- key/value config options:
// @param {ModelResource} options.resource - a ModelCache prefetched resource to observe for determining load state
// @param {Uuid} options.objectID - an inworld object to observe for determining readiness states
// @param {Function} [options.precondition=ModelReadyWatcher.JOINTS] - the precondition used to determine if the model is usable
// @param {int} [options.maxWaitSeconds=10] - max seconds to wait for the model to become usable, after which a timeout error is emitted
// @return {ModelReadyWatcher}
function ModelReadyWatcher(options) {
    options = utils.assign({
        precondition: ModelReadyWatcher.JOINTS,
        maxWaitSeconds: ModelReadyWatcher.DEFAULT_TIMEOUT_SECS
    }, options);

    assert(!Uuid.isNull(options.objectID), 'Error: invalid options.objectID');
    assert(options.resource && 'state' in options.resource, 'Error: invalid options.resource');
    assert(typeof options.precondition === 'function', 'Error: invalid options.precondition');

    utils.assign(this, {
        resource: options.resource,
        objectID: options.objectID,
        precondition: options.precondition,

        // @signal - emitted when the model becomes ready, or with the error that prevented it
        modelReady: utils.signal(function modelReady(error, result){}),

        // @public
        ready: false, // tracks readiness state
        jointNames: null, // populated with detected jointNames
        naturalDimensions: null, // populated with detected naturalDimensions

        _startTime: new Date,
        _watchdogTimer: Script.setTimeout(utils.bind(this, function() {
            this._watchdogTimer = null;
        }), options.maxWaitSeconds * 1000),
        _interval: Script.setInterval(utils.bind(this, '_waitUntilReady'), ModelReadyWatcher.RECHECK_MS)
    });

    this.modelReady.connect(this, function(error, result) {
        this.ready = !error && result;
    });
}

ModelReadyWatcher.prototype = {
    contructor: ModelReadyWatcher,
    // @public method -- cancels monitoring and (if model was not yet ready) emits an error
    cancel: function() {
        return this._stop() && !this.ready && this.modelReady('cancelled', null);
    },
    // stop pending timers
    _stop: function() {
        var stopped = 0;
        if (this._watchdogTimer) {
            Script.clearTimeout(this._watchdogTimer);
            this._watchdogTimer = null;
            stopped++;
        }
        if (this._interval) {
            Script.clearInterval(this._interval);
            this._interval = null;
            stopped++;
        }
        return stopped;
    },
    // the monitoring thread func
    _waitUntilReady: function() {
        var error = null, result = null;
        if (!this._watchdogTimer) {
            error = this.precondition.name || 'timeout';
        } else if (this.resource.state === Resource.State.FAILED) {
            error = 'prefetch_failed';
        } else if (this.resource.state === Resource.State.FINISHED) {
            // in theory there will always be at least one "joint name" that represents the main submesh
            var names = modelHelper.getJointNames(this.objectID);
            if (Array.isArray(names) && names.length) {
                this.jointNames = names;
            }
            var props = modelHelper.getProperties(this.objectID, ['naturalDimensions']);
            if (props && props.naturalDimensions && Vec3.length(props.naturalDimensions)) {
                this.naturalDimensions = props.naturalDimensions;
            }
            var state = {
                resource: this.resource,
                objectID: this.objectID,
                waitTime: (new Date - this._startTime) / 1000,
                jointNames: this.jointNames,
                naturalDimensions: this.naturalDimensions
            };
            if (this.precondition(state)) {
                result = state;
            }
        }
        if (error || result !== null) {
            this._stop();
            this.modelReady(error, result);
        }
    }
}; // ModelReadyWatcher.prototype


/***/ }),
/* 2 */
/***/ (function(module, exports, __webpack_require__) {

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
/* global console */
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

Doppleganger.version = '0.0.1a';
log(Doppleganger.version);

var _modelHelper = __webpack_require__(1),
    modelHelper = _modelHelper.modelHelper,
    ModelReadyWatcher = _modelHelper.ModelReadyWatcher,
    utils = __webpack_require__(0);

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
    this.active = false; // whether doppleganger is currently being displayed/updated
    this.objectID = null; // current doppleganger's Overlay or Entity id
    this.frame = 0; // current joint update frame

    // @signal - emitted when .active state changes
    this.activeChanged = utils.signal(function(active, reason) {});
    // @signal - emitted once model is either loaded or errors out
    this.modelLoaded = utils.signal(function(error, result){});
    // @signal - emitted each time the model's joint data has been synchronized
    this.jointsUpdated = utils.signal(function(objectID){});
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
            var jointUpdates = {
                jointRotations: rotations,
                jointTranslations: translations
            };
            modelHelper.editObject(this.objectID, jointUpdates);
            this.jointsUpdated(this.objectID, jointUpdates);
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
        options = utils.assign(this.options, options);
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
            var naturalDimensions = this.naturalDimensions = modelHelper.getProperties(this.objectID, ['naturalDimensions']).naturalDimensions;
            debugPrint('naturalDimensions:', JSON.stringify(naturalDimensions));
            var props = { visible: true };
            if (naturalDimensions) {
                props.dimensions = this.dimensions = Vec3.multiply(this.scale, naturalDimensions);
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
            this._interval = Script.setInterval(utils.bind(this, 'update'), timeout);
        }
    }

};

// @function - debug logging
function log() {
    // eslint-disable-next-line no-console
    (typeof console === 'object' ? console.log : print)('doppleganger | ' + [].slice.call(arguments).join(' '));
}

function debugPrint() {
    Doppleganger.WANT_DEBUG && log.apply(this, arguments);
}


/***/ }),
/* 3 */
/***/ (function(module, exports, __webpack_require__) {

//  doppleganger-app.js
//
//  Created by Timothy Dedischew on 04/21/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  This Client script creates an instance of a Doppleganger that can be toggled on/off via tablet button.
//  (for more info see doppleganger.js)
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* eslint-env commonjs */
/* global DriveKeys, require:true, console */
/* eslint-disable comma-dangle */

// decomment next line for automatic require cache-busting
// var require = function require(id) { return Script.require(id + '?'+new Date().getTime().toString(36)); };
if (false) {
    require = Script.require;
}

var VERSION = '0.0.0';
var WANT_DEBUG = false;
var DEBUG_MOVE_AS_YOU_MOVE = false;
var ROTATE_AS_YOU_MOVE = false;

log(VERSION);

var DopplegangerClass = __webpack_require__(2),
    DopplegangerAttachments = __webpack_require__(4),
    DebugControls = __webpack_require__(5),
    modelHelper = __webpack_require__(1).modelHelper,
    utils = __webpack_require__(0);

// eslint-disable-next-line camelcase
var isWebpack = typeof __webpack_require__ === 'function';

var buttonConfig = utils.assign({
    text: 'MIRROR',
}, !isWebpack ? {
    icon: Script.resolvePath('./doppleganger-i.svg'),
    activeIcon: Script.resolvePath('./doppleganger-a.svg'),
} : {
    icon: __webpack_require__(6),
    activeIcon: __webpack_require__(7),
});

var tablet = Tablet.getTablet('com.highfidelity.interface.tablet.system'),
    button = tablet.addButton(buttonConfig);

Script.scriptEnding.connect(function() {
    tablet.removeButton(button);
    button = null;
});

var doppleganger = new DopplegangerClass({
    avatar: MyAvatar,
    mirrored: false,
    autoUpdate: true,
    type: 'overlay'
});

// add support for displaying regular (non-soft) attachments on the doppleganger
{
    var RECHECK_ATTACHMENT_MS = 1000;
    var dopplegangerAttachments = new DopplegangerAttachments(doppleganger),
        attachmentInterval = null,
        lastHash = dopplegangerAttachments.getAttachmentsHash();

    // monitor for attachment changes, but only when the doppleganger is active
    doppleganger.activeChanged.connect(function(active, reason) {
        if (attachmentInterval) {
            Script.clearInterval(attachmentInterval);
        }
        if (active) {
            attachmentInterval = Script.setInterval(checkAttachmentsHash, RECHECK_ATTACHMENT_MS);
        } else {
            attachmentInterval = null;
        }
    });
    function checkAttachmentsHash() {
        var currentHash = dopplegangerAttachments.getAttachmentsHash();
        if (currentHash !== lastHash) {
            lastHash = currentHash;
            debugPrint('app-doppleganger | detect attachment change');
            dopplegangerAttachments.refreshAttachments();
        }
    }
}

// add support for "move as you move"
{
    var movementKeys = 'W,A,S,D,Up,Down,Right,Left'.split(',');
    var controllerKeys = 'LX,LY,RY'.split(',');
    var translationKeys = Object.keys(DriveKeys).filter(function(p) {
        return /translate/i.test(p);
    });
    var startingPosition;

    // returns an array of any active driving keys (eg: ['W', 'TRANSLATE_Z'])
    function currentDrivers() {
        return [].concat(
            movementKeys.map(function(key) {
                return Controller.getValue(Controller.Hardware.Keyboard[key]) && key;
            })
        ).concat(
            controllerKeys.map(function(key) {
                return Controller.getValue(Controller.Standard[key]) !== 0.0 && key;
            })
        ).concat(
            translationKeys.map(function(key) {
                return MyAvatar.getRawDriveKey(DriveKeys[key]) !== 0.0 && key;
            })
        ).filter(Boolean);
    }

    doppleganger.jointsUpdated.connect(function(objectID) {
        var drivers = currentDrivers(),
            isDriving = drivers.length > 0;
        if (isDriving) {
            if (startingPosition) {
                debugPrint('resetting startingPosition since drivers == ', drivers.join('|'));
                startingPosition = null;
            }
        } else if (HMD.active || DEBUG_MOVE_AS_YOU_MOVE) {
            startingPosition = startingPosition || MyAvatar.position;
            var movement = Vec3.subtract(MyAvatar.position, startingPosition);
            startingPosition = MyAvatar.position;
            // Vec3.length(movement) > 0.0001 && Vec3.print('+avatarMovement', movement);

            // "mirror" the relative translation vector
            movement.x *= -1;
            movement.z *= -1;
            var props = {};
            props.position = doppleganger.position = Vec3.sum(doppleganger.position, movement);
            if (ROTATE_AS_YOU_MOVE) {
                props.rotation = doppleganger.orientation = MyAvatar.orientation;
            }
            modelHelper.editObject(doppleganger.objectID, props);
        }
    });
}

// hide the doppleganger if this client script is unloaded
Script.scriptEnding.connect(doppleganger, 'stop');

// hide the doppleganger if the user switches domains (which might place them arbitrarily far away in world space)
function onDomainChanged() {
    if (doppleganger.active) {
        doppleganger.stop('domain_changed');
    }
}
Window.domainChanged.connect(onDomainChanged);
Window.domainConnectionRefused.connect(onDomainChanged);
Script.scriptEnding.connect(function() {
    Window.domainChanged.disconnect(onDomainChanged);
    Window.domainConnectionRefused.disconnect(onDomainChanged);
});

// toggle on/off via tablet button
button.clicked.connect(doppleganger, 'toggle');

// highlight tablet button based on current doppleganger state
doppleganger.activeChanged.connect(function(active, reason) {
    if (button) {
        button.editProperties({ isActive: active });
        debugPrint('doppleganger.activeChanged', active, reason);
    }
});

// alert the user if there was an error applying their skeletonModelURL
doppleganger.modelLoaded.connect(function(error, result) {
    if (doppleganger.active && error) {
        Window.alert('doppleganger | ' + error + '\n' + doppleganger.skeletonModelURL);
    }
});

// ----------------------------------------------------------------------------

// add debug indicators, but only if the user has configured the settings value
if (Settings.getValue('debug.doppleganger', false)) {
    WANT_DEBUG = WANT_DEBUG || true;
    DopplegangerClass.WANT_DEBUG = WANT_DEBUG;
    DopplegangerAttachments.WANT_DEBUG = WANT_DEBUG;
    new DebugControls(doppleganger);
}

function log() {
    // eslint-disable-next-line no-console
    (typeof console === 'object' ? console.log : print)('app-doppleganger | ' + [].slice.call(arguments).join(' '));
}

function debugPrint() {
    WANT_DEBUG && log.apply(this, arguments);
}

// ----------------------------------------------------------------------------

UserActivityLogger.logAction('doppleganger_app_load');
doppleganger.activeChanged.connect(function(active, reason) {
    if (active) {
        UserActivityLogger.logAction('doppleganger_enable');
    } else {
        if (reason === 'stop') {
            // user intentionally toggled the doppleganger
            UserActivityLogger.logAction('doppleganger_disable');
        } else {
            debugPrint('doppleganger stopped:', reason);
            UserActivityLogger.logAction('doppleganger_autodisable', { reason: reason });
        }
    }
});
dopplegangerAttachments.attachmentsUpdated.connect(function(attachments) {
    UserActivityLogger.logAction('doppleganger_attachments', { count: attachments.length });
});



/***/ }),
/* 4 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";

/* eslint-env commonjs */
/* eslint-disable comma-dangle */
/* global console */

// var require = function(id) { return Script.require(id + '?'+new Date().getTime().toString(36)); }
module.exports = DopplegangerAttachments;

DopplegangerAttachments.version = '0.0.1b';
DopplegangerAttachments.WANT_DEBUG = false;

var _modelHelper = __webpack_require__(1),
    modelHelper = _modelHelper.modelHelper,
    ModelReadyWatcher = _modelHelper.ModelReadyWatcher,
    utils = __webpack_require__(0);

function log() {
    // eslint-disable-next-line no-console
    (typeof console === 'object' ? console.log : print)('doppleganger-attachments | ' + [].slice.call(arguments).join(' '));
}
log(DopplegangerAttachments.version);

function debugPrint() {
    DopplegangerAttachments.WANT_DEBUG && log.apply(this, arguments);
}

function DopplegangerAttachments(doppleganger, options) {
    utils.assign(this, {
        _options: options,
        doppleganger: doppleganger,
        attachments: undefined,
        manualJointSync: true,
        attachmentsUpdated: utils.signal(function attachmentsUpdated(currentAttachments, previousAttachments){}),
    });
    this._initialize();
    debugPrint('DopplegangerAttachments...', JSON.stringify(options));
}
DopplegangerAttachments.prototype = {
    // "hash" the current attachments (so that changes can be detected)
    getAttachmentsHash: function() {
        return JSON.stringify(this.doppleganger.avatar.getAttachmentsVariant());
    },
    // create a pure Object copy of the current native attachments variant
    _cloneAttachmentsVariant: function() {
        return JSON.parse(JSON.stringify(this.doppleganger.avatar.getAttachmentsVariant()));
    },
    // fetch and resolve attachments to include jointIndex and other relevant $metadata
    _getResolvedAttachments: function() {
        var attachments = this._cloneAttachmentsVariant(),
            objectID = this.doppleganger.objectID;
        function toString() {
            return '[attachment #' + this.$index + ' ' + this.$path + ' @ ' + this.jointName + '{' + this.$jointIndex + '}]';
        }
        return attachments.map(function(attachment, i) {
            var jointIndex = modelHelper.getJointIndex(objectID, attachment.jointName),
                path = (attachment.modelUrl+'').split(/[?#]/)[0].split('/').slice(-3).join('/');
            return Object.defineProperties(attachment, {
                $hash: { value: JSON.stringify(attachment) },
                $index: { value: i },
                $jointIndex: { value: jointIndex },
                $path: { value: path },
                toString: { value: toString },
            });
        });
    },
    // compare before / after attachment sets to determine which ones need to be (re)created
    refreshAttachments: function() {
        if (!this.doppleganger.objectID) {
            return log('refreshAttachments -- canceling; !this.doppleganger.objectID');
        }
        var before = this.attachments || [],
            beforeIndex = before.reduce(function(out, att, index) {
                out[att.$hash] = index; return out;
            }, {});
        var after = this._getResolvedAttachments(),
            afterIndex = after.reduce(function(out, att, index) {
                out[att.$hash] = index; return out;
            }, {});

        Object.keys(beforeIndex).concat(Object.keys(afterIndex)).forEach(function(hash) {
            if (hash in beforeIndex && hash in afterIndex) {
                // print('reusing previous attachment', hash);
                after[afterIndex[hash]] = before[beforeIndex[hash]];
            } else if (!(hash in afterIndex)) {
                var attachment = before[beforeIndex[hash]];
                attachment.properties && attachment.properties.objectID &&
                    modelHelper.deleteObject(attachment.properties.objectID);
                delete attachment.properties;
            }
        });
        this.attachments = after;
        this._createAttachmentObjects();
        this.attachmentsUpdated(after, before);
    },
    _createAttachmentObjects: function() {
        try {
            var attachments = this.attachments,
                parentID = this.doppleganger.objectID,
                jointNames = this.doppleganger.jointNames,
                properties = modelHelper.getProperties(this.doppleganger.objectID),
                modelType = properties && properties.type;
            utils.assert(modelType === 'model' || modelType === 'Model', 'unrecognized doppleganger modelType:' + modelType);
            debugPrint('DopplegangerAttachments..._createAttachmentObjects', JSON.stringify({
                modelType: modelType,
                attachments: attachments.length,
                parentID: parentID,
                jointNames: jointNames.join(' | '),
            },0,2));
            return attachments.map(utils.bind(this, function(attachment, i) {
                var objectType = modelHelper.type(attachment.properties && attachment.properties.objectID);
                if (objectType === 'overlay') {
                    debugPrint('skipping already-provisioned attachment object', objectType, attachment.properties && attachment.properties.name);
                    return attachment;
                }
                var jointIndex = attachment.$jointIndex, // jointNames.indexOf(attachment.jointName),
                    scale = this.doppleganger.avatar.scale * (attachment.scale||1.0);

                attachment.properties = utils.assign({
                    name: attachment.toString(),
                    type: modelType,
                    modelURL: attachment.modelUrl,
                    scale: scale,
                    dimensions: modelHelper.type(parentID) === 'entity' ?
                        Vec3.multiply(attachment.scale||1.0, Vec3.ONE) : undefined,
                    visible: false,
                    collisionless: true,
                    dynamic: false,
                    shapeType: 'none',
                    lifetime: 60,
                }, !this.manualJointSync && {
                    parentID: parentID,
                    parentJointIndex: jointIndex,
                    localPosition: attachment.translation,
                    localRotation: Quat.fromVec3Degrees(attachment.rotation),
                });
                var objectID = attachment.properties.objectID = modelHelper.addObject(attachment.properties);
                utils.assert(!Uuid.isNull(objectID), 'could not create attachment: ' + [objectID, JSON.stringify(attachment.properties,0,2)]);
                attachment._resource = ModelCache.prefetch(attachment.properties.modelURL);
                attachment._modelReadier = new ModelReadyWatcher({
                    resource: attachment._resource,
                    objectID: objectID,
                });
                this.doppleganger.activeChanged.connect(attachment._modelReadier, '_stop');

                attachment._modelReadier.modelReady.connect(this, function(err, result) {
                    if (err) {
                        log('>>>>> modelReady ERROR <<<<<: ' + err, attachment.properties.modelURL);
                        modelHelper.deleteObject(objectID);
                        return objectID = null;
                    }
                    debugPrint('attachment model ('+modelHelper.type(result.objectID)+') is ready; # joints ==',
                        result.jointNames && result.jointNames.length, JSON.stringify(result.naturalDimensions), result.objectID);
                    var properties = modelHelper.getProperties(result.objectID),
                        naturalDimensions = attachment.properties.naturalDimensions = properties.naturalDimensions || result.naturalDimensions;
                    modelHelper.editObject(objectID, {
                        dimensions: naturalDimensions ? Vec3.multiply(attachment.scale, naturalDimensions) : undefined,
                        localRotation: Quat.normalize({}),
                        localPosition: Vec3.ZERO,
                    });
                    this.onJointsUpdated(parentID); // trigger once to initialize position/rotation
                    // give time for model overlay to "settle", then make it visible
                    Script.setTimeout(utils.bind(this, function() {
                        modelHelper.editObject(objectID, {
                            visible: true,
                        });
                        attachment._loaded = true;
                    }), 100);
                });
                return attachment;
            }));
        } catch (e) {
            log('_createAttachmentObjects ERROR:', e.stack || e, e.fileName, e.lineNumber);
        }
    },

    _removeAttachmentObjects: function() {
        if (this.attachments) {
            this.attachments.forEach(function(attachment) {
                if (attachment.properties) {
                    if (attachment.properties.objectID) {
                        modelHelper.deleteObject(attachment.properties.objectID);
                    }
                    delete attachment.properties.objectID;
                }
            });
            delete this.attachments;
        }
    },

    onJointsUpdated: function onJointsUpdated(objectID, jointUpdates) {
        var jointOrientations = modelHelper.getJointOrientations(objectID),
            jointPositions = modelHelper.getJointPositions(objectID),
            parentID = objectID,
            avatarScale = this.doppleganger.scale,
            manualJointSync = this.manualJointSync;

        if (!this.attachments) {
            this.refreshAttachments();
        }
        var updatedObjects = this.attachments.reduce(function(updates, attachment, i) {
            if (!attachment.properties || !attachment._loaded) {
                return updates;
            }
            var objectID = attachment.properties.objectID,
                jointIndex = attachment.$jointIndex,
                jointOrientation = jointOrientations[jointIndex],
                jointPosition = jointPositions[jointIndex];

            var translation = Vec3.multiply(avatarScale, attachment.translation),
                rotation = Quat.fromVec3Degrees(attachment.rotation);

            var localPosition = Vec3.multiplyQbyV(jointOrientation, translation),
                localRotation = rotation;

            updates[objectID] = manualJointSync ? {
                visible: true,
                position: Vec3.sum(jointPosition, localPosition),
                rotation: Quat.multiply(jointOrientation, localRotation),
                scale: avatarScale * attachment.scale,
            } : {
                visible: true,
                parentID: parentID,
                parentJointIndex: jointIndex,
                localRotation: localRotation,
                localPosition: localPosition,
                scale: attachment.scale,
            };
            return updates;
        }, {});
        modelHelper.editObjects(updatedObjects);
    },

    _initialize: function() {
        var doppleganger = this.doppleganger;
        if ('$attachmentControls' in doppleganger) {
            throw new Error('only one set of attachment controls can be added per doppleganger');
        }
        doppleganger.$attachmentControls = this;
        doppleganger.activeChanged.connect(this, function(active) {
            if (active) {
                doppleganger.jointsUpdated.connect(this, 'onJointsUpdated');
            } else {
                doppleganger.jointsUpdated.disconnect(this, 'onJointsUpdated');
                this._removeAttachmentObjects();
            }
        });

        Script.scriptEnding.connect(this, '_removeAttachmentObjects');
    },
};


/***/ }),
/* 5 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";
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


/* eslint-env commonjs */
/* eslint-disable comma-dangle */
/* global console */

var DopplegangerClass = __webpack_require__(2),
    modelHelper = __webpack_require__(1).modelHelper,
    utils = __webpack_require__(0);

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


/***/ }),
/* 6 */
/***/ (function(module, exports) {

module.exports = "data:image/svg+xml;xml,<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n<!-- Generator: Adobe Illustrator 19.2.0, SVG Export Plug-In . SVG Version: 6.00 Build 0)  -->\n\n<svg\n   xmlns:osb=\"http://www.openswatchbook.org/uri/2009/osb\"\n   xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n   xmlns:cc=\"http://creativecommons.org/ns#\"\n   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n   xmlns:svg=\"http://www.w3.org/2000/svg\"\n   xmlns=\"http://www.w3.org/2000/svg\"\n   xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n   xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n   version=\"1.1\"\n   x=\"0px\"\n   y=\"0px\"\n   viewBox=\"0 0 50 50\"\n   style=\"enable-background:new 0 0 50 50;\"\n   xml:space=\"preserve\"\n   id=\"svg2\"\n   inkscape:version=\"0.91 r13725\"\n   sodipodi:docname=\"doppleganger-i.svg\"><metadata\n     id=\"metadata36\"><rdf:RDF><cc:Work\n         rdf:about=\"\"><dc:format>image/svg+xml</dc:format><dc:type\n           rdf:resource=\"http://purl.org/dc/dcmitype/StillImage\" /><dc:title></dc:title></cc:Work></rdf:RDF></metadata><defs\n     id=\"defs34\"><linearGradient\n       id=\"linearGradient8353\"\n       osb:paint=\"solid\"><stop\n         style=\"stop-color:#000000;stop-opacity:1;\"\n         offset=\"0\"\n         id=\"stop8355\" /></linearGradient></defs><sodipodi:namedview\n     pagecolor=\"#ff4900\"\n     bordercolor=\"#666666\"\n     borderopacity=\"1\"\n     objecttolerance=\"10\"\n     gridtolerance=\"10\"\n     guidetolerance=\"10\"\n     inkscape:pageopacity=\"0\"\n     inkscape:pageshadow=\"2\"\n     inkscape:window-width=\"1920\"\n     inkscape:window-height=\"1004\"\n     id=\"namedview32\"\n     showgrid=\"false\"\n     inkscape:zoom=\"9.44\"\n     inkscape:cx=\"-3.2806499\"\n     inkscape:cy=\"20.640561\"\n     inkscape:window-x=\"0\"\n     inkscape:window-y=\"24\"\n     inkscape:window-maximized=\"1\"\n     inkscape:current-layer=\"g4308\" /><style\n     type=\"text/css\"\n     id=\"style4\">\n\t.st0{fill:#FFFFFF;}\n</style><g\n     id=\"Layer_2\" /><g\n     id=\"Layer_1\"\n     style=\"fill:#000000;fill-opacity:1\"><g\n       id=\"g8375\"\n       transform=\"matrix(1.0667546,0,0,1.0667546,-2.1894733,-1.7818707)\"><g\n         id=\"g4308\"\n         transform=\"translate(1.0333645e-6,0)\"><g\n           id=\"g8\"\n           style=\"fill:#ffffff;fill-opacity:1\"\n           transform=\"matrix(1.1059001,0,0,1.1059001,-17.342989,-7.9561147)\"><path\n             class=\"st0\"\n             d=\"m 23.2,24.1 c -0.8,0.9 -1.5,1.8 -2.2,2.6 -0.1,0.2 -0.1,0.5 -0.1,0.7 0.1,1.7 0.2,3.4 0.2,5.1 0,0.8 -0.4,1.2 -1.1,1.3 -0.7,0.1 -1.3,-0.4 -1.4,-1.1 -0.2,-2.2 -0.3,-4.3 -0.5,-6.5 0,-0.3 0.1,-0.7 0.4,-1 1.1,-1.5 2.3,-3 3.4,-4.5 0.6,-0.7 1.6,-1.6 2.6,-1.6 0.3,0 1.1,0 1.4,0 0.8,-0.1 1.3,0.1 1.9,0.9 1,1.2 1.5,2.3 2.4,3.6 0.7,1.1 1.4,1.6 2.9,1.9 1.1,0.2 2.2,0.5 3.3,0.8 0.3,0.1 0.6,0.2 0.8,0.3 0.5,0.3 0.7,0.8 0.6,1.3 -0.1,0.5 -0.5,0.7 -1,0.8 -0.4,0 -0.9,0 -1.3,-0.1 -1.4,-0.3 -2.7,-0.6 -4.1,-0.9 -0.8,-0.2 -1.5,-0.6 -2.1,-1.1 -0.3,-0.3 -0.6,-0.5 -0.9,-0.8 0,0.3 0,0.5 0,0.7 0,1.2 0,2.4 0,3.6 0,0.4 -0.3,12.6 -0.1,16.8 0,0.5 -0.1,1 -0.2,1.5 -0.2,0.7 -0.6,1 -1.4,1.1 -0.8,0 -1.4,-0.3 -1.7,-1 C 24.8,48 24.7,47.4 24.6,46.9 24.2,42.3 23.7,34 23.5,33.1 23.4,32.3 23.3,32 23.2,31 c -0.1,-0.5 -0.1,-0.9 -0.1,-1.3 0.2,-1.8 0.1,-3.6 0.1,-5.6 z\"\n             id=\"path10\"\n             style=\"fill:#ffffff;fill-opacity:1\"\n             inkscape:connector-curvature=\"0\" /><path\n             class=\"st0\"\n             d=\"m 28.2,14.6 c 0,1.4 -1.1,2.6 -2.6,2.6 l 0,0 C 24.2,17.2 23,16.1 23,14.6 L 23,13 c 0,-1.4 1.1,-2.6 2.6,-2.6 l 0,0 c 1.4,0 2.6,1.1 2.6,2.6 l 0,1.6 z\"\n             id=\"path12\"\n             style=\"fill:#ffffff;fill-opacity:1\"\n             inkscape:connector-curvature=\"0\" /></g><g\n           id=\"g8-3\"\n           style=\"opacity:0.5;fill:#808080;fill-opacity:1;stroke:#ffffff;stroke-width:0.59335912;stroke-linecap:butt;stroke-miterlimit:4;stroke-dasharray:0.29667956, 0.29667956000000001;stroke-dashoffset:0;stroke-opacity:1\"\n           transform=\"matrix(-1.1059001,0,0,1.1059001,67.821392,-7.9561147)\"><path\n             class=\"st0\"\n             d=\"m 23.2,24.1 c -0.8,0.9 -1.5,1.8 -2.2,2.6 -0.1,0.2 -0.1,0.5 -0.1,0.7 0.1,1.7 0.2,3.4 0.2,5.1 0,0.8 -0.4,1.2 -1.1,1.3 -0.7,0.1 -1.3,-0.4 -1.4,-1.1 -0.2,-2.2 -0.3,-4.3 -0.5,-6.5 0,-0.3 0.1,-0.7 0.4,-1 1.1,-1.5 2.3,-3 3.4,-4.5 0.6,-0.7 1.6,-1.6 2.6,-1.6 0.3,0 1.1,0 1.4,0 0.8,-0.1 1.3,0.1 1.9,0.9 1,1.2 1.5,2.3 2.4,3.6 0.7,1.1 1.4,1.6 2.9,1.9 1.1,0.2 2.2,0.5 3.3,0.8 0.3,0.1 0.6,0.2 0.8,0.3 0.5,0.3 0.7,0.8 0.6,1.3 -0.1,0.5 -0.5,0.7 -1,0.8 -0.4,0 -0.9,0 -1.3,-0.1 -1.4,-0.3 -2.7,-0.6 -4.1,-0.9 -0.8,-0.2 -1.5,-0.6 -2.1,-1.1 -0.3,-0.3 -0.6,-0.5 -0.9,-0.8 0,0.3 0,0.5 0,0.7 0,1.2 0,2.4 0,3.6 0,0.4 -0.3,12.6 -0.1,16.8 0,0.5 -0.1,1 -0.2,1.5 -0.2,0.7 -0.6,1 -1.4,1.1 -0.8,0 -1.4,-0.3 -1.7,-1 C 24.8,48 24.7,47.4 24.6,46.9 24.2,42.3 23.7,34 23.5,33.1 23.4,32.3 23.3,32 23.2,31 c -0.1,-0.5 -0.1,-0.9 -0.1,-1.3 0.2,-1.8 0.1,-3.6 0.1,-5.6 z\"\n             id=\"path10-6\"\n             style=\"fill:#808080;fill-opacity:1;stroke:#ffffff;stroke-width:0.59335912;stroke-linecap:butt;stroke-miterlimit:4;stroke-dasharray:0.29667956, 0.29667956000000001;stroke-dashoffset:0;stroke-opacity:1\"\n             inkscape:connector-curvature=\"0\" /><path\n             class=\"st0\"\n             d=\"m 28.2,14.6 c 0,1.4 -1.1,2.6 -2.6,2.6 l 0,0 C 24.2,17.2 23,16.1 23,14.6 L 23,13 c 0,-1.4 1.1,-2.6 2.6,-2.6 l 0,0 c 1.4,0 2.6,1.1 2.6,2.6 l 0,1.6 z\"\n             id=\"path12-7\"\n             style=\"fill:#808080;fill-opacity:1;stroke:#ffffff;stroke-width:0.59335912;stroke-linecap:butt;stroke-miterlimit:4;stroke-dasharray:0.29667956, 0.29667956000000001;stroke-dashoffset:0;stroke-opacity:1\"\n             inkscape:connector-curvature=\"0\" /></g></g><rect\n         style=\"opacity:0.5;fill:#ffffff;fill-opacity:1;stroke:#ffffff;stroke-width:0.15729524;stroke-linecap:butt;stroke-miterlimit:4;stroke-dasharray:0.62918094, 1.25836187000000010;stroke-dashoffset:0;stroke-opacity:1\"\n         id=\"rect4306\"\n         width=\"0.12393159\"\n         height=\"46.498554\"\n         x=\"25.227457\"\n         y=\"1.8070068\"\n         rx=\"0\"\n         ry=\"0.9407174\" /></g></g></svg>";

/***/ }),
/* 7 */
/***/ (function(module, exports) {

module.exports = "data:image/svg+xml;xml,<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n<!-- Generator: Adobe Illustrator 19.2.0, SVG Export Plug-In . SVG Version: 6.00 Build 0)  -->\n\n<svg\n   xmlns:osb=\"http://www.openswatchbook.org/uri/2009/osb\"\n   xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n   xmlns:cc=\"http://creativecommons.org/ns#\"\n   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n   xmlns:svg=\"http://www.w3.org/2000/svg\"\n   xmlns=\"http://www.w3.org/2000/svg\"\n   xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n   xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n   version=\"1.1\"\n   x=\"0px\"\n   y=\"0px\"\n   viewBox=\"0 0 50 50\"\n   style=\"enable-background:new 0 0 50 50;\"\n   xml:space=\"preserve\"\n   id=\"svg2\"\n   inkscape:version=\"0.91 r13725\"\n   sodipodi:docname=\"doppleganger-a.svg\"><metadata\n     id=\"metadata36\"><rdf:RDF><cc:Work\n         rdf:about=\"\"><dc:format>image/svg+xml</dc:format><dc:type\n           rdf:resource=\"http://purl.org/dc/dcmitype/StillImage\" /><dc:title /></cc:Work></rdf:RDF></metadata><defs\n     id=\"defs34\"><linearGradient\n       id=\"linearGradient8353\"\n       osb:paint=\"solid\"><stop\n         style=\"stop-color:#000000;stop-opacity:1;\"\n         offset=\"0\"\n         id=\"stop8355\" /></linearGradient></defs><sodipodi:namedview\n     pagecolor=\"#ff4900\"\n     bordercolor=\"#666666\"\n     borderopacity=\"1\"\n     objecttolerance=\"10\"\n     gridtolerance=\"10\"\n     guidetolerance=\"10\"\n     inkscape:pageopacity=\"0\"\n     inkscape:pageshadow=\"2\"\n     inkscape:window-width=\"1920\"\n     inkscape:window-height=\"1004\"\n     id=\"namedview32\"\n     showgrid=\"false\"\n     inkscape:zoom=\"9.44\"\n     inkscape:cx=\"-3.2806499\"\n     inkscape:cy=\"20.640561\"\n     inkscape:window-x=\"0\"\n     inkscape:window-y=\"24\"\n     inkscape:window-maximized=\"1\"\n     inkscape:current-layer=\"g4308\" /><style\n     type=\"text/css\"\n     id=\"style4\">\n\t.st0{fill:#FFFFFF;}\n</style><g\n     id=\"Layer_2\" /><g\n     id=\"Layer_1\"\n     style=\"fill:#000000;fill-opacity:1\"><g\n       id=\"g8375\"\n       transform=\"matrix(1.0667546,0,0,1.0667546,-2.1894733,-1.7818707)\"><g\n         id=\"g4308\"\n         transform=\"translate(1.0333645e-6,0)\"><g\n           id=\"g8\"\n           style=\"fill:#000000;fill-opacity:1\"\n           transform=\"matrix(1.1059001,0,0,1.1059001,-17.342989,-7.9561147)\"><path\n             class=\"st0\"\n             d=\"m 23.2,24.1 c -0.8,0.9 -1.5,1.8 -2.2,2.6 -0.1,0.2 -0.1,0.5 -0.1,0.7 0.1,1.7 0.2,3.4 0.2,5.1 0,0.8 -0.4,1.2 -1.1,1.3 -0.7,0.1 -1.3,-0.4 -1.4,-1.1 -0.2,-2.2 -0.3,-4.3 -0.5,-6.5 0,-0.3 0.1,-0.7 0.4,-1 1.1,-1.5 2.3,-3 3.4,-4.5 0.6,-0.7 1.6,-1.6 2.6,-1.6 0.3,0 1.1,0 1.4,0 0.8,-0.1 1.3,0.1 1.9,0.9 1,1.2 1.5,2.3 2.4,3.6 0.7,1.1 1.4,1.6 2.9,1.9 1.1,0.2 2.2,0.5 3.3,0.8 0.3,0.1 0.6,0.2 0.8,0.3 0.5,0.3 0.7,0.8 0.6,1.3 -0.1,0.5 -0.5,0.7 -1,0.8 -0.4,0 -0.9,0 -1.3,-0.1 -1.4,-0.3 -2.7,-0.6 -4.1,-0.9 -0.8,-0.2 -1.5,-0.6 -2.1,-1.1 -0.3,-0.3 -0.6,-0.5 -0.9,-0.8 0,0.3 0,0.5 0,0.7 0,1.2 0,2.4 0,3.6 0,0.4 -0.3,12.6 -0.1,16.8 0,0.5 -0.1,1 -0.2,1.5 -0.2,0.7 -0.6,1 -1.4,1.1 -0.8,0 -1.4,-0.3 -1.7,-1 C 24.8,48 24.7,47.4 24.6,46.9 24.2,42.3 23.7,34 23.5,33.1 23.4,32.3 23.3,32 23.2,31 c -0.1,-0.5 -0.1,-0.9 -0.1,-1.3 0.2,-1.8 0.1,-3.6 0.1,-5.6 z\"\n             id=\"path10\"\n             style=\"fill:#000000;fill-opacity:1\"\n             inkscape:connector-curvature=\"0\" /><path\n             class=\"st0\"\n             d=\"m 28.2,14.6 c 0,1.4 -1.1,2.6 -2.6,2.6 l 0,0 C 24.2,17.2 23,16.1 23,14.6 L 23,13 c 0,-1.4 1.1,-2.6 2.6,-2.6 l 0,0 c 1.4,0 2.6,1.1 2.6,2.6 l 0,1.6 z\"\n             id=\"path12\"\n             style=\"fill:#000000;fill-opacity:1\"\n             inkscape:connector-curvature=\"0\" /></g><g\n           id=\"g8-3\"\n           style=\"opacity:0.5;fill:#808080;fill-opacity:1;stroke:#000000;stroke-width:0.59335912;stroke-linecap:butt;stroke-miterlimit:4;stroke-dasharray:0.29667956,0.29667956;stroke-dashoffset:0;stroke-opacity:1\"\n           transform=\"matrix(-1.1059001,0,0,1.1059001,67.821392,-7.9561147)\"><path\n             class=\"st0\"\n             d=\"m 23.2,24.1 c -0.8,0.9 -1.5,1.8 -2.2,2.6 -0.1,0.2 -0.1,0.5 -0.1,0.7 0.1,1.7 0.2,3.4 0.2,5.1 0,0.8 -0.4,1.2 -1.1,1.3 -0.7,0.1 -1.3,-0.4 -1.4,-1.1 -0.2,-2.2 -0.3,-4.3 -0.5,-6.5 0,-0.3 0.1,-0.7 0.4,-1 1.1,-1.5 2.3,-3 3.4,-4.5 0.6,-0.7 1.6,-1.6 2.6,-1.6 0.3,0 1.1,0 1.4,0 0.8,-0.1 1.3,0.1 1.9,0.9 1,1.2 1.5,2.3 2.4,3.6 0.7,1.1 1.4,1.6 2.9,1.9 1.1,0.2 2.2,0.5 3.3,0.8 0.3,0.1 0.6,0.2 0.8,0.3 0.5,0.3 0.7,0.8 0.6,1.3 -0.1,0.5 -0.5,0.7 -1,0.8 -0.4,0 -0.9,0 -1.3,-0.1 -1.4,-0.3 -2.7,-0.6 -4.1,-0.9 -0.8,-0.2 -1.5,-0.6 -2.1,-1.1 -0.3,-0.3 -0.6,-0.5 -0.9,-0.8 0,0.3 0,0.5 0,0.7 0,1.2 0,2.4 0,3.6 0,0.4 -0.3,12.6 -0.1,16.8 0,0.5 -0.1,1 -0.2,1.5 -0.2,0.7 -0.6,1 -1.4,1.1 -0.8,0 -1.4,-0.3 -1.7,-1 C 24.8,48 24.7,47.4 24.6,46.9 24.2,42.3 23.7,34 23.5,33.1 23.4,32.3 23.3,32 23.2,31 c -0.1,-0.5 -0.1,-0.9 -0.1,-1.3 0.2,-1.8 0.1,-3.6 0.1,-5.6 z\"\n             id=\"path10-6\"\n             style=\"fill:#808080;fill-opacity:1;stroke:#000000;stroke-width:0.59335912;stroke-linecap:butt;stroke-miterlimit:4;stroke-dasharray:0.29667956,0.29667956;stroke-dashoffset:0;stroke-opacity:1\"\n             inkscape:connector-curvature=\"0\" /><path\n             class=\"st0\"\n             d=\"m 28.2,14.6 c 0,1.4 -1.1,2.6 -2.6,2.6 l 0,0 C 24.2,17.2 23,16.1 23,14.6 L 23,13 c 0,-1.4 1.1,-2.6 2.6,-2.6 l 0,0 c 1.4,0 2.6,1.1 2.6,2.6 l 0,1.6 z\"\n             id=\"path12-7\"\n             style=\"fill:#808080;fill-opacity:1;stroke:#000000;stroke-width:0.59335912;stroke-linecap:butt;stroke-miterlimit:4;stroke-dasharray:0.29667956,0.29667956;stroke-dashoffset:0;stroke-opacity:1\"\n             inkscape:connector-curvature=\"0\" /></g></g><rect\n         style=\"opacity:0.5;fill:#808080;fill-opacity:1;stroke:#000000;stroke-width:0.15729524;stroke-linecap:butt;stroke-miterlimit:4;stroke-dasharray:0.62918094, 1.25836187;stroke-dashoffset:0;stroke-opacity:1\"\n         id=\"rect4306\"\n         width=\"0.12393159\"\n         height=\"46.498554\"\n         x=\"25.227457\"\n         y=\"1.8070068\"\n         rx=\"0\"\n         ry=\"0.9407174\" /></g></g></svg>";

/***/ })
/******/ ]);