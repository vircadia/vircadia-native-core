var isActive = false;

var toolBar = (function() {
    var that = {},
        toolBar,
        activeButton,
        newModelButton,
        newCubeButton,
        newSphereButton,
        newLightButton,
        newTextButton,
        newWebButton,
        newZoneButton,
        newParticleButton

    var toolIconUrl = Script.resolvePath("../../system/assets/images/tools/");

    function initialize() {
        print("Toolbars: " + Toolbars);
        toolBar = Toolbars.getToolbar("highfidelity.edit.toolbar");
        print("Toolbar: " + toolBar);
        activeButton = toolBar.addButton({
            objectName: "activeButton",
            imageURL: toolIconUrl + "edit-01.svg",
            visible: true,
            alpha: 0.9,
        });

        print("Button " + activeButton);
        print("Button signal " + activeButton.clicked);
        activeButton.clicked.connect(function(){
            print("Clicked on button " + isActive);
            that.setActive(!isActive);
        });

        newModelButton = toolBar.addButton({
            objectName: "newModelButton",
            imageURL: toolIconUrl + "model-01.svg",
            alpha: 0.9,
            visible: false
        });

        newCubeButton = toolBar.addButton({
            objectName: "newCubeButton",
            imageURL: toolIconUrl + "cube-01.svg",
            alpha: 0.9,
            visible: false
        });

        newSphereButton = toolBar.addButton({
            objectName: "newSphereButton",
            imageURL: toolIconUrl + "sphere-01.svg",
            alpha: 0.9,
            visible: false
        });

        newLightButton = toolBar.addButton({
            objectName: "newLightButton",
            imageURL: toolIconUrl + "light-01.svg",
            alpha: 0.9,
            visible: false
        });

        newTextButton = toolBar.addButton({
            objectName: "newTextButton",
            imageURL: toolIconUrl + "text-01.svg",
            alpha: 0.9,
            visible: false
        });

        newWebButton = toolBar.addButton({
            objectName: "newWebButton",
            imageURL: toolIconUrl + "web-01.svg",
            alpha: 0.9,
            visible: false
        });

        newZoneButton = toolBar.addButton({
            objectName: "newZoneButton",
            imageURL: toolIconUrl + "zone-01.svg",
            alpha: 0.9,
            visible: false
        });

        newParticleButton = toolBar.addButton({
            objectName: "newParticleButton",
            imageURL: toolIconUrl + "particle-01.svg",
            alpha: 0.9,
            visible: false
        });

        that.setActive(false);
        newModelButton.clicked();
    }

    that.setActive = function(active) {
        if (active != isActive) {
            isActive = active;
            that.showTools(isActive);
        }
    };

    // Sets visibility of tool buttons, excluding the power button
    that.showTools = function(doShow) {
        newModelButton.writeProperty('visible', doShow);
        newCubeButton.writeProperty('visible', doShow);
        newSphereButton.writeProperty('visible', doShow);
        newLightButton.writeProperty('visible', doShow);
        newTextButton.writeProperty('visible', doShow);
        newWebButton.writeProperty('visible', doShow);
        newZoneButton.writeProperty('visible', doShow);
        newModelButton.writeProperty('visible', doShow);
        newParticleButton.writeProperty('visible', doShow);
    };

    initialize();
    return that;
}());
