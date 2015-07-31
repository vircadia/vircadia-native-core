//  utilities.js
//  examples
//
//  Created by Eric Levin on June 8
//  Copyright 2015 High Fidelity, Inc.
//
//  Common utilities
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html



map = function(value, min1, max1, min2, max2) {
    return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
}

hslToRgb = function(hslColor) {
    var h = hslColor.hue;
    var s = hslColor.sat;
    var l = hslColor.light;
    var r, g, b;

    if (s == 0) {
        r = g = b = l; // achromatic
    } else {
        var hue2rgb = function hue2rgb(p, q, t) {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1 / 6) return p + (q - p) * 6 * t;
            if (t < 1 / 2) return q;
            if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
            return p;
        }

        var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        var p = 2 * l - q;
        r = hue2rgb(p, q, h + 1 / 3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1 / 3);
    }

    return {
        red: Math.round(r * 255),
        green: Math.round(g * 255),
        blue: Math.round(b * 255)
    };

}



randFloat = function(low, high) {
    return low + Math.random() * (high - low);
}


randInt = function(low, high) {
    return Math.floor(randFloat(low, high));
}