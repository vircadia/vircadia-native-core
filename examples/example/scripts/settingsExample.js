//
//  settingsExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 3/22/14
//  Copyright 2013 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Menu object
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

print("mySetting: " + Settings.getValue("mySetting"));
Settings.setValue("mySetting", "spam");
print("mySetting: " + Settings.getValue("mySetting"));

//Script.stop();