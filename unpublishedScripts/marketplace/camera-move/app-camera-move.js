//  app-camera-move.js
//
//  Created by Timothy Dedischew on 05/05/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  This Client script sets up the Camera Control Tablet App, which can be used to configure and
//  drive your avatar with easing/smoothing movement constraints for a less jittery filming experience.

/* eslint-disable comma-dangle, no-empty */
"use strict";

var VERSION = '0.0.0d',
    NAMESPACE = 'app-camera-move',
    APP_HTML_URL = Script.resolvePath('app.html'),
    BUTTON_CONFIG = {
        text: '\nCam Drive',
        icon: Script.resolvePath('Eye-Camera.svg'),
    },
    DEFAULT_TOGGLE_KEY = { text: 'SPACE' };

var MINIMAL_CURSOR_SCALE = 0.5,
    FILENAME = Script.resolvePath(''),
    WANT_DEBUG = (
        false || (FILENAME.match(/[&#?]debug[=](\w+)/)||[])[1] ||
            Settings.getValue(NAMESPACE + '/debug')
    ),
    EPSILON = 1e-6;

function log() {
    print( NAMESPACE + ' | ' + [].slice.call(arguments).join(' ') );
}

var require = Script.require,
    debugPrint = function(){},
    _debugChannel = NAMESPACE + '.stats',
    overlayDebugOutput = function(){};

if (WANT_DEBUG) {
    log('WANT_DEBUG is true; instrumenting debug rigging', WANT_DEBUG);
    _instrumentDebugValues();
}

var _utils = require('./modules/_utils.js'),
    assert = _utils.assert,
    CustomSettingsApp = require('./modules/custom-settings-app/CustomSettingsApp.js'),
    movementUtils = require('./modules/movement-utils.js?'+ +new Date),
    configUtils = require('./modules/config-utils.js'),
    AvatarUpdater = require('./avatar-updater.js');


Object.assign = Object.assign || _utils.assign;

var cameraControls, eventMapper, cameraConfig, applicationConfig;

var DEFAULTS = {
    'namespace': NAMESPACE,
    'debug': WANT_DEBUG,
    'camera-move-enabled': false,
    'thread-update-mode': movementUtils.CameraControls.SCRIPT_UPDATE,
    'fps': 90,
    'drive-mode': movementUtils.DriveModes.MOTOR,
    'use-head': true,
    'stay-grounded': true,
    'prevent-roll': true,
    'constant-delta-time': false,
    'minimal-cursor': false,
    'normalize-inputs': false,
    'enable-mouse-smooth': true,
    'translation-max-velocity': 5.50,
    'translation-ease-in': 1.25,
    'translation-ease-out': 5.50,
    'rotation-max-velocity': 90.00,
    'rotation-ease-in': 1.00,
    'rotation-ease-out': 5.50,
    'rotation-x-speed': 45,
    'rotation-y-speed': 60,
    'rotation-z-speed': 1,
    'mouse-multiplier': 1.0,
    'keyboard-multiplier': 1.0,

    'ui-enable-tooltips': true,
    'ui-show-advanced-options': false,

    // 'toggle-key': DEFAULT_TOGGLE_KEY,
};

// map setting names to/from corresponding Menu and API properties
var APPLICATION_SETTINGS = {
    'Avatar/pitchSpeed': 'pitchSpeed' in MyAvatar && {
        object: [ MyAvatar, 'pitchSpeed' ]
    },
    'Avatar/yawSpeed': 'yawSpeed' in MyAvatar && {
        object: [ MyAvatar, 'yawSpeed' ]
    },
    'Avatar/Enable Avatar Collisions': {
        menu: 'Avatar > Enable Avatar Collisions',
        object: [ MyAvatar, 'collisionsEnabled' ],
    },
    'Avatar/Draw Mesh': {
        menu: 'Developer > Draw Mesh',
        // object: [ MyAvatar, 'shouldRenderLocally' ], // shouldRenderLocally seems to be broken...
        object: [ MyAvatar, 'getEnableMeshVisible', 'setEnableMeshVisible' ],
    },
    'Avatar/useSnapTurn': {
        object: [ MyAvatar, 'getSnapTurn', 'setSnapTurn' ],
    },
    'Avatar/lookAtSnappingEnabled': 'lookAtSnappingEnabled' in MyAvatar && {
        menu: 'Developer > Enable LookAt Snapping',
        object: [ MyAvatar, 'lookAtSnappingEnabled' ]
    },
    'camera-move-enabled': {
        get: function() {
            return cameraControls && cameraControls.enabled;
        },
        set: function(nv) {
            cameraControls.setEnabled(!!nv);
        },
    },
    // 'toggle-key': {
    //     get: function() { try {
    //         return JSON.parse(cameraConfig.getValue('toggle-key'));
    //     } catch (e) {
    //         return DEFAULT_TOGGLE_KEY;
    //     } },
    //     set: function(nv) {
    //         assert(typeof nv === 'object', 'new toggle-key is not an object: ' + nv);
    //         cameraConfig.setValue('toggle-key', JSON.parse(JSON.stringify(nv)));
    //     },
    // },
};

var DEBUG_INFO = {
    // these values are also sent to the tablet app after EventBridge initialization
    appVersion: VERSION,
    utilsVersion: _utils.version,
    movementVersion: movementUtils.version,
    configVersion: configUtils.version,
    clientScript: Script.resolvePath(''),
    MyAvatar: {
        supportsPitchSpeed: 'pitchSpeed' in MyAvatar,
        supportsYawSpeed: 'yawSpeed' in MyAvatar,
        supportsLookAtSnappingEnabled: 'lookAtSnappingEnabled' in MyAvatar,
        supportsDensity: 'density' in MyAvatar,
    },
    Reticle: {
        supportsScale: 'scale' in Reticle,
    },
    protocolVersion: location.protocolVersion,
};

var globalState = {
    // cached values from the last animation frame
    previousValues: {
        reset: function() {
            this.pitchYawRoll = Vec3.ZERO;
            this.thrust = Vec3.ZERO;
        },
    },

    // batch updates to MyAvatar/Camera properties (submitting together seems to help reduce timeslice jitter)
    pendingChanges: _utils.DeferredUpdater.createGroup({
        Camera: Camera,
        MyAvatar: MyAvatar,
    }, { dedupe: false }),

    // current input controls' effective velocities
    currentVelocities: new movementUtils.VelocityTracker({
        translation: Vec3.ZERO,
        step_translation: Vec3.ZERO, // eslint-disable-line camelcase
        rotation: Vec3.ZERO,
        step_rotation: Vec3.ZERO,    // eslint-disable-line camelcase
        zoom: Vec3.ZERO,
    }),
};

function main() {
    log('initializing...', VERSION);

    var tablet = Tablet.getTablet('com.highfidelity.interface.tablet.system'),
        button = tablet.addButton(BUTTON_CONFIG);

    Script.scriptEnding.connect(function() {
        tablet.removeButton(button);
        button = null;
    });

    // track both runtime state (applicationConfig) and settings state (cameraConfig)
    // (this is necessary because Interface does not yet consistently keep config Menus, APIs and Settings in sync)
    applicationConfig = new configUtils.ApplicationConfig({
        namespace: DEFAULTS.namespace,
        config: APPLICATION_SETTINGS,
    });
    cameraConfig = new configUtils.SettingsConfig({
        namespace: DEFAULTS.namespace,
        defaultValues: DEFAULTS,
    });

    var toggleKey = DEFAULT_TOGGLE_KEY;
    if (cameraConfig.getValue('toggle-key')) {
        try {
            toggleKey = JSON.parse(cameraConfig.getValue('toggle-key'));
        } catch (e) {}
    }
    // set up a monitor to observe configuration changes between the two sources
    var MONITOR_INTERVAL_MS = 1000;
    _startConfigationMonitor(applicationConfig, cameraConfig, MONITOR_INTERVAL_MS);

    // ----------------------------------------------------------------------------
    // set up the tablet webview app
    log('APP_HTML_URL', APP_HTML_URL);
    var settingsApp = new CustomSettingsApp({
        namespace: cameraConfig.namespace,
        uuid: cameraConfig.uuid,
        settingsAPI: cameraConfig,
        url: APP_HTML_URL,
        tablet: tablet,
        extraParams: Object.assign({
            toggleKey: toggleKey,
        }, _utils.getSystemMetadata(), DEBUG_INFO),
        debug: WANT_DEBUG > 1,
    });
    Script.scriptEnding.connect(settingsApp, 'cleanup');
    settingsApp.valueUpdated.connect(function(key, value, oldValue, origin) {
        log('[settingsApp.valueUpdated @ ' + origin + ']', key + ' = ' + JSON.stringify(value) + ' (was: ' + JSON.stringify(oldValue) + ')');
        if (/tablet/i.test(origin)) {
            log('cameraConfig applying immediate setting', key, value);
            // apply relevant settings immediately when changed from the app UI
            applicationConfig.applyValue(key, value, origin);
        }
    });

    settingsApp.onUnhandledMessage = function(msg) {
        switch (msg.method) {
            case 'window.close': {
                this.toggle(false);
            } break;
            case 'reloadClientScript': {
                log('reloadClientScript...');
                _utils.reloadClientScript(FILENAME);
            } break;
            case 'resetSensors': {
                Menu.triggerOption('Reset Sensors');
                Script.setTimeout(function() {
                    MyAvatar.bodyPitch = 0;
                    MyAvatar.bodyRoll = 0;
                    MyAvatar.orientation = Quat.cancelOutRollAndPitch(MyAvatar.orientation);
                }, 500);
            } break;
            case 'reset': {
                // if (!Window.confirm('Reset all camera move settings to system defaults?')) {
                //    return;
                // }
                var resetValues = {};
                Object.keys(DEFAULTS).reduce(function(out, key) {
                    var resolved = cameraConfig.resolve(key);
                    out[resolved] = resolved in out ? out[resolved] : DEFAULTS[key];
                    return out;
                }, resetValues);
                Object.keys(applicationConfig.config).reduce(function(out, key) {
                    var resolved = applicationConfig.resolve(key);
                    out[resolved] = resolved in out ? out[resolved] : applicationConfig.getValue(key);
                    return out;
                }, resetValues);
                log('resetValues', JSON.stringify(resetValues, 0, 2));
                for (var p in resetValues) {
                    var value = resetValues[p];
                    applicationConfig.applyValue(p, value, 'reset');
                    cameraConfig.setValue(p, value);
                }
            } break;
            case 'overlayWebWindow': {
                _utils._overlayWebWindow(msg.options);
            } break;
            default: {
                log('onUnhandledMessage', JSON.stringify(msg,0,2));
            } break;
        }
    };

    // ----------------------------------------------------------------------------
    // set up the keyboard/mouse/controller/meta input state manager
    eventMapper = new movementUtils.MovementEventMapper({
        namespace: DEFAULTS.namespace,
        mouseSmooth: cameraConfig.getValue('enable-mouse-smooth'),
        xexcludeNames: [ 'Keyboard.C', 'Keyboard.E', 'Actions.TranslateY' ],
        mouseMultiplier: cameraConfig.getValue('mouse-multiplier'),
        keyboardMultiplier: cameraConfig.getValue('keyboard-multiplier'),
        eventFilter: function eventFilter(from, event, defaultFilter) {
            var result = defaultFilter(from, event),
                driveKeyName = event.driveKeyName;
            if (!result || !driveKeyName) {
                if (from === 'Keyboard.RightMouseButton') {
                    settingsApp.syncValue('Keyboard.RightMouseButton', event.actionValue, 'eventFilter');
                }
                return 0;
            }
            if (cameraConfig.getValue('normalize-inputs')) {
                result = _utils.sign(result);
            }
            if (from === 'Actions.Pitch') {
                result *= cameraConfig.getFloat('rotation-x-speed');
            }
            if (from === 'Actions.Yaw') {
                result *= cameraConfig.getFloat('rotation-y-speed');
            }
            return result;
        },
    });
    Script.scriptEnding.connect(eventMapper, 'disable');
    applicationConfig.register({
        'enable-mouse-smooth': { object: [ eventMapper.options, 'mouseSmooth' ] },
        'keyboard-multiplier': { object: [ eventMapper.options, 'keyboardMultiplier' ] },
        'mouse-multiplier': { object: [ eventMapper.options, 'mouseMultiplier' ] },
    });

    // ----------------------------------------------------------------------------
    // set up the top-level camera controls manager / animator
    var avatarUpdater = new AvatarUpdater({
        debugChannel: _debugChannel,
        globalState: globalState,
        getCameraMovementSettings: getCameraMovementSettings,
        getMovementState: _utils.bind(eventMapper, 'getState'),
    });
    cameraControls = new movementUtils.CameraControls({
        namespace: DEFAULTS.namespace,
        update: avatarUpdater,
        threadMode: cameraConfig.getValue('thread-update-mode'),
        fps: cameraConfig.getValue('fps'),
        getRuntimeSeconds: _utils.getRuntimeSeconds,
    });
    Script.scriptEnding.connect(cameraControls, 'disable');
    applicationConfig.register({
        'thread-update-mode': { object: [ cameraControls, 'threadMode' ] },
        'fps': { object: [ cameraControls, 'fps' ] },
    });

    // ----------------------------------------------------------------------------
    // set up SPACEBAR for toggling camera movement mode
    var spacebar = new _utils.KeyListener(Object.assign(toggleKey, {
        onKeyPressEvent: function(event) {
            cameraControls.setEnabled(!cameraControls.enabled);
        },
    }));
    Script.scriptEnding.connect(spacebar, 'disconnect');

    // ----------------------------------------------------------------------------
    // set up ESC for reset drive key states
    Script.scriptEnding.connect(new _utils.KeyListener({
        text: 'ESC',
        onKeyPressEvent: function(event) {
            if (cameraControls.enabled) {
                log('ESC pressed -- resetting drive keys values:', JSON.stringify({
                    virtualDriveKeys: eventMapper.states,
                    movementState: eventMapper.getState(),
                }, 0, 2));
                eventMapper.states.reset();
            }
        },
    }), 'disconnect');

    // set up the tablet button to toggle the app UI display
    button.clicked.connect(settingsApp, function(enable) {
        Object.assign(this.extraParams, _utils.getSystemMetadata());
        this.toggle(enable);
    });

    settingsApp.isActiveChanged.connect(function(isActive) {
        updateButtonText();
    });

    cameraControls.modeChanged.connect(onCameraModeChanged);

    var fpsTimeout = 0;
    function updateButtonText() {
        var lines = [
            settingsApp.isActive ? '(app open)' : '',
            cameraControls.enabled ? (avatarUpdater.update.momentaryFPS||0).toFixed(2) + 'fps' : BUTTON_CONFIG.text.trim()
        ];
        button && button.editProperties({ text: lines.join('\n') });
    }

    cameraControls.enabledChanged.connect(function(enabled) {
        log('enabledChanged', enabled);
        button && button.editProperties({ isActive: enabled });
        if (enabled) {
            onCameraControlsEnabled();
            fpsTimeout = Script.setInterval(updateButtonText, 1000);
        } else {
            if (fpsTimeout) {
                Script.clearInterval(fpsTimeout);
                fpsTimeout = 0;
            }
            eventMapper.disable();
            avatarUpdater._resetMyAvatarMotor({ MyAvatar: MyAvatar });
            updateButtonText();
        }
        overlayDebugOutput.overlayID && Overlays.editOverlay(overlayDebugOutput.overlayID, { visible: enabled });
    });

    var resetIfChanged = [
        'minimal-cursor', 'drive-mode', 'fps', 'thread-update-mode',
        'mouse-multiplier', 'keyboard-multiplier',
        'enable-mouse-smooth', 'constant-delta-time',
    ].filter(Boolean).map(_utils.bind(cameraConfig, 'resolve'));

    cameraConfig.valueUpdated.connect(function(key, value, oldValue, origin) {
        var triggerReset = !!~resetIfChanged.indexOf(key);
        log('[cameraConfig.valueUpdated @ ' + origin + ']',
            key + ' = ' + JSON.stringify(value), '(was:' + JSON.stringify(oldValue) + ')',
            'triggerReset: ' + triggerReset);

        if (/tablet/i.test(origin)) {
            log('cameraConfig applying immediate setting', key, value);
            // apply relevant settings immediately when changed from the app UI
            applicationConfig.applyValue(key, value, origin);
            log(JSON.stringify(cameraConfig.getValue(key)));
        }
        0&&debugPrint('//cameraConfig.valueUpdated', JSON.stringify({
            key: key,
            cameraConfig: cameraConfig.getValue(key),
            applicationConfig: applicationConfig.getValue(key),
            value: value,
            triggerReset: triggerReset,
        },0,2));

        if (triggerReset) {
            log('KEYBOARD multiplier', eventMapper.options.keyboardMultiplier);
            cameraControls.reset();
        }
    });

    if (cameraConfig.getValue('camera-move-enabled')) {
        cameraControls.enable();
    }

    log('DEFAULTS', JSON.stringify(DEFAULTS, 0, 2));
} // main()

function onCameraControlsEnabled() {
    log('onCameraControlsEnabled!!!! ----------------------------------------------');
    globalState.previousValues.reset();
    globalState.currentVelocities.reset();
    globalState.pendingChanges.reset();
    eventMapper.enable();
    if (cameraConfig.getValue('minimal-cursor')) {
        Reticle.scale = MINIMAL_CURSOR_SCALE;
    }
    log('cameraConfig', JSON.stringify({
        cameraConfig: getCameraMovementSettings(),
        //DEFAULTS: DEFAULTS
    }));
}

// reset values based on the selected Camera.mode (to help keep the visual display/orientation more reasonable)
function onCameraModeChanged(mode, oldMode) {
    globalState.pendingChanges.reset();
    globalState.previousValues.reset();
    eventMapper.reset();
    var preventRoll = cameraConfig.getValue('prevent-roll');
    var avatarOrientation = cameraConfig.getValue('use-head') ? MyAvatar.headOrientation : MyAvatar.orientation;
    if (preventRoll) {
        avatarOrientation = Quat.cancelOutRollAndPitch(avatarOrientation);
    }
    switch (Camera.mode) {
        case 'mirror':
        case 'entity':
        case 'independent':
            globalState.currentVelocities.reset();
            break;
        default:
            Camera.position = MyAvatar.position;
            Camera.orientation = avatarOrientation;
            break;
    }
    MyAvatar.orientation = avatarOrientation;
    if (preventRoll) {
        MyAvatar.headPitch = MyAvatar.headRoll = 0;
    }
}

// consolidate and normalize cameraConfig settings
function getCameraMovementSettings() {
    return {
        epsilon: EPSILON,
        debug: cameraConfig.getValue('debug'),
        driveMode: cameraConfig.getValue('drive-mode'),
        threadMode: cameraConfig.getValue('thread-update-mode'),
        fps: cameraConfig.getValue('fps'),
        useHead: cameraConfig.getValue('use-head'),
        stayGrounded: cameraConfig.getValue('stay-grounded'),
        preventRoll: cameraConfig.getValue('prevent-roll'),
        useConstantDeltaTime: cameraConfig.getValue('constant-delta-time'),

        collisionsEnabled: applicationConfig.getValue('Avatar/Enable Avatar Collisions'),
        mouseSmooth: cameraConfig.getValue('enable-mouse-smooth'),
        mouseMultiplier: cameraConfig.getValue('mouse-multiplier'),
        keyboardMultiplier: cameraConfig.getValue('keyboard-multiplier'),

        rotation: _getEasingGroup(cameraConfig, 'rotation'),
        translation: _getEasingGroup(cameraConfig, 'translation'),
        zoom: _getEasingGroup(cameraConfig, 'zoom'),
    };

    // extract an easing group (translation, rotation, or zoom) from cameraConfig
    function _getEasingGroup(cameraConfig, group) {
        var multiplier = 1.0;
        if (group === 'zoom') {
            // BoomIn / TranslateCameraZ support is only partially plumbed -- for now use scaled translation easings
            group = 'translation';
            multiplier = 0.001;
        }
        return {
            easeIn: cameraConfig.getFloat(group + '-ease-in'),
            easeOut: cameraConfig.getFloat(group + '-ease-out'),
            maxVelocity: multiplier * cameraConfig.getFloat(group + '-max-velocity'),
            speed: Vec3.multiply(multiplier, {
                x: cameraConfig.getFloat(group + '-x-speed'),
                y: cameraConfig.getFloat(group + '-y-speed'),
                z: cameraConfig.getFloat(group + '-z-speed')
            }),
        };
    }
}

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
function _startConfigationMonitor(applicationConfig, cameraConfig, interval) {
    // monitor and sync Application state -> Settings values
    return Script.setInterval(function monitor() {
        var settingNames = Object.keys(applicationConfig.config);
        settingNames.forEach(function(key) {
            applicationConfig.resyncValue(key); // align Menus <=> APIs
            var value = cameraConfig.getValue(key),
                appValue = applicationConfig.getValue(key);
            if (appValue !== undefined && String(appValue) !== String(value)) {
                log('applicationConfig -> cameraConfig',
                    key, [typeof appValue, appValue], '(was:'+[typeof value, value]+')');
                cameraConfig.setValue(key, appValue); // align Application <=> Settings
            }
        });
    }, interval);
}

// ----------------------------------------------------------------------------
function _fixedPrecisionStringifiyFilter(key, value, object) {
    if (typeof value === 'object' && value && 'w' in value) {
        return Quat.safeEulerAngles(value);
    } else if (typeof value === 'number') {
        return value.toFixed(4)*1;
    }
    return value;
}
function _createOverlayDebugOutput(options) {
    options = require('./modules/_utils.js').assign({
        x: 0, y: 0, width: 500, height: 800, visible: false
    }, options || {});
    options.lineHeight = options.lineHeight || Math.round(options.height / 36);
    options.font = options.font || { size: Math.floor(options.height / 36) };
    overlayDebugOutput.overlayID = Overlays.addOverlay('text', options);

    Messages.subscribe(_debugChannel);
    Messages.messageReceived.connect(onMessageReceived);

    Script.scriptEnding.connect(function() {
        Overlays.deleteOverlay(overlayDebugOutput.overlayID);
        Messages.unsubscribe(_debugChannel);
        Messages.messageReceived.disconnect(onMessageReceived);
    });
    function overlayDebugOutput(output) {
        var text = JSON.stringify(output, _fixedPrecisionStringifiyFilter, 2);
        if (text !== overlayDebugOutput.lastText) {
            overlayDebugOutput.lastText = text;
            Overlays.editOverlay(overlayDebugOutput.overlayID, { text: text });
        }
    }
    function onMessageReceived(channel, message, ssend, local) {
        if (!local || channel !== _debugChannel) {
            return;
        }
        overlayDebugOutput(JSON.parse(message));
    }

    return overlayDebugOutput;
}


// ----------------------------------------------------------------------------
_patchCameraModeSetting();
function _patchCameraModeSetting() {
    // FIXME: looks like the Camera API suffered a regression where setting Camera.mode = 'first person' or 'third person'
    //  no longer works; the only reliable way to set it now seems to be jury-rigging the Menu items...
    Camera.$setModeString = Camera.$setModeString || function(mode) {
        // 'independent' => "Independent Mode", 'first person' => 'First Person', etc.
        var cameraMenuItem = (mode+'')
            .replace(/^(independent|entity)$/, '$1 mode')
            .replace(/\b[a-z]/g, function(ch) {
                return ch.toUpperCase();
            });

        log('working around Camera.mode bug by enabling the menuItem:', cameraMenuItem);
        Menu.setIsOptionChecked(cameraMenuItem, true);
    };
}
// ----------------------------------------------------------------------------

function _instrumentDebugValues() {
    debugPrint = log;
    var cacheBuster = '?' + new Date().getTime().toString(36);
    require = Script.require(Script.resolvePath('./modules/_utils.js') + cacheBuster).makeDebugRequire(Script.resolvePath('.'));
    APP_HTML_URL += cacheBuster;
    overlayDebugOutput = _createOverlayDebugOutput({
        lineHeight: 12,
        font: { size: 12 },
        width: 250, height: 800 });
    // auto-disable camera move mode when debugging
    Script.scriptEnding.connect(function() {
        cameraConfig && cameraConfig.setValue('camera-move-enabled', false);
    });
}

// Show fatal (unhandled) exceptions in a BSOD popup
Script.unhandledException.connect(function onUnhandledException(error) {
    log('UNHANDLED EXCEPTION!!', error, error && error.stack);
    try {
        cameraControls.disable();
    } catch (e) {}
    Script.unhandledException.disconnect(onUnhandledException);
    if (WANT_DEBUG) {
        // show blue screen of death with the error details
        new _utils.BSOD({
            error: error,
            buttons: [ 'Abort', 'Retry', 'Fail' ],
            debugInfo: DEBUG_INFO,
        }, function(error, button) {
            log('BSOD.result', error, button);
            if (button === 'Abort') {
                Script.stop();
            } else if (button === 'Retry') {
                _utils.reloadClientScript(FILENAME);
            }
        });
    } else {
        // use a simple alert to display just the error message
        Window.alert('app-camera-move error: ' + error.message);
        Script.stop();
    }
});
// ----------------------------------------------------------------------------

main();
