// app.js -- support functions

/* eslint-env console, jquery, browser, shared-node-browser */
/* eslint-disable comma-dangle */
/* global Mousetrap, TooltipManager, SettingsJSON, PARAMS, signal, assert, log, debugPrint */
// ----------------------------------------------------------------------------

var viewportUpdated, bridgedSettings, jquerySettings, tooltipManager, lessManager;

function setupUI() {
    viewportUpdated.connect(lessManager, 'onViewportUpdated');

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
    viewportUpdated.connect(tooltipManager, 'updateViewport');

    $(window).trigger('resize'); // populate viewport

    $('#debug-menu button, footer button').hifiButton();

    var $json = SettingsJSON;

    window.buttonHandlers = {
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
            var checkbox = $(this).hifiButton('instance').checkbox;
            var on = checkbox.value(!checkbox.value());
            $('body').toggleClass('ui-show-advanced-options', on);
            evt.stopPropagation();
            evt.preventDefault();
            if ($(this).is('.tooltipstered')) {
                $(this).tooltipster('instance').content((on ? 'hide' : 'show') + ' advanced options');
            }
            if (checkbox.value()) {
                $('.scrollable').delay(100).animate({
                    scrollTop: innerHeight - $('header').innerHeight() - 24
                }, 1500);
            }
        },
        'appVersion': function(evt) {
            evt.shiftKey && $json.showSettings(jquerySettings);
        },
        'errors': function() {
            $(this).find('.output').text('').end().hide();
        },
    };
    Object.keys(window.buttonHandlers).forEach(function(p) {
        $('#' + p).on('click', window.buttonHandlers[p]);
    });

    // ----------------------------------------------------------------

    $( "input[type=number],input[type=spinner],input[step]" ).each(function() {
        // set up default numeric precisions
        var step = $(this).prop('step') || 0.01;
        var precision = ((1 / step).toString().length - 1);
        $(this).prop('step', step || Math.pow(10, -precision));
    });

    var settingsNodes = $('[type=radio-group],input:not([type=radio]),button').filter('[id],[data-for],[for]');
    settingsNodes.filter(':not(button)').each(function() {
        // set up the bidirectional mapping between DOM and Settings
        jquerySettings.registerNode(this);
    });

    var spinnerOptions = {
        disabled: true,
        create: function() {
            var input = $(this),
                key = assert(jquerySettings.getKey(this.id));

            var options = input.hifiSpinner('instance').options;
            options.min = options.min || 0.0;

            bridgedSettings.getValueAsync(key, function(err, result) {
                options.autoenable !== false && input.hifiSpinner('enable');
                jquerySettings.setValue(key, result);
            });
        },
    };

    // numeric settings
    $( ".number.row input.setting" )
        .hifiSpinner(spinnerOptions);

    // radio groups settings
    $( ".radio.row" )
        .hifiRadioGroup({
            direction: 'horizontal',
            disabled: true,
            create: function() {
                var group = $(this), id = this.id;
                var key = assert(jquerySettings.getKey(id));
                bridgedSettings.getValueAsync(key, function(err, result) {
                    debugPrint('> GOT RADIO', key, id, result);
                    group.hifiRadioGroup('enable');
                    jquerySettings.setValue(key, result);
                    group.change();
                });
            },
        });

    // checkbox settings
    $( ".bool.row input.setting" )
        .hifiCheckbox({ disabled: true })
        .each(function() {
            var key = assert(jquerySettings.getKey(this.id)),
                input = $(this);

            bridgedSettings.getValueAsync(key, function(err, result) {
                input.hifiCheckbox('enable');
                jquerySettings.setValue(key, result);
            });
        });

    // slider + numeric settings
    $( ".slider.row .control" ).each(function(ent) {
        var element = $(this),
            phor = element.attr('data-for'),
            input = $('input#' + phor),
            id = input .prop('id'),
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
            value: 0.0
        }, commonOptions)).hifiSlider('instance');

        // setup chrome up/down arrow steps and propagate input field -> slider
        var spinner = input.on('change', function() {
            var value = spinner.value();
            if (isFinite(value) && slider.value() !== value) {
                slider.value(value);
            }
        }).hifiSpinner(
            Object.assign({}, commonOptions, { max: Infinity })
        ).hifiSpinner('instance');

        bridgedSettings.getValueAsync(key, function(err, result) {
            slider.enable();
            spinner.enable();
            spinner.value(result);
        });
    });

    // other numeric settings
    $('input[data-type=number]:not(:ui-hifiSpinner)')
        .hifiSpinner(Object.assign({
            autoenable: false,
        }, spinnerOptions));

    // set up DOM MutationObservers
    settingsNodes.each(function tcobo() {
        if (this.dataset.hifiType === 'hifiButton') {
            return;
        }
        var id = assert(this.dataset['for'] || this.id, 'could not id for node: ' + this.outerHTML);
        assert(!tcobo[id]); // detect dupes
        tcobo[id] = true;
        debugPrint('OBSERVING NODE', id, this.id || this.getAttribute('for'));
        jquerySettings.observeNode(this);
    });

    // detect invalid numbers entered into spinner fields
    $(':ui-hifiSpinner').on('change.validation', function(evt) {
        var spinner = $(this).hifiSpinner('instance');
        $(this).closest('.row').toggleClass('invalid', !spinner.isValid());
    });

    // ----------------------------------------------------------------------------
    // allow tabbing between checkboxes
    $('[type=checkbox]:ui-hifiCheckbox').parent().prop('tabindex', 0);

    // select the input field text when first focused
    $('input').not('input[type=radio],input[type=checkbox]').on('focus', function () {
        var dt = (new Date - this.blurredAt);
        if (!(dt < 5)) { // debounce
            this.blurredAt = +new Date;
            $(this).one('mouseup.selectit', function() {
                $(this).select();
                return false;
            }).select();
        }
    }).on('blur', function(e) {
        this.blurredAt = new Date;
    });

    // monitor changes to specific settings that affect the UI
    monitorSettings({
        // advanced options toggle
        'ui-show-advanced-options': function onChange(value) {
            function handle(err, result) {
                log('** ui-show-advanced-options updated', result+'');
                $('body').toggleClass('ui-show-advanced-options', !!result);
            }
            if (!onChange.fetched) {
                bridgedSettings.getValueAsync('ui-show-advanced-options', handle);
                return onChange.fetched = true;
            }
            handle(null, value);
        },

        // UI tooltips toggle
        'ui-enable-tooltips': function(value) {
            if (value) {
                tooltipManager.enable();
                tooltipManager.openFocusedTooltip();
            } else {
                tooltipManager.disable();
            }
        },

        // enable/disable fps field (based on whether thread mode is requestAnimationFrame)
        'thread-update-mode': function(value) {
            var enabled = (value === 'requestAnimationFrame'), fps = $('#fps');
            fps.hifiSpinner(enabled ? 'enable' : 'disable');
            fps.closest('.tooltip-target').toggleClass('disabled', !enabled);
        },

        // flag BODY with CSS class to indicate active camera move mode
        'camera-move-enabled': function(value) {
            $('body').toggleClass('camera-move-enabled', value);
        },

        // update the "keybinding" and #appVersion extraParams displays
        '.extraParams': function extraParams(value, other) {
            value = bridgedSettings.extraParams;
            if (value.mode) {
                for (var p in value.mode) {
                    $('body').toggleClass(p + '-mode', value.mode[p]);
                }
            }
            var versionDisplay = [
                value.appVersion || '(unknown appVersion)',
                PARAMS.debug && '(debug)',
                value.mode && value.mode.tablet ? '(tablet)' : '',
            ].filter(Boolean).join(' | ');
            $('#appVersion').find('.output').text(versionDisplay).end().show();

            if (value.toggleKey) {
                $('#toggleKey').find('.binding').empty()
                    .append(getKeysHTML(value.toggleKey)).end().show();
            }
        },

        // gray out / ungray out page content if user is mouse looking around in Interface
        // (otherwise the cursor still interacts with web content...)
        'Keyboard.RightMouseButton': function(localValue, key, value) {
            log(localValue, '... Keyboard.RightMouseButton:' + value);
            window.active = !value;
        },
    });
    // disable selection
    // $('input').css('-webkit-user-select', 'none');

    // set up key bindings
    setupMousetrapKeys();

    function getKeysHTML(binding) {
        var text = binding.text || ('(#' + binding.key + ')');
        // translate hifi's proprietary key scheme into human-friendly KBDs
        return [ 'Control', 'Meta', 'Alt', 'Super', 'Menu', 'Shifted' ]
            .map(function(flag) {
                return binding['is' + flag] && flag; 
            })
            .concat(text)
            .filter(Boolean)
            .map(function(key) {
                return '<kbd>' + key.replace('Shifted','Shift') + '</kbd>';
            })
            .join('-');
    }
} // setupUI

// helper for instrumenting local jquery onchange handlers
function monitorSettings(options) {
    return Object.keys(options).reduce(function(out, id) {
        var key = bridgedSettings.resolve(id),
            domId = jquerySettings.getId(key, true);

        if (!domId) {
            var placeholder = {
                id: id,
                type: 'placeholder',
                toString: function() {
                    return this.id; 
                },
                value: undefined,
            };
            jquerySettings.registerSetting(placeholder, key);
            log('registered placeholder value for setting', id, key);
            assert(jquerySettings.findNodeByKey(key) === placeholder);
        }

        // if (domId === 'toggle-advanced-options') alert([key,id,domId, jquerySettings.findNodeByKey(key)])
        assert(function assertion(){
            return typeof key === 'string'; 
        }, 'monitorSettings -- received invalid key type');

        var context = {
            id: id,
            key: key,
            domId: domId,
            options: options,
            lastValue: undefined,
            initializer: function(hint) {
                var key = this.key,
                    lastValue = this.lastValue;
                if (lastValue !== undefined) {
                    return log('skipping repeat initializer', key, hint);
                }
                this.lastValue = lastValue = jquerySettings.getValue(key);
                this._onChange.call(jquerySettings, key, lastValue, undefined, hint);
            },
            _onChange: function _onChange(key, value) {
                var currentValue = this.getValue(context.id),
                    jsonCurrentValue = JSON.stringify(currentValue);

                if (jsonCurrentValue === context.jsonLastValue) {
                    if (jsonCurrentValue !== undefined) {
                        log([context.key, '_onChange', this, 'not triggering _onChange for duplicated value']);
                    }
                    return;
                }
                context.jsonLastValue = jsonCurrentValue;
                var args = [].slice.call(arguments, 0);
                log('monitorSetting._onChange', context.key, value, this+'', id, args);
                context.options[context.id].apply(this, [ currentValue ].concat(args));
            },

            onValueReceived: function(key) {
                if (key === this.key) {
                    this._onChange.apply(bridgedSettings, arguments);
                }
            },
            onMutationEvent: function(event) {
                if (event.key === this.key) {
                    context._onChange.call(jquerySettings, event.key, event.value, event.oldValue, event.hifiType+':mutation');
                }
            },
            onPendingRequestsFinished: function onPendingRequestsFinished() {
                bridgedSettings.pendingRequestsFinished.disconnect(this, 'onPendingRequestsFinished');
                this.initializer('pendingRequestsFinished');
            },
        };

        bridgedSettings.valueReceived.connect(context, 'onValueReceived');
        jquerySettings.mutationEvent.connect(context, 'onMutationEvent');

        if (bridgedSettings.pendingRequestCount()) {
            bridgedSettings.pendingRequestsFinished.connect(context, 'onPendingRequestsFinished');
        } else {
            window.setTimeout(context.initializer.bind(context, 'monitorSettings init'), 1);
        }
        return context;
    }, {});
}

function initializeDOM() {

    // document.onselectstart = document.ondragstart =
    //     document.body.ondragstart = document.body.onselectstart = function(){ return false; };

    document.body.oncontextmenu = document.oncontextmenu = document.body.ontouchstart = function(evt) {
        evt.stopPropagation();
        evt.preventDefault();
        return false;
    };

    Object.defineProperty(window, 'active', {
        get: function() {
            return window._active; 
        },
        set: function(nv) {
            nv = !!nv;
            window._active = nv;
            log('window.active == ' + nv);
            if (!nv) {
                document.activeElement && document.activeElement.blur();
                document.body.focus();
            }
            $('body').toggleClass('active', nv);
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
    viewportUpdated = signal(function viewportUpdated(viewport) {});
    function triggerViewportUpdate() {
        var viewport = {
            inner: { width: innerWidth, height: innerHeight },
            client: {
                width: document.body.clientWidth || window.innerWidth,
                height: document.body.clientHeight || window.innerHeight,
            },
            min: { width: window.innerWidth / 3, height: 32 },
            max: { width: window.innerWidth * 7/8, height: window.innerHeight * 7/8 },
        };
        log('viewportUpdated', viewport);
        PARAMS.viewport = Object.assign(PARAMS.viewport||{}, viewport);
        viewportUpdated(viewport, triggerViewportUpdate.lastViewport);
        triggerViewportUpdate.lastViewport = viewport;
    }
    $(window).on({
        resize: function resize() {
            window.clearTimeout(resize.to);
            resize.to = window.setTimeout(triggerViewportUpdate, 100);
        },
        mousedown: checkAnim, mouseup: checkAnim, scroll: checkAnim, wheel: checkAnim,
        blur: function() {
            log('** BLUR ** ');
            $('body').addClass('window-blurred');
            document.body.focus();
            document.activeElement && document.activeElement.blur();
            tooltipManager.disable();
            // tooltipManager.closeAll();
        },
        focus: function() {
            log('** FOCUS **');
            $('body').removeClass('window-blurred');
            bridgedSettings.getValue('ui-enable-tooltips') && tooltipManager.enable();
        },
    });
}

function setupMousetrapKeys() {
    if (!window.Mousetrap) {
        return log('WARNING: window.Mousetrap not found; not configurating keybindings');
    }
    mousetrapMultiBind({
        'ctrl+a, option+a': function global(evt, combo) {
            $(document.activeElement).filter('input').select();
        },
        'enter': function global(evt, combo) {
            var node = document.activeElement;
            if ($(node).is('input')) {
                log('enter on input element');
                tooltipManager.closeAll();
                node.blur();
                var nexts = $('[tabindex],input,:focusable').not('[tabindex=-1],.ui-slider-handle');
                nexts.add(nexts.find('input'));
                nexts = nexts.toArray();
                if (~nexts.indexOf(node)) {
                    var nextActive = nexts[nexts.indexOf(node)+1];
                    log('setting focus to', nextActive);
                    $(nextActive).focus();
                } else {
                    log('could not deduce next tabbable element', nexts.length, this);
                }
            }
            return true;
        },
        'ctrl+w': bridgedSettings.sendEvent.bind(bridgedSettings, { method: 'window.close' }),
        'r': location.reload.bind(location),
        'space': function global(evt, combo) {
            log('SPACE', evt.target, document.activeElement);
            $(document.activeElement).filter('.bool.row').find(':ui-hifiCheckbox').click();
            if (!$(document.activeElement).is('input,.ui-widget')) {
                return false;
            }
            return true;
        },
    });
    // $('input').addClass('mousetrap');
    function mousetrapMultiBind(a, b) {
        var obj = typeof a === 'object' ? a :
            Object.defineProperty({}, a, {enumerable: true, value: b });
        Object.keys(obj).forEach(function(key) {
            var method = obj[key].name === 'global' ? 'bindGlobal' : 'bind';
            key.split(/\s*,\s*/).forEach(function(combo) {
                log('Mousetrap', method, combo, typeof obj[key]);
                Mousetrap[method](combo, function(evt, combo) {
                    log('Mousetrap', method, combo);
                    return obj[key].apply(this, arguments);
                });
            });
        });
    }
}
