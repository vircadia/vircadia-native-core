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

Script.include("./entitySpawnTool.js");


ENTITY_SPAWNER({
    updateInterval: 2.0
})

