/* globals EntityIconOverlayManager:true */

EntityIconOverlayManager = function(entityTypes, getOverlayPropertiesFunc) {
    var visible = false;

    // List of all created overlays
    var allOverlays = [];

    // List of overlays not currently being used
    var unusedOverlays = [];

    // Map from EntityItemID to overlay id
    var entityOverlays = {};

    // Map from EntityItemID to EntityItemID object
    var entityIDs = {};

    this.updatePositions = function(ids) {
        for (var id in entityIDs) {
            var entityID = entityIDs[id];
            var properties = Entities.getEntityProperties(entityID);
            var overlayProperties = {
                position: properties.position
            };
            if (getOverlayPropertiesFunc) {
                var customProperties = getOverlayPropertiesFunc(entityID, properties);
                for (var key in customProperties) {
                    overlayProperties[key] = customProperties[key];
                }
            }
            Overlays.editOverlay(entityOverlays[entityID], overlayProperties);
        }
    };

    this.findRayIntersection = function(pickRay) {
        var result = Overlays.findRayIntersection(pickRay);
        var found = false;

        if (result.intersects) {
            for (var id in entityOverlays) {
                if (result.overlayID === entityOverlays[id]) {
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
        if (visible !== isVisible) {
            visible = isVisible;
            for (var id in entityOverlays) {
                Overlays.editOverlay(entityOverlays[id], {
                    visible: visible
                });
            }
        }
    };

    // Allocate or get an unused overlay
    function getOverlay() {
        var overlay;
        if (unusedOverlays.length === 0) {
            overlay = Overlays.addOverlay("image3d", {});
            allOverlays.push(overlay);
        } else {
            overlay = unusedOverlays.pop();
        }
        return overlay;
    }

    function releaseOverlay(overlay) {
        unusedOverlays.push(overlay);
        Overlays.editOverlay(overlay, {
            visible: false
        });
    }

    function addEntity(entityID) {
        var properties = Entities.getEntityProperties(entityID, ['position', 'type']);
        if (entityTypes.indexOf(properties.type) > -1 && !(entityID in entityOverlays)) {
            var overlay = getOverlay();
            entityOverlays[entityID] = overlay;
            entityIDs[entityID] = entityID;
            var overlayProperties = {
                position: properties.position,
                rotation: Quat.fromPitchYawRollDegrees(0, 0, 270),
                visible: visible,
                alpha: 0.9,
                scale: 0.5,
                drawInFront: true,
                isFacingAvatar: true,
                color: {
                    red: 255,
                    green: 255,
                    blue: 255
                }
            };
            if (getOverlayPropertiesFunc) {
                var customProperties = getOverlayPropertiesFunc(entityID, properties);
                for (var key in customProperties) {
                    overlayProperties[key] = customProperties[key];
                }
            }
            Overlays.editOverlay(overlay, overlayProperties);
        }
    }

    function deleteEntity(entityID) {
        if (entityID in entityOverlays) {
            releaseOverlay(entityOverlays[entityID]);
            delete entityOverlays[entityID];
        }
    }

    function clearEntities() {
        for (var id in entityOverlays) {
            releaseOverlay(entityOverlays[id]);
        }
        entityOverlays = {};
        entityIDs = {};
    }

    Entities.addingEntity.connect(addEntity);
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
