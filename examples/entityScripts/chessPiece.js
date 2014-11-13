(function(){
  this.GRID_POSITION = { x: 1, y: 1, z: 1 };
  this.GRID_SIZE = 1.0;
  this.metadata = null;
  this.sound = null;
  this.entityID = null;
  this.properties = null;
  this.startingTile = null;
  this.pieces = new Array();
  
  this.updateProperties = function(entityID) {
    if (this.entityID === null) {
      this.entityID = entityID;
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
    if (this.isOutsideGrid(this.getIndexPosition(intersection))) {
      intersection = this.getAbsolutePosition(this.startingTile);
    }
    Entities.editEntity(this.entityID, { position: intersection });
    //this.moveDeadPiece();
  }
  this.isOutsideGrid = function(pos) {
    return !(pos.i >= 0 && pos.j >= 0 && pos.i <= 9 && pos.j <= 9);
  }
  this.getIndexPosition = function(position) {
    var tileSize = this.GRID_SIZE / 10.0;
    var relative = Vec3.subtract(position, this.GRID_POSITION);
    return {
      i: Math.floor(relative.x / tileSize),
      j: Math.floor(relative.z / tileSize)
    }
  }
  this.getAbsolutePosition = function(pos) {
    var tileSize = this.GRID_SIZE / 10.0;
    var relative = Vec3.subtract(this.properties.position, this.GRID_POSITION);
    relative.x = (pos.i + 0.5) * tileSize;
    relative.z = (pos.j + 0.5) * tileSize;
    
    return Vec3.sum(this.GRID_POSITION, relative);
  }
  this.snapToGrid = function() {
    var pos = this.getIndexPosition(this.properties.position);
    var finalPos = this.getAbsolutePosition((this.isOutsideGrid(pos)) ?  this.startingTile : pos);
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
    if (this.sound !== null && this.sound.downloaded) {
  		Audio.playSound(this.sound, { position: this.properties.position });
      print("playing sound");
    }
  }
  this.getMetadata = function() {
    if (this.properties.animationURL !== "") {
      var metadataObject = JSON.parse(this.properties.animationURL);
      if (metadataObject.gamePosition) {
        this.GRID_POSITION = metadataObject.gamePosition;
      }
      if (metadataObject.gameSize) {
        this.GRID_SIZE = metadataObject.gameSize;
      }
      if (metadataObject.pieces) {
        this.pieces = metadataObject.pieces;
      }
    }
  }

  
  this.grab = function(mouseEvent) {
    this.startingTile = this.getIndexPosition(this.properties.position);
    this.updatePosition(mouseEvent);
  }
  this.move = function(mouseEvent) {
    this.updatePosition(mouseEvent);
  }
  this.release = function(mouseEvent) {
    this.updatePosition(mouseEvent);
    this.snapToGrid();
    
    this.playSound();
  }
  this.moveDeadPiece = function() {
    var myPos = this.getIndexPosition(this.properties.position);
    for (var i = 0; i < this.pieces.length; i++) {
      if (this.pieces[i].id != this.entityID.id) {
        var piecePos = this.getIndexPosition(Entities.getEntityProperties(this.pieces[i]).position);
        if (myPos.i === piecePos.i && myPos.j === piecePos.j) {
          Entities.editEntity(this.pieces[i], { position: this.getAbsolutePosition({ i: 0, j: 0 })});
          break;
        }
      }
    }
  }
  
  this.preload = function() {
    this.maybeDownloadSound();
  }
  this.clickDownOnEntity = function(entityID, mouseEvent) {
    this.maybeDownloadSound();
    this.updateProperties(entityID);
    this.getMetadata();
    this.grab(mouseEvent);
  };
  this.holdingClickOnEntity = function(entityID, mouseEvent) {
    this.updateProperties(entityID);
    this.move(mouseEvent);
  };
  this.clickReleaseOnEntity = function(entityID, mouseEvent) {
    this.updateProperties(entityID);
    this.release(mouseEvent);
  };
})