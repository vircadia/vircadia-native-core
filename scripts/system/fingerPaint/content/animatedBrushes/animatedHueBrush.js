//Superclass
Script.include("animatedBrush.js");
Script.include("../js/ColorUtils.js");

function AnimatedHueBrushClass(settings, entityID) {
    AnimatedBrush.call(this);
    this.hsvColor = rgb2hsv(Entities.getEntityProperties(entityID).color);//   {hue: 0, saturation: 1.0, value: 1.0};
    this.animatedColor = {red: 0, green: 0, blue: 0};
}

AnimatedHueBrushClass.prototype.ANIMATED_BRUSH_TIME = 10; //inteval in milliseconds to update the brush width;
AnimatedHueBrushClass.prototype.ANIMATED_BRUSH_INCREMENT = 0.5; //linear increment of brush size;
AnimatedHueBrushClass.prototype.NAME = "animatedHueBrush"; 

AnimatedHueBrushClass.prototype.onUpdate = function(deltaSeconds, entityID) {
    this.hsvColor.hue = this.hsvColor.hue + ((deltaSeconds * this.ANIMATED_BRUSH_INCREMENT)/this.ANIMATED_BRUSH_TIME);
    this.hsvColor.hue = this.hsvColor.hue >= 360 ? 0 : this.hsvColor.hue; //restart hue cycle
    this.animatedColor = hsv2rgb(this.hsvColor);
    Entities.editEntity(entityID, { color : this.animatedColor});
    this.parent.updateUserData(entityID, this);
}

AnimatedHueBrushClass.prototype.constructor = AnimatedHueBrushClass;
AnimatedHueBrushClass.prototype.parent = AnimatedBrush.prototype;

AnimatedHueBrush = AnimatedHueBrushClass;