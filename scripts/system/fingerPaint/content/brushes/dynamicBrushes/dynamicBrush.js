//This file is just used for the superclass
function DynamicBrushClass() {}

/**
 * Called on every frame draw after the user stops painting
 * 
 * @abstract
 * @param deltaSeconds, the number of seconds past since the last frame was rendered
 * @param entityID: the id of the polyline being drawn
 * @param fingerOverlayID: the id of the overlay that shows over the finger when using fingerpaint
 */
DynamicBrushClass.prototype.onUpdate = function(deltaSeconds, entityIDs) {
	    //To be implemented on the child
    throw "Abstract method onUpdate not implemented";
}

/**
 * This function updates the user data so in the next frame the animation gets the previous values.
	 *
 * @param entityID: the id of the polyline being animated
 * @param dynamicBrushObject: the animation object (should be a subclass of dynamicBrush)
 */
DynamicBrushClass.prototype.updateUserData = function(entityID, dynamicBrushObject) {
	//print("Saving class " + dynamicBrushObject.NAME);
	var prevUserData = Entities.getEntityProperties(entityID).userData;
	if (prevUserData) {
	    prevUserData = prevUserData == "" ? new Object() : JSON.parse(prevUserData); //preserve other possible user data
	    if (prevUserData.animations != null && prevUserData.animations[dynamicBrushObject.NAME] != null) {
	    	delete prevUserData.animations[dynamicBrushObject.NAME];
	    	prevUserData.animations[dynamicBrushObject.NAME] = dynamicBrushObject;    	
	    }
		Entities.editEntity(entityID, {userData: JSON.stringify(prevUserData)});
	}
}

DynamicBrush = DynamicBrushClass;
