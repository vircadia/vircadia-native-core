(function () {
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    this.clickDownOnEntity = function (entityID, mouseEvent) {
        tablet.loadQMLSource("hifi/AvatarApp.qml");
    };
}
);
