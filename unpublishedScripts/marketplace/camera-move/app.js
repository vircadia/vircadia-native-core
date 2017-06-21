// app.js -- support functions

/* eslint-env console, jquery, browser, shared-node-browser */
/* eslint-disable comma-dangle */
/* global Mousetrap, TooltipManager, SettingsJSON, PARAMS, signal, assert, log, debugPrint */
// ----------------------------------------------------------------------------

var viewportUpdated, bridgedSettings, jquerySettings, tooltipManager, lessManager;

function setupUI() {
    $('#debug-menu button, footer button')
        .hifiButton({
            create: function() {
                $(this).addClass('tooltip-target')
                    .data('tooltipster', { side: ['top','bottom'] });
            }
        });

    var $json = SettingsJSON;

    window.buttonHandlers = {
        'test-event-bridge': function() {
            log('bridgedSettings.eventBridge === Window.EventBridge', bridgedSettings.eventBridge === window.EventBridge);
            bridgedSettings.sendEvent({ method: 'test-event-bridge' });
            EventBridge.emitWebEvent('EventBridge.emitWebEvent: testing 1..2..3..');
        },
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
            tooltipManager && tooltipManager.closeAll();
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
        'errors': function(evt) {
            $(evt.target).is('button') && $(this).find('.output').text('').end().hide();
        },
    };
    buttonHandlers['button-toggle-advanced-options'] =
        buttonHandlers['toggle-advanced-options'];
    Object.keys(window.buttonHandlers).forEach(function(p) {
        $('#' + p).on('click', window.buttonHandlers[p]);
    });

    // ----------------------------------------------------------------
    // trim whitespace in labels
    $('label').contents().filter(function() {
        if (this.nodeType !== window.Node.TEXT_NODE) {
            return false;
        }
        this.textContent = this.textContent.trim();
        return !this.textContent.length;
    }).remove();

    var settingsNodes = $('fieldset,button[data-for],input:not([type=radio])');
    settingsNodes.each(function() {
        // set up the bidirectional mapping between DOM and Settings
        jquerySettings.registerNode(this);
    });

    var spinnerOptions = {
        disabled: true,
        create: function() {
            var input = $(this),
                key = assert(jquerySettings.getKey(input.data('for')));

            var options = input.hifiSpinner('instance').options;
            options.min = options.min || 0.0;

            bridgedSettings.getValueAsync(key, function(err, result) {
                input.filter(':not([data-autoenable=false])').hifiSpinner('enable');
                jquerySettings.setValue(key, result);
            });
        },
    };

    $( ".rows > label" ).each(function() {
        var label = $(this),
            input = label.find('input'),
            type = input.data('type') || input.attr('type');
        label.wrap('<div>').parent().addClass(['hifi-'+type, type, 'row'].join(' '))
            .on('focus.row, click.row, hover.row', function() {
                $(this).find('.tooltipstered').tooltipster('open');
            });
    });

    debugPrint('initializing hifiSpinners');
    // numeric settings
    $( ".number.row" )
        .find( "input[data-type=number]" )
        .addClass('setting')
        .hifiSpinner(spinnerOptions);

    // radio groups settings
    $( ".radio.rows" )
        .find('label').addClass('tooltip-target').end()
        .addClass('setting')
        .hifiRadioGroup({
            direction: 'vertical',
            disabled: true,
            create: function() {
                assert(this !== window);
                var group = $(this), id = this.id;
                var key = assert(jquerySettings.getKey(group.data('for')));

                bridgedSettings.getValueAsync(key, function(err, result) {
                    debugPrint('> GOT RADIO', key, id, result);
                    group.filter(':not([data-autoenable=false])').hifiRadioGroup('enable');
                    jquerySettings.setValue(key, result);
                    group.change();
                });
            },
        })

    // checkbox settings
    $( "input[type=checkbox]" )
        .addClass('setting')
        .hifiCheckbox({
            disabled: true,
            create: function() {
                var key = assert(jquerySettings.getKey(this.id)),
                    input = $(this);
                input.closest('label').addClass('tooltip-target');
                bridgedSettings.getValueAsync(key, function(err, result) {
                    input.filter(':not([data-autoenable=false])').hifiCheckbox('enable');
                    jquerySettings.setValue(key, result);
                });
            },
        });

    // slider + numeric settings
    // use the whole row as a tooltip target
    $( ".slider.row" ).addClass('tooltip-target').data('tooltipster', {
        distance: -20,
        side: ['top', 'bottom'],
    }).each(function(ent) {
        var element = $(this).find( ".control" ),
            input = $(this).find('input'),
            id = input.prop('id'),
            key = assert(jquerySettings.getKey(id));

        var commonOptions = {
            disabled: true,
            min: parseFloat(input.prop('min') || 0),
            max: parseFloat(input.prop('max') || 10),
            step: parseFloat(input.prop('step') || 0.01),
            autoenable: input.data('autoenable') !== 'false',
        };
        debugPrint('commonOptions', commonOptions);

        // see: https://api.jqueryui.com/slider/ for more options
        var slider = element.hifiSlider(Object.assign({
            orientation: "horizontal",
            range: "min",
            animate: 'fast',
            value: 0.0
        }, commonOptions)).hifiSlider('instance');

        debugPrint('initializing hifiSlider->hifiSpinner');
        // setup chrome up/down arrow steps and propagate input field -> slider
        var spinner = input.on('change', function() {
            var value = spinner.value();
            if (isFinite(value) && slider.value() !== value) {
                slider.value(value);
            }
        }).addClass('setting')
            .hifiSpinner(
                Object.assign({}, commonOptions, { max: 1e4 })
            ).hifiSpinner('instance');

        bridgedSettings.getValueAsync(key, function(err, result) {
            slider.options.autoenable !== false && slider.enable();
            spinner.options.autoenable !== false && spinner.enable();
            spinner.value(result);
        });
    });

    $('#fps').hifiSpinner(spinnerOptions).closest('.row').css('pointer-events', 'all').on('click.subinput', function(evt) {
        jquerySettings.setValue('thread-update-mode', 'requestAnimationFrame');
        evt.target.focus();
    });
    // detect invalid numbers entered into spinner fields
    $(':ui-hifiSpinner').on('change.validation', function(evt) {
        var spinner = $(this).hifiSpinner('instance');
        $(this).closest('.row').toggleClass('invalid', !spinner.isValid());
    });

    // ----------------------------------------------------------------------------
    // allow tabbing between checkboxes using the container row
    $(':ui-hifiCheckbox,:ui-hifiRadioButton').prop('tabindex', -1).closest('.row').prop('tabindex', 0);


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
    var monitors = {
        // advanced options toggle
        'ui-show-advanced-options': function onChange(value) {
            function handle(err, result) {
                log('** ui-show-advanced-options updated', result+'');
                $('body').toggleClass('ui-show-advanced-options', !!result);
                jquerySettings.setValue('ui-show-advanced-options', result)
            }
            if (!onChange.fetched) {
                bridgedSettings.getValueAsync('ui-show-advanced-options', handle);
                return onChange.fetched = true;
            }
            handle(null, value);
        },

        // UI tooltips toggle
        'ui-enable-tooltips': function(value) {
            if (!tooltipManager) return;
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
            fps.closest('.row').toggleClass('disabled', !enabled);
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
                    // tablet-mode, hmd-mode, etc.
                    $('body').toggleClass(p + '-mode', value.mode[p]);
                }
                document.oncontextmenu = value.mode.tablet ? function(evt) { return evt.preventDefault(),false; }  : null;
                $('[data-type=number]').prop('type', value.mode.tablet ? 'number' : 'text');
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

            var activateLookAtOption = value.MyAvatar && value.MyAvatar.supportsLookAtSnappingEnabled;
            $(jquerySettings.findNodeByKey('Avatar/lookAtSnappingEnabled'))
                .hifiCheckbox(activateLookAtOption ? 'enable' : 'disable')
                .closest('.row').toggleClass('disabled', !activateLookAtOption)
                .css('pointer-events', 'all') // so tooltips display regardless

            var activateCursorOption = value.Reticle && value.Reticle.supportsScale;
            $('#minimal-cursor:ui-hifiCheckbox')
                .hifiCheckbox(activateCursorOption ? 'enable' : 'disable')
                .closest('.row').toggleClass('disabled', !activateCursorOption)
                .css('pointer-events', 'all') // so tooltips display regardless
        },

        // gray out / ungray out page content if user is mouse looking around in Interface
        // (otherwise the cursor still interacts with web content...)
        'Keyboard.RightMouseButton': function(localValue, key, value) {
            debugPrint(localValue, '... Keyboard.RightMouseButton:' + value);
            window.active = !value;
        },
    };
    monitors['toggle-advanced-options'] = monitors['ui-toggle-advanced-options'];
    monitorSettings(monitors);
    // disable selection
    // $('input').css('-webkit-user-select', 'none');

    viewportUpdated.connect(lessManager, 'onViewportUpdated');

    setupTooltips();

    $(window).trigger('resize'); // populate viewport

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

function setupTooltips() {
    // extract the tooltip captions
    var tooltips = window.tooltips = {};
    var target = '[id], [data-for], [for]';
    $('.tooltip')
        .removeClass('tooltip').addClass('x-tooltip')
        .each(function() {
            var element = $(this),
                input = $(element.parent().find('input').get(0) ||
                          element.closest('button').get(0));
            id = element.prop('id') || element.data('for') ||
                input.prop('id') || input.data('for');
            assert(id);
            tooltips[id] = this.outerHTML;
        }).hide();
    tooltipManager = new TooltipManager({
        enabled: false,
        testmode: PARAMS.tooltiptest,
        viewport: PARAMS.viewport,
        tooltips: tooltips,
        elements: '#reset-to-defaults, button, input',
    });
    viewportUpdated.connect(tooltipManager, 'updateViewport');
    // tooltips aren't needed right away, so defer initializing for better page load times
    window.setTimeout(tooltipManager.initialize.bind(tooltipManager), 1000);
}

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
            debugPrint('registered placeholder value for setting', id, key);
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
                        debugPrint([context.key, '_onChange', this, 'not triggering _onChange for duplicated value']);
                    }
                    return;
                }
                context.jsonLastValue = jsonCurrentValue;
                var args = [].slice.call(arguments, 0);
                debugPrint('monitorSetting._onChange', context.key, value, [].concat(args).pop());
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

    Object.defineProperty(window, 'active', {
        get: function() {
            return window._active;
        },
        set: function(nv) {
            nv = !!nv;
            window._active = nv;
            debugPrint('window.active == ' + nv);
            if (!nv) {
                document.activeElement && document.activeElement.blur();
                document.body.focus();
                tooltipManager && tooltipManager.disable();
                debugPrint('TOOLTIPS DISABLED');
            } else if (tooltipManager && bridgedSettings) {
                if (bridgedSettings.getValue('ui-enable-tooltips')){
                    tooltipManager.enable();
                    debugPrint('TOOLTIPS RE-ENABLED');
                }
            }
            $('body').toggleClass('active', window._active);
        },
    });
    $('body').toggleClass('active', window._active = true);

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
        debugPrint('viewportUpdated', viewport);
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
            // tooltipManager.closeAll();
        },
        focus: function() {
            log('** FOCUS **');
            $('body').removeClass('window-blurred');
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
            $(document.activeElement).filter('.row').find(':ui-hifiCheckbox,:ui-hifiRadioButton').click();
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
                debugPrint('Mousetrap', method, combo, typeof obj[key]);
                Mousetrap[method](combo, function(evt, combo) {
                    debugPrint('Mousetrap', method, combo);
                    return obj[key].apply(this, arguments);
                });
            });
        });
    }
}

// support the URL having a #node-id (or #debug=1&node-id) hash fragment jumping to that element
function jumpToAnchor(id) {
    id = JSON.stringify(id);
    $('[id='+id+'],[name='+id+']').first().each(function() {
        log('jumpToAnchor', id);
        $(this).show();
        this.scrollIntoView({ behavior: 'smooth' });
    });
};
