//
//  Created by Bradley Austin Davis on 2015/08/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// note: this constant is currently duplicated in edit.js and ambientSound.js
EDIT_SETTING = "io.highfidelity.isEditing";
isInEditMode = function isInEditMode() {
    return Settings.getValue(EDIT_SETTING);
};

if (!Function.prototype.bind) {
    Function.prototype.bind = function(oThis) {
        if (typeof this !== 'function') {
            // closest thing possible to the ECMAScript 5
            // internal IsCallable function
            throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
        }

        var aArgs   = Array.prototype.slice.call(arguments, 1),
            fToBind = this,
            fNOP    = function() {},
            fBound  = function() {
                return fToBind.apply(this instanceof fNOP
                        ? this
                        : oThis,
                        aArgs.concat(Array.prototype.slice.call(arguments)));
            };

        if (this.prototype) {
            // Function.prototype doesn't have a prototype property
            fNOP.prototype = this.prototype;
        }
        fBound.prototype = new fNOP();

        return fBound;
    };
}

vec3toStr = function(v, digits) {
    if (!digits) { digits = 3; }
    return "{ " + v.x.toFixed(digits) + ", " + v.y.toFixed(digits) + ", " + v.z.toFixed(digits)+ " }";
}

quatToStr = function(q, digits) {
    if (!digits) { digits = 3; }
    return "{ " + q.w.toFixed(digits) + ", " + q.x.toFixed(digits) + ", " +
        q.y.toFixed(digits) + ", " + q.z.toFixed(digits)+ " }";
}

vec3equal = function(v0, v1) {
    return (v0.x == v1.x) && (v0.y == v1.y) && (v0.z == v1.z);
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
    return Controller.findAction(name);
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
    var properties = Entities.getEntityProperties(id, "userData");
    if (properties.userData) {
        try {
            results = JSON.parse(properties.userData);
        } catch(err) {
            logDebug(err);
        }
    }
    return results ? results : {};
}


// Non-destructively modify the user data of an entity.
setEntityCustomData = function(customKey, id, data) {
    var userData = getEntityUserData(id);
    if (data == null) {
        delete userData[customKey];
    } else {
        userData[customKey] = data;
    }
    setEntityUserData(id, userData);
}

getEntityCustomData = function(customKey, id, defaultValue) {
    var userData = getEntityUserData(id);
    if (undefined != userData[customKey]) {
        return userData[customKey];
    } else {
        return defaultValue;
    }
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
    var EPSILON = 0.000001;     //smallish positive number - used as margin of error for some computations
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

/**
 * Converts an HSL color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns r, g, and b in the set [0, 255].
 *
 * @param   Number  h       The hue
 * @param   Number  s       The saturation
 * @param   Number  l       The lightness
 * @return  Array           The RGB representation
 */
hslToRgb = function(hsl) {
    var r, g, b;
    if (hsl.s == 0) {
        r = g = b = hsl.l; // achromatic
    } else {
        var hue2rgb = function hue2rgb(p, q, t) {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1 / 6) return p + (q - p) * 6 * t;
            if (t < 1 / 2) return q;
            if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
            return p;
        }

        var q = hsl.l < 0.5 ? hsl.l * (1 + hsl.s) : hsl.l + hsl.s - hsl.l * hsl.s;
        var p = 2 * hsl.l - q;
        r = hue2rgb(p, q, hsl.h + 1 / 3);
        g = hue2rgb(p, q, hsl.h);
        b = hue2rgb(p, q, hsl.h - 1 / 3);
    }

    return {
        red: Math.round(r * 255),
        green: Math.round(g * 255),
        blue: Math.round(b * 255)
    };
}

map = function(value, min1, max1, min2, max2) {
    return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
}

orientationOf = function(vector) {
    var Y_AXIS = {
        x: 0,
        y: 1,
        z: 0
    };
    var X_AXIS = {
        x: 1,
        y: 0,
        z: 0
    };

    var theta = 0.0;

    var RAD_TO_DEG = 180.0 / Math.PI;
    var direction, yaw, pitch;
    direction = Vec3.normalize(vector);
    yaw = Quat.angleAxis(Math.atan2(direction.x, direction.z) * RAD_TO_DEG, Y_AXIS);
    pitch = Quat.angleAxis(Math.asin(-direction.y) * RAD_TO_DEG, X_AXIS);
    return Quat.multiply(yaw, pitch);
}

randFloat = function(low, high) {
    return low + Math.random() * (high - low);
}


randInt = function(low, high) {
    return Math.floor(randFloat(low, high));
}


randomColor = function() {
    return {
                red: randInt(0, 255),
                green: randInt(0, 255),
                blue: randInt(0, 255)
            }
}


hexToRgb = function(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        red: parseInt(result[1], 16),
        green: parseInt(result[2], 16),
        blue: parseInt(result[3], 16)
    } : null;
}

calculateHandSizeRatio = function() {
    // Get the ratio of the current avatar's hand to Owen's hand

    var standardCenterHandPoint = 0.11288;
    var jointNames = MyAvatar.getJointNames();
    //get distance from handJoint up to leftHandIndex3 as a proxy for center of hand
    var wristToFingertipDistance = 0;;
    for (var i = 0; i < jointNames.length; i++) {
        var jointName = jointNames[i];
        print(jointName)
        if (jointName.indexOf("LeftHandIndex") !== -1) {
            // translations are relative to parent joint, so simply add them together
            // joints face down the y-axis
            var translation = MyAvatar.getDefaultJointTranslation(i).y;
            wristToFingertipDistance += translation;
        }
    }
    // Right now units are in cm, so convert to meters
    wristToFingertipDistance /= 100;

    var centerHandPoint = wristToFingertipDistance/2;

    // Compare against standard hand (Owen)
    var handSizeRatio = centerHandPoint/standardCenterHandPoint;
    return handSizeRatio;
}

hifiClamp = function(val, min, max) {
     return Math.max(min, Math.min(max, val));
}

// flattens an array of arrays into a single array
// example: flatten([[1], [3, 4], []) => [1, 3, 4]
// NOTE: only does one level of flattening, it is not recursive.
flatten = function(array) {
    return [].concat.apply([], array);
}

getTabletWidthFromSettings = function () {
    var DEFAULT_TABLET_WIDTH = 0.4375;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var toolbarMode = tablet.toolbarMode;
    var DEFAULT_DESKTOP_TABLET_SCALE = 75;
    var DEFAULT_HMD_TABLET_SCALE = 60;
    var tabletScalePercentage = DEFAULT_HMD_TABLET_SCALE;
    if (!toolbarMode) {
        if (HMD.active) {
            tabletScalePercentage = Settings.getValue("hmdTabletScale") || DEFAULT_HMD_TABLET_SCALE;
        } else {
            tabletScalePercentage = Settings.getValue("desktopTabletScale") || DEFAULT_DESKTOP_TABLET_SCALE;
        }
    }
    return DEFAULT_TABLET_WIDTH * (tabletScalePercentage / 100);
};

resizeTablet = function (width, newParentJointIndex, sensorToWorldScaleOverride) {

    if (!HMD.tabletID || !HMD.tabletScreenID || !HMD.homeButtonID || !HMD.homeButtonHighlightID) {
        return;
    }
    var sensorScaleFactor = sensorToWorldScaleOverride || MyAvatar.sensorToWorldScale;
    var sensorScaleOffsetOverride = 1;
    var SENSOR_TO_ROOM_MATRIX = 65534;
    var parentJointIndex = newParentJointIndex || Overlays.getProperty(HMD.tabletID, "parentJointIndex");
    if (parentJointIndex === SENSOR_TO_ROOM_MATRIX) {
        sensorScaleOffsetOverride = 1 / sensorScaleFactor;
    }


    // will need to be recaclulated if dimensions of fbx model change.
    var TABLET_NATURAL_DIMENSIONS = {x: 32.083, y: 48.553, z: 2.269};
    var DEFAULT_DPI = 31;
    var DEFAULT_WIDTH = 0.4375;

    // scale factor of natural tablet dimensions.
    var tabletWidth = (width || DEFAULT_WIDTH) * sensorScaleFactor;
    var tabletScaleFactor = tabletWidth / TABLET_NATURAL_DIMENSIONS.x;
    var tabletHeight = TABLET_NATURAL_DIMENSIONS.y * tabletScaleFactor;
    var tabletDepth = TABLET_NATURAL_DIMENSIONS.z * tabletScaleFactor;
    var tabletDpi = DEFAULT_DPI * (DEFAULT_WIDTH / tabletWidth);

    // update tablet model dimensions
    Entities.editEntity(HMD.tabletID, {
        dimensions: { x: tabletWidth, y: tabletHeight, z: tabletDepth }
    });

    // update webOverlay
    var WEB_ENTITY_Z_OFFSET = (tabletDepth / 2.5) * sensorScaleOffsetOverride;
    var WEB_ENTITY_Y_OFFSET = 1.25 * tabletScaleFactor * sensorScaleOffsetOverride;
    var screenWidth = 0.9367 * tabletWidth;
    var screenHeight = 0.9000 * tabletHeight;
    var landscape = Tablet.getTablet("com.highfidelity.interface.tablet.system").landscape;
    Entities.editEntity(HMD.tabletScreenID, {
        localPosition: { x: 0, y: WEB_ENTITY_Y_OFFSET, z: -WEB_ENTITY_Z_OFFSET},
        dimensions: {x: landscape ? screenHeight : screenWidth, y: landscape ? screenWidth : screenHeight, z: 1.0},
        dpi: tabletDpi
    });

    // update homeButton
    var homeButtonDim = 4.0 * tabletScaleFactor / 1.5;
    var HOME_BUTTON_X_OFFSET = 0.00079 * sensorScaleOffsetOverride * sensorScaleFactor;
    var HOME_BUTTON_Y_OFFSET = -1 * ((tabletHeight / 2) - (4.0 * tabletScaleFactor / 2)) * sensorScaleOffsetOverride;
    var HOME_BUTTON_Z_OFFSET = (tabletDepth / 1.9) * sensorScaleOffsetOverride;
    Entities.editEntity(HMD.homeButtonID, {
        localPosition: { x: HOME_BUTTON_X_OFFSET, y: HOME_BUTTON_Y_OFFSET, z: -HOME_BUTTON_Z_OFFSET },
        dimensions: { x: homeButtonDim, y: homeButtonDim, z: homeButtonDim }
    });

    Entities.editEntity(HMD.homeButtonHighlightID, {
        localPosition: { x: -HOME_BUTTON_X_OFFSET, y: HOME_BUTTON_Y_OFFSET, z: -HOME_BUTTON_Z_OFFSET },
        dimensions: { x: homeButtonDim, y: homeButtonDim, z: homeButtonDim }
    });
};


reparentAndScaleTablet = function(width, reparentProps) {

    if (!HMD.tabletID || !HMD.tabletScreenID || !HMD.homeButtonID || !HMD.homeButtonHighlightID) {
        return;
    }
    var sensorScaleFactor = MyAvatar.sensorToWorldScale;
    var sensorScaleOffsetOverride = 1;
    var SENSOR_TO_ROOM_MATRIX = 65534;
    var parentJointIndex = reparentProps.parentJointIndex;
    if (parentJointIndex === SENSOR_TO_ROOM_MATRIX) {
        sensorScaleOffsetOverride = 1 / sensorScaleFactor;
    }


    // will need to be recaclulated if dimensions of fbx model change.
    var TABLET_NATURAL_DIMENSIONS = {x: 32.083, y: 48.553, z: 2.269};
    var DEFAULT_DPI = 31;
    var DEFAULT_WIDTH = 0.4375;

    // scale factor of natural tablet dimensions.
    var tabletWidth = (width || DEFAULT_WIDTH) * sensorScaleFactor;
    var tabletScaleFactor = tabletWidth / TABLET_NATURAL_DIMENSIONS.x;
    var tabletHeight = TABLET_NATURAL_DIMENSIONS.y * tabletScaleFactor;
    var tabletDepth = TABLET_NATURAL_DIMENSIONS.z * tabletScaleFactor;
    var tabletDpi = DEFAULT_DPI * (DEFAULT_WIDTH / tabletWidth);

    // update tablet model dimensions

    Entities.editEntity(HMD.tabletID, {
        parentID: reparentProps.parentID,
        parentJointIndex: reparentProps.parentJointIndex,
        dimensions: { x: tabletWidth, y: tabletHeight, z: tabletDepth}
    });
    // update webOverlay
    var WEB_ENTITY_Z_OFFSET = (tabletDepth / 2.5) * sensorScaleOffsetOverride;
    var WEB_ENTITY_Y_OFFSET = 1.25 * tabletScaleFactor * sensorScaleOffsetOverride;
    var screenWidth = 0.9367 * tabletWidth;
    var screenHeight = 0.9000 * tabletHeight;
    var landscape = Tablet.getTablet("com.highfidelity.interface.tablet.system").landscape;
    Entities.editEntity(HMD.tabletScreenID, {
        localPosition: { x: 0, y: WEB_ENTITY_Y_OFFSET, z: -WEB_ENTITY_Z_OFFSET},
        dimensions: {x: landscape ? screenHeight : screenWidth, y: landscape ? screenWidth : screenHeight, z: 1.0},
        dpi: tabletDpi
    });

    // update homeButton
    var homeButtonDim = 4.0 * tabletScaleFactor / 1.5;
    var HOME_BUTTON_X_OFFSET = 0.00079 * sensorScaleOffsetOverride * sensorScaleFactor;
    var HOME_BUTTON_Y_OFFSET = -1 * ((tabletHeight / 2) - (4.0 * tabletScaleFactor / 2)) * sensorScaleOffsetOverride;
    var HOME_BUTTON_Z_OFFSET = (tabletDepth / 1.9) * sensorScaleOffsetOverride;
    Entities.editEntity(HMD.homeButtonID, {
        localPosition: { x: HOME_BUTTON_X_OFFSET, y: HOME_BUTTON_Y_OFFSET, z: -HOME_BUTTON_Z_OFFSET },
        dimensions: { x: homeButtonDim, y: homeButtonDim, z: homeButtonDim }
    });

    Entities.editEntity(HMD.homeButtonHighlightID, {
        localPosition: { x: -HOME_BUTTON_X_OFFSET, y: HOME_BUTTON_Y_OFFSET, z: -HOME_BUTTON_Z_OFFSET },
        dimensions: { x: homeButtonDim, y: homeButtonDim, z: homeButtonDim }
    });
}

getMainTabletIDs = function () {
    var tabletIDs = [];
    if (HMD.tabletID) {
        tabletIDs.push(HMD.tabletID);
    }
    if (HMD.tabletScreenID) {
        tabletIDs.push(HMD.tabletScreenID);
    }
    if (HMD.homeButtonID) {
        tabletIDs.push(HMD.homeButtonID);
    }
    return tabletIDs;
};

// returns 1 if A is greater, 0 if equal, -1 if A is lesser
semanticVersionCompare = function(versionA, versionB) {
    var versionAParts = versionA.split('.');
    var versionBParts = versionB.split('.');

    // make sure each version has 3 parts
    var partsLength = versionAParts.length;
    while (partsLength < 3) {
        partsLength = versionAParts.push(0);
    }

    partsLength = versionBParts.length;
    while (partsLength < 3) {
        partsLength = versionBParts.push(0);
    }

    for (var i = 0; i < 3; ++i) {
        // We're detecting if there's tags, indicating a pre-release OR it's obsolete.
        var versionAPartHasTag = (versionAParts[i].indexOf('-') !== -1);
        var versionBPartHasTag = (versionBParts[i].indexOf('-') !== -1);

        if (versionAPartHasTag || versionBPartHasTag) {
            // In semantic versioning, if version A has a tag and B does not, B is a higher version.
            if (versionAPartHasTag < versionBPartHasTag) {
                return 1;
            } else if (versionAPartHasTag > versionBPartHasTag) {
                return -1;
            } else {
                // They both have tags
                var splitA = versionAParts[i].split('-')[0];
                var splitB = versionBParts[i].split('-')[0];
                
                if (splitA < splitB) {
                    return -1;
                } else if (splitA > splitB) {
                    return 1;
                }
            }
        } else { // Else, they're both numbers so compare the strings as if they were.
            if (versionAParts[i] < versionBParts[i]) {
                return -1;
            } else if (versionAParts[i] > versionBParts[i]) {
                return 1;
            }
        }
    }
    
    // They're both equal!
    return 0;
}
