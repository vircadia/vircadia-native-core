if (!Function.prototype.bind) {
  Function.prototype.bind = function(oThis) {
    if (typeof this !== 'function') {
      // closest thing possible to the ECMAScript 5
      // internal IsCallable function
      throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
    }

    var aArgs   = Array.prototype.slice.call(arguments, 1),
        fToBind = this,
        fNOP    = function() {},
        fBound  = function() {
          return fToBind.apply(this instanceof fNOP
                 ? this
                 : oThis,
                 aArgs.concat(Array.prototype.slice.call(arguments)));
        };

    if (this.prototype) {
      // Function.prototype doesn't have a prototype property
      fNOP.prototype = this.prototype; 
    }
    fBound.prototype = new fNOP();

    return fBound;
  };
}

(function() {
    Script.include("ownershipToken.js");
    Script.include("tutorial.js");

    var CHANNEL_AWAY_ENABLE = "Hifi-Away-Enable";
    function setAwayEnabled(value) {
        var message = value ? 'enable' : 'disable';
        Messages.sendLocalMessage(CHANNEL_AWAY_ENABLE, message);
    }

    var TutorialZone = function() {
        print("TutorialZone | Creating");
        this.token = null;
    };

    TutorialZone.prototype = {
        keyReleaseHandler: function(event) {
            print(event.text);
            if (event.isShifted && event.isAlt) {
                if (event.text == "F12") {
                    if (!this.tutorialManager.startNextStep()) {
                        this.tutorialManager.startTutorial();
                    }
                } else if (event.text == "F11") {
                    this.tutorialManager.restartStep();
                } else if (event.text == "F10") {
                    MyAvatar.shouldRenderLocally = !MyAvatar.shouldRenderLocally;
                } else if (event.text == "r") {
                    this.tutorialManager.stopTutorial();
                    this.tutorialManager.startTutorial();
                }
            }
        },
        preload: function(entityID) {
            print("TutorialZone | Preload");
            this.entityID = entityID;
        },
        onEnteredStartZone: function() {
            print("TutorialZone | Got onEnteredStartZone");
            var self = this;
            if (!this.token) {
                print("TutorialZone | Creating token");
                // The start zone has been entered, hide the overlays immediately
                setAwayEnabled(false);
                Menu.setIsOptionChecked("Overlays", false);
                MyAvatar.shouldRenderLocally = false;
                Toolbars.getToolbar("com.highfidelity.interface.toolbar.system").writeProperty("visible", false);
                this.token = new OwnershipToken(Math.random() * 100000, this.entityID, {
                    onGainedOwnership: function(token) {
                        print("TutorialZone | GOT OWNERSHIP");
                        if (!self.tutorialManager) {
                            self.tutorialManager = new TutorialManager();
                        }
                        self.tutorialManager.startTutorial();
                        print("TutorialZone | making bound release handler");
                        self.keyReleaseHandlerBound = self.keyReleaseHandler.bind(self);
                        print("TutorialZone | binding");
                        Controller.keyReleaseEvent.connect(self.keyReleaseHandlerBound);
                        print("TutorialZone | done");
                    },
                    onLostOwnership: function(token) {
                        print("TutorialZone | LOST OWNERSHIP");
                        if (self.tutorialManager) {
                            print("TutorialZone | stopping tutorial..");
                            self.tutorialManager.stopTutorial();
                            print("TutorialZone | done");
                            Controller.keyReleaseEvent.disconnect(self.keyReleaseHandlerBound);
                        } else {
                            print("TutorialZone | no tutorial manager...");
                        }
                    }
                });
            }
        },
        onLeftStartZone: function() {
            print("TutorialZone | Got onLeftStartZone");

            // If the start zone was exited, and the tutorial hasn't started, go ahead and
            // re-enable the HUD/Overlays
            if (!this.tutorialManager) {
                Menu.setIsOptionChecked("Overlays", true);
                MyAvatar.shouldRenderLocally = true;
                setAwayEnabled(true);
                Toolbars.getToolbar("com.highfidelity.interface.toolbar.system").writeProperty("visible", true);
            }
        },

        onEnteredEntryPortal: function() {
            print("TutorialZone | Got onEnteredEntryPortal");
            if (this.tutorialManager) {
                print("TutorialZone | Calling enteredEntryPortal");
                this.tutorialManager.enteredEntryPortal();
            }
        },

        enterEntity: function() {
            print("TutorialZone | ENTERED THE TUTORIAL AREA");
        },
        leaveEntity: function() {
            print("TutorialZone | EXITED THE TUTORIAL AREA");
            if (this.token) {
                print("TutorialZone | Destroying token");
                this.token.destroy();
                this.token = null;
            }
            if (this.tutorialManager) {
                this.tutorialManager.stopTutorial();
                //this.tutorialManager = null;
            }
        }
    };

    return new TutorialZone();
});
