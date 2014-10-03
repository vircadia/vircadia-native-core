//
//  dataViewHelpers.js
//  examples/libraries
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

if (typeof DataView.prototype.indexOf !== "function") {
    DataView.prototype.indexOf = function (searchString, position) {
        var searchLength = searchString.length,
            byteArrayLength = this.byteLength,
            maxSearchIndex = byteArrayLength - searchLength,
            searchCharCodes = [],
            found,
            i,
            j;

        searchCharCodes[searchLength] = 0;
        for (j = 0; j < searchLength; j += 1) {
            searchCharCodes[j] = searchString.charCodeAt(j);
        }

        i = position;
        found = false;
        while (i < maxSearchIndex && !found) {
            j = 0;
            while (j < searchLength && this.getUint8(i + j) === searchCharCodes[j]) {
                j += 1;
            }
            found = (j === searchLength);
            i += 1;
        }

        return found ? i - 1 : -1;
    };
}

if (typeof DataView.prototype.string !== "function") {
    DataView.prototype.string = function (start, length) {
        var charCodes = [],
            end,
            i;

        if (start === undefined) {
            start = 0;
        }
        if (length === undefined) {
            length = this.length;
        }

        end = start + length;
        for (i = start; i < end; i += 1) {
            charCodes.push(this.getUint8(i));
        }

        return String.fromCharCode.apply(String, charCodes);
    };
}

