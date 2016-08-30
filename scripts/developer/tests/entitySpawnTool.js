// entityEditStressTest.js
//
// Created by Seiji Emery on 8/31/15
// Copyright 2015 High Fidelity, Inc.
//
// Stress tests the client + server-side entity trees by spawning huge numbers of entities in
// close proximity to your avatar and updating them continuously (ie. applying position edits), 
// with the intent of discovering crashes and other bugs related to the entity, scripting, 
// rendering, networking, and/or physics subsystems.
//
// This script was originally created to find + diagnose an a clientside crash caused by improper
// locking of the entity tree, but can be reused for other purposes.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

ENTITY_SPAWNER = function (properties) {
    properties = properties || {};
    var RADIUS = properties.radius || 5.0;   // Spawn within this radius (square)
    var Y_OFFSET = properties.yOffset || 1.5; // Spawn at an offset below the avatar
    var TEST_ENTITY_NAME = properties.entityName || "EntitySpawnTest";

    var NUM_ENTITIES = properties.count || 1000; // number of entities to spawn
    var ENTITY_SPAWN_LIMIT = properties.spawnLimit || 100;
    var ENTITY_SPAWN_INTERVAL = properties.spawnInterval || properties.interval || 1.0;

    var UPDATE_INTERVAL = properties.updateInterval || properties.interval || 0.1;  // Re-randomize the entity's position every x seconds / ms
    var ENTITY_LIFETIME = properties.lifetime || 30;   // Entity timeout (when/if we crash, we need the entities to delete themselves)
    var KEEPALIVE_INTERVAL = properties.keepAlive || 5; // Refreshes the timeout every X seconds
    var UPDATES = properties.updates || false
    var SHAPES = properties.shapes || ["Icosahedron", "Tetrahedron", "Cube", "Sphere" ];

    function makeEntity(properties) {
        var entity = Entities.addEntity(properties);
        // print("spawning entity: " + JSON.stringify(properties));

        return {
            update: function (properties) {
                Entities.editEntity(entity, properties);
            },
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
    
    function randomPosition(center, radius) {
        return {
            x: center.x + (Math.random() * radius * 2.0) - radius,
            y: center.y + (Math.random() * radius * 2.0) - radius,
            z: center.z + (Math.random() * radius * 2.0) - radius
        };
    }
    
    
    function randomColor() {
        return {
            red: Math.floor(Math.random() * 255),
            green: Math.floor(Math.random() * 255),
            blue: Math.floor(Math.random() * 255),
        };
    }
    
    function randomDimensions() {
        return {
            x: 0.1 + Math.random() * 0.5,
            y: 0.1 + Math.random() * 0.1,
            z: 0.1 + Math.random() * 0.5
        };
    }

    var entities = [];
    var entitiesToCreate = 0;
    var entitiesSpawned = 0;
    var spawnTimer = 0.0;
    var keepAliveTimer = 0.0;
    var updateTimer = 0.0;

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
        print("Creating " + NUM_ENTITIES + " entities (UPDATE_INTERVAL = " + UPDATE_INTERVAL + ", KEEPALIVE_INTERVAL = " + KEEPALIVE_INTERVAL + ")");
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

            var center = MyAvatar.position;
            center.y -= Y_OFFSET;

            for (; n > 0; --n) {
                entities.push(makeEntity({
                    type: "Shape",
                    shape: SHAPES[n % SHAPES.length],
                    name: TEST_ENTITY_NAME,
                    position: randomPosition(center, RADIUS),
                    color: randomColor(),
                    dimensions: randomDimensions(),
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

    // Runs the following entity updates:
    // a) refreshes the timeout interval every KEEPALIVE_INTERVAL seconds, and
    // b) re-randomizes its position every UPDATE_INTERVAL seconds.
    // This should be sufficient to crash the client until the entity tree bug is fixed (and thereafter if it shows up again).
    function updateEntities (dt) {
        var updateLifetime = ((keepAliveTimer -= dt) < 0.0) ? ((keepAliveTimer = KEEPALIVE_INTERVAL), true) : false;
        var updateProperties = ((updateTimer -= dt) < 0.0) ? ((updateTimer = UPDATE_INTERVAL), true) : false;

        if (updateLifetime || updateProperties) {
            var center = MyAvatar.position;
            center.y -= Y_OFFSET;

            entities.forEach((updateLifetime && updateProperties && function (entity) {
                entity.update({
                    lifetime: entity.getAge() + ENTITY_LIFETIME,
                    position: randomPosition(center, RADIUS)
                });
            }) || (updateLifetime && function (entity) {
                entity.update({
                    lifetime: entity.getAge() + ENTITY_LIFETIME
                });
            }) || (updateProperties && function (entity) {
                entity.update({
                    position: randomPosition(center, RADIUS)
                });
            }) || null, this);
        }
    }

    function init () {
        Script.update.disconnect(init);
        clear();
        createEntities();
        Script.update.connect(updateEntities);
        Script.scriptEnding.connect(despawnEntities);
    }
    
    Script.update.connect(init);
};