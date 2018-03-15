//
// Sandbox/zoneItemBase.js
// 
// Author: Liv Erickson
// Copyright High Fidelity 2018
//
// Licensed under the Apache 2.0 License
// See accompanying license file or http://apache.org/
//
(function(){

    var ZoneItem = function(){

    };

    var showPanelsForVR = function(deviceType) {
        switch (deviceType) {
            case "Rift" :
                if (HMD.isHandControllerAvailable()) {
                    // Assume Touch Controllers
                    
                } else if (!(typeof(Controller.Hardware.GamePad) === 'undefined')) {
                    // Xbox controller & Keyboard/Mouse
                } else {
                    // Keyboard & mouse
                }
                break;
            default:
            // Assume hand controllers are present for OpenVR devices
        } 
    };

    var showPanelsForDesktop = function() {
        if (!(typeof(Controller.Hardware.GamePad) === 'undefined')) {
            // We have a game pad

        } else {
            // Just show keyboard / mouse
        }
    };

    var setDisplayType = function() {
        if (!HMD.active) {
            // Desktop mode, because not in VR
            showPanelsForDesktop();
        } else {
            var deviceType = HMD.isDeviceAvailable("Oculus Rift") ? "Rift" : "Vive";
            showPanelsForVR(deviceType);
        }
    };

    ZoneItem.prototype = {
        preload: function(entityID) {

        }
    };
    return new ZoneItem();

});