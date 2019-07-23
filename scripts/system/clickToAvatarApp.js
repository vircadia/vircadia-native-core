(function () {
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    
    
    this.clickDownOnEntity = function (entityID, mouseEvent) {
        var scripts = ScriptDiscoveryService.getRunning();
       
        var runningSimplified =
            !scripts.every(function (item) {
                return item.name !== "simplifiedUI.js";
            });

        var avatarAppQML = runningSimplified ? "hifi/simplifiedUI/avatarApp/AvatarApp.qml" : "hifi/AvatarApp.qml";
        tablet.loadQMLSource(avatarAppQML);
    };
}
);
