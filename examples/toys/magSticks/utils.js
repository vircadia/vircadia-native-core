
DEBUG_MAGSTICKS = true;

debugPrint = function (str) {
    if (DEBUG_MAGSTICKS) {
        print(str);
    }
}

vec3toStr = function (v) {
    return "{ " +
        (Math.round(v.x*1000)/1000) + ", " +
        (Math.round(v.y*1000)/1000) + ", " +
        (Math.round(v.z*1000)/1000) + " }";
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
    debugPrint(str);
}