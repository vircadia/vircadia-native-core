//
//  dynamics-tests-interface.js
//  scripts/developer/tests/dynamics/
//
//  Created by Seth Alves 2017-4-30
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


"use strict";
/* globals $, EventBridge */

var parameters = {
    "lifetime":"integer"
};


function getQueryArgByName(name, url) {
    if (!url) {
        url = window.location.href;
    }
    name = name.replace(/[\[\]]/g, "\\$&");
    var regex = new RegExp("[?&]" + name + "(=([^&#]*)|&|#|$)"),
        results = regex.exec(url);
    if (!results) return null;
    if (!results[2]) return '';
    return decodeURIComponent(results[2].replace(/\+/g, " "));
}


function addCommandParameters(params) {
    // copy from html elements into an associative-array which will get passed (as JSON) through the EventBridge
    for (var parameterName in parameters) {
        if (parameters.hasOwnProperty(parameterName)) {
            var parameterType = parameters[parameterName];
            var strVal = $("#" + parameterName).val();
            if (parameterType == "integer") {
                params[parameterName] = parseInt(strVal);
            } else if (parameterType == "float") {
                params[parameterName] = parseFloat(strVal);
            } else {
                params[parameterName] = strVal;
            }
        }
    }
    return params;
}


$(document).ready(function() {
    // hook all buttons to EventBridge
    $(":button").each(function(index) {
        $(this).click(function() {
            EventBridge.emitWebEvent(JSON.stringify(addCommandParameters({ "dynamics-tests-command": this.id })));
        });
    });

    // copy parameters from query-args into elements
    for (var parameterName in parameters) {
        if (parameters.hasOwnProperty(parameterName)) {
            var val = getQueryArgByName(parameterName);
            if (val) {
                var parameterType = parameters[parameterName];
                if (parameterType == "integer") {
                    val = parseInt(val);
                } else if (parameterType == "float") {
                    val = parseFloat(val);
                }
                $("#" + parameterName).val(val.toString());
            }
        }
    }
});
