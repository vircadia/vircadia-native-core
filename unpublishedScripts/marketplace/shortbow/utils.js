//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals utils:true */

if (!Function.prototype.bind) {
    Function.prototype.bind = function(oThis) {
        if (typeof this !== 'function') {
            // closest thing possible to the ECMAScript 5
            // internal IsCallable function
            throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
        }

        var aArgs = Array.prototype.slice.call(arguments, 1),
            fToBind = this,
            NOP = function() {},
            fBound = function() {
                return fToBind.apply(this instanceof NOP
                        ? this
                        : oThis,
                        aArgs.concat(Array.prototype.slice.call(arguments)));
            };

        if (this.prototype) {
            // Function.prototype doesn't have a prototype property
            NOP.prototype = this.prototype;
        }
        fBound.prototype = new NOP();

        return fBound;
    };
}

utils = {
    parseJSON: function(json) {
        try {
            return JSON.parse(json);
        } catch (e) {
            return undefined;
        }
    },
    findSurfaceBelowPosition: function(pos) {
        var result = Entities.findRayIntersection({
            origin: pos,
            direction: { x: 0.0, y: -1.0, z: 0.0 }
        }, true);
        if (result.intersects) {
            return result.intersection;
        }
        return pos;
    }
};
