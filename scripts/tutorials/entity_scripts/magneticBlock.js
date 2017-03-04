(function(){

  // Helper for detecting nearby objects
  function findEntitiesInRange(releasedProperties) {
    var dimensions = releasedProperties.dimensions;
    return Entities.findEntities(releasedProperties.position, ((dimensions.x + dimensions.y + dimensions.z) / 3) *1.5);
  }
  function getNearestValidEntityProperties(id) {
    var releasedProperties = Entities.getEntityProperties(id,["position", "rotation", "dimensions"]);
    var entities = findEntitiesInRange(releasedProperties);
    var nearestEntity = null;
    var nearest = -1;
    var releaseSize = Vec3.length(releasedProperties.dimensions);
    entities.forEach(function(entityId) {
      print('ftest ' + entityId);
      var entity = Entities.getEntityProperties(entityId, ['position', 'rotation', 'dimensions']);
      var distance = Vec3.distance(releasedProperties.position, entity.position);
      var scale = releaseSize/Vec3.length(entity.dimensions);
      if ((nearest === -1 || distance < nearest && scale >= 0.5 && scale <= 2 ) && entity.id !== entityId) {
        nearestEntity = entity;
        dnearest = distance;
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
        var nearest = getNearestValidEntityProperties(id);

        print(JSON.stringify(nearest));
      }

      this.releaseGrab = this.callbacks["releaseGrab"];

      Script.scriptEnding.connect( function () {
        Script.removeEventHandler(id, "releaseGrab", this.callbacks["releaseGrab"]); //continueNearGrab
      })
    }
  }
  return new MagneticBlock();
})
