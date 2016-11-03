//
//  grabInspector.js
//  examples/debugging/
//
//  Created by Seth Alves on 2015-12-19.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// This script draws an overlay cube around nearby entities to show their queryAABoxes.


Script.include("../libraries/utils.js");

var INSPECT_RADIUS = 10;
var overlays = {};

function updateOverlay(entityID, queryAACube) {
    var cubeCenter = {
        x: queryAACube.x + queryAACube.scale / 2.0,
        y: queryAACube.y + queryAACube.scale / 2.0,
        z: queryAACube.z + queryAACube.scale / 2.0
    };

    if (entityID in overlays) {
        var overlay = overlays[entityID];
        Overlays.editOverlay(overlay, {
            position: cubeCenter,
            size: queryAACube.scale
        });
    } else {
        overlays[entityID] = Overlays.addOverlay("cube", {
            position: cubeCenter,
            size: queryAACube.scale,
            color: {
                red: 0,
                green: 0,
                blue: 255
            },
            alpha: 1,
            // borderSize: ...,
            solid: false
        });
    }
}

Script.setInterval(function() {
    var nearbyEntities = Entities.findEntities(MyAvatar.position, INSPECT_RADIUS);
    for (var entityIndex = 0; entityIndex < nearbyEntities.length; entityIndex++) {
        var entityID = nearbyEntities[entityIndex];
        var queryAACube = Entities.getEntityProperties(entityID, ["queryAACube"]).queryAACube;
        updateOverlay(entityID, queryAACube);
    }
}, 100);

function cleanup() {
    for (var entityID in overlays) {
        Overlays.deleteOverlay(overlays[entityID]);
    }
}

Script.scriptEnding.connect(cleanup);