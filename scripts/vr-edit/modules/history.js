//
//  history.js
//
//  Created by David Rowe on 12 Sep 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global History */

History = (function () {
    // Provides undo facility.
    // Global object.

    "use strict";

    var history = [
            /*
            {
                undoData: {
                    setProperties: [
                        {
                            entityID: <entity ID>,
                            properties: { <name>: <value>, <name>: <value>, ... }
                        }
                    ],
                    createEntities: [
                        {
                            entityID: <entity ID>,
                            properties: { <name>: <value>, <name>: <value>, ... }
                        }
                    ],
                    deleteEntities: [
                        {
                            entityID: <entity ID>,
                            properties: { <name>: <value>, <name>: <value>, ... }
                        }
                    ]
                },
                redoData: {
                    ""
                }
            }
            */
        ],
        MAX_HISTORY_ITEMS = 100,
        undoPosition = -1;  // The next history item to undo; the next history item to redo = undoIndex + 1.

    function push(undoData, redoData) {
        // Wipe any redo history after current undo position.
        if (undoPosition < history.length - 1) {
            history.splice(undoPosition + 1, history.length - undoPosition - 1);
        }

        // Limit the number of history items.
        if (history.length >= MAX_HISTORY_ITEMS) {
            history.splice(0, history.length - MAX_HISTORY_ITEMS + 1);
        }

        history.push({ undoData: undoData, redoData: redoData });
        undoPosition += 1;
    }

    function hasUndo() {
        return undoPosition > -1;
    }

    function hasRedo() {
        return undoPosition < history.length - 1;
    }

    function undo() {
        if (undoPosition > -1) {

            // TODO

            undoPosition -= 1;
        }
    }

    function redo() {
        if (undoPosition < history.length - 1) {

            // TODO

            undoPosition += 1;
        }
    }

    return {
        push: push,
        hasUndo: hasUndo,
        hasRedo: hasRedo,
        undo: undo,
        redo: redo
    };
}());
