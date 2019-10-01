//
//  materialInspector.js
//
//  Created by Sabrina Shanman on 2019-01-17
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
"use strict";

var activeWindow;

// Adapted from Samuel G's material painting script
function getTopMaterial(multiMaterial) {
    // For non-models: multiMaterial[0] will be the top material
    // For models, multiMaterial[0] is the base material, and multiMaterial[1] is the highest priority applied material
    if (multiMaterial.length > 1) {
        if (multiMaterial[1].priority > multiMaterial[0].priority) {
            return multiMaterial[1];
        }
    }

    return multiMaterial[0];
}

function updateMaterial(type, id, meshPart) {
    var mesh = Graphics.getModel(id);
    var meshPartString = meshPart.toString();
    if (!mesh) {
        return;
    }
    var materials = mesh.materialLayers;
    if (!materials[meshPartString] || materials[meshPartString].length <= 0) {
        return;
    }
    
    var topMaterial = getTopMaterial(materials[meshPartString]);
    var materialJSONText = JSON.stringify({
        materialVersion: 1,
        materials: topMaterial.material
    }, null, 2);
    
    toQml({method: "setObjectInfo", params: {id: id, type: type, meshPart: meshPart}});
    toQml({method: "setMaterialJSON", params: {materialJSONText: materialJSONText}});
}

// Adapted from Samuel G's material painting script
function getHoveredMaterialLocation(event) {
    var pickRay = Camera.computePickRay(event.x, event.y);
    var closest;
    var id;
    var type = "Entity";
    var avatar = AvatarManager.findRayIntersection(pickRay);
    var entity = Entities.findRayIntersection(pickRay, true);
    var overlay = Overlays.findRayIntersection(pickRay, true);

    closest = entity;
    id = entity.entityID;

    if (avatar.intersects && avatar.distance < closest.distance) {
        closest = avatar;
        id = avatar.avatarID;
        type = "Avatar";
    } else if (overlay.intersects && overlay.distance < closest.distance) {
        closest = overlay;
        id = overlay.overlayID;
        type = "Overlay";
    }

    if (closest.intersects) {
        return {
            type: type,
            id: id,
            meshPart: (closest.extraInfo.shapeID ? closest.extraInfo.shapeID : 0)
        };
    } else {
        return undefined;
    }
}

var pressedID;
var pressedMeshPart;

function mousePressEvent(event) {
    if (!event.isLeftButton) {
        return;
    }
    
    var result = getHoveredMaterialLocation(event);

    if (result !== undefined) {
        pressedID = result.id;
        pressedMeshPart = result.meshPart;
    }
}

function mouseReleaseEvent(event) {
    if (!event.isLeftButton) {
        return;
    }
    
    var result = getHoveredMaterialLocation(event);
    
    if (result !== undefined && result.id === pressedID && result.meshPart === pressedMeshPart) {
        updateMaterial(result.type, result.id, result.meshPart);
        setSelectedObject(result.id, result.type);
    }
}

function killWindow() {
    activeWindow = undefined;

 //   setWindow(undefined);
}

function toQml(message) {
    if (activeWindow === undefined) {
        return; // Shouldn't happen
    }
    
    activeWindow.sendToQml(message);
}

function fromQml(message) {
    // No cases currently
}

var SELECT_LIST = "luci_materialInspector_SelectionList";
Selection.enableListHighlight(SELECT_LIST, {
    outlineUnoccludedColor: { red: 125, green: 255, blue: 225 }
});
function setSelectedObject(id, type) {
    Selection.clearSelectedItemsList(SELECT_LIST);
    if (id !== undefined && !Uuid.isNull(id)) {
        Selection.addToSelectedItemsList(SELECT_LIST, type.toLowerCase(), id);
    }
}

function setWindow(window) {
    if (activeWindow !== undefined) {
        setSelectedObject(Uuid.NULL, "");
       // activeWindow.closed.disconnect(killWindow);
        activeWindow.fromQml.disconnect(fromQml);
        Controller.mousePressEvent.disconnect(mousePressEvent);
        Controller.mouseReleaseEvent.disconnect(mouseReleaseEvent);
        activeWindow.close();
    }
    if (window !== undefined) {
       // window.closed.connect(killWindow);
        window.fromQml.connect(fromQml);
        Controller.mousePressEvent.connect(mousePressEvent);
        Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
    }
    activeWindow = window;
}

function cleanup() {
    setWindow(undefined);
    Selection.disableListHighlight(SELECT_LIST);
}

Script.scriptEnding.connect(cleanup);

module.exports = {
    setWindow: setWindow
};
