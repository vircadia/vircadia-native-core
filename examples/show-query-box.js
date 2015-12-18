Script.include("libraries/utils.js");

var INSPECT_RADIUS = 10;
var overlays = {};

function updateOverlay(entityID, queryAACube) {
    var cubeCenter = {x: queryAACube.x + queryAACube.scale / 2.0,
                      y: queryAACube.y + queryAACube.scale / 2.0,
                      z: queryAACube.z + queryAACube.scale / 2.0};

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
            color: { red: 0, green: 0, blue: 255},
            alpha: 1,
            // borderSize: ...,
            solid: false
        });
    }
}

Script.setInterval(function() {
    // {f4b3936e-c452-4b31-ab40-dd9a550cb756}
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
