// given a selected light, instantiate some entities that represent various values you can dynamically adjust
//

var BOX_SCRIPT_URL = Script.resolvePath('box.js');

function entitySlider(color) {
    this.color = color;
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

//what's the ux for adjusting values?  start with simple entities, try image overlays etc
entitySlider.prototype = {
    createAxis: function() {
        var properties = {
            type: 'Line',
            color: this.color,
            collisionsWillMove: false,
            ignoreForCollisions: true,
        };

        this.axis = Entities.addEntity(properties);
    },
    createBoxIndicator: function() {
        var properties = {
            type: 'Box',
            dimensions: {
                x: 0.04,
                y: 0.04,
                z: 0.04
            },
            color: this.color,
            position: position,
            script: BOX_SCRIPT_URL
        };



        this.boxIndicator = Entities.addEntity(properties);
    },
    moveIndicatorAlongAxis: function(direction) {

    }
};

//create them for this light
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
        var r = new entitySlider(RED);
        var g = new entitySlider(GREEN);
        var b = new entitySlider(BLUE);
    }
    if (USE_INTENSITY_SLIDER === true) {
        var intensity = new entitySlider(WHITE);
    }
    if (USE_CUTOFF_SLIDER === true) {
        var cutoff = new entitySlider(PURPLE);
    }
    if (USE_EXPONENT_SLIDER === true) {
        var exponent = new entitySlider(PURPLE);
    }
};