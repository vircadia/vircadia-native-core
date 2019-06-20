//
//  loadPerfTest.js
//  scripts/developer/tests/scriptableResource 
//
//  Created by Zach Pomerantz on 4/27/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Preloads 158 textures 50 times for performance profiling.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var TIMES = 50;

Script.include([
    '../../../developer/utilities/cache/cacheStats.js',
    'lib.js',
], function() {
    var fetch = function() {
        prefetch(function(frames) {
            while (frames.length) { frames.pop(); }
            Script.requestGarbageCollection();

            if (--TIMES > 0) {
                // Pause a bit to avoid a deadlock
                var DEADLOCK_AVOIDANCE_TIMEOUT = 100;
                Script.setTimeout(fetch, DEADLOCK_AVOIDANCE_TIMEOUT);
            }
        });
    };

    fetch();
});
