'use strict';

//
//  onEscape.js
//
//  Created by Kalila L. on Feb 3 2021.
//  Copyright 2021 Vircadia contributors.
//
//  This script manages actions when the user triggers an "escape" key or action.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() { // BEGIN LOCAL_SCOPE
    
    function maybeEscapeKeyPressed (event) {
        if (event.isAutoRepeat) { // isAutoRepeat is true when held down (or when Windows feels like it)
            return;
        }

        if (event.text === 'ESC') {
            var CHANNEL_AWAY_ENABLE = 'Hifi-Away-Enable';
            Messages.sendMessage(CHANNEL_AWAY_ENABLE, 'toggle', true);
        }
    }

    Controller.keyPressEvent.connect(maybeEscapeKeyPressed);

    Script.scriptEnding.connect(function () {
        Controller.keyPressEvent.disconnect(maybeEscapeKeyPressed);
    });
    
}());
