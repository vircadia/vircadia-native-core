// 
//  solarsystem.js
//  games 
//  
//  Created by Bridget Went, 5/28/15.
//  Copyright 2015 High Fidelity, Inc.
//  
//  The start to a project to build a virtual physics classroom to simulate the solar system, gravity, and orbital physics. 
//      - A sun with oribiting planets is created in front of the user
//      - UI elements allow for adjusting the period, gravity, trails, and energy recalculations
//      - Click "PAUSE" to pause the animation and show planet labels
//      - In this mode, double-click a planet label to zoom in on that planet
//              -Double-clicking on earth label initiates satellite orbiter game
//              -Press "TAB" to toggle back to solar system view
//
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Script.include([
//     'games/satellite.js'
// ]);

var DAMPING = 0.0;
var LIFETIME = 6000;
var BASE_URL = "https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/planets/";

// Save intiial avatar and camera position
var startingPosition = {
    x: 8000,
    y: 8000,
    z: 8000
};
MyAvatar.position = startingPosition;
var cameraStart = Camera.getOrientation();


// Place the sun
var MAX_RANGE = 80.0;
var center = Vec3.sum(startingPosition, Vec3.multiply(MAX_RANGE, Vec3.normalize(Quat.getFront(Camera.getOrientation()))));
var SUN_SIZE = 7.0;

var theSun = Entities.addEntity({
    type: "Model",
    modelURL: BASE_URL + "sun.fbx",
    position: center,
    dimensions: {
        x: SUN_SIZE,
        y: SUN_SIZE,
        z: SUN_SIZE
    },
    angularVelocity: {
        x: 0.0,
        y: 0.1,
        z: 0.0
    },
    angularDamping: DAMPING,
    damping: DAMPING,
    ignoreCollisions: false,
    lifetime: LIFETIME,
    collisionsWillMove: false
});



// Reference values for physical constants
var referenceRadius = 15.0;
var referenceDiameter = 0.6;
var referencePeriod = 3.0;
var LARGE_BODY_MASS = 250.0;
var SMALL_BODY_MASS = LARGE_BODY_MASS * 0.000000333;
var GRAVITY = (Math.pow(referenceRadius, 3.0) / Math.pow((referencePeriod / (2.0 * Math.PI)), 2.0)) / LARGE_BODY_MASS;
var REFERENCE_GRAVITY = GRAVITY;

var planets = [];
var planetCount = 0;

var TRAILS_ENABLED = false;
var MAX_POINTS_PER_LINE = 10;
var LINE_DIM = 200;
var LINE_WIDTH = 20;

var VELOCITY_OFFSET_Y = -0.3;
var VELOCITY_OFFSET_Z = 0.9;

var index = 0;

var Planet = function(name, trailColor, radiusScale, sizeScale) {

    this.name = name;
    this.trailColor = trailColor;

    this.trail = [];
    this.lineStack = [];

    this.radius = radiusScale * referenceRadius;
    this.position = Vec3.sum(center, {
        x: this.radius,
        y: 0.0,
        z: 0.0
    });
    this.period = (2.0 * Math.PI) * Math.sqrt(Math.pow(this.radius, 3.0) / (GRAVITY * LARGE_BODY_MASS));
    this.gravity = GRAVITY;
    this.initialVelocity = Math.sqrt((GRAVITY * LARGE_BODY_MASS) / this.radius);
    this.velocity = Vec3.multiply(this.initialVelocity, Vec3.normalize({
        x: 0,
        y: VELOCITY_OFFSET_Y,
        z: VELOCITY_OFFSET_Z
    }));
    this.dimensions = sizeScale * referenceDiameter;

    this.planet = Entities.addEntity({
        type: "Model",
        modelURL: BASE_URL + name + ".fbx",
        position: this.position,
        dimensions: {
            x: this.dimensions,
            y: this.dimensions,
            z: this.dimensions
        },
        velocity: this.velocity,
        angularDamping: DAMPING,
        damping: DAMPING,
        ignoreCollisions: false,
        lifetime: LIFETIME,
        collisionsWillMove: true,
    });

    this.computeAcceleration = function() {
        var acc = -(this.gravity * LARGE_BODY_MASS) * Math.pow(this.radius, (-2.0));
        return acc;
    };

    this.update = function(timeStep) {
        var between = Vec3.subtract(this.position, center);
        var speed = this.computeAcceleration(this.radius) * timeStep;
        var vel = Vec3.multiply(speed, Vec3.normalize(between));

        // Update velocity and position
        this.velocity = Vec3.sum(this.velocity, vel);
        this.position = Vec3.sum(this.position, Vec3.multiply(timeStep, this.velocity));
        Entities.editEntity(this.planet, {
            velocity: this.velocity,
            position: this.position
        });

        if (TRAILS_ENABLED) {
            this.updateTrail();
        }
    };

    this.resetTrails = function() {
        elapsed = 0.0;

        this.trail = [];
        this.lineStack = [];
        //add the first line to both the line entity stack and the trail
        this.trail.push(newLine(this.lineStack, this.position, this.period, this.trailColor));

    };

    this.updateTrail = function() {
        var point = this.position;
        var linePos = Entities.getEntityProperties(this.lineStack[this.lineStack.length - 1]).position;

        this.trail.push(computeLocalPoint(linePos, point));

        Entities.editEntity(this.lineStack[this.lineStack.length - 1], {
            linePoints: this.trail
        });

        if (this.trail.length == MAX_POINTS_PER_LINE) {
            this.trail = newLine(this.lineStack, point, this.period, this.trailColor);
        }
    };

    this.zoom = function() {
        var offset = {
            x: 0.0,
            y: 0.0,
            z: 1.2 * sizeScale,
        };
        MyAvatar.position = Vec3.sum(offset, this.position);
    };

    this.adjustPeriod = function(alpha) {
        // Update global G constant, period, poke velocity to new value
        var ratio = this.last_alpha / alpha;
        this.gravity = Math.pow(ratio, 2.0) * GRAVITY;
        this.period = ratio * this.period;
        this.velocity = Vec3.multiply(ratio, this.velocity);

        this.last_alpha = alpha;
    };


    index++;
    this.resetTrails();

}


var MERCURY_LINE_COLOR = {
    red: 255,
    green: 255,
    blue: 255
};
var VENUS_LINE_COLOR = {
    red: 255,
    green: 160,
    blue: 110
};
var EARTH_LINE_COLOR = {
    red: 10,
    green: 150,
    blue: 160
};
var MARS_LINE_COLOR = {
    red: 180,
    green: 70,
    blue: 10
};
var JUPITER_LINE_COLOR = {
    red: 250,
    green: 140,
    blue: 0
};
var SATURN_LINE_COLOR = {
    red: 235,
    green: 215,
    blue: 0
};
var URANUS_LINE_COLOR = {
    red: 135,
    green: 205,
    blue: 240
};
var NEPTUNE_LINE_COLOR = {
    red: 30,
    green: 140,
    blue: 255
};
var PLUTO_LINE_COLOR = {
    red: 255,
    green: 255,
    blue: 255
};

planets.push(new Planet("mercury", MERCURY_LINE_COLOR, 0.387, 0.383));
planets.push(new Planet("venus", VENUS_LINE_COLOR, 0.723, 0.949));
planets.push(new Planet("earth", EARTH_LINE_COLOR, 1.0, 1.0));
planets.push(new Planet("mars", MARS_LINE_COLOR, 1.52, 0.532));
planets.push(new Planet("jupiter", JUPITER_LINE_COLOR, 5.20, 11.21));
planets.push(new Planet("saturn", SATURN_LINE_COLOR, 9.58, 9.45));
planets.push(new Planet("uranus", URANUS_LINE_COLOR, 19.20, 4.01));
planets.push(new Planet("neptune", NEPTUNE_LINE_COLOR, 30.05, 3.88));
planets.push(new Planet("pluto", PLUTO_LINE_COLOR, 39.48, 0.186));

var LABEL_X = 8.0;
var LABEL_Y = 3.0;
var LABEL_Z = 1.0;
var LABEL_DIST = 8.0;
var TEXT_HEIGHT = 1.0;

var PlanetLabel = function(name, index) {
    var text = name + "                    Speed: " + Vec3.length(planets[index].velocity).toFixed(2);
    this.labelPos = Vec3.sum(planets[index].position, {
        x: 0.0,
        y: LABEL_DIST,
        z: LABEL_DIST
    });
    this.linePos = planets[index].position;

    this.line = Entities.addEntity({
        type: "Line",
        position: this.linePos,
        dimensions: {
            x: 20,
            y: 20,
            z: 20
        },
        lineWidth: LINE_WIDTH,
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        linePoints: [{
                x: 0,
                y: 0,
                z: 0
            },
            computeLocalPoint(this.linePos, this.labelPos)
        ],
        visible: false
    });

    this.label = Entities.addEntity({
        type: "Text",
        text: text,
        lineHeight: TEXT_HEIGHT,
        dimensions: {
            x: LABEL_X,
            y: LABEL_Y,
            z: LABEL_Z
        },
        position: this.labelPos,
        backgroundColor: {
            red: 10,
            green: 10,
            blue: 10
        },
        faceCamera: true,
        visible: false
    });

    this.show = function() {
        this.showing = true;
        Entities.editEntity(this.line, {
            visible: true
        });
        Entities.editEntity(this.label, {
            visible: true
        });
    }

    this.hide = function() {
        this.showing = false;
        Entities.editEntity(this.line, {
            visible: false
        });
        Entities.editEntity(this.label, {
            visible: false
        });
    }
}


var time = 0.0;
var elapsed;
var TIME_STEP = 60.0;
var dt = 1.0 / TIME_STEP;

var planetLines = [];
var trails = [];
var paused = false;

function update(deltaTime) {
    if (paused) {
        return;
    }
    //deltaTime = dt;
    time++;
    if (time % TIME_STEP == 0) {
        elapsed++;
    }

    for (var i = 0; i < planets.length; ++i) {
        planets[i].update(deltaTime);
    }
}

function pause() {
    for (var i = 0; i < planets.length; ++i) {
        Entities.editEntity(planets[i].planet, {
            velocity: {
                x: 0.0,
                y: 0.0,
                z: 0.0
            }
        });
        planets[i].label = new PlanetLabel(planets[i].name, i);
        planets[i].label.show();
    }
    paused = true;
}

function resume() {
    for (var i = 0; i < planets.length; ++i) {
        planets[i].label.hide();
    }
    paused = false;
}

function computeLocalPoint(linePos, worldPoint) {
    var localPoint = Vec3.subtract(worldPoint, linePos);
    return localPoint;
}

// Create a new line
function newLine(lineStack, point, period, color) {
    if (elapsed < period) {
        var line = Entities.addEntity({
            position: point,
            type: "Line",
            color: color,
            dimensions: {
                x: LINE_DIM,
                y: LINE_DIM,
                z: LINE_DIM
            },
            lifetime: LIFETIME,
            lineWidth: LINE_WIDTH
        });
        lineStack.push(line);
    } else {
        // Begin overwriting first lines after one full revolution (one period)
        var firstLine = lineStack.shift();
        Entities.editEntity(firstLine, {
            position: point,
            linePoints: [{
                x: 0.0,
                y: 0.0,
                z: 0.0
            }]
        });
        lineStack.push(firstLine);

    }
    var points = [];
    points.push(computeLocalPoint(point, point));
    return points;
}


var paddingX = 8;
var paddingY = 8;
var buttonWidth = 30;
var buttonHeight = 30;

var ICONS_URL = 'https://s3.amazonaws.com/hifi-public/marketplace/hificontent/Scripts/planets/images/';

// var UIPanel = function(x, y, width, height) {
//     this.visible = false;
//     this.buttons = [];
//     this.x = x;
//     this.y = y;
//     this.offsetX = paddingX;
//     this.offsetY = paddingY;

//     this.background = Overlays.addOverlay("text", {
//         backgroundColor: {
//             red: 240,
//             green: 240,
//             blue: 255
//         },
//         textColor: {
//             red: 10,
//             green: 10,
//             blue: 20
//         },
//         x: this.x,
//         y: this.y,
//         width: width,
//         height: height,
//         alpha: 1.0,
//         backgroundAlpha: 0.7,
//         visible: false
//     });

//     this.addIcon = function(iconID) {
//         var icon = Overlays.addOverlay("image", {
//             color: {
//                 red: 10,
//                 green: 10,
//                 blue: 10
//             },
//             imageURL: ICONS_URL + iconID + '.svg',
//             x: this.x + this.offsetX,
//             y: this.y + this.offsetY,
//             width: buttonWidth,
//             height: buttonHeight,
//             alpha: 1.0,
//             visible: false
//         });

//         if (width > height) {
//             this.offsetX += buttonWidth + paddingX;
//         } else {
//             this.offsetY += buttonHeight + paddingY;
//         }


//         this.buttons.push(icon);
//         return icon;
//     }

//     this.addText = function(text) {
//         var icon = Overlays.addOverlay("text", {
//             color: {
//                 red: 240,
//                 green: 240,
//                 blue: 255
//             },
//             text: text,
//             x: this.x + this.offsetX,
//             y: this.y + this.offsetY,
//             width: buttonWidth + paddingX * 4.0,
//             height: buttonHeight,
//             visible: false
//         });

//         if (width > height) {
//             this.offsetX += buttonWidth + paddingX * 5.0;
//         } else {
//             this.offsetY += buttonHeight + paddingY * 5.0;
//         }



//         this.buttons.push(icon);
//         return icon;
//     }


//     this.show = function() {
//         Overlays.editOverlay(this.background, {
//             visible: true
//         });
//         for (var i in this.buttons) {
//             Overlays.editOverlay(this.buttons[i], {
//                 visible: true
//             });
//         }
//         this.visible = true;
//     }

//     this.hide = function() {
//         Overlays.editOverlay(this.background, {
//             visible: false
//         });
//         for (var i in this.buttons) {
//             Overlays.editOverlay(this.buttons[i], {
//                 visible: false
//             });
//         }
//         this.visible = false;
//     }

//     this.remove = function() {
//         Overlays.deleteOverlay(this.background);
//         for (var i in this.buttons) {
//             Overlays.deleteOverlay(this.buttons[i]);
//         }
//     };
// 
// }

var panelX = 1250;
var panelY = 500;
var panelWidth = 50;
var panelHeight = 210;

// var mainPanel = new UIPanel(panelX, panelY, panelWidth, panelHeight);
// var systemViewButton = mainPanel.addIcon('solarsystems');
// var zoomButton = mainPanel.addIcon('magnifier');
// var satelliteButton = mainPanel.addIcon('satellite');
// var settingsButton = mainPanel.addIcon('settings');
// var stopButton = mainPanel.addIcon('close');
// 
// mainPanel.show();
// 
// var systemViewPanel = new UIPanel(panelX - 120, panelY, 120, 40);
// var reverseButton = systemViewPanel.addIcon('reverse');
// var pauseButton = systemViewPanel.addIcon('playpause');
// var forwardButton = systemViewPanel.addIcon('forward');
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
function addIcon(panel, iconId) {
    var icon = panel.add(new UI.Icon({
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

var resizablePanel = new UI.Label({
    text: "Resizable panel",
    width: 200, height: 200,
    backgroundAlpha: 0.5
});
resizablePanel.setPosition(1100, 200);

var debugToggle = new UI.Box({
    text: "debug", width: 150, height: 20
});
debugToggle.setPosition(1000, 0);
debugToggle.addAction('onClick', function () {
    UI.debug.setVisible(!UI.debug.isVisible());
})





debugEvents.forEach(function (action) {
    resizablePanel.addAction(action, function (event, widget) {
        widget.setText(action + " " + widget);
    });
})

addDebugActions(resizablePanel, "", debugEvents);

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
    
var systemViewButton = addIcon(mainPanel, 'solarsystems');
var zoomButton       = addIcon(mainPanel, 'magnifier');
var satelliteButton  = addIcon(mainPanel, 'satellite');
var settingsButton   = addIcon(mainPanel, 'settings');
var stopButton       = addIcon(mainPanel, 'close');


addTooltip(systemViewButton, "system view");
addTooltip(zoomButton, "zoom");
addTooltip(satelliteButton, "satelite view");
addTooltip(settingsButton, "settings");
addTooltip(stopButton, "exit");


var systemViewPanel = addPanel({ dir: '+x', visible: false });
var reverseButton   = addIcon(systemViewPanel, 'reverse');
var pauseButton     = addIcon(systemViewPanel, 'playpause');
var forwardButton   = addIcon(systemViewPanel, 'forward');

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

// checkbox.onValueChanged = function (red) {
//     zoomPanel.getOverlay().update({
//         // backgroundAlpha: 0.1,
//         backgroundColor: red ? redColor : defaultColor
//     });
// }





addIcon(zoomPanel, 'reverse');

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

reverseButton.addAction('onClick', function() {

});
systemViewPanel.addAction('onMouseOver', function() {
    hideSubpanels();
    UI.updateLayout();
    // paused ? resume() : pause();
});


zoomButton.addAction('onClick', function() {
    hideSubpanels();
    UI.updateLayout();

    if (zoomButton.visible) {
        MyAvatar.position = startingPosition;
        Camera.setOrientation(cameraStart);
        // resume();
    } else {
        // pause();
    }
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




// Script.include('../utilities/tools/cookies.js');
// var settings;

// var satelliteView;
// var satelliteGame;

// // satelliteButton.addAction('onclick', function() {
// //     if (satelliteView) {
// //         satelliteGame.endGame();
// //         MyAvatar.position = startingPosition;
// //         satelliteView = false;
// //         resume();
// //     } else {
// //         pause();
// //         var confirmed = Window.confirm("Start satellite game?");
// //         if (!confirmed) {
// //             resume();
// //             return;
// //         }
// //         satelliteView = true;
// //         MyAvatar.position = {
// //             x: 200,
// //             y: 200,
// //             z: 200
// //         };
// //         Camera.setPosition({
// //             x: 200,
// //             y: 200,
// //             z: 200
// //         });
// //         satelliteGame = new SatelliteGame();
// //     } 
// // });

// settingsButton.addAction('onclick', function() {
//     if (!settings) {
//         settings = new Panel(panelX - 610, panelY - 130);
//         settings.visible = false;
//         var g_multiplier = 1.0;
//         settings.newSlider("Gravitational Force ", 0.1, 5.0,
//             function(value) {
//                 g_multiplier = value;
//                 GRAVITY = REFERENCE_GRAVITY * g_multiplier;
//             },

//             function() {
//                 return g_multiplier;
//             },
//             function(value) {
//                 return value.toFixed(1) + "x";
//         });

//         var subPanel = settings.newSubPanel("Orbital Periods");
//         for (var i = 0; i < planets.length; ++i) {
//             var period_multiplier = 1.0;
//             var last_alpha = period_multiplier;

//             subPanel.newSlider(planets[i].name, 0.1, 3.0,
//                 function(value) {
//                     period_multiplier = value;
//                     planets[i].adjustPeriod(value);
//                 },
//                 function() {
//                     return period_multiplier;
//                 },
//                 function(value) {
//                     return (value).toFixed(2) + "x";
//                 });
//         }
//         settings.newCheckbox("Leave Trails: ",
//             function(value) {
//                 TRAILS_ENABLED = value;
//                 if (TRAILS_ENABLED) {
//                     for (var i = 0; i < planets.length; ++i) {
//                         planets[i].resetTrails();
//                     }
//                     //if trails are off and we've already created trails, remove existing trails
//                 } else {
//                     for (var i = 0; i < planets.length; ++i) {
//                         for (var j = 0; j < planets[i].lineStack.length; ++j) {
//                             Entities.editEntity(planets[i].lineStack[j], {
//                                 visible: false
//                             });
//                         }
//                         planets[i].lineStack = [];
//                     }
//                 }
//             },
//             function() {
//                 return TRAILS_ENABLED;
//             },
//             function(value) {
//                 return value;
//         });
//     } else {
//         settings.destroy();
//         settings = null;
//     }
// });








// function mousePressEvent(event) {
//     var clickedOverlay = Overlays.getOverlayAtPoint({
//         x: event.x,
//         y: event.y
//     });

//     if (clickedOverlay == systemViewButton) {
//         if (systemViewPanel.visible) {
//             systemViewPanel.hide();
//         } else {
//             systemViewPanel.show();
//         }
//     }
//     if (clickedOverlay == pauseButton) {
//         if (!paused) {
//             pause();
//         } else {
//             resume();
//         }

//     }
//     if (clickedOverlay == zoomButton) {
//         if (zoomPanel.visible) {
//             zoomPanel.hide();
//             MyAvatar.position = startingPosition;
//             Camera.setOrientation(cameraStart);
//             resume();
//         } else {
//             zoomPanel.show();
//             pause();
//         }
//     }
//     var zoomed = false;
//     if (zoomPanel.visible) {
//         for (var i = 0; i < planets.length; ++i) {
//             if (clickedOverlay == zoomPanel.buttons[i]) {
//                 pause();
//                 planets[i].zoom();
//                 zoomed = true;
//             }
//         }
//     }

    
//     if (clickedOverlay == satelliteButton) {
        
        
//     }

//     if (clickedOverlay == settingsButton) {
        
//     }

//     if(clickedOverlay == stopButton) {
//         Script.stop();
//     }
// }

// UI.printWidgets();

// Clean up models, UI panels, lines, and button overlays
function teardown() {
    UI.teardown();
    // mainPanel.remove();
    // systemViewPanel.remove();
    // zoomPanel.remove();
    // if (settings) {
        // settings.destroy();
    // }
    
    Entities.deleteEntity(theSun);
    for (var i = 0; i < planets.length; ++i) {
        Entities.deleteEntity(planets[i].planet);
    }

    var e = Entities.findEntities(MyAvatar.position, 16000);
    for (i = 0; i < e.length; i++) {
        var props = Entities.getEntityProperties(e[i]);
        if (props.type === "Line" || props.type === "Text") {
            Entities.deleteEntity(e[i]);
        }
    }
};

// Controller.mousePressEvent.connect(mousePressEvent);

// if (settings) {
//     Controller.mouseMoveEvent.connect(function panelMouseMoveEvent(event) { return settings.mouseMoveEvent(event); });
//     Controller.mousePressEvent.connect( function panelMousePressEvent(event) { return settings.mousePressEvent(event); });
//     Controller.mouseDoublePressEvent.connect( function panelMouseDoublePressEvent(event) { return settings.mouseDoublePressEvent(event); });
//     Controller.mouseReleaseEvent.connect(function(event) { return settings.mouseReleaseEvent(event); });
//     Controller.keyPressEvent.connect(function(event) { return settings.keyPressEvent(event); });
// }

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
Script.update.connect(update);