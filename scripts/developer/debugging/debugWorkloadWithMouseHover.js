//
//  debugWorkloadWithMouseHover.js - render workload proxy for entity under mouse hover
//
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

"use strict";

(function() {
   
    Script.scriptEnding.connect(function () {
    });
   
    // Create a Laser pointer used to pick and add entity to selection
    var END_DIMENSIONS = { x: 0.05, y: 0.05, z: 0.05 };
    var COLOR1 = {red: 255, green: 0, blue: 255}; // magenta
    var COLOR2 = {red: 255, green: 255, blue: 0}; // yellow
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
        filter: Picks.PICK_ENTITIES | Picks.PICK_BYPASS_IGNORE | Picks.PICK_INCLUDE_COLLIDABLE | Picks.PICK_INCLUDE_NONCOLLIDABLE,
        renderStates: [{name: "one", end: end1}],
        defaultRenderStates: [{name: "one", end: end2, distance: 2.0}],
        enabled: true
    });
    Pointers.setRenderState(laser, "one");
    var hoveredObject = undefined;

    var SelectionListName = "DebugWorkloadSelection"; // sekret undocumented selection list (hard coded in C++)
    var selectionStyle = {
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
    Selection.enableListHighlight(SelectionListName, selectionStyle)
    
    var isSelectionEnabled = false
     
    function setSelectionEnabled(enabled) {
        if (isSelectionEnabled != enabled) {
            isSelectionEnabled = enabled;
            //print("isSelectionEnabled set to " + isSelectionEnabled.toString())
            if (isSelectionEnabled) {
                Pointers.enablePointer(laser)
            } else {
                Pointers.disablePointer(laser)
                Selection.clearSelectedItemsList(SelectionListName)        
            }
        }
    }
    setSelectionEnabled(true);

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
            if (hoveredObject !== undefined && result.objectID !== hoveredObject.objectID) {
                // Hovering on something different
                if (isSelectionEnabled) {
                    Selection.removeFromSelectedItemsList(SelectionListName, getIntersectionTypeString(hoveredObject.type), hoveredObject.objectID)
                    //print("remove helloDebugHighlight " + hoveredObject.objectID.toString());
                }
            }

            if (isSelectionEnabled) {
                if (hoveredObject === undefined || result.objectID !== hoveredObject.objectID) {
                    // Hovering over something new
                    Selection.addToSelectedItemsList(SelectionListName, getIntersectionTypeString(result.type), result.objectID);
                    hoveredObject = result;
                    //print("add helloDebugHighlight " + hoveredObject.objectID.toString() + "  type = '" + getIntersectionTypeString(result.type) + "'");
                }
            }
        } else if (hoveredObject !== undefined) {
            // Stopped hovering
            if (isSelectionEnabled) {
                Selection.removeFromSelectedItemsList(SelectionListName, getIntersectionTypeString(hoveredObject.type), hoveredObject.objectID)
                hoveredObject = undefined;
                //print("clear helloDebugHighlight");
            }
        }
    }
    Script.update.connect(update);
    
    function cleanup() {
        Pointers.removePointer(laser);
        Selection.disableListHighlight(SelectionListName)
        Selection.removeListFromMap(SelectionListName)
    
    }
    Script.scriptEnding.connect(cleanup);
         
}()); 


