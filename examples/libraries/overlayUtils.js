//
//  overlayUtils.js
//  examples/libraries
//
//  Copyright 2015 High Fidelity, Inc.
//


//
//  DEPRECATION WARNING: Will be deprecated soon in favor of FloatingUIPanel.
//
//  OverlayGroup provides a way to create composite overlays and control their
//  position relative to a settable rootPosition and rootRotation.
//
OverlayGroup = function(opts) {
    var that = {};

    var overlays = {};

    var rootPosition = opts.position || { x: 0, y: 0, z: 0 };
    var rootRotation = opts.rotation || Quat.fromPitchYawRollRadians(0, 0, 0);
    var visible = opts.visible == true;

    function updateOverlays() {
        for (overlayID in overlays) {
            var overlay = overlays[overlayID];
            var newPosition = Vec3.multiplyQbyV(rootRotation, overlay.position);
            newPosition = Vec3.sum(rootPosition, newPosition);
            Overlays.editOverlay(overlayID, {
                visible: visible,
                position: newPosition,
                rotation: Quat.multiply(rootRotation, overlay.rotation),
            });
        };
    }

    that.createOverlay = function(type, properties) {
        properties.position = properties.position || { x: 0, y: 0, z: 0 };
        properties.rotation = properties.rotation || Quat.fromPitchYawRollRadians(0, 0, 0);

        var overlay = Overlays.addOverlay(type, properties);

        overlays[overlay] = {
            position: properties.position,
            rotation: properties.rotation,
        };

        updateOverlays();

        return overlay;
    }

    that.setProperties = function(properties) {
        if (properties.position !== undefined) {
            rootPosition = properties.position;
        }
        if (properties.rotation !== undefined) {
            rootRotation = properties.rotation;
        }
        if (properties.visible !== undefined) {
            visible = properties.visible;
        }
        updateOverlays();
    };

    that.destroy = function() {
        for (var overlay in overlays) {
            Overlays.deleteOverlay(overlay);
        }
        overlays = {};
    }

    return that;
};
