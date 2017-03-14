//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals utils, SHORTBOW_ENTITIES, TEMPLATES:true */

Script.include('utils.js');
Script.include('shortbow.js');
Script.include('shortbowGameManager.js');
TEMPLATES = SHORTBOW_ENTITIES.Entities;

// Merge two objects into a new object. If a key name appears in both a and b,
// the value in a will be used.
//
// @param {object} a
// @param {object} b
// @returns {object} The new object
function mergeObjects(a, b) {
    var obj = {};
    var key;
    for (key in b) {
        obj[key] = b[key];
    }
    for (key in a) {
        obj[key] = a[key];
    }
    return obj;
}

// Spawn an entity from a template.
//
// The overrides can be used to override or add properties in the template. For instance,
// it's common to override the `position` property so that you can set the position
// of the entity to be spawned.
//
// @param {string} templateName The name of the template to spawn
// @param {object} overrides An object containing properties that will override
//                           any properties set in the template.
function spawnTemplate(templateName, overrides) {
    var template = getTemplate(templateName);
    if (template === null) {
        print("ERROR, unknown template name:", templateName);
        return null;
    }
    print("Spawning: ", templateName);
    var properties = mergeObjects(overrides, template);
    return Entities.addEntity(properties);
}

function spawnTemplates(templateName, overrides) {
    var templates = getTemplates(templateName);
    if (template.length === 0) {
        print("ERROR, unknown template name:", templateName);
        return [];
    }

    var spawnedEntities = [];
    for (var i = 0; i < templates.length; ++i) {
        print("Spawning: ", templateName);
        var properties = mergeObjects(overrides, templates[i]);
        spawnedEntities.push(Entities.addEntity(properties));
    }
    return spawnedEntities;
}

// TEMPLATES contains a dictionary of different named entity templates. An entity
// template is just a list of properties.
//
// @param name Name of the template to get
// @return {object} The matching template, or null if not found
function getTemplate(name) {
    for (var i = 0; i < TEMPLATES.length; ++i) {
        if (TEMPLATES[i].name === name) {
            return TEMPLATES[i];
        }
    }
    return null;
}
function getTemplates(name) {
    var templates = [];
    for (var i = 0; i < TEMPLATES.length; ++i) {
        if (TEMPLATES[i].name === name) {
            templates.push(TEMPLATES[i]);
        }
    }
    return templates;
}


// Cleanup Shortbow template data
for (var i = 0; i < TEMPLATES.length; ++i) {
    var template = TEMPLATES[i];

    // Fixup model url
    if (template.type === "Model") {
        var urlParts = template.modelURL.split("/");
        var filename = urlParts[urlParts.length - 1];
        var newURL = Script.resolvePath("models/" + filename);
        print("Updated url", template.modelURL, "to", newURL);
        template.modelURL = newURL;
    }
}

var entityIDs = [];

var scoreboardID = null;
var buttonID = null;
var waveDisplayID = null;
var scoreDisplayID = null;
var highScoreDisplayID = null;
var livesDisplayID = null;
var platformID = null;
function createLocalGame() {
    var rootPosition = utils.findSurfaceBelowPosition(MyAvatar.position);
    rootPosition.y += 6.11;

    scoreboardID = spawnTemplate("SB.Scoreboard", {
        position: rootPosition
    });
    entityIDs.push(scoreboardID);

    // Create start button
    buttonID = spawnTemplate("SB.StartButton", {
        parentID: scoreboardID,
        script: Script.resolvePath("startGameButtonClientEntity.js"),
        userData: JSON.stringify({
            grabbableKey: {
                wantsTrigger: true
            }
        })
    });
    entityIDs.push(buttonID);


    waveDisplayID = spawnTemplate("SB.DisplayWave", {
        parentID: scoreboardID,
        userData: JSON.stringify({
            displayType: "wave"
        })
    });
    entityIDs.push(waveDisplayID);

    scoreDisplayID = spawnTemplate("SB.DisplayScore", {
        parentID: scoreboardID,
        userData: JSON.stringify({
            displayType: "score"
        })
    });
    entityIDs.push(scoreDisplayID);

    livesDisplayID = spawnTemplate("SB.DisplayLives", {
        parentID: scoreboardID,
        userData: JSON.stringify({
            displayType: "lives"
        })
    });
    entityIDs.push(livesDisplayID);

    highScoreDisplayID = spawnTemplate("SB.DisplayHighScore", {
        parentID: scoreboardID,
        userData: JSON.stringify({
            displayType: "highscore"
        })
    });
    entityIDs.push(highScoreDisplayID);

    platformID = spawnTemplate("SB.Platform", {
        parentID: scoreboardID
    });
    entityIDs.push(platformID);

    spawnTemplate("SB.GateCollider", {
        parentID: scoreboardID,
        visible: false
    });
    entityIDs.push(platformID);

    Entities.editEntity(scoreboardID, {
        serverScripts: Script.resolvePath('shortbowServerEntity.js')
    });

    spawnTemplates("SB.BowSpawn", {
        parentID: scoreboardID,
        visible: false
    });
    spawnTemplates("SB.EnemySpawn", {
        parentID: scoreboardID,
        visible: false
    });

    var bowPositions = [];
    var spawnPositions = [];
    for (var i = 0; i < TEMPLATES.length; ++i) {
        var template = TEMPLATES[i];

        if (template.name === "SB.BowSpawn") {
            bowPositions.push(Vec3.sum(rootPosition, template.localPosition));
            Vec3.print("Pushing bow position", Vec3.sum(rootPosition, template.localPosition));
        } else if (template.name === "SB.EnemySpawn") {
            spawnPositions.push(Vec3.sum(rootPosition, template.localPosition));
            Vec3.print("Pushing spawnposition", Vec3.sum(rootPosition, template.localPosition));
        }
    }
}

createLocalGame();
Script.stop();
