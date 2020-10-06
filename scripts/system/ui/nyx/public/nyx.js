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

function menuOverlay (entityID, menuItems) {
    var menu_this = this;
    var menuWebEntity;

    function sendToWeb(command, data) {
        var dataToSend = {
            "command": command,
            "data": data
        };
        Entities.emitScriptEvent(menuWebEntity, dataToSend);
    }
    
    function onMenuEventReceived(sendingEntityID, event) {
        var eventJSON = JSON.parse(event);
        if (sendingEntityID === menuWebEntity) {
            if (eventJSON.command === "ready") {
                var dataToSend = {
                    menuItems: menuItems
                };
                sendToWeb('script-to-web-initialize', dataToSend);
            }
        }
    }

    Entities.webEventReceived.connect(onMenuEventReceived);

    function onMousePressOnEntity (pressedEntityID, event) {
        if (entityID === pressedEntityID && event.isPrimaryButton) {
            toggleMenu();
        }
    }

    Entities.mousePressOnEntity.connect(onMousePressOnEntity);

    function toggleMenu() {
        if (!menuWebEntity) {
            menuWebEntity = Entities.addEntity({
                type: "Web",
                sourceUrl: Script.resolvePath("./index.html"),
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
            Entities.deleteEntity(menuWebEntity);
            menuWebEntity = null;
        }
    }

    menu_this.onScriptEnding = function onScriptEnding () {
        Entities.webEventReceived.disconnect(onMenuEventReceived);
        Entities.mousePressOnEntity.disconnect(onMousePressOnEntity);
        Entities.deleteEntity(menuWebEntity);
    }
}

var newMenu = new menuOverlay('{a3afc217-d299-41ea-bfc6-66eaa9bd0409}', ['This', 'Is', 'Nice']);

// module.exports = {
//     menuOverlay: menuOverlay
// };