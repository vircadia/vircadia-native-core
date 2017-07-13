//Superclass
Script.include("dynamicBrush.js");

function DynamicTranslationBrushClass(settings) {
	//dynamic brush vars
	DynamicBrush.call(this);
	print("Starting dynamic Translation brush");
	this.startingPosition = null;
	this.translation = 0;
	this.translationAxis = settings.axis;
}

DynamicTranslationBrushClass.prototype.constructor = DynamicTranslationBrushClass;
DynamicTranslationBrushClass.prototype.parent = DynamicBrush.prototype;

DynamicTranslationBrushClass.prototype.DYNAMIC_BRUSH_TIME = 10; //inteval in milliseconds to update the brush width;
DynamicTranslationBrushClass.prototype.DYNAMIC_BRUSH_INCREMENT = 0.005; //linear increment of brush size;
DynamicTranslationBrushClass.prototype.MAX_TRANSLATION = 2;
DynamicTranslationBrushClass.prototype.NAME = "DynamicTranslationBrush"; //linear increment of brush size;

DynamicTranslationBrushClass.prototype.onUpdate = function(deltaSeconds, entityID) {
	var currentPosition = Entities.getEntityProperties(entityID).position;
	//print("currentPosition " + JSON.stringify(currentPosition));
	if (this.startingPosition == null) {
		//print("setting starting position ");
		this.startingPosition = currentPosition;
	}
	this.translation = this.translation + ((deltaSeconds * this.DYNAMIC_BRUSH_INCREMENT)/this.DYNAMIC_BRUSH_TIME);
	this.translation = Math.abs(this.startingPosition[this.translationAxis]) + this.translation >= 
	Math.abs(this.startingPosition[this.translationAxis]) + this.MAX_TRANSLATION ? 0 : this.translation;
	//print(Math.abs(this.startingPosition.z) + this.translation  + " >= " + Math.abs(this.startingPosition.z) + this.MAX_TRANSLATION);
	var nextPosition = {x: this.startingPosition.x, y: this.startingPosition.y, z: this.startingPosition.z};
	nextPosition[this.translationAxis] = this.translation + nextPosition[this.translationAxis];
    Entities.editEntity(entityID, {position : nextPosition});
    this.parent.updateUserData(entityID, this);
}

DynamicTranslationBrush = DynamicTranslationBrushClass;