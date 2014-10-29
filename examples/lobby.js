//
//  lobby.js
//  examples
//
//  Created by Stephen Birarda on October 17, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var panelWall = false;

var avatarStickPosition = {};

var panelsNaturalExtentsMin = { x: -1181, y: -326, z: 56 };
var panelsNaturalExtentsMax = { x: 1181, y: 576, z: 1183 };

var panelsNaturalDimensions = Vec3.subtract(panelsNaturalExtentsMax, panelsNaturalExtentsMin);
var SCALING_FACTOR = 0.01;
var panelsDimensions = Vec3.multiply(panelsNaturalDimensions, SCALING_FACTOR);

function drawLobby() {
  if (!panelWall) {
    print("Adding an overlay for the lobby panel wall.")
    
    var front = Quat.getFront(Camera.getOrientation());
    front.y = 0
    front = Vec3.normalize(front)
    
    var cameraEuler = Quat.safeEulerAngles(Camera.getOrientation());

    var panelWallProps = {
      url: "https://s3.amazonaws.com/hifi-public/models/sets/Lobby/LobbyPrototype/PanelWall3.fbx",
      position: Vec3.sum(Camera.getPosition(), Vec3.multiply(front, 0.5)),
      rotation: Quat.angleAxis(cameraEuler.y + 180, { x: 0, y: 1, z: 0}),
      dimensions: panelsDimensions
    }
    
    avatarStickPosition = MyAvatar.position

    panelWall = Overlays.addOverlay("model", panelWallProps)
  }
}

var locations = {}

function changeLobbyTextures() {
  var req = new XMLHttpRequest();
  req.open("GET", "https://data.highfidelity.io/api/v1/locations?limit=21", false);
  req.send();

  locations = JSON.parse(req.responseText).data.locations
  
  var NUM_PANELS = locations.length;

  var textureProp = { 
    textures: {}
  };
  
  for (var j = 0; j < NUM_PANELS; j++) {
    textureProp["textures"]["file" + (j + 1)] = "http:" + locations[j].thumbnail_url
  }
  
  Overlays.editOverlay(panelWall, textureProp)
}

function cleanupLobby() {
  Overlays.deleteOverlay(panelWall)
  panelWall = false
  locations = {}
}

function actionStartEvent(event) {
  if (panelWall) {
    // we've got an action event and our panel wall is up
    // check if we hit a panel and if we should jump there
    var pickRay = Camera.computePickRay(event.x, event.y);
    var result = Overlays.findRayIntersection(pickRay);

    if (result.intersects && result.overlayID == panelWall) {
      var panelName = result.extraInfo
      var panelStringIndex = panelName.indexOf("Panel")
      if (panelStringIndex != -1) {
        var panelIndex = parseInt(panelName.slice(5)) - 1
        if (panelIndex < locations.length) {
          var actionLocation = locations[panelIndex]
          
          print("Jumping to " + actionLocation.name + " at " + actionLocation.path + " in " + actionLocation.domain.name)
          
          Window.location = actionLocation
          maybeCleanupLobby()
        }
      }
    }
  }
}

function backStartEvent() {  
  if (!panelWall) {
    drawLobby()
    changeLobbyTextures()
  } else {
    cleanupLobby()
  }
}

var CLEANUP_EPSILON_DISTANCE = 0.025

function maybeCleanupLobby() {
  if (Vec3.length(Vec3.subtract(avatarStickPosition, MyAvatar.position)) > CLEANUP_EPSILON_DISTANCE) {
    cleanupLobby()
  }
}

Controller.actionStartEvent.connect(actionStartEvent)
Controller.backStartEvent.connect(backStartEvent)
Script.update.connect(maybeCleanupLobby)
Script.scriptEnding.connect(maybeCleanupLobby);