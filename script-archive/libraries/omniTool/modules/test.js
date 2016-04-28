
Script.include("../../avatarRelativeOverlays.js");

OmniToolModules.Test = function(omniTool, activeEntityId) {
    this.omniTool = omniTool;
    this.activeEntityId = activeEntityId;
    this.avatarOverlays = new AvatarRelativeOverlays();
}

OmniToolModules.Test.prototype.onUnload = function() {
    if (this.testOverlay) {
        Overlays.deleteOverlay(this.testOverlay);
        this.testOverlay = 0;
    }
}

var CUBE_POSITION = {
    x: 0.1,
    y: -0.1,
    z: -0.4
};

OmniToolModules.Test.prototype.onClick = function() {
    if (this.testOverlay) {
        Overlays.deleteOverlay(this.testOverlay);
        this.testOverlay = 0;
    }
}


OmniToolModules.Test.prototype.onUpdate = function(deltaTime) {
    this.avatarOverlays.onUpdate(deltaTime);
}


OmniToolModuleType = "Test"