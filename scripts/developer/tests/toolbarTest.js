(function () {

    // Get the system toolbar.
    var toolbar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
    if (!toolbar) {
        print("ERROR: Couldn't get system toolbar.");
        return;
    }

    Script.setTimeout(function () {
        // Report the system toolbar visibility.
        var isToolbarVisible = toolbar.readProperty("visible");
        print("Toolbar visible: " + isToolbarVisible);

        // Briefly toggle the system toolbar visibility.
        print("Toggle toolbar");
        toolbar.writeProperty("visible", !isToolbarVisible);
        Script.setTimeout(function () {
            print("Toggle toolbar");
            toolbar.writeProperty("visible", isToolbarVisible);
        }, 2000);
    }, 2000);

    Script.setTimeout(function () {
        // Report the system toolbar visibility alternative method.
        isToolbarVisible = toolbar.readProperties(["visible"]).visible;
        print("Toolbar visible: " + isToolbarVisible);

        // Briefly toggle the system toolbar visibility.
        print("Toggle toolbar");
        toolbar.writeProperties({ visible: !isToolbarVisible });
        Script.setTimeout(function () {
            print("Toggle toolbar");
            toolbar.writeProperties({ visible: isToolbarVisible });
        }, 2000);
    }, 6000);

}());
