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
        var rootID,
            i,
            count;

        // Make the first entity in the first group the root and link the first entities of all other groups to it.
        rootID = groupRootEntityIDs[0];
        for (i = 1, count = groupRootEntityIDs.length; i < count; i += 1) {
            Entities.editEntity(groupRootEntityIDs[i], {
                parentID: rootID
            });
        }

        // Update selection.
        groupRootEntityIDs.splice(1, groupRootEntityIDs.length - 1);
        for (i = 1, count = groupSelectionDetails.length; i < count; i += 1) {
            groupSelectionDetails[i][0].parentID = rootID;
            groupSelectionDetails[0] = groupSelectionDetails[0].concat(groupSelectionDetails[i]);
        }
        groupSelectionDetails.splice(1, groupSelectionDetails.length - 1);
    }

    function ungroup() {
        var rootID,
            childrenIDs,
            childrenIDIndexes,
            i,
            count,
            NULL_UUID = "{00000000-0000-0000-0000-000000000000}";

        // Compile information on children.
        rootID = groupRootEntityIDs[0];
        childrenIDs = [];
        childrenIDIndexes = [];
        for (i = 1, count = groupSelectionDetails[0].length; i < count; i += 1) {
            if (groupSelectionDetails[0][i].parentID === rootID) {
                childrenIDs.push(groupSelectionDetails[0][i].id);
                childrenIDIndexes.push(i);
            }
        }
        childrenIDIndexes.push(groupSelectionDetails[0].length);  // Extra item at end to aid updating selection.

        // Unlink direct children from root entity.
        for (i = 0, count = childrenIDs.length; i < count; i += 1) {
            Entities.editEntity(childrenIDs[i], {
                parentID: NULL_UUID
            });
        }

        // Update selection.
        groupRootEntityIDs = groupRootEntityIDs.concat(childrenIDs);
        for (i = 0, count = childrenIDs.length; i < count; i += 1) {
            groupSelectionDetails.push(groupSelectionDetails[0].slice(childrenIDIndexes[i], childrenIDIndexes[i + 1]));
            groupSelectionDetails[i + 1][0].parentID = NULL_UUID;
        }
        groupSelectionDetails[0].splice(1, groupSelectionDetails[0].length - childrenIDIndexes[0]);
    }

    function clear() {
        groupRootEntityIDs = [];
        groupSelectionDetails = [];
        numberOfEntitiesSelected = 0;
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
