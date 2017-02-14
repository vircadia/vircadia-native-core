//
// displayLastEditedBy.js
//
// Created by Si Fi Faye Li on 2 December, 2016
//
// Draws a line from each entity to the user in the current session who last changed a property, if any, as recorded
// by the lastEditedBy property.

(function () {
    var SHOW_LAST_EDITED_BY_ME = true;
    var SEARCH_RADIUS = 40;
    // in meter, if the entities is too far away(out of search radius), we won't display its last edited by

    var LINE_COLOR = { red: 0, green: 255, blue: 255};
    var LINE_EXPRIRATION_TIME = 3000; // in ms
    var UPDATE_INTERVAL = 1 / 60; // 60fps 
    var myHashMap = {};  // stores {entityID of target entity : overlayID of the line}

    var timer = 0;
    var lastUpdateTime = 0;
    function update(deltaTime) {
        timer += deltaTime;
        if (timer - lastUpdateTime > UPDATE_INTERVAL) {
            var targetEntityIDs = Entities.findEntities(MyAvatar.position,SEARCH_RADIUS);

            targetEntityIDs.forEach(function(targetEntityID){
                var targetEntityProps = Entities.getEntityProperties(targetEntityID);


                // don't draw lines for entities that were last edited long time ago
                if (targetEntityProps.hasOwnProperty("lastEdited")) {
                    var currentTime = new Date().getTime();
                    // lastEdited is in usec while JS date object returns msec
                    var timeDiff = currentTime - targetEntityProps.lastEdited/1000;
                    if (timeDiff > LINE_EXPRIRATION_TIME) {
                        if (myHashMap.hasOwnProperty(targetEntityID)) {
                            var overlayID = myHashMap[targetEntityID];
                            Overlays.deleteOverlay(overlayID);
                        }
                        return;
                    }
                }

                var targetAvatarUUID = targetEntityProps.lastEditedBy;

                // don't draw lines for entities last edited by myself
                // you may set SHOW_LAST_EDITED_BY_ME to true if you want to see these lines
                if (targetAvatarUUID === MyAvatar.sessionUUID && !SHOW_LAST_EDITED_BY_ME) {
                    if (myHashMap.hasOwnProperty(targetEntityID)) {
                        var overlayID = myHashMap[targetEntityID];
                        Overlays.deleteOverlay(overlayID);
                    }
                    return;
                }
                // don't draw lines for entities with no last edited by
                if (targetAvatarUUID === "{00000000-0000-0000-0000-000000000000}") {
                    if (myHashMap.hasOwnProperty(targetEntityID)) {
                        var overlayID = myHashMap[targetEntityID];
                        Overlays.deleteOverlay(overlayID);
                    }
                    return;
                } 

                var targetAvatar = AvatarList.getAvatar(targetAvatarUUID);
                
                // skip adding overlay if the avatar can't be found
                if (targetAvatar === null) {
                    // delete overlay if the avatar was found before but no long here
                    if (myHashMap.hasOwnProperty(targetEntityID)) {
                        var overlayID = myHashMap[targetEntityID];
                        Overlays.deleteOverlay(overlayID);
                    }
                    return;
                }

                var props = {
                    start: targetEntityProps.position,
                    end: targetAvatar.position,
                    color: LINE_COLOR,
                    alpha: 1,
                    ignoreRayIntersection: true,
                    visible: true,
                    solid: true,
                    drawInFront: true
                };

                if (myHashMap.hasOwnProperty(targetEntityID)) {
                    var overlayID = myHashMap[targetEntityID];
                    Overlays.editOverlay(overlayID, props);
                } else {
                    var newOverlayID = Overlays.addOverlay("line3d", props);
                    myHashMap[targetEntityID] = newOverlayID;
                }
                
            });

            // remove lines for entities no longer within search radius
            for (var key in myHashMap) {
                if (myHashMap.hasOwnProperty(key)) {
                    if (targetEntityIDs.indexOf(key) === -1) {
                        var overlayID = myHashMap[key];
                        Overlays.deleteOverlay(overlayID);
                        delete myHashMap[key];
                    }
                } 
            }

            lastUpdateTime = timer;
        }     
    }
    Script.update.connect(update);

    function cleanup() {
        for (var key in myHashMap) {
            if (myHashMap.hasOwnProperty(key)) {
                var overlayID = myHashMap[key];
                Overlays.deleteOverlay(overlayID);
            } 
        }
    }
    Script.scriptEnding.connect(cleanup);
})();
