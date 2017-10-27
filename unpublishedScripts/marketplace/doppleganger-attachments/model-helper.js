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

var utils = require('./utils.js'),
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
