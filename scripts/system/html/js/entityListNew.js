//  entityListNew.js
//
//  Created by David Back on 27 Aug 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var entities = {};
var selectedEntities = [];
var currentSortColumn = 'type';
var currentSortOrder = 'des';
var entityList = null;
var refreshEntityListTimer = null;

debugPrint = function (message) {
    console.log(message);
};

function loaded() {
    openEventBridge(function() {
        elEntityTable = document.getElementById("entity-table");
        elEntityTableBody = document.getElementById("entity-table-body");
        elEntityTableScroll = document.getElementById("entity-table-scroll");
        elEntityTableHeader = document.getElementById("entity-table-header");
        elEntityTableFooter = document.getElementById("entity-table-footer");
        elRefresh = document.getElementById("refresh");
        elToggleLocked = document.getElementById("locked");
        elToggleVisible = document.getElementById("visible");
        elDelete = document.getElementById("delete");
        elFilter = document.getElementById("filter");
        elInView = document.getElementById("in-view")
        elRadius = document.getElementById("radius");
        elExport = document.getElementById("export");
        elPal = document.getElementById("pal");
        elInfoToggle = document.getElementById("info-toggle");
        //elInfoToggleGlyph = elInfoToggle.firstChild;
        elFooter = document.getElementById("footer-text");
        elNoEntitiesMessage = document.getElementById("no-entities");
        elNoEntitiesInView = document.getElementById("no-entities-in-view");
        elNoEntitiesRadius = document.getElementById("no-entities-radius");

        entityList = new ListView("entity-table", "entity-table-body", "entity-table-scroll", createRowFunction, updateRowFunction, clearRowFunction);

        function createRowFunction(elBottomBuffer) {
            var row = document.createElement("tr");
            var typeCell = document.createElement("td");
            var typeCellText = document.createTextNode("");
            var nameCell = document.createElement("td");
            var nameCellText = document.createTextNode("");
            typeCell.appendChild(typeCellText);
            nameCell.appendChild(nameCellText);
            row.appendChild(typeCell);
            row.appendChild(nameCell);
            row.onclick = onRowClicked;
            row.ondblclick = onRowDoubleClicked;
            elEntityTableBody.insertBefore(row, elBottomBuffer);
            return row;
        }
        
        function updateRowFunction(elRow, itemData) {
            var typeCell = elRow.childNodes[0];
            var nameCell = elRow.childNodes[1];
            typeCell.innerHTML = itemData.type;
            nameCell.innerHTML = itemData.name;
            
            var id = elRow.getAttribute("id");
            entities[id].el = elRow;
        }
        
        function clearRowFunction(elRow) {
            var typeCell = elRow.childNodes[0];
            var nameCell = elRow.childNodes[1];
            typeCell.innerHTML = "";
            nameCell.innerHTML = "";
            
            var id = elRow.getAttribute("id");
            if (id && entities[id]) {
                entities[id].el = null;
            }
        }

        function onRowClicked(clickEvent) {
            var id = this.getAttribute("id");
            var selection = [id];

            /*
            if (clickEvent.ctrlKey) {
                var selectedIndex = selectedEntities.indexOf(id);
                if (selectedIndex >= 0) {
                    selection = selectedEntities;
                    selection.splice(selectedIndex, 1)
                } else {
                    selection = selection.concat(selectedEntities);
                }
            } else if (clickEvent.shiftKey && selectedEntities.length > 0) {
                var previousItemFound = -1;
                var clickedItemFound = -1;
                for (var entity in entityList.visibleItems) {
                    if (clickedItemFound === -1 && this.dataset.entityId == entityList.visibleItems[entity].values().id) {
                        clickedItemFound = entity;
                    } else if(previousItemFound === -1 && selectedEntities[0] == entityList.visibleItems[entity].values().id) {
                        previousItemFound = entity;
                    }
                }
                if (previousItemFound !== -1 && clickedItemFound !== -1) {
                    var betweenItems = [];
                    var toItem = Math.max(previousItemFound, clickedItemFound);
                    // skip first and last item in this loop, we add them to selection after the loop
                    for (var i = (Math.min(previousItemFound, clickedItemFound) + 1); i < toItem; i++) {
                        entityList.visibleItems[i].elm.className = 'selected';
                        betweenItems.push(entityList.visibleItems[i].values().id);
                    }
                    if (previousItemFound > clickedItemFound) {
                        // always make sure that we add the items in the right order
                        betweenItems.reverse();
                    }
                    selection = selection.concat(betweenItems, selectedEntities);
                }
            }
            */

            selectedEntities = selection;

            this.className = 'selected';

            EventBridge.emitWebEvent(JSON.stringify({
                type: "selectionUpdate",
                focus: false,
                entityIds: selection,
            }));

            refreshFooter();
        }

        function onRowDoubleClicked() {
            var id = this.getAttribute("id");
            EventBridge.emitWebEvent(JSON.stringify({
                type: "selectionUpdate",
                focus: true,
                entityIds: [id],
            }));
        }

        function addEntity(id, name, type) {            
            //var urlParts = url.split('/');
            //var filename = urlParts[urlParts.length - 1];

            var IMAGE_MODEL_NAME = 'default-image-model.fbx';

            //if (filename === IMAGE_MODEL_NAME) {
            //    type = "Image";
            //}

            if (entities[id] === undefined) {
                var entityData = {name: name, type: type};
                entityList.addItem(id, entityData);
                entityData.el = null;
                entities[id] = entityData;
            }
        }

        function removeEntities(deletedIDs) {
            for (var i = 0, length = deletedIDs.length; i < length; i++) {
                var deleteID = deletedIDs[i];
                delete entities[deleteID];
                entityList.removeItem(deleteID);
            }
        }

        function scheduleRefreshEntityList() {
            var REFRESH_DELAY = 50;
            if (refreshEntityListTimer) {
                clearTimeout(refreshEntityListTimer);
            }
            refreshEntityListTimer = setTimeout(refreshEntityListObject, REFRESH_DELAY);
        }

        function clearEntities() {
            entities = {};
            entityList.clear();
            refreshFooter();
        }

        var elSortOrder = {
            name: document.querySelector('#entity-name .sort-order'),
            type: document.querySelector('#entity-type .sort-order'),
        }
        function setSortColumn(column) {
            if (currentSortColumn == column) {
                currentSortOrder = currentSortOrder == "asc" ? "desc" : "asc";
            } else {
                elSortOrder[currentSortColumn].innerHTML = "";
                currentSortColumn = column;
                currentSortOrder = "asc";
            }
            elSortOrder[column].innerHTML = currentSortOrder == "asc" ? ASCENDING_STRING : DESCENDING_STRING;
            //entityList.sort(currentSortColumn, { order: currentSortOrder });
        }

        function refreshEntities() {
            clearEntities();
            EventBridge.emitWebEvent(JSON.stringify({ type: 'refresh' }));
        }

        function refreshFooter() {
            if (selectedEntities.length > 1) {
                elFooter.firstChild.nodeValue = selectedEntities.length + " entities selected";
            } else if (selectedEntities.length === 1) {
                elFooter.firstChild.nodeValue = "1 entity selected";
            } /* else if (entityList.visibleItems.length === 1) {
                elFooter.firstChild.nodeValue = "1 entity found";
            } else {
                elFooter.firstChild.nodeValue = entityList.visibleItems.length + " entities found";
            } */
        }

        function refreshEntityListObject() {
            refreshEntityListTimer = null;
            //entityList.sort(currentSortColumn, { order: currentSortOrder });
            //entityList.search(elFilter.value);
            entityList.refresh();
            refreshFooter();
        }

        function updateSelectedEntities(selectedIDs) {
            var notFound = false;
            for (var id in entities) {
                if (entities[id].el) {
                    entities[id].el.className = '';
                }
            }

            selectedEntities = [];
            for (var i = 0; i < selectedIDs.length; i++) {
                var id = selectedIDs[i];
                selectedEntities.push(id);
                if (id in entities) {
                    var entity = entities[id];
                    entity.el.className = 'selected';
                } else {
                    notFound = true;
                }
            }

            refreshFooter();

            return notFound;
        }

        function resize() {
            // Take up available window space
            elEntityTableScroll.style.height = window.innerHeight - 207;
            
            var SCROLLABAR_WIDTH = 21;
            var tds = document.querySelectorAll("#entity-table-body tr:first-child td");
            var ths = document.querySelectorAll("#entity-table thead th");
            if (tds.length >= ths.length) {
                // Update the widths of the header cells to match the body
                for (var i = 0; i < ths.length; i++) {
                    ths[i].width = tds[i].offsetWidth;
                }
            } else {
                // Reasonable widths if nothing is displayed
                var tableWidth = document.getElementById("entity-table").offsetWidth - SCROLLABAR_WIDTH;
                if (showExtraInfo) {
                    ths[0].width = 0.10 * tableWidth;
                    ths[1].width = 0.20 * tableWidth;
                    ths[2].width = 0.20 * tableWidth;
                    ths[3].width = 0.04 * tableWidth;
                    ths[4].width = 0.04 * tableWidth;
                    ths[5].width = 0.08 * tableWidth;
                    ths[6].width = 0.08 * tableWidth;
                    ths[7].width = 0.10 * tableWidth;
                    ths[8].width = 0.04 * tableWidth;
                    ths[9].width = 0.08 * tableWidth;
                    ths[10].width = 0.04 * tableWidth + SCROLLABAR_WIDTH;
                } else {
                    ths[0].width = 0.16 * tableWidth;
                    ths[1].width = 0.34 * tableWidth;
                    //ths[2].width = 0.34 * tableWidth;
                    //ths[3].width = 0.08 * tableWidth;
                    //ths[4].width = 0.08 * tableWidth;
                }
            }
        };
        
        var showExtraInfo = false;
        var COLLAPSE_EXTRA_INFO = "E";
        var EXPAND_EXTRA_INFO = "D";

        function toggleInfo(event) {
            showExtraInfo = !showExtraInfo;
            if (showExtraInfo) {
                elEntityTable.className = "showExtraInfo";
                elInfoToggleGlyph.innerHTML = COLLAPSE_EXTRA_INFO;
            } else {
                elEntityTable.className = "";
                elInfoToggleGlyph.innerHTML = EXPAND_EXTRA_INFO;
            }
            resize();
            event.stopPropagation();
        }
        elInfoToggle.addEventListener("click", toggleInfo, true);
        
        elRefresh.onclick = function() {
            refreshEntities();
        }
        elToggleLocked.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'toggleLocked' }));
        }
        elToggleVisible.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'toggleVisible' }));
        }
        elExport.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'export'}));
        }
        elPal.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'pal' }));
        }
        elDelete.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'delete' }));
        }

        document.addEventListener("keydown", function (keyDownEvent) {
            if (keyDownEvent.target.nodeName === "INPUT") {
                return;
            }
            var keyCode = keyDownEvent.keyCode;
            if (keyCode === DELETE) {
                EventBridge.emitWebEvent(JSON.stringify({ type: 'delete' }));
                refreshEntities();
            }
            if (keyDownEvent.keyCode === KEY_P && keyDownEvent.ctrlKey) {
                if (keyDownEvent.shiftKey) {
                    EventBridge.emitWebEvent(JSON.stringify({ type: 'unparent' }));
                } else {
                    EventBridge.emitWebEvent(JSON.stringify({ type: 'parent' }));
                }
            }
        }, false);

        var isFilterInView = false;
        var FILTER_IN_VIEW_ATTRIBUTE = "pressed";
        elNoEntitiesInView.style.display = "none";
        elInView.onclick = function () {
            isFilterInView = !isFilterInView;
            if (isFilterInView) {
                elInView.setAttribute(FILTER_IN_VIEW_ATTRIBUTE, FILTER_IN_VIEW_ATTRIBUTE);
                elNoEntitiesInView.style.display = "inline";
            } else {
                elInView.removeAttribute(FILTER_IN_VIEW_ATTRIBUTE);
                elNoEntitiesInView.style.display = "none";
            }
            EventBridge.emitWebEvent(JSON.stringify({ type: "filterInView", filterInView: isFilterInView }));
            refreshEntities();
        }

        elRadius.onchange = function () {
            elRadius.value = Math.max(elRadius.value, 0);
            EventBridge.emitWebEvent(JSON.stringify({ type: 'radius', radius: elRadius.value }));
            refreshEntities();
            elNoEntitiesRadius.firstChild.nodeValue = elRadius.value;
        }
        
        if (window.EventBridge !== undefined) {
            EventBridge.scriptEventReceived.connect(function(data) {
                data = JSON.parse(data);
                if (data.type === "clearEntityList") {
                    clearEntities();
                } else if (data.type == "selectionUpdate") {
                    var notFound = updateSelectedEntities(data.selectedIDs);
                    if (notFound) {
                        refreshEntities();
                    }
                } else if (data.type === "update" && data.selectedIDs !== undefined) {
                    var newEntities = data.entities;
                    if (newEntities && newEntities.length == 0) {
                        elNoEntitiesMessage.style.display = "block";
                        elFooter.firstChild.nodeValue = "0 entities found";
                    } else if (newEntities) {
                        elNoEntitiesMessage.style.display = "none";
                        for (var i = 0; i < newEntities.length; i++) {
                            addEntity(newEntities[i].id, newEntities[i].name, newEntities[i].type);
                        }
                        updateSelectedEntities(data.selectedIDs);
                        entityList.refresh();
                        //scheduleRefreshEntityList();
                        resize();
                    }
                } else if (data.type === "removeEntities" && data.deletedIDs !== undefined && data.selectedIDs !== undefined) {
                    removeEntities(data.deletedIDs);
                    updateSelectedEntities(data.selectedIDs);
                    //scheduleRefreshEntityList();
                } else if (data.type === "deleted" && data.ids) {
                    removeEntities(data.ids);
                    refreshFooter();
                }
            });
        }
        
        resize();
        entityList.initialize(572);
        refreshEntities();
    });

    // Disable right-click context menu which is not visible in the HMD and makes it seem like the app has locked
    document.addEventListener("contextmenu", function (event) {
        event.preventDefault();
    }, false);
}
