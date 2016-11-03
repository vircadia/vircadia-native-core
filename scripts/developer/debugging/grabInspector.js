//
//  grabInspector.js
//  examples/debugging/
//
//  Created by Seth Alves on 2015-9-30.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../libraries/utils.js");

var INSPECT_RADIUS = 10;
var overlays = {};

var toType = function(obj) {
    return ({}).toString.call(obj).match(/\s([a-zA-Z]+)/)[1].toLowerCase()
}

function grabDataToString(grabData) {
    var result = "";

    for (var argumentName in grabData) {
        if (grabData.hasOwnProperty(argumentName)) {
            if (argumentName == "type") {
                continue;
            }
            var arg = grabData[argumentName];
            var argType = toType(arg);
            var argString = arg;
            if (argType == "object") {
                if (Object.keys(arg).length == 3) {
                    argString = vec3toStr(arg, 1);
                }
            } else if (argType == "number") {
                argString = arg.toFixed(2);
            }
            result += argumentName + ": "
                // + toType(arg) + " -- "
                + argString + "\n";
        }
    }

    return result;
}



function updateOverlay(entityID, grabText) {
    var properties = Entities.getEntityProperties(entityID, ["position", "dimensions"]);
    var position = Vec3.sum(properties.position, {
        x: 0,
        y: properties.dimensions.y,
        z: 0
    });
    if (entityID in overlays) {
        var overlay = overlays[entityID];
        Overlays.editOverlay(overlay, {
            text: grabText,
            position: position
        });
    } else {
        var lines = grabText.split(/\r\n|\r|\n/);

        var maxLineLength = lines[0].length;
        for (var i = 1; i < lines.length; i++) {
            if (lines[i].length > maxLineLength) {
                maxLineLength = lines[i].length;
            }
        }

        var textWidth = maxLineLength * 0.034; // XXX how to know this?
        var textHeight = .5;
        var numberOfLines = lines.length;
        var textMargin = 0.05;
        var lineHeight = (textHeight - (2 * textMargin)) / numberOfLines;

        overlays[entityID] = Overlays.addOverlay("text3d", {
            position: position,
            dimensions: {
                x: textWidth,
                y: textHeight
            },
            backgroundColor: {
                red: 0,
                green: 0,
                blue: 0
            },
            color: {
                red: 255,
                green: 255,
                blue: 255
            },
            topMargin: textMargin,
            leftMargin: textMargin,
            bottomMargin: textMargin,
            rightMargin: textMargin,
            text: grabText,
            lineHeight: lineHeight,
            alpha: 0.9,
            backgroundAlpha: 0.9,
            ignoreRayIntersection: true,
            visible: true,
            isFacingAvatar: true
        });
    }
}


function cleanup() {
    for (var entityID in overlays) {
        Overlays.deleteOverlay(overlays[entityID]);
    }
}


Script.setInterval(function() {
    var nearbyEntities = Entities.findEntities(MyAvatar.position, INSPECT_RADIUS);
    for (var entityIndex = 0; entityIndex < nearbyEntities.length; entityIndex++) {
        var entityID = nearbyEntities[entityIndex];
        var userData = getEntityUserData(entityID);
        var grabData = userData["grabKey"]

        // {"grabbableKey":{"invertSolidWhileHeld":true},
        //  "grabKey":{"activated":true,"avatarId":"{6ea8b092-10e0-4058-888b-6facc40d0fe9}","refCount":1,"gravity":{"x":0,"y":0,"z":0},"collisionless":0,"dynamic":1}
        // }

        if (typeof grabData != 'undefined') {
            var grabText = grabDataToString(grabData);
            updateOverlay(entityID, grabText);
        } else {
            if (entityID in overlays) {
                Overlays.deleteOverlay(overlays[entityID]);
                delete overlays[entityID];
            }
        }
    }

    // if an entity is too far away, remove its overlay
    for (var entityID in overlays) {
        var position = Entities.getEntityProperties(entityID, ["position"]).position;
        if (Vec3.distance(position, MyAvatar.position) > INSPECT_RADIUS) {
            Overlays.deleteOverlay(overlays[entityID]);
            delete overlays[entityID];
        }
    }

}, 100);


Script.scriptEnding.connect(cleanup);