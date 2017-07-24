/* eslint-env jquery, browser */
/* eslint-disable comma-dangle, no-empty */
/* global PARAMS, signal, assert, log, debugPrint */

// ----------------------------------------------------------------------------
// manage jquery-tooltipster hover tooltips
var TooltipManager = (function(global) {
    Object.assign(TooltipManager, {
        BASECONFIG: {
            theme: ['tooltipster-noir'],
            side: ['right','top','bottom', 'left'],
            updateAnimation: 'scale',
            delay: [750, 1000],
            distance: { right: 24, left: 8, top: 8, bottom: 8 },
            contentAsHTML: true,
        },
    });

    function TooltipManager(options) {
        assert(options.elements && options.tooltips, 'TooltipManager constructor expects .elements and .tooltips');
        Object.assign(this, {
            instances: [],
            options: options,
            config: Object.assign({}, TooltipManager.BASECONFIG, {
                trigger: !options.testmode ? 'hover' : 'click',
                interactive: options.testmode,
                minWidth: options.viewport && options.viewport.min.width,
                maxWidth: options.viewport && options.viewport.max.width,
            }),
        });
        options.enabled && this.initialize();
    }

    TooltipManager.prototype = {
        constructor: TooltipManager,
        initialize: function() {
            var options = this.options,
                _config = this.config,
                _self = this,
                candidates = $(options.elements);

            candidates.add($('button')).each(function() {
                var id = this.id,
                    input = $(this),
                    tip = options.tooltips[id] || options.tooltips[input.data('for')];

                var alreadyTipped = input.is('.tooltipstered') || input.closest('.tooltipstered').get(0);
                if (alreadyTipped || !tip) {
                    return !tip && _debugPrint('!tooltippable -- missing tooltip for ' + (id || input.data('for') || input.text()));
                }
                var config = Object.assign({ content: tip }, _config);

                function mergeConfig() {
                    var attr = $(this).attr('data-tooltipster'),
                        object = $(this).data('tooltipster');
                    typeof object === 'object' && Object.assign(config, object);
                    attr && Object.assign(config, JSON.parse(attr));
                }
                try {
                    input.parents(':data(tooltipster),[data-tooltipster]').each(mergeConfig);
                    input.each(mergeConfig); // prioritize own settings
                } catch(e) {
                    console.error('error extracting tooltipster data:' + [e, id]);
                }

                var target = $(input.closest('.tooltip-target').get(0) ||
                               (input.is('input') && input) || null);

                assert(target && target[0] && tip);
                debugPrint('binding tooltip', config, target[0].nodeName, id || target[0]);
                var instance = target.tooltipster(config)
                    .tooltipster('instance');

                instance.on('close', function(event) {
                    if (options.keepopen === target) {
                        debugPrint(event.type, 'canceling close keepopen === target', id);
                        event.stop();
                        options.keepopen = null;
                    }
                });
                instance.on('before', function(event) {
                    debugPrint(event.type, 'before', event);
                    !options.testmode && _self.closeAll();
                    !options.enabled && event.stop();
                });
                target.find(':focusable, input, [tabindex], button, .control')
                    .add(target).add(input)
                    .add(input.closest(':focusable, input, [tabindex]'))
                    .on({
                        click: function(evt) {
                            if (input.is('button')) {
                                return setTimeout(instance.close.bind(instance,null),50);
                            }
                            options.keepopen = target;
                        },
                        focus: instance.open.bind(instance, null),
                        blur: function(evt) {
                            instance.close(); _self.openFocusedTooltip();
                        },
                    });
                _self.instances.push(instance);
            });
            return this.instances;
        },
        openFocusedTooltip: function() {
            if (!this.options.enabled) {
                return;
            }
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
                    // log('opening focused tooltip', tip.length, tip[0].id);
                    tip.tooltipster('open');
                }
            }, 1);
        },
        rapidClose: function(instance, reopen) {
            if (!instance.status().open) {
                return;
            }
            instance.elementTooltip() && $(instance.elementTooltip()).hide();
            instance.close(function() {
                reopen && instance.open();
            });
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
                minWidth: viewport.min.width,
                maxWidth: viewport.max.width,
            };
            Object.assign(this.config, options);
            $.tooltipster.setDefaults(options);
            debugPrint('updating tooltipster options', JSON.stringify(options));
            $.tooltipster.instances().forEach(function(instance) {
                instance.option('minWidth', options.minWidth);
                instance.option('maxWidth', options.maxWidth);
                this.rapidClose(instance, instance.status().open);
            }.bind(this));
        },
        enable: function() {
            this.options.enabled = true;
            if (this.options.testmode) {
                this.openAll();
            }
        },
        disable: function() {
            this.options.enabled = false;
            this.closeAll();
        },
    };// prototype

    return TooltipManager;
})(this);
