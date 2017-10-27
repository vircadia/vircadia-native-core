// movement-utils.js -- helper classes for managing related Controller.*Event and input API bindings

/* eslint-disable comma-dangle, no-empty */
/* global require: true, DriveKeys, console, __filename, __dirname */
/* eslint-env commonjs */
"use strict";

module.exports = {
    version: '0.0.2c',

    CameraControls: CameraControls,
    MovementEventMapper: MovementEventMapper,
    MovementMapping: MovementMapping,
    VelocityTracker: VelocityTracker,
    VirtualDriveKeys: VirtualDriveKeys,

    applyEasing: applyEasing,
    calculateThrust: calculateThrust,
    vec3damp: vec3damp,
    vec3eclamp: vec3eclamp,

    DriveModes: {
        POSITION: 'position', // ~ MyAvatar.position
        MOTOR: 'motor',       // ~ MyAvatar.motorVelocity
        THRUST: 'thrust',     // ~ MyAvatar.setThrust
    },
};

var MAPPING_TEMPLATE = require('./movement-utils.mapping.json');
var WANT_DEBUG = false;

function log() {
    // eslint-disable-next-line no-console
    (typeof Script === 'object' ? print : console.log)('movement-utils | ' + [].slice.call(arguments).join(' '));
}

var debugPrint = function() {};

log(module.exports.version);

var _utils = require('./_utils.js'),
    assert = _utils.assert;

if (WANT_DEBUG) {
    require = _utils.makeDebugRequire(__dirname);
    _utils = require('./_utils.js'); // re-require in debug mode
    debugPrint = log;
}

Object.assign = Object.assign || _utils.assign;

var enumMeta = require('./EnumMeta.js');
assert(enumMeta.version >= '0.0.1', 'enumMeta >= 0.0.1 expected but got: ' + enumMeta.version);

Object.assign(MovementEventMapper, {
    CAPTURE_DRIVE_KEYS: 'drive-keys',
    CAPTURE_ACTION_EVENTS: 'action-events',
});

function MovementEventMapper(options) {
    assert('namespace' in options, '.namespace expected ' + Object.keys(options) );
    this.namespace = options.namespace;
    this.enabled = false;

    this.options = Object.assign({
        namespace: this.namespace,
        captureMode: MovementEventMapper.CAPTURE_ACTION_EVENTS,
        excludeNames: null,
        mouseSmooth: true,
        keyboardMultiplier: 1.0,
        mouseMultiplier: 1.0,
        eventFilter: null,
        controllerMapping: MAPPING_TEMPLATE,
    }, options);

    this.isShifted = false;
    this.isGrounded = false;
    this.isRightMouseButton = false;
    this.rightMouseButtonReleased = undefined;

    this.inputMapping = new MovementMapping(this.options);
    this.inputMapping.virtualActionEvent.connect(this, 'onVirtualActionEvent');
}
MovementEventMapper.prototype = {
    constructor: MovementEventMapper,
    defaultEventFilter: function(from, event) {
        return event.actionValue;
    },
    getState: function(options) {
        var state = this.states ? this.states.getDriveKeys(options) : {};

        state.enabled = this.enabled;

        state.mouseSmooth = this.options.mouseSmooth;
        state.captureMode = this.options.captureMode;
        state.mouseMultiplier = this.options.mouseMultiplier;
        state.keyboardMultiplier = this.options.keyboardMultiplier;

        state.isGrounded = this.isGrounded;
        state.isShifted = this.isShifted;
        state.isRightMouseButton = this.isRightMouseButton;
        state.rightMouseButtonReleased = this.rightMouseButtonReleased;

        return state;
    },
    updateOptions: function(options) {
        return _updateOptions(this.options, options || {}, this.constructor.name);
    },
    applyOptions: function(options, applyNow) {
        if (this.updateOptions(options) && applyNow) {
            this.reset();
        }
    },
    reset: function() {
        if (this.enabled) {
            this.disable();
            this.enable();
        }
    },
    disable: function() {
        this.inputMapping.disable();
        this.bindEvents(false);
        this.enabled = false;
    },
    enable: function() {
        if (!this.enabled) {
            this.enabled = true;
            this.states = new VirtualDriveKeys({
                eventFilter: this.options.eventFilter && _utils.bind(this, this.options.eventFilter)
            });
            this.bindEvents(true);
            this.inputMapping.updateOptions(this.options);
            this.inputMapping.enable();
        }
    },
    bindEvents: function bindEvents(capture) {
        var captureMode = this.options.captureMode;
        assert(function assertion() {
            return captureMode === MovementEventMapper.CAPTURE_ACTION_EVENTS ||
                captureMode === MovementEventMapper.CAPTURE_DRIVE_KEYS;
        });
        log('bindEvents....', capture, this.options.captureMode);
        var exclude = Array.isArray(this.options.excludeNames) && this.options.excludeNames;

        var tmp;
        if (!capture || this.options.captureMode === MovementEventMapper.CAPTURE_ACTION_EVENTS) {
            tmp = capture ? 'captureActionEvents' : 'releaseActionEvents';
            log('bindEvents -- ', tmp.toUpperCase());
            Controller[tmp]();
        }
        if (!capture || this.options.captureMode === MovementEventMapper.CAPTURE_DRIVE_KEYS) {
            tmp = capture ? 'disableDriveKey' : 'enableDriveKey';
            log('bindEvents -- ', tmp.toUpperCase());
            for (var p in DriveKeys) {
                if (capture && (exclude && ~exclude.indexOf(p))) {
                    log(tmp.toUpperCase(), 'excluding DriveKey===' + p);
                } else {
                    MyAvatar[tmp](DriveKeys[p]);
                }
            }
        }
        try {
            Controller.actionEvent[capture ? 'connect' : 'disconnect'](this, 'onActionEvent');
        } catch (e) { }

        if (!capture || !/person/i.test(Camera.mode)) {
            Controller[capture ? 'captureWheelEvents' : 'releaseWheelEvents']();
            try {
                Controller.wheelEvent[capture ? 'connect' : 'disconnect'](this, 'onWheelEvent');
            } catch (e) { /* eslint-disable-line empty-block */ }
        }
    },
    onWheelEvent: function onWheelEvent(event) {
        var actionID = enumMeta.ACTION_TRANSLATE_CAMERA_Z,
            actionValue = -event.delta;
        return this.onActionEvent(actionID, actionValue, event);
    },
    onActionEvent: function(actionID, actionValue, extra) {
        var actionName = enumMeta.Controller.ActionNames[actionID],
            driveKeyName = enumMeta.getDriveKeyNameFromActionName(actionName),
            prefix = (actionValue > 0 ? '+' : actionValue < 0 ? '-' : ' ');

        var event = {
            id: prefix + actionName,
            actionName: actionName,
            driveKey: DriveKeys[driveKeyName],
            driveKeyName: driveKeyName,
            actionValue: actionValue,
            extra: extra
        };
        // debugPrint('onActionEvent', actionID, actionName, driveKeyName);
        this.states.handleActionEvent('Actions.' + actionName, event);
    },
    onVirtualActionEvent: function(from, event) {
        if (from === 'Application.Grounded') {
            this.isGrounded = !!event.applicationValue;
        } else if (from === 'Keyboard.Shift') {
            this.isShifted = !!event.value;
        } else if (from === 'Keyboard.RightMouseButton') {
            this.isRightMouseButton = !!event.value;
            this.rightMouseButtonReleased = !event.value ? new Date : undefined;
        }
        this.states.handleActionEvent(from, event);
    }
}; // MovementEventMapper.prototype

// ----------------------------------------------------------------------------
// helper JS class to track drive keys -> translation / rotation influences
function VirtualDriveKeys(options) {
    options = options || {};
    Object.defineProperties(this, {
        $pendingReset: { value: {} },
        $eventFilter: { value: options.eventFilter },
        $valueUpdated: { value: _utils.signal(function valueUpdated(action, newValue, oldValue){}) }
    });
}
VirtualDriveKeys.prototype = {
    constructor: VirtualDriveKeys,
    update: function update(dt) {
        Object.keys(this.$pendingReset).forEach(_utils.bind(this, function(i) {
            var event = this.$pendingReset[i].event;
            (event.driveKey in this) && this.setValue(event, 0);
        }));
    },
    getValue: function(driveKey, defaultValue) {
        return driveKey in this ? this[driveKey] : defaultValue;
    },
    _defaultFilter: function(from, event) {
        return event.actionValue;
    },
    handleActionEvent: function(from, event) {
        var value = this.$eventFilter ? this.$eventFilter(from, event, this._defaultFilter) : event.actionValue;
        return event.driveKeyName && this.setValue(event, value);
    },
    setValue: function(event, value) {
        var driveKeyName = event.driveKeyName,
            driveKey = DriveKeys[driveKeyName],
            id = event.id,
            previous = this[driveKey],
            autoReset = (driveKeyName === 'ZOOM');

        this[driveKey] = value;

        if (previous !== value) {
            this.$valueUpdated(event, value, previous);
        }
        if (value === 0.0) {
            delete this.$pendingReset[id];
        } else if (autoReset) {
            this.$pendingReset[id] = { event: event, value: value };
        }
    },
    reset: function() {
        Object.keys(this).forEach(_utils.bind(this, function(p) {
            this[p] = 0.0;
        }));
        Object.keys(this.$pendingReset).forEach(_utils.bind(this, function(p) {
            delete this.$pendingReset[p];
        }));
    },
    toJSON: function() {
        var obj = {};
        for (var key in this) {
            if (enumMeta.DriveKeyNames[key]) {
                obj[enumMeta.DriveKeyNames[key]] = this[key];
            }
        }
        return obj;
    },
    getDriveKeys: function(options) {
        options = options || {};
        try {
            return {
                translation: {
                    x: this.getValue(DriveKeys.TRANSLATE_X) || 0,
                    y: this.getValue(DriveKeys.TRANSLATE_Y) || 0,
                    z: this.getValue(DriveKeys.TRANSLATE_Z) || 0
                },
                rotation: {
                    x: this.getValue(DriveKeys.PITCH) || 0,
                    y: this.getValue(DriveKeys.YAW) || 0,
                    z: 'ROLL' in DriveKeys && this.getValue(DriveKeys.ROLL) || 0
                },
                zoom: Vec3.multiply(this.getValue(DriveKeys.ZOOM) || 0, Vec3.ONE)
            };
        } finally {
            options.update && this.update(options.update);
        }
    }
};

// ----------------------------------------------------------------------------
// MovementMapping

function MovementMapping(options) {
    options = options || {};
    assert('namespace' in options && 'controllerMapping' in options);
    this.namespace = options.namespace;
    this.enabled = false;
    this.options = {
        keyboardMultiplier: 1.0,
        mouseMultiplier: 1.0,
        mouseSmooth: true,
        captureMode: MovementEventMapper.CAPTURE_ACTION_EVENTS,
        excludeNames: null,
        controllerMapping: MAPPING_TEMPLATE,
    };
    this.updateOptions(options);
    this.virtualActionEvent = _utils.signal(function virtualActionEvent(from, event) {});
}
MovementMapping.prototype = {
    constructor: MovementMapping,
    enable: function() {
        this.enabled = true;
        if (this.mapping) {
            this.mapping.disable();
        }
        this.mapping = this._createMapping();
        log('ENABLE CONTROLLER MAPPING', this.mapping.name);
        this.mapping.enable();
    },
    disable: function() {
        this.enabled = false;
        if (this.mapping) {
            log('DISABLE CONTROLLER MAPPING', this.mapping.name);
            this.mapping.disable();
        }
    },
    reset: function() {
        var enabled = this.enabled;
        enabled && this.disable();
        this.mapping = this._createMapping();
        enabled && this.enable();
    },
    updateOptions: function(options) {
        return _updateOptions(this.options, options || {}, this.constructor.name);
    },
    applyOptions: function(options, applyNow) {
        if (this.updateOptions(options) && applyNow) {
            this.reset();
        }
    },
    onShiftKey: function onShiftKey(value, key) {
        var event = {
            type: value ? 'keypress' : 'keyrelease',
            keyboardKey: key,
            keyboardText: 'SHIFT',
            keyboardValue: value,
            actionName: 'Shift',
            actionValue: !!value,
            value: !!value,
            at: +new Date
        };
        this.virtualActionEvent('Keyboard.Shift', event);
    },
    onRightMouseButton: function onRightMouseButton(value, key) {
        var event = {
            type: value ? 'mousepress' : 'mouserelease',
            keyboardKey: key,
            keyboardValue: value,
            actionName: 'RightMouseButton',
            actionValue: !!value,
            value: !!value,
            at: +new Date
        };
        this.virtualActionEvent('Keyboard.RightMouseButton', event);
    },
    onApplicationEvent: function _onApplicationEvent(key, name, value) {
        var event = {
            type: 'application',
            actionName: 'Application.' + name,
            applicationKey: key,
            applicationName: name,
            applicationValue: value,
            actionValue: !!value,
            value: !!value
        };
        this.virtualActionEvent('Application.' + name, event);
    },
    _createMapping: function() {
        this._mapping = this._getTemplate();
        var mappingJSON = JSON.stringify(this._mapping, 0, 2);
        var mapping = Controller.parseMapping(mappingJSON);
        debugPrint(mappingJSON);
        mapping.name = mapping.name || this._mapping.name;

        mapping.from(Controller.Hardware.Keyboard.Shift).peek().to(_utils.bind(this, 'onShiftKey'));
        mapping.from(Controller.Hardware.Keyboard.RightMouseButton).peek().to(_utils.bind(this, 'onRightMouseButton'));

        var boundApplicationHandler = _utils.bind(this, 'onApplicationEvent');
        Object.keys(Controller.Hardware.Application).forEach(function(name) {
            var key = Controller.Hardware.Application[name];
            debugPrint('observing Controller.Hardware.Application.'+ name, key);
            mapping.from(key).to(function(value) {
                boundApplicationHandler(key, name, value);
            });
        });

        return mapping;
    },
    _getTemplate: function() {
        assert(this.options.controllerMapping, 'MovementMapping._getTemplate -- !this.options.controllerMapping');
        var template = JSON.parse(JSON.stringify(this.options.controllerMapping)); // make a local copy
        template.name = this.namespace;
        template.channels = template.channels.filter(function(item) {
            // ignore any "JSON comment" or other bindings without a from spec
            return item.from && item.from.makeAxis;
        });
        var exclude = Array.isArray(this.options.excludeNames) ? this.options.excludeNames : [];
        if (!this.options.mouseSmooth) {
            exclude.push('Keyboard.RightMouseButton');
        }

        log('EXCLUSIONS:' + exclude);

        template.channels = template.channels.filter(_utils.bind(this, function(item, i) {
            debugPrint('channel['+i+']', item.from && item.from.makeAxis, item.to, JSON.stringify(item.filters) || '');
            // var hasFilters = Array.isArray(item.filters) && !item.filters[1];
            item.filters = Array.isArray(item.filters) ? item.filters :
                typeof item.filters === 'string' ? [ { type: item.filters }] : [ item.filters ];

            if (/Mouse/.test(item.from && item.from.makeAxis)) {
                item.filters.push({ type: 'scale', scale: this.options.mouseMultiplier });
                log('applied mouse multiplier:', item.from.makeAxis, item.when, item.to, this.options.mouseMultiplier);
            } else if (/Keyboard/.test(item.from && item.from.makeAxis)) {
                item.filters.push({ type: 'scale', scale: this.options.keyboardMultiplier });
                log('applied keyboard multiplier:', item.from.makeAxis, item.when, item.to, this.options.keyboardMultiplier);
            }
            item.filters = item.filters.filter(Boolean);
            if (~exclude.indexOf(item.to)) {
                log('EXCLUDING item.to === ' + item.to);
                return false;
            }
            var when = Array.isArray(item.when) ? item.when : [item.when];
            for (var j=0; j < when.length; j++) {
                if (~exclude.indexOf(when[j])) {
                    log('EXCLUDING item.when contains ' + when[j]);
                    return false;
                }
            }
            function shouldInclude(p, i) {
                if (~exclude.indexOf(p)) {
                    log('EXCLUDING from.makeAxis[][' + i + '] === ' + p);
                    return false;
                }
                return true;
            }

            if (item.from && Array.isArray(item.from.makeAxis)) {
                var makeAxis = item.from.makeAxis;
                item.from.makeAxis = makeAxis.map(function(axis) {
                    if (Array.isArray(axis)) {
                        return axis.filter(shouldInclude);
                    } else {
                        return shouldInclude(axis, -1) && axis;
                    }
                }).filter(Boolean);
            }
            return true;
        }));
        debugPrint(JSON.stringify(template,0,2));
        return template;
    }
}; // MovementMapping.prototype

// update target properties from source, but iff the property already exists in target
function _updateOptions(target, source, debugName) {
    debugName = debugName || '_updateOptions';
    var changed = 0;
    if (!source || typeof source !== 'object') {
        return changed;
    }
    for (var p in target) {
        if (p in source && target[p] !== source[p]) {
            log(debugName, 'updating source.'+p, target[p] + ' -> ' + source[p]);
            target[p] = source[p];
            changed++;
        }
    }
    for (p in source) {
        (!(p in target)) && log(debugName, 'warning: ignoring unknown option:', p, (source[p] +'').substr(0, 40)+'...');
    }
    return changed;
}

// ----------------------------------------------------------------------------
function calculateThrust(maxVelocity, targetVelocity, previousThrust) {
    var THRUST_FALLOFF = 0.1; // round to ZERO if component is below this threshold
    // Note: MyAvatar.setThrust might need an update to account for the recent avatar density changes...
    // For now, this discovered scaling factor seems to accomodate a similar easing effect to the other movement models.
    var magicScalingFactor = 12.0 * (maxVelocity + 120) / 16 - Math.sqrt( maxVelocity / 8 );

    var targetThrust = Vec3.multiply(magicScalingFactor, targetVelocity);
    targetThrust = vec3eclamp(targetThrust, THRUST_FALLOFF, maxVelocity);
    if (Vec3.length(MyAvatar.velocity) > maxVelocity) {
        targetThrust = Vec3.multiply(0.5, targetThrust);
    }
    return targetThrust;
}

// ----------------------------------------------------------------------------
// clamp components and magnitude to maxVelocity, rounding to Vec3.ZERO if below epsilon
function vec3eclamp(velocity, epsilon, maxVelocity) {
    velocity = {
        x: Math.max(-maxVelocity, Math.min(maxVelocity, velocity.x)),
        y: Math.max(-maxVelocity, Math.min(maxVelocity, velocity.y)),
        z: Math.max(-maxVelocity, Math.min(maxVelocity, velocity.z))
    };

    if (Math.abs(velocity.x) < epsilon) {
        velocity.x = 0;
    }
    if (Math.abs(velocity.y) < epsilon) {
        velocity.y = 0;
    }
    if (Math.abs(velocity.z) < epsilon) {
        velocity.z = 0;
    }

    var length = Vec3.length(velocity);
    if (length > maxVelocity) {
        velocity = Vec3.multiply(maxVelocity, Vec3.normalize(velocity));
    } else if (length < epsilon) {
        velocity = Vec3.ZERO;
    }
    return velocity;
}

function vec3damp(active, positiveEffect, negativeEffect) {
    // If force isn't being applied in a direction, incorporate negative effect (drag);
    negativeEffect = {
        x: active.x ? 0 : negativeEffect.x,
        y: active.y ? 0 : negativeEffect.y,
        z: active.z ? 0 : negativeEffect.z,
    };
    return Vec3.subtract(Vec3.sum(active, positiveEffect), negativeEffect);
}

// ----------------------------------------------------------------------------
function VelocityTracker(defaultValues) {
    Object.defineProperty(this, 'defaultValues', { configurable: true, value: defaultValues });
}
VelocityTracker.prototype = {
    constructor: VelocityTracker,
    reset: function() {
        Object.assign(this, this.defaultValues);
    },
    integrate: function(targetState, currentVelocities, drag, settings) {
        var args = [].slice.call(arguments);
        this._applyIntegration('translation', args);
        this._applyIntegration('rotation', args);
        this._applyIntegration('zoom', args);
    },
    _applyIntegration: function(component, args) {
        return this._integrate.apply(this, [component].concat(args));
    },
    _integrate: function(component, targetState, currentVelocities, drag, settings) {
        assert(targetState[component], component + ' not found in targetState (which has: ' + Object.keys(targetState) + ')');
        var result = vec3damp(
            targetState[component],
            currentVelocities[component],
            drag[component]
        );
        var maxVelocity = settings[component].maxVelocity;
        return this[component] = vec3eclamp(result, settings.epsilon, maxVelocity);
    },
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
Object.assign(CameraControls, {
    SCRIPT_UPDATE: 'update',
    ANIMATION_FRAME: 'requestAnimationFrame', // emulated
    NEXT_TICK: 'nextTick', // emulated
    SET_IMMEDIATE: 'setImmediate', // emulated
    //WORKER_THREAD: 'workerThread',
});

function CameraControls(options) {
    options = options || {};
    assert('update' in options && 'threadMode' in options);
    this.updateObject = typeof options.update === 'function' ? options : options.update;
    assert(typeof this.updateObject.update === 'function',
           'construction options expected either { update: function(){}... } object or a function(){}');
    this.update = _utils.bind(this.updateObject, 'update');
    this.threadMode = options.threadMode;
    this.fps = options.fps || 60;
    this.getRuntimeSeconds = options.getRuntimeSeconds || function() {
        return +new Date / 1000.0;
    };
    this.backupOptions = _utils.DeferredUpdater.createGroup({
        MyAvatar: MyAvatar,
        Camera: Camera,
        Reticle: Reticle,
    });

    this.enabled = false;
    this.enabledChanged = _utils.signal(function enabledChanged(enabled){});
    this.modeChanged = _utils.signal(function modeChanged(mode, oldMode){});
}
CameraControls.prototype = {
    constructor: CameraControls,
    $animate: null,
    $start: function() {
        if (this.$animate) {
            return;
        }

        var lastTime;
        switch (this.threadMode) {
            case CameraControls.SCRIPT_UPDATE: {
                this.$animate = this.update;
                Script.update.connect(this, '$animate');
                this.$animate.disconnect = _utils.bind(this, function() {
                    Script.update.disconnect(this, '$animate');
                });
            } break;

            case CameraControls.ANIMATION_FRAME: {
                this.requestAnimationFrame = _utils.createAnimationStepper({
                    getRuntimeSeconds: this.getRuntimeSeconds,
                    fps: this.fps
                });
                this.$animate = _utils.bind(this, function(dt) {
                    this.update(dt);
                    this.requestAnimationFrame(this.$animate);
                });
                this.$animate.disconnect = _utils.bind(this.requestAnimationFrame, 'reset');
                this.requestAnimationFrame(this.$animate);
            } break;

            case CameraControls.SET_IMMEDIATE: {
                // emulate process.setImmediate (attempt to execute at start of next update frame, sans Script.update throttling)
                lastTime = this.getRuntimeSeconds();
                this.$animate = Script.setInterval(_utils.bind(this, function() {
                    this.update(this.getRuntimeSeconds(lastTime));
                    lastTime = this.getRuntimeSeconds();
                }), 5);
                this.$animate.disconnect = function() {
                    Script.clearInterval(this);
                };
            } break;

            case CameraControls.NEXT_TICK: {
                // emulate process.nextTick (attempt to queue at the very next opportunity beyond current scope)
                lastTime = this.getRuntimeSeconds();
                this.$animate = _utils.bind(this, function() {
                    this.$animate.timeout = 0;
                    if (this.$animate.quit) {
                        return;
                    }
                    this.update(this.getRuntimeSeconds(lastTime));
                    lastTime = this.getRuntimeSeconds();
                    this.$animate.timeout = Script.setTimeout(this.$animate, 0);
                });
                this.$animate.quit = false;
                this.$animate.disconnect = function() {
                    this.timeout && Script.clearTimeout(this.timeout);
                    this.timeout = 0;
                    this.quit = true;
                };
                this.$animate();
            } break;

            default: throw new Error('unknown threadMode: ' + this.threadMode);
        }
        log(
            '...$started update thread', '(threadMode: ' + this.threadMode + ')',
            this.threadMode === CameraControls.ANIMATION_FRAME && this.fps
        );
    },
    $stop: function() {
        if (!this.$animate) {
            return;
        }
        try {
            this.$animate.disconnect();
        } catch (e) {
            log('$animate.disconnect error: ' + e, '(threadMode: ' + this.threadMode +')');
        }
        this.$animate = null;
        log('...$stopped updated thread', '(threadMode: ' + this.threadMode +')');
    },
    onModeUpdated: function onModeUpdated(mode, oldMode) {
        oldMode = oldMode || this.previousMode;
        this.previousMode = mode;
        log('onModeUpdated', oldMode + '->' + mode);
        // user changed modes, so leave the current mode intact later when restoring backup values
        delete this.backupOptions.Camera.$setModeString;
        if (/person/.test(oldMode) && /person/.test(mode)) {
            return; // disregard first -> third and third ->first transitions
        }
        this.modeChanged(mode, oldMode);
    },

    reset: function() {
        if (this.enabled) {
            this.disable();
            this.enable();
        }
    },
    setEnabled: function setEnabled(enabled) {
        if (!this.enabled && enabled) {
            this.enable();
        } else if (this.enabled && !enabled) {
            this.disable();
        }
    },
    enable: function enable() {
        if (this.enabled) {
            throw new Error('CameraControls.enable -- already enabled..');
        }
        log('ENABLE enableCameraMove', this.threadMode);

        this._backup();

        this.previousMode = Camera.mode;
        Camera.modeUpdated.connect(this, 'onModeUpdated');

        this.$start();

        this.enabledChanged(this.enabled = true);
    },
    disable: function disable() {
        log("DISABLE CameraControls");
        try {
            Camera.modeUpdated.disconnect(this, 'onModeUpdated');
        } catch (e) {
            debugPrint(e);
        }
        this.$stop();

        this._restore();

        if (this.enabled !== false) {
            this.enabledChanged(this.enabled = false);
        }
    },
    _restore: function() {
        var submitted = this.backupOptions.submit();
        log('restored previous values: ' + JSON.stringify(submitted,0,2));
        return submitted;
    },
    _backup: function() {
        this.backupOptions.reset();
        Object.assign(this.backupOptions.Reticle, {
            scale: Reticle.scale,
        });
        Object.assign(this.backupOptions.Camera, {
            $setModeString: Camera.mode,
        });
        Object.assign(this.backupOptions.MyAvatar, {
            motorTimescale: MyAvatar.motorTimescale,
            motorReferenceFrame: MyAvatar.motorReferenceFrame,
            motorVelocity: Vec3.ZERO,
            velocity: Vec3.ZERO,
            angularVelocity: Vec3.ZERO,
        });
    },
}; // CameraControls

// ----------------------------------------------------------------------------
function applyEasing(deltaTime, direction, settings, state, scaling) {
    var obj = {};
    for (var p in scaling) {
        var group = settings[p],          // translation | rotation | zoom
            easeConst = group[direction], // easeIn | easeOut
            scale = scaling[p],
            stateVector = state[p];
        obj[p] = Vec3.multiply(easeConst * scale * deltaTime, stateVector);
    }
    return obj;
}
