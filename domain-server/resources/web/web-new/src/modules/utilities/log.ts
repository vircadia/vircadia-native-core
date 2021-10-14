/*
//  log.js
//
//  Created by Kalila L. on May 10th, 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

/* eslint-disable */

const Log = (function () {
    enum types {
        OTHER = "[OTHER]",
        DOMAIN = "[DOMAIN]",
        METAVERSE = "[METAVERSE]"
    }

    enum levels {
        ERROR = "[ERROR]",
        DEBUG = "[DEBUG]",
        WARN = "[WARN]",
        INFO = "[INFO]"
    }

    function print (pType: types, pLevel: levels, pMsg: string): void {
        console.info(pType, pLevel, pMsg);
    }

    // Print out message if debugging
    function debug (pType: types, pMsg: string) {
        print(pType, levels.DEBUG, pMsg);
    }

    function error (pType: types, pMsg: string) {
        print(pType, levels.ERROR, pMsg);
    }

    function warn (pType: types, pMsg: string) {
        print(pType, levels.WARN, pMsg);
    }

    function info (pType: types, pMsg: string) {
        print(pType, levels.INFO, pMsg);
    }

    return {
        // Tables
        types,
        levels,
        // Functions
        print,
        debug,
        error,
        warn,
        info
    };
}());

export default Log;
