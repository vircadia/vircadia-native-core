// _utils.js -- misc. helper classes/functions

"use strict";
/* eslint-env commonjs, hifi */
/* eslint-disable comma-dangle, no-empty */
/* global HIRES_CLOCK, Desktop */
// var HIRES_CLOCK = (typeof Window === 'object' && Window && Window.performance) && Window.performance.now;
var USE_HIRES_CLOCK = typeof HIRES_CLOCK === 'function';

var exports = {
    version: '0.0.1b' + (USE_HIRES_CLOCK ? '-hires' : ''),
    bind: bind,
    signal: signal,
    assign: assign,
    sortedAssign: sortedAssign,
    sign: sign,
    assert: assert,
    getSystemMetadata: getSystemMetadata,
    makeDebugRequire: makeDebugRequire,
    DeferredUpdater: DeferredUpdater,
    KeyListener: KeyListener,
    getRuntimeSeconds: getRuntimeSeconds,
    createAnimationStepper: createAnimationStepper,
    reloadClientScript: reloadClientScript,

    normalizeStackTrace: normalizeStackTrace,
    BrowserUtils: BrowserUtils,
    BSOD: BSOD, // exception reporter
    _overlayWebWindow: _overlayWebWindow,
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
        log: function(msg) {
            this.console.log.apply(this.console, ['browserUtils | ' + msg].concat([].slice.call(arguments, 1)));
        },
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
            this.log('openEventBridge |', 'typeof global.EventBridge == ' + typeof global.EventBridge);
            try {
                global.EventBridge.scriptEventReceived.connect.exists;
                // this.log('openEventBridge| EventBridge already exists... -- invoking callback', 'typeof EventBridge == ' + typeof global.EventBridge);
                return callback(global.EventBridge);
            } catch (e) {
                this.log('EventBridge does not yet exist -- attempting to instrument via qt.webChannelTransport');
                var QWebChannel = assert(global.QWebChannel, 'expected global.QWebChannel to exist'),
                    qt = assert(global.qt, 'expected global.qt to exist');
                assert(qt.webChannelTransport, 'expected global.qt.webChannelTransport to exist');
                new QWebChannel(qt.webChannelTransport, bind(this, function (channel) {
                    global.EventBridge = channel.objects.eventBridgeWrapper.eventBridge;
                    global.EventBridge.$WebChannel = channel;
                    this.log('openEventBridge opened -- invoking callback', 'typeof EventBridge === ' + typeof global.EventBridge);
                    callback(global.EventBridge);
                }));
            }
        },
    };
}
// ----------------------------------------------------------------------------
// queue pending updates so the exact order of application can be varied
// (currently Interface exists sporadic jitter that seems to depend on whether
//  Camera or MyAvatar gets updated first)
function DeferredUpdater(target, options) {
    options = options || {};
    // define _meta as a non-enumerable instance property (so it doesn't show up in for(var p in ...) loops)
    Object.defineProperty(this, '_meta', { value: {
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
            // lastValue = meta.lastValue,
            // dedupe = meta.dedupe,
            self = this,
            submitted = {};

        self.submit = getRuntimeSeconds();
        Object.keys(self).forEach(function(property) {
            var newValue = self[property];
            // if (dedupe) {
            //     var stringified = JSON.stringify(newValue);
            //     var last = lastValue[property];
            //     if (stringified === last) {
            //         return;
            //     }
            //     lastValue[property] = stringified;
            // }
            // if (0) {
            //     var tmp = lastValue['_'+property];
            //     if (typeof tmp === 'object') {
            //         if ('w' in tmp) {
            //             newValue = Quat.normalize(Quat.slerp(tmp, newValue, 0.95));
            //         } else if ('z' in tmp) {
            //             newValue = Vec3.mix(tmp, newValue, 0.95);
            //         }
            //     } else if (typeof tmp === 'number') {
            //         newValue = (newValue + tmp)/2.0;
            //     }
            //     lastValue['_'+property] = newValue;
            // }
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

// monotonic session runtime (in seconds)
getRuntimeSeconds.EPOCH = getRuntimeSeconds(0);
function getRuntimeSeconds(since) {
    since = since === undefined ? getRuntimeSeconds.EPOCH : since;
    var now = USE_HIRES_CLOCK ? new HIRES_CLOCK() : +new Date;
    return ((now / 1000.0) - since);
}


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
// KeyListener provides a scoped wrapper where options.onKeyPressEvent only gets
//   called when the specified event.text matches the input options
// example: var listener = new KeyListener({ text: 'SPACE', isShifted: false, onKeyPressEvent: function(event) { ... } });
//          Script.scriptEnding.connect(listener, 'disconnect');
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
                return this.log('KeyListener -- different modifiers, disregarding keystroke', JSON.stringify({
                    expected: this.modifiers,
                    received: modifiers,
                },0,2));
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


// helper to show a verbose exception report in a BSOD-like poup window, in some cases enabling users to
// report specfic feedback without having to manually scan through their local debug logs

// creates an OverlayWebWindow using inline HTML content
function _overlayWebWindow(options) {
    options = Object.assign({
        title: '_overlayWebWindow',
        width: Overlays.width() * 2 / 3,
        height: Overlays.height() * 2 / 3,
        content: '(empty)',
    }, options||{}, {
        source: 'about:blank',
    });
    var window = new OverlayWebWindow(options);
    window.options = options;
    options.content && window.setURL('data:text/html;text,' + encodeURIComponent(options.content));
    return window;
}

function BSOD(options, callback) {
    var buttonHTML = Array.isArray(options.buttons) && [
        '<div onclick="EventBridge.emitWebEvent(arguments[0].target.innerText)">',
        options.buttons.map(function(innerText) {
            return '<button>' + innerText + '</button>';
        }).join(',&nbsp;'),
        '</div>'].join('\n');

    var HTML = [
        '<style>body { background:#0000aa; color:#ffffff; font-family:courier; font-size:8pt; margin:10px; }</style>',
        buttonHTML,
        '<pre style="white-space:pre-wrap;">',
        '<strong>' + options.error + '</strong>',
        normalizeStackTrace(options.error, {
            wrapLineNumbersWith: ['<b style=color:lime>','</b>'],
            wrapFilesWith: ['<b style=color:#f99>','</b>'],
        }),
        '</pre>',
        '<hr />DEBUG INFO:<br />',
        '<pre>' + JSON.stringify(Object.assign({ date: new Date }, options.debugInfo), 0, 2) + '</pre>',
    ].filter(Boolean).join('\n');

    var popup = _overlayWebWindow({
        title: options.title || 'PC LOAD LETTER',
        content: HTML,
    });
    popup.webEventReceived.connect(function(message) {
        print('popup.webEventReceived', message);
        try {
            callback(null, message);
        } finally {
            popup.close();
        }
    });
    return popup;
}

function getSystemMetadata() {
    var tablet = Tablet.getTablet('com.highfidelity.interface.tablet.system');
    return {
        mode: {
            hmd: HMD.active,
            desktop: !HMD.active,
            toolbar: Uuid.isNull(HMD.tabletID),
            tablet: !Uuid.isNull(HMD.tabletID),
        },
        tablet: {
            toolbarMode: tablet.toolbarMode,
            desktopScale: Settings.getValue('desktopTabletScale'),
            hmdScale: Settings.getValue('hmdTabletScale'),
        },
        avatar: {
            pitchSpeed: MyAvatar.pitchSpeed,
            yawSpeed: MyAvatar.yawSpeed,
            density: MyAvatar.density,
            scale: MyAvatar.scale,
        },
        overlays: {
            width: Overlays.width(),
            height: Overlays.height(),
        },
        window: {
            width: Window.innerWidth,
            height: Window.innerHeight,
        },
        desktop: {
            width: Desktop.width,
            height: Desktop.height,
        },
    };
}

function reloadClientScript(filename) {
    function log() {
        print('reloadClientScript | ', [].slice.call(arguments).join(' '));
    }
    log('reloading', filename);
    var result = ScriptDiscoveryService.stopScript(filename, true);
    log('...stopScript', filename, result);
    if (!result) {
        var matches = ScriptDiscoveryService.getRunning().filter(function(script) {
            // log(script.path, script.url);
            return 0 === script.path.indexOf(filename);
        });
        log('...matches', JSON.stringify(matches,0,2));
        var path = matches[0] && matches[0].path;
        if (path) {
            log('...stopScript', path);
            result = ScriptDiscoveryService.stopScript(path, true);
            log('///stopScript', result);
        }
    }
    return result;
}
