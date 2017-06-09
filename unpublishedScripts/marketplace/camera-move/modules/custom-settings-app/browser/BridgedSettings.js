// BridgedSettings.js -- HTML-side implementation of bridged/async Settings
// see ../CustomSettingsApp.js for the corresponding Interface script

/* eslint-env commonjs, browser */
(function(global) {
    "use strict";

    BridgedSettings.version = '0.0.2';

    try {
        module.exports = BridgedSettings;
    } catch (e) {
        global.BridgedSettings = BridgedSettings;
    }

    var _utils = global._utils || (global.require && global.require('../_utils.js'));

    if (!_utils || !_utils.signal) {
        throw new Error('html.BridgedSettings.js -- expected _utils to be available on the global object (ie: window._utils)');
    }
    var signal = _utils.signal,
        assert = _utils.assert;

    function log() {
        console.info('bridgedSettings | ' + [].slice.call(arguments).join(' ')); // eslint-disable-line no-console
    }
    log('version', BridgedSettings.version);

    var debugPrint = function() {}; // = log

    function BridgedSettings(options) {
        options = options || {};
        this.eventBridge = options.eventBridge || global.EventBridge;
        this.namespace = options.namespace || 'BridgedSettings';
        this.uuid = options.uuid || undefined;
        this.valueUpdated = signal(function valueUpdated(key, newValue, oldValue, origin){});
        this.callbackError = signal(function callbackError(error, message){});
        this.pendingRequestsFinished = signal(function pendingRequestsFinished(){});
        this.extraParams = {};

        // keep track of accessed settings so they can be kept in sync when changed on this side
        this._activeSettings = {
            sent: {},
            received: {},
            remote: {},
        };

        this.debug = options.debug;
        this.log = log.bind({}, this.namespace + ' |');
        this.debugPrint = function() { return this.debug && this.log.apply(this, arguments); };

        this.log('connecting to EventBridge.scriptEventReceived');
        this._boundScriptEventReceived = this.onScriptEventReceived.bind(this);
        this.eventBridge.scriptEventReceived.connect(this._boundScriptEventReceived);

        this.callbacks = Object.defineProperties(options.callbacks || {}, {
            extraParams: { value: this.handleExtraParams },
            valueUpdated: { value: this.handleValueUpdated },
        });
    }

    BridgedSettings.prototype = {
        _callbackId: 1,
        resolve: function(key) {
            if (0 !== key.indexOf('.') && !~key.indexOf('/')) {
                return [ this.namespace, key ].join('/');
            } else {
                return key;
            }
        },
        handleValueUpdated: function(msg) {
            // client script notified us that a value was updated on that side
            var key = this.resolve(msg.params[0]),
                value = msg.params[1],
                oldValue = msg.params[2],
                origin = msg.params[3];
                log('callbacks.valueUpdated', key, value, oldValue, origin);
            this._activeSettings.received[key] = this._activeSettings.remote[key] = value;
            this.valueUpdated(key, value, oldValue, (origin?origin+':':'') + 'callbacks.valueUpdated');
        },
        handleExtraParams: function(msg) {
            // client script sent us extraParams
            var extraParams = msg.extraParams;
            Object.assign(this.extraParams, extraParams);
            this.debugPrint('received .extraParams', JSON.stringify(extraParams,0,2));
            var key = '.extraParams';
            this._activeSettings.received[key] = this._activeSettings.remote[key] = extraParams;
            this.valueUpdated(key, this.extraParams, this.extraParams, 'html.bridgedSettings.handleExtraParams');
        },
        cleanup: function() {
            try {
                this.eventBridge.scriptEventReceived.disconnect(this._boundScriptEventReceived);
            } catch(e) {
                this.log('error disconnecting from scriptEventReceived:', e);
            }
        },
        pendingRequestCount: function() {
            return Object.keys(this.callbacks).length;
        },
        onScriptEventReceived: function(_msg) {
            var error;
            this.debugPrint(this.namespace, '_onScriptEventReceived......' + _msg);
            try {
                var msg = JSON.parse(_msg);
                if (msg.ns === this.namespace && msg.uuid === this.uuid) {
                    this.debugPrint('_onScriptEventReceived', msg);
                    var callback = this.callbacks[msg.id],
                        handled = false,
                        debug = this.debug;
                    if (callback) {
                        try {
                            callback.call(this, msg);
                            handled = true;
                        } catch (e) {
                            error = e;
                            this.log('CALLBACK ERROR', this.namespace, msg.id, '_onScriptEventReceived', e);
                            this.callbackError(error, msg);
                            if (debug) {
                                throw error;
                            }
                        }
                    }
                    if (!handled) {
                        if (this.onUnhandledMessage) {
                            return this.onUnhandledMessage(msg, _msg);
                        } else {
                            error = new Error('unhandled message: ' + _msg);
                        }
                    }
                }
            } catch (e) { error = e; }
            if (this.debug && error) {
                throw error;
            }
            return error;
        },
        sendEvent: function(msg) {
            msg.ns = msg.ns || this.namespace;
            msg.uuid = msg.uuid || this.uuid;
            this.eventBridge.emitWebEvent(JSON.stringify(msg));
        },
        getValue: function(key, defaultValue) {
            key = this.resolve(key);
            return key in this._activeSettings.remote ? this._activeSettings.remote[key] : defaultValue;
        },
        setValue: function(key, value) {
            key = this.resolve(key);
            var current = this.getValue(key);
            if (current !== value) {
                return this.syncValue(key, value, 'setValue');
            }
            return false;
        },
        syncValue: function(key, value, origin) {
            key = this.resolve(key);
            var oldValue = this._activeSettings.remote[key];
            this.sendEvent({ method: 'valueUpdated', params: [key, value, oldValue, origin] });
            this._activeSettings.sent[key] = this._activeSettings.remote[key] = value;
            this.valueUpdated(key, value, oldValue, (origin ? origin+':' : '') + 'html.bridgedSettings.syncValue');
        },
        getValueAsync: function(key, defaultValue, callback) {
            key = this.resolve(key);
            if (typeof defaultValue === 'function') {
                callback = defaultValue;
                defaultValue = undefined;
            }
            var params = defaultValue !== undefined ? [ key, defaultValue ] : [ key ],
                event = { method: 'Settings.getValue', params: params };

            this.debugPrint('< getValueAsync...', key, params);
            if (callback) {
                event.id = this._callbackId++;
                this.callbacks[event.id] = function(obj) {
                    try {
                        callback(obj.error, obj.result);
                        if (!obj.error) {
                            this._activeSettings.received[key] = this._activeSettings.remote[key] = obj.result;
                        }
                    } finally {
                        delete this.callbacks[event.id];
                    }
                    this.pendingRequestCount() === 0 && this.pendingRequestsFinished();
                };
            }
            this.sendEvent(event);
        },
        setValueAsync: function(key, value, callback) {
            key = this.resolve(key);
            this.log('< setValueAsync', key, value);
            var params = [ key, value ],
                event = { method: 'Settings.setValue', params: params };
            if (callback) {
                event.id = this._callbackId++;
                this.callbacks[event.id] = function(obj) {
                    try {
                        callback(obj.error, obj.result);
                    } finally {
                        delete this.callbacks[event.id];
                    }
                };
            }
            this._activeSettings.sent[key] = this._activeSettings.remote[key] = value;
            this.sendEvent(event);
        }
    };

})(this);
