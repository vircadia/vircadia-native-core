"use strict";

//
//  debugTransition.js
//  developer/utilities/render
//
//  Olivier Prat, created on 30/04/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var TABLET_BUTTON_NAME = "Transition";
    var QMLAPP_URL = Script.resolvePath("./transition.qml");
    var ICON_URL = Script.resolvePath("../../../system/assets/images/transition-i.svg");
    var ACTIVE_ICON_URL = Script.resolvePath("../../../system/assets/images/transition-a.svg");

   
    var onScreen = false;

    function onClicked() {
        if (onScreen) {
            tablet.gotoHomeScreen();
        } else {
            tablet.loadQMLSource(QMLAPP_URL);
        }
    }

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        text: TABLET_BUTTON_NAME,
        icon: ICON_URL,
        activeIcon: ACTIVE_ICON_URL
    });

    var hasEventBridge = false;

    function wireEventBridge(on) {
        if (!tablet) {
            print("Warning in wireEventBridge(): 'tablet' undefined!");
            return;
        }
        if (on) {
            if (!hasEventBridge) {
                tablet.fromQml.connect(fromQml);
                hasEventBridge = true;
            }
        } else {
            if (hasEventBridge) {
                tablet.fromQml.disconnect(fromQml);
                hasEventBridge = false;
            }
        }
    }

    function onScreenChanged(type, url) {
        if (url === QMLAPP_URL) {
            onScreen = true;
        } else { 
            onScreen = false;
        }
        
        button.editProperties({isActive: onScreen});
        wireEventBridge(onScreen);
    }

    function fromQml(message) {
    }
        
    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);
    
    Script.scriptEnding.connect(function () {
        if (onScreen) {
            tablet.gotoHomeScreen();
        }
        button.clicked.disconnect(onClicked);
        tablet.screenChanged.disconnect(onScreenChanged);
        tablet.removeButton(button);
    });


    // Create a Laser pointer used to pick and add objects to selections
    var END_DIMENSIONS = { x: 0.05, y: 0.05, z: 0.05 };
    var COLOR1 = {red: 255, green: 0, blue: 0}; 
    var COLOR2 = {red: 0, green: 255, blue: 0};
    var end1 = {
        type: "sphere",
        dimensions: END_DIMENSIONS,
        color: COLOR1,
        ignoreRayIntersection: true
    }
    var end2 = {
        type: "sphere",
        dimensions: END_DIMENSIONS,
        color: COLOR2,
        ignoreRayIntersection: true
    }
    var laser = Pointers.createPointer(PickType.Ray, {
        joint: "Mouse",
        filter: Picks.PICK_ENTITIES,
        renderStates: [{name: "one", end: end1}],
        defaultRenderStates: [{name: "one", end: end2, distance: 2.0}],
        enabled: true
    });
    Pointers.setRenderState(laser, "one");

    var currentSelectionName = ""
    var SelectionList = "TransitionEdit"
    var selectedEntity = null
    Pointers.enablePointer(laser)
    Selection.enableListToScene(SelectionList)
    Selection.clearSelectedItemsList(SelectionList)

    Entities.clickDownOnEntity.connect(function (id, event) {
        if (selectedEntity) {
            Selection.removeFromSelectedItemsList(SelectionList, "entity", selectedEntity)
        }
        selectedEntity = id
        Selection.addToSelectedItemsList(SelectionList, "entity", selectedEntity)
    })
  
    function cleanup() {
        Pointers.disablePointer(laser)
        Pointers.removePointer(ray);
        Selection.removeListFromMap(SelectionList)
        Selection.disableListToScene(SelectionList);
    }

}()); 