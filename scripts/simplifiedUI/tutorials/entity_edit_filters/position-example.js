//
// position-example.js
// 
//
// Created by Brad Hefta-Gaub to use Entities on Dec. 15, 2017
// Copyright 2017 High Fidelity, Inc.
//
// This sample entity edit filter script will only allow position to be changed by no more than 5 meters on any axis.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function filter(properties, type, originalProperties) {    

    /* Clamp position changes.*/
    var maxChange = 5;

    function clamp(val, min, max) {
        if (val > max) {
            val = max;
        } else if (val < min) {
            val = min;
        }
        return val;
    }


    if (properties.position) {
        /* Random near-zero value used as "zero" to prevent two sequential updates from being
        exactly the same (which would cause them to be ignored) */
        var nearZero = 0.0001 * Math.random() + 0.001;
        var maxFudgeChange = (maxChange + nearZero);
        properties.position.x = clamp(properties.position.x, originalProperties.position.x 
                                        - maxFudgeChange, originalProperties.position.x + maxFudgeChange);
        properties.position.y = clamp(properties.position.y, originalProperties.position.y 
                                        - maxFudgeChange, originalProperties.position.y + maxFudgeChange);
        properties.position.z = clamp(properties.position.z, originalProperties.position.z 
                                        - maxFudgeChange, originalProperties.position.z + maxFudgeChange);
    }

    return properties;
}
filter.wantsOriginalProperties = "position";
filter;