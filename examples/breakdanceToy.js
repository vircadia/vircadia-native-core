//
//  breakdanceToy.js
//  examples/toys
//
//  This is an local script version of the breakdance game
//
//  Created by Brad Hefta-Gaub on Sept 3, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/utils.js");
Script.include("breakdanceCore.js");
breakdanceStart();
Script.update.connect(breakdanceUpdate);
Script.scriptEnding.connect(breakdanceEnd);
