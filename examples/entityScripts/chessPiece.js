(function(){
  this.GRID_POSITION = { x: 1, y: 1, z: 1 };
  this.GRID_SIZE = 1.0;
  this.sound = null;
  this.entityID = null;
  this.properties = null;
  this.updateProperties = function(entityID) {
    if (this.entityID === null) {
      this.entityID = entityID;
      print("entityID=" + JSON.stringify(this.entityID));
    }
    if (!entityID.isKnownID || this.entityID.id !== entityID.id) {
      print("Something is very wrong: Bailing!");
      return;
    }
    this.properties = Entities.getEntityProperties(this.entityID);
    if (!this.properties.isKnownID) {
      print("Unknown entityID " + this.entityID.id + " should be self.")
    } 
  }
  this.updatePosition = function(mouseEvent) {
    var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
    var upVector = { x: 0, y: 1, z: 0 };
    var intersection = this.rayPlaneIntersection(pickRay.origin, pickRay.direction,
                                                 this.properties.position, upVector);
    Entities.editEntity(this.entityID, { position: intersection });
  }
  this.snapToGrid = function() {
    var position = this.GRID_POSITION;
    var size = this.GRID_SIZE;
    var tileSize = size / 10.0;

    var relative = Vec3.subtract(this.properties.position, position);
    var i = Math.floor(relative.x / tileSize);
    var j = Math.floor(relative.z / tileSize);
    i = Math.min(Math.max(1, i), 8);
    j = Math.min(Math.max(0, j), 10);
    
    relative.x = (i + 0.5) * tileSize;
    relative.z = (j + 0.5) * tileSize;
    var finalPos = Vec3.sum(position, relative);
    Entities.editEntity(this.entityID, { position: finalPos });
  }
  // Pr, Vr are respectively the Ray's Point of origin and Vector director
  // Pp, Np are respectively the Plane's Point of origin and Normal vector
  this.rayPlaneIntersection = function(Pr, Vr, Pp, Np) {
    var d = -Vec3.dot(Pp, Np);
    var t = -(Vec3.dot(Pr, Np) + d) / Vec3.dot(Vr, Np);
    return Vec3.sum(Pr, Vec3.multiply(t, Vr));
  }
  this.maybeDownloadSound = function() {
    if (this.sound === null) {
      this.sound = new Sound("http://public.highfidelity.io/sounds/Footsteps/FootstepW3Left-12db.wav");
    }
  }
  this.playSound = function() {
		var options = new AudioInjectionOptions();
		options.position = this.properties.position;
		options.volume = 0.5;
		Audio.playSound(this.sound, options);
  }
  
  this.clickDownOnEntity = function(entityID, mouseEvent){
    this.maybeDownloadSound();
    this.updateProperties(entityID);
    this.updatePosition(mouseEvent);
  };
  this.holdingClickOnEntity = function(entityID, mouseEvent){
    this.updateProperties(entityID);
    this.updatePosition(mouseEvent);
  };
  this.clickReleaseOnEntity = function(entityID, mouseEvent){
    this.updateProperties(entityID);
    this.updatePosition(mouseEvent);
    this.snapToGrid();
    this.playSound();
  };
})