//Superclass
Script.include("animatedBrush.js");

function AnimatedRotationBrushClass(settings, entityID) {
    AnimatedBrush.call(this);
    this.angle = 0;
    this.activeAxis = settings.axis;
}

AnimatedRotationBrushClass.prototype.constructor = AnimatedRotationBrushClass;
AnimatedRotationBrushClass.prototype.parent = AnimatedBrush.prototype;

AnimatedRotationBrushClass.prototype.ANIMATED_BRUSH_TIME = 100; //inteval in milliseconds to update the entity rotation;
AnimatedRotationBrushClass.prototype.ANIMATED_BRUSH_INCREMENT = 5; //linear increment of brush size;
AnimatedRotationBrushClass.prototype.NAME = "animatedRotationBrush"; //linear increment of brush size;

AnimatedRotationBrushClass.prototype.onUpdate = function(deltaSeconds, entityID) {
    this.angle = this.angle + ((deltaSeconds * this.ANIMATED_BRUSH_INCREMENT)/this.ANIMATED_BRUSH_TIME);
    this.angle = this.angle >= 360 ? 0 : this.angle; //restart hue cycle
    var rotation = Vec3.multiply(this.angle, this.activeAxis);
    Entities.editEntity(entityID, {rotation : Quat.fromPitchYawRollDegrees(rotation.x, rotation.y, rotation.z)});
    this.parent.updateUserData(entityID, this);
}

AnimatedRotationBrush = AnimatedRotationBrushClass;