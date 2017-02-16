(function() {
  var ROLE = "fly";
  var ANIMATION_URL = "C:/Users/Clement/hifi/build/interface/RelWithDebInfo/resources/avatar/animations/sitting_idle.fbx";
  var ANIMATION_FPS = 30;
  var ANIMATION_FIRST_FRAME = 1;
  var ANIMATION_LAST_FRAME = 10;
  var RELEASE_KEYS = ['w', 'a', 's', 'd', 'UP', 'LEFT', 'DOWN', 'RIGHT'];
  var RELEASE_TIME = 500; // ms
  var RELEASE_DISTANCE = 0.2; // meters

  this.entityID = null;
  this.timers = {};
  this.animStateHandlerID = null;

  this.preload = function(entityID) {
    this.entityID = entityID;
  }

  this.update = function(dt) {
    if (MyAvatar.getParentID() === this.entityID) {
      var properties = Entities.getEntityProperties(this.entityID, ["position"]);
      if (Vec3.distance(MyAvatar.position, properties.position) > RELEASE_DISTANCE) {
        this.sitUp(this.entityID);
      }
    }
  }
  this.keyPressed = function(event) {
    if (RELEASE_KEYS.indexOf(event.text) !== -1) {
      var that = this;
      this.timers[event.text] = Script.setTimeout(function() {
        print("Timeout");
        that.sitUp();
      }, RELEASE_TIME);
    }
  }
  this.keyReleased = function(event) {
    if (RELEASE_KEYS.indexOf(event.text) !== -1) {
      if (this.timers[event.text]) {
        print("Clear timeout");
        Script.clearTimeout(this.timers[event.text]);
        delete this.timers[event.text];
      }
    }
  }

  this.sitDown = function() {
    MyAvatar.overrideRoleAnimation(ROLE, ANIMATION_URL, ANIMATION_FPS, true, ANIMATION_FIRST_FRAME, ANIMATION_LAST_FRAME);
    MyAvatar.setParentID(this.entityID);
    MyAvatar.characterControllerEnabled = false;
    MyAvatar.hmdLeanRecenterEnabled = false;

    var properties = Entities.getEntityProperties(this.entityID, ["position", "rotation"]);
    var index = MyAvatar.getJointIndex("Hips");
    MyAvatar.pinJoint(index, properties.position, properties.rotation);

    this.animStateHandlerID = MyAvatar.addAnimationStateHandler(function(props) {
      return { headType: 0 };
    }, ["headType"]);

    Script.update.connect(this, this.update);
    Controller.keyPressEvent.connect(this, this.keyPressed);
    Controller.keyReleaseEvent.connect(this, this.keyReleased);
    for (var i in RELEASE_KEYS) {
      Controller.captureKeyEvents({ text: RELEASE_KEYS[i] });
    }
  }

  this.sitUp = function() {
    MyAvatar.restoreRoleAnimation(ROLE);
    MyAvatar.setParentID("");
    MyAvatar.characterControllerEnabled = true;
    MyAvatar.hmdLeanRecenterEnabled = true;

    var index = MyAvatar.getJointIndex("Hips");
    MyAvatar.clearPinOnJoint(index);

    MyAvatar.removeAnimationStateHandler(this.animStateHandlerID);

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
    }
  }
  this.sit = function () {
      this.sitDown();
  }
});
