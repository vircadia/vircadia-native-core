var mappingName, advancedMapping;

function addAdvancedMovementItemToSettingsMenu() {
    Menu.addMenuItem({
        menuName: "Settings",
        menuItemName: "Advanced Movement",
        isCheckable: true,
        isChecked: false
    });

}

function addTranslationToLeftStick() {
    Controller.enableMapping(mappingName);
}

function registerMappings() {
    mappingName = 'Hifi-AdvancedMovement-Dev-' + Math.random();
    advancedMapping = Controller.newMapping(mappingName);
    advancedMapping.from(Controller.Hardware.Vive.LY).when(Controller.getValue(Controller.Hardware.Vive.LSY)).invert().to(
        function(val) {
            testPrint('LY:' + val);
        });
    advancedMapping.from(Controller.Hardware.Vive.LX).when(Controller.getValue(Controller.Hardware.Vive.LSX)).to(
        function(val) {
            testPrint('LX' + val)
        });
    advancedMapping.from(Controller.Hardware.Vive.RY).when(Controller.getValue(Controller.Hardware.Vive.RSY)).invert().to(
        function(val) {
            testPrint('RY' + val)
        });
}

function testPrint(what) {
    print(what)
}

function removeTranslationFromLeftStick() {
    Controller.disableMapping(mappingName);
}

function scriptEnding() {
    Menu.removeMenuItem("Settings", "Advanced Movement");
    removeTranslationFromLeftStick();
}

function menuItemEvent(menuItem) {
    if (menuItem == "Advanced Movement") {
        print("  checked=" + Menu.isOptionChecked("Advanced Movement"));
        var isChecked = Menu.isOptionChecked("Advanced Movement");
        if (isChecked === true) {
            addTranslationToLeftStick();
        } else if (isChecked === false) {
            removeTranslationFromLeftStick();
        }
    }

}

addAdvancedMovementItemToSettingsMenu();

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);

Menu.menuItemEvent.connect(menuItemEvent);

registerMappings();