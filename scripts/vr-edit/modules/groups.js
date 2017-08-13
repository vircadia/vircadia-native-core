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

    var rootEntityIDs = [],
        selections = [],
        entitiesSelectedCount = 0;

    if (!this instanceof Groups) {
        return new Groups();
    }

    function add(selection) {
        rootEntityIDs.push(selection[0].id);
        selections.push(Object.clone(selection));
        entitiesSelectedCount += selection.length;
    }

    function remove(selection) {
        var index = rootEntityIDs.indexOf(selection[0].id);

        entitiesSelectedCount -= selections[index].length;
        rootEntityIDs.splice(index, 1);
        selections.splice(index, 1);
    }

    function toggle(selection) {
        if (selection.length === 0) {
            return;
        }

        if (rootEntityIDs.indexOf(selection[0].id) === -1) {
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

        for (i = 0, lengthI = rootEntityIDs.length; i < lengthI; i += 1) {
            if (excludes.indexOf(rootEntityIDs[i]) === -1) {
                for (j = 0, lengthJ = selections[i].length; j < lengthJ; j += 1) {
                    result.push(selections[i][j]);
                }
            }
        }

        return result;
    }

    function includes(rootEntityID) {
        return rootEntityIDs.indexOf(rootEntityID) !== -1;
    }

    function groupsCount() {
        return selections.length;
    }

    function entitiesCount() {
        return entitiesSelectedCount;
    }

    function group() {
        // Groups all selections into one.
        var rootID,
            i,
            count;

        // Make the first entity in the first group the root and link the first entities of all other groups to it.
        rootID = rootEntityIDs[0];
        for (i = 1, count = rootEntityIDs.length; i < count; i += 1) {
            Entities.editEntity(rootEntityIDs[i], {
                parentID: rootID
            });
        }

        // Update selection.
        rootEntityIDs.splice(1, rootEntityIDs.length - 1);
        for (i = 1, count = selections.length; i < count; i += 1) {
            selections[i][0].parentID = rootID;
            selections[0] = selections[0].concat(selections[i]);
        }
        selections.splice(1, selections.length - 1);
    }

    function ungroup() {
        // Ungroups the first and assumed to be only selection.
        // If the first entity in the selection has a mix of solo and group children then just the group children are unlinked,
        // otherwise all are unlinked.
        var rootID,
            childrenIDs = [],
            childrenIndexes = [],
            childrenIndexIsGroup = [],
            previousWasGroup,
            hasSoloChildren = false,
            hasGroupChildren = false,
            isUngroupAll,
            i,
            count;

        function updateGroupInformation() {
            var childrenIndexesLength = childrenIndexes.length;
            if (childrenIndexesLength > 1) {
                previousWasGroup = childrenIndexes[childrenIndexesLength - 2] < i - 1;
                childrenIndexIsGroup.push(previousWasGroup);
                if (previousWasGroup) {
                    hasGroupChildren = true;
                } else {
                    hasSoloChildren = true;
                }
            }
        }

        if (entitiesSelectedCount === 0) {
            App.log("ERROR: Groups: Nothing to ungroup!");
            return;
        }
        if (entitiesSelectedCount === 1) {
            App.log("ERROR: Groups: Cannot ungroup sole entity!");
            return;
        }

        // Compile information on immediate children.
        rootID = rootEntityIDs[0];
        for (i = 1, count = selections[0].length; i < count; i += 1) {
            if (selections[0][i].parentID === rootID) {
                childrenIDs.push(selections[0][i].id);
                childrenIndexes.push(i);
                updateGroupInformation();
            }
        }
        childrenIndexes.push(selections[0].length);  // Special extra item at end to aid updating selection.
        updateGroupInformation();

        // Unlink children.
        isUngroupAll = hasSoloChildren !== hasGroupChildren;
        for (i = childrenIDs.length - 1; i >= 0; i -= 1) {
            if (isUngroupAll || childrenIndexIsGroup[i]) {
                Entities.editEntity(childrenIDs[i], {
                    parentID: Uuid.NULL
                });
                rootEntityIDs.push(childrenIDs[i]);
                selections[0][childrenIndexes[i]].parentID = Uuid.NULL;
                selections.push(selections[0].splice(childrenIndexes[i], childrenIndexes[i + 1] - childrenIndexes[i]));
            }
        }
    }

    function clear() {
        rootEntityIDs = [];
        selections = [];
        entitiesSelectedCount = 0;
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
