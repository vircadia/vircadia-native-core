(function() {
  this.preload = function(entityId) {
    var soundURLs = ["https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_1.wav",
      "https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_2.wav",
      "https://hifi-public.s3.amazonaws.com/sounds/Switches%20and%20sliders/lamp_switch_3.wav"];
    this.entityId = entityId;
    this.properties = Entities.getEntityProperties(this.entityId);
    this.previousPosition = this.properties.position;
    this.previousRotation = this.properties.rotation;
    this.getUserData()
    if (!this.userData) {
      this.userData = {};
      this.userData.lightOn = false;
      this.userData.soundIndex = Math.floor(Math.random() * soundURLs.length);
      this.updateUserData();
    }
    this.sound = SoundCache.getSound(soundURLs[this.userData.soundIndex]);
  }


  this.getUserData = function() {
    if (this.properties.userData) {
      this.userData = JSON.parse(this.properties.userData);
    }
    return false;
  }

  this.updateUserData = function() {
    Entities.editEntity(this.entityId, {
      userData: JSON.stringify(this.userData)
    });
  }

  this.clickReleaseOnEntity = function(entityId, mouseEvent) {
    if (!mouseEvent.isLeftButton) {
      return;
    }
    //first find closest light 
    this.entityId = entityId
    this.playSound();
    this.properties = Entities.getEntityProperties(this.entityId)
    this.light = this.findClosestLight();
    if (this.light) {
      this.lightProperties = Entities.getEntityProperties(this.light);
      this.getUserData();
      Entities.editEntity(this.light, {
        visible: !this.userData.lightOn
      });

      this.userData.lightOn = !this.userData.lightOn;
      this.updateUserData();
      this.tryMoveLight();
    }
  }

  this.playSound = function() {
    if (this.sound && this.sound.downloaded) {
      Audio.playSound(this.sound, {
        position: this.properties.position,
        volume: 0.3
      });
    } else {
      print("Warning: Couldn't play sound.");
    }
  }

  this.tryMoveLight = function() {
    if (this.light) {
      //compute offset position
      var offsetPosition = Quat.multiply(Quat.inverse(this.previousRotation), Vec3.subtract(this.lightProperties.position, this.previousPosition));
      var newPosition = Vec3.sum(this.properties.position, Vec3.multiplyQbyV(this.properties.rotation, offsetPosition));
      if (!Vec3.equal(newPosition, this.lightProperties.position)) {
        Entities.editEntity(this.light, {position: newPosition});
      }
      this.previousPosition = this.properties.position;
      this.previousRotation = this.properties.rotation;
    }

  }

  this.findClosestLight = function() {
    var entities = Entities.findEntities(this.properties.position, 10);
    var lightEntities = [];
    var closestLight = null;
    var nearestDistance = 20

    for (var i = 0; i < entities.length; i++) {
      var props = Entities.getEntityProperties(entities[i]);
      if (props.type === "Light") {
        var distance = Vec3.distance(props.position, this.properties.position)
        if (distance < nearestDistance) {
          closestLight = entities[i];
          nearestDistance = distance
        }
      }
    }
    return closestLight;
  }

});