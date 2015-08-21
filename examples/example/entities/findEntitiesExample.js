//
//  findEntityExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates "finding" entities
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var iteration = 0;

var entityA = Entities.addEntity(
                    {
                        type: "Sphere",
                        position: { x: 2, y: 0, z: 2 },
                        velocity: { x: 0, y: 0, z: 0 },
                        gravity: { x: 0, y: 0, z: 0 },
                        dimensions: { x: 0.1, y: 0.1, z: 0.1 },
                        color: { red: 0, green: 255, blue: 0 }
                    });

var entityB = Entities.addEntity(
                    {
                        type: "Sphere",
                        position: { x: 5, y: 0, z: 5 },
                        velocity: { x: 0, y: 0, z: 0 },
                        gravity: { x: 0, y: 0, z: 0 },
                        dimensions: { x: 0.1, y: 0.1, z: 0.1 },
                        color: { red: 0, green: 255, blue: 255 }
                    });

var searchAt = { x: 0, y: 0, z: 0};
var moveSearch = { x: 0.1, y: 0, z: 0.1};
var searchRadius = 1;
var searchRadiusChange = 0;

function scriptEnding() {
    print("calling Entities.deleteEntity()");
    Entities.deleteEntity(entityA);
    Entities.deleteEntity(entityB);
}

function printProperties(properties) {
    for (var property in properties) {
        if (properties.hasOwnProperty(property)) { 
            if (property == "position" || 
                property == "gravity" || 
                property == "velocity") {
                print(property +": " + properties[property].x + ", " + properties[property].y + ", " + properties[property].z);
            } else if (property == "color") {
                print(property +": " + properties[property].red + ", " 
                            + properties[property].green + ", " + properties[property].blue);
            } else {
                print(property +": " + properties[property])
            }
        }
    }
}

function findEntities(deltaTime) {

    // run for a while, then clean up
    // stop it...
    if (iteration >= 100) {
        print("calling Script.stop()");
        Script.stop();
    }

    print("--------------------------");
    print("iteration =" + iteration);
    iteration++;
    
    // also check to see if we can "find" entities...
    print("searching for entities at:" + searchAt.x + ", " + searchAt.y + ", " + searchAt.z + " radius:" + searchRadius);
    var foundEntities = Entities.findEntities(searchAt, searchRadius);
    print("found this many entities: "+ foundEntities.length);
    for (var i = 0; i < foundEntities.length; i++) {
        print("   foundEntities[" + i + "].id:" + foundEntities[i].id);
        if (foundEntities[i].id == entityA.id) {
            print(">>>> found entityA!!");
            var propertiesA = Entities.getEntityProperties(entityA);
            printProperties(propertiesA);
        }
        if (foundEntities[i].id == entityB.id) {
            print(">>>> found entityB!!");
            var propertiesB = Entities.getEntityProperties(entityB);
            printProperties(propertiesB);
        }
    }
    // move search
    searchAt.x += moveSearch.x;
    searchAt.y += moveSearch.y;
    searchAt.z += moveSearch.z;
    searchRadius += searchRadiusChange;

    // after we've searched for 80 iterations, change our search mechanism to be from the center with expanding radius    
    if (iteration == 80) {
        searchAt = { x: 3.5, y: 0, z: 3.5};
        moveSearch = { x: 0, y: 0, z: 0};
        searchRadius = 0.5;
        searchRadiusChange = 0.5;
    }
    
}


// register the call back so it fires before each data send
Script.update.connect(findEntities);

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);
