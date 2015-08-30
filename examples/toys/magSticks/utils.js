//
//  Created by Bradley Austin Davis on 2015/08/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

vec3toStr = function (v, digits) {
    if (!digits) { digits = 3; }
    return "{ " + v.x.toFixed(digits) + ", " + v.y.toFixed(digits) + ", " + v.z.toFixed(digits)+ " }";
}

scaleLine = function (start, end, scale) {
    var v = Vec3.subtract(end, start);
    var length = Vec3.length(v);
    v = Vec3.multiply(scale, v);
    return Vec3.sum(start, v);
}

findAction = function(name) {
    var actions = Controller.getAllActions();
    for (var i = 0; i < actions.length; i++) {
        if (actions[i].actionName == name) {
            return i;
        }
    }
    return 0;
}


var LINE_DIMENSIONS = {
    x: 5,
    y: 5,
    z: 5
}

var EDGE_NAME = "MagStick"

var LINE_PROTOTYPE = {
    type: "Line",
    name: EDGE_NAME,
    color: COLORS.CYAN,
    dimensions: LINE_DIMENSIONS,
    lineWidth: 5,
    visible: true,
    ignoreCollisions: true,
    collisionsWillMove: false
}


addLine = function(origin, vector, color) {
    if (!color) {
        color = COLORS.WHITE
    }
    return Entities.addEntity(mergeObjects(LINE_PROTOTYPE, {
        position: origin,
        linePoints: [
            ZERO_VECTOR,
            vector,
        ],
        color: color
    }));
}

// FIXME fetch from a subkey of user data to support non-destructive modifications
setEntityUserData = function(id, data) {
    Entities.editEntity(id, { userData: JSON.stringify(data) });    
}

// FIXME do non-destructive modification of the existing user data
getEntityUserData = function(id) {
    var results = null;
    var properties = Entities.getEntityProperties(id);
    if (properties.userData) {
        results = JSON.parse(this.properties.userData);    
    }
    return results;
}

// Non-destructively modify the user data of an entity.
setEntityCustomData = function(customKey, id, data) {
    var userData = getEntityUserData(id);
    userData[customKey] = data;
    setEntityUserData(id, userData);
}

getEntityCustomData = function(customKey, id, defaultValue) {
    var userData = getEntityUserData();
    return userData[customKey] ? userData[customKey] : defaultValue;
}

mergeObjects = function(proto, custom) {
    var result = {};
    for (var attrname in proto) {
        result[attrname] = proto[attrname];
    }
    for (var attrname in custom) {
        result[attrname] = custom[attrname];
    }
    return result;
}

logWarn = function(str) {
    print(str);
}

logError = function(str) {
    print(str);
}

logInfo = function(str) {
    print(str);
}

logDebug = function(str) {
    print(str);
}