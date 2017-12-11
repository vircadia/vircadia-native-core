//
//  debugHighlight.js
//  developer/utilities/render
//
//  Olivier Prat, created on 08/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

"use strict";

(function() {
    var TABLET_BUTTON_NAME = "Highlight";
    var QMLAPP_URL = Script.resolvePath("./highlight.qml");
    var ICON_URL = Script.resolvePath("../../../system/assets/images/luci-i.svg");
    var ACTIVE_ICON_URL = Script.resolvePath("../../../system/assets/images/luci-a.svg");

   
    var onLuciScreen = false;

    function onClicked() {
        if (onLuciScreen) {
            tablet.gotoHomeScreen();
        } else {
            tablet.loadQMLSource(QMLAPP_URL);
        }
    }

    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var button = tablet.addButton({
        text: TABLET_BUTTON_NAME,
        icon: ICON_URL,
        activeIcon: ACTIVE_ICON_URL,
        sortOrder: 1
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
            onLuciScreen = true;
        } else { 
            onLuciScreen = false;
        }
        
        button.editProperties({isActive: onLuciScreen});
        wireEventBridge(onLuciScreen);
    }
     
    button.clicked.connect(onClicked);
    tablet.screenChanged.connect(onScreenChanged);

    Script.scriptEnding.connect(function () {
        if (onLuciScreen) {
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

    var HoveringList = "Hovering"
    var hoveringStyle = {
        isOutlineSmooth: true,
        outlineWidth: 5,
        outlineUnoccludedColor: {red: 255, green: 128, blue: 128}, 
        outlineUnoccludedAlpha: 0.88,
        outlineOccludedColor: {red: 255, green: 128, blue: 128},
        outlineOccludedAlpha:0.5,
        fillUnoccludedColor: {red: 26, green: 0, blue: 0},
        fillUnoccludedAlpha: 0.0,
        fillOccludedColor: {red: 26, green: 0, blue: 0},
        fillOccludedAlpha: 0.0
    }
    Selection.enableListHighlight(HoveringList, hoveringStyle)
    
    var currentSelectionName = ""
    var isSelectionEnabled = false
    Pointers.disablePointer(laser)
     
    function fromQml(message) {
        tokens = message.split(' ')
        print("Received '"+message+"' from hightlight.qml")
        if (tokens[0]=="highlight") {
            currentSelectionName = tokens[1];
            print("Switching to highlight name "+currentSelectionName)
        } else if (tokens[0]=="pick") {
            isSelectionEnabled = tokens[1]=='true'
            print("Ray picking set to "+isSelectionEnabled.toString())
            if (isSelectionEnabled) {
                Pointers.enablePointer(laser)
            } else {
                Pointers.disablePointer(laser)
                Selection.clearSelectedItemsList(HoveringList)        
            }
            time = 0
        }
    }
    
    Entities.hoverEnterEntity.connect(function (id, event) {
      //  print("hoverEnterEntity");
        if (isSelectionEnabled) Selection.addToSelectedItemsList(HoveringList, "entity", id)
    })
    
    Entities.hoverOverEntity.connect(function (id, event) {
       // print("hoverOverEntity");
    })

    
    Entities.hoverLeaveEntity.connect(function (id, event) {
        if (isSelectionEnabled)  Selection.removeFromSelectedItemsList(HoveringList, "entity", id)        
       // print("hoverLeaveEntity");   
    })
    
    function cleanup() {
        Pointers.removePointer(ray);
        Selection.disableListHighlight(HoveringList)
        Selection.removeListFromMap(HoveringList)
    
    }
    Script.scriptEnding.connect(cleanup);
         
}()); 


