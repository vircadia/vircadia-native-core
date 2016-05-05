//
// #20628:  JS Stream Player Domain-Zone-Entity
// ********************************************
//
// Created by Kevin M. Thomas and Thoys 07/20/15.
// Copyright 2015 High Fidelity, Inc.
// kevintown.net
//
// JavaScript for the High Fidelity interface that is an entity script to be placed in a chosen entity inside a domain-zone.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// Function which exists inside of an entity which triggers as a user approches it.
(function() { 
     const SCRIPT_NAME = "https://dl.dropboxusercontent.com/u/17344741/jsstreamplayer/jsstreamplayerdomain-zone.js";
     function isScriptRunning(script) {
        script = script.toLowerCase().trim();
        var runningScripts = ScriptDiscoveryService.getRunning();
        for (i in runningScripts) {
            if (runningScripts[i].url.toLowerCase().trim() == script) {
                return true;
            }
        }
        return false;
    };
    
    if (!isScriptRunning(SCRIPT_NAME)) {
        ScriptDiscoveryService.loadOneScript(SCRIPT_NAME);
    } 
})