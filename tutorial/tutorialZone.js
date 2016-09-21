// A user designates ownership of the tutorial by creating a child entity (token) 
// of the tutorial zone. The entity should have a short lifetime (5 seconds), and 
// should have it's lifetime reset every second.
//
//  * When you enter the "tutorial" begin zone
//    * If the tutorial is owned
//      * Show a "waiting" text, and check for ownership periodically
//    * If the tutorial is not owned
//      * Create the ownership token, begin tutorial
//        * For extra safety, to avoid races, check after 1 second to confirm that 
//          another user hasn't created a token. If they have, use some method to 
//          resolve the conflict.
//      * Once the user has finished the tutorial, stop creating the token to 
//        release ownership.
//
//  * The tutorial will expose a local message API for controlling the tutorial
//  * A special script will be used to:
//    * Create a key shortcut to go to the beginning of the tutorial
//    * 
//

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
    Script.include("file:///C:/Users/Ryan/dev/hifi/tutorial/ownershipToken.js");
    Script.include("file:///C:/Users/Ryan/dev/hifi/tutorial/tutorial.js");

    var TutorialZone = function() {
        this.token = null;
    };

    TutorialZone.prototype = {
        keyReleaseHandler: function(event) {
            print(event.text);
            if (event.text == ",") {
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
        },
        preload: function(entityID) {
            this.entityID = entityID;
        },
        start: function() {
            print("Got start");
            var self = this;
            if (!this.token) {
                this.token = new OwnershipToken(Math.random() * 100000, this.entityID, {
                    onGainedOwnership: function(token) {
                        print("GOT OWNERSHIP");
                        if (!self.tutorialManager) {
                            self.tutorialManager = new TutorialManager();
                        }
                        self.tutorialManager.startTutorial();
                        print("making bound release handler");
                        self.keyReleaseHandlerBound = self.keyReleaseHandler.bind(self);
                        print("binding");
                        Controller.keyReleaseEvent.connect(self.keyReleaseHandlerBound);
                        print("done");
                    },
                    onLostOwnership: function(token) {
                        print("LOST OWNERSHIP");
                        if (self.tutorialManager) {
                        print("stopping tutorial..");
                            self.tutorialManager.stopTutorial();
                            print("done");
                            Controller.keyReleaseEvent.disconnect(self.keyReleaseHandlerBound);
                        } else {
                            print("no tutorial manager...");
                        }
                    }
                });
            }
        },

        enterEntity: function() {
            print("ENTERED THE TUTORIAL AREA");
        },
        leaveEntity: function() {
            print("EXITED THE TUTORIAL AREA");
            if (this.token) {
                print("destroying token");
                this.token.destroy();
                this.token = null;
            }
        }
    };

    return new TutorialZone();
});
