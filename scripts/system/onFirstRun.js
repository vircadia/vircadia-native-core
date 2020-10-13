'use strict';

//
//  onFirstRun.js
//
//  Created by Kalila L. on Oct 5 2020.
//  Copyright 2020 Vircadia contributors.
//
//  This script triggers on first run to perform bootstrapping actions.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    var SETTING_TO_CHECK = 'firstRun';
    var DEFAULT_DISPLAY_NAME = '';

    if (!Settings.getValue(SETTING_TO_CHECK, false)) {
        return;
    }
    
    if (MyAvatar.displayName === '') {
        var selectedDisplayName = Window.prompt('Enter a display name.', MyAvatar.displayName);

        if (selectedDisplayName === '') {
            MyAvatar.displayName = DEFAULT_NAME;
        } else {
            MyAvatar.displayName = selectedDisplayName;
        }
    }
}());
