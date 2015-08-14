// 
//  widgets-example.js
//  games 
//  
//  Copyright 2015 High Fidelity, Inc.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var ICONS_URL = 'https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/images/';

var panelX = 1250;
var panelY = 500;
var panelWidth = 50;
var panelHeight = 210;

Script.include('../libraries/uiwidgets.js');

UI.setDefaultVisibility(true);

var ICON_WIDTH = 40.0;
var ICON_HEIGHT = 40.0;
var ICON_COLOR = UI.rgba(45, 45, 45, 0.7);
var FOCUSED_COLOR = UI.rgba(250, 250, 250, 1.0);

var PANEL_BACKGROUND_COLOR = UI.rgba(120, 120, 120, 0.8);

var PANEL_PADDING = 7.0;
var PANEL_BORDER = 12.0;
var SUBPANEL_GAP = 1.0;

var icons = [];

function addImage(panel, iconId) {
    var icon = panel.add(new UI.Image({
        'imageURL': ICONS_URL + iconId + '.svg',
        'width': ICON_WIDTH,
        'height': ICON_HEIGHT,
        'color': ICON_COLOR,
        'alpha': ICON_COLOR.a
    }));
    icons.push(icon);
    return icon;
}


var panels = [];

function addPanel(properties) {
    properties.background = properties.background || {};
    properties.background.backgroundColor = properties.background.backgroundColor ||
        PANEL_BACKGROUND_COLOR;
    properties.background.backgroundAlpha = properties.background.backgroundAlpha ||
        PANEL_BACKGROUND_COLOR.a;
    properties.padding = properties.padding || {
        x: PANEL_PADDING,
        y: PANEL_PADDING
    };
    properties.border = properties.border || {
        x: PANEL_BORDER,
        y: PANEL_BORDER
    };

    var panel = new UI.WidgetStack(properties);
    panels.push(panel);
    return panel;
}

function makeDraggable(panel, target) {
    if (!target) {
        target = panel;
    }
    var dragStart = null;
    var initialPos = null;

    panel.addAction('onDragBegin', function(event) {
        dragStart = {
            x: event.x,
            y: event.y
        };
        initialPos = {
            x: target.position.x,
            y: target.position.y
        };
    });
    panel.addAction('onDragUpdate', function(event) {
        target.setPosition(
            initialPos.x + event.x - dragStart.x,
            initialPos.y + event.y - dragStart.y
        );
        UI.updateLayout();
    });
    panel.addAction('onDragEnd', function() {
        dragStart = dragEnd = null;
    });
}

function setText(text) {
    return function() {
        demoLabel.setText(text);
        UI.updateLayout();
    };
}

function join(obj) {
    var s = "{";
    var sep = "\n";
    for (var k in obj) {
        s += sep + k + ": " + ("" + obj[k]).replace("\n", "\n");
        sep = ",\n";
    }
    if (s.length > 1)
        return s + " }";
    return s + "}";
}

setText = undefined;

var tooltipWidget = new UI.Label({
    text: "<tooltip>",
    width: 500,
    height: 20,
    visible: false
});

function addTooltip(widget, text) {
    widget.addAction('onMouseOver', function(event, widget) {
        tooltipWidget.setVisible(true);
        tooltipWidget.setPosition(widget.position.x + widget.getWidth() + 20, widget.position.y + 10);
        tooltipWidget.setText(text);
        UI.updateLayout();
    });
    widget.addAction('onMouseExit', function() {
        tooltipWidget.setVisible(false);
        UI.updateLayout();
    });
}

var mainPanel = addPanel({
    dir: '+y'
});
makeDraggable(mainPanel);
mainPanel.setPosition(1200, 250);
mainPanel.setVisible(true);

var systemViewButton = addImage(mainPanel, 'solarsystems');
var zoomButton = addImage(mainPanel, 'magnifier');
var satelliteButton = addImage(mainPanel, 'satellite');
var settingsButton = addImage(mainPanel, 'settings');
var stopButton = addImage(mainPanel, 'close');

addTooltip(systemViewButton, "system view");
addTooltip(zoomButton, "zoom");
addTooltip(satelliteButton, "satelite view");
addTooltip(settingsButton, "settings");
addTooltip(stopButton, "exit");

var systemViewPanel = addPanel({
    dir: '+x',
    visible: false
});
var restartButton = addImage(systemViewPanel, 'refresh');
var pauseButton = addImage(systemViewPanel, 'playpause');
var rideButton = addImage(systemViewPanel, 'rocket');

var tweening, tweeningPaused;
Script.include('https://hifi-staff.s3.amazonaws.com/bridget/tween.js');


pauseButton.addAction('onClick', function() {
    if (tweening) {
        if (!tweeningPaused) {
            tweeningPaused = true;
        } else {
            tweeningPaused = false;
        }
        return;
    }
    if (!paused) {
        pause();
    } else {
        resume();
    }
});

// Allow to toggle pause with spacebar
function keyPressEvent(event) {
    if (event.text == "SPACE") {
        if (!paused) {
            pause();
        } else {
            resume();
        }
    }
}

rideButton.addAction('onClick', function() {
    if (!paused) {
        pause();
    }
    if (tweening) {
        tweening = false;
        tweeningPaused = true;
        restart();
        return;
    }
    var confirmed = Window.confirm('Ride through the solar system?');
    if (confirmed) {
        init();
        tweening = true;
        tweeningPaused = false;
    }
});

restartButton.addAction('onClick', function() {
    restart();
    tweening = false;
});

var zoomPanel = addPanel({
    dir: '+x',
    visible: false
});
var zoomButtons = [];
for (var i = 0; i < planets.length; ++i) {
    var label = zoomPanel.add(new UI.Label({
        text: planets[i].name,
        width: 80,
        height: 20
    }));
    zoomButtons.push(label);
    UI.updateLayout();
}
UI.updateLayout();


var zoomView = false;
zoomButtons.forEach(function(button, i) {
    var planet = planets[i];
    button.addAction('onClick', function() {
        if (!planets[i].isZoomed) {
            planet.zoom();
            planet.isZoomed = true;
            zoomView = true;
        } else {
            MyAvatar.position = startingPosition;
            Camera.setPosition(cameraStart);
            planet.isZoomed = false;
            zoomView = false;
        }

    });
});


var settingsPanel = addPanel({
    dir: '+y',
    visible: false
});

function addCheckbox(parent, label, labelWidth, enabled, onValueChanged) {
    var layout = parent.add(new UI.WidgetStack({
        dir: '+x',
        visible: true,
        backgroundAlpha: 0.0
    }));
    var label = layout.add(new UI.Label({
        text: label,
        width: labelWidth,
        height: 20,
        backgroundAlpha: 0.0
    }));

    var defaultColor = UI.rgb(10, 10, 10);

    var checkbox = layout.add(new UI.Checkbox({
        width: 20,
        height: 20,
        padding: {
            x: 3,
            y: 3
        },
        backgroundColor: defaultColor,
        backgroundAlpha: 0.9,
        checked: enabled,
        onValueChanged: onValueChanged
    }));

    checkbox.label = label;
    checkbox.layout = layout;
    checkbox.setValue = function(value) {
        checkbox.setChecked(value);
    }
    return checkbox;
}

function addSlider(parent, label, labelWidth, defaultValue, min, max, valueChanged) {
    var layout = parent.add(new UI.WidgetStack({
        dir: '+x',
        visible: true
    }));
    var label = layout.add(new UI.Label({
        text: label,
        width: labelWidth,
        height: 27
    }));
    var display = layout.add(new UI.Label({
        text: " ",
        width: 50,
        height: 27
    }));
    var slider = layout.add(new UI.Slider({
        value: defaultValue,
        maxValue: max,
        minValue: min,
        width: 300,
        height: 20,
        backgroundColor: UI.rgb(10, 10, 10),
        backgroundAlpha: 1.0,
        slider: { // slider knob
            width: 30,
            height: 18,
            backgroundColor: UI.rgb(120, 120, 120),
            backgroundAlpha: 1.0
        }
    }));
    slider.addAction('onDoubleClick', function() {
        slider.setValue(defaultValue);
        UI.updateLayout();
    });
    display.setText("" + (+slider.getValue().toFixed(2)));
    slider.onValueChanged = function(value) {
        valueChanged(value);
        display.setText("" + (+value.toFixed(2)));
        UI.updateLayout();
    }
    slider.label = label;
    slider.layout = layout;
    return slider;
}

settingsPanel.showTrailsButton = addCheckbox(settingsPanel, "show trails", 120, trailsEnabled, function(value) {
    trailsEnabled = value;
    if (trailsEnabled) {
        for (var i = 0; i < planets.length; ++i) {
            planets[i].resetTrails();
        }
        //if trails are off and we've already created trails, remove existing trails
    } else {
        for (var i = 0; i < planets.length; ++i) {
            planets[i].clearTrails();
        }
    }
});
var g_multiplier = 1.0;
settingsPanel.gravitySlider = addSlider(settingsPanel, "gravity scale ", 200, g_multiplier, 0.0, 5.0, function(value) {
    g_multiplier = value;
    GRAVITY = REFERENCE_GRAVITY * g_multiplier;
});

var period_multiplier = 1.0;
var last_alpha = period_multiplier;

settingsPanel.periodSlider = addSlider(settingsPanel, "orbital period scale ", 200, period_multiplier, 0.0, 3.0, function(value) {
    period_multiplier = value;
    changePeriod(period_multiplier);
});

function changePeriod(alpha) {
    var ratio = last_alpha / alpha;
    GRAVITY = Math.pow(ratio, 2.0) * GRAVITY;
    for (var i = 0; i < planets.length; ++i) {
        planets[i].period = ratio * planets[i].period;
        planets[i].velocity = Vec3.multiply(ratio, planets[i].velocity);
        planets[i].resetTrails();
    }
    last_alpha = alpha;
}

var satelliteGame;
satelliteButton.addAction('onClick', function() {
    if (satelliteGame && satelliteGame.isActive) {
        MyAvatar.position = startingPosition;
        satelliteGame.quitGame();
        if (paused) {
            resume();
        }
    } else {
        pause();
        satelliteGame = new SatelliteCreator();
        satelliteGame.init();
    }
});


var subpanels = [systemViewPanel, zoomPanel, settingsPanel];

function hideSubpanelsExcept(panel) {
    subpanels.forEach(function(x) {
        if (x != panel) {
            x.setVisible(false);
        }
    });
}

function attachPanel(panel, button) {
    button.addAction('onClick', function() {
        hideSubpanelsExcept(panel);
        panel.setVisible(!panel.isVisible());
        UI.updateLayout();
    })

    UI.addAttachment(panel, button, function(target, rel) {
        target.setPosition(
            rel.position.x - (target.getWidth() + target.border.x + SUBPANEL_GAP),
            rel.position.y - target.border.y
        );
    });
}
attachPanel(systemViewPanel, systemViewButton);
attachPanel(zoomPanel, zoomButton);
attachPanel(settingsPanel, settingsButton);


var addColorToggle = function(widget) {
    widget.addAction('onMouseOver', function() {
        widget.setColor(FOCUSED_COLOR);
    });
    widget.addAction('onMouseExit', function() {
        widget.setColor(ICON_COLOR);
    });
}


systemViewPanel.addAction('onMouseOver', function() {
    hideSubpanelsExcept(systemViewPanel);
    UI.updateLayout();
});


zoomButton.addAction('onClick', function() {
    if (zoomView) {
        restart();
    }
    hideSubpanelsExcept(zoomPanel);
    UI.updateLayout();
});
UI.updateLayout();

stopButton.addAction('onClick', function() {
    teardown();
    Script.stop();
});

// Panel drag behavior
// (click + drag on border to drag)
(function() {
    var dragged = null;
    this.startDrag = function(dragAction) {
        dragged = dragAction;
    }
    this.updateDrag = function(event) {
        if (dragged) {
            print("Update drag");
            dragged.updateDrag(event);
        }
    }
    this.clearDrag = function(event) {
        if (dragged)
            print("End drag");
        dragged = null;
    }
})();

var buttons = icons;

buttons.map(addColorToggle);
panels.map(function(panel) {
    makeDraggable(panel, mainPanel);
});

// Cleanup script resources
function teardown() {
    UI.teardown();
    if (satelliteGame) {
        satelliteGame.quitGame();
    }
};


UI.debug.setVisible(false);

var inputHandler = {
    onMouseMove: function(event) {
        updateDrag(event);
        UI.handleMouseMove(event);
    },
    onMousePress: function(event) {
        UI.handleMousePress(event);
    },
    onMouseRelease: function(event) {
        clearDrag(event);
        UI.handleMouseRelease(event);
    },
    onMouseDoublePress: function(event) {
        UI.handleMouseDoublePress(event);
    }
};
Controller.mousePressEvent.connect(inputHandler.onMousePress);
Controller.mouseMoveEvent.connect(inputHandler.onMouseMove);
Controller.mouseReleaseEvent.connect(inputHandler.onMouseRelease);
Controller.mouseDoublePressEvent.connect(inputHandler.onMouseDoublePress);

Controller.keyPressEvent.connect(keyPressEvent);

Script.scriptEnding.connect(teardown);