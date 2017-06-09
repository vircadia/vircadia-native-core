// JQuerySettings.js -- HTML-side helper class for managing settings-linked jQuery UI elements

/* eslint-env commonjs, browser */
(function(global) {
    "use strict";

    JQuerySettings.version = '0.0.0';

    try {
        module.exports = JQuerySettings;
    } catch (e) {
        global.JQuerySettings= JQuerySettings
    }

    var _utils = global._utils || (global.require && global.require('../_utils.js'));

    if (!_utils || !_utils.signal) {
        throw new Error('html.BridgedSettings.js -- expected _utils to be available on the global object (ie: window._utils)');
    }
    var signal = _utils.signal,
        assert = _utils.assert;

    function log() {
        console.info('jquerySettings | ' + [].slice.call(arguments).join(' ')); // eslint-disable-line no-console
    }
    log('version', JQuerySettings.version);

    var debugPrint = function() {}; // = log

    function JQuerySettings(options) {
        assert('namespace' in options);
        Object.assign(this, {
            id2Setting: {},   // DOM id -> qualified Settings key
            Setting2id: {},   // qualified Settings key -> DOM id
            _activeSettings: {
                received: {}, // from DOM elements
                sent: {},     // to DOM elements
                remote: {},   // MRU values
            },

        }, options);
        this.valueUpdated = signal(function valueUpdated(key, value, oldValue, origin){});
    }
    JQuerySettings.prototype = {
        resolve: function(key) {
            if (0 !== key.indexOf('.') && !~key.indexOf('/')) {
                return [ this.namespace, key ].join('/');
            } else {
                return key;
            }
        },
        registerSetting: function(id, key) {
            this.id2Setting[id] = key;
            this.Setting2id[key] = id;
        },
        registerNode: function(node) {
            var name = node.name || node.id,
                key = this.resolve(name);
            this.registerSetting(node.id, key);
            if (node.type === 'radio') {
                // for radio buttons also map the overall radio-group to the key
                this.registerSetting(name, key);
            }
        },
        // lookup the DOM id for a given Settings key
        getId: function(key, missingOk) {
            key = this.resolve(key);
            return assert(this.Setting2id[key] || missingOk || true, 'jquerySettings.getId: !Setting2id['+key+']');
        },
        // lookup the Settings key for a given DOM id
        getKey: function(id, missingOk) {
            return assert(this.id2Setting[id] || missingOk || true, 'jquerySettings.getKey: !id2Setting['+id+']');
        },
        // lookup the DOM node for a given Settings key
        findNodeByKey: function(key, missingOk) {
            key = this.resolve(key);
            var id = this.getId(key, missingOk);
            var node = document.getElementById(id);
            if (typeof node !== 'object') {
                log('jquerySettings.getNodeByKey -- node not found:', 'key=='+key, 'id=='+id);
            }
            return node;
        },
        _notifyValueUpdated: function(key, value, oldValue, origin) {
            this._activeSettings.sent[key] = value;
            this._activeSettings.remote[key] = value;
            this.valueUpdated(this.resolve(key), value, oldValue, origin);
        },
        getValue: function(key, defaultValue) {
            key = this.resolve(key);
            var node = this.findNodeByKey(key);
            if (node) {
                var value = this.__getNodeValue(node);
                this._activeSettings.remote[key] = value;
                this._activeSettings.received[key] = value;
                return value;
            }
            return defaultValue;
        },
        setValue: function(key, value, origin) {
            key = this.resolve(key);
            var lastValue = this.getValue(key, value);
            if (lastValue !== value || origin) {
                var node = assert(this.findNodeByKey(key), 'jquerySettings.setValue -- node not found: ' + key);
                var ret = this.__setNodeValue(node, value);
                if (origin) {
                    this._notifyValueUpdated(key, value, lastValue, origin || 'jquerySettings.setValue');
                }
                return ret;
            }
        },
        resyncValue: function(key, origin) {
            var sentValue = key in this._activeSettings.sent ? this._activeSettings.sent[key] : null,
                receivedValue = key in this._activeSettings.received ? this._activeSettings.received[key] : null,
                currentValue = this.getValue(key, null);

            log('resyncValue', JSON.stringify({
                key: key, current: currentValue, sent: sentValue, received: receivedValue,
                'current !== received': [typeof currentValue, currentValue+'', typeof receivedValue, receivedValue+'', currentValue !== receivedValue],
                'current !== sent': [currentValue+'', sentValue+'', currentValue !== sentValue],
            }));

            if (currentValue !== receivedValue || currentValue !== sentValue) {
                this._notifyValueUpdated(key, currentValue, sentValue, (origin?origin+':':'')+'jquerySettings.resyncValue');
            }
        },

        domAccessors: {
            'default': {
                get: function() { return this.value; },
                set: function(nv) { $(this).val(nv); },
            },
            'checkbox': {
                get: function() { return this.checked; },
                set: function(nv) { $(this).prop('checked', nv); },
            },
            'number': {
                get: function() { return parseFloat(this.value); },
                set: function(nv) {
                    var step = this.step || 1, precision = (1/step).toString(this).length - 1;
                    var value = parseFloat(newValue).toFixed(precision);
                    if (isNaN(value)) {
                        log('domAccessors.number.set', id, 'ignoring NaN value:', value+'');
                        return;
                    }
                    $(this).val(value);
                },
            },
            'radio': {
                get: function() { assert(false, 'use radio-group to get current selected radio value for ' + this.id); },
                set: function(nv) {},
            },
            'radio-group': {
                get: function() {
                    var checked = $(this).closest(':ui-hifiControlGroup').find('input:checked');
                    debugPrint('_getthisValue.radio-group checked item: ' + checked[0], checked.val());
                    return checked.val();
                },
                set: function(nv) {

                },
            },
        },

        __getNodeValue: function(node) {
            assert(node && typeof node === 'object', '__getNodeValue expects a DOM node');
            node = node.jquery ? node.get(0) : node;
            var type = node ? (node.dataset.type || node.type) : undefined;
            var value;
            assert(type in this.domAccessors, 'DOM value accessor not defined for node type: ' + type);
            debugPrint('__getNodeValue', type);
            return this.domAccessors[type].get.call(node);

            // switch(node.type) {
            //     case 'checkbox': value = node.checked; break;
            //     case 'number': value = parseFloat(node.value); break;
            //     case 'radio':
            //     case 'radio-group':
            //         var checked = $(node).closest(':ui-hifiControlGroup').find('input:checked');
            //         debugPrint('_getNodeValue.radio-group checked item: ' + checked[0], checked.val());
            //         value = checked.val();
            //         break;
            //     default: value = node.value;
            // }
            // debugPrint('_getNodeValue', node, node.id, node.type, node.type !== 'radio-group' && node.value, value);
            // return value;
        },

        __setNodeValue: function(node, newValue) {
            if (node && node.jquery) {
                node = node[0];
            }
            var id = node.id,
                key = this.getKey(node.id),
                type = node.dataset.type || node.type,
                value = newValue,
                element = $(node);

            debugPrint('__setNodeValue', '('+type+') #' + key + '=' + newValue);

            switch(type) {
                case 'radio':
                    assert(false, 'radio buttons should be set through their radio-group parent');
                    break;
                case 'checkbox':
                    element.prop('checked', newValue);
                    element.is(':ui-hifiCheckboxRadio') && element.hifiCheckboxRadio('refresh');
                    break;
                case 'radio-group':
                    var input = element.find('input[type=radio]#' + newValue);
                    assert(input[0], 'ERROR: ' + key + ': could not find "input[type=radio]#' + newValue + '" to set new radio-group value of: ' + newValue);
                    input.prop('checked', true);
                    input.closest(':ui-hifiControlGroup').find(':ui-hifiCheckboxRadio').hifiCheckboxRadio('refresh');
                    break;
                case 'number':
                    var step = node.step || 1, precision = (1/step).toString().length - 1;
                    value = parseFloat(newValue).toFixed(precision);
                    if (isNaN(value)) {
                        log(id, 'ignoring NaN value');
                        break;
                    }
                    element.val(value);
                    element.closest('.row').find(':ui-hifiSlider').hifiSlider('value', parseFloat(value));
                    break;
                default:
                    element.val(newValue);
            }
            return value;
        }
    };
})(this);
