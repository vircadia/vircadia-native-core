// _utils.js -- misc. helper classes/functions

"use strict";
/* eslint-env commonjs, hifi */
/* eslint-disable comma-dangle, no-empty */
/* global HIRES_CLOCK, Desktop, OverlayWebWindow */
// var HIRES_CLOCK = (typeof Window === 'object' && Window && Window.performance) && Window.performance.now;
var USE_HIRES_CLOCK = typeof HIRES_CLOCK === 'function';

var exports = {
    version: '0.0.1c' + (USE_HIRES_CLOCK ? '-hires' : ''),
    bind: bind,
    signal: signal,
    assign: assign,
    sortedAssign: sortedAssign,
    sign: sign,
    assert: assert,
    makeDebugRequire: makeDebugRequire,
    DeferredUpdater: DeferredUpdater,
    KeyListener: KeyListener,
    getRuntimeSeconds: getRuntimeSeconds,
    createAnimationStepper: createAnimationStepper,
    reloadClientScript: reloadClientScript,

    normalizeStackTrace: normalizeStackTrace,
    BrowserUtils: BrowserUtils,
};
try {
    module.exports = exports; // Interface / Node.js
} catch (e) {
    this._utils = assign(this._utils || {}, exports); // browser
}

// ----------------------------------------------------------------------------
function makeDebugRequire(relativeTo) {
    return function boundDebugRequire(id) {
        return debugRequire(id, relativeTo);
    };
}
function debugRequire(id, relativeTo) {
    if (typeof Script === 'object') {
        relativeTo = (relativeTo||Script.resolvePath('.')).replace(/\/+$/, '');
        // hack-around for use during local development / testing that forces every require to re-fetch the script from the server
        var modulePath = Script._requireResolve(id, relativeTo+'/') + '?' + new Date().getTime().toString(36);
        print('========== DEBUGREQUIRE:' + modulePath);
        Script.require.cache[modulePath] = Script.require.cache[id] = undefined;
        Script.require.__qt_data__[modulePath] = Script.require.__qt_data__[id] = true;
        return Script.require(modulePath);
    } else {
        return require(id);
    }
}

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


// ----------------------------------------------------------------------------
function sign(x) {
    x = +x;
    if (x === 0 || isNaN(x)) {
        return Number(x);
    }
    return x > 0 ? 1 : -1;
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

// hack to sort keys in v8 for prettier JSON exports
function sortedAssign(target, sources) {
    var allParams = assign.apply(this, [{}].concat([].slice.call(arguments)));
    for (var p in target) {
        delete target[p];
    }
    Object.keys(allParams).sort(function(a,b) {
        function swapCase(ch) {
            return /[A-Z]/.test(ch) ? ch.toLowerCase() : ch.toUpperCase();
        }
        a = a.replace(/^./, swapCase);
        b = b.replace(/^./, swapCase);
        a = /Version/.test(a) ? 'AAAA'+a : a;
        b = /Version/.test(b) ? 'AAAA'+b : b;
        return a < b ? -1 : a > b ? 1 : 0;
    }).forEach(function(key) {
        target[key] = allParams[key];
    });
    return target;
}

// ----------------------------------------------------------------------------
// @function - bind a function to a `this` context
// @param {Object} - the `this` context
// @param {Function|String} - function or method name
bind.debug = true;
function bind(thiz, method) {
    var methodName = typeof method === 'string' ? method : method.name;
    method = thiz[method] || method;
    if (bind.debug && methodName) {
        methodName = methodName.replace(/[^A-Za-z0-9_$]/g, '_');
        var debug = {};
        debug[methodName] = method;
        return eval('1,function bound'+methodName+'() { return debug.'+methodName+'.apply(thiz, arguments); }');
    }
    return function() {
        return method.apply(thiz, arguments);
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
// ----------------------------------------------------------------------------
function normalizeStackTrace(err, options) {
    options = options || {};
    // * Chromium:   "  at <anonymous> (file://.../filename.js:45:65)"
    // * Interface:  "  at <anonymous() file://.../filename.js:45"
    // * normalized: "  at <anonymous> filename.js:45"
    var output = err.stack ? err.stack.replace(
            /((?:https?|file):[/][/].*?):(\d+)(?::\d+)?([)]|\s|$)/g,
        function(_, url, lineNumber, suffix) {
            var fileref = url.split(/[?#]/)[0].split('/').pop();
            if (Array.isArray(options.wrapFilesWith)) {
                fileref = options.wrapFilesWith[0] + fileref + options.wrapFilesWith[1];
            }
            if (Array.isArray(options.wrapLineNumbersWith)) {
                lineNumber = options.wrapLineNumbersWith[0] + lineNumber + options.wrapLineNumbersWith[1];
            }
            return fileref + ':' + lineNumber + suffix;
        }
    ).replace(/[(]([-\w.%:]+[.](?:html|js))[)]/g, '$1') : err.message;
    return '    '+output;
}

// utilities specific to use from web browsers / embedded Interface web windows
function BrowserUtils(global) {
    global = global || (1,eval)('this');
    return {
        global: global,
        console: global.console,
        log: function(msg) { return this.console.log('browserUtils | ' + [].slice.call(arguments).join(' ')); },
        makeConsoleWorkRight: function(console, forcePatching) {
            if (console.$patched || !(forcePatching || global.qt)) {
                return console;
            }
            var patched = ['log','debug','info','warn','error'].reduce(function(output, method) {
                output[method] = function() {
                    return console[method]([].slice.call(arguments).join(' '));
                };
                return output;
            }, { $patched: console });
            for (var p in console) {
                if (typeof console[p] === 'function' && !(p in patched)) {
                    patched[p] = console[p].bind(console);
                }
            }
            patched.__proto__ = console; // let scope chain find constants and other non-function values
            return patched;
        },
        parseQueryParams: function(querystring) {
            return this.extendWithQueryParams({}, querystring);
        },
        extendWithQueryParams: function(obj, querystring) {
            querystring = querystring || global.location.href;
            querystring.replace(/\b(\w+)=([^&?#]+)/g, function(_, key, value) {
                value = unescape(value);
                obj[key] = value;
            });
            return obj;
        },
        // openEventBridge handles the cluster of scenarios Interface has imposed on webviews for making EventBridge connections
        openEventBridge: function openEventBridge(callback) {
            this.log('openEventBridge |', 'typeof global.EventBridge == ' + [typeof global.EventBridge, global.EventBridge ]);
            var error;
            try {
                global.EventBridge.toString = function() { return '[global.EventBridge at startup]'; };
                global.EventBridge.scriptEventReceived.connect.exists;
                // this.log('openEventBridge| EventBridge already exists... -- invoking callback', 'typeof EventBridge == ' + typeof global.EventBridge);
                try {
                    return callback(global.EventBridge);
                } catch(e) {
                    error = e;
                }
            } catch (e) {
                this.log('EventBridge not found in a usable state -- attempting to instrument via qt.webChannelTransport',
                         Object.keys(global.EventBridge||{}));
                var QWebChannel = assert(global.QWebChannel, 'expected global.QWebChannel to exist'),
                    qt = assert(global.qt, 'expected global.qt to exist');
                assert(qt.webChannelTransport, 'expected global.qt.webChannelTransport to exist');
                new QWebChannel(qt.webChannelTransport, bind(this, function (channel) {
                    var objects = channel.objects;
                    if (global.EventBridge) {
                        log('>>> global.EventBridge was unavailable at page load, but has spontaneously  materialized; ' +
                            [ typeof global.EventBridge, global.EventBridge ]);
                    }
                    var eventBridge = objects.eventBridge || (objects.eventBridgeWrapper && objects.eventBridgeWrapper.eventBridge);
                    eventBridge.toString = function() { return '[window.EventBridge per QWebChannel]'; };
                    assert(!global.EventBridge || global.EventBridge === eventBridge, 'global.EventBridge !== QWebChannel eventBridge\n' +
                           [global.EventBridge, eventBridge]);
                    global.EventBridge = eventBridge;
                    global.EventBridge.$WebChannel = channel;
                    this.log('openEventBridge opened -- invoking callback', 'typeof EventBridge === ' + typeof global.EventBridge);
                    callback(global.EventBridge);
                }));
            }
            if (error) {
                throw error;
            }
        },
    };
}
// ----------------------------------------------------------------------------
// queue property/method updates to target so that they can be applied all-at-once
function DeferredUpdater(target, options) {
    options = options || {};
    // define _meta as a non-enumerable (so it doesn't show up in for (var p in ...) loops)
    Object.defineProperty(this, '_meta', { enumerable: false, value: {
        target: target,
        lastValue: {},
        dedupe: options.dedupe,
    }});
}
DeferredUpdater.prototype = {
    reset: function() {
        var self = this;
        Object.keys(this).forEach(function(property) {
            delete self[property];
        });
        this._meta.lastValue = {};
    },
    submit: function() {
        var meta = this._meta,
            target = meta.target,
            self = this,
            submitted = {};
        self.submit = getRuntimeSeconds();
        Object.keys(self).forEach(function(property) {
            var newValue = self[property];
            submitted[property] = newValue;
            if (typeof target[property] === 'function') {
                target[property](newValue);
            } else {
                target[property] = newValue;
            }
            delete self[property];
        });
        return submitted;
    }
};
// create a group of deferred updaters eg: DeferredUpdater.createGroup({ MyAvatar: MyAvatar, Camera: Camera })
DeferredUpdater.createGroup = function(items, options) {
    var result = {
        __proto__: {
            reset: function() {
                Object.keys(this).forEach(bind(this, function(item) {
                    this[item].reset();
                }));
            },
            submit: function() {
                var submitted = {};
                Object.keys(this).forEach(bind(this, function(item) {
                    submitted[item] = this[item].submit();
                }));
                return submitted;
            }
        }
    };
    Object.keys(items).forEach(function(item) {
        result[item] = new DeferredUpdater(items[item], options);
    });
    return result;
};

// ----------------------------------------------------------------------------

// session runtime in seconds
getRuntimeSeconds.EPOCH = getRuntimeSeconds(0);
function getRuntimeSeconds(since) {
    since = since === undefined ? getRuntimeSeconds.EPOCH : since;
    var now = USE_HIRES_CLOCK ? new HIRES_CLOCK() : +new Date;
    return ((now / 1000.0) - since);
}

// requestAnimationFrame emulation
function createAnimationStepper(options) {
    options = options || {};
    var fps = options.fps || 30,
        waitMs = 1000 / fps,
        getTime = options.getRuntimeSeconds || getRuntimeSeconds,
        lastUpdateTime = -1e-6,
        timeout = 0;

    requestAnimationFrame.fps = fps;
    requestAnimationFrame.reset = function() {
        if (timeout) {
            Script.clearTimeout(timeout);
            timeout = 0;
        }
    };

    function requestAnimationFrame(update) {
        requestAnimationFrame.reset();
        timeout = Script.setTimeout(function() {
            timeout = 0;
            update(getTime(lastUpdateTime));
            lastUpdateTime = getTime();
        }, waitMs );
    }

    return requestAnimationFrame;
}

// ----------------------------------------------------------------------------
// KeyListener provides a scoped wrapper where options.onKeyPressEvent gets
//   called when a key event matches the specified event.text / key spec
// example: var listener = new KeyListener({ text: 'SPACE', isShifted: false, onKeyPressEvent: function(event) { ... } });
//    Script.scriptEnding.connect(listener, 'disconnect');
function KeyListener(options) {
    assert(typeof options === 'object' && 'text' in options && 'onKeyPressEvent' in options);

    this._options = options;
    assign(this, {
        modifiers: this._getEventModifiers(options, true)
    }, options);
    this.log = options.log || function log() {
        print('KeyListener | ', [].slice.call(arguments).join(' '));
    };
    this.log('created KeyListener', JSON.stringify(this.text), this.modifiers);
    this.connect();
}
KeyListener.prototype = {
    _getEventModifiers: function(event, trueOnly) {
        return '(' + [ 'Control', 'Meta', 'Alt', 'Super', 'Menu', 'Shifted' ].map(function(mod) {
            var isMod = 'is' + mod,
                value = event[isMod],
                found = (trueOnly ? value : typeof value === 'boolean');
            return found && isMod + ' == ' + value;
        }).filter(Boolean).sort().join(' | ') + ')';
    },
    handleEvent: function(event, target) {
        if (event.text === this.text) {
            var modifiers = this._getEventModifiers(event, true);
            if (modifiers !== this.modifiers) {
                return;
            }
            return this[target](event);
        }
    },
    connect: function() {
        return this.$bindEvents(true);
    },
    disconnect: function() {
        return this.$bindEvents(false);
    },
    $onKeyPressEvent: function(event) {
        return this.handleEvent(event, 'onKeyPressEvent');
    },
    $onKeyReleaseEvent: function(event) {
        return this.handleEvent(event, 'onKeyReleaseEvent');
    },
    $bindEvents: function(connect) {
        if (this.onKeyPressEvent) {
            Controller.keyPressEvent[connect ? 'connect' : 'disconnect'](this, '$onKeyPressEvent');
        }
        if (this.onKeyReleaseEvent) {
            Controller.keyReleaseEvent[connect ? 'connect' : 'disconnect'](this, '$onKeyReleaseEvent');
        }
        Controller[(connect ? 'capture' : 'release') + 'KeyEvents'](this._options);
    }
};

// helper to reload a client script
reloadClientScript._findRunning = function(filename) {
    return ScriptDiscoveryService.getRunning().filter(function(script) {
        return 0 === script.path.indexOf(filename);
    });
};
function reloadClientScript(filename) {
    function log() {
        print('reloadClientScript | ', [].slice.call(arguments).join(' '));
    }
    log('attempting to reload using stopScript(..., true):', filename);
    var result = ScriptDiscoveryService.stopScript(filename, true);
    if (!result) {
        var matches = reloadClientScript._findRunning(filename),
            path = matches[0] && matches[0].path;
        if (path) {
            log('attempting to reload using matched getRunning path: ' + path);
            result = ScriptDiscoveryService.stopScript(path, true);
        }
    }
    log('///result:' + result);
    return result;
}
