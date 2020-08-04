/*
    clickToOpen.js

    Created by Kalila L. on 3 Aug 2020
    Copyright 2020 Vircadia and contributors.
    
    Distributed under the Apache License, Version 2.0.
    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
*/

// Standard preload and unload, initialize the entity script here.

(function () {
    "use strict";
    this.entityID = null;
    var _this = this;
    
    var overlayWebWindow;
    
    function getURLfromEntityDescription() {
        return Entities.getEntityProperties(_this.entityID, ["description"]).description;
    }
    
    function onMousePressOnEntity(pressedEntityID, event) {
        if (_this.entityID == pressedEntityID) {
            overlayWebWindow = new OverlayWebWindow({
                title: "Vircadia Browser",
                source: getURLfromEntityDescription(),
                width: 800,
                height: 600
            });
        }
    }

    Entities.mousePressOnEntity.connect(onMousePressOnEntity);

    this.preload = function (ourID) {
        this.entityID = ourID;
        
        Entities.mousePressOnEntity.connect(onMousePressOnEntity);
    };

    this.unload = function(entityID) {
        Entities.mousePressOnEntity.disconnect(onMousePressOnEntity);
    };

});