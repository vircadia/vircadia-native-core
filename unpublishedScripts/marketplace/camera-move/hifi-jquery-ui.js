// extended jQuery UI controls

/* eslint-env console, jquery, browser */
/* eslint-disable comma-dangle, no-empty */
/* global assert, log, debugPrint */
// ----------------------------------------------------------------------------

// WIDGET BASE
Object.assign($.Widget.prototype, {
    // common bootstrapping across widget types
    initHifiControl: function initHifiControl(hifiType) {
        initHifiControl.widgetCount = (initHifiControl.widgetCount || 0) + 1;
        hifiType = hifiType || this.widgetName;

        var element = this.element, options = this.options, node = element.get(0), dataset = node.dataset;
        assert(!this.element.is('.initialized'));
        this.element.addClass('initialized');
        var attributes = [].reduce.call(node.attributes, function(out, attribute) {
            out[attribute.name] = attribute.value;
            return out;
        }, {});

        var searchOrder = [ options, dataset, attributes, node ];
        function setData(key, fallback) {
            var value = searchOrder.map(function(obj) {
                return obj[key];
            }).concat(fallback).filter(function(value) {
                return value !== undefined;
            })[0];
            return value === undefined ? null : (dataset[key] = value);
        }
        options.hifiWidgetId = hifiType + '-' + initHifiControl.widgetCount;
        node.id = node.id || options.hifiWidgetId;
        dataset.hifiType = hifiType;
        setData('type');
        setData('for', node.id);
        setData('checked');
        if (setData('value', null) !== null) {
            element.attr('value', dataset.value);
        }

        return node.id;
    },
    hifiFindWidget: function(hifiType, quiet) {
        var selector = ':ui-'+hifiType;
        var _for = JSON.stringify(this.element.data('for')||undefined),
            element = _for && $('[id='+_for+']').filter(selector);
        if (!element.is(selector)) {
            element = this.element.closest(selector);
        }
        var instance = element.filter(selector)[hifiType]('instance');

        if (!instance && !quiet) {
            // eslint-disable-next-line no-console
            console.error([
                instance, 'could not find target instance ' + selector +
                    ' for ' + this.element.data('hifi-type') +
                    ' #' + this.element.prop('id') + ' for=' + this.element.data('for')
            ]);
        }
        return instance;
    },
});

// CHECKBOX
$.widget('ui.hifiCheckbox', $.ui.checkboxradio, {
    value: function value(nv) {
        if (arguments.length) {
            var currentValue = this.element.prop('checked');
            if (nv !== currentValue){
                this.element.prop('checked', nv);
                this.element.change();
            }
        }
        return this.element.prop('checked');
    },
    _create: function() {
        var id = this.initHifiControl();
        this.element.attr('value', id);
        // add an implicit label if missing
        var forId = 'for=' + JSON.stringify(id);
        var label = $(this.element.get(0)).closest('label').add($('label[' + forId + ']'));
        if (!label.get(0)) {
            $('<label ' + forId + '>' + forId + '</label>').appendTo(this.element);
        }
        this._super();
        this.element.on('change._hifiCheckbox, click._hifiCheckbox', function() {
            var checked = this.value(),
                attr = this.element.attr('checked');
            if (checked && !attr) {
                this.element.attr('checked', 'checked');
            } else if (!checked && attr) {
                this.element.removeAttr('checked');
            }
            this.refresh();
        }.bind(this));
    },
});

// BUTTON
$.widget('ui.hifiButton', $.ui.button, {
    value: function(nv) {
        var dataset = this.element[0].dataset;
        if (arguments.length) {
            var checked = (dataset.checked === 'true');
            nv = (nv === 'true' || !!nv);
            if (nv !== checked) {
                debugPrint('hifibutton checked changed', nv, checked);
                dataset.checked = nv;
                this.element.change();
            } else {
                debugPrint('hifibutton value same', nv, checked);
            }
        }
        return dataset.checked === 'true';
    },
    _create: function() {
        this.element.data('type', 'checkbox');
        this.initHifiControl();
        this._super();
        this.element[0].dataset.checked = !!this.element.attr('checked');
        var _for = this.element.data('for') || undefined;
        if (_for && _for !== this.element[0].id) {
            _for = JSON.stringify(_for);
            var checkbox = this.hifiFindWidget('hifiCheckbox', true);
            if (!checkbox) {
                var input = $('<label><input type=checkbox id=' + _for + ' value=' + _for +' /></label>').hide();
                input.appendTo(this.element);
                checkbox = input.find('input')
                    .hifiCheckbox()
                    .hifiCheckbox('instance');
            }
            this.element.find('.tooltip-target').removeClass('tooltip-target');
            this.element.prop('id', 'button-'+this.element.prop('id'));
            checkbox.element.on('change._hifiButton', function() {
                debugPrint('checkbox -> button');
                this.value(checkbox.value());
            }.bind(this));
            this.element.on('change', function() {
                debugPrint('button -> checkbox');
                checkbox.value(this.value());
            }.bind(this));
            this.checkbox = checkbox;
        }
    },
});

// RADIO BUTTON
$.widget('ui.hifiRadioButton', $.ui.checkboxradio, {
    value: function value(nv) {
        if (arguments.length) {
            this.element.prop('checked', !!nv);
            this.element.change();
        }
        return this.element.prop('checked');
    },
    _create: function() {
        var id = this.initHifiControl();
        this.element.attr('value', this.element.data('value') || id);
        // console.log(this.element[0]);
        assert(this.element.data('for'));
        this._super();

        this.element.on('change._hifiRadioButton, click._hifiRadioButton', function() {
            var group = this.hifiFindWidget('hifiRadioGroup'),
                checked = !!this.element.attr('checked'),
                dotchecked = this.element.prop('checked'),
                value = this.element.attr('value');

            if (dotchecked !== checked || group.value() !== value) {
                if (dotchecked && group.value() !== value) {
                    log(value, 'UPDATING GRUOP', group.element[0].id);
                    group.value(value);
                }
            }
        }.bind(this));
    },
});

// RADIO GROUP
$.widget('ui.hifiRadioGroup', $.ui.controlgroup, {
    radio: function(selector) {
        return this.element.find(':ui-hifiRadioButton' + selector).hifiRadioButton('instance');
    },
    refresh: function() {
        var value = this.value();
        this.element.find(':ui-hifiRadioButton').each(function() {
            $(this).prop('checked', $(this).attr('value') === value).hifiRadioButton('refresh');
        });
        this._super();
    },
    value: function value(nv) {
        if (arguments.length) {
            var id = this.element[0].id,
                previous = this.value();
            debugPrint('RADIOBUTTON GROUP value', id + ' = ' + nv + '(was: ' + previous + ')');
            this.element.attr('value', nv);
            this.refresh();
        }
        return this.element.attr('value');
    },
    _create: function(x) {
        debugPrint('ui.hifiRadioGroup._create', this.element[0]);
        this.initHifiControl();
        this.options.items = {
            hifiRadioButton: 'input[type=radio]',
        };
        this._super();
        // allow setting correct radio button by assign to .value property (or $.fn.val() etc.)
        Object.defineProperty(this.element[0], 'value', {
            set: function(nv) {
                try {
                    this.radio('#' + nv).value(true);
                } catch (e) {}
                return this.value();
            }.bind(this),
            get: function() {
                return this.element.attr('value');
            }.bind(this),
        });
    },
});

// SPINNER (numeric input + up/down buttons)
$.widget('ui.hifiSpinner', $.ui.spinner, {
    value: function value(nv) {
        if (arguments.length) {
            var num = parseFloat(nv);
            debugPrint('ui.hifiSpinner.value set', this.element[0].id, num, '(was: ' + this.value() + ')', 'raw:'+nv);
            this._value(num);
            this.element.change();
        }
        return parseFloat(this.element.val());
    },
    _value: function(value, allowAny) {
        debugPrint('ui.hifiSpinner._value', value, allowAny);
        return this._super(value, allowAny);
    },
    _create: function() {
        this.initHifiControl();
        var step = this.options.step = this.options.step || 1.0;
        // allow step=".01" for precision and data-step=".1" for default increment amount
        this.options.prescale = parseFloat(this.element.data('step') || step) / (step);
        this._super();
        this.previous = null;
        this.element.on('change._hifiSpinner', function() {
            var value = this.value(),
                invalid = !this.isValid();
            debugPrint('hifiSpinner.changed', value, invalid ? '!!!invalid' : 'valid');
            !invalid && this.element.attr('value', value);
        }.bind(this));
    },
    _spin: function( step, event ) {
        step = step * this.options.prescale * (
            event.shiftKey ? 0.1 : event.ctrlKey ? 10 : 1
        );
        return this._super( step, event );
    },
    _stop: function( event, ui ) {
        try {
            return this._super(event, ui);
        } finally {
            if (/mouse/.test(event && event.type)) {
                var value = this.value();
                if ((value || value === 0) && !isNaN(value) && this.previous !== null && this.previous !== value) {
                    this.value(this.value());
                }
                this.previous = value;
            }
        }
    },
    _format: function(n) {
        var precision = this._precision();
        return parseFloat(n).toFixed(precision);
    },
    _events: {
        mousewheel: function(event, delta) {
            if (document.activeElement === this.element[0]) {
                // fix broken mousewheel on Chrome / embedded webkit
                delta = delta === undefined ? -(event.originalEvent.deltaY+event.originalEvent.deltaX) : delta;
                $.ui.spinner.prototype._events.mousewheel.call(this, event, delta);
            }
        }
    }
});

// SLIDER
$.widget('ui.hifiSlider', $.ui.slider, {
    value: function value(nv) {
        if (arguments.length) {
            var num = this._trimAlignValue(nv);
            debugPrint('hifiSlider.value', nv, num);
            if (this.options.value !== num) {
                this.options.value = num;
                this.element.change();
            }
        }
        return this.options.value;
    },
    _create: function() {
        this.initHifiControl();
        this._super();
        this.element
            .attr('type', this.element.attr('type') || 'slider')
            .find('.ui-slider-handle').html('<div class="inner-ui-slider-handle"></div>').end()
            .on('change', function() {
                this.hifiFindWidget('hifiSpinner').value(this.value());
                this._refresh();
            }.bind(this));
    },
});
