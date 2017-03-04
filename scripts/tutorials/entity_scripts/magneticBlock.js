(function(){


  const SNAPSOUND = SoundCache.getSound(Script.resolvePath("../../system/assets/sounds/entitySnap.wav?xrs"));
  const RANGE_MULTIPLER = 1.5;

  // Helper for detecting nearby objects
  function findEntitiesInRange(releasedProperties) {
    var dimensions = releasedProperties.dimensions;
    return Entities.findEntities(releasedProperties.position, ((dimensions.x + dimensions.y + dimensions.z) / 3) * RANGE_MULTIPLER);
  }

  function getNearestValidEntityProperties(releasedProperties) {
    var entities = findEntitiesInRange(releasedProperties);
    var nearestEntity = null;
    var nearest = 9999999999999;
    var releaseSize = Vec3.length(releasedProperties.dimensions);
    entities.forEach(function(entityId) {
      if(entityId !== releasedProperties.id) {
        var entity = Entities.getEntityProperties(entityId, ['position', 'rotation', 'dimensions']);
        var distance = Vec3.distance(releasedProperties.position, entity.position);
        var scale = releaseSize/Vec3.length(entity.dimensions);

        if (distance < nearest && (scale >= 0.5 && scale <= 2)) {
          nearestEntity = entity;
          nearest = distance;
        }
      }
    })
    return nearestEntity;
  }
  // Create the 'class'
  function MagneticBlock () {  }
  // Bind pre-emptive events
  MagneticBlock.prototype = {
    // When script is bound to an entity, preload is the first callback called with the entityID. It will behave as the constructor
    preload: function (id) {
      /*
        We will now override any existing userdata with the grabbable property.
        Only retrieving userData
      */
      var val = Entities.getEntityProperties(id, ['userData'])
      var userData = {};
      if (val.userData && val.userData.length > 0 ) {
        try {
          userData = JSON.parse(val.userData);
        } catch (e) {}
      }
      // Object must be triggerable inorder to bind events.
      userData.grabbableKey = {grabbable: true};
      // Apply the new properties to entity of id
      Entities.editEntity(id, {userData: JSON.stringify(userData)});
      this.held = false;

      // We will now create a custom binding, to keep the 'current' context as these are callbacks called without context
      var t = this;
      this.callbacks = {};
      this.callbacks["releaseGrab"] = function () {
          var released = Entities.getEntityProperties(id,["position", "rotation", "dimensions"]);
          var target = getNearestValidEntityProperties(released);
          if (target !== null) {
              // We found nearest, now lets do the snap calculations
              // Plays the snap sound between the two objects.
              Audio.playSound(SNAPSOUND, {
                  volume: 1,
                  position: Vec3.mix(target.position, released.position, 0.5)
              });
              // Check Nearest Axis
              var difference = Vec3.subtract(released.position, target.position);
              var relativeDifference = Vec3.multiplyQbyV(Quat.inverse(target.rotation), difference);

              var abs = {
                  x: Math.abs(relativeDifference.x),
                  y: Math.abs(relativeDifference.y),
                  z: Math.abs(relativeDifference.z)
              };

              if (abs.x >= abs.y && abs.x >= abs.z) {
                  relativeDifference.y = 0;
                  relativeDifference.z = 0;
                  if (relativeDifference.x > 0) {
                      relativeDifference.x = target.dimensions.x / 2 + released.dimensions.x / 2;
                  } else {
                      relativeDifference.x = -target.dimensions.x / 2 - released.dimensions.x / 2;
                  }
              } else if (abs.y >= abs.x && abs.y >= abs.z) {
                  relativeDifference.x = 0;
                  relativeDifference.z = 0;
                  if (relativeDifference.y > 0) {
                      relativeDifference.y = target.dimensions.y / 2 + released.dimensions.y / 2;
                  } else {
                      relativeDifference.y = -target.dimensions.y / 2 - released.dimensions.y / 2;
                  }
              } else if (abs.z >= abs.x && abs.z >= abs.y ) {
                  relativeDifference.x = 0;
                  relativeDifference.y = 0;
                  if (relativeDifference.z > 0) {
                      relativeDifference.z = target.dimensions.z / 2 + released.dimensions.z / 2;
                  } else {
                      relativeDifference.z = -target.dimensions.z / 2 - released.dimensions.z / 2;
                  }
              }
              var newPosition = Vec3.multiplyQbyV(target.rotation, relativeDifference);
              Entities.editEntity(id, {rotation: target.rotation, position: Vec3.sum(target.position, newPosition)})
          }
      }

      this.releaseGrab = this.callbacks["releaseGrab"];
      Script.scriptEnding.connect( function () {
        Script.removeEventHandler(id, "releaseGrab", this.callbacks["releaseGrab"]); //continueNearGrab
      })
    }
  }
  return new MagneticBlock();
})
