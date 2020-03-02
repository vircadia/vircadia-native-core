"use strict";

//  app-more.js
//  VERSION 1.0
//
//  Created by Keb Helion, February 2020.
//  Copyright 2020 Project Athena and contributors.
//
//  App maintained in: https://github.com/kasenvr/community-apps
//  App copied to: https://github.com/kasenvr/project-athena
//
//  This script adds a "More Apps" selector to "Project Athena" to allow the user to add optional functionalities to the tablet.
//  This application has been designed to work directly from the Github repository.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//   
 
(function() {
    var ROOT = Script.resolvePath('').split("app-more.js")[0];
    var APP_NAME = "MORE...";
    var APP_URL = ROOT + "more.html";
    var APP_ICON_INACTIVE = ROOT + "appicon_i.png";
    var APP_ICON_ACTIVE = ROOT + "appicon_a.png";
    var Appstatus = false;
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
        if (Appstatus) {
            tablet.webEventReceived.disconnect(onMoreAppWebEventReceived);
            tablet.gotoHomeScreen();
            Appstatus = false;
        } else {
            tablet.gotoWebScreen(APP_URL);            
            tablet.webEventReceived.connect(onMoreAppWebEventReceived);        
            Appstatus = true;
        }
            
        button.editProperties({
            isActive: Appstatus
        });

    }
    
    button.clicked.connect(clicked);

    function sendRunningScriptList() {
        var currentlyRunningScripts = ScriptDiscoveryService.getRunning();
        tablet.emitScriptEvent(JSON.stringify(currentlyRunningScripts));
    }

    function onMoreAppWebEventReceived(eventz) {
        
        if (typeof eventz === "string") {
            eventzget = JSON.parse(eventz);
            
            //print("EVENT ACTION: " + eventzget.action);
            //print("EVENT SCRIPT: " + eventzget.script);
            
            if (eventzget.action === "installScript") {
                
                if (lastProcessing.action === eventzget.action && lastProcessing.script === eventzget.script) {
                    return;
                } else {
                    ScriptDiscoveryService.loadOneScript(eventzget.script);
                
                    lastProcessing.action = eventzget.action;
                    lastProcessing.script = eventzget.script;
                    
                    Script.setTimeout(function() {
                        sendRunningScriptList(); 
                    }, 2000);
                }
            }

            if (eventzget.action === "uninstallScript") {
                
                if (lastProcessing.action === eventzget.action && lastProcessing.script === eventzget.script) {
                    return;
                } else {
                    ScriptDiscoveryService.stopScript(eventzget.script, false);
                    
                    lastProcessing.action = eventzget.action;
                    lastProcessing.script = eventzget.script;
                    
                    Script.setTimeout(function() {
                        sendRunningScriptList(); 
                    }, 2000);
                }    
            }            

            if (eventzget.action === "requestRunningScriptData") {
                sendRunningScriptList();
            }    

        }
        
    }


    function onScreenChanged(type, url) {
        if (type === "Web" && url.indexOf(APP_URL) !== -1) {
            //Active
            //print("MORE... ACTIVE");
            Appstatus = true;
        } else {
            //Inactive
            //print("MORE... INACTIVE");
            Appstatus = false;
        }
        
        button.editProperties({
            isActive: Appstatus
        });
    }
    
    
    function cleanup() {
        if (Appstatus) {
            tablet.gotoHomeScreen();
            tablet.webEventReceived.disconnect(onMoreAppWebEventReceived);
        }
        tablet.screenChanged.disconnect(onScreenChanged);
        tablet.removeButton(button);
    }
    
    Script.scriptEnding.connect(cleanup);
}());