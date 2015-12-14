// given a selected light, instantiate some entities that represent various values you can dynamically adjust
//

var BOX_SCRIPT_URL = Script.resolvePath('box.js');

var RED = {
    r: 255,
    g: 0,
    b: 0
};

var GREEN = {
    r: 0,
    g: 255,
    b: 0
};

var BLUE = {
    r: 0,
    g: 0,
    b: 255
};

var PURPLE = {
    r: 255,
    g: 0,
    b: 255
};

var WHITE = {
    r: 255
    g: 255,
    b: 255
};

var AXIS_SCALE = 1;

var BOX_DIMENSIONS = {
    x: 0.05,
    y: 0.05,
    z: 0.05
};
var PER_ROW_OFFSET = {
    x: 0,
    y: 0.2,
    z: 0
};

function entitySlider(light, color, sliderType, row) {
    this.light = light;
    this.lightID = light.id;
    this.initialProperties = light.initialProperties;
    this.color = color;
    this.sliderType = sliderType;
    this.verticalOffset = Vec3.multiply(row, PER_ROW_OFFSET);

    var formattedMessage = {
        'color_red': this.initialProperties.color.r,
        'color_green': this.initialProperties.color.g,
        'color_blue': this.initialProperties.color.b,
        'intensity': this.initialProperties.intensity,
        'exponent': this.initialProperties.exponent,
        'cutoff': this.initialProperties.cutoff,
    }

    this.setValueFromMessage(formattedMessage);
    this.setInitialSliderPositions();

    return this;
}

//what's the ux for adjusting values?  start with simple entities, try image overlays etc
entitySlider.prototype = {
    createAxis: function() {
        //start of line
        var position;
        //1 meter along orientationAxis
        var endOfAxis;
        var properties = {
            type: 'Line',
            color: this.color,
            collisionsWillMove: false,
            ignoreForCollisions: true,
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
            }, endOfAxis],
            lineWidth: 5,
        };

        this.axis = Entities.addEntity(properties);
    },
    createBoxIndicator: function() {
        var properties = {
            type: 'Box',
            dimensions: BOX_DIMENSIONS,
            color: this.color,
            position: position,
            script: BOX_SCRIPT_URL
        };

        this.boxIndicator = Entities.addEntity(properties);
    },
    handleValueMessages: function(channel, message, sender) {
        if (channel !== 'Hifi-Slider-Value-Reciever') {
            return;
        }
        //easily protect from other people editing your values, but group editing might be fun so lets try that first.
        // if (sender !== MyAvatar.sessionUUID) {
        //     return;
        // }
        var parsedMessage = JSON.parse(message);
        setValueFromMessage(parsedMessage);
    },
    setValueFromMessage: function(message) {
        var lightProperties = Entities.getEntityProperties(this.lightID);

        if (this.sliderType === 'color_red') {
            Entities.editEntity(this.lightID, {
                color: {
                    red: message.sliderValue,
                    green: lightProperties.color.g,
                    blue: lightProperties.color.b
                }
            });
        }

        if (this.sliderType === 'color_green') {
            Entities.editEntity(this.lightID, {
                color: {
                    red: lightProperties.color.r
                    green: message.sliderValue,
                    blue: lightProperties.color.b
                }
            });
        }

        if (this.sliderType === 'color_blue') {
            Entities.editEntity(this.lightID, {
                color: {
                    red: lightProperties.color.r,
                    green: lightProperties.color.g,
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
    subscribeToBoxMessages: function() {
        Messages.subscribe('Hifi-Slider-Value-Reciever');
        Messages.messageReceived.connect(this.handleValueMessages);
    },
    setInitialSliderPositions:function(){

        var distanceRed = (this.initialProperties.color.r / 255) * AXIS_SCALE;
         var distanceGreen = (this.initialProperties.color.g / 255) * AXIS_SCALE;
          var distanceBlue = (this.initialProperties.color.b / 255) * AXIS_SCALE;
          var distanceIntensity =  (this.initialProperties.intensity / 255) * AXIS_SCALE;
          var distanceCutoff =  (this.initialProperties.cutoff / 360) * AXIS_SCALE;
          var distanceExponent =  (this.initialProperties.exponent / 255) * AXIS_SCALE;
    },
    cleanup: function() {
        Entities.deleteEntity(this.boxIndicator);
        Entities.deleteEntity(this.axis);
        Messages.messageReceived.disconnect(this.handleValueMessages);
    }
};

//create them for a given light
function makeSliders(light) {
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
        var r = new entitySlider(RED, 'color_red', 1);
        var g = new entitySlider(GREEN, 'color_green', 2);
        var b = new entitySlider(BLUE, 'color_blue', 3);
    }
    if (USE_INTENSITY_SLIDER === true) {
        var intensity = new entitySlider(WHITE, 'intensity', 4);
    }
    if (USE_CUTOFF_SLIDER === true) {
        var cutoff = new entitySlider(PURPLE, 'cutoff', 5);
    }
    if (USE_EXPONENT_SLIDER === true) {
        var exponent = new entitySlider(PURPLE, 'exponent', 6);
    }
};

function subScribeToNewLights() {
    Messages.subscribe('Hifi-Light-Mod-Receiver');
    Messages.messageReceived.connect(handleLightModMessages);
}

function handleLightModMessages(channel, message, sender) {
    if (channel !== 'Hifi-Light-Mod-Receiver') {
        return;
    }
    if (sender !== MyAvatar.sessionUUID) {
        return;
    }
    var parsedMessage = JSON.parse(message);
    var light = message.light;
    makeSliders(light);
}

subScribeToNewLights();

    // diffuseColor: { red: 255, green: 255, blue: 255 },
    // ambientColor: { red: 255, green: 255, blue: 255 },
    // specularColor: { red: 255, green: 255, blue: 255 },

    // constantAttenuation: 1,
    // linearAttenuation: 0,
    // quadraticAttenuation: 0,
    // exponent: 0,
    // cutoff: 180, // in degrees