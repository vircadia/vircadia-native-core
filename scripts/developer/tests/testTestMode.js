//
// Created by Bradley Austin Davis on 2016/12/12
// Copyright 2013-2016 High Fidelity, Inc.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


Script.setTimeout(function() {
    var loggingRules = "" + 
"*.debug=false\n" +
"hifi.render.debug=true\n" +
"hifi.interface.debug=true\n" +
"";
    Test.startTracing(loggingRules);
}, 1 * 1000);


Script.setTimeout(function() {
    Test.stopTracing("h:/testScriptTrace.json.gz");
    Test.quit();
}, 10 * 1000);


