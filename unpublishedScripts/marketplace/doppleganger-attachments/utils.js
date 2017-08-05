/* eslint-env commonjs */
/* global console */

module.exports = {
    version: '0.0.1',
    bind: bind,
    signal: signal,
    assign: assign,
    assert: assert
};

function log() {
    // eslint-disable-next-line no-console
    (typeof console === 'object' ? console.log : print)('utils | ' + [].slice.call(arguments).join(' '));
}
log(module.exports.version);

// @function - bind a function to a `this` context
// @param {Object} - the `this` context
// @param {Function|String} - function or method name
// @param {value} varargs... - optional curry-right arguments (passed to method after any explicit arguments)
function bind(thiz, method, varargs) {
    method = thiz[method] || method;
    varargs = [].slice.call(arguments, 2);
    return function() {
        var args = [].slice.call(arguments).concat(varargs);
        return method.apply(thiz, args);
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
            var callback = {scope: scope, handler: scope[handler] || handler || scope};
            if (!callback.handler || !callback.handler.apply) {
                throw new Error('invalid arguments to connect:' + [template, scope, handler]);
            }
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
