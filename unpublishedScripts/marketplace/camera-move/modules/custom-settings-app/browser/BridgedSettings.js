// BridgedSettings.js -- HTML-side implementation of bridged/async Settings
// see ../CustomSettingsApp.js for the corresponding Interface script

/* eslint-env commonjs, browser */
/* eslint-disable comma-dangle, no-empty */

(function(global) {
    "use strict";

    BridgedSettings.version = '0.0.2';

    try {
        module.exports = BridgedSettings;
    } catch (e) {
        global.BridgedSettings = BridgedSettings;
    }

    var _utils = global._utils || (typeof require === 'function' && require('../../_utils.js'));
    if (!_utils || !_utils.signal) {
        throw new Error('html.BridgedSettings.js -- expected _utils to be available on the global object (ie: window._utils)');
    }
    var signal = _utils.signal;

    function log() {
        console.info('bridgedSettings | ' + [].slice.call(arguments).join(' ')); // eslint-disable-line no-console
    }
    log('version', BridgedSettings.version);

    var debugPrint = function() {}; // = log

    function BridgedSettings(options) {
        options = options || {};
        // Note: Interface changed how window.EventBridge behaves again; it now arbitrarily replaces the global value
        // sometime after the initial page load, invaliding any held references to it.
        // As a workaround this proxies the local property to the current global value.
        var _lastEventBridge = global.EventBridge;
        Object.defineProperty(this, 'eventBridge', { enumerable: true, get: function() {
            if (_lastEventBridge !== global.EventBridge) {
                log('>>> EventBridge changed in-flight', '(was: ' + _lastEventBridge + ' | is: ' + global.EventBridge + ')');
                _lastEventBridge = global.EventBridge;
            }
            return global.EventBridge;
        }});
        Object.assign(this, {
            //eventBridge: options.eventBridge || global.EventBridge,
            namespace: options.namespace || 'BridgedSettings',
            uuid: options.uuid || undefined,
            valueReceived: signal(function valueReceived(key, newValue, oldValue, origin){}),
            callbackError: signal(function callbackError(error, message){}),
            pendingRequestsFinished: signal(function pendingRequestsFinished(){}),
            extraParams: options.extraParams || {},
            _hifiValues: {},

            debug: options.debug,
            log: log.bind({}, options.namespace + ' |'),
            debugPrint: function() {
                return this.debug && this.log.apply(this, arguments);
            },
            _boundScriptEventReceived: this.onScriptEventReceived.bind(this),
            callbacks: Object.defineProperties(options.callbacks || {}, {
                extraParams: { value: this.handleExtraParams },
                valueUpdated: { value: this.handleValueUpdated },
            })
        });
        this.log('connecting to EventBridge.scriptEventReceived');
        this.eventBridge.scriptEventReceived.connect(this._boundScriptEventReceived);
    }

    BridgedSettings.prototype = {
        _callbackId: 1,
        toString: function() {
            return '[BridgedSettings namespace='+this.namespace+']';
        },
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
            this._hifiValues[key] = value;
            this.valueReceived(key, value, oldValue, (origin?origin+':':'') + 'callbacks.valueUpdated');
        },
        handleExtraParams: function(msg) {
            // client script sent us extraParams
            var extraParams = msg.extraParams;
            var previousParams = JSON.parse(JSON.stringify(this.extraParams));

            _utils.sortedAssign(this.extraParams, extraParams);

            this._hifiValues['.extraParams'] = this.extraParams;
            this.debugPrint('received .extraParams', JSON.stringify(extraParams,0,2));
            this.valueReceived('.extraParams', this.extraParams, previousParams, 'html.bridgedSettings.handleExtraParams');
        },
        cleanup: function() {
            try {
                this.eventBridge.scriptEventReceived.disconnect(this._boundScriptEventReceived);
            } catch (e) {
                this.log('error disconnecting from scriptEventReceived:', e);
            }
        },
        pendingRequestCount: function() {
            return Object.keys(this.callbacks).length;
        },
        _handleValidatedMessage: function(obj, msg) {
            var callback = this.callbacks[obj.id];
            if (callback) {
                try {
                    return callback.call(this, obj) || true;
                } catch (e) {
                    this.log('CALLBACK ERROR', this.namespace, obj.id, '_onScriptEventReceived', e);
                    this.callbackError(e, obj);
                    if (this.debug) {
                        throw e;
                    }
                }
            } else if (this.onUnhandledMessage) {
                return this.onUnhandledMessage(obj, msg);
            }
        },
        onScriptEventReceived: function(msg) {
            this.debugPrint(this.namespace, '_onScriptEventReceived......' + msg);
            try {
                var obj = JSON.parse(msg);
                var validSender = obj.ns === this.namespace && obj.uuid === this.uuid;
                if (validSender) {
                    return this._handleValidatedMessage(obj, msg);
                } else {
                    debugPrint('xskipping', JSON.stringify([obj.ns, obj.uuid]), JSON.stringify(this), msg);
                }
            } catch (e) {
                log('rpc error:', e, msg);
                return e;
            }
        },
        sendEvent: function(msg) {
            msg.ns = msg.ns || this.namespace;
            msg.uuid = msg.uuid || this.uuid;
            debugPrint('sendEvent', JSON.stringify(msg));
            this.eventBridge.emitWebEvent(JSON.stringify(msg));
        },
        getValue: function(key, defaultValue) {
            key = this.resolve(key);
            return key in this._hifiValues ? this._hifiValues[key] : defaultValue;
        },
        setValue: function(key, value) {
            key = this.resolve(key);
            var current = this.getValue(key);
            if (current !== value) {
                debugPrint('SET VALUE : ' + JSON.stringify({ key: key, current: current, value: value }));
                return this.syncValue(key, value, 'setValue');
            }
            this._hifiValues[key] = value;
            return false;
        },
        syncValue: function(key, value, origin) {
            return this.sendEvent({ method: 'valueUpdated', params: [key, value, this.getValue(key), origin] });
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
                            this._hifiValues[key] = obj.result;
                        }
                    } finally {
                        delete this.callbacks[event.id];
                    }
                    if (this.pendingRequestCount() === 0) {
                        setTimeout(function() {
                            this.pendingRequestsFinished();
                        }.bind(this), 1);
                    }
                };
            }
            this.sendEvent(event);
        },
    };
})(this);
