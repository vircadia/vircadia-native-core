//advanced movements settings are in individual controller json files
//what we do is check the status of the 'advance movement' checkbox when you enter HMD mode
//if 'advanced movement' is checked...we give you the defaults that are in the json.
//if 'advanced movement' is not checked... we override the advanced controls with basic ones.
//when the script stops, 

//todo: store in prefs
//


var mappingName, basicMapping;

function addAdvancedMovementItemToSettingsMenu() {
    Menu.addMenuItem({
        menuName: "Settings",
        menuItemName: "Advanced Movement For Hand Controllers",
        isCheckable: true,
        isChecked: false
    });

}

function rotate180() {
    MyAvatar.orientation = Quat.inverse(MyAvatar.orientation);
}

function registerBasicMapping() {
    mappingName = 'Hifi-AdvancedMovement-Dev-' + Math.random();
    basicMapping = Controller.newMapping(mappingName);
    basicMapping.from(Controller.Standard.LY).to(function(value) {
        var stick = Controller.getValue(Controller.Standard.LS);
        if (value === 1) {
            rotate180();
        }
        print('should do LY stuff' + value + ":stick:" + stick);
        return;
    });
    basicMapping.from(Controller.Standard.LX).to(Controller.Standard.RX);
    basicMapping.from(Controller.Standard.RY).to(function(value) {
        var stick = Controller.getValue(Controller.Standard.RS);
        if (value === 1) {
            rotate180();
        }
        print('should do RY stuff' + value + ":stick:" + stick);
        return;
    })
}

function testPrint(what) {
    print('it was controller: ' + what)
}

function enableMappings() {
    Controller.enableMapping(mappingName);
}

function disableMappings() {
    Controller.disableMapping(mappingName);
}

function scriptEnding() {
    Menu.removeMenuItem("Settings", "Advanced Movement For Hand Controllers");
    disableMappings();
}

var isChecked = false;

function menuItemEvent(menuItem) {
    if (menuItem == "Advanced Movement For Hand Controllers") {
        print("  checked=" + Menu.isOptionChecked("Advanced Movement For Hand Controllers"));
        isChecked = Menu.isOptionChecked("Advanced Movement For Hand Controllers");
        if (isChecked === true) {
            disableMappings();
        } else if (isChecked === false) {
            enableMappings();
        }
    }

}

addAdvancedMovementItemToSettingsMenu();

Script.scriptEnding.connect(scriptEnding);

Menu.menuItemEvent.connect(menuItemEvent);

registerBasicMapping();
enableMappings();

HMD.displayModeChanged.connect(function(isHMDMode) {
    if (isHMDMode) {
        if (Controller.Hardware.Vive !== undefined || Controller.Hardware.OculusTouch !== undefined) {
            if (isChecked === true) {
                disableMappings();
            } else if (isChecked === false) {
                enableMappings();
            }

        }
    }
});