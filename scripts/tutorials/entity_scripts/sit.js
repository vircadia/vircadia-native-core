(function() {
  var ANIMATION_URL = "C:/Users/Clement/hifi/build/interface/RelWithDebInfo/resources/avatar/animations/sitting_idle.fbx";
  var ANIMATION_FPS = 30;
  var ANIMATION_FIRST_FRAME = 1;
  var ANIMATION_LAST_FRAME = 10;
  var RELEASE_KEYS = ['w', 'a', 's', 'd', 'UP', 'LEFT', 'DOWN', 'RIGHT'];

  this.entityID = null;
  this.timestamp = null;

  this.preload = function(entityID) {
    this.entityID = entityID;
  }

  this.keyPressed = function(event) {
    if (RELEASE_KEYS.indexOf(event.text) !== -1) {
      this.timestamp = Date.now();
    }
  }
  this.keyReleased = function(event) {
    if (RELEASE_KEYS.indexOf(event.text) !== -1) {
      print('Time: ' + (Date.now() - this.timestamp).getMilliseconds());
      if ((Date.now() - this.timestamp).getMilliseconds() >= 500) {
        this.sitUp();
      }
        this.timestamp = null;
    }
  }

  this.sitDown = function(entityID) {
    print("sitDown");
    MyAvatar.overrideAnimation(ANIMATION_URL, ANIMATION_FPS, true, ANIMATION_FIRST_FRAME, ANIMATION_LAST_FRAME);
    MyAvatar.setParentID(entityID);
    MyAvatar.characterControllerEnabled = false;

    var properties = Entities.getEntityProperties(entityID, ["position", "rotation"]);
    var index = MyAvatar.getJointIndex("Hips");
    print("Data: " + index + " " + JSON.stringify(properties));
    print(MyAvatar.pinJoint(index, properties.position, properties.rotation));
    // Controller.keyPressEvent.connect(this, this.keyPressed);
    // Controller.keyReleaseEvent.connect(this, this.keyReleased);
  }

  this.sitUp = function() {
    print("sitUp");
    MyAvatar.restoreAnimation();
    MyAvatar.setParentID("");
    MyAvatar.characterControllerEnabled = true;

    var index = MyAvatar.getJointIndex("Hips");
    print(MyAvatar.clearPinOnJoint(index));
    // Controller.keyPressEvent.disconnect(this, this.keyPressed);
    // Controller.keyReleaseEvent.disconnect(this, this.keyReleased);
  }

  this.clickDownOnEntity = function (entityID) {
    Vec3.print("pos:", MyAvatar.position);
    if (MyAvatar.getParentID() !== entityID) {
      this.sitDown(entityID);
    } else {
      this.sitUp();
    }
  }
});
