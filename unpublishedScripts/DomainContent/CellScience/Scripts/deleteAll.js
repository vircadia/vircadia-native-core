function deleteAllInRadius(r) {
    var n = 0;
    var arrayFound = Entities.findEntities(MyAvatar.position, r);
    for (var i = 0; i < arrayFound.length; i++) {
        Entities.deleteEntity(arrayFound[i]);
    }
    print("deleted " + arrayFound.length + " entities");
}

deleteAllInRadius(100000);