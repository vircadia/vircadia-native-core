(function() {
  var ROLE = "fly";
  var ANIMATION_URL = "C:/Users/Clement/hifi/build/interface/RelWithDebInfo/resources/avatar/animations/sitting_idle.fbx";
  var ANIMATION_FPS = 30;
  var ANIMATION_FIRST_FRAME = 1;
  var ANIMATION_LAST_FRAME = 10;
  var RELEASE_KEYS = ['w', 'a', 's', 'd', 'UP', 'LEFT', 'DOWN', 'RIGHT'];
  var RELEASE_TIME = 500;

  this.entityID = null;
  this.timestamp = null;

  this.preload = function(entityID) {
    this.entityID = entityID;
  }

  this.update = function(dt) {
    if (MyAvatar.getParentID() === this.entityID) {
      var properties = Entities.getEntityProperties(this.entityID, ["position"]);
      if (Vec3.distance(MyAvatar.position, properties.position) > 0.2) {
        this.sitUp(this.entityID);
      }
    }
  }

  this.keyPressed = function(event) {
    print("press");
    if (RELEASE_KEYS.indexOf(event.text) !== -1) {
      this.timestamp = Date.now();
    }
  }
  this.keyReleased = function(event) {
    print("release");
    if (RELEASE_KEYS.indexOf(event.text) !== -1) {
      print('Time: ' + (Date.now() - this.timestamp));
      if (Date.now() - this.timestamp > RELEASE_TIME) {
        this.sitUp();
      }
      this.timestamp = null;
    }
  }

  this.sitDown = function() {
    print("sitDown");
    MyAvatar.overrideRoleAnimation(ROLE, ANIMATION_URL, ANIMATION_FPS, true, ANIMATION_FIRST_FRAME, ANIMATION_LAST_FRAME);
    MyAvatar.setParentID(this.entityID);
    MyAvatar.characterControllerEnabled = false;
    MyAvatar.hmdLeanRecenterEnabled = false;

    var properties = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
    var index = MyAvatar.getJointIndex("Hips");
    print("Data: " + index + " " + JSON.stringify(properties));
    print(MyAvatar.pinJoint(index, properties.position, properties.rotation));

    Script.update.connect(this, this.update);
    Controller.keyPressEvent.connect(this, this.keyPressed);
    Controller.keyReleaseEvent.connect(this, this.keyReleased);
    for (var i in RELEASE_KEYS) {
      Controller.captureKeyEvents({ text: RELEASE_KEYS[i] });
    }
  }

  this.sitUp = function() {
    print("sitUp");
    MyAvatar.restoreRoleAnimation(ROLE);
    MyAvatar.setParentID("");
    MyAvatar.characterControllerEnabled = true;
    MyAvatar.hmdLeanRecenterEnabled = true;

    var index = MyAvatar.getJointIndex("Hips");
    print(MyAvatar.clearPinOnJoint(index));

    Script.update.disconnect(this, this.update);
    Controller.keyPressEvent.disconnect(this, this.keyPressed);
    Controller.keyReleaseEvent.disconnect(this, this.keyReleased);
    for (var i in RELEASE_KEYS) {
      Controller.releaseKeyEvents({ text: RELEASE_KEYS[i] });
    }
  }

  this.clickDownOnEntity = function () {
    if (MyAvatar.getParentID() !== this.entityID) {
      this.sitDown();
    } else {
      this.sitUp();
    }
  }
  this.sit = function () {
      this.sitDown();
  }
});
