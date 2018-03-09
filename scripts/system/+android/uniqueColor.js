"use strict";
//
//  uniqueColor.js
//  scripts/system/
//
//  Created by Gabriel Calero & Cristian Duarte on 17 Oct 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var colorsMap = {};
var colorsCount = 0;
                            // 'red',  'orange',  'yellow',  'green',    'cyan',    'blue',   'magenta'
var baseColors = [ '#EB3345', '#F0851F', '#FFCD29', '#94C338', '#11A6C5', '#294C9F', '#C01D84' ];

function getNextColor(n) {
    var N = baseColors.length;
    /*if (n < baseColors.length) {
        return baseColors[n];
    } else {
        var baseColor = baseColors[n % N];
        var d = (n / N) % 10;
        var c2 = "" + Qt.lighter(baseColor, 1 + d / 10);
        return c2;
    }*/
    return baseColors[n%N];
}

function getColorForId(uuid) {
    if (colorsMap == undefined) {
        colorsMap = {};
    }
    if (!colorsMap.hasOwnProperty(uuid)) {
        colorsMap[uuid] = getNextColor(colorsCount);
        colorsCount = colorsCount + 1;
    }
    return colorsMap[uuid];
}

module.exports = {
    getColor: function(id) {
        return getColorForId(id);
    },
    convertHexToRGB: function(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
        return result ? {
            red: parseInt(result[1], 16),
            green: parseInt(result[2], 16),
            blue: parseInt(result[3], 16)
        } : null;
    }
};
