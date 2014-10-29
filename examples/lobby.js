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

var panelWall = false
var orbShell = false

var avatarStickPosition = {}

var orbNaturalExtentsMin = { x: -1230, y: -1223, z: -1210 }
var orbNaturalExtentsMax = { x: 1230, y: 1229, z: 1223 }
var panelsNaturalExtentsMin = { x: -1181, y: -326, z: 56 }
var panelsNaturalExtentsMax = { x: 1181, y: 576, z: 1183 }

var orbNaturalDimensions = Vec3.subtract(orbNaturalExtentsMax, orbNaturalExtentsMin)
var panelsNaturalDimensions = Vec3.subtract(panelsNaturalExtentsMax, panelsNaturalExtentsMin)

var SCALING_FACTOR = 0.01;
var orbDimensions = Vec3.multiply(orbNaturalDimensions, SCALING_FACTOR)
var panelsDimensions = Vec3.multiply(panelsNaturalDimensions, SCALING_FACTOR)

var orbNaturalCenter = Vec3.sum(orbNaturalExtentsMin, Vec3.multiply(orbNaturalDimensions, 0.5))
var panelsNaturalCenter = Vec3.sum(panelsNaturalExtentsMin, Vec3.multiply(panelsNaturalDimensions, 0.5))
var orbCenter = Vec3.multiply(orbNaturalCenter, SCALING_FACTOR)
var panelsCenter = Vec3.multiply(panelsNaturalCenter, SCALING_FACTOR)
var panelsCenterShift = Vec3.subtract(panelsCenter, orbCenter)

function drawLobby() {
  if (!panelWall) {
    print("Adding overlays for the lobby panel wall and orb shell.")
    
    var cameraEuler = Quat.safeEulerAngles(Camera.getOrientation());
    var towardsMe = Quat.angleAxis(cameraEuler.y + 180, { x: 0, y: 1, z: 0})
    
    var orbPosition = Vec3.subtract(Camera.getPosition(), { x: 0, y: 2.0, z: 0}) 
    
    var panelWallProps = {
      url: "http://s3.amazonaws.com/hifi-public/models/sets/Lobby/LobbyPrototype/PanelWall3.fbx",
      position: Vec3.sum(orbPosition, Vec3.multiplyQbyV(towardsMe, panelsCenterShift)),
      rotation: towardsMe,
      dimensions: panelsDimensions
    }
    
    var orbShellProps = {
      url:  "https://s3.amazonaws.com/hifi-public/models/sets/Lobby/LobbyConcepts/Lobby5_OrbShellOnly.fbx",
      position: orbPosition,
      rotation: towardsMe,
      dimensions: orbDimensions,
      ignoreRayIntersection: true
    }
    
    avatarStickPosition = MyAvatar.position

    panelWall = Overlays.addOverlay("model", panelWallProps)
    orbShell = Overlays.addOverlay("model", orbShellProps)
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
  Overlays.deleteOverlay(orbShell)
  panelWall = false
  locations = {}
  toggleEnvironmentRendering(true)
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
          
          print("Jumping to " + actionLocation.name + " at " + actionLocation.path + " in " + actionLocation.domain.name + " after click on panel " + panelIndex)
          
          Window.location = actionLocation
          maybeCleanupLobby()
        }
      }
    }
  }
}

function backStartEvent() {
  if (!panelWall) {
    toggleEnvironmentRendering(false)
    drawLobby()
    changeLobbyTextures()
  } else {
    cleanupLobby()
  }
}

var CLEANUP_EPSILON_DISTANCE = 0.025

function maybeCleanupLobby() {
  if (panelWall && Vec3.length(Vec3.subtract(avatarStickPosition, MyAvatar.position)) > CLEANUP_EPSILON_DISTANCE) {
    cleanupLobby()
  }
}

function toggleEnvironmentRendering(shouldRender) {
  Menu.setIsOptionChecked("Voxels", shouldRender)
  Menu.setIsOptionChecked("Models", shouldRender)
  Menu.setIsOptionChecked("Metavoxels", shouldRender)
}

Controller.actionStartEvent.connect(actionStartEvent)
Controller.backStartEvent.connect(backStartEvent)
Script.update.connect(maybeCleanupLobby)
Script.scriptEnding.connect(maybeCleanupLobby);