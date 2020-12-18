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
            Entities.editEntity(entityOverlays[entityID], overlayProperties);
        }
    };

    // Finds the id for the corresponding entity that is associated with an overlay id. 
    // Returns null if the overlay id is not contained in this manager.
    this.findEntity = function(overlayId) {
        for (var id in entityOverlays) {
            if (overlayId === entityOverlays[id]) {
                return entityIDs[id];
            }
        }
        
        return null;
    };

    this.findRayIntersection = function(pickRay) {
        var result = Overlays.findRayIntersection(pickRay);

        if (result.intersects) {
            result.entityID = this.findEntity(result.overlayID);

            if (result.entityID === null) {
                result.intersects = false;
            }
        }

        return result;
    };

    this.setVisible = function(isVisible) {
        if (visible !== isVisible) {
            visible = isVisible;
            for (var id in entityOverlays) {
                Entities.editEntity(entityOverlays[id], {
                    visible: visible,
                    ignorePickIntersection: !visible
                });
            }
        }
    };

    // Allocate or get an unused overlay
    function getOverlay() {
        var overlay;
        if (unusedOverlays.length === 0) {
            overlay = Entities.addEntity({
                type: "Image", 
                billboardMode: "full", 
                emissive: true
            }, "local");
            allOverlays.push(overlay);
        } else {
            overlay = unusedOverlays.pop();
        }
        return overlay;
    }

    function releaseOverlay(overlay) {
        unusedOverlays.push(overlay);
        Entities.editEntity(overlay, {
            visible: false,
            ignorePickIntersection: true
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
                ignorePickIntersection: !visible,
                alpha: 0.9,
                dimensions: { x: 0.5, y: 0.5, z: 0.01 },
                renderLayer: "front",
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
            Entities.editEntity(overlay, overlayProperties); 
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
            Entities.deleteEntity(allOverlays[i]);
        }
    });
};
