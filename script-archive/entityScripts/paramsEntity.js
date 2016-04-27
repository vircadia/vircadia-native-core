//
//  paramsEntity.js
//
//  Script Type: Entity
//
//  Created by James B. Pollack @imgntn on 11/6/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script demonstrates how to recieve parameters from a Entities.callEntityMethod call
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function() {

    function ParamsEntity() {
        return;
    }

    ParamsEntity.prototype = {
        preload: function(entityID) {
            print('entity loaded')
            this.entityID = entityID;
        },
        testParams: function(myID, paramsArray) {

            paramsArray.forEach(function(param) {
                var p;
                try {
                    p = JSON.parse(param);
                    print("it's a json param")
                    print('json param property:' + p.thing);
                } catch (err) {
                    print('not a json param')
                    p = param;
                    print('param is:' + p);
                }

            });

        }

    }

    return new ParamsEntity();
});