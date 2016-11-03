//
//  loadingAreaEntity.js
//
//  Created by James B. Pollack @imgntn on 1/7/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  This script shows how to hook up a model entity to your avatar to act as a doppelganger.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {

    var WEARABLES_MANAGER_SCRIPT = Script.resolvePath('wearablesManager.js');
    var DOPPELGANGER_SCRIPT = Script.resolvePath('doppelganger.js');
    var CREATE_TEST_WEARABLE_SCRIPT = Script.resolvePath('createTestWearable.js');
    this.preload = function(entityID) {
        print("preload(" + entityID + ")");
    };

    this.enterEntity = function(entityID) {
        print("enterEntity(" + entityID + ")");
        // Script.load(WEARABLES_MANAGER_SCRIPT);
        // Script.load(DOPPELGANGER_SCRIPT);
        // Script.load(CREATE_TEST_WEARABLE_SCRIPT);

    };

    this.leaveEntity = function(entityID) {
        print("leaveEntity(" + entityID + ")");
    };
    
})