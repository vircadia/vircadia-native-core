//
// prevent-delete-in-zone-example.js
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

    if (type == Entities.DELETE_FILTER_TYPE) {
        return false;
    }
    return properties;
}

filter.wantsToFilterDelete = true;  // do run on deletes
filter;