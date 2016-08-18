// Created by james b. pollack @imgntn on 7/2/2016
// Copyright 2016 High Fidelity, Inc.
//advanced movements settings are in individual controller json files
//what we do is check the status of the 'advance movement' checkbox when you enter HMD mode
//if 'advanced movement' is checked...we give you the defaults that are in the json.
//if 'advanced movement' is not checked... we override the advanced controls with basic ones.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var mappingName, basicMapping, isChecked;

var previousSetting = Settings.getValue('advancedMovementForHandControllersIsChecked');
if (previousSetting === '') {
    previousSetting = false;
    isChecked = false;
}

if (previousSetting === true) {
    isChecked = true;
}
if (previousSetting === false) {
    isChecked = false;
}

function addAdvancedMovementItemToSettingsMenu() {
    Menu.addMenuItem({
        menuName: "Settings",
        menuItemName: "Advanced Movement For Hand Controllers",
        isCheckable: true,
        isChecked: previousSetting
    });

}

function rotate180() {
    MyAvatar.orientation = Quat.inverse(MyAvatar.orientation);
}

var inFlipTurn = false;

function registerBasicMapping() {
    mappingName = 'Hifi-AdvancedMovement-Dev-' + Math.random();
    basicMapping = Controller.newMapping(mappingName);
    basicMapping.from(Controller.Standard.LY).to(function(value) {
        var stick = Controller.getValue(Controller.Standard.LS);
        if (value === 1 && Controller.Hardware.OculusTouch !== undefined) {
            rotate180();
        }
        if (Controller.Hardware.Vive !== undefined) {
            if (value > 0.75 && inFlipTurn === false) {
                print('vive should flip turn')
                inFlipTurn = true;
                rotate180();
                Script.setTimeout(function() {
                    print('vive should be able to flip turn again')
                    inFlipTurn = false;
                }, 250)
            } else {
                print('vive should not flip turn')

            }
        }
        return;
    });
    basicMapping.from(Controller.Standard.LX).to(Controller.Standard.RX);
    basicMapping.from(Controller.Standard.RY).to(function(value) {
        var stick = Controller.getValue(Controller.Standard.RS);
        if (value === 1 && Controller.Hardware.OculusTouch !== undefined) {
            rotate180();
        }
        if (Controller.Hardware.Vive !== undefined) {
            if (value > 0.75 && inFlipTurn === false) {
                print('vive should flip turn')
                inFlipTurn = true;
                rotate180();
                Script.setTimeout(function() {
                    print('vive should be able to flip turn again')
                    inFlipTurn = false;
                }, 250)
            } else {
                print('vive should not flip turn')

            }
        }
        print('should do RY stuff' + value + ":stick:" + stick);
        return;
    })
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


function menuItemEvent(menuItem) {
    if (menuItem == "Advanced Movement For Hand Controllers") {
        isChecked = Menu.isOptionChecked("Advanced Movement For Hand Controllers");
        if (isChecked === true) {
            Settings.setValue('advancedMovementForHandControllersIsChecked', true);
            disableMappings();
        } else if (isChecked === false) {
            Settings.setValue('advancedMovementForHandControllersIsChecked', false);
            enableMappings();
        }
    }

}

addAdvancedMovementItemToSettingsMenu();

Script.scriptEnding.connect(scriptEnding);

Menu.menuItemEvent.connect(menuItemEvent);

registerBasicMapping();
if (previousSetting === true) {
    disableMappings();
} else if (previousSetting === false) {
    enableMappings();
}


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