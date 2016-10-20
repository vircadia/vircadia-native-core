// webSpawnTool.js
//
// Stress tests the rendering of web surfaces over time
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

ENTITY_SPAWNER = function (properties) {
    properties = properties || {};
    var RADIUS = properties.radius || 5.0;   // Spawn within this radius (square)
    var TEST_ENTITY_NAME = properties.entityName || "WebEntitySpawnTest";
    var NUM_ENTITIES = properties.count || 10000; // number of entities to spawn
    var ENTITY_SPAWN_LIMIT = properties.spawnLimit || 1;
    var ENTITY_SPAWN_INTERVAL = properties.spawnInterval || properties.interval || 2;
    var ENTITY_LIFETIME = properties.lifetime || 10;   // Entity timeout (when/if we crash, we need the entities to delete themselves)

    function makeEntity(properties) {
        var entity = Entities.addEntity(properties);
        return {
            destroy: function () {
                Entities.deleteEntity(entity)
            },
            getAge: function () {
                return Entities.getEntityProperties(entity).age;
            }
        };
    }

    function randomPositionXZ(center, radius) {
        return {
            x: center.x + (Math.random() * radius * 2.0) - radius,
            y: center.y,
            z: center.z + (Math.random() * radius * 2.0) - radius
        };
    }
    
    var entities = [];
    var entitiesToCreate = 0;
    var entitiesSpawned = 0;
    var spawnTimer = 0.0;
    var keepAliveTimer = 0.0;

    function clear () {
        var ids = Entities.findEntities(MyAvatar.position, 50);
        var that = this;
        ids.forEach(function(id) {
            var properties = Entities.getEntityProperties(id);
            if (properties.name == TEST_ENTITY_NAME) {
                Entities.deleteEntity(id);
            }
        }, this);
    }    
    
    function createEntities () {
        entitiesToCreate = NUM_ENTITIES;
        Script.update.connect(spawnEntities);
    }

    function spawnEntities (dt) {
        if (entitiesToCreate <= 0) {
            Script.update.disconnect(spawnEntities);
            print("Finished spawning entities");
        } 
        else if ((spawnTimer -= dt) < 0.0){
            spawnTimer = ENTITY_SPAWN_INTERVAL;

            var n = Math.min(entitiesToCreate, ENTITY_SPAWN_LIMIT);
            print("Spawning " + n + " entities (" + (entitiesSpawned += n) + ")");

            entitiesToCreate -= n;

            var center = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: RADIUS * -1.5 }));
            for (; n > 0; --n) {
                entities.push(makeEntity({
                    type: "Web",
                    sourceUrl: "https://www.reddit.com/r/random/",
                    name: TEST_ENTITY_NAME,
                    position: randomPositionXZ(center, RADIUS),
                    rotation: MyAvatar.orientation,
                    dimensions: { x: .8 + Math.random() * 0.8, y: 0.45 + Math.random() * 0.45, z: 0.01 },
                    lifetime: ENTITY_LIFETIME
                }));
            }
        }
    }

    function despawnEntities () {
        print("despawning entities");
        entities.forEach(function (entity) {
            entity.destroy();
        });
        entities = [];
    }

    function init () {
        Script.update.disconnect(init);
        clear();
        createEntities();
        Script.scriptEnding.connect(despawnEntities);
    }
    Script.update.connect(init);
};

ENTITY_SPAWNER();
