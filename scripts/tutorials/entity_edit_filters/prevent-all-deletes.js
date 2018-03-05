//
// prevent-all-deletes.js
// 
//
// Created by Brad Hefta-Gaub to use Entities on Jan. 25, 2018
// Copyright 2018 High Fidelity, Inc.
//
// This sample entity edit filter script will prevent deletes of any entities.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function filter() {    
    return false; // all deletes are blocked
}

filter.wantsToFilterAdd = false; // don't run on adds
filter.wantsToFilterEdit = false; // don't run on edits
filter.wantsToFilterPhysics = false;  // don't run on physics
filter.wantsToFilterDelete = true;  // do run on deletes
filter;