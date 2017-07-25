//Superclass
Script.include("dynamicBrush.js");

function DynamicRotationBrushClass(settings) {
    //dynamic brush vars
    DynamicBrush.call(this);
    //print("Starting dynamic rotation brush");
    this.angle = 0;
    this.activeAxis = settings.axis;
}

DynamicRotationBrushClass.prototype.constructor = DynamicRotationBrushClass;
DynamicRotationBrushClass.prototype.parent = DynamicBrush.prototype;

DynamicRotationBrushClass.prototype.DYNAMIC_BRUSH_TIME = 100; //inteval in milliseconds to update the entity rotation;
DynamicRotationBrushClass.prototype.DYNAMIC_BRUSH_INCREMENT = 5; //linear increment of brush size;
DynamicRotationBrushClass.prototype.NAME = "DynamicRotationBrush"; //linear increment of brush size;

DynamicRotationBrushClass.prototype.onUpdate = function(deltaSeconds, entityID) {
    //print("Dynamic rotation this:  " + JSON.stringify(rotation));
    this.angle = this.angle + ((deltaSeconds * this.DYNAMIC_BRUSH_INCREMENT)/this.DYNAMIC_BRUSH_TIME);
    this.angle = this.angle >= 360 ? 0 : this.angle; //restart hue cycle
    var rotation = Vec3.multiply(this.angle, this.activeAxis);
    Entities.editEntity(entityID, {rotation : Quat.fromPitchYawRollDegrees(rotation.x, rotation.y, rotation.z)});
    this.parent.updateUserData(entityID, this);
}

DynamicRotationBrush = DynamicRotationBrushClass;