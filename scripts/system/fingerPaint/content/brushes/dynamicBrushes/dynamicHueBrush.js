//Superclass
Script.include("dynamicBrush.js");

function DynamicHueBrushClass(settings) {
    //dynamic brush vars
    DynamicBrush.call(this);
    //print("Starting dynamic hue brush");
    this.hsvColor = {hue: 0, saturation: 1.0, value: 1.0};
    this.dynamicColor = {red: 0, green: 0, blue: 0};
}

DynamicHueBrushClass.prototype.DYNAMIC_BRUSH_TIME = 10; //inteval in milliseconds to update the brush width;
DynamicHueBrushClass.prototype.DYNAMIC_BRUSH_INCREMENT = 0.5; //linear increment of brush size;
DynamicHueBrushClass.prototype.NAME = "DynamicHueBrush"; //linear increment of brush size;

DynamicHueBrushClass.prototype.onUpdate = function(deltaSeconds, entityID) {
    //print("Dynamic Hue Brush");
    this.hsvColor.hue = this.hsvColor.hue + ((deltaSeconds * this.DYNAMIC_BRUSH_INCREMENT)/this.DYNAMIC_BRUSH_TIME);
    this.hsvColor.hue = this.hsvColor.hue >= 360 ? 0 : this.hsvColor.hue; //restart hue cycle
    this.dynamicColor = this.convertHsvToRgb(this.hsvColor);
    Entities.editEntity(entityID, { color : this.dynamicColor});
    this.parent.updateUserData(entityID, this);
}


DynamicHueBrushClass.prototype.convertHsvToRgb = function(hsvColor) {
    var c = hsvColor.value * hsvColor.saturation;
    var x = c * (1 - Math.abs((hsvColor.hue/60) % 2 - 1));
    var m = hsvColor.value - c;
    var rgbColor = new Object();
    if (hsvColor.hue >= 0 && hsvColor.hue < 60) {
        rgbColor.red = (c + m) * 255;
        rgbColor.green = (x + m) * 255;
        rgbColor.blue = (0 + m) * 255;
    } else if (hsvColor.hue >= 60 && hsvColor.hue < 120) {
        rgbColor.red = (x + m) * 255;
        rgbColor.green = (c + m) * 255;
        rgbColor.blue = (0 + m) * 255;
    } else if (hsvColor.hue >= 120 && hsvColor.hue < 180) {
        rgbColor.red = (0 + m) * 255;
        rgbColor.green = (c + m) * 255;
        rgbColor.blue = (x + m) * 255;
    } else if (hsvColor.hue >= 180 && hsvColor.hue < 240) {
        rgbColor.red = (0 + m) * 255;
        rgbColor.green = (x + m) * 255;
        rgbColor.blue = (c + m) * 255;
    } else if (hsvColor.hue >= 240 && hsvColor.hue < 300) {
        rgbColor.red = (x + m) * 255;
        rgbColor.green = (0 + m) * 255;
        rgbColor.blue = (c + m) * 255;
    } else if (hsvColor.hue >= 300 && hsvColor.hue < 360) {
        rgbColor.red = (c + m) * 255;
        rgbColor.green = (0 + m) * 255;
        rgbColor.blue = (x + m) * 255;
    } 
    return rgbColor;
}

//DynamicHueBrushClass.prototype = Object.create(DynamicBrush.prototype);
DynamicHueBrushClass.prototype.constructor = DynamicHueBrushClass;
DynamicHueBrushClass.prototype.parent = DynamicBrush.prototype;

DynamicHueBrush = DynamicHueBrushClass;