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
        ignorePickIntersection: true
    }
    var end2 = {
        type: "sphere",
        dimensions: END_DIMENSIONS,
        color: COLOR2,
        ignorePickIntersection: true
    }
    var laser = Pointers.createPointer(PickType.Ray, {
        joint: "Mouse",
        filter: Picks.PICK_ENTITIES | Picks.PICK_OVERLAYS | Picks.PICK_AVATARS,
        renderStates: [{name: "one", end: end1}],
        defaultRenderStates: [{name: "one", end: end2, distance: 2.0}],
        enabled: true
    });
    Pointers.setRenderState(laser, "one");
    var hoveredObject = undefined;

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
        print("Received message from QML")
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

    function getIntersectionTypeString(type) {
        if (type === Picks.INTERSECTED_ENTITY) {
            return "entity";
        } else if (type === Picks.INTERSECTED_OVERLAY) {
            return "overlay";
        } else if (type === Picks.INTERSECTED_AVATAR) {
            return "avatar";
        }
    }

    function update() {
        var result = Pointers.getPrevPickResult(laser);
        if (result.intersects) {
            // Hovering on something different
            if (hoveredObject !== undefined && result.objectID !== hoveredObject.objectID) {
                if (isSelectionEnabled) {
                    Selection.removeFromSelectedItemsList(HoveringList, getIntersectionTypeString(hoveredObject.type), hoveredObject.objectID)
                }
            }

            // Hovering over something new
            if (isSelectionEnabled) {
                Selection.addToSelectedItemsList(HoveringList, getIntersectionTypeString(result.type), result.objectID);
                hoveredObject = result;
            }
        } else if (hoveredObject !== undefined) {
            // Stopped hovering
            if (isSelectionEnabled) {
                Selection.removeFromSelectedItemsList(HoveringList, getIntersectionTypeString(hoveredObject.type), hoveredObject.objectID)
                hoveredObject = undefined;
            }
        }
    }
    Script.update.connect(update);
    
    function cleanup() {
        Pointers.removePointer(laser);
        Selection.disableListHighlight(HoveringList)
        Selection.removeListFromMap(HoveringList)
    
    }
    Script.scriptEnding.connect(cleanup);
         
}()); 


