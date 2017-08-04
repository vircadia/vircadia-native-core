//Superclass
Script.include("animatedBrush.js");

function AnimatedTranslationBrushClass(settings, entityID) {
    AnimatedBrush.call(this);
    this.startingPosition = null;
    this.translation = 0;
    this.activeAxis = settings.axis;
}

AnimatedTranslationBrushClass.prototype.constructor = AnimatedTranslationBrushClass;
AnimatedTranslationBrushClass.prototype.parent = AnimatedBrush.prototype;

AnimatedTranslationBrushClass.prototype.ANIMATED_BRUSH_TIME = 10; //inteval in milliseconds to update the brush width;
AnimatedTranslationBrushClass.prototype.ANIMATED_BRUSH_INCREMENT = 0.005; //linear increment of brush size;
AnimatedTranslationBrushClass.prototype.MAX_TRANSLATION = 2;
AnimatedTranslationBrushClass.prototype.NAME = "animatedTranslationBrush"; //linear increment of brush size;

AnimatedTranslationBrushClass.prototype.onUpdate = function(deltaSeconds, entityID) {
    var currentPosition = Entities.getEntityProperties(entityID).position;
    if (this.startingPosition == null) {
        this.startingPosition = currentPosition;
    }
    this.translation = this.translation + ((deltaSeconds * this.ANIMATED_BRUSH_INCREMENT)/this.ANIMATED_BRUSH_TIME);
    
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

AnimatedTranslationBrush = AnimatedTranslationBrushClass;