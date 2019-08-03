//
//  utils.js
//
//  Created by David Back on 19 Nov 2018
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function disableDragDrop() {
    document.addEventListener("drop", function(event) {
        event.preventDefault();
    });
    
    document.addEventListener("dragover", function(event) {
        event.dataTransfer.effectAllowed = "none";
        event.dataTransfer.dropEffect = "none";
        event.preventDefault();
    });
    
    document.addEventListener("dragenter", function(event) {
        event.dataTransfer.effectAllowed = "none";
        event.dataTransfer.dropEffect = "none";
        event.preventDefault();
    }, false);
}

// mergeDeep function from https://stackoverflow.com/a/34749873
/**
 * Simple object check.
 * @param item
 * @returns {boolean}
 */
function mergeDeepIsObject(item) {
    return (item && typeof item === 'object' && !Array.isArray(item));
}

/**
 * Deep merge two objects.
 * @param target
 * @param sources
 */
function mergeDeep(target, ...sources) {
    if (!sources.length) {
        return target;
    }
    const source = sources.shift();

    if (mergeDeepIsObject(target) && mergeDeepIsObject(source)) {
        for (const key in source) {
            if (!source.hasOwnProperty(key)) {
                continue;
            }
            if (mergeDeepIsObject(source[key])) {
                if (!target[key]) {
                    Object.assign(target, { [key]: {} });
                }
                mergeDeep(target[key], source[key]);
            } else {
                Object.assign(target, { [key]: source[key] });
            }
        }
    }

    return mergeDeep(target, ...sources);
}

function deepEqual(a, b) {
    if (a === b) {
        return true;
    }

    if (typeof(a) !== "object" || typeof(b) !== "object") {
        return false;
    }

    if (Object.keys(a).length !== Object.keys(b).length) {
        return false;
    }

    for (let property in a) {
        if (!a.hasOwnProperty(property)) {
            continue;
        }
        if (!b.hasOwnProperty(property)) {
            return false;
        }
        if (!deepEqual(a[property], b[property])) {
            return false;
        }
    }
    return true;
}
