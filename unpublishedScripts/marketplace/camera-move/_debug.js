/* eslint-env jquery, browser */
/* eslint-disable comma-dangle, no-empty */
/* global EventBridge: true, PARAMS, signal, assert, log, debugPrint,
   bridgedSettings, _utils, jquerySettings,  */

// helper functions for debugging and testing the UI in an external web brower
var _debug = {
    handleUncaughtException: function onerror(message, fileName, lineNumber, colNumber, err) {
        if (message === onerror.lastMessage) {
            return;
        }
        onerror.lastMessage = message;
        var error = (err || Error.lastError);
        // var stack = error && error.stack;
        var output = _utils.normalizeStackTrace(error || { message: message });
        window.console.error(['window.onerror: ', output, message]); // eslint-disable-line no-console
        var errorNode = document.querySelector('#errors'),
            textNode = errorNode && errorNode.querySelector('.output');
        if (textNode) {
            textNode.innerText = output;
        }
        if (errorNode) {
            errorNode.style.display = 'block';
        }
        if (error){
            error.onerrored = true;
        }
    },
    loadScriptNodes: function loadScriptNodes(selector) {
        // scripts are loaded this way to ensure that when the client script refreshes, so are the app's dependencies
        [].forEach.call(document.querySelectorAll(selector), function(script) {
            script.parentNode.removeChild(script);
            if (script.src) {
                script.src += location.search;
            }
            script.type = 'application/javascript';
            document.write(script.outerHTML);
        });
    },

    // TESTING MOCKs
    openEventBridgeMock: function openEventBridgeMock(onEventBridgeOpened) {
        var updatedValues = openEventBridgeMock.updatedValues = {};
        // emulate EventBridge's API
        EventBridge = {
            emitWebEvent: signal(function emitWebEvent(message){}),
            scriptEventReceived: signal(function scriptEventReceived(message){}),
        };
        EventBridge.emitWebEvent.connect(onEmitWebEvent);
        onEventBridgeOpened(EventBridge);
        assert(!bridgedSettings.onUnhandledMessage);
        bridgedSettings.onUnhandledMessage = function(msg) {
            log('bridgedSettings.onUnhandledMessage', msg);
            return true;
        };
        // manually trigger initial bootstrapping responses (that the client script would normally send)
        bridgedSettings.handleExtraParams({uuid: PARAMS.uuid, ns: PARAMS.ns, extraParams: {
            mock: true,
            appVersion: 'browsermock',
            toggleKey: { text: 'SPACE', isShifted: true },
            mode: {
                toolbar: true,
                browser: true,
                desktop: true,
                tablet: /tablet/.test(location) || /android|ipad|iphone/i.test(navigator.userAgent),
                hmd: /hmd/.test(location),
            },
        } });
        bridgedSettings.setValue('ui-show-advanced-options', true);

        function log(msg) {
            // eslint-disable-next-line no-console
            console.log.apply(console, ['[mock] ' + msg].concat([].slice.call(arguments,1)));
        }

        // generate mock data in response to outgoing web page events
        function onEmitWebEvent(message) {
            try {
                var obj = JSON.parse(message);
            } catch (e) {}
            if (!obj) {
                // message isn't JSON so just log it and bail early
                log('consuming non-callback web event', message);
                return;
            }
            switch (obj.method) {
                case 'valueUpdated': {
                    log('valueUpdated',obj.params);
                    updatedValues[obj.params[0]] = obj.params[1];
                    return;
                }
                case 'Settings.getValue': {
                    var key = obj.params[0];
                    var node = jquerySettings.findNodeByKey(key, true);
                    // log('Settings.getValue.findNodeByKey', key, node);
                    var type = node && (node.dataset.hifiType || node.dataset.type || node.type);
                    switch (type) {
                        case 'hifiButton':
                        case 'hifiCheckbox': {
                            obj.result = /tooltip|advanced-options/i.test(key) || PARAMS.tooltiptest ? true : Math.random() > 0.5;
                        } break;
                        case 'hifiRadioGroup': {
                            var radios = $(node).find('input[type=radio]').toArray();
                            while (Math.random() < 0.9) {
                                radios.push(radios.shift());
                            }
                            obj.result = radios[0].value;
                        } break;
                        case 'hifiSpinner':
                        case 'hifiSlider': {
                            var step = node.step || 1, precision = (1/step).toString().length - 1;
                            var magnitude = node.max || (precision >=1 ? Math.pow(10, precision-1) : 10);
                            obj.result = parseFloat((Math.random() * magnitude).toFixed(precision||1));
                        } break;
                        default: {
                            log('unhandled node type for making dummy data: ' + [key, node && node.type, type, node && node.type] + ' @ ' + (node && node.id));
                            obj.result = updatedValues[key] || false;
                        } break;
                    }
                    debugPrint('mock getValue data %c%s = %c%s', 'color:blue',
                        JSON.stringify(key), 'color:green', JSON.stringify(obj.result));
                } break;
                default: {
                    log('ignoring outbound method call', obj);
                } break;
            }
            setTimeout(function() {
                EventBridge.scriptEventReceived(JSON.stringify(obj));
            }, 100);
        }
    },
};

