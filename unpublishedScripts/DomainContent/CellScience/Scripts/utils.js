setEntityUserData = function(id, data) {
    var json = JSON.stringify(data)
    Entities.editEntity(id, {
        userData: json
    });
}

// FIXME do non-destructive modification of the existing user data
getEntityUserData = function(id) {
    var results = null;
    var properties = Entities.getEntityProperties(id, "userData");
    if (properties.userData) {
        try {
            results = JSON.parse(properties.userData);
        } catch (err) {
            logDebug(err);
            logDebug(properties.userData);
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