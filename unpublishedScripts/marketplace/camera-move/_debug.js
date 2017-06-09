_debug = {
    handleUncaughtException: function onerror(message, fileName, lineNumber, colNumber, err) {
        var output = _utils.normalizeStackTrace(err || { message: message });
        console.error('window.onerror: ' + output, err);
        var errorNode = document.querySelector('#errors'),
            textNode = errorNode && errorNode.querySelector('.output');
        if (textNode) textNode.innerText = output;
        if (errorNode) errorNode.style.display = 'block';
    },
    loadScriptNodes: function loadScriptNodes(selector) {
        // scripts are loaded this way to ensure refreshing the client script refreshes dependencies too
        [].forEach.call(document.querySelectorAll(selector), function(script) {
            script.parentNode.removeChild(script);
            if (script.src) {
                script.src += location.search;
            }
            script.type = 'application/javascript';
            document.write(script.outerHTML);
        });
    },
    // TESTING MOCK (allows the UI to be tested using a normal web browser, outside of Interface
    openEventBridgeMock: function openEventBridgeMock(onEventBridgeOpened) {
        // emulate EventBridge's API
        EventBridge = {
            emitWebEvent: signal(function emitWebEvent(message){}),
            scriptEventReceived: signal(function scriptEventReceived(message){}),
        };
        EventBridge.emitWebEvent.connect(onEmitWebEvent);
        onEventBridgeOpened(EventBridge);
        setTimeout(function() {
            assert(!bridgedSettings.onUnhandledMessage);
            bridgedSettings.onUnhandledMessage = function(msg) {
                return true;
            };
            // manually trigger bootstrapping responses
            $('.slider .control').parent().css('visibility','visible');
            bridgedSettings.handleExtraParams({uuid: PARAMS.uuid, ns: PARAMS.ns, extraParams: {
                mock: true,
                appVersion: 'browsermock',
                toggleKey: { text: 'SPACE', isShifted: true },
            } });
            bridgedSettings.setValue('ui-show-advanced-options', true);
            if (/fps/.test(location.hash)) setTimeout(function() { $('#fps').each(function(){ this.scrollIntoView(); }); }, 100);
        },1);

        function log(msg) {
            console.log.apply(console, ['[mock] ' + msg].concat([].slice.call(arguments,1)));
        }

        var updatedValues = {};
        // generate mock data in response to outgoing web events
        function onEmitWebEvent(message) {
            try { var obj = JSON.parse(message); } catch(e) {}
            if (!obj) {
                // message isn't JSON or doesn't expect a reply so just log it and bail early
                log('consuming non-callback web event', message);
                return;
            }
            switch(obj.method) {
                case 'valueUpdated': {
                    updatedValues[obj.params[0]] = obj.params[1];
                } break;
                case 'Settings.getValue': {
                    var key = obj.params[0];
                    var node = jquerySettings.findNodeByKey(key, true);
                    var type = node && (node.dataset.type || node.getAttribute('type'));
                    switch(type) {
                        case 'checkbox': {
                            obj.result = /tooltip/i.test(key) || PARAMS.tooltiptest ? true : Math.random() > .5;
                        } break;
                        case 'radio-group': {
                            var radios = $(node).find('input[type=radio]').toArray();
                            while(Math.random() < .9) { radios.push(radios.shift()); }
                            obj.result = radios[0].value;
                        } break;
                        case 'number': {
                            var step = node.step || 1, precision = (1/step).toString().length - 1;
                            var magnitude = node.max || (precision >=1 ? Math.pow(10, precision-1) : 10);
                            obj.result = parseFloat((Math.random() * magnitude).toFixed(precision||1));
                        } break;
                        default: {
                            log('unhandled node type for making dummy data: ' + [key, node && node.type, type, node && node.getAttribute('type')] + ' @ ' + (node && node.id));
                            obj.result = updatedValues[key] || false;
                        } break;
                    }
                    log('mock getValue data %c%s = %c%s', 'color:blue',
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

