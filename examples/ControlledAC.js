//
//  ControlledAC.js
//  examples
//
//  Created by Clément Brisset on 8/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set the following variables to the values needed
var filename = "http://s3-us-west-1.amazonaws.com/highfidelity-public/ozan/bartender.rec";
var playFromCurrentLocation = true;
var useDisplayName = true;
var useAttachments = true;
var useHeadModel = true;
var useSkeletonModel = true;

// ID of the agent. Two agents can't have the same ID.
var id = 0;

// Set head and skeleton models
Avatar.faceModelURL = "http://public.highfidelity.io/models/heads/EvilPhilip_v7.fst";
Avatar.skeletonModelURL = "http://public.highfidelity.io/models/skeletons/Philip_Carl_Body_A-Pose.fst";
// Set position/orientation/scale here if playFromCurrentLocation is true
Avatar.position = { x:1, y: 1, z: 1 };
Avatar.orientation = Quat.fromPitchYawRollDegrees(0, 0, 0);
Avatar.scale = 1.0;

// Those variables MUST be common to every scripts
var controlVoxelSize = 0.25;
var controlVoxelPosition = { x: 2000 , y: 0, z: 0 };

// Script. DO NOT MODIFY BEYOND THIS LINE.
var DO_NOTHING = 0;
var PLAY = 1;
var PLAY_LOOP = 2;
var STOP = 3;
var SHOW = 4;
var HIDE = 5;

var COLORS = [];
COLORS[PLAY] = { red: PLAY, green: 0,  blue: 0 };
COLORS[PLAY_LOOP] = { red: PLAY_LOOP, green: 0,  blue: 0 };
COLORS[STOP] = { red: STOP, green: 0,  blue: 0 };
COLORS[SHOW] = { red: SHOW, green: 0,  blue: 0 };
COLORS[HIDE] = { red: HIDE, green: 0,  blue: 0 };

controlVoxelPosition.x += id * controlVoxelSize;
    
Avatar.loadRecording(filename);

Avatar.setPlayFromCurrentLocation(playFromCurrentLocation);
Avatar.setPlayerUseDisplayName(useDisplayName);
Avatar.setPlayerUseAttachments(useAttachments);
Avatar.setPlayerUseHeadModel(useHeadModel);
Avatar.setPlayerUseSkeletonModel(useSkeletonModel);

function setupVoxelViewer() {
  var voxelViewerOffset = 10;
  var voxelViewerPosition = JSON.parse(JSON.stringify(controlVoxelPosition));
  voxelViewerPosition.x -= voxelViewerOffset;
  var voxelViewerOrientation = Quat.fromPitchYawRollDegrees(0, -90, 0);

  VoxelViewer.setPosition(voxelViewerPosition);
  VoxelViewer.setOrientation(voxelViewerOrientation);
  VoxelViewer.queryOctree();
}

function getAction(controlVoxel) {
  if (controlVoxel.x != controlVoxelPosition.x ||
      controlVoxel.y != controlVoxelPosition.y ||
      controlVoxel.z != controlVoxelPosition.z ||
      controlVoxel.s != controlVoxelSize) {
    return DO_NOTHING;
  }

  for (i in COLORS) {
    if (controlVoxel.red === COLORS[i].red &&
        controlVoxel.green === COLORS[i].green &&
        controlVoxel.blue === COLORS[i].blue) {
          Voxels.eraseVoxel(controlVoxelPosition.x,
                          controlVoxelPosition.y,
                          controlVoxelPosition.z,
                          controlVoxelSize);
          return parseInt(i);
    }
  }

  return DO_NOTHING;
}

count = 300; // This is necessary to wait for the audio mixer to connect
function update(event) {
  VoxelViewer.queryOctree();
  if (count > 0) {
    count--;
    return;
  }

  var controlVoxel = Voxels.getVoxelAt(controlVoxelPosition.x,
                                       controlVoxelPosition.y,
                                       controlVoxelPosition.z,
                                       controlVoxelSize);
  var action = getAction(controlVoxel);

  switch(action) {
    case PLAY:
      print("Play");
      if (!Agent.isAvatar) {
        Agent.isAvatar = true;
      }
      if (!Avatar.isPlaying()) {
        Avatar.startPlaying();
      }
      Avatar.setPlayerLoop(false);
      break;
    case PLAY_LOOP:
      print("Play loop");
      if (!Agent.isAvatar) {
        Agent.isAvatar = true;
      }
      if (!Avatar.isPlaying()) {
        Avatar.startPlaying();
      }
      Avatar.setPlayerLoop(true);
      break;
    case STOP:
      print("Stop");
      if (Avatar.isPlaying()) {
        Avatar.stopPlaying();
      }
      break;
    case SHOW:
      print("Show");
      if (!Agent.isAvatar) {
        Agent.isAvatar = true;
      }
      break;
    case HIDE:
      print("Hide");
      if (Avatar.isPlaying()) {
        Avatar.stopPlaying();
      }
      Agent.isAvatar = false;
      break;
    case DO_NOTHING:
      break;
    default:
      print("Unknown action: " + action);
      break;
  }
  
  if (Avatar.isPlaying()) {
    Avatar.play();
  }
}

Script.update.connect(update);
setupVoxelViewer();