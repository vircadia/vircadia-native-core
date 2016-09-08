var ENTITY_LIST_HTML_URL = Script.resolvePath('../html/entityList.html');

EntityListTool = function(opts) {
    var that = {};

    var url = ENTITY_LIST_HTML_URL;
    var webView = new OverlayWebWindow({
        title: 'Entity List',  source: url,  toolWindow: true   
    });


    var filterInView = false;
    var searchRadius = 100;

    var visible = false;

    webView.setVisible(visible);

    that.webView = webView;

    that.setVisible = function(newVisible) {
        visible = newVisible;
        webView.setVisible(visible);
    };

    that.toggleVisible = function() {
        that.setVisible(!visible);
    }

    selectionManager.addEventListener(function() {
        var selectedIDs = [];

        for (var i = 0; i < selectionManager.selections.length; i++) {
            selectedIDs.push(selectionManager.selections[i]);
        }

        var data = {
            type: 'selectionUpdate',
            selectedIDs: selectedIDs,
        };
        webView.emitScriptEvent(JSON.stringify(data));
    });

    that.clearEntityList = function () {
        var data = {
            type: 'clearEntityList'
        }
        webView.emitScriptEvent(JSON.stringify(data));
    };

    function valueIfDefined(value) {
        return value !== undefined ? value : "";
    }

    that.sendUpdate = function() {
        var entities = [];

        var ids;
        if (filterInView) {
            ids = Entities.findEntitiesInView(MyAvatar.position, searchRadius);
        } else {
            ids = Entities.findEntities(MyAvatar.position, searchRadius);
        }

        for (var i = 0; i < ids.length; i++) {
            var id = ids[i];
            var properties = Entities.getEntityProperties(id);
            entities.push({
                id: id,
                name: properties.name,
                type: properties.type,
                url: properties.type == "Model" ? properties.modelURL : "",
                locked: properties.locked,
                visible: properties.visible,
                verticesCount: valueIfDefined(properties.renderInfo.verticesCount),
                texturesCount: valueIfDefined(properties.renderInfo.texturesCount),
                texturesSize: valueIfDefined(properties.renderInfo.texturesSize),
                hasTransparent: valueIfDefined(properties.renderInfo.hasTransparent),
                drawCalls: valueIfDefined(properties.renderInfo.drawCalls),
                hasScript: properties.script !== ""
            });
        }

        var selectedIDs = [];
        for (var i = 0; i < selectionManager.selections.length; i++) {
            selectedIDs.push(selectionManager.selections[i].id);
        }

        var data = {
            type: "update",
            entities: entities,
            selectedIDs: selectedIDs,
        };
        webView.emitScriptEvent(JSON.stringify(data));
    }


    webView.webEventReceived.connect(function(data) {
        data = JSON.parse(data);
        if (data.type == "selectionUpdate") {
            var ids = data.entityIds;
            var entityIDs = [];
            for (var i = 0; i < ids.length; i++) {
                entityIDs.push(ids[i]);
            }
            selectionManager.setSelections(entityIDs);
            if (data.focus) {
                cameraManager.enable();
                cameraManager.focus(selectionManager.worldPosition,
                                    selectionManager.worldDimensions,
                                    Menu.isOptionChecked(MENU_EASE_ON_FOCUS));
            }
        } else if (data.type == "refresh") {
            that.sendUpdate();
        } else if (data.type == "teleport") {
            if (selectionManager.hasSelection()) {
                MyAvatar.position = selectionManager.worldPosition;
            }
        } else if (data.type == "delete") {
            deleteSelectedEntities();
        } else if (data.type == "toggleLocked") {
            toggleSelectedEntitiesLocked();
        } else if (data.type == "toggleVisible") {
            toggleSelectedEntitiesVisible();
        } else if (data.type === "filterInView") {
            filterInView = data.filterInView === true;
        } else if (data.type === "radius") {
            searchRadius = data.radius;
        }
    });

    webView.visibleChanged.connect(function () {
        if (webView.visible) {
            that.sendUpdate();
        }
    });

    return that;
};