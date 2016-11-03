// entitySpawnerAC
//
//  Created by James B. Pollack @imgntn on 1/29/2016
//
//  This script shows how to use an AC to create entities, and delete and recreate those entities if the AC gets restarted.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var basePosition = {
    x: 0,
    y: 0,
    z: 0
};

var NUMBER_OF_BOXES = 4;
Agent.isAvatar = true;

function makeBoxes() {
    var i;
    for (i = 0; i < NUMBER_OF_BOXES; i++) {
        createBox();
    }
    Script.clearInterval(octreeQueryInterval);
}

function createBox() {
    var boxProps = {
        dimensions: {
            x: 1,
            y: 1,
            z: 1
        },
        color: {
            red: 0,
            green: 255,
            blue: 0
        },
        type: 'Box',
        name: 'TestBox',
        position: {
            x: basePosition.x + Math.random() * 5,
            y: basePosition.y + Math.random() * 5,
            z: basePosition.z + Math.random() * 5
        }
    }
    Entities.addEntity(boxProps)
}

var secondaryInit = false;

function deleteBoxes() {
    if (secondaryInit === true) {
        return;
    }

    if (EntityViewer.getOctreeElementsCount() <= 1) {
        Script.setTimeout(function() {
            deleteBoxes();
        }, 1000)
        return;
    }

    var results = Entities.findEntities(basePosition, 2000);
    results.forEach(function(r) {
        var name = Entities.getEntityProperties(r, 'name').name;
        if (name === "TestBox") {
            Entities.deleteEntity(r);
        }

    })

    makeBoxes();
    secondaryInit = true;
}

var initialized = false;

function update(deltaTime) {
    if (!initialized) {
        if (Entities.serversExist() && Entities.canRez()) {
            Entities.setPacketsPerSecond(6000);
            deleteBoxes()
            initialized = true;
            Script.update.disconnect(update);
        }
        return;
    }
}


EntityViewer.setPosition({
    x: 0,
    y: 0,
    z: 0
});
EntityViewer.setCenterRadius(60000);
var octreeQueryInterval = Script.setInterval(function() {
    EntityViewer.queryOctree();
}, 1000);

Script.update.connect(update);
