// webSpawnTool.js
//
// Stress tests the rendering of web surfaces over time
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

SPAWNER = function (properties) {
    properties = properties || {};
    var RADIUS = properties.radius || 5.0;   // Spawn within this radius (square)
    var SPAWN_COUNT = properties.count || 10000; // number of entities to spawn
    var SPAWN_LIMIT = properties.spawnLimit || 1;
    var SPAWN_INTERVAL = properties.spawnInterval || properties.interval || 2;
    var SPAWN_LIFETIME = properties.lifetime || 10;   // Entity timeout (when/if we crash, we need the entities to delete themselves)

    function randomPositionXZ(center, radius) {
        return {
            x: center.x + (Math.random() * radius * 2.0) - radius,
            y: center.y,
            z: center.z + (Math.random() * radius * 2.0) - radius
        };
    }

    function makeObject(properties) {
        
        var overlay = Overlays.addOverlay("web3d", {
            name: "Web",
            url: "https://www.reddit.com/r/random",
            localPosition: randomPositionXZ( { x: 0, y: 0, z: -1 }, 1),
            localRotation: Quat.angleAxis(180, Vec3.Y_AXIS),
            dimensions: {x: .8, y: .45, z: 0.1},
            color: { red: 255, green: 255, blue: 255 },
            alpha: 1.0,
            showKeyboardFocusHighlight: false,
            visible: true
        });
        
        var now = Date.now();

        return {
            destroy: function () {
                Overlays.deleteOverlay(overlay)
            },
            getAge: function () {
                return (Date.now() - now) / 1000.0;
            }
        };
    }

    
    var items = [];
    var toCreate = 0;
    var spawned = 0;
    var spawnTimer = 0.0;
    var keepAliveTimer = 0.0;

    function clear () {
    }    
    
    function create() {
        toCreate = SPAWN_COUNT;
        Script.update.connect(spawn);
    }

    function spawn(dt) {
        if (toCreate <= 0) {
            Script.update.disconnect(spawn);
            print("Finished spawning");
        } 
        else if ((spawnTimer -= dt) < 0.0){
            spawnTimer = SPAWN_INTERVAL;

            var n = Math.min(toCreate, SPAWN_LIMIT);
            print("Spawning " + n + " items (" + (spawned += n) + ")");

            toCreate -= n;
            for (; n > 0; --n) {
                items.push(makeObject());
            }
        }
    }

    function despawn() {
        print("despawning");
        items.forEach(function (item) {
            item.destroy();
        });
        item = [];
    }

    function init () {
        Script.update.disconnect(init);
        Script.scriptEnding.connect(despawn);
        clear();
        create();
    }
    
    Script.update.connect(init);
};

SPAWNER();
