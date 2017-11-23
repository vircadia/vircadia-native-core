// CustomSettingsApp.js -- manages Settings between a Client script and connected "settings" tablet WebView page
// see browser/BridgedSettings.js for the webView side

// example:
//   var button = tablet.addButton({ text: 'My Settings' });
//   var mySettingsApp = new CustomSettingsApp({
//       namespace: 'mySettingsGroup',
//       url: Script.resolvePath('myapp.html'),
//       uuid: button.uuid,
//       tablet: tablet
//   });
//
//   // settings are automatically sync'd from the web page back to Interface; to be notified when that happens, use:
//   myAppSettings.valueUpdated.connect(function(name, value, oldValue, origin) {
//      print('setting updated from web page', name, value, oldValue, origin);
//   });
//
//   // settings are also automatically sync'd from Interface back to the web page; to manually sync a value, use:
//   myAppSettings.syncValue(fullSettingsKey, value);

/* eslint-env commonjs */
"use strict";

CustomSettingsApp.version = '0.0.0';
module.exports = CustomSettingsApp;

var _utils = require('../_utils.js');
Object.assign = Object.assign || _utils.assign;

function assert(truthy, message) {
    return _utils.assert.call(this, truthy, 'CustomSettingsApp | ' + message);
}

function _debugPrint() {
    print('CustomSettingsApp | ' + [].slice.call(arguments).join(' '));
}
var debugPrint = function() {};

function CustomSettingsApp(options) {
    assert('url' in options, 'expected options.url');
    if (options.debug) {
        debugPrint = _debugPrint;
    }

    this.url = options.url;
    this.namespace = options.namespace || 'BridgedSettings';
    this.uuid = options.uuid || Uuid.generate();
    this.recheckInterval = options.recheckInterval || 1000;

    this.settingsScreenVisible = false;
    this.isActive = false;

    this.extraParams = Object.assign(options.extraParams || {}, {
        customSettingsVersion: CustomSettingsApp.version+'',
        protocolVersion: location.protocolVersion && location.protocolVersion()
    });

    var params = {
        namespace: this.namespace,
        uuid: this.uuid,
        debug: options.debug || undefined
    };

    // encode PARAMS into '?key=value&...'
    var query = Object.keys(params).map(function encodeValue(key) {
        var value = encodeURIComponent(params[key] === undefined ? '' : params[key]);
        return [ key, value ].join('=');
    }).join('&');
    this.url += '?&' + query;

    this.isActiveChanged = _utils.signal(function(isActive) {});
    this.valueUpdated = _utils.signal(function(key, value, oldValue, origin) {});

    this.settingsAPI = options.settingsAPI || Settings;

    // keep track of accessed settings so they can be kept in sync if changed externally
    this._activeSettings = { sent: {}, received: {}, remote: {} };

    if (options.tablet) {
        this._initialize(options.tablet);
    }
}

CustomSettingsApp.prototype = {
    tablet: null,
    resolve: function(key) {
        if (0 === key.indexOf('.') || ~key.indexOf('/')) {
            // key is already qualified under a group; return as-is
            return key;
        }
        // nest under the current namespace
        return [ this.namespace, key ].join('/');
    },
    sendEvent: function(msg) {
        assert(this.tablet, '!this.tablet');
        msg.ns = msg.ns || this.namespace;
        msg.uuid = msg.uuid || this.uuid;
        this.tablet.emitScriptEvent(JSON.stringify(msg));
    },
    getValue: function(key, defaultValue) {
        key = this.resolve(key);
        return key in this._activeSettings.remote ? this._activeSettings.remote[key] : defaultValue;
    },
    setValue: function(key, value) {
        key = this.resolve(key);
        var current = this.getValue(key);
        if (current !== value) {
            return this.syncValue(key, value, 'CustomSettingsApp.setValue');
        }
        return false;
    },
    syncValue: function(key, value, origin) {
        key = this.resolve(key);
        var oldValue = this._activeSettings.remote[key];
        assert(value !== null, 'CustomSettingsApp.syncValue value is null');
        this.sendEvent({ id: 'valueUpdated', params: [key, value, oldValue, origin] });
        this._activeSettings.sent[key] = value;
        this._activeSettings.remote[key] = value;
        this.valueUpdated(key, value, oldValue, (origin ? origin+':' : '') + 'CustomSettingsApp.syncValue');
    },
    onScreenChanged: function onScreenChanged(type, url) {
        this.settingsScreenVisible = (url === this.url);
        debugPrint('===> onScreenChanged', type, url, 'settingsScreenVisible: ' + this.settingsScreenVisible);
        if (this.isActive && !this.settingsScreenVisible) {
            this.isActiveChanged(this.isActive = false);
        }
    },

    _apiGetValue: function(key, defaultValue) {
        // trim rooted keys like "/desktopTabletBecomesToolbar" => "desktopTabletBecomesToolbar"
        key = key.replace(/^\//,'');
        return this.settingsAPI.getValue(key, defaultValue);
    },
    _apiSetValue: function(key, value) {
        key = key.replace(/^\//,'');
        return this.settingsAPI.setValue(key, value);
    },
    _setValue: function(key, value, oldValue, origin) {
        var current = this._apiGetValue(key),
            lastRemoteValue = this._activeSettings.remote[key];
        debugPrint('.setValue(' + JSON.stringify({key: key, value: value, current: current, lastRemoteValue: lastRemoteValue })+')');
        this._activeSettings.received[key] = value;
        this._activeSettings.remote[key] = value;
        var result;
        if (lastRemoteValue !== value) {
            this.valueUpdated(key, value, lastRemoteValue, 'CustomSettingsApp.tablet');
        }
        if (current !== value) {
            result = this._apiSetValue(key, value);
        }
        return result;
    },

    _handleValidatedMessage: function(obj, msg) {
        var tablet = this.tablet;
        if (!tablet) {
            throw new Error('_handleValidatedMessage called when not connected to tablet...');
        }
        var params = Array.isArray(obj.params) ? obj.params : [obj.params];
        var parts = (obj.method||'').split('.'), api = parts[0], method = parts[1];
        switch(api) {
            case 'valueUpdated': obj.result = this._setValue.apply(this, params); break;
            case 'Settings':
                if (method && params[0]) {
                    var key = this.resolve(params[0]), value = params[1];
                    debugPrint('>>>>', method, key, value);
                    switch (method) {
                        case 'getValue':
                            obj.result = this._apiGetValue(key, value);
                            this._activeSettings.sent[key] = obj.result;
                            this._activeSettings.remote[key] = obj.result;
                            break;
                        case 'setValue':
                            obj.result = this._setValue(key, value, params[2], params[3]);
                            break;
                        default:
                            obj.error = 'unmapped Settings method: ' + method;
                            throw new Error(obj.error);
                    }
                    break;
                }
            default: if (this.onUnhandledMessage) {
                this.onUnhandledMessage(obj, msg);
            } else {
                obj.error = 'unmapped method call: ' + msg;
            }
        }
        if (obj.id) {
            // if message has an id, reply with the same message obj which now has a .result or .error field
            // note: a small delay is needed because of an apparent race condition between ScriptEngine and Tablet WebViews
            Script.setTimeout(_utils.bind(this, function() {
                this.sendEvent(obj);
            }), 100);
        } else if (obj.error) {
            throw new Error(obj.error);
        }
    },
    onWebEventReceived: function onWebEventReceived(msg) {
        debugPrint('onWebEventReceived', msg);
        var tablet = this.tablet;
        if (!tablet) {
            throw new Error('onWebEventReceived called when not connected to tablet...');
        }
        if (msg === this.url) {
            if (this.isActive) {
                // user (or page) refreshed the web view; trigger !isActive so client script can perform cleanup
                this.isActiveChanged(this.isActive = false);
            }
            this.isActiveChanged(this.isActive = true);
            // reply to initial HTML page ACK with any extraParams that were specified
            this.sendEvent({ id: 'extraParams', extraParams: this.extraParams });
            return;
        }
        try {
            var obj = assert(JSON.parse(msg));
        } catch (e) {
            return;
        }
        if (obj.ns === this.namespace && obj.uuid === this.uuid) {
            debugPrint('valid onWebEventReceived', msg);
            this._handleValidatedMessage(obj, msg);
        }
    },

    _initialize: function(tablet) {
        if (this.tablet) {
            throw new Error('CustomSettingsApp._initialize called but this.tablet already has a value');
        }
        this.tablet = tablet;
        tablet.webEventReceived.connect(this, 'onWebEventReceived');
        tablet.screenChanged.connect(this, 'onScreenChanged');

        this.onAPIValueUpdated = function(key, newValue, oldValue, origin) {
            if (this._activeSettings.remote[key] !== newValue) {
                _debugPrint('onAPIValueUpdated: ' + key + ' = ' + JSON.stringify(newValue),
                            '(was: ' + JSON.stringify(oldValue) +')');
                this.syncValue(key, newValue, (origin ? origin+':' : '') + 'CustomSettingsApp.onAPIValueUpdated');
            }
        };
        this.isActiveChanged.connect(this, function(isActive) {
            this._activeSettings.remote = {}; // reset assumptions about remote values
            isActive ? this.$startMonitor() : this.$stopMonitor();
        });
        debugPrint('CustomSettingsApp...initialized', this.namespace);
    },

    $syncSettings: function() {
        for (var p in this._activeSettings.sent) {
            var value = this._apiGetValue(p),
                lastValue = this._activeSettings.remote[p];
            if (value !== undefined && value !== null && value !== '' && value !== lastValue) {
                _debugPrint('CustomSettingsApp... detected external settings change', p, value);
                this.syncValue(p, value, 'Settings');
                this.valueUpdated(p, value, lastValue, 'CustomSettingsApp.$syncSettings');
            }
        }
    },
    $startMonitor: function() {
        if (!(this.recheckInterval > 0)) {
            _debugPrint('$startMonitor -- recheckInterval <= 0; not starting settings monitor thread');
            return false;
        }
        if (this.interval) {
            this.$stopMonitor();
        }
        if (this.settingsAPI.valueUpdated) {
            _debugPrint('settingsAPI supports valueUpdated -- binding to detect settings changes', this.settingsAPI);
            this.settingsAPI.valueUpdated.connect(this, 'onAPIValueUpdated');
        }
        this.interval = Script.setInterval(_utils.bind(this, '$syncSettings'), this.recheckInterval);
        _debugPrint('STARTED MONITORING THREAD');
    },
    $stopMonitor: function() {
        if (this.interval) {
            Script.clearInterval(this.interval);
            this.interval = 0;
            if (this.settingsAPI.valueUpdated) {
                this.settingsAPI.valueUpdated.disconnect(this, 'onAPIValueUpdated');
            }
            _debugPrint('stopped monitoring thread');
            return true;
        }
    },

    cleanup: function() {
        if (!this.tablet) {
            return _debugPrint('CustomSettingsApp...cleanup called when not initialized');
        }
        var tablet = this.tablet;
        tablet.webEventReceived.disconnect(this, 'onWebEventReceived');
        tablet.screenChanged.disconnect(this, 'onScreenChanged');
        this.$stopMonitor();
        if (this.isActive) {
            try {
                this.isActiveChanged(this.isActive = false);
            } catch (e) {
                _debugPrint('ERROR: cleanup error during isActiveChanged(false)', e);
            }
        }
        this.toggle(false);
        this.settingsScreenVisible = false;
        this.tablet = null;
        debugPrint('cleanup completed', this.namespace);
    },

    toggle: function(show) {
        if (!this.tablet) {
            return _debugPrint('CustomSettingsApp...cleanup called when not initialized');
        }
        if (typeof show !== 'boolean') {
            show = !this.settingsScreenVisible;
        }

        if (this.settingsScreenVisible && !show) {
            this.tablet.gotoHomeScreen();
        } else if (!this.settingsScreenVisible && show) {
            Script.setTimeout(_utils.bind(this, function() {
                // Interface sometimes crashes if not for adding a small timeout here :(
                this.tablet.gotoWebScreen(this.url);
            }), 1);
        }
    }
};

