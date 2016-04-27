//
//  MonoHMD.js
//
//  Created by Chris Collins on 10/5/15
//  Copyright 2015 High Fidelity, Inc.
//
//  This script allows you to switch between mono and stereo mode within the HMD.
//  It will add adition menu to Tools called "IPD". 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html




function setupipdMenu() {
    if (!Menu.menuExists("Tools > IPD")) {
        Menu.addMenu("Tools > IPD");
    }
    if (!Menu.menuItemExists("Tools > IPD", "Stereo")) {
        Menu.addMenuItem({
            menuName: "Tools > IPD",
            menuItemName: "Stereo",
            isCheckable: true,
            isChecked: true
        });
    }
    if (!Menu.menuItemExists("Tools > IPD", "Mono")) {
        Menu.addMenuItem({
            menuName: "Tools > IPD",
            menuItemName: "Mono",
            isCheckable: true,
            isChecked: false
        });
    }

}


function menuItemEvent(menuItem) {
    if (menuItem == "Stereo") {
        Menu.setIsOptionChecked("Mono", false);
        HMD.ipdScale = 1.0;

    }
    if (menuItem == "Mono") {
        Menu.setIsOptionChecked("Stereo", false);
        HMD.ipdScale = 0.0;
    }

}



function scriptEnding() {

    Menu.removeMenuItem("Tools > IPD", "Stereo");
    Menu.removeMenuItem("Tools > IPD", "Mono");
    Menu.removeMenu("Tools > IPD");
    //reset the HMD to stereo mode
    HMD.setIPDScale(1.0);

}


setupipdMenu();
Menu.menuItemEvent.connect(menuItemEvent);
Script.scriptEnding.connect(scriptEnding);