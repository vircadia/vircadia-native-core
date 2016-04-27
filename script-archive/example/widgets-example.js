// 
//  widgets-example.js
//  games 
//  
//  Copyright 2015 High Fidelity, Inc.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var paddingX = 8;
var paddingY = 8;
var buttonWidth = 30;
var buttonHeight = 30;

var ICONS_URL = 'https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/images/';

var panelX = 1250;
var panelY = 500;
var panelWidth = 50;
var panelHeight = 210;

// var mainPanel = new UIPanel(panelX, panelY, panelWidth, panelHeight);
// var systemViewButton = mainPanel.addImage('solarsystems');
// var zoomButton = mainPanel.addImage('magnifier');
// var satelliteButton = mainPanel.addImage('satellite');
// var settingsButton = mainPanel.addImage('settings');
// var stopButton = mainPanel.addImage('close');
// 
// mainPanel.show();
// 
// var systemViewPanel = new UIPanel(panelX - 120, panelY, 120, 40);
// var reverseButton = systemViewPanel.addImage('reverse');
// var pauseButton = systemViewPanel.addImage('playpause');
// var forwardButton = systemViewPanel.addImage('forward');
// 
// var zoomPanel = new UIPanel(panelX - 60, panelY + buttonHeight + paddingY, 650, 50);
// for (var i = 0; i < planets.length; ++i) {
    // zoomPanel.addText(planets[i].name);
// }
Script.include('../libraries/uiwidgets.js');

UI.setDefaultVisibility(true);
UI.setErrorHandler(function(err) {
    teardown();
    // print(err);
    // Script.stop();
});

// Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) { return settings.mouseMoveEvent(event); });
//     Controller.mousePressEvent.connect( function panelMousePressEvent(event) { return settings.mousePressEvent(event); });
//     Controller.mouseDoublePressEvent.connect( function panelMouseDoublePressEvent(event) { return settings.mouseDoublePressEvent(event); });
//     Controller.mouseReleaseEvent.connect(function(event) { return settings.mouseReleaseEvent(event); });
//     Controller.keyPressEvent.connect(function(event) { return settings.keyPressEvent(event); });

// var ICON_WIDTH  = 50.0;
// var ICON_HEIGHT = 50.0;
var ICON_WIDTH = 40.0;
var ICON_HEIGHT = 40.0;
var ICON_COLOR = UI.rgba(45, 45, 45, 0.7);
var FOCUSED_COLOR = UI.rgba(250, 250, 250, 1.0);

var PANEL_BACKGROUND_COLOR = UI.rgba(50, 50, 50, 0.7);

var PANEL_PADDING = 7.0;
var PANEL_BORDER = 12.0;
var SUBPANEL_GAP = 1.0;

var icons = [];
function addImage(panel, iconId) {
    var icon = panel.add(new UI.Image({
        'imageURL': ICONS_URL + iconId + '.svg',
        'width':  ICON_WIDTH,
        'height': ICON_HEIGHT,
        'color':  ICON_COLOR,
        'alpha':  ICON_COLOR.a
    }));
    icons.push(icon);
    return icon;
}

var panels = [];
function addPanel (properties) {
    properties.background = properties.background || {};
    properties.background.backgroundColor = properties.background.backgroundColor ||
        PANEL_BACKGROUND_COLOR;
    properties.background.backgroundAlpha = properties.background.backgroundAlpha ||
        PANEL_BACKGROUND_COLOR.a;
    properties.padding = properties.padding || { x: PANEL_PADDING, y: PANEL_PADDING };
    properties.border = properties.border || { x: PANEL_BORDER, y: PANEL_BORDER };

    var panel = new UI.WidgetStack(properties);
    panels.push(panel);
    return panel;
}

function makeDraggable (panel, target) {
    if (!target)
        target = panel;

    var dragStart = null;
    var initialPos = null;

    panel.addAction('onDragBegin', function (event) {
        dragStart = { x: event.x, y: event.y };
        initialPos = { x: target.position.x, y: target.position.y };
    });
    panel.addAction('onDragUpdate', function (event) {
        target.setPosition(
            initialPos.x + event.x - dragStart.x,
            initialPos.y + event.y - dragStart.y
        );
        UI.updateLayout();
    });
    panel.addAction('onDragEnd', function () {
        dragStart = dragEnd = null;
    });
}

// var panelContainer = new UI.WidgetContainer();
// panelContainer.setPosition(500, 250);
// panelContainer.setVisible(true);

var demoPane = addPanel({ dir: '+y' });
var demoLabel = demoPane.add(new UI.Label({
    text: "< no events >",
    width: 400, height: 20
}));
var demoButton = demoPane.add(new UI.Box({
    width: 200, height: 80,
    text: "Button"
}));
function setText(text) {
    return function () {
        demoLabel.setText(text);
        UI.updateLayout();
    };
}
function addDebugActions(widget, msg, actions) {
    actions.forEach(function(action) {
        widget.addAction(action, setText(action + " " + msg + widget));
    });
}

var debugEvents = [ 
    'onMouseOver', 
    'onMouseExit', 
    'onMouseDown', 
    'onMouseUp',
    'onDragBegin',
    'onDragEnd',
    'onDragUpdate'
];
addDebugActions(demoPane, "(container) ", debugEvents);
addDebugActions(demoButton, "(button) ", debugEvents);
addDebugActions(demoLabel, "(label) ", debugEvents);

// demoPane.addAction('onMouseOver', setText("onMouseOver " + demoPane));
// demoPane.addAction('onMouseExit', setText("onMouseExit " + demoPane));
// demoPane.addAction('onMouseDown', setText("onMouseDown " + demoPane));
// demoPane.addAction('onMouseUp', setText("onMouseUp " + demoPane));
makeDraggable(demoPane, demoPane);
demoPane.setPosition(600, 200);

// demoButton.addAction('onMouseOver', setText("onMouseOver " + demoButton));
// demoButton.addAction('onMouseExit', setText("onMouseExit " + demoButton));
// demoButton.addAction()

// var resizablePanel = new UI.Label({
//     text: "Resizable panel",
//     width: 200, height: 200,
//     backgroundAlpha: 0.5
// });
// resizablePanel.setPosition(1100, 200);

var debugToggle = new UI.Box({
    text: "debug", width: 150, height: 20
});
debugToggle.setPosition(200, 0);
debugToggle.addAction('onClick', function () {
    UI.debug.setVisible(!UI.debug.isVisible());
});

// debugEvents.forEach(function (action) {
//     resizablePanel.addAction(action, function (event, widget) {
//         widget.setText(action + " " + widget);
//     });
// })

function join(obj) {
    var s = "{";
    var sep = "\n";
    for (var k in obj) {
        s += sep + k + ": " + (""+obj[k]).replace("\n", "\n");
        sep = ",\n";
    }
    if (s.length > 1)
        return s + " }";
    return s + "}";
}

// resizablePanel.getOverlay().update({
//     text: "" + join(resizablePanel.actions)
// });


setText = addDebugActions = undefined;


var tooltipWidget = new UI.Label({
    text: "<tooltip>",
    width: 500, height: 20,
    visible: false
});
function addTooltip (widget, text) {
    widget.addAction('onMouseOver', function (event, widget) {
        tooltipWidget.setVisible(true);
        tooltipWidget.setPosition(widget.position.x + widget.getWidth() + 20, widget.position.y);
        tooltipWidget.setText(text);
        UI.updateLayout();
    });
    widget.addAction('onMouseExit', function () {
        tooltipWidget.setVisible(false);
        UI.updateLayout();
    });
}

var mainPanel = addPanel({ dir: '+y' });
mainPanel.setPosition(500, 250);
mainPanel.setVisible(true);
    
var systemViewButton = addImage(mainPanel, 'solarsystems');
var zoomButton       = addImage(mainPanel, 'magnifier');
var satelliteButton  = addImage(mainPanel, 'satellite');
var settingsButton   = addImage(mainPanel, 'settings');
var stopButton       = addImage(mainPanel, 'close');


addTooltip(systemViewButton, "system view");
addTooltip(zoomButton, "zoom");
addTooltip(satelliteButton, "satelite view");
addTooltip(settingsButton, "settings");
addTooltip(stopButton, "exit");

var systemViewPanel = addPanel({ dir: '+x', visible: false });
var reverseButton   = addImage(systemViewPanel, 'reverse');
var pauseButton     = addImage(systemViewPanel, 'playpause');
var forwardButton   = addImage(systemViewPanel, 'forward');

var zoomPanel = addPanel({ dir: '+y', visible: true });
var label = new UI.Label({
    text: "Foo",
    width: 120,
    height: 15,
    color: UI.rgb(245, 290, 20),
    alpha: 1.0,
    backgroundColor: UI.rgb(10, 10, 10),
    backgroundAlpha: 0.0
});
zoomPanel.add(label);
label.addAction('onMouseOver', function () {
    label.setText("Bar");
    UI.updateLayout();
});
label.addAction('onMouseExit', function () {
    label.setText("Foo");
    UI.updateLayout();
});
label.setText("Label id: " + label.id + ", parent id " + label.parent.id);
label.parent.addAction('onMouseOver', function () {
    label.setText("on parent");
    UI.updateLayout();
});
label.parent.addAction('onMouseExit', function () {
    label.setText('exited parent');
    UI.updateLayout();
});

var sliderLayout = zoomPanel.add(new UI.WidgetStack({
    dir: '+x', visible: true, backgroundAlpha: 0.0
}));
var sliderLabel = sliderLayout.add(new UI.Label({
    text: " ", width: 45, height: 20
}));
var slider = sliderLayout.add(new UI.Slider({
    value: 10, maxValue: 100, minValue: 0,
    width: 300, height: 20,
    backgroundColor: UI.rgb(10, 10, 10),
    backgroundAlpha: 1.0,
    slider: {   // slider knob
        width: 30,
        height: 18,
        backgroundColor: UI.rgb(120, 120, 120),
        backgroundAlpha: 1.0
    }
}));
sliderLabel.setText("" + (+slider.getValue().toFixed(1)));
slider.onValueChanged = function (value) {
    sliderLabel.setText("" + (+value.toFixed(1)));
    UI.updateLayout();
}




var checkBoxLayout = zoomPanel.add(new UI.WidgetStack({
    dir: '+x', visible: true, backgroundAlpha: 0.0
}));
// var padding = checkBoxLayout.add(new UI.Label({
//     text: " ", width: 45, height: 20
// }));
var checkBoxLabel = checkBoxLayout.add(new UI.Label({
    text: "set red", width: 60, height: 20,
    backgroundAlpha: 0.0
}));
checkBoxLabel.setText("set red");

var defaultColor = UI.rgb(10, 10, 10);
var redColor = UI.rgb(210, 80, 80);

var checkbox = checkBoxLayout.add(new UI.Checkbox({
    width: 20, height: 20, padding: { x: 3, y: 3 },
    backgroundColor: defaultColor,
    backgroundAlpha: 0.9,
    checked: false,
    onValueChanged: function (red) {
         zoomPanel.getOverlay().update({
        // backgroundAlpha: 0.1,
        backgroundColor: red ? redColor : defaultColor
    });
}
}));

addImage(zoomPanel, 'reverse');
UI.updateLayout();

var subpanels = [ systemViewPanel, zoomPanel ];
function hideSubpanelsExcept (panel) {
    subpanels.forEach(function (x) {
        if (x != panel) {
            x.setVisible(false);
        }
    });
}

function attachPanel (panel, button) {
    button.addAction('onClick', function () {
        hideSubpanelsExcept(panel);
        panel.setVisible(!panel.isVisible());
        UI.updateLayout();
    })

    UI.addAttachment(panel, button, function (target, rel) {
        target.setPosition(
            rel.position.x - (target.getWidth() + target.border.x + SUBPANEL_GAP),
            rel.position.y - target.border.y
        );
    });
}
attachPanel(systemViewPanel, systemViewButton);
attachPanel(zoomPanel, zoomButton);

var addColorToggle = function (widget) {
    widget.addAction('onMouseOver', function () {
        widget.setColor(FOCUSED_COLOR);
    });
    widget.addAction('onMouseExit', function () {
        widget.setColor(ICON_COLOR);
    });
}

reverseButton.addAction('onClick', function() {});

systemViewPanel.addAction('onMouseOver', function() {
    hideSubpanels();
    UI.updateLayout();
});


zoomButton.addAction('onClick', function() {
    hideSubpanels();
    UI.updateLayout();
});
UI.updateLayout();

stopButton.addAction('onClick', function() {
    // Script.stop();
    teardown();
});

// Panel drag behavior
// (click + drag on border to drag)
(function () {
    var dragged = null;
    this.startDrag = function (dragAction) {
        dragged = dragAction;
    }
    this.updateDrag = function (event) {
        if (dragged) {
            print("Update drag");
            dragged.updateDrag(event);
        }
    }
    this.clearDrag = function (event) {
        if (dragged)
            print("End drag");
        dragged = null;
    }
})();

var buttons = icons;

buttons.map(addColorToggle);
panels.map(function (panel) { makeDraggable(panel, mainPanel); });

// Cleanup script resources
function teardown() {
    UI.teardown();

    // etc...
};

var inputHandler = {
    onMouseMove: function (event) {
        updateDrag(event);
        UI.handleMouseMove(event);
    },
    onMousePress: function (event) {
        UI.handleMousePress(event);
    },
    onMouseRelease: function (event) {
        clearDrag(event);
        UI.handleMouseRelease(event);
    }
};
Controller.mousePressEvent.connect(inputHandler.onMousePress);
Controller.mouseMoveEvent.connect(inputHandler.onMouseMove);
Controller.mouseReleaseEvent.connect(inputHandler.onMouseRelease);

Script.scriptEnding.connect(teardown);
