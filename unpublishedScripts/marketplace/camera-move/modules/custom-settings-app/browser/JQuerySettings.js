// JQuerySettings.js -- HTML-side helper class for managing settings-linked jQuery UI elements

/* eslint-env jquery, commonjs, browser */
/* eslint-disable comma-dangle, no-empty */
/* global assert, log */
// ----------------------------------------------------------------------------
(function(global) {
    "use strict";

    JQuerySettings.version = '0.0.0';

    try {
        module.exports = JQuerySettings;
    } catch (e) {
        global.JQuerySettings= JQuerySettings;
    }

    var _utils = global._utils || (typeof require === 'function' && require('../../_utils.js'));

    if (!_utils || !_utils.signal) {
        throw new Error('html.JQuerySettings.js -- expected _utils to be available on the global object (ie: window._utils)'+module);
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
            id2Setting: {}, // DOM id -> qualified Settings key
            Setting2id: {}, // qualified Settings key -> DOM id
            observers: {},  // DOM MutationObservers
            mutationEvent: signal(function mutationEvent(event) {}),
            boundOnDOMMutation: this._onDOMMutation.bind(this),
        }, options);
    }
    JQuerySettings.idCounter = 0;
    JQuerySettings.prototype = {
        toString: function() {
            return '[JQuerySettings namespace='+this.namespace+']';
        },
        mutationConfig: {
            attributes: true,
            attributeOldValue: true,
            attributeFilter: [ 'value', 'checked', 'data-checked', 'data-value' ]
        },
        _onDOMMutation: function(mutations, observer) {
            mutations.forEach(function(mutation, index) {
                var target = mutation.target,
                    targetId = target.dataset['for'] || target.id,
                    domId = target.id,
                    attrValue = target.getAttribute(mutation.attributeName),
                    hifiType = target.dataset.hifiType,
                    value = hifiType ? $(target)[hifiType]('instance').value() : attrValue,
                    oldValue = mutation.oldValue;
                var event = {
                    key:       this.getKey(targetId, true) || this.getKey(domId),
                    value:     value,
                    oldValue:  oldValue,
                    hifiType:  hifiType,
                    domId:     domId,
                    domType:   target.getAttribute('type') || target.type,
                    targetId:  targetId,
                    attrValue: attrValue,
                    domName:   target.name,
                    type:      mutation.type,
                };

                switch (typeof value) {
                    case 'boolean': event.oldValue = !!event.oldValue; break;
                    case 'number':
                        var tmp = parseFloat(oldValue);
                        if (isFinite(tmp)) {
                            event.oldValue = tmp;
                        }
                        break;
                }

                return (event.oldValue === event.value) ?
                    debugPrint('SKIP NON-MUTATION', event.key, event.hifiType) :
                    this.mutationEvent(event);
            }.bind(this));
        },
        observeNode: function(node) {
            assert(node.id);
            var observer = this.observers[node.id];
            if (!observer) {
                observer = new MutationObserver(this.boundOnDOMMutation);
                observer.observe(node, this.mutationConfig);
                this.observers[node.id] = observer;
            }
            debugPrint('observeNode', node.id, node.dataset.hifiType, node.name);
            return observer;
        },
        resolve: function(key) {
            assert(typeof key === 'string');
            if (0 !== key.indexOf('.') && !~key.indexOf('/')) {
                return [ this.namespace, key ].join('/');
            } else {
                return key;
            }
        },
        registerSetting: function(id, key) {
            assert(id, 'registerSetting -- invalid id: ' + id + ' for key:' + key);
            this.id2Setting[id] = key;
            if (!(key in this.Setting2id)) {
                this.Setting2id[key] = id;
            } else {
                key = null;
            }
            debugPrint('JQuerySettings.registerSetting -- registered: ' + JSON.stringify({ id: id, key: key }));
        },
        registerNode: function(node) {
            var element = $(node),
                target = element.data('for') || element.attr('for') || element.prop('id');
            assert(target, 'registerNode could determine settings target: ' + node.outerHTML);
            if (!node.id) {
                node.id = ['id', target.replace(/[^-\w]/g,'-'), JQuerySettings.idCounter++ ].join('-');
            }
            var key = node.dataset['key'] = this.resolve(target);
            this.registerSetting(node.id, key);

            debugPrint('registerNode', node.id, target, key);
            // return this.observeNode(node);
        },
        // lookup the DOM id for a given Settings key
        getId: function(key, missingOk) {
            key = this.resolve(key);
            assert(missingOk || function assertion(){
                return typeof key === 'string';
            });
            if (key in this.Setting2id || missingOk) {
                return this.Setting2id[key];
            }
            log('WARNING: jquerySettings.getId: !Setting2id['+key+'] ' + this.Setting2id[key], key in this.Setting2id);
        },
        getAllNodes: function() {
            return Object.keys(this.Setting2id)
                .map(function(key) {
                    return this.findNodeByKey(key);
                }.bind(this))
                .filter(function(node) {
                    return node.type !== 'placeholder';
                }).filter(Boolean);
        },
        // lookup the Settings key for a given DOM id
        getKey: function(id, missingOk) {
            if ((id in this.id2Setting) || missingOk) {
                return this.id2Setting[id];
            }
            log('WARNING: jquerySettings.getKey: !id2Setting['+id+']');
        },
        // lookup the DOM node for a given Settings key
        findNodeByKey: function(key, missingOk) {
            key = this.resolve(key);
            var id = this.getId(key, missingOk);
            var node = typeof id === 'object' ? id : document.getElementById(id);
            if (node || missingOk) {
                return node;
            }
            log('WARNING: jquerySettings.findNodeByKey -- node not found:', 'key=='+key, 'id=='+id);
        },
        getValue: function(key, defaultValue) {
            return this.getNodeValue(this.findNodeByKey(key));
        },
        setValue: function(key, value, origin) {
            return this.setNodeValue(this.findNodeByKey(key), value, origin || 'setValue');
        },
        getNodeValue: function(node) {
            assert(node && typeof node === 'object', 'getNodeValue expects a DOM node');
            node = node.jquery ? node.get(0) : node;
            if (node.type === 'placeholder') {
                return node.value;
            }
            assert(node.dataset.hifiType);
            return $(node)[node.dataset.hifiType]('instance').value();
        },
        setNodeValue: function(node, newValue) {
            assert(node, 'JQuerySettings::setNodeValue -- invalid node:' + node);
            node = node.jquery ? node[0] : node;
            if (node.type === 'placeholder') {
                return node.value = newValue;
            }
            var hifiType = assert(node.dataset.hifiType);
            return $(node)[hifiType]('instance').value(newValue);
        },
    };
})(this);
