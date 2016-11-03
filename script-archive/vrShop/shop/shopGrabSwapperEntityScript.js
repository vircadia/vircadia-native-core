// shopGrapSwapperEntityScript.js
//
//  This script handle the transition from handControllerGrab to shopItemGrab
//  When an avatar enters the zone a message is sent to the handControllerGrab script to disable itself and the shopItemGrab is loaded.
//  When exit from the zone the handControllerGrab is re-enabled.
//  This mechanism may not work well with the last changes to the animations in handControllerGrab, if it's the case please load this script manually and disable that one.

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var SHOP_GRAB_SCRIPT_URL = Script.resolvePath("../item/shopItemGrab.js");
    var SHOP_GRAB_CHANNEL = "Hifi-vrShop-Grab";
    var _this;

    function SwapGrabZone() {
        _this = this;
        return;
    };
    
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
    
    SwapGrabZone.prototype = {
        
        enterEntity: function (entityID) {
            print("entering in the shop area");
            
            if (!isScriptRunning(SHOP_GRAB_SCRIPT_URL)) {
                Script.load(SHOP_GRAB_SCRIPT_URL);
            }
            
        },

        leaveEntity: function (entityID) {
            print("leaving the shop area");
            Messages.sendMessage(SHOP_GRAB_CHANNEL, null);      //signal to shopItemGrab that it has to kill itself
        }

    }

    return new SwapGrabZone();
});