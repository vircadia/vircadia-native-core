//
//  lightModifier.js
//
//  Created by James Pollack @imgntn on 12/15/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Given a selected light, instantiate some entities that represent various values you can dynamically adjust by grabbing and moving.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//  

//some experimental options
var ONLY_I_CAN_EDIT = false;
var SLIDERS_SHOULD_STAY_WITH_AVATAR = false;
var VERTICAL_SLIDERS = false;
var SHOW_OVERLAYS = true;
var SHOW_LIGHT_VOLUME = true;
var USE_PARENTED_PANEL = true;
var VISIBLE_PANEL = true;
var USE_LABELS = true;
var LEFT_LABELS = false;
var RIGHT_LABELS = true;
var ROTATE_CLOSE_BUTTON = false;

//variables for managing overlays
var selectionDisplay;
var selectionManager;
var lightOverlayManager;

//for when we make a 3d model of a light a parent for the light
var PARENT_SCRIPT_URL = Script.resolvePath('lightParent.js?' + Math.random(0 - 100));

if (SHOW_OVERLAYS === true) {

    Script.include('../libraries/gridTool.js');
    Script.include('../libraries/entitySelectionTool.js?' + Math.random(0 - 100));
    Script.include('../libraries/lightOverlayManager.js');

    var grid = Grid();
    gridTool = GridTool({
        horizontalGrid: grid
    });
    gridTool.setVisible(false);

    selectionDisplay = SelectionDisplay;
    selectionManager = SelectionManager;
    lightOverlayManager = new LightOverlayManager();
    selectionManager.addEventListener(function() {
        selectionDisplay.updateHandles();
        lightOverlayManager.updatePositions();
    });
    lightOverlayManager.setVisible(true);
}

var DEFAULT_PARENT_ID = '{00000000-0000-0000-0000-000000000000}'

var AXIS_SCALE = 1;
var COLOR_MAX = 255;
var INTENSITY_MAX = 0.05;
var CUTOFF_MAX = 360;
var EXPONENT_MAX = 1;

var SLIDER_SCRIPT_URL = Script.resolvePath('slider.js?' + Math.random(0, 100));
var LIGHT_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/james/light_modifier/source4_very_good.fbx';
var CLOSE_BUTTON_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/james/light_modifier/red_x.fbx';
var CLOSE_BUTTON_SCRIPT_URL = Script.resolvePath('closeButton.js?' + Math.random(0, 100));
var TRANSPARENT_PANEL_URL = 'http://hifi-content.s3.amazonaws.com/james/light_modifier/transparent_box_alpha_15.fbx';
var VISIBLE_PANEL_SCRIPT_URL = Script.resolvePath('visiblePanel.js?' + Math.random(0, 100));

var RED = {
    red: 255,
    green: 0,
    blue: 0
};

var GREEN = {
    red: 0,
    green: 255,
    blue: 0
};

var BLUE = {
    red: 0,
    green: 0,
    blue: 255
};

var PURPLE = {
    red: 255,
    green: 0,
    blue: 255
};

var WHITE = {
    red: 255,
    green: 255,
    blue: 255
};

var ORANGE = {
    red: 255,
    green: 165,
    blue: 0
}

var SLIDER_DIMENSIONS = {
    x: 0.075,
    y: 0.075,
    z: 0.075
};

var CLOSE_BUTTON_DIMENSIONS = {
    x: 0.1,
    y: 0.025,
    z: 0.1
}

var LIGHT_MODEL_DIMENSIONS = {
    x: 0.58,
    y: 1.21,
    z: 0.57
}

var PER_ROW_OFFSET = {
    x: 0,
    y: -0.2,
    z: 0
};
var sliders = [];
var slidersRef = {
    'color_red': null,
    'color_green': null,
    'color_blue': null,
    intensity: null,
    cutoff: null,
    exponent: null
};
var light = null;

var basePosition;
var avatarRotation;

function entitySlider(light, color, sliderType, displayText, row) {
    this.light = light;
    this.lightID = light.id.replace(/[{}]/g, "");
    this.initialProperties = light.initialProperties;
    this.color = color;
    this.sliderType = sliderType;
    this.displayText = displayText;
    this.verticalOffset = Vec3.multiply(row, PER_ROW_OFFSET);
    this.avatarRot = Quat.fromPitchYawRollDegrees(0, MyAvatar.bodyYaw, 0.0);
    this.basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(1.5, Quat.getFront(this.avatarRot)));
    this.basePosition.y += 1;
    basePosition = this.basePosition;
    avatarRot = this.avatarRot;

    var message = {
        lightID: this.lightID,
        sliderType: this.sliderType,
        sliderValue: null
    };

    if (this.sliderType === 'color_red') {
        message.sliderValue = this.initialProperties.color.red
        this.setValueFromMessage(message);
    }
    if (this.sliderType === 'color_green') {
        message.sliderValue = this.initialProperties.color.green
        this.setValueFromMessage(message);
    }
    if (this.sliderType === 'color_blue') {
        message.sliderValue = this.initialProperties.color.blue
        this.setValueFromMessage(message);
    }

    if (this.sliderType === 'intensity') {
        message.sliderValue = this.initialProperties.intensity
        this.setValueFromMessage(message);
    }

    if (this.sliderType === 'exponent') {
        message.sliderValue = this.initialProperties.exponent
        this.setValueFromMessage(message);
    }

    if (this.sliderType === 'cutoff') {
        message.sliderValue = this.initialProperties.cutoff
        this.setValueFromMessage(message);
    }

    this.setInitialSliderPositions();
    this.createAxis();
    this.createSliderIndicator();
    if (USE_LABELS === true) {
        this.createLabel()
    }
    return this;
}

//what's the ux for adjusting values?  start with simple entities, try image overlays etc
entitySlider.prototype = {
    createAxis: function() {
        //start of line
        var position;
        var extension;

        if (VERTICAL_SLIDERS == true) {
            position = Vec3.sum(this.basePosition, Vec3.multiply(row, (Vec3.multiply(0.2, Quat.getRight(this.avatarRot)))));
            //line starts on bottom and goes up
            var upVector = Quat.getUp(this.avatarRot);
            extension = Vec3.multiply(AXIS_SCALE, upVector);
        } else {
            position = Vec3.sum(this.basePosition, this.verticalOffset);
            //line starts on left and goes to right
            //set the end of the line to the right
            var rightVector = Quat.getRight(this.avatarRot);
            extension = Vec3.multiply(AXIS_SCALE, rightVector);
        }


        this.axisStart = position;
        this.endOfAxis = Vec3.sum(position, extension);
        this.createEndOfAxisEntity();

        var properties = {
            type: 'Line',
            name: 'Hifi-Slider-Axis::' + this.sliderType,
            color: this.color,
            dynamic: false,
            collisionless: true,
            dimensions: {
                x: 3,
                y: 3,
                z: 3
            },
            position: position,
            linePoints: [{
                x: 0,
                y: 0,
                z: 0
            }, extension],
            lineWidth: 5,
        };

        this.axis = Entities.addEntity(properties);
    },
    createEndOfAxisEntity: function() {
        //we use this to track the end of the axis while parented to a panel
        var properties = {
            name: 'Hifi-End-Of-Axis',
            type: 'Box',
            dynamic: false,
            collisionless: true,
            dimensions: {
                x: 0.01,
                y: 0.01,
                z: 0.01
            },
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            position: this.endOfAxis,
            parentID: this.axis,
            visible: false
        }

        this.endOfAxisEntity = Entities.addEntity(this.endOfAxis);
    },
    createLabel: function() {

        var LABEL_WIDTH = 0.25
        var PER_LETTER_SPACING = 0.1;
        var textWidth = this.displayText.length * PER_LETTER_SPACING;

        var position;
        if (LEFT_LABELS === true) {
            var leftVector = Vec3.multiply(-1, Quat.getRight(this.avatarRot));

            var extension = Vec3.multiply(textWidth, leftVector);

            position = Vec3.sum(this.axisStart, extension);
        }

        if (RIGHT_LABELS === true) {
            var rightVector = Quat.getRight(this.avatarRot);

            var extension = Vec3.multiply(textWidth / 1.75, rightVector);

            position = Vec3.sum(this.endOfAxis, extension);
        }


        var labelProperties = {
            name: 'Hifi-Slider-Label-' + this.sliderType,
            type: 'Text',
            dimensions: {
                x: textWidth,
                y: 0.2,
                z: 0.1
            },
            textColor: {
                red: 255,
                green: 255,
                blue: 255
            },
            text: this.displayText,
            lineHeight: 0.14,
            backgroundColor: {
                red: 0,
                green: 0,
                blue: 0
            },
            position: position,
            rotation: this.avatarRot,
        }
        print('BEFORE CREATE LABEL' + JSON.stringify(labelProperties))
        this.label = Entities.addEntity(labelProperties);
        print('AFTER CREATE LABEL')
    },
    createSliderIndicator: function() {
        var extensionVector;
        var position;
        if (VERTICAL_SLIDERS == true) {
            position = Vec3.sum(this.basePosition, Vec3.multiply(row, (Vec3.multiply(0.2, Quat.getRight(this.avatarRot)))));
            extensionVector = Quat.getUp(this.avatarRot);

        } else {
            position = Vec3.sum(this.basePosition, this.verticalOffset);
            extensionVector = Quat.getRight(this.avatarRot);

        }

        var initialDistance;
        if (this.sliderType === 'color_red') {
            initialDistance = this.distanceRed;
        }
        if (this.sliderType === 'color_green') {
            initialDistance = this.distanceGreen;
        }
        if (this.sliderType === 'color_blue') {
            initialDistance = this.distanceBlue;
        }
        if (this.sliderType === 'intensity') {
            initialDistance = this.distanceIntensity;
        }
        if (this.sliderType === 'cutoff') {
            initialDistance = this.distanceCutoff;
        }
        if (this.sliderType === 'exponent') {
            initialDistance = this.distanceExponent;
        }

        var extension = Vec3.multiply(initialDistance, extensionVector);
        var sliderPosition = Vec3.sum(position, extension);

        var properties = {
            type: 'Sphere',
            name: 'Hifi-Slider-' + this.sliderType,
            dimensions: SLIDER_DIMENSIONS,
            dynamic: true,
            color: this.color,
            position: sliderPosition,
            script: SLIDER_SCRIPT_URL,
            collisionless: true,
            userData: JSON.stringify({
                lightModifierKey: {
                    lightID: this.lightID,
                    sliderType: this.sliderType,
                    axisStart: position,
                    axisEnd: this.endOfAxis,
                },
                handControllerKey: {
                    disableReleaseVelocity: true,
                    disableMoveWithHead: true,
                    disableNearGrab:true
                }
            }),
        };

        this.sliderIndicator = Entities.addEntity(properties);
    },
    setValueFromMessage: function(message) {

        //message is not for our light
        if (message.lightID !== this.lightID) {
            //    print('not our light')
            return;
        }

        //message is not our type
        if (message.sliderType !== this.sliderType) {
            //    print('not our slider type')
            return
        }

        var lightProperties = Entities.getEntityProperties(this.lightID);

        if (this.sliderType === 'color_red') {
            Entities.editEntity(this.lightID, {
                color: {
                    red: message.sliderValue,
                    green: lightProperties.color.green,
                    blue: lightProperties.color.blue
                }
            });
        }

        if (this.sliderType === 'color_green') {
            Entities.editEntity(this.lightID, {
                color: {
                    red: lightProperties.color.red,
                    green: message.sliderValue,
                    blue: lightProperties.color.blue
                }
            });
        }

        if (this.sliderType === 'color_blue') {
            Entities.editEntity(this.lightID, {
                color: {
                    red: lightProperties.color.red,
                    green: lightProperties.color.green,
                    blue: message.sliderValue,
                }
            });
        }

        if (this.sliderType === 'intensity') {
            Entities.editEntity(this.lightID, {
                intensity: message.sliderValue
            });
        }

        if (this.sliderType === 'cutoff') {
            Entities.editEntity(this.lightID, {
                cutoff: message.sliderValue
            });
        }

        if (this.sliderType === 'exponent') {
            Entities.editEntity(this.lightID, {
                exponent: message.sliderValue
            });
        }
    },
    setInitialSliderPositions: function() {
        this.distanceRed = (this.initialProperties.color.red / COLOR_MAX) * AXIS_SCALE;
        this.distanceGreen = (this.initialProperties.color.green / COLOR_MAX) * AXIS_SCALE;
        this.distanceBlue = (this.initialProperties.color.blue / COLOR_MAX) * AXIS_SCALE;
        this.distanceIntensity = (this.initialProperties.intensity / INTENSITY_MAX) * AXIS_SCALE;
        this.distanceCutoff = (this.initialProperties.cutoff / CUTOFF_MAX) * AXIS_SCALE;
        this.distanceExponent = (this.initialProperties.exponent / EXPONENT_MAX) * AXIS_SCALE;
    }

};


var panel;
var visiblePanel;

function makeSliders(light) {

    if (USE_PARENTED_PANEL === true) {
        panel = createPanelEntity(MyAvatar.position);
    }

    if (light.type === 'spotlight') {
        var USE_COLOR_SLIDER = true;
        var USE_INTENSITY_SLIDER = true;
        var USE_CUTOFF_SLIDER = true;
        var USE_EXPONENT_SLIDER = true;
    }
    if (light.type === 'pointlight') {
        var USE_COLOR_SLIDER = true;
        var USE_INTENSITY_SLIDER = true;
        var USE_CUTOFF_SLIDER = false;
        var USE_EXPONENT_SLIDER = false;
    }
    if (USE_COLOR_SLIDER === true) {
        slidersRef.color_red = new entitySlider(light, RED, 'color_red', 'Red', 1);
        slidersRef.color_green = new entitySlider(light, GREEN, 'color_green', 'Green', 2);
        slidersRef.color_blue = new entitySlider(light, BLUE, 'color_blue', 'Blue', 3);

        sliders.push(slidersRef.color_red);
        sliders.push(slidersRef.color_green);
        sliders.push(slidersRef.color_blue);

    }
    if (USE_INTENSITY_SLIDER === true) {
        slidersRef.intensity = new entitySlider(light, WHITE, 'intensity', 'Intensity', 4);
        sliders.push(slidersRef.intensity);
    }
    if (USE_CUTOFF_SLIDER === true) {
        slidersRef.cutoff = new entitySlider(light, PURPLE, 'cutoff', 'Cutoff', 5);
        sliders.push(slidersRef.cutoff);
    }
    if (USE_EXPONENT_SLIDER === true) {
        slidersRef.exponent = new entitySlider(light, ORANGE, 'exponent', 'Exponent', 6);
        sliders.push(slidersRef.exponent);
    }

    createCloseButton(slidersRef.color_red.axisStart);

    subscribeToSliderMessages();

    if (USE_PARENTED_PANEL === true) {
        parentEntitiesToPanel(panel);
    }

    if (SLIDERS_SHOULD_STAY_WITH_AVATAR === true) {
        parentPanelToAvatar(panel);
    }

    if (VISIBLE_PANEL === true) {
        visiblePanel = createVisiblePanel();
    }
};

function parentPanelToAvatar(panel) {
    //this is going to need some more work re: the sliders actually being grabbable.  probably something to do with updating axis movement
    Entities.editEntity(panel, {
        parentID: MyAvatar.sessionUUID,
        //actually figure out which one to parent it to -- probably a spine or something.
        parentJointIndex: 1,
    })
}


function parentEntitiesToPanel(panel) {

    sliders.forEach(function(slider) {
        Entities.editEntity(slider.axis, {
            parentID: panel
        })
        Entities.editEntity(slider.sliderIndicator, {
            parentID: panel
        })
    })

    closeButtons.forEach(function(button) {
        Entities.editEntity(button, {
            parentID: panel
        })
    })
}

function createPanelEntity(position) {
    print('CREATING PANEL at ' + JSON.stringify(position));
    var panelProperties = {
        name: 'Hifi-Slider-Panel',
        type: 'Box',
        dimensions: {
            x: 0.1,
            y: 0.1,
            z: 0.1
        },
        visible: false,
        dynamic: false,
        collisionless: true
    }

    var panel = Entities.addEntity(panelProperties);
    return panel
}

function createVisiblePanel() {
    var totalOffset = -PER_ROW_OFFSET.y * sliders.length;

    var moveRight = Vec3.sum(basePosition, Vec3.multiply(AXIS_SCALE / 2, Quat.getRight(avatarRot)));

    var moveDown = Vec3.sum(moveRight, Vec3.multiply((sliders.length + 1) / 2, PER_ROW_OFFSET))
    var panelProperties = {
        name: 'Hifi-Visible-Transparent-Panel',
        type: 'Model',
        modelURL: TRANSPARENT_PANEL_URL,
        dimensions: {
            x: AXIS_SCALE + 0.1,
            y: totalOffset,
            z: SLIDER_DIMENSIONS.z / 4
        },
        visible: true,
        dynamic: false,
        collisionless: true,
        position: moveDown,
        rotation: avatarRot,
        script: VISIBLE_PANEL_SCRIPT_URL
    }

    var panel = Entities.addEntity(panelProperties);

    return panel
}


function createLightModel(position, rotation) {
    var blockProperties = {
        name: 'Hifi-Spotlight-Model',
        type: 'Model',
        shapeType: 'box',
        modelURL: LIGHT_MODEL_URL,
        dimensions: LIGHT_MODEL_DIMENSIONS,
        dynamic: true,
        position: position,
        rotation: rotation,
        script: PARENT_SCRIPT_URL,
        userData: JSON.stringify({
            handControllerKey: {
                disableReleaseVelocity: true
            }
        })
    };

    var block = Entities.addEntity(blockProperties);

    return block
}

var closeButtons = [];

function createCloseButton(axisStart) {
    var MARGIN = 0.10;
    var VERTICAL_OFFFSET = {
        x: 0,
        y: 0.15,
        z: 0
    };
    var leftVector = Vec3.multiply(-1, Quat.getRight(avatarRot));
    var extension = Vec3.multiply(MARGIN, leftVector);
    var position = Vec3.sum(axisStart, extension);

    var buttonProperties = {
        name: 'Hifi-Close-Button',
        type: 'Model',
        modelURL: CLOSE_BUTTON_MODEL_URL,
        dimensions: CLOSE_BUTTON_DIMENSIONS,
        position: Vec3.sum(position, VERTICAL_OFFFSET),
        rotation: Quat.multiply(avatarRot, Quat.fromPitchYawRollDegrees(90, 0, 45)),
        //rotation: Quat.fromPitchYawRollDegrees(0, 0, 90),
        dynamic: false,
        collisionless: true,
        script: CLOSE_BUTTON_SCRIPT_URL,
        userData: JSON.stringify({
            grabbableKey: {
                wantsTrigger: true
            }
        })
    }

    var button = Entities.addEntity(buttonProperties);

    closeButtons.push(button);

    if (ROTATE_CLOSE_BUTTON === true) {
        Script.update.connect(rotateCloseButtons);
    }
}

function rotateCloseButtons() {
    closeButtons.forEach(function(button) {
        Entities.editEntity(button, {
            angularVelocity: {
                x: 0,
                y: 0.5,
                z: 0
            }
        })

    })
}

function subScribeToNewLights() {
    Messages.subscribe('Hifi-Light-Mod-Receiver');
    Messages.messageReceived.connect(handleLightModMessages);
}

function subscribeToSliderMessages() {
    Messages.subscribe('Hifi-Slider-Value-Reciever');
    Messages.messageReceived.connect(handleValueMessages);
}

function subscribeToLightOverlayRayCheckMessages() {
    Messages.subscribe('Hifi-Light-Overlay-Ray-Check');
    Messages.messageReceived.connect(handleLightOverlayRayCheckMessages);
}

function subscribeToCleanupMessages() {
    Messages.subscribe('Hifi-Light-Modifier-Cleanup');
    Messages.messageReceived.connect(handleCleanupMessages);
}


function handleLightModMessages(channel, message, sender) {
    if (channel !== 'Hifi-Light-Mod-Receiver') {
        return;
    }
    if (sender !== MyAvatar.sessionUUID) {
        return;
    }
    var parsedMessage = JSON.parse(message);

    makeSliders(parsedMessage.light);
    light = parsedMessage.light.id
    if (SHOW_LIGHT_VOLUME === true) {
        selectionManager.setSelections([parsedMessage.light.id]);
    }
}

function handleValueMessages(channel, message, sender) {

    if (channel !== 'Hifi-Slider-Value-Reciever') {
        return;
    }
    if (ONLY_I_CAN_EDIT === true && sender !== MyAvatar.sessionUUID) {
        return;
    }
    var parsedMessage = JSON.parse(message);

    slidersRef[parsedMessage.sliderType].setValueFromMessage(parsedMessage);
}

var currentLight;
var block;
var oldParent = null;
var hasParent = false;

function handleLightOverlayRayCheckMessages(channel, message, sender) {
    if (channel !== 'Hifi-Light-Overlay-Ray-Check') {
        return;
    }
    if (ONLY_I_CAN_EDIT === true && sender !== MyAvatar.sessionUUID) {
        return;
    }

    var pickRay = JSON.parse(message);

    var doesIntersect = lightOverlayManager.findRayIntersection(pickRay);
    //  print('DOES INTERSECT A LIGHT WE HAVE???' + doesIntersect.intersects);
    if (doesIntersect.intersects === true) {
        // print('FULL MESSAGE:::' + JSON.stringify(doesIntersect))

        var lightID = doesIntersect.entityID;
        if (currentLight === lightID) {
            //  print('ALREADY HAVE A BLOCK, EXIT')
            return;
        }

        currentLight = lightID;
        var lightProperties = Entities.getEntityProperties(lightID);
        if (lightProperties.parentID !== DEFAULT_PARENT_ID) {
            //this light has a parent already.  so lets call our block the parent and then make sure not to delete it at the end;
            oldParent = lightProperties.parentID;
            hasParent = true;
            block = lightProperties.parentID;
            if (lightProperties.parentJointIndex !== -1) {
                //should make sure to retain the parent too.  but i don't actually know what the
            }
        } else {
            block = createLightModel(lightProperties.position, lightProperties.rotation);
        }

        var light = {
            id: lightID,
            type: 'spotlight',
            initialProperties: lightProperties
        }

        makeSliders(light);

        if (SHOW_LIGHT_VOLUME === true) {
            selectionManager.setSelections([lightID]);
        }

        Entities.editEntity(lightID, {
            parentID: block,
            parentJointIndex: -1
        });

    }
}

function handleCleanupMessages(channel, message, sender) {

    if (channel !== 'Hifi-Light-Modifier-Cleanup') {
        return;
    }
    if (ONLY_I_CAN_EDIT === true && sender !== MyAvatar.sessionUUID) {
        return;
    }
    if (message === 'callCleanup') {
        cleanup(true);
    }
}

function updateSliderAxis() {
    sliders.forEach(function(slider) {

    })
}

function cleanup(fromMessage) {
    var i;
    for (i = 0; i < sliders.length; i++) {
        Entities.deleteEntity(sliders[i].axis);
        Entities.deleteEntity(sliders[i].sliderIndicator);
        Entities.deleteEntity(sliders[i].label);
    }

    while (closeButtons.length > 0) {
        Entities.deleteEntity(closeButtons.pop());
    }

    //if the light was already parented to something we will want to restore that.  or come up with groups or something clever.
    if (oldParent !== null) {
        Entities.editEntity(currentLight, {
            parentID: oldParent,
        });
    } else {
        Entities.editEntity(currentLight, {
            parentID: null,
        });
    }


    if (fromMessage !== true) {
        Messages.messageReceived.disconnect(handleLightModMessages);
        Messages.messageReceived.disconnect(handleValueMessages);
        Messages.messageReceived.disconnect(handleLightOverlayRayCheckMessages);
        lightOverlayManager.setVisible(false);
    }


    Entities.deleteEntity(panel);
    Entities.deleteEntity(visiblePanel);

    selectionManager.clearSelections();

    if (ROTATE_CLOSE_BUTTON === true) {
        Script.update.disconnect(rotateCloseButtons);
    }

    if (hasParent === false) {
        Entities.deleteEntity(block);
    }

    oldParent = null;
    hasParent = false;
    currentLight = null;
    sliders = [];

}

Script.scriptEnding.connect(cleanup);

Script.scriptEnding.connect(function() {
    lightOverlayManager.setVisible(false);
})


subscribeToLightOverlayRayCheckMessages();
subScribeToNewLights();
subscribeToCleanupMessages();



//other light properties
// diffuseColor: { red: 255, green: 255, blue: 255 },
// ambientColor: { red: 255, green: 255, blue: 255 },
// specularColor: { red: 255, green: 255, blue: 255 },
// constantAttenuation: 1,
// linearAttenuation: 0,
// quadraticAttenuation: 0,
// exponent: 0,
// cutoff: 180, // in degrees
