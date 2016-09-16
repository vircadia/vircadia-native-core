(function() {
    var findEntity = function(properties, searchRadius, filterFn) {
        var entities = findEntities(properties, searchRadius, filterFn);
        return entities.length > 0 ? entities[0] : null;
    }

    // Return all entities with properties `properties` within radius `searchRadius`
    var findEntities = function(properties, searchRadius, filterFn) {
        if (!filterFn) {
            filterFn = function(properties, key, value) {
                return value == properties[key];
            }
        }
        searchRadius = searchRadius ? searchRadius : 100000;
        var entities = Entities.findEntities({ x: 0, y: 0, z: 0 }, searchRadius);
        var matchedEntities = [];
        var keys = Object.keys(properties);
        for (var i = 0; i < entities.length; ++i) {
            var match = true;
            var candidateProperties = Entities.getEntityProperties(entities[i], keys);
            for (var key in properties) {
                if (!filterFn(properties, key, candidateProperties[key])) {
                    // This isn't a match, move to next entity
                    match = false;
                    break;
                }
            }
            if (match) {
                matchedEntities.push(entities[i]);
            }
        }

        return matchedEntities;
    }

    function getChildProperties(entityID, propertyNames) {
        var childEntityIDs = Entities.getChildrenIDs(entityID);
        var results = {}
        for (var i = 0; i < childEntityIDs.length; ++i) {
            var childEntityID = childEntityIDs[i];
            var properties = Entities.getEntityProperties(childEntityID, propertyNames);
            results[childEntityID] = properties;
        }
        return results;
    }

    var Fuse = function() {
    };
    Fuse.prototype = {
        onLit: function() {
            print("LIT", this.entityID);
            var fuseID = findEntity({ name: "tutorial/equip/fuse" }, 20); 
            Entities.callEntityMethod(fuseID, "light");
        },
        preload: function(entityID) {
            this.entityID = entityID;
        },
    };
    return new Fuse();
});
