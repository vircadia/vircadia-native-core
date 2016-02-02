//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function deleteAllInRadius(r) {
    var n = 0;
    var arrayFound = Entities.findEntities(MyAvatar.position, r);
    for (var i = 0; i < arrayFound.length; i++) {
        Entities.deleteEntity(arrayFound[i]);
    }
    print("deleted " + arrayFound.length + " entities");
}

deleteAllInRadius(100000);