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

Script.include("libraries/globals.js");

var panelWall = false;
var orbShell = false;
var reticle = false;

var avatarStickPosition = {};

var orbNaturalExtentsMin = { x: -1230, y: -1223, z: -1210 };
var orbNaturalExtentsMax = { x: 1230, y: 1229, z: 1210 };
var panelsNaturalExtentsMin = { x: -1223, y: -348, z: 45 };
var panelsNaturalExtentsMax = { x: 1223, y: 604, z: 1223 };

var orbNaturalDimensions = Vec3.subtract(orbNaturalExtentsMax, orbNaturalExtentsMin);
var panelsNaturalDimensions = Vec3.subtract(panelsNaturalExtentsMax, panelsNaturalExtentsMin);

var SCALING_FACTOR = 0.01;
var orbDimensions = Vec3.multiply(orbNaturalDimensions, SCALING_FACTOR);
var panelsDimensions = Vec3.multiply(panelsNaturalDimensions, SCALING_FACTOR);

var orbNaturalCenter = Vec3.sum(orbNaturalExtentsMin, Vec3.multiply(orbNaturalDimensions, 0.5));
var panelsNaturalCenter = Vec3.sum(panelsNaturalExtentsMin, Vec3.multiply(panelsNaturalDimensions, 0.5));
var orbCenter = Vec3.multiply(orbNaturalCenter, SCALING_FACTOR);
var panelsCenter = Vec3.multiply(panelsNaturalCenter, SCALING_FACTOR);
var panelsCenterShift = Vec3.subtract(panelsCenter, orbCenter);

var ORB_SHIFT = { x: 0, y: -1.4, z: -0.8};

var HELMET_ATTACHMENT_URL =  HIFI_PUBLIC_BUCKET + "models/attachments/IronManMaskOnly.fbx"

var droneSound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Lobby/drone.raw")
var currentDrone = null;

var latinSound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Lobby/latin.raw")
var elevatorSound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Lobby/elevator.raw")
var currentMusak = null;

function reticlePosition() {
  var RETICLE_DISTANCE = 1;
  return Vec3.sum(Camera.position, Vec3.multiply(Quat.getFront(Camera.orientation), RETICLE_DISTANCE));
}

function drawLobby() {
  if (!panelWall) {
    print("Adding overlays for the lobby panel wall and orb shell.");
    
    var cameraEuler = Quat.safeEulerAngles(Camera.orientation);
    var towardsMe = Quat.angleAxis(cameraEuler.y + 180, { x: 0, y: 1, z: 0});
    
    var orbPosition = Vec3.sum(Camera.position, Vec3.multiplyQbyV(towardsMe, ORB_SHIFT));
    
    var panelWallProps = {
      url: HIFI_PUBLIC_BUCKET + "models/sets/Lobby/LobbyPrototype/Lobby5_PanelsWithFrames.fbx",
      position: Vec3.sum(orbPosition, Vec3.multiplyQbyV(towardsMe, panelsCenterShift)),
      rotation: towardsMe,
      dimensions: panelsDimensions
    };
    
    var orbShellProps = {
      url: HIFI_PUBLIC_BUCKET + "models/sets/Lobby/LobbyPrototype/Lobby5_OrbNoFrames.fbx",
      position: orbPosition,
      rotation: towardsMe,
      dimensions: orbDimensions,
      ignoreRayIntersection: true
    };
    
    avatarStickPosition = MyAvatar.position;

    panelWall = Overlays.addOverlay("model", panelWallProps);
    orbShell = Overlays.addOverlay("model", orbShellProps);
    
    // for HMD wearers, create a reticle in center of screen
    var CURSOR_SCALE = 0.025;
    
    reticle = Overlays.addOverlay("billboard", {
      url: HIFI_PUBLIC_BUCKET + "images/cursor.svg",
      position: reticlePosition(),
      ignoreRayIntersection: true,
      isFacingAvatar: true,
      alpha: 1.0,
      scale: CURSOR_SCALE
    });
    
    // add an attachment on this avatar so other people see them in the lobby
    MyAvatar.attach(HELMET_ATTACHMENT_URL, "Neck", {x: 0, y: 0, z: 0}, Quat.fromPitchYawRollDegrees(0, 0, 0), 1.15);
    
    // start the drone sound
    currentDrone = Audio.playSound(droneSound, { stereo: true, loop: true, localOnly: true });
    
    // start one of our musak sounds
    playRandomMusak();
  }
}

var locations = {};

function changeLobbyTextures() {
  var req = new XMLHttpRequest();
  req.open("GET", "https://data.highfidelity.io/api/v1/locations?limit=21", false);
  req.send();

  locations = JSON.parse(req.responseText).data.locations;
  
  var NUM_PANELS = locations.length;

  var textureProp = { 
    textures: {}
  };
  
  for (var j = 0; j < NUM_PANELS; j++) {
    textureProp["textures"]["file" + (j + 1)] = "http:" + locations[j].thumbnail_url
  };
  
  Overlays.editOverlay(panelWall, textureProp);
}

function playRandomMusak() {
  chosenSound = null;
  
  if (latinSound.downloaded && elevatorSound.downloaded) {
    chosenSound = Math.random < 0.5 ? latinSound : elevatorSound;
  } else if (latinSound.downloaded) {
    chosenSound = latinSound;
  } else if (elevator.downloaded) {
    chosenSound = elevatorSound;
  }
  
  if (chosenSound) {
    currentMusak = Audio.playSound(chosenSound, { stereo: true, localOnly: true })
  } else {
    currentMusak = null;
  }
}

function cleanupLobby() {
  Overlays.deleteOverlay(panelWall);
  Overlays.deleteOverlay(orbShell);
  Overlays.deleteOverlay(reticle);
  
  if (currentDrone) {
    currentDrone.stop();
    currentDrone = null;
  }
  
  if (currentMusak) {
    currentMusak.stop();
    currentMusak = null;
  }
  
  panelWall = false;
  orbShell = false;
  reticle = false;
  
  locations = {};
  toggleEnvironmentRendering(true);
  
  MyAvatar.detachOne(HELMET_ATTACHMENT_URL);
}

function actionStartEvent(event) {
  if (panelWall) {
        
    // we've got an action event and our panel wall is up
    // check if we hit a panel and if we should jump there
    var result = Overlays.findRayIntersection(event.actionRay);

    if (result.intersects && result.overlayID == panelWall) {
      
      var panelName = result.extraInfo;
      var panelStringIndex = panelName.indexOf("Panel");
      if (panelStringIndex != -1) {
        var panelIndex = parseInt(panelName.slice(5)) - 1;
        if (panelIndex < locations.length) {
          var actionLocation = locations[panelIndex];
          
          print("Jumping to " + actionLocation.name + " at " + actionLocation.path 
            + " in " + actionLocation.domain.name + " after click on panel " + panelIndex);
          
          Window.location = actionLocation;
          maybeCleanupLobby();
        }
      }
    }
  }
}

function backStartEvent() {
  if (!panelWall) {
    toggleEnvironmentRendering(false);
    drawLobby();
    changeLobbyTextures();
  } else {
    cleanupLobby();
  }
}

var CLEANUP_EPSILON_DISTANCE = 0.025

function maybeCleanupLobby() {
  if (panelWall && Vec3.length(Vec3.subtract(avatarStickPosition, MyAvatar.position)) > CLEANUP_EPSILON_DISTANCE) {
    cleanupLobby();
  }
}

function toggleEnvironmentRendering(shouldRender) {
  Menu.setIsOptionChecked("Voxels", shouldRender);
  Menu.setIsOptionChecked("Entities", shouldRender);
  Menu.setIsOptionChecked("Metavoxels", shouldRender);
  Menu.setIsOptionChecked("Avatars", shouldRender);
}

function update(deltaTime) {
  maybeCleanupLobby();
  if (reticle) {
    Overlays.editOverlay(reticle, {
      position: reticlePosition()
    });
  }
}

Controller.actionStartEvent.connect(actionStartEvent);
Controller.backStartEvent.connect(backStartEvent);
Script.update.connect(update);
Script.scriptEnding.connect(maybeCleanupLobby);