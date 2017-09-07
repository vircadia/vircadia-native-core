//
//  accountUtils.js
//  scripts/system/libraries/libraries
//
//  Copyright 2017 High Fidelity, Inc.
//

openLoginWindow = function openLoginWindow() {
    if ((HMD.active && Settings.getValue("hmdTabletBecomesToolbar", false))
        || (!HMD.active && Settings.getValue("desktopTabletBecomesToolbar", true))) {
        Menu.triggerOption("Login / Sign Up");
    } else {
        tablet.loadQMLOnTop("../../dialogs/TabletLoginDialog.qml");
        HMD.openTablet();
    }
};        
