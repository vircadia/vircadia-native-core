// extended jQuery UI controls

/* eslint-env console, jquery, browser */
/* eslint-disable comma-dangle, no-empty */
/* global assert, log, debugPrint */
// ----------------------------------------------------------------------------


// WIDGET BASE
Object.assign($.Widget.prototype, {
    initHifiControl: function(hifiType) {
        hifiType = hifiType || this.widgetName;
        var type = this.element.attr('data-type') || this.element.attr('type') || type,
            id = this.element.prop('id') || hifiType + '-' + ~~(Math.random()*1e20).toString(36),
            fer = this.element.attr('for'),
            value = this.element.attr('data-value') || this.element.attr('value');

        this.element.prop('id', id);
        this.element.attr('data-type', type);
        this.element.attr('data-hifi-type', hifiType || 'wtf');
        value && this.element.attr('value', value);
        fer && this.element.attr('data-for', fer);
        return id;
    },
    hifiFindWidget: function(hifiType, quiet) {
        var selector = ':ui-'+hifiType;
        // first try based on for property (eg: "[id=translation-ease-in]:ui-hifiSpinner")
        var fer = this.element.attr('data-for'),
            element = fer && $('[id='+fer+']').filter(selector);
        if (!element.is(selector)) {
            element = this.element.closest(selector);
        }
        var instance = element.filter(selector)[hifiType]('instance');

        if (!instance && !quiet) {
            // eslint-disable-next-line no-console
            console.error([
                instance, 'could not find target instance ' + selector +
                    ' for ' + this.element.attr('data-hifi-type') +
                    ' #' + this.element.prop('id') + ' for=' + this.element.attr('data-for')
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
                // this.refresh();
            }
        }
        return this.element.prop('checked');
    },
    _create: function() {
        var id = this.initHifiControl();

        this.element.attr('value', id);
        // add an implicit label if missing
        var forId = 'for=' + id;
        var label = $('label[' + forId + ']');
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
        // log('ui.hifiButton.value', this.element[0].id, nv);
        var dataset = this.element[0].dataset;
        if (arguments.length) {
            var checked = (dataset.checked === 'true');
            nv = (nv === 'true' || !!nv);
            if (nv !== checked) {
                log('hifibutton checked changed', nv, checked);
                dataset.checked = nv;
                this.element.change();
            } else {
                log('hifibutton value same', nv, checked);
            }
        }
        return dataset.checked === 'true';
    },
    _create: function() {
        this.element[0].dataset.type = 'checkbox';
        this.initHifiControl();
        this._super();

        this.element[0].dataset.checked = !!this.element.attr('checked');
        var fer = this.element.attr('data-for');
        if (fer) {
            var checkbox = this.hifiFindWidget('hifiCheckbox', true);
            if (!checkbox) {
                var input = $('<label><input type=checkbox id=' + fer + ' value=' + fer +' /></label>').hide();
                input.appendTo(this.element.parent());
                checkbox = input.find('input')
                    .hifiCheckbox()
                    .hifiCheckbox('instance');
            }
            checkbox.element.on('change._hifiButton', function() {
                log('checkbox -> button');
                this.value(checkbox.value());
            }.bind(this));
            this.element.on('change', function() {
                log('button -> checkbox');
                checkbox.value(this.value());
            }.bind(this));
            this.checkbox = checkbox;
        }
    },
});

$.widget('ui.hifiRadioButton', $.ui.checkboxradio, {
    value: function value(nv) {
        if (arguments.length) {
            log('RADIOBUTTON VALUE', nv);
            this.element.prop('checked', !!nv);
            this.element.change();
            // this.refresh();
        }
        return this.element.prop('checked');
    },
    _create: function() {
        var id = this.initHifiControl();
        this.element.attr('value', id);
        // console.log(this.element[0]);
        assert(this.element.attr('data-for'));
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

// RADIO-GROUP
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
            log('RADIOBUTTON GROUP value', id + ' = ' + nv + '(was: ' + previous + ')');
            this.element.attr('value', nv);
            this.refresh();
        }
        return this.element.attr('value');
    },
    _create: function(x) {
        debugPrint('ui.hifiRadioGroup._create', this.element[0]);
        this.initHifiControl();

        var tmp = this.options.items.checkboxradio;
        delete this.options.items.checkboxradio;
        this.options.items.hifiRadioButton = tmp;

        this._super();
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

// SPINNER (numeric input w/ up/down arrows)
$.widget('ui.hifiSpinner', $.ui.spinner, {
    value: function value(nv) {
        if (arguments.length) {
            var num = parseFloat(nv);
            debugPrint('ui.hifiSpinner.value set', num, '(was: ' + this.value() + ')', 'raw:'+nv);
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
        if (this.options.min === '-Infinity') {
            this.options.min = -1e10;
        }
        if (this.options.max === '-Infinity') {
            this.options.max = 1e10;
        }
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
                var value = this.value();
                if ((value || value === 0) && !isNaN(value) && this.previous !== null && this.previous !== value) {
                    debugPrint(this.element[0].id, 'spinner.changed', event.type, JSON.stringify({
                        previous: isNaN(this.previous) ? this.previous+'' : this.previous,
                        val: isNaN(value) ? value+'' : value,
                    }));
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
            if (document.activeElement !== this.element[0]) {
                return;
            }
            // fix broken mousewheel on Chrome / webkit
            delta = delta === undefined ? event.originalEvent.deltaY : delta;
            $.ui.spinner.prototype._events.mousewheel.call(this, event, delta);
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
        assert(this.element.attr('data-for'));
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
