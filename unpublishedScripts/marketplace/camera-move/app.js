// app.js -- jquery support functions

/* eslint-env commonjs, browser */

// ----------------------------------------------------------------------------
function defineCustomWidgets() {
    $.widget('ui.hifiCheckboxRadio', $.ui.checkboxradio, {
        _create: function() {
            this._super();
            this.element[0].value = this.element[0].id;
            debugPrint('ui.hifiCheckboxRadio._create', this.element[0].type, this.element[0].id, this.element[0].value);
        },
    });//$.fn.hifiCheckboxRadio = $.fn.checkboxradio;
    $.widget('ui.hifiControlGroup', $.ui.controlgroup, {
        _create: function(x) {
            debugPrint('ui.hifiControlGroup._create', this.element[0])
            var tmp = this.options.items.checkboxradio;
            delete this.options.items.checkboxradio;
            this.options.items.hifiCheckboxRadio = tmp;
            this._super();

            Object.defineProperty(this.element[0], 'value', {
                enumerable: true,
                get: function() { assert(false, 'attempt to access hifiControlGroup.element[0].value...' +[this.id,this.name]); },
                set: function(nv) { assert(false, 'attempt to set hifiControlGroup.element[0].value...' +[this.id,this.name]); },
            });
        },
    });
    $.widget('ui.hifiSpinner', $.ui.spinner, {
        _create: function() {
            debugPrint('ui.hifiSpinner._create', this.element[0])
            this.previous = null;
            this._super();
        },
        _spin: function( step, event ) {
            if (event.type === 'mousewheel') {
                if (!event.shiftKey) {
                    step *= ('1e'+Math.max(1,this._precision()))/10;
                }
                if (event.ctrlKey) {
                    step *= 10;
                }
            }
            return this._super( step, event );
        },
        _stop: function( event, ui ) {
            try {
                return this._super(event, ui);
            } finally {
                if (/mouse/.test(event && event.type)) {
                    var value = this.element.val();
                    if (value != "" && !isNaN(value) && this.previous !== null && this.previous !== value) {
                        debugPrint(this.element[0].id, 'spinner.changed', event.type, JSON.stringify({
                            previous: isNaN(this.previous) ? this.previous+'' : this.previous,
                            val: isNaN(value) ? value+'' : value,
                        }));
                        this.element.change();
                    }
                    this.previous = value;
                }
            }
        },
        _format: function(n) {
            var precision = this._precision()
            return parseFloat(n).toFixed(precision);
        },
        _events: {
            mousewheel: function(event, delta) {
                if (document.activeElement !== this.element[0])
                    return;
                // fix broken mousewheel on Chrome / webkit
                delta = delta === undefined ? event.originalEvent.deltaY : delta;
                $.ui.spinner.prototype._events.mousewheel.call(this, event, delta);
            }
        }
    });
    $.widget('ui.hifiSlider', $.ui.slider, {
        _create: function() {
            this._super();
            // add the inner circle and border (per design specs) to jquery-ui's existing slider handle
            this.element.find('.ui-slider-handle')
                .html('<div class="inner-ui-slider-handle"></div>');
        },
    });
}

// JSON export / import helpers proto module
JQuerySettings.$json = (function() {
    return {
        setPath: setPath,
        rollupPaths: rollupPaths,
        encodeNodes: encodeNodes,
        exportAll: exportAll,
        showSettings: showSettings,
        applyJSON: applyJSON,
        promptJSON: promptJSON,
        popupJSON, popupJSON,
    };

    function encodeNodes(resolver, elements) {
        return elements.toArray().reduce((function(out, input, i) {
            log('input['+i+']', input.id);
            var id = input.type === 'radio' ? $(input).closest(':ui-hifiControlGroup').prop('id') : input.id;
            var key = resolver.getKey(id);
            log('toJSON', id, key, input.id);
            setPath(out, key.split('/'), resolver.getValue(key));
            return out;
        }).bind(this), {});
    }

    function setPath(obj, path, value) {
        var key = path.pop();
        obj = path.reduce(function(obj, subkey) {
            return obj[subkey] = obj[subkey] || {};
        }, obj);
        debugPrint('setPath', key, Object.keys(obj));
        obj[key] = value;
    }

    function rollupPaths(obj, output, path) {
        path = path || [];
        output = output || {};
        log('rollupPaths', Object.keys(obj||{}), Object.keys(output), path);
        for (var p in obj) {
            path.push(p);
            var value = obj[p];
            if (value && typeof value === 'object') {
                rollupPaths(obj[p], output, path);
            } else {
                output[path.join('/')] = value;
            }
            path.pop();
        }
        return output;
    }

    function exportAll(resolver, name) {
        return {
            version: VERSION,
            name: name || undefined,
            settings: encodeNodes(resolver, $('input')),
            _metadata: { timestamp: new Date(), PARAMS: PARAMS, url: location.href, }
        };
    };

    function showSettings(resolver, saveName) {
        JQuerySettings.$json.popupJSON(saveName || '(current settings)', Object.assign(JQuerySettings.$json.exportAll(resolver, saveName), {
            extraParams: bridgedSettings.extraParams,
        }));
    };

    function popupJSON(title, tmp) {
        var HTML = POPUP.innerHTML
            .replace(/\bxx-script\b/g, 'script')
            .replace('JSON', JSON.stringify(tmp, 0, 2).replace(/\n/g, '<br />'));
        if (0) {
            bridgedSettings.sendEvent({
                method: 'overlayWebWindow',
                options: {
                    title: 'app-camera-move-export' + (title ? '::'+title : ''),
                    content: HTML,
                },
            });
        } else {
            // make the browser address bar less ugly by putting spaces and friedly name as a "URL footer"
            var footer = '<\!-- #' + HTML.substr(0,256).replace(/./g,' ') + (title || 'Camera Move Settings');
            window.open("data:text/html;escape," + encodeURIComponent(HTML) + footer,"app-camera-move-export");
        }
    }

    function applyJSON(resolver, name, tmp) {
        assert('version' in tmp && 'settings' in tmp, 'invalid settings record: ' + JSON.stringify(tmp))
        var settings = rollupPaths(tmp.settings);
        for(var p in settings) {
            var key = resolver.getId(p, true);
            if (!key) {
                log('$applySettings -- skipping unregistered Settings key: ', p);
            } else {
                resolver.setValue(p, settings[p], name+'.settings.'+p);
            }
        }
    };

    function promptJSON() {
        var json = window.prompt('(paste JSON here)', '');
        if (!json) return;
        try {
            json = JSON.parse(json);
        } catch(e) {
            throw new Error('Could not parse pasted JSON: ' + e + '\n\n' + (json||'').replace(/</g,'&lt;'));
        }
        return json;
    }
})(this);

// ----------------------------------------------------------------------------
// manage jquery-tooltipster hover tooltips
TooltipManager = (function(global) {
    function TooltipManager(options) {
        this.options = options;
        assert(options.elements, 'TooltipManager constructor expects options.elements');
        assert(options.tooltips, 'TooltipManager constructor expects options.tooltips');
        var TOOLTIPS = this.TOOLTIPS = {};
        $(options.tooltips).find('[for]').each(function() {
            var id = $(this).attr('for');
            TOOLTIPS[id] = $(this).html();
        });

        var _self = this;
        options.elements.each(function() {
            var element = $($(this).closest('.tooltip-target').get(0) || this),
                input = element.is('input, button') ? element : element.find('input, button'),
                parent = element.is('.row, button') ? element : element.parent(),
                id = element.attr('for') || input.prop('id'),
                tip = TOOLTIPS[id] || element.prop('title');

            var tooltipSide = element.data('tooltipSide');

            log('binding tooltip', tooltipSide, element[0].nodeName, id || element[0], tip);
            if(!tip)
                return log('missing tooltip: ' + (id || this.id || this.name || this.nodeName));
            if(element.is('.tooltipstered')) {
                console.info('already tooltipstered!?', this.id, this.name, id);
                return;
            }
            var instance = element.tooltipster({
                theme: ['tooltipster-noir'],
                side: tooltipSide || (
                    input.is('button') ? 'top' :
                        input.closest('.slider').get(0) || input.closest('.column').get(0) ? ['top','bottom'] :
                        ['right','top','bottom', 'left']
                ),
                content: tip,
                updateAnimation: 'scale',
                trigger: !options.testmode ? 'hover' : 'click',
                distance: element.is('.slider.row') ? -20 : undefined,//element
                delay: [500, 1000],
                contentAsHTML: true,
                interactive: options.testmode,
                minWidth: options.viewport && options.viewport.min.x,
                maxWidth: options.viewport && options.viewport.max.w,
            }).tooltipster('instance');

            instance.on('close', function(event) {
                if (options.keepopen === element) {
                    debugPrint(event.type, 'canceling close keepopen === element', id);
                    event.stop();
                    options.keepopen = null;
                }
            });
            instance.on('before', function(event) {
                debugPrint(event.type, 'before', event);
                !options.testmode && _self.closeAll();
                !options.enabled && event.stop();
                return;
            });
            parent.find(':focusable, input, [tabindex], button, .control')
                .add(parent)
                .add(input.closest(':focusable, input, [tabindex]'))
                .on({
                    click: function(evt) {
                        if (input.is('button')) return setTimeout(instance.close.bind(instance,null),50);
                        options.keepopen = element; 0&&instance.open();
                    },
                    focus: instance.open.bind(instance, null),
                    blur: function(evt) { instance.close(); _self.openFocusedTooltip(); },
                });
        });
        Object.assign(this, {
            openFocusedTooltip: function() {
                if (!this.options.enabled)
                    return;
                setTimeout(function() {
                    if (!document.activeElement || document.activeElement === document.body ||
                        !$(document.activeElement).closest('section')) {
                        return;
                    }
                    var tip = $([])
                        .add($(document.activeElement))
                        .add($(document.activeElement).find('.tooltipstered'))
                        .add($(document.activeElement).closest('.tooltipstered'))
                        .filter('.tooltipstered');
                    if (tip.is('.tooltipstered')) {
                        log('opening focused tooltip', tip.length, tip[0].id);
                        tip.tooltipster('open');
                    }
                },1);
            },
            rapidClose: function(instance, reopen) {
                if (!instance.status().open) {
                    return;
                }
                instance.elementTooltip() && $(instance.elementTooltip()).hide();
                instance.close(function() { reopen && instance.open(); });
            },
            openAll: function() {
                $('.tooltipstered').tooltipster('open');
            },
            closeAll: function() {
                $.tooltipster.instances().forEach(function(instance) {
                    this.rapidClose(instance);
                }.bind(this));
            },
            updateViewport: function(viewport) {
                var options = {
                    minWidth: viewport.min.x,
                    maxWidth: viewport.max.x,
                };
                $.tooltipster.setDefaults(options);
                log('updating tooltipster options', JSON.stringify(options, 0, 2));
                $.tooltipster.instances().forEach(function(instance) {
                    instance.option('minWidth', options.minWidth);
                    instance.option('maxWidth', options.maxWidth);
                    this.rapidClose(instance, instance.status().open);
                }.bind(this));
            },
            enable: function() {
                this.options.enabled = true;
                if (this.options.testmode)
                    this.openAll();
            },
            disable: function() {
                this.options.enabled = false;
                this.closeAll();
            },
        });
    }
    return TooltipManager;
})(this);

function setupUI() {
    $(window).trigger('resize'); // populate viewport

    $('#debug-menu button, footer button').button();

    var $json = JQuerySettings.$json;

    var buttonHandlers = {
        'page-reload': function() {
            log('triggering location.reload');
            location.reload();
        },
        'script-reload': function() {
            log('triggering script.reload');
            bridgedSettings.sendEvent({ method: 'reloadClientScript' });
        },
        'reset-sensors': function() {
            log('resetting avatar orientation');
            bridgedSettings.sendEvent({ method: 'resetSensors' });
        },
        'reset-to-defaults': function() {
            tooltipManager.closeAll();
            document.activeElement && document.activeElement.blur();
            document.body.focus();
            setTimeout(function() {
                bridgedSettings.sendEvent({ method: 'reset' });
            },1);
        },
        'copy-json': $json.showSettings.bind($json, jquerySettings, null),
        'paste-json': function() {
            $json.applyJSON(
                jquerySettings,
                'pasted',
                $json.promptJSON()
            );
        },
        'toggle-advanced-options': function(evt) {
            var on = $('body').hasClass('ui-show-advanced-options');
            bridgedSettings.setValue('ui-show-advanced-options', !on);
            evt.stopPropagation();
            evt.preventDefault();
            $(this).tooltipster('instance').content((on ? 'show' : 'hide') + ' advanced options');
            if (!on) {
                $('.scrollable').delay(100).animate({
                    scrollTop: innerHeight - $('header').innerHeight() - 24
                }, 1500);
            }
        },
        'appVersion': function(evt) { evt.shiftKey && $json.showSettings(jquerySettings); },
        'errors': function() {
            $(this).find('.output').text('').end().hide();
        },
    };
    Object.keys(buttonHandlers).forEach(function(p) {
        $('#' + p).css('cursor', 'pointer').on('click', buttonHandlers[p]);
    });

    // ----------------------------------------------------------------
    tooltipManager = new TooltipManager({
        enabled: false,
        testmode: PARAMS.tooltiptest,
        viewport: PARAMS.viewport,
        tooltips: $('#tooltips'),
        elements: $($.unique($('input.setting, button').map(function() {
            var input = $(this),
                button = input.is('button') && input,
                row = input.closest('.row').is('.slider') && input.closest('.row'),
                label = input.is('[type=checkbox],[type=radio]') && input.parent().find('label');

            return (button || label || row || input).get(0);
        }))),
    });

    $( "input[data-type=number]" ).each(function() {
        // set up default numeric precisions
        var step = $(this).prop('step') || .01;
        var precision = ((1 / step).toString().length - 1);
        $(this).prop('step', step || Math.pow(10, -precision));
    });

    $('input.setting').each(function() {
        // set up the bidirectional mapping between DOM and Settings
        jquerySettings.registerNode(this);
    }).on('change', function(evt) {
        // set up change detection
        var input = $(this),
            key = assert(jquerySettings.getKey(this.id)),
            type = this.dataset.type || input.prop('type'),
            valid = (this.value !== '' && (type !== 'number' || isFinite(this.value)));

        debugPrint('input.setting.change', key, evt, this);
        $(this).closest('.row').toggleClass('invalid', !valid);

        if (!valid) {
            debugPrint('input.setting.change: ignoring invalid value ' + this.value + ' for ' + key);
            return;
        }

        jquerySettings.resyncValue(key, 'input.setting.change');

        // radio buttons are codependent (ie: TCOBO selected at a time)
        //   so if under a control group, trigger a display refresh
        $(evt.target).closest(':ui-hifiControlGroup').find(':ui-hifiCheckboxRadio').hifiCheckboxRadio('refresh');
    });

    if (PARAMS.debug || true) {
        // support hitting 'r' key to refresh during development
        $(window).on('keypress.debug', function(evt) {
            if (!$(evt.target).is('input')) {
                var ch = String.fromCharCode(evt.keyCode);
                if (evt.originalEvent.keyIdentifier === 'U+0057')
                    ch = 'w';
                switch(ch) {
                    case 'r': {
                        log('... "r" reloading', evt.keyCode);
                        location.reload()
                    } break;
                    case 'w': {
                        if (evt.ctrlKey) {
                            log('ctrl-w detected; closing via client script');
                            bridgedSettings.sendEvent({ method: 'window.close' });
                        }
                    } break;
                }
            }
        });
    }

    // numeric fields
    $( ".number.row input.setting" )
        .hifiSpinner({
            disabled: true,
            create: function() {
                var input = $(this),
                    id = assert(this.id, '.number input.setting without id attribute: ' + this),
                    key = assert(jquerySettings.getKey(this.id));

                var options = input.hifiSpinner('instance').options;
                options.min = options.min || 0.0;

                bridgedSettings.getValueAsync(key, function(err, result) {
                    input.hifiSpinner('enable');
                    jquerySettings.setValue(key, result);
                });
            },
        });

    // radio button groups
    $( ".radio.row" )
        .hifiControlGroup({
            direction: 'horizontal',
            disabled: true,
            create: function() {
                var group = $(this), id = this.id;
                this.setAttribute('type', this.type = this.dataset.type = 'radio-group');
                var key = assert(jquerySettings.getKey(id));
                bridgedSettings.getValueAsync(key, function(err, result) {
                    debugPrint('> GOT RADIO', key, id, result);
                    group.hifiControlGroup('enable');
                    jquerySettings.setValue(key, result);
                    group.change();
                });
            },
        });

    // checkbox fields
    $( ".bool.row input.setting" )
        .hifiCheckboxRadio({ disabled: true })
        .each(function() {
            var key = assert(jquerySettings.getKey(this.id)),
                input = $(this);

            bridgedSettings.getValueAsync(key, function(err, result) {
                input.hifiCheckboxRadio('enable');
                jquerySettings.setValue(key, result);
            });
        });

    // slider + numeric field sets
    $( ".slider.row .control" ).each(function(ent) {
        var element = $(this),
            input = element.parent().find('input'),
            id = input.prop('id'),
            key = assert(jquerySettings.getKey(id));

        var commonOptions = {
            disabled: true,
            min: parseFloat(input.prop('min') || 0),
            max: parseFloat(input.prop('max') || 10),
            step: parseFloat(input.prop('step') || 0.01),
        };
        debugPrint('commonOptions', commonOptions);

        // see: https://api.jqueryui.com/slider/ for more options
        var slider = element.hifiSlider(Object.assign({
            orientation: "horizontal",
            range: "min",
            animate: 'fast',
            value: 0.0,
            start: function(event, ui) { this.$startValue = ui.value; },
            slide: function(event, ui) { input.hifiSpinner('value', ui.value); },
            stop: function(event, ui) {
                if (ui.value != this.$startValue) {
                    jquerySettings.setValue(key, ui.value, 'slider.stop');
                }
            },
        }, commonOptions));

        // setup chrome up/down arrow steps and change event for propagating input field -> slider
        input.on('change', function() {
            element.hifiSlider("value", this.value);
        }).hifiSpinner(Object.assign({}, commonOptions, { max: Infinity }));

        bridgedSettings.getValueAsync(key, function(err, result) {
            input.hifiSpinner('enable');
            element.hifiSlider('enable');
            jquerySettings.setValue(key, result);
        });
    });

    // make all other numeric fields into custom jquery spinners
    $('input[data-type=number]:not(:ui-hifiSpinner)').hifiSpinner({
        disabled: true,
        create: function() {
            var input = $(this),
                id = assert(this.id, '.number input.setting without id attribute: ' + this),
                key = assert(jquerySettings.getKey(this.id));

            log('======================================', id, key);

            var options = input.hifiSpinner('instance').options;
            options.min = options.min || 0.0;

            bridgedSettings.getValueAsync(key, function(err, result) {
                jquerySettings.setValue(key, result);
            });
        },
    });


    // ----------------------------------------------------------------------------
    // allow spacebar to toggle checkbox / radiobutton fields
    $('[type=checkbox]:ui-hifiCheckboxRadio').parent().prop('tabindex',0)
        .on('keydown.toggle', function spaceToggle(evt) {
            if (evt.keyCode === 32) {
                var input = $(this).find(':ui-hifiCheckboxRadio'),
                    id = input.prop('id'),
                    key = assert(jquerySettings.getKey(id));
                log('spaceToggle', evt.target+'', id, key);
                if (!input.is('[type=radio]') || !input.prop('checked')) {
                    input.prop('checked', !input.prop('checked'));
                    input.change();
                }
                evt.preventDefault();
                evt.stopPropagation();
            }
        });

    // when user presses ENTER on a number field, blur it to provide visual feedback
    $('input[data-type=number]').on('keydown.enter', function(evt) {
        if (evt.keyCode === 13 && $(document.activeElement).is('input')) {
            tooltipManager.closeAll();
            document.activeElement.blur();
            var nexts = $('[tabindex],input').not('[tabindex=-1],.ui-slider-handle').toArray();
            if (~nexts.indexOf(this)) {
                var nextActive = nexts[nexts.indexOf(this)+1];
                $(nextActive).focus();
            }
        }
    });

    // by default webkit spacebar behaves like PageDown; this disables that to avoid confusion
    window.onkeydown = function(evt) {
        if (evt.keyCode === 32 && document.activeElement !== document.body) {
            log('snarfing spacebar event', document.activeElement);
            return evt.preventDefault(), evt.stopPropagation(), false;
        }
    };

    // select-all text when an input field is first focused
    $('input').not('input[type=radio],input[type=checkbox]').on('focus', function (e) {
        var dt = (new Date - this.blurredAt);
        if (!(dt < 5)) { // debounce
            this.blurredAt = +new Date;
            //log('FOCUS', dt, e.target === document.activeElement, this === document.activeElement);
            $(this).one('mouseup.selectit', function () {
                $(this).select();
                return false;
            }).select();
        }
    }).on('blur', function(e) {
        this.blurredAt = new Date;
        //log('BLUR', e.target === document.activeElement, this === document.activeElement);
    });

    // monitor specific settings for live changes
    jquerySettings.registerSetting({ type: 'placeholder' }, '.extraParams');
    jquerySettings.registerSetting({ type: 'placeholder' }, 'ui-show-advanced-options');
    jquerySettings.registerSetting({ type: 'placeholder' }, 'Keyboard.RightMouseButton');
    monitorSettings({
        // advanced options toggle
        'ui-show-advanced-options': function(value) {
            function handle(err, result) {
                log('***************************** ui-show-advanced-options updated', result+'');
                $('body').toggleClass('ui-show-advanced-options', !!result);
            }
            if (value !== undefined) {
                handle(null, value);
            } else {
                bridgedSettings.getValueAsync('ui-show-advanced-options', handle);
            }
        },
        // UI tooltips checkbox
        'ui-enable-tooltips': function(value) {
            if (value) {
                tooltipManager.enable();
                tooltipManager.openFocusedTooltip();
            } else {
                tooltipManager.disable();
            }
        },

        // enable/disable fps field based on thread update mode
        'thread-update-mode': function(value) {
            var enabled = value === 'requestAnimationFrame',
                fps = $('#fps');

            log('onThreadModeChanged', value, enabled);
            fps.hifiSpinner(enabled ? 'enable' : 'disable');
            fps.closest('.tooltip-target').toggleClass('disabled', !enabled);
        },

        // apply CSS to body based on whether camera move mode is currently enabled
        'camera-move-enabled': function(value) {
            $('body').toggleClass('camera-move-enabled', value);
        },

        // update keybinding and appVersion whenever extraParams arrives
        '.extraParams': function(value, other) {
            value = bridgedSettings.extraParams;
            //log('.extraParams', value, other, arguments);
            if (value.mode) {
                $('body').toggleClass('tablet-mode', value.mode.tablet);
                $('body').toggleClass('hmd-mode', value.mode.hmd);
                $('body').toggleClass('desktop-mode', value.mode.desktop);
                $('body').toggleClass('toolbar-mode', value.mode.toolbar);
                //setupTabletModeScrolling(value.mode.tablet);
            }
            var versionDisplay = [
                value.appVersion || '(unknown appVersion)',
                PARAMS.debug && '(debug)',
                value.mode && value.mode.tablet ? '(tablet)' : '',
            ].filter(Boolean).join(' | ');
            $('#appVersion').find('.output').text(versionDisplay).end().fadeIn();

            if (value.toggleKey) {
                function getKeysHTML(binding) {
                    return [ 'Control', 'Meta', 'Alt', 'Super', 'Menu', 'Shifted' ]
                        .map(function(flag) { return binding['is' + flag] && flag; })
                        .concat(binding.text || ('(#' + binding.key + ')'))
                        .filter(Boolean)
                        .map(function(key) { return '<kbd>' + key.replace('Shifted','Shift') + '</kbd>'; })
                        .join('-');
                }
                $('#toggleKey').find('.binding').empty()
                    .append(getKeysHTML(value.toggleKey)).end().fadeIn();
            }
        },

        // gray out / ungray out page content if user is mouse looking around in Interface
        // (otherwise the cursor still interacts with web content...)
        'Keyboard.RightMouseButton': function(localValue, key, value) {
            log('... Keyboard.RightMouseButton:' + value);
            window.active = !value;
        },
    });
    $('input').css('-webkit-user-select', 'none');
} // setupUI

// helper for instrumenting local jquery onchange handlers
function monitorSettings(options) {
    return Object.keys(options).reduce(function(out, id) {
        var key = bridgedSettings.resolve(id);
        assert(function assertion(){ return typeof key === 'string' }, 'monitorSettings -- received invalid key type')
        function onChange(varargs) {
            debugPrint('onChange', id, typeof varargs === 'string' ? varargs : typeof varargs);
            var args = [].slice.call(arguments);
            options[id].apply(this, [ jquerySettings.getValue(id) ].concat(args));
        }
        if (bridgedSettings.pendingRequestCount()) {
            bridgedSettings.pendingRequestsFinished.connect(function once() {
                bridgedSettings.pendingRequestsFinished.disconnect(once);
                onChange('pendingRequestsFinished');
            });
        } else {
            onChange('initialization');
        }
        function _onValueUpdated(_key) {  _key === key && onChange.apply(this, arguments); }
        bridgedSettings.valueUpdated.connect(bridgedSettings, _onValueUpdated);
        jquerySettings.valueUpdated.connect(jquerySettings, _onValueUpdated);
        return out;
    }, {});
}

function logValueUpdate(hint, key, value, oldValue, origin) {
    if (0 === key.indexOf('.')) {
        return;
    }
    oldValue = JSON.stringify(oldValue), value = JSON.stringify(value);
    _debugPrint('[ ' + hint +' @ ' + origin + '] ' + key + ' = ' + value + ' (was: ' + oldValue + ')');
}

function initializeDOM() {
    // DOM initialization
    window.viewportUpdated = signal(function viewportUpdated(viewport) {});
    function triggerViewportUpdate() {
        var viewport = {
            geometry: { x: innerWidth, y: innerHeight },
            min: { x: window.innerWidth / 3, y: 32 },
            max: { x: window.innerWidth * 7/8, y: window.innerHeight * 7/8 },
        };
        viewportUpdated(viewport, triggerViewportUpdate.lastViewport);
        triggerViewportUpdate.lastViewport = viewport;
    }
    viewportUpdated.connect(PARAMS, function(viewport, oldViewport) {
        log('viewportUpdated', viewport);
        Object.assign(PARAMS, {
            viewport: Object.assign(PARAMS.viewport||{}, viewport),
        });
        tooltipManager.updateViewport(viewport);
    });
    document.onselectstart = document.ondragstart =
        document.body.ondragstart = document.body.onselectstart = function(){ return false; };

    document.body.oncontextmenu = document.oncontextmenu = document.body.ontouchstart = function(evt) {
        evt.stopPropagation();
        evt.preventDefault();
        return false;
    };

    $('.scrollable').on('mousemove.unselect', function() {
        if (!$(document.activeElement).is('input')) {
            if (document.selection) {
                log('snarfing mousemove.unselect.selection');
                document.selection.empty()
            } else {
                window.getSelection().removeAllRanges()
            }
        }
    }).on('selectstart.unselect', function(evt) {
        if (!$(document.activeElement).is('input')) {
            log('snarfing selectstart');
            evt.stopPropagation();
            evt.preventDefault();
            return false;
        }
    });

    Object.defineProperty(window, 'active', {
        get: function() { return window._active; },
        set: function(nv) {
            nv = !!nv;
            window._active = nv;
            log('window.active == ' + nv);
            if (!nv) {
                document.activeElement && document.activeElement.blur();
                document.body.focus();
                $('body').toggleClass('active', nv);
            } else {
                $('body').toggleClass('active', nv);
            }
        },
    });
    window.active = true;

    function checkAnim(evt) {
        if (!checkAnim.disabled) {
            if ($('.scrollable').is(':animated')) {
                $('.scrollable').stop();
                log(evt.type, 'stop animation');
            }
        }
    }
    $(window).on({
        resize: function resize() {
            window.clearTimeout(resize.to);
            resize.to = window.setTimeout(triggerViewportUpdate, 100);
        },
        mousedown: checkAnim,
        mouseup: checkAnim,
        scroll: checkAnim,
        wheel: checkAnim,
        blur: function() {
            log('** BLUR ** ');
            document.body.focus();
            document.activeElement && document.activeElement.blur();
            tooltipManager.disable();
            //tooltipManager.closeAll();
        },
        focus: function() {
            log('** FOCUS **');
            bridgedSettings.getValue('ui-enable-tooltips') && tooltipManager.enable();
        },
    });
}

function preconfigureLESS() {
    window.lessOriginalNodes = $('style[type="text/less"]').remove();
    window.lessGlobalVars = Object.assign({
        debug: !!PARAMS.debug,
        localhost: 1||/^\b(?:localhost|127[.])/.test(location),
        hash: '',
    }, {
        'header-height': 48,
        'footer-height': 32,
        'custom-font-family': 'Raleway-Regular',
        'input-font-family': 'FiraSans-Regular',
        'color-highlight': '#009bd5',
        'color-text': '#afafaf',
        'color-bg': '#393939',
        'color-bg-darker': '#252525',
        'color-bg-icon': '#545454',
        'color-primary-button': 'darkblue',
        'color-alt-button': 'green',
        'color-caution-button': 'darkred',
    });

    window.less = {
        //poll: 1000,
        //watch: true,
        globalVars: lessGlobalVars,
    };

    preconfigureLESS.onViewportUpdated = function onViewportUpdated(viewport) {
        if (onViewportUpdated.to) {
            clearTimeout(onViewportUpdated.to);
        } else if (lessGlobalVars.hash) {
            onViewportUpdated.to = setTimeout(onViewportUpdated, 500); // debounce
            return;
        }
        delete lessGlobalVars.hash;
        Object.assign(lessGlobalVars, {
            'interface-mode': /highfidelity/i.test(navigator.userAgent),
            'inner-width': window.innerWidth,
            'inner-height': window.innerHeight,
            'client-width': document.body.clientWidth || window.innerWidth,
            'client-height': document.body.clientHeight || window.innerHeight,
            'hash': '',
        });
        lessGlobalVars.hash = JSON.stringify(JSON.stringify(lessGlobalVars,0,2)).replace(/\\n/g , '\\000a');
        var hash = JSON.stringify(lessGlobalVars, 0, 2);
        log('onViewportUpdated', JSON.parse(onViewportUpdated.lastHash||'{}')['inner-width'], JSON.parse(hash)['inner-width']);
        if (onViewportUpdated.lastHash !== hash) {
            //log('updating lessVars', 'less.modifyVars:' + typeof less.modifyVars, JSON.stringify(lessGlobalVars, 0, 2));
            PARAMS.debug && $('#errors').show().html("<pre>").children(0).text(hash);
            // LESS needs some help to recompile inline styles, so a fresh copy of the source nodes is swapped-in
            var newNodes = lessOriginalNodes.clone().appendTo(document.body);
            less.modifyVars && less.modifyVars(true, lessGlobalVars);
            var oldNodes = onViewportUpdated.lastNodes;
            oldNodes && oldNodes.remove();
            onViewportUpdated.lastNodes = newNodes;
        }
        onViewportUpdated.lastHash = hash;
    };
}
