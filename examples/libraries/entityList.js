EntityListTool = function(opts) {
    var that = {};

    var url = Script.resolvePath('html/entityList.html');
    var webView = new WebWindow('Entities', url, 200, 280);

    var visible = false;

    webView.setVisible(visible);

    that.setVisible = function(newVisible) {
        visible = newVisible;
        webView.setVisible(visible);
    };

    selectionManager.addEventListener(function() {
        var selectedIDs = [];

        for (var i = 0; i < selectionManager.selections.length; i++) {
            selectedIDs.push(selectionManager.selections[i].id);
        }

        data = {
            type: 'selectionUpdate',
            selectedIDs: selectedIDs,
        };
        webView.eventBridge.emitScriptEvent(JSON.stringify(data));
    });

    webView.eventBridge.webEventReceived.connect(function(data) {
        data = JSON.parse(data);
        if (data.type == "selectionUpdate") {
            var ids = data.entityIds;
            var entityIDs = [];
            for (var i = 0; i < ids.length; i++) {
                entityIDs.push(Entities.getEntityItemID(ids[i]));
            }
            selectionManager.setSelections(entityIDs);
            if (data.focus) {
                cameraManager.focus(selectionManager.worldPosition,
                                    selectionManager.worldDimensions,
                                    Menu.isOptionChecked(MENU_EASE_ON_FOCUS));
            }
        } else if (data.type == "refresh") {
            var entities = [];
            var ids = Entities.findEntities(MyAvatar.position, 100);
            for (var i = 0; i < ids.length; i++) {
                var id = ids[i];
                var properties = Entities.getEntityProperties(id);
                entities.push({
                    id: id.id,
                    type: properties.type,
                    url: properties.type == "Model" ? properties.modelURL : "",
                });
            }
            var data = {
                type: "update",
                entities: entities,
            };
            webView.eventBridge.emitScriptEvent(JSON.stringify(data));
        }
    });

    return that;
};
