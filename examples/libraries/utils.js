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


colorMix = function(colorA, colorB, mix) {
    var result = {};
    for (var key in colorA) {
        result[key] = (colorA[key] * (1 - mix)) + (colorB[key] * mix);
    }
    return result;
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
    var json = JSON.stringify(data)
    Entities.editEntity(id, { userData: json });    
}

// FIXME do non-destructive modification of the existing user data
getEntityUserData = function(id) {
    var results = null;
    var properties = Entities.getEntityProperties(id);
    if (properties.userData) {
        try {
            results = JSON.parse(properties.userData);    
        } catch(err) {
            logDebug(err);
            logDebug(properties.userData);
        }
    }
    return results ? results : {};
}


// Non-destructively modify the user data of an entity.
setEntityCustomData = function(customKey, id, data) {
    var userData = getEntityUserData(id);
    userData[customKey] = data;
    setEntityUserData(id, userData);
}

getEntityCustomData = function(customKey, id, defaultValue) {
    var userData = getEntityUserData(id);
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

LOG_WARN = 1;

logWarn = function(str) {
    if (LOG_WARN) {
        print(str);
    }
}

LOG_ERROR = 1;

logError = function(str) {
    if (LOG_ERROR) {
        print(str);
    }
}

LOG_INFO = 1;

logInfo = function(str) {
    if (LOG_INFO) {
        print(str);
    }
}

LOG_DEBUG = 0;

logDebug = function(str) {
    if (LOG_DEBUG) {
        print(str);
    }
}

LOG_TRACE = 0;

logTrace = function(str) {
    if (LOG_TRACE) {
        print(str);
    }
}

// Computes the penetration between a point and a sphere (centered at the origin)
// if point is inside sphere: returns true and stores the result in 'penetration'
// (the vector that would move the point outside the sphere)
// otherwise returns false
findSphereHit = function(point, sphereRadius) {
    var EPSILON = 0.000001;	//smallish positive number - used as margin of error for some computations
    var vectorLength = Vec3.length(point);
    if (vectorLength < EPSILON) {
        return true;
    }
    var distance = vectorLength - sphereRadius;
    if (distance < 0.0) {
        return true;
    }
    return false;
}

findSpherePointHit = function(sphereCenter, sphereRadius, point) {
    return findSphereHit(Vec3.subtract(point,sphereCenter), sphereRadius);
}

findSphereSphereHit = function(firstCenter, firstRadius, secondCenter, secondRadius) {
    return findSpherePointHit(firstCenter, firstRadius + secondRadius, secondCenter);
}

// Given a vec3 v, return a vec3 that is the same vector relative to the avatars
// DEFAULT eye position, rotated into the avatars reference frame.
getEyeRelativePosition = function(v) {
    return Vec3.sum(MyAvatar.getDefaultEyePosition(), Vec3.multiplyQbyV(MyAvatar.orientation, v));
}

getAvatarRelativeRotation = function(q) {
    return Quat.multiply(MyAvatar.orientation, q);
}

pointInExtents = function(point, minPoint, maxPoint) {
    return (point.x >= minPoint.x && point.x <= maxPoint.x) &&
           (point.y >= minPoint.y && point.y <= maxPoint.y) &&
           (point.z >= minPoint.z && point.z <= maxPoint.z);
}