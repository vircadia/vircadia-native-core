//  entityList.js
//
//  Created by Ryan Huffman on 19 Nov 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var entities = {};
var selectedEntities = [];
var currentSortColumn = 'type';
var currentSortOrder = 'des';
var entityList = null;
var refreshEntityListTimer = null;
const ASCENDING_STRING = '&#x25B4;';
const DESCENDING_STRING = '&#x25BE;';
const LOCKED_GLYPH = "&#xe006;";
const VISIBLE_GLYPH = "&#xe007;";
const TRANSPARENCY_GLYPH = "&#xe00b;";
const BAKED_GLYPH = "&#xe01a;"
const SCRIPT_GLYPH = "k";
const DELETE = 46; // Key code for the delete key.
const KEY_P = 80; // Key code for letter p used for Parenting hotkey.
const MAX_ITEMS = Number.MAX_VALUE; // Used to set the max length of the list of discovered entities.

log = function(msg) {
    EventBridge.emitWebEvent(msg);
}

var profileIndent = '';
PROFILE = function(name, fn, args) {
    log("PROFILE-Web " + profileIndent + "(" + name + ") Begin");
    console.log("PROFILE-Web " + profileIndent + "(" + name + ") Begin");
    var previousIndent = profileIndent;
    profileIndent += '  ';
    var before = Date.now();
    fn.apply(this, args);
    var delta = Date.now() - before;
    profileIndent = previousIndent;
    log("PROFILE-Web " + profileIndent + "(" + name + ") End " + delta + "ms");
    console.log("PROFILE-Web " + profileIndent + "(" + name + ") End " + delta + "ms");
}

debugPrint = function (message) {
    console.log(message);
};

function loaded() {
    openEventBridge(function() {
        entityList = new List('entity-list', { valueNames: ['name', 'type', 'url', 'locked', 'visible'], page: MAX_ITEMS});
        entityList.clear();
        elEntityTable = document.getElementById("entity-table");
        elEntityTableBody = document.getElementById("entity-table-body");
        elRefresh = document.getElementById("refresh");
        elToggleLocked = document.getElementById("locked");
        elToggleVisible = document.getElementById("visible");
        elDelete = document.getElementById("delete");
        elFilter = document.getElementById("filter");
        elInView = document.getElementById("in-view")
        elRadius = document.getElementById("radius");
        elExport = document.getElementById("export");
        elPal = document.getElementById("pal");
        elEntityTable = document.getElementById("entity-table");
        elInfoToggle = document.getElementById("info-toggle");
        elInfoToggleGlyph = elInfoToggle.firstChild;
        elFooter = document.getElementById("footer-text");
        elNoEntitiesMessage = document.getElementById("no-entities");
        elNoEntitiesInView = document.getElementById("no-entities-in-view");
        elNoEntitiesRadius = document.getElementById("no-entities-radius");
        elEntityTableScroll = document.getElementById("entity-table-scroll");

        document.getElementById("entity-name").onclick = function() {
            setSortColumn('name');
        };
        document.getElementById("entity-type").onclick = function() {
            setSortColumn('type');
        };
        document.getElementById("entity-url").onclick = function() {
            setSortColumn('url');
        };
        document.getElementById("entity-locked").onclick = function () {
            setSortColumn('locked');
        };
        document.getElementById("entity-visible").onclick = function () {
            setSortColumn('visible');
        };
        document.getElementById("entity-verticesCount").onclick = function () {
            setSortColumn('verticesCount');
        };
        document.getElementById("entity-texturesCount").onclick = function () {
            setSortColumn('texturesCount');
        };
        document.getElementById("entity-texturesSize").onclick = function () {
            setSortColumn('texturesSize');
        };
        document.getElementById("entity-hasTransparent").onclick = function () {
            setSortColumn('hasTransparent');
        };
        document.getElementById("entity-isBaked").onclick = function () {
            setSortColumn('isBaked');
        };
        document.getElementById("entity-drawCalls").onclick = function () {
            setSortColumn('drawCalls');
        };
        document.getElementById("entity-hasScript").onclick = function () {
            setSortColumn('hasScript');
        };

        function onRowClicked(clickEvent) {
            var id = this.dataset.entityId;
            var selection = [this.dataset.entityId];
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
            EventBridge.emitWebEvent(JSON.stringify({
                type: "selectionUpdate",
                focus: true,
                entityIds: [this.dataset.entityId],
            }));
        }

        const BYTES_PER_MEGABYTE = 1024 * 1024;

        function decimalMegabytes(number) {
            return number ? (number / BYTES_PER_MEGABYTE).toFixed(1) : "";
        }

        function displayIfNonZero(number) {
            return number ? number : "";
        }

        function getFilename(url) {
            let urlParts = url.split('/');
            return urlParts[urlParts.length - 1];
        }

        //function addEntity(
        //id, name, type, url, locked, visible, verticesCount,
        //texturesCount, texturesSize, hasTransparent,
        //isBaked, drawCalls, hasScript) {
        function addEntities(entityData) {
            const IMAGE_MODEL_NAME = 'default-image-model.fbx';

            let newEntities = entityData.filter(function(entity) {
                if (entity.id in entities) {
                    var item = entities[entity.id].item;
                    item.values({
                        name: entity.name,
                        url: getFilename(entity.url),
                        locked: entity.locked,
                        visible: entity.visible
                    });
                    return false;
                }
                return true;
            });

            if (newEntities.length === 0) {
                return;
            }

            newEntities = newEntities.map(function(entity) {
                let type = entity.type;
                let filename = getFilename(entity.url);
                if (filename === IMAGE_MODEL_NAME) {
                    type = "Image";
                }
                return {
                    id: entity.id,
                    name: entity.name,
                    type: type,
                    url: filename,
                    fullUrl: entity.url,
                    locked: entity.locked ? LOCKED_GLYPH : null,
                    visible: entity.visible ? VISIBLE_GLYPH : null,
                    verticesCount: displayIfNonZero(entity.verticesCount),
                    texturesCount: displayIfNonZero(entity.texturesCount),
                    texturesSize: decimalMegabytes(entity.texturesSize),
                    hasTransparent: entity.hasTransparent ? TRANSPARENCY_GLYPH : null,
                    isBaked: entity.isBaked ? BAKED_GLYPH : null,
                    drawCalls: displayIfNonZero(entity.drawCalls),
                    hasScript: entity.hasScript ? SCRIPT_GLYPH : null
                }
            });
                //newEntities = newEntities.splice(newEntities.length - 10);
                console.log("Adding: " + newEntities.length);

            let size = 2000;
            let sets = Math.ceil(newEntities.length / size);
            for (let i = 0; i < sets; i++) {
                
                console.log(Date.now(), "Adding", i * size, (i + 1) * size);
            entityList.add(newEntities.splice(i * size, (i + 1) * size),
                function (items) {
                    console.log(Date.now(), "added: " + items.length);
                    items.forEach(function(item) {
                        var currentElement = item.elm;
                        var values = item._values;

                        entities[values.id] = {
                            id: values.id,
                            name: values.name,
                            el: currentElement,
                            item: item
                        };
                        currentElement.setAttribute('id', 'entity_' + values.id);
                        currentElement.setAttribute('title', values.fullUrl);
                        currentElement.dataset.entityId = values.id;
                        currentElement.onclick = onRowClicked;
                        currentElement.ondblclick = onRowDoubleClicked;
                    });
                });
            }
            console.log(Date.now(), "DONE");
        }

        function removeEntities(deletedIDs) {
            for (i = 0, length = deletedIDs.length; i < length; i++) {
                delete entities[deletedIDs[i]];
                entityList.remove("id", deletedIDs[i]);
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
            url: document.querySelector('#entity-url .sort-order'),
            locked: document.querySelector('#entity-locked .sort-order'),
            visible: document.querySelector('#entity-visible .sort-order'),
            verticesCount: document.querySelector('#entity-verticesCount .sort-order'),
            texturesCount: document.querySelector('#entity-texturesCount .sort-order'),
            texturesSize: document.querySelector('#entity-texturesSize .sort-order'),
            hasTransparent: document.querySelector('#entity-hasTransparent .sort-order'),
            isBaked: document.querySelector('#entity-isBaked .sort-order'),
            drawCalls: document.querySelector('#entity-drawCalls .sort-order'),
            hasScript: document.querySelector('#entity-hasScript .sort-order'),
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
            entityList.sort(currentSortColumn, { order: currentSortOrder });
        }
        setSortColumn('type');

        function refreshEntities() {
            clearEntities();
            EventBridge.emitWebEvent(JSON.stringify({ type: 'refresh' }));
        }

        function refreshFooter() {
            if (selectedEntities.length > 1) {
                elFooter.firstChild.nodeValue = selectedEntities.length + " entities selected";
            } else if (selectedEntities.length === 1) {
                elFooter.firstChild.nodeValue = "1 entity selected";
            } else if (entityList.visibleItems.length === 1) {
                elFooter.firstChild.nodeValue = "1 entity found";
            } else {
                elFooter.firstChild.nodeValue = entityList.visibleItems.length + " entities found";
            }
        }

        function refreshEntityListObject() {
            refreshEntityListTimer = null;
            entityList.sort(currentSortColumn, { order: currentSortOrder });
            entityList.search(elFilter.value);
            refreshFooter();
        }

        function updateSelectedEntities(selectedIDs) {
            var notFound = false;
            for (var id in entities) {
                entities[id].el.className = '';
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

        elRefresh.onclick = function() {
            refreshEntities();
        }
        elToggleLocked.onclick = function () {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'toggleLocked' }));
        }
        elToggleVisible.onclick = function () {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'toggleVisible' }));
        }
        elExport.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'export'}));
        }
        elPal.onclick = function () {
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
                    PROFILE("update", function() {
                        var newEntities = data.entities;
                        if (newEntities && newEntities.length == 0) {
                            elNoEntitiesMessage.style.display = "block";
                            elFooter.firstChild.nodeValue = "0 entities found";
                        } else if (newEntities) {
                            elNoEntitiesMessage.style.display = "none";
                            addEntities(newEntities);
                            updateSelectedEntities(data.selectedIDs);
                            scheduleRefreshEntityList();
                            resize();
                        }
                    });
                } else if (data.type === "removeEntities" && data.deletedIDs !== undefined && data.selectedIDs !== undefined) {
                    removeEntities(data.deletedIDs);
                    updateSelectedEntities(data.selectedIDs);
                    scheduleRefreshEntityList();
                } else if (data.type === "deleted" && data.ids) {
                    removeEntities(data.ids);
                    refreshFooter();
                }
            });
            setTimeout(refreshEntities, 1000);
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
                    ths[2].width = 0.34 * tableWidth;
                    ths[3].width = 0.08 * tableWidth;
                    ths[4].width = 0.08 * tableWidth;
                }
            }
        };

        window.onresize = resize;
        elFilter.onchange = resize;
        elFilter.onblur = refreshFooter;


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


        resize();
    });

    augmentSpinButtons();

    // Disable right-click context menu which is not visible in the HMD and makes it seem like the app has locked
    document.addEventListener("contextmenu", function (event) {
        event.preventDefault();
    }, false);
}
