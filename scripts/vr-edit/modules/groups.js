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
        groupSelectionDetails = [];

    if (!this instanceof Groups) {
        return new Groups();
    }

    function add(selection) {
        groupRootEntityIDs.push(selection[0].id);
        groupSelectionDetails.push(Object.clone(selection));
    }

    function remove(selection) {
        var index = groupRootEntityIDs.indexOf(selection[0].id);

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

    function getGroups() {
        return groupSelectionDetails;
    }

    function count() {
        return groupSelectionDetails.length;
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
        groups: getGroups,
        count: count,
        group: group,
        ungroup: ungroup,
        clear: clear,
        destroy: destroy
    };
};

Groups.prototype = {};
