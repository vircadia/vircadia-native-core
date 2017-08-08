(function() {    
    Script.include("animatedBrushesList.js");
    var UPDATE_TIME = 33; //run at aproximatelly 30fps
    var MIN_PLAY_DISTANCE = 6; //Minimum distance from player to entity in order to play animation
    var self = this;
    this.preload = function(entityID) {
        //print("After adding script 2 : " + JSON.stringify(Entities.getEntityProperties(entityID)));
        
        self.intervalID = Script.setInterval(function() {
            if (MyAvatar.sessionUUID != Entities.getEntityProperties(entityID).lastEditedBy) {
                Script.clearInterval(self.intervalID);
                return;
            }
            if (Vec3.withinEpsilon(MyAvatar.position, Entities.getEntityProperties(entityID).position, MIN_PLAY_DISTANCE)) {
                var userData = Entities.getEntityProperties(entityID).userData;
                if (userData) {
                    var userDataObject = JSON.parse(userData);
                    var animationObject = userDataObject.animations;
                    var newAnimationObject = null;
                    if (!userDataObject.timeFromLastAnimation) {
                        userDataObject.timeFromLastAnimation = Date.now();
                    }
                    Object.keys(animationObject).forEach(function(animationName) {
                        newAnimationObject = animationObject[animationName];
                        //print("Proto 0001: " + JSON.stringify(newAnimationObject));
                        newAnimationObject.__proto__ = AnimatedBrushesInfo[animationName].proto;
                        //print("time from last draw " + (Date.now() - userDataObject.animations.timeFromLastDraw));
                        newAnimationObject.onUpdate(Date.now() - userDataObject.timeFromLastAnimation, entityID);
                    });
                }
            }        
        }, UPDATE_TIME);
    };
    this.unload = function() {
        Script.clearInterval(self.intervalID);
    }
});
