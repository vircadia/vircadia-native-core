//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

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
    for (var key in b) {
        obj[key] = b[key];
    }
    for (var key in a) {
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

// TEMPLATES contains a dictionary of different named entity templates. An entity
// template is just a list of properties.
//
// @param name Name of the template to get
// @return {object} The matching template, or null if not found
function getTemplate(name) {
    for (var i = 0; i < TEMPLATES.length; ++i) {
        if (TEMPLATES[i].name == name) {
            return TEMPLATES[i];
        }
    }
    return null;
}

// Cleanup Shortbow template data
var scoreboardTemplate = getTemplate('SB.Scoreboard');
Vec3.print("Scoreboard:", scoreboardTemplate.position);
for (var i = 0; i < TEMPLATES.length; ++i) {
    if (TEMPLATES[i].name !== scoreboardTemplate.name) {
        var template = TEMPLATES[i];

        // Fixup position
        template.localPosition = Vec3.subtract(template.position, scoreboardTemplate.position);
        delete template.position;
    }

    // Fixup model url
    if (template.type == "Model") {
        var urlParts = template.modelURL.split("/");
        var filename = urlParts[urlParts.length - 1];
        var newURL = Script.resolvePath("assets/" + filename);
        print("Updated url", template.modelURL, "to", newURL);
        template.modelURL = newURL;
    }
}

var entityIDs = [];

var rootPosition = null;
var goalPosition = null;
var scoreboardID = null;
var buttonID = null;
var waveDisplayID = null;
var scoreDisplayID = null;
var highScoreDisplayID = null;
var livesDisplayID = null;
var platformID = null;
function createLocalGame() {
    rootPosition = utils.findSurfaceBelowPosition(MyAvatar.position);
    rootPosition.y += 6.11;

    scoreboardID = spawnTemplate("SB.Scoreboard", {
        position: rootPosition
    });
    entityIDs.push(scoreboardID);

    // Create start button
    buttonID = spawnTemplate("SB.StartButton", {
        parentID: scoreboardID,
        script: Script.resolvePath("startGameButtonClientEntity.js"),
        serverScripts: Script.resolvePath("startGameButtonServerEntity.js"),
        userData: JSON.stringify({
            grabbableKey: { 
                wantsTrigger: true
            }
        }),
    });
    entityIDs.push(buttonID);


    // Generate goal that the enemies try to get to
    goalPosition = Vec3.sum(rootPosition, { x: 0, y: -10, z: -20 });
    const BASES_HEIGHT = 16;
    const ROOF_HEIGHT = 0.2;

    goalPosition.y += BASES_HEIGHT - ROOF_HEIGHT;

    waveDisplayID = spawnTemplate("SB.DisplayWave", {
        parentID: scoreboardID,
        userData: JSON.stringify({
            displayType: "wave"
        }),
        serverScripts: Script.resolvePath("displayServerEntity.js")
    });
    entityIDs.push(waveDisplayID);

    scoreDisplayID = spawnTemplate("SB.DisplayScore", {
        parentID: scoreboardID,
        userData: JSON.stringify({
            displayType: "score"
        }),
        serverScripts: Script.resolvePath("displayServerEntity.js")
    });
    entityIDs.push(scoreDisplayID);

    livesDisplayID = spawnTemplate("SB.DisplayLives", {
        parentID: scoreboardID,
        userData: JSON.stringify({
            displayType: "lives"
        }),
        serverScripts: Script.resolvePath("displayServerEntity.js")
    });
    entityIDs.push(livesDisplayID);

    highScoreDisplayID = spawnTemplate("SB.DisplayHighScore", {
        parentID: scoreboardID,
        userData: JSON.stringify({
            displayType: "highscore"
        }),
        serverScripts: Script.resolvePath("displayServerEntity.js")
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
        userData: JSON.stringify({
            platformID: platformID,
            buttonID: buttonID,
            waveDisplayID: waveDisplayID,
            scoreDisplayID: scoreDisplayID,
            livesDisplayID: livesDisplayID,
            highScoreDisplayID: highScoreDisplayID,
        }),
        serverScripts: Script.resolvePath('shortbowServerEntity.js')
    });

    bowPositions = [];
    spawnPositions = [];
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

if (Script.isClientScript()) {
    createLocalGame();
    //var gameManager = new ShortbowGameManager(rootPosition, goalPositionFront, bowPositions, spawnPositions, scoreboardID, buttonID, waveDisplayID, scoreDisplayID, livesDisplayID, highScoreDisplayID);

    function cleanup() {
        for (var i = 0; i < entityIDs.length; ++i) {
            Entities.deleteEntity(entityIDs[i]);
        }
        gameManager.cleanup();
    }

    Script.scriptEnding.connect(cleanup);
}
