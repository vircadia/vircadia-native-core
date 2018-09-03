// raypickTester.js
//
// display intersection details (including material) when hovering over entities/avatars/overlays
//

/* eslint-disable comma-dangle, no-empty, no-magic-numbers */

var PICK_FILTERS = Picks.PICK_ENTITIES | Picks.PICK_OVERLAYS | Picks.PICK_AVATARS | Picks.PICK_INCLUDE_NONCOLLIDABLE;
var HAND_JOINT = '_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND'.replace('RIGHT', MyAvatar.getDominantHand().toUpperCase());
var JOINT_NAME = HMD.active ? HAND_JOINT : 'Mouse';
var UPDATE_MS = 1000/30;

// create tect3d overlay to display hover results
var overlayID = Overlays.addOverlay('text3d', {
    text: 'hover',
    visible: false,
    backgroundAlpha: 0,
    isFacingAvatar: true,
    lineHeight: 0.05,
    dimensions: Vec3.HALF,
});
Script.scriptEnding.connect(function() {
    Overlays.deleteOverlay(overlayID);
});

// create raycast picker
var pickID = Picks.createPick(PickType.Ray, {
    joint: JOINT_NAME,
    filter: PICK_FILTERS,
    enabled: true,
});
var blacklist = [ overlayID ]; // exclude hover text from ray pick results
Picks.setIgnoreItems(pickID, blacklist);
Script.scriptEnding.connect(function() {
    Picks.removePick(pickID);
});

// query object materials (using the Graphics.* API)
function getSubmeshMaterial(objectID, shapeID) {
    try {
        var materialLayers = Graphics.getModel(objectID).materialLayers;
        var shapeMaterialLayers = materialLayers[shapeID];
        return shapeMaterialLayers[0].material;
    } catch (e) {
        return { name: '<unknown>' };
    }
}

// refresh hover overlay text based on intersection results
function updateOverlay(overlayID, result) {
    var material = this.getSubmeshMaterial(result.objectID, result.extraInfo.shapeID);
    var position = Vec3.mix(result.searchRay.origin, result.intersection, 0.5);
    var extraInfo = result.extraInfo;
    var text = [
        'mesh: ' + extraInfo.subMeshName,
        'materialName: ' + material.name,
        'type: ' + Entities.getNestableType(result.objectID),
        'distance: ' + result.distance.toFixed(2)+'m',
        ['submesh: ' + extraInfo.subMeshIndex, 'part: '+extraInfo.partIndex, 'shape: '+extraInfo.shapeID].join(' | '),
    ].filter(Boolean).join('\n');

    Overlays.editOverlay(overlayID, {
        text: text,
        position: position,
        visible: result.intersects,
    });
}

// monitor for enw results at 30fps
Script.setInterval(function() {
    var result = Picks.getPrevPickResult(pickID);
    updateOverlay(overlayID, result);
}, UPDATE_MS);
