"use strict";

//  entityList.js
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global EntityListTool, Tablet, selectionManager, Entities, Camera, MyAvatar, Vec3, Menu, Messages,
   cameraManager, MENU_EASE_ON_FOCUS, deleteSelectedEntities, toggleSelectedEntitiesLocked, toggleSelectedEntitiesVisible */

EntityListTool = function(opts) {
    var that = {};

    var webView = null;
    webView = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    webView.setVisible = function(value) {};

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
    };

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
        };
        webView.emitScriptEvent(JSON.stringify(data));
    };

    function valueIfDefined(value) {
        return value !== undefined ? value : "";
    }

    that.sendUpdate = function() {
        var entities = [];

        var ids;
        if (filterInView) {
            ids = Entities.findEntitiesInFrustum(Camera.frustum);
        } else {
            ids = Entities.findEntities(MyAvatar.position, searchRadius);
        }

        var cameraPosition = Camera.position;
        for (var i = 0; i < ids.length; i++) {
            var id = ids[i];
            var properties = Entities.getEntityProperties(id);

            if (!filterInView || Vec3.distance(properties.position, cameraPosition) <= searchRadius) {
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
                    isBaked: properties.type == "Model" ? properties.modelURL.toLowerCase().endsWith(".baked.fbx") : false, 
                    drawCalls: valueIfDefined(properties.renderInfo.drawCalls),
                    hasScript: properties.script !== ""
                });
            }
        }

        var selectedIDs = [];
        for (var j = 0; j < selectionManager.selections.length; j++) {
            selectedIDs.push(selectionManager.selections[j].id);
        }

        var data = {
            type: "update",
            entities: entities,
            selectedIDs: selectedIDs,
        };
        webView.emitScriptEvent(JSON.stringify(data));
    };

    webView.webEventReceived.connect(function(data) {
        try {
            data = JSON.parse(data);
        } catch(e) {
            print("entityList.js: Error parsing JSON: " + e.name + " data " + data)
            return;
        }

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
        } else if (data.type == "export") {
            if (!selectionManager.hasSelection()) {
                Window.notifyEditError("No entities have been selected.");
            } else {
                var filename = Window.save("Select Where to Save", "", "*.json");
                if (filename) {
                    var success = Clipboard.exportEntities(filename, selectionManager.selections);
                    if (!success) {
                        Window.notifyEditError("Export failed.");
                    }
                }
            }
        } else if (data.type == "pal") {
            var sessionIds = {}; // Collect the sessionsIds of all selected entitities, w/o duplicates.
            selectionManager.selections.forEach(function (id) {
                var lastEditedBy = Entities.getEntityProperties(id, 'lastEditedBy').lastEditedBy;
                if (lastEditedBy) {
                    sessionIds[lastEditedBy] = true;
                }
            });
            var dedupped = Object.keys(sessionIds);
            if (!selectionManager.selections.length) {
                Window.alert('No objects selected.');
            } else if (!dedupped.length) {
                Window.alert('There were no recent users of the ' + selectionManager.selections.length + ' selected objects.');
            } else {
                // No need to subscribe if we're just sending.
                Messages.sendMessage('com.highfidelity.pal', JSON.stringify({method: 'select', params: [dedupped, true, false]}), 'local');
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

    // webView.visibleChanged.connect(function () {
    //     if (webView.visible) {
    //         that.sendUpdate();
    //     }
    // });

    return that;
};
