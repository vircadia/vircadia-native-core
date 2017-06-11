/* eslint-env jquery, browser */
/* eslint-disable comma-dangle, no-empty */
/* global PARAMS, signal, assert, log, debugPrint */

// ----------------------------------------------------------------------------
// manage jquery-tooltipster hover tooltips
var TooltipManager = (function(global) {
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

            debugPrint('binding tooltip', tooltipSide, element[0].nodeName, id || element[0], tip);
            if (!tip) {
                return log('missing tooltip:', this.nodeName, id || this.id || this.name || $(this).text());
            }
            if (element.is('.tooltipstered')) {
                log('already tooltipstered!?', this.id, this.name, id);
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
                distance: element.is('.slider.row') ? -20 : undefined,// element
                delay: [500, 1000],
                contentAsHTML: true,
                interactive: options.testmode,
                minWidth: options.viewport && options.viewport.min.width,
                maxWidth: options.viewport && options.viewport.max.width,
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
                        if (input.is('button')) {
                            return setTimeout(instance.close.bind(instance,null),50);
                        }
                        options.keepopen = element; 0&&instance.open();
                    },
                    focus: instance.open.bind(instance, null),
                    blur: function(evt) {
                        instance.close(); _self.openFocusedTooltip(); 
                    },
                });
        });
        Object.assign(this, {
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
                },1);
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
                $.tooltipster.setDefaults(options);
                log('updating tooltipster options', JSON.stringify(options));
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
        });
    }
    return TooltipManager;
})(this);
