//
//  settingsExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/22/14
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Menu object
//




print("mySetting: " + Settings.getValue("mySetting"));
Settings.setValue("mySetting", "spam");
print("mySetting: " + Settings.getValue("mySetting"));

Script.stop();