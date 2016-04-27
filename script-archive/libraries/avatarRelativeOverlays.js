
AvatarRelativeOverlays = function() {
    // id -> position & rotation
    this.overlays = {};
    this.lastAvatarTransform = {
        position: ZERO_VECTOR,
        rotation: IDENTITY_QUATERNION,
    };
}

// FIXME judder in movement is annoying....  add an option to
// automatically hide all overlays when the position or orientation change and then
// restore the ones that were previously visible once the movement stops.
AvatarRelativeOverlays.prototype.onUpdate = function(deltaTime) {
    // cache avatar position and orientation and only update on change
    if (Vec3.equal(this.lastAvatarTransform.position, MyAvatar.position) &&
        Quat.equal(this.lastAvatarTransform.rotation, MyAvatar.orientation)) {
        return;
    }

    this.lastAvatarTransform.position = MyAvatar.position;
    this.lastAvatarTransform.rotation = MyAvatar.orientation;
    for (var overlayId in this.overlays) {
        this.updateOverlayTransform(overlayId);
    }
}

AvatarRelativeOverlays.prototype.updateOverlayTransform = function(overlayId) {
    Overlays.editOverlay(overlayId, {
        position: getEyeRelativePosition(this.overlays[overlayId].position),
        rotation: getAvatarRelativeRotation(this.overlays[overlayId].rotation),
    })
}

AvatarRelativeOverlays.prototype.addOverlay = function(type, overlayDefinition) {
    var overlayId = Overlays.addOverlay(type, overlayDefinition);
    if (!overlayId) {
        logDebug("Failed to create overlay of type " + type);
        return;
    }
    this.overlays[overlayId] = {
        position: overlayDefinition.position || ZERO_VECTOR,
        rotation: overlayDefinition.rotation || IDENTITY_QUATERNION,
    };
    this.updateOverlayTransform(overlayId);
    return overlayId;
}

AvatarRelativeOverlays.prototype.deleteAll = function() {
    for (var overlayId in this.overlays) {
        Overlays.deleteOverlay(overlayId);
    }
    this.overlays = {};
}

