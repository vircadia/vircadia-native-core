// config-utils.js -- helpers for coordinating Application runtime vs. Settings configuration
//
//   * ApplicationConfig -- Menu items and API values.
//   * SettingsConfig -- scoped Settings values.

"use strict";
/* eslint-env commonjs */
/* global log */
module.exports = {
    version: '0.0.1a',
    ApplicationConfig: ApplicationConfig,
    SettingsConfig: SettingsConfig
};

var _utils = require('./_utils.js'),
    assert = _utils.assert;
Object.assign = Object.assign || _utils.assign;

function _debugPrint() {
    print('config-utils | ' + [].slice.call(arguments).join(' '));
}

var debugPrint = function() {};

// ----------------------------------------------------------------------------
// Application-specific configuration values using runtime state / API props
//
// options.config[] supports the following item formats:
//   'settingsName': { menu: 'Menu > MenuItem'}, // assumes MenuItem is a checkbox / checkable value
//   'settingsName': { object: [ MyAvatar, 'property' ] },
//   'settingsName': { object: [ MyAvatar, 'getterMethod', 'setterMethod' ] },
//   'settingsName': { menu: 'Menu > MenuItem', object: [ MyAvatar, 'property' ] },
//   'settingsName': { get: function getter() { ...}, set: function(nv) { ... } },

function ApplicationConfig(options) {
    options = options || {};
    assert('namespace' in options && 'config' in options);
    if (options.debug) {
        debugPrint = _debugPrint;
        debugPrint('debugPrinting enabled');
    }
    this.namespace = options.namespace;
    this.valueUpdated = _utils.signal(function valueUpdated(key, newValue, oldValue, origin){});

    this.config = {};
    this.register(options.config);
}
ApplicationConfig.prototype = {
    resolve: function resolve(key) {
        assert(typeof key === 'string', 'ApplicationConfig.resolve error: key is not a string: ' + key);
        if (0 !== key.indexOf('.') && !~key.indexOf('/')) {
            key = [ this.namespace, key ].join('/');
        }
        return (key in this.config) ? key : (debugPrint('ApplicationConfig -- could not resolve key: ' + key),undefined);
    },
    registerItem: function(settingName, item) {
        item._settingName = settingName;
        item.settingName = ~settingName.indexOf('/') ? settingName : [ this.namespace, settingName ].join('/');
        return this.config[item.settingName] = this.config[settingName] = new ApplicationConfigItem(item);
    },
    // process items into fully-qualfied ApplicationConfigItem instances
    register: function(items) {
        for (var p in items) {
            var item = items[p];
            item && this.registerItem(p, item);
        }
    },
    _getItem: function(key) {
        return this.config[this.resolve(key)];
    },
    getValue: function(key, defaultValue) {
        var item = this._getItem(key);
        if (!item) {
            return defaultValue;
        }
        return item.get();
    },
    setValue: function setValue(key, value) {
        key = this.resolve(key);
        var lastValue = this.getValue(key, value);
        var ret = this._getItem(key).set(value);
        if (lastValue !== value) {
            this.valueUpdated(key, value, lastValue, 'ApplicationConfig.setValue');
        }
        return ret;
    },
    // sync dual-source (ie: Menu + API) items
    resyncValue: function(key) {
        var item = this._getItem(key);
        return item && item.resync();
    },
    // sync Settings values -> Application state
    applyValue: function applyValue(key, value, origin) {
        if (this.resolve(key)) {
            var appValue = this.getValue(key, value);
            debugPrint('applyValue', key, value, origin ? '['+origin+']' : '', appValue);
            if (appValue !== value) {
                this.setValue(key, value);
                debugPrint('applied new setting', key, value, '(was:'+appValue+')');
                return true;
            }
        }
    }
};

// ApplicationConfigItem represents a single API/Menu item accessor
function ApplicationConfigItem(item) {
    Object.assign(this, item);
    Object.assign(this, {
        _item: item.get && item,
        _object: this._parseObjectConfig(this.object),
        _menu: this._parseMenuConfig(this.menu)
    });
    this.authority = this._item ? 'item' : this._object ? 'object' : this._menu ? 'menu' : null;
    this._authority = this['_'+this.authority];
    debugPrint('_authority', this.authority, this._authority, Object.keys(this._authority));
    assert(this._authority, 'expected item.get, .object or .menu definition; ' + this.settingName);
}
ApplicationConfigItem.prototype = {
    resync: function resync() {
        var authoritativeValue = this._authority.get();
        if (this._menu && this._menu.get() !== authoritativeValue) {
            _debugPrint(this.settingName, this._menu.menuItem,
                        '... menu value ('+this._menu.get()+') out of sync;',
                        'setting to authoritativeValue ('+authoritativeValue+')');
            this._menu.set(authoritativeValue);
        }
        if (this._object && this._object.get() !== authoritativeValue) {
            _debugPrint(this.settingName, this._object.getter || this._object.property,
                        '... object value ('+this._object.get()+') out of sync;',
                        'setting to authoritativeValue ('+authoritativeValue+')');
            this._object.set(authoritativeValue);
        }
    },
    toString: function() {
        return '[ApplicationConfigItem ' + [
            'setting:' + JSON.stringify(this.settingName),
            'authority:' + JSON.stringify(this.authority),
            this._object && 'object:' + JSON.stringify(this._object.property || this._object.getter),
            this._menu && 'menu:' + JSON.stringify(this._menu.menu)
        ].filter(Boolean).join(' ') + ']';
    },
    get: function get() {
        return this._authority.get();
    },
    set: function set(nv) {
        this._object && this._object.set(nv);
        this._menu && this._menu.set(nv);
        return nv;
    },
    _raiseError: function(errorMessage) {
        if (this.debug) {
            throw new Error(errorMessage);
        } else {
            _debugPrint('ERROR: ' + errorMessage);
        }
    },
    _parseObjectConfig: function(parts) {
        if (!Array.isArray(parts) || parts.length < 2) {
            return null;
        }
        var object = parts[0], getter = parts[1], setter = parts[2];
        if (typeof object[getter] === 'function' && typeof object[setter] === 'function') {
            // [ API, 'getter', 'setter' ]
            return {
                object: object, getter: getter, setter: setter,
                get: function getObjectValue() {
                    return this.object[this.getter]();
                },
                set: function setObjectValue(nv) {
                    return this.object[this.setter](nv), nv;
                }
            };
        } else if (getter in object) {
            // [ API, 'property' ]
            return {
                object: object, property: getter,
                get: function() {
                    return this.object[this.property];
                },
                set: function(nv) {
                    return this.object[this.property] = nv;
                }
            };
        }
        this._raiseError('{ object: [ Object, getterOrPropertyName, setterName ] } -- invalid params or does not exist: ' +
                         [ this.settingName, this.object, getter, setter ].join(' | '));
    },
    _parseMenuConfig: function(menu) {
        if (!menu || typeof menu !== 'string') {
            return null;
        }
        var parts = menu.split(/\s*>\s*/), menuItemName = parts.pop(), menuName = parts.join(' > ');
        if (menuItemName && Menu.menuItemExists(menuName, menuItemName)) {
            return {
                menu: menu, menuName: menuName, menuItemName: menuItemName,
                get: function() {
                    return Menu.isOptionChecked(this.menuItemName);
                },
                set: function(nv) {
                    return Menu.setIsOptionChecked(this.menuItemName, nv), nv;
                }
            };
        }
        this._raiseError('{ menu: "Menu > Item" } structure -- invalid params or does not exist: ' +
                         [ this.settingName, this.menu, menuName, menuItemName ].join(' | '));
    }
}; // ApplicationConfigItem.prototype

// ----------------------------------------------------------------------------
// grouped configuration using the Settings.* API
function SettingsConfig(options) {
    options = options || {};
    assert('namespace' in options);
    this.namespace = options.namespace;
    this.defaultValues = {};
    this.valueUpdated = _utils.signal(function valueUpdated(key, newValue, oldValue, origin){});
    if (options.defaultValues) {
        Object.keys(options.defaultValues)
            .forEach(_utils.bind(this, function(key) {
                var fullSettingsKey = this.resolve(key);
                this.defaultValues[fullSettingsKey] = options.defaultValues[key];
            }));
    }
}
SettingsConfig.prototype = {
    resolve: function(key) {
        assert(typeof key === 'string', 'SettingsConfig.resolve error: key is not a string: ' + key);
        return (0 !== key.indexOf('.') && !~key.indexOf('/')) ?
            [ this.namespace, key ].join('/') : key;
    },
    getValue: function(key, defaultValue) {
        key = this.resolve(key);
        defaultValue = defaultValue === undefined ? this.defaultValues[key] : defaultValue;
        return Settings.getValue(key, defaultValue);
    },
    setValue: function setValue(key, value) {
        key = this.resolve(key);
        var lastValue = this.getValue(key);
        var ret = Settings.setValue(key, value);
        if (lastValue !== value) {
            this.valueUpdated(key, value, lastValue, 'SettingsConfig.setValue');
        }
        return ret;
    },
    getFloat: function getFloat(key, defaultValue) {
        key = this.resolve(key);
        defaultValue = defaultValue === undefined ? this.defaultValues[key] : defaultValue;
        var value = parseFloat(this.getValue(key, defaultValue));
        return isFinite(value) ? value : isFinite(defaultValue) ? defaultValue : 0.0;
    }
};
