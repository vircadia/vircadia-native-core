ZoneOverlayManager = function(isEntityFunc, entityAddedFunc, entityRemovedFunc, entityMovedFunc) {
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
            Overlays.editOverlay(entityOverlays[entityID.id].solid, {
                position: properties.position,
                rotation: properties.rotation,
                dimensions: properties.dimensions,
            });
            Overlays.editOverlay(entityOverlays[entityID.id].outline, {
                position: properties.position,
                rotation: properties.rotation,
                dimensions: properties.dimensions,
            });
        }
    };

    this.setVisible = function(isVisible) {
        if (visible != isVisible) {
            visible = isVisible;
            for (var id in entityOverlays) {
                Overlays.editOverlay(entityOverlays[id].solid, { visible: visible });
                Overlays.editOverlay(entityOverlays[id].outline, { visible: visible });
            }
        }
    };

    // Allocate or get an unused overlay
    function getOverlay() {
        if (unusedOverlays.length == 0) {
            var overlay = Overlays.addOverlay("cube", {
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
        if (properties.type == "Zone" && !(entityID.id in entityOverlays)) {
            var overlaySolid = getOverlay();
            var overlayOutline = getOverlay();

            entityOverlays[entityID.id] = {
                solid: overlaySolid,
                outline: overlayOutline,
            }
            entityIDs[entityID.id] = entityID;

            var color = {
                red: Math.round(Math.random() * 255),
                green: Math.round(Math.random() * 255),
                blue: Math.round(Math.random() * 255)
            };
            Overlays.editOverlay(overlaySolid, {
                position: properties.position,
                rotation: properties.rotation,
                dimensions: properties.dimensions,
                visible: visible,
                solid: true,
                alpha: 0.1,
                color: color,
                ignoreRayIntersection: true,
            });
            Overlays.editOverlay(overlayOutline, {
                position: properties.position,
                rotation: properties.rotation,
                dimensions: properties.dimensions,
                visible: visible,
                solid: false,
                dashed: false,
                lineWidth: 2.0,
                alpha: 1.0,
                color: color,
                ignoreRayIntersection: true,
            });
        }
    }

    function deleteEntity(entityID) {
        if (entityID.id in entityOverlays) {
            releaseOverlay(entityOverlays[entityID.id].outline);
            releaseOverlay(entityOverlays[entityID.id].solid);
            delete entityIDs[entityID.id];
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
            releaseOverlay(entityOverlays[id].outline);
            releaseOverlay(entityOverlays[id].solid);
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
