//
//  currentAPI.js
//  examples
//
//  Created by Clément Brisset on 5/30/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function(){
    var array = [];
    var mainKeys = Object.keys(this);
    var qml = Script.resourcesPath() + '/qml/CurrentAPI.qml';
    var needsUpdate = false;
    var updateTime = 20;
    var updateData = [];
    var deltaTime = 0;
    var maxUpdatingMethods = 20;
    var scriptPath = "";
    
    if (ScriptDiscoveryService.debugScriptUrl != "") {
        Script.include(ScriptDiscoveryService.debugScriptUrl);
    }
    
    var window = new OverlayWindow({
        title: 'API Debugger',
        source: qml,
        width: 500,
        height: 700
    });

    window.closed.connect(function () {
        Script.stop();
    });

    function addMainKeys(){
        var keys = Object.keys(this);
        for (var i = 0; i < keys.length; i++) {
            array.push({member:keys[i] , type: "class"});
        }
    }

    function memberHasValue(member, type) {
        if (type === "function()") {
            if (member.indexOf(".has") < 0 && member.indexOf(".is") < 0 && member.indexOf(".get") < 0) {
                return false;
            }
            if (member.indexOf("indow") < 0) {
                return true;
            }
        } else if (type === "boolean" || type === "string" || type === "number" || type === "user") {
            return true;
        }
        return false;
    }

    function listKeys(string, object) {
        if (string === "listKeys" || string === "array" || string === "buffer" || string === "i") {
            return;
        }
        
        if (typeof(object) !== "object" || object === null) {
            var type = typeof(object);
            if (type === "function") {
                chain = string.split("(");
                if (chain.length > 1) {
                    string = chain[0];
                    type = "function(" + chain[1];
                } else {
                    type = "function()";
                }
            }   
            var value = "";
            var hasValue = false; 
            if (memberHasValue(string, type)){
                var evalstring = type === "function()" ? string+"()" : string;
                try{
                    value = "" + eval(evalstring);
                    hasValue = true;
                } catch(e) {
                    value = "Error evaluating";
                }
            }
            array.push({member:string , type: type, value: value, hasValue: hasValue});
            return;
        }
        
        var keys = Object.keys(object);
        for (var i = 0; i < keys.length; ++i) {
            if (string === "") {
                listKeys(keys[i], object[keys[i]]);
            } else if (keys[i] !== "parent") {
                listKeys(string + "." + keys[i], object[keys[i]]);
            }
        }
    }

    function findMethods(addon, object, addkeys) {
        array = [];
        var string = addkeys ? "" : addon+".";
        listKeys(string, object);
        if (addkeys) {
            addMainKeys();
        }
        array.sort(function(a, b){
            if(a.member < b.member) return -1;
            if(a.member > b.member) return 1;
            return 0;
        });
    };

    findMethods("", this, true);
    window.sendToQml({type:"methods", data:array});

    window.fromQml.connect(function(message){
        if (message.type == "refreshValues") {
            updateData = message.data;
            updateValues();
        } else if (message.type == "startRefreshValues") {
            updateData = message.data;
            if (updateData.length > maxUpdatingMethods) {
                updateData = message.data;
                updateValues();
            } else {
                needsUpdate = true;
                deltaTime = updateTime;
            }
        } else if (message.type == "stopRefreshValues") {
            needsUpdate = false;
            deltaTime = 0;
        } else if (message.type == "evaluateMember") {
            var value = ""
            try {
                value = "" + eval(message.data.member);
            } catch(e) {
                value = "Error evaluating"
            }
            window.sendToQml({type:"evaluateMember", data:{value:value, index:message.data.index}});
        } else if (message.type == "selectScript") {
            scriptPath = Window.browse("Select script to debug", "*.js", "JS files(*.js)");
            if (scriptPath) {
                ScriptDiscoveryService.stopScript(Paths.defaultScripts + "/developer/utilities/tools/currentAPI.js", true);
            }
        }
    });

    function updateValues() {
        for (var i = 0; i < updateData.length; i++) {
            try {
                updateData[i].value = "" + eval(updateData[i].member);
            } catch(e) {
                updateData[i].value = "Error evaluating"
            }
        }
        window.sendToQml({type: "refreshValues", data: updateData});
    }

    Script.update.connect(function(){
        deltaTime++;
        if (deltaTime > updateTime) {
            deltaTime = 0;
            if (needsUpdate) {
                updateValues();
            }
        }
    });
    
    Script.scriptEnding.connect(function(){
        if (!scriptPath || scriptPath.length == 0) {
            ScriptDiscoveryService.debugScriptUrl = "";
        } else {
            ScriptDiscoveryService.debugScriptUrl = scriptPath;
        }
        console.log("done running");
        window.close();
    });
}());

