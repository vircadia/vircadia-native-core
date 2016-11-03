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

var NUM_ENTITIES = 20000;           // number of entities to spawn
var ENTITY_SPAWN_LIMIT = 1000;
var ENTITY_SPAWN_INTERVAL = 0.1;

var UPDATE_INTERVAL = 0.05;  // Re-randomize the entity's position every x seconds / ms
var ENTITY_LIFETIME = 30;   // Entity timeout (when/if we crash, we need the entities to delete themselves)
var KEEPALIVE_INTERVAL = 5; // Refreshes the timeout every X seconds 

var RADIUS = 5.0;   // Spawn within this radius (square)
var Y_OFFSET = 1.5; // Spawn at an offset below the avatar
var TEST_ENTITY_NAME = "EntitySpawnTest";
    
(function () {
    this.makeEntity = function (properties) {
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

    this.randomPositionXZ = function (center, radius) {
        return {
            x: center.x + (Math.random() * radius * 2.0) - radius,
            y: center.y,
            z: center.z + (Math.random() * radius * 2.0) - radius
        };
    }
    this.randomColor = function () {
        var shade = Math.floor(Math.random() * 255);
        var hue   = Math.floor(Math.random() * (255 - shade));

        return {
            red: shade + hue,
            green: shade,
            blue: shade
        };
    }
    this.randomDimensions = function () {
        return {
            x: 0.1 + Math.random() * 0.5,
            y: 0.1 + Math.random() * 0.1,
            z: 0.1 + Math.random() * 0.5
        };
    }
})();

(function () {
    var entities = [];
    var entitiesToCreate = 0;
    var entitiesSpawned = 0;


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

    var spawnTimer = 0.0;
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
                    type: "Box",
                    name: TEST_ENTITY_NAME,
                    position: randomPositionXZ(center, RADIUS),
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

    var keepAliveTimer = 0.0;
    var updateTimer = 0.0;

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
                    position: randomPositionXZ(center, RADIUS)
                });
            }) || (updateLifetime && function (entity) {
                entity.update({
                    lifetime: entity.getAge() + ENTITY_LIFETIME
                });
            }) || (updateProperties && function (entity) {
                entity.update({
                    position: randomPositionXZ(center, RADIUS)
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
})();