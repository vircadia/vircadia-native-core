//
// prevent-add-of-entities-named-bob-example.js
// 
//
// Created by Brad Hefta-Gaub to use Entities on Jan. 25, 2018
// Copyright 2018 High Fidelity, Inc.
//
// This sample entity edit filter script will get all edits, adds, physcis, and deletes, but will only block
// deletes, and will pass through all others.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function filter(properties, type) {
    if (properties.name == "bob") {
        return false;
    }
    return properties;
}

filter.wantsToFilterAdd = true;  // do run on add
filter.wantsToFilterEdit = false;  // do not run on edit
filter.wantsToFilterPhysics = false;  // do not run on physics
filter.wantsToFilterDelete = false;  // do not run on delete
filter;