//Superclass
Script.include("dynamicBrush.js");

function DynamicTranslationBrushClass(settings) {
    //dynamic brush vars
    DynamicBrush.call(this);
    //print("Starting dynamic Translation brush");
    this.startingPosition = null;
    this.translation = 0;
    this.activeAxis = settings.axis;
}

DynamicTranslationBrushClass.prototype.constructor = DynamicTranslationBrushClass;
DynamicTranslationBrushClass.prototype.parent = DynamicBrush.prototype;

DynamicTranslationBrushClass.prototype.DYNAMIC_BRUSH_TIME = 10; //inteval in milliseconds to update the brush width;
DynamicTranslationBrushClass.prototype.DYNAMIC_BRUSH_INCREMENT = 0.005; //linear increment of brush size;
DynamicTranslationBrushClass.prototype.MAX_TRANSLATION = 2;
DynamicTranslationBrushClass.prototype.NAME = "DynamicTranslationBrush"; //linear increment of brush size;

DynamicTranslationBrushClass.prototype.onUpdate = function(deltaSeconds, entityID) {
    //print("translation this: " + JSON.stringify(this) + " : " + JSON.stringify(this.activeAxis));
    var currentPosition = Entities.getEntityProperties(entityID).position;
    //print("currentPosition " + JSON.stringify(currentPosition));
    if (this.startingPosition == null) {
        this.startingPosition = currentPosition;
    }
    this.translation = this.translation + ((deltaSeconds * this.DYNAMIC_BRUSH_INCREMENT)/this.DYNAMIC_BRUSH_TIME);
    
    var translationVec = Vec3.multiply(this.translation, this.activeAxis);
    var nextPosition = {
        x: this.startingPosition.x + translationVec.x, 
        y: this.startingPosition.y + translationVec.y, 
        z: this.startingPosition.z + translationVec.z
    };

    if (Vec3.distance(nextPosition, this.startingPosition) > this.MAX_TRANSLATION) {
        this.translation = 0;
        nextPosition = this.startingPosition;
    }
    Entities.editEntity(entityID, {position : nextPosition});
    this.parent.updateUserData(entityID, this);
}

DynamicTranslationBrush = DynamicTranslationBrushClass;