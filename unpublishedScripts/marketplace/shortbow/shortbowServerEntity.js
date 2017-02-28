//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals TEMPLATES:true, SHORTBOW_ENTITIES, ShortbowGameManager */

(function() {
    Script.include('utils.js');
    Script.include('shortbow.js');
    Script.include('shortbowGameManager.js');

    TEMPLATES = SHORTBOW_ENTITIES.Entities;

    this.entityID = null;
    var gameManager = null;
    this.preload = function(entityID) {
        this.entityID = entityID;

        var bowPositions = [];
        var spawnPositions = [];
        for (var i = 0; i < TEMPLATES.length; ++i) {
            var template = TEMPLATES[i];
            if (template.name === "SB.BowSpawn") {
                bowPositions.push(template.localPosition);
            } else if (template.name === "SB.EnemySpawn") {
                spawnPositions.push(template.localPosition);
            }
        }

        gameManager = new ShortbowGameManager(this.entityID, bowPositions, spawnPositions);
    };
    this.unload = function() {
        if (gameManager) {
            gameManager.cleanup();
            gameManager = null;
        }
    };
});
