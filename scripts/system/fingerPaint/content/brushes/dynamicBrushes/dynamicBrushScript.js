(function() {
	Script.include("dynamicBrushesList.js");
	var UPDATE_TIME = 33; //run at aproximatelly 30fps
    var self = this;
    this.preload = function(entityID) {
	    print("After adding script 2 : " + JSON.stringify(Entities.getEntityProperties(entityID)));
	    self.intervalID = Script.setInterval(function() {
	    	if (Vec3.withinEpsilon(MyAvatar.position, Entities.getEntityProperties(entityID).position, 3)) {
	    		var userData = Entities.getEntityProperties(entityID).userData;
		        //print("UserData:  " + userData);

		        if (userData) {
		        	var userDataObject = JSON.parse(userData);
					var animationObject = userDataObject.animations;
					//print("Playing animation " + JSON.stringify(animationObject));
					var newAnimationObject = null;
					Object.keys(animationObject).forEach(function(animationName) {
					    newAnimationObject = animationObject[animationName];
					    //print("Proto 0001: " + JSON.stringify(newAnimationObject));
					    newAnimationObject.__proto__ = DynamicBrushesInfo[animationName].proto;
					    newAnimationObject.onUpdate(UPDATE_TIME, entityID);
					});
		        }
	    	}		
	    }, UPDATE_TIME);
    };
	this.unload = function() {
	    Script.clearInterval(self.intervalID);
	}
});
/*
(function() {
	Script.include("dynamicBrushesList.js");
	var UPDATE_TIME = 10;
    var self = this;
    this.preload = function(entityID) {
	    Script.setTimeout(playAnimations, UPDATE_TIME);
	    function playAnimations() {
	        var userData = Entities.getEntityProperties(entityID).userData;
	        //print("UserData:  " + userData);

	        if (userData) {
	        	var userDataObject = JSON.parse(userData);
				var animationObject = userDataObject.animations;
				//print("Playing animation " + JSON.stringify(animationObject));
				var newAnimationObject = null;
				Object.keys(animationObject).forEach(function(animationName) {
				    newAnimationObject = animationObject[animationName];
				    //print("Proto 0001: " + JSON.stringify(newAnimationObject));
				    newAnimationObject.__proto__ = DynamicBrushesInfo[animationName].proto;
				    if (userDataObject.isDrawing) {
				    	newAnimationObject.onDraw(UPDATE_TIME, entityID, null);
				    } else {
				    	newAnimationObject.onUpdate(UPDATE_TIME, entityID, null);
				    }
				});
	        }
			Script.setTimeout(playAnimations, UPDATE_TIME);
	    }
    };
});*/



