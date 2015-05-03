var POINT_LIGHT_URL = "http://s3.amazonaws.com/hifi-public/images/tools/point-light.svg";
var SPOT_LIGHT_URL = "http://s3.amazonaws.com/hifi-public/images/tools/spot-light.svg";

LightOverlayManager = function() {
    var self = this;

    var visible = false;

    // List of all created overlays
    var allOverlays = [];

    // List of overlays not currently being used
    var unusedOverlays = [];

    // Map from EntityItemID.id to overlay id
    var entityOverlays = {};

    // Map from EntityItemID.id to EntityItemID object
    var entityIDs = {};

    this.updatePositions = function(ids) {
        for (var id in entityIDs) {
            var entityID = entityIDs[id];
            var properties = Entities.getEntityProperties(entityID);
            Overlays.editOverlay(entityOverlays[entityID.id], {
                position: properties.position
            });
        }
    };

    this.findRayIntersection = function(pickRay) {
        var result = Overlays.findRayIntersection(pickRay);
        var found = false;

        if (result.intersects) {
            for (var id in entityOverlays) {
                if (result.overlayID == entityOverlays[id]) {
                    result.entityID = entityIDs[id];
                    found = true;
                    break;
                }
            }

            if (!found) {
                result.intersects = false;
            }
        }

        return result;
    };

    this.setVisible = function(isVisible) {
        if (visible != isVisible) {
            visible = isVisible;
            for (var id in entityOverlays) {
                Overlays.editOverlay(entityOverlays[id], { visible: visible });
            }
        }
    };

    // Allocate or get an unused overlay
    function getOverlay() {
        if (unusedOverlays.length == 0) {
            var overlay = Overlays.addOverlay("billboard", {
            });
            allOverlays.push(overlay);
        } else {
            var overlay = unusedOverlays.pop();
        };
        return overlay;
    }

    function releaseOverlay(overlay) {
        unusedOverlays.push(overlay);
        Overlays.editOverlay(overlay, { visible: false });
    }

    function addEntity(entityID) {
        var properties = Entities.getEntityProperties(entityID);
        if (properties.type == "Light" && !(entityID.id in entityOverlays)) {
            var overlay = getOverlay();
            entityOverlays[entityID.id] = overlay; 
            entityIDs[entityID.id] = entityID;
            Overlays.editOverlay(overlay, {
                position: properties.position,
                url: properties.isSpotlight ? SPOT_LIGHT_URL : POINT_LIGHT_URL,
                rotation: Quat.fromPitchYawRollDegrees(0, 0, 270),
                visible: visible,
                alpha: 0.9,
                scale: 0.5,
                color: { red: 255, green: 255, blue: 255 }
            });
        }
    }

    function deleteEntity(entityID) {
        if (entityID.id in entityOverlays) {
            releaseOverlay(entityOverlays[entityID.id]);
            delete entityOverlays[entityID.id];
        }
    }

    function changeEntityID(oldEntityID, newEntityID) {
        entityOverlays[newEntityID.id] = entityOverlays[oldEntityID.id];
        entityIDs[newEntityID.id] = newEntityID;

        delete entityOverlays[oldEntityID.id];
        delete entityIDs[oldEntityID.id];
    }

    function clearEntities() {
        for (var id in entityOverlays) {
            releaseOverlay(entityOverlays[id]);
        }
        entityOverlays = {};
        entityIDs = {};
    }

    Entities.addingEntity.connect(addEntity);
    Entities.changingEntityID.connect(changeEntityID);
    Entities.deletingEntity.connect(deleteEntity);
    Entities.clearingEntities.connect(clearEntities);

    // Add existing entities
    var ids = Entities.findEntities(MyAvatar.position, 64000);
    for (var i = 0; i < ids.length; i++) {
        addEntity(ids[i]);
    }

    Script.scriptEnding.connect(function() {
        for (var i = 0; i < allOverlays.length; i++) {
            Overlays.deleteOverlay(allOverlays[i]);
        }
    });
};
