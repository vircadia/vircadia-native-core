(function() {
    var TutorialStartZone = function() {
    };

    TutorialStartZone.prototype = {
        preload: function(entityID) {
            this.entityID = entityID;
        },
        enterEntity: function() {
            // send message to outer zone
            print("Entered the tutorial start area");
            if (HMD.isHMDAvailable() && HMD.isHandControllerAvailable()) {
                var parentID = Entities.getEntityProperties(this.entityID, 'parentID').parentID;
                if (parentID) {
                    Entities.callEntityMethod(parentID, 'start');
                } else {
                    print("ERROR: No parent id found on tutorial start zone");
                }
            } else {
                Window.alert("To proceed with this tutorial, please connect your VR headset and hand controllers.");
                location = "/";
            }
        },
        leaveEntity: function() {
            print("Exited the tutorial start area");
        }
    };

    return new TutorialStartZone();
});
