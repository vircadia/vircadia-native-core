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

var orbNaturalExtentsMin = { x: -1.230354, y: -1.22077, z: -1.210487 };
var orbNaturalExtentsMax = { x: 1.230353, y: 1.229819, z: 1.210487 };
var panelsNaturalExtentsMin = { x: -1.223182, y: -0.348487, z: 0.0451369 };
var panelsNaturalExtentsMax = { x: 1.223039, y: 0.602978, z: 1.224298 };

var orbDimensions = Vec3.subtract(orbNaturalExtentsMax, orbNaturalExtentsMin);
var panelsDimensions = Vec3.subtract(panelsNaturalExtentsMax, panelsNaturalExtentsMin);

var orbCenter = Vec3.sum(orbNaturalExtentsMin, Vec3.multiply(orbDimensions, 0.5));
var panelsCenter = Vec3.sum(panelsNaturalExtentsMin, Vec3.multiply(panelsDimensions, 0.5));
var panelsCenterShift = Vec3.subtract(panelsCenter, orbCenter);

var ORB_SHIFT = { x: 0, y: -0.2, z: 0.05};

var HELMET_ATTACHMENT_URL = HIFI_PUBLIC_BUCKET + "models/attachments/IronManMaskOnly.fbx"

var droneSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Lobby/drone.stereo.raw")
var currentDrone = null;

var latinSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Lobby/latin.stereo.raw")
var elevatorSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Lobby/elevator.stereo.raw")
var currentMusakInjector = null;
var currentSound = null;

var inOculusMode =  Menu.isOptionChecked("Enable VR Mode");

function reticlePosition() {
  var RETICLE_DISTANCE = 1;
  return Vec3.sum(Camera.position, Vec3.multiply(Quat.getFront(Camera.orientation), RETICLE_DISTANCE));
}

var MAX_NUM_PANELS = 21;
var DRONE_VOLUME = 0.3;

function drawLobby() {
  if (!panelWall) {
    print("Adding overlays for the lobby panel wall and orb shell.");
    
    var cameraEuler = Quat.safeEulerAngles(Camera.orientation);
    var towardsMe = Quat.angleAxis(cameraEuler.y + 180, { x: 0, y: 1, z: 0});
    
    var orbPosition = Vec3.sum(Camera.position, Vec3.multiplyQbyV(towardsMe, ORB_SHIFT));
    
    var panelWallProps = {
      url: HIFI_PUBLIC_BUCKET + "models/sets/Lobby/Lobby_v8/forStephen1/PanelWall2.fbx",
      position: Vec3.sum(orbPosition, Vec3.multiplyQbyV(towardsMe, panelsCenterShift)),
      rotation: towardsMe
    };
    
    var orbShellProps = {
      url: HIFI_PUBLIC_BUCKET + "models/sets/Lobby/Lobby_v8/forStephen1/LobbyShell1.fbx",
      position: orbPosition,
      rotation: towardsMe,
      ignoreRayIntersection: true
    };
    
    avatarStickPosition = MyAvatar.position;

    panelWall = Overlays.addOverlay("model", panelWallProps);    
    orbShell = Overlays.addOverlay("model", orbShellProps);
    
    // for HMD wearers, create a reticle in center of screen
    if (inOculusMode) {
      var CURSOR_SCALE = 0.025;
    
      reticle = Overlays.addOverlay("billboard", {
        url: HIFI_PUBLIC_BUCKET + "images/cursor.svg",
        position: reticlePosition(),
        ignoreRayIntersection: true,
        isFacingAvatar: true,
        alpha: 1.0,
        scale: CURSOR_SCALE
      });
    }
      
    // add an attachment on this avatar so other people see them in the lobby
    MyAvatar.attach(HELMET_ATTACHMENT_URL, "Neck", {x: 0, y: 0, z: 0}, Quat.fromPitchYawRollDegrees(0, 0, 0), 1.15);
    
    // start the drone sound
    currentDrone = Audio.playSound(droneSound, { stereo: true, loop: true, localOnly: true, volume: DRONE_VOLUME });
    
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

var MUSAK_VOLUME = 0.1;

function playNextMusak() {
  if (panelWall) {
    if (currentSound == latinSound) {
      if (elevatorSound.downloaded) {
        currentSound = elevatorSound;
      }
    } else if (currentSound == elevatorSound) {
      if (latinSound.downloaded) {
        currentSound = latinSound;
      }
    }
  
    currentMusakInjector = Audio.playSound(currentSound, { localOnly: true, volume: MUSAK_VOLUME });
  }
}

function playRandomMusak() {
  currentSound = null;
  
  if (latinSound.downloaded && elevatorSound.downloaded) {
    currentSound = Math.random() < 0.5 ? latinSound : elevatorSound;
  } else if (latinSound.downloaded) {
    currentSound = latinSound;
  } else if (elevatorSound.downloaded) {
    currentSound = elevatorSound;
  }
  
  if (currentSound) {    
    // pick a random number of seconds from 0-10 to offset the musak
    var secondOffset = Math.random() * 10;
    currentMusakInjector = Audio.playSound(currentSound, { localOnly: true, secondOffset: secondOffset, volume: MUSAK_VOLUME });
  } else {
    currentMusakInjector = null;
  }
}

function cleanupLobby() {
  // for each of the 21 placeholder textures, set them back to default so the cached model doesn't have changed textures
  var panelTexturesReset = {};
  panelTexturesReset["textures"] = {};
      
  for (var j = 0; j < MAX_NUM_PANELS; j++) { 
    panelTexturesReset["textures"]["file" + (j + 1)] = HIFI_PUBLIC_BUCKET + "models/sets/Lobby/LobbyPrototype/Texture.jpg";
  };
  
  Overlays.editOverlay(panelWall, panelTexturesReset);
  
  Overlays.deleteOverlay(panelWall);
  Overlays.deleteOverlay(orbShell);
  
  if (reticle) {
    Overlays.deleteOverlay(reticle);
  }
  
  panelWall = false;
  orbShell = false;
  reticle = false;
  
  Audio.stopInjector(currentDrone);
  currentDrone = null;
  
  Audio.stopInjector(currentMusakInjector);
  currentMusakInjector = null;
  
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

var CLEANUP_EPSILON_DISTANCE = 0.05;

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
  if (panelWall) {
    
    if (reticle) {
      Overlays.editOverlay(reticle, {
        position: reticlePosition()
      });
    }
    
    // if the reticle is up then we may need to play the next musak
    if (!Audio.isInjectorPlaying(currentMusakInjector)) {
      playNextMusak();
    }
  }
}

Controller.actionStartEvent.connect(actionStartEvent);
Controller.backStartEvent.connect(backStartEvent);
Script.update.connect(update);
Script.scriptEnding.connect(maybeCleanupLobby);