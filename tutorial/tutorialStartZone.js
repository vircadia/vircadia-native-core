(function() {
    var TutorialStartZone = function() {
        print("TutorialStartZone | Creating");
    };

    TutorialStartZone.prototype = {
        preload: function(entityID) {
            print("TutorialStartZone | Preload");
            this.entityID = entityID;
            this.sendStartIntervalID = null;
        },
        enterEntity: function() {
            var self = this;
            // send message to outer zone
            print("TutorialStartZone | Entered the tutorial start area");
            if (HMD.isHMDAvailable() && HMD.isHandControllerAvailable()) {
                function sendStart() {
                    print("TutorialStartZone | Checking parent ID");
                    var parentID = Entities.getEntityProperties(self.entityID, 'parentID').parentID;
                    print("TutorialStartZone | Parent ID is: ", parentID);
                    if (parentID) {
                        print("TutorialStartZone | Sending start");
                        Entities.callEntityMethod(parentID, 'start');
                    } else {
                        print("TutorialStartZone | ERROR: No parent id found on tutorial start zone");
                    }
                }
                this.sendStartIntervalID = Script.setInterval(sendStart, 1500);
                sendStart();
            } else {
                print("TutorialStartZone | User tried to go to tutorial with HMD and hand controllers, sending back to /");
                Window.alert("To proceed with this tutorial, please connect your VR headset and hand controllers.");
                location = "/";
            }
        },
        leaveEntity: function() {
            print("TutorialStartZone | Exited the tutorial start area");
            if (this.sendStartIntervalID) {
                Script.clearInterval(this.sendStartIntervalID);
            }
        }
    };

    return new TutorialStartZone();
});
