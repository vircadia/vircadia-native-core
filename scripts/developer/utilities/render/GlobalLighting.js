//
//  Global lighting.js
//
//  Sam Gateau, created on 6/7/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var createdOverlays = [];
var overlayFrames = {};

Script.scriptEnding.connect(function () {
    for (var i = 0; i < createdOverlays.length; i++) {
        Overlays.deleteOverlay(createdOverlays[i]);
    }
});


var DIM = {x: 0.1, y: 0.13, z: 0.1};
var avatarHeadJoint = MyAvatar.getJointIndex("Head");

function createOrb(i) {

    var props = {
        dimensions: DIM,      
    }

    props["url"] = "https://github.com/highfidelity/hifi_tests/blob/master/assets/models/material_matrix_models/fbx/blender/hifi_metallicV_albedoV_ao.fbx?raw=true"
    props["position"] = getCamePos(i)
  //  props["localPosition"] = { x: 0.4 * i, y: 0, z: 0}
    props["rotation"] = getCameOri()

    if (createdOverlays.length > 0) {
        props["parentID"] =  createdOverlays[0] 

    }

    var oID = Overlays.addOverlay("model", props);
    
   /* {
        position: getCamePos(),
    //  position: MyAvatar.getJointPosition(avatarHeadJoint),
    //  localPosition: {x: 0, y: 1, z: 0},
    //    localRotation: {x: 0, y: 0, z: 0, w:1},
        url: "https://github.com/highfidelity/hifi_tests/blob/master/assets/models/material_matrix_models/fbx/blender/hifi_metallicV_albedoV_ao.fbx?raw=true",
        dimensions: DIM,
        // parentID: MyAvatar.SELF_ID,
        // parentJointIndex: avatarHeadJoint,
    })*/

    overlayFrames[oID] = {  position: getCamePos(), rotation: getCameOri() }

  //  Overlays.editOverlay(oID, overlayFrames[oID])

    props = Overlays.getProperties(oID, ["position", "rotation"])
    print("createOrb" + oID + JSON.stringify(props))

    return oID;
}


function createSnap(i) {

    var props = {
      //  dimensions: DIM,  
       // url: "resource://spectatorCameraFrame",
        emissive: true,
        url: "https://hifi-public.s3.amazonaws.com/sam/2018-oct/code/PackagedApp/asset/CarrotHunt.png",
        dimensions: DIM,
      //  parentID: MyAvatar.SELF_ID,
      //  parentJointIndex: avatarHeadJoint,
        alpha: 1,
      //  localRotation: { w: 1, x: 0, y: 0, z: 0 },
      //  localPosition: { x: 0, y: 0.0, z: -1.0 },
        dimensions: DIM    
    }

  //  props["url"] = "https://github.com/highfidelity/hifi_tests/blob/master/assets/models/material_matrix_models/fbx/blender/hifi_metallicV_albedoV_ao.fbx?raw=true"
    props["position"] = getCamePos(i)
  //  props["localPosition"] = { x: 0.4 * i, y: 0, z: 0}
    props["rotation"] = getCameOri()

    if (createdOverlays.length > 0) {
        props["parentID"] =  createdOverlays[0] 

    }

    var oID = Overlays.addOverlay("image3d", props);
    
   /* {
        position: getCamePos(),
    //  position: MyAvatar.getJointPosition(avatarHeadJoint),
    //  localPosition: {x: 0, y: 1, z: 0},
    //    localRotation: {x: 0, y: 0, z: 0, w:1},
        url: "https://github.com/highfidelity/hifi_tests/blob/master/assets/models/material_matrix_models/fbx/blender/hifi_metallicV_albedoV_ao.fbx?raw=true",
        dimensions: DIM,
        // parentID: MyAvatar.SELF_ID,
        // parentJointIndex: avatarHeadJoint,
    })*/

    overlayFrames[oID] = {  position: getCamePos(), rotation: getCameOri() }

  //  Overlays.editOverlay(oID, overlayFrames[oID])

    props = Overlays.getProperties(oID, ["position", "rotation"])
    print("createOrb" + oID + JSON.stringify(props))

    return oID;
}

function createOrbs() {
    print("createOrbs")

    createdOverlays.push(createOrb(0));
    createdOverlays.push(createOrb(1));
    createdOverlays.push(createSnap(2));
  
}

var camSpace = {}

function updateCamSpace() {
    camSpace["pos"] = Camera.Position;
    camSpace["ori"] = Camera.orientation;
    camSpace["X"] = Vec3.multiply(Quat.getRight(Camera.orientation), Camera.frustum.aspectRatio);
    camSpace["Y"] = Quat.getUp(Camera.orientation);
    camSpace["Z"] = Vec3.multiply(Quat.getForward(), -1);
}

function getCamePos(i) {

    return Vec3.sum(Camera.position, Vec3.multiplyQbyV(Camera.orientation, { x: -0.5 + 0.2 * i, y: -0.3, z: -1 }))
 //   return Vec3.add(Camera.position, {x: i * 0.3, y:0, z:1}
}
function getCameOri() {
    return Camera.orientation
}
function updateFrames() {
    for (var i = 0; i < createdOverlays.length; i++) {
        overlayFrames[createdOverlays[i]] = {  position: getCamePos(i), rotation: getCameOri() }
    }
    Overlays.editOverlays(overlayFrames)   
   
}


createOrbs()

var accumulated = 0;
Script.update.connect(function(deltaTime) {
    accumulated += deltaTime;
    if (accumulated > 1)
       updateFrames()
});