'use strict';

//
//  nyx.js
//
//  Created by Kalila L. on Oct 6 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function menuOverlay (entityID, properties) {
    var menu_this = this;
    var menuEntity;

    // function onMenuEventReceived(event) {
    //     var eventJSON = JSON.parse(event);
    // 
    //     if (eventJSON.command === "ready") {
    //         initializeInventoryApp();
    //     }
    // }
    
    // var menuObject = Entities.getEntityObject(onMenuEventReceived);

    function onMousePressOnEntity (pressedEntityID, event) {
        console.log("SOMETHING WAS PRESSED!");
        console.log("ENTITY PRESSED!" + pressedEntityID);
        if (entityID === pressedEntityID && event.isPrimaryButton) {
            toggleMenu();
        }
    }
    
    function toggleMenu() {
        console.log("TOGGLING!!!" + menuEntity);
        if (!menuEntity) {
            menuEntity = Entities.addEntity({
                type: "Web",
                sourceUrl: "https://vircadia.com/",
                position: Entities.getEntityProperties(entityID, ["position"]).position,
                billboardMode: 'full',
                dimensions: {
                    x: 3,
                    y: 3 * 1080 / 1920,
                    z: 0.01
                },
                dpi: 15
            });
        } else {
            Entities.deleteEntity(menuEntity);
            menuEntity = null;
        }
    }
    
    Entities.mousePressOnEntity.connect(onMousePressOnEntity);
    
    menu_this.onScriptEnding = function onScriptEnding () {
        Entities.mousePressOnEntity.disconnect(onMousePressOnEntity);
        Entities.deleteEntity(menuEntity);
    }
}

var newMenu = new menuOverlay('{a3afc217-d299-41ea-bfc6-66eaa9bd0409}', {});

// module.exports = {
//     menuOverlay: menuOverlay
// };