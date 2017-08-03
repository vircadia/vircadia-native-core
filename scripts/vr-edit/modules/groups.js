//
//  groups.js
//
//  Created by David Rowe on 1 Aug 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Groups */

Groups = function () {
    // Groups and ungroups trees of entities.

    "use strict";

    var groupRootEntityIDs = [],
        groupSelectionDetails = [],
        numberOfEntitiesSelected = 0;

    if (!this instanceof Groups) {
        return new Groups();
    }

    function add(selection) {
        groupRootEntityIDs.push(selection[0].id);
        groupSelectionDetails.push(Object.clone(selection));
        numberOfEntitiesSelected += selection.length;
    }

    function remove(selection) {
        var index = groupRootEntityIDs.indexOf(selection[0].id);

        numberOfEntitiesSelected -= groupSelectionDetails[index].length;
        groupRootEntityIDs.splice(index, 1);
        groupSelectionDetails.splice(index, 1);
    }

    function toggle(selection) {
        if (selection.length === 0) {
            return;
        }

        if (groupRootEntityIDs.indexOf(selection[0].id) === -1) {
            add(selection);
        } else {
            remove(selection);
        }
    }

    function selection(excludes) {
        var result = [],
            i,
            lengthI,
            j,
            lengthJ;

        for (i = 0, lengthI = groupRootEntityIDs.length; i < lengthI; i += 1) {
            if (excludes.indexOf(groupRootEntityIDs[i]) === -1) {
                for (j = 0, lengthJ = groupSelectionDetails[i].length; j < lengthJ; j += 1) {
                    result.push(groupSelectionDetails[i][j]);
                }
            }
        }

        return result;
    }

    function includes(rootEntityID) {
        return groupRootEntityIDs.indexOf(rootEntityID) !== -1;
    }

    function groupsCount() {
        return groupSelectionDetails.length;
    }

    function entitiesCount() {
        return numberOfEntitiesSelected;
    }

    function group() {
        // TODO
    }

    function ungroup() {
        // TODO
    }

    function clear() {
        groupRootEntityIDs = [];
        groupSelectionDetails = [];
    }

    function destroy() {
        clear();
    }

    return {
        toggle: toggle,
        selection: selection,
        includes: includes,
        groupsCount: groupsCount,
        entitiesCount: entitiesCount,
        group: group,
        ungroup: ungroup,
        clear: clear,
        destroy: destroy
    };
};

Groups.prototype = {};
