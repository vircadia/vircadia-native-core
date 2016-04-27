//
//  breakdanceOmniToolModule.js
//  examples/libraries/omniTool/modules
//
//  This is an omniTool module version of the breakdance game
//
//  Created by Brad Hefta-Gaub on Sept 3, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../../breakdanceCore.js");

OmniToolModules.Breakdance = function() {
    print("OmniToolModules.Breakdance...");
}

OmniToolModules.Breakdance.prototype.onLoad = function(deltaTime) {
    print("OmniToolModules.Breakdance.prototype.onLoad()...");
    breakdanceStart();
 }

OmniToolModules.Breakdance.prototype.onUpdate = function(deltaTime) {
    print("OmniToolModules.Breakdance.prototype.onUpdate()...");
    breakdanceUpdate();
}

 OmniToolModules.Breakdance.prototype.onUnload = function() {
    print("OmniToolModules.Breakdance.prototype.onUnload()...");
    breakdanceEnd();
}

OmniToolModuleType = "Breakdance";
