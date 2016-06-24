//
//  firstPersonHMD.js
//  system
//
//  Created by Zander Otavka on 6/24/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Automatically enter first person mode when entering HMD mode
HMD.displayModeChanged.connect(function(isHMDMode) {
    if (isHMDMode) {
        Camera.setModeString("first person");
    }
});
