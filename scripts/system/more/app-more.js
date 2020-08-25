"use strict";

//  app-more.js
//  VERSION 1.0
//
//  Created by Keb Helion, February 2020.
//  Copyright 2020 Vircadia contributors.
//
//  This script adds a "More Apps" selector to Vircadia to allow the user to add optional functionalities to the tablet.
//  This application has been designed to work directly from the Github repository.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//    
(function() {
    var ROOT = Script.resolvePath('').split("app-more.js")[0];
    var DEV_PARAMETER = Script.resolvePath('').split("?")[1];
    var APP_NAME = "MORE...";
    var APP_URL = (ROOT + "more.html" + (DEV_PARAMETER === "dev" ? "?dev" : "")).replace(/%5C/g, "/");
    var APP_ICON_INACTIVE = ROOT + "appicon_i.png";
    var APP_ICON_ACTIVE = ROOT + "appicon_a.png";
    var appStatus = false;
    var lastProcessing = {
            "action": "",
            "script": ""
        };
    
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    tablet.screenChanged.connect(onScreenChanged);
    var button = tablet.addButton({
        text: APP_NAME,
        icon: APP_ICON_INACTIVE,
        activeIcon: APP_ICON_ACTIVE
    });
    
    function clicked() {
        if (appStatus) {
            tablet.webEventReceived.disconnect(onMoreAppWebEventReceived);
            tablet.gotoHomeScreen();
            appStatus = false;
        } else {
            tablet.gotoWebScreen(APP_URL);
            tablet.webEventReceived.connect(onMoreAppWebEventReceived);
            appStatus = true;
        }
        button.editProperties({
            isActive: appStatus
        });
    }
    
    button.clicked.connect(clicked);

    function sendRunningScriptList() {
        var currentlyRunningScripts = ScriptDiscoveryService.getRunning();
        var newMessage = "RSL4MOREAPP:";
        var runningScriptJson;
        for (var j = 0; j < currentlyRunningScripts.length; j++) {
            runningScriptJson = currentlyRunningScripts[j].url;
            if (runningScriptJson.indexOf("https://cdn.vircadia.com/community-apps/applications") !== -1) {
                newMessage += "_" + runningScriptJson;
            }
        }
        tablet.emitScriptEvent(newMessage);
    }

    function onMoreAppWebEventReceived(message) {       
        if (typeof message === "string") {
            var instruction = JSON.parse(message);

            if (instruction.action === "installScript") {
                if (lastProcessing.action !== instruction.action || lastProcessing.script !== instruction.script) {
                    ScriptDiscoveryService.loadScript(instruction.script, true, false, false, true, false); // Force reload the script, do not use cache.
                    lastProcessing.action = instruction.action;
                    lastProcessing.script = instruction.script;
                    Script.setTimeout(function() {
                        sendRunningScriptList(); 
                    }, 1500);
                }
            }

            if (instruction.action === "uninstallScript") {
                if (lastProcessing.action !== instruction.action || lastProcessing.script !== instruction.script) {
                    ScriptDiscoveryService.stopScript(instruction.script, false);
                    lastProcessing.action = instruction.action;
                    lastProcessing.script = instruction.script;
                    Script.setTimeout(function() {
                        sendRunningScriptList(); 
                    }, 1500);
                }
            }

            if (instruction.action === "requestRunningScriptData") {
                sendRunningScriptList();
            }
        }
    }

    function onScreenChanged(type, url) {
        if (type === "Web" && url.indexOf(APP_URL) !== -1) {
            appStatus = true;
        } else {
            appStatus = false;
        }       
        button.editProperties({
            isActive: appStatus
        });
    }

    function cleanup() {
        if (appStatus) {
            tablet.gotoHomeScreen();
            tablet.webEventReceived.disconnect(onMoreAppWebEventReceived);
        }
        tablet.screenChanged.disconnect(onScreenChanged);
        tablet.removeButton(button);
    }

    Script.scriptEnding.connect(cleanup);
}());
