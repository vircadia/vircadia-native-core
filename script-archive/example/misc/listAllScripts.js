//
//  listAllScripts.js
//  examples/example/misc
//
//  Created by Thijs Wenker on 7 Apr 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Outputs the running, public and local scripts to the console.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var runningScripts = ScriptDiscoveryService.getRunning();
print("Running Scripts:");
for (var i = 0; i < runningScripts.length; i++) {
    print(" - " + runningScripts[i].name + (runningScripts[i].local ? "[Local]" : "") + " {" + runningScripts[i].url + "}");
}

var localScripts = ScriptDiscoveryService.getLocal();
print("Local Scripts:");
for (var i = 0; i < localScripts.length; i++) {
    print(" - " + localScripts[i].name + " {" + localScripts[i].path + "}");
}

// recursive function to walk through all folders in public scripts
// adding 2 spaces to the prefix per depth level
function displayPublicScriptFolder(nodes, prefix) {
    for (var i = 0; i < nodes.length; i++) {
        if (nodes[i].type == "folder") {
            print(prefix + "<" + nodes[i].name + ">");
            displayPublicScriptFolder(nodes[i].children, "  " + prefix);
            continue;
        }
        print(prefix + nodes[i].name + " {" + nodes[i].url + "}");
    }
}

var publicScripts = ScriptDiscoveryService.getPublic();
print("Public Scripts:");
displayPublicScriptFolder(publicScripts, " - ");