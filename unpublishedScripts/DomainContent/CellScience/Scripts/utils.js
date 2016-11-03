//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

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
         //   print('error parsing json');
         //   print('properties are:'+ properties.userData);
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