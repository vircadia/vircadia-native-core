// given a selected light, instantiate some entities that represent various values you can dynamically adjust
//

var BOX_SCRIPT_URL = Script.resolvePath('box.js');

function entitySlider(light, color, sliderType, row) {
    this.light = light;
    this.color = color;
    this.sliderType = sliderType;
    this.verticalOffset = Vec3.multiply(row, PER_ROW_OFFSET);;
    return this;
}

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

//what's the ux for adjusting values?  start with simple entities, try image overlays etc
entitySlider.prototype = {
    createAxis: function() {
        var position =
            var properties = {
                type: 'Line',
                color: this.color,
                collisionsWillMove: false,
                ignoreForCollisions: true,
                position: position,
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
        var g = new entitySlider(GREEN, 'color_green', Vec3.multiply(2, perRowOffset));
        var b = new entitySlider(BLUE, 'color_blue', Vec3.multiply(3, perRowOffset));
    }
    if (USE_INTENSITY_SLIDER === true) {
        var intensity = new entitySlider(WHITE, 'intensity', Vec3.multiply(4, perRowOffset));
    }
    if (USE_CUTOFF_SLIDER === true) {
        var cutoff = new entitySlider(PURPLE, 'cutoff', Vec3.multiply(5, perRowOffset));
    }
    if (USE_EXPONENT_SLIDER === true) {
        var exponent = new entitySlider(PURPLE, 'exponent', Vec3.multiply(6, perRowOffset));
    }
};


makeSliders(light);