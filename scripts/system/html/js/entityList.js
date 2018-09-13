//  entityList.js
//
//  Created by Ryan Huffman on 19 Nov 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

const ASCENDING_SORT = 1;
const DESCENDING_SORT = -1;
const ASCENDING_STRING = '&#x25B4;';
const DESCENDING_STRING = '&#x25BE;';
const LOCKED_GLYPH = "&#xe006;";
const VISIBLE_GLYPH = "&#xe007;";
const TRANSPARENCY_GLYPH = "&#xe00b;";
const BAKED_GLYPH = "&#xe01a;"
const SCRIPT_GLYPH = "k";
const BYTES_PER_MEGABYTE = 1024 * 1024;
const IMAGE_MODEL_NAME = 'default-image-model.fbx';
const COLLAPSE_EXTRA_INFO = "E";
const EXPAND_EXTRA_INFO = "D";
const FILTER_IN_VIEW_ATTRIBUTE = "pressed";
const WINDOW_NONVARIABLE_HEIGHT = 206;
const NUM_COLUMNS = 12;
const DELETE = 46; // Key code for the delete key.
const KEY_P = 80; // Key code for letter p used for Parenting hotkey.

const COMPARE_ASCENDING = function(a, b) {
    let va = a[currentSortColumn];
    let vb = b[currentSortColumn];

    if (va < vb) {
        return -1;
    }  else if (va > vb) {
        return 1;
    }
    return 0;
}
const COMPARE_DESCENDING = function(a, b) {
    return COMPARE_ASCENDING(b, a);
}

// List of all entities
let entities = []
// List of all entities, indexed by Entity ID
var entitiesByID = {};
// The filtered and sorted list of entities
var visibleEntities = [];

var entityList = null;
var selectedEntities = [];
var currentSortColumn = 'type';
var currentSortOrder = ASCENDING_SORT;
var isFilterInView = false;
var showExtraInfo = false;

const ENABLE_PROFILING = false;
var profileIndent = '';
const PROFILE_NOOP = function(_name, fn, args) {
    fn.apply(this, args);
} ;
const PROFILE = !ENABLE_PROFILING ? PROFILE_NOOP : function(name, fn, args) {
    console.log("PROFILE-Web " + profileIndent + "(" + name + ") Begin");
    var previousIndent = profileIndent;
    profileIndent += '  ';
    var before = Date.now();
    fn.apply(this, args);
    var delta = Date.now() - before;
    profileIndent = previousIndent;
    console.log("PROFILE-Web " + profileIndent + "(" + name + ") End " + delta + "ms");
};

debugPrint = function (message) {
    console.log(message);
};

function loaded() {
    openEventBridge(function() {
        elEntityTable = document.getElementById("entity-table");
        elEntityTableBody = document.getElementById("entity-table-body");
        elEntityTableScroll = document.getElementById("entity-table-scroll");
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
        elInfoToggleGlyph = elInfoToggle.firstChild;
        elFooter = document.getElementById("footer-text");
        elNoEntitiesMessage = document.getElementById("no-entities");
        elNoEntitiesInView = document.getElementById("no-entities-in-view");
        elNoEntitiesRadius = document.getElementById("no-entities-radius");
        
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
        elFilter.onkeyup = refreshEntityList;
        elFilter.onpaste = refreshEntityList;
        elFilter.onchange = onFilterChange;
        elFilter.onblur = refreshFooter;
        elInView.onclick = toggleFilterInView;
        elRadius.onchange = onRadiusChange;
        elInfoToggle.onclick = toggleInfo;
        window.onresize = resize;
        
        elNoEntitiesInView.style.display = "none";
        
        entityList = new ListView("entity-table", "entity-table-body", "entity-table-scroll", createRowFunction, updateRowFunction, clearRowFunction);
        
        function onRowClicked(clickEvent) {
            let entityID = this.dataset.entityID;
            let selection = [entityID];
            
            if (clickEvent.ctrlKey) {
                let selectedIndex = selectedEntities.indexOf(entityID);
                if (selectedIndex >= 0) {
                    selection = selectedEntities;
                    selection.splice(selectedIndex, 1)
                } else {
                    selection = selection.concat(selectedEntities);
                }
            } else if (clickEvent.shiftKey && selectedEntities.length > 0) {
                let previousItemFound = -1;
                let clickedItemFound = -1;
                for (let i = 0, len = visibleEntities.length; i < len; ++i) {
                    let entity = visibleEntities[i];
                    if (clickedItemFound === -1 && entityID === entity.id) {
                        clickedItemFound = i;
                    } else if (previousItemFound === -1 && selectedEntities[0] === entity.id) {
                        previousItemFound = i;
                    }
                }
                if (previousItemFound !== -1 && clickedItemFound !== -1) {
                    let betweenItems = [];
                    let toItem = Math.max(previousItemFound, clickedItemFound);
                    // skip first and last item in this loop, we add them to selection after the loop
                    for (let i = (Math.min(previousItemFound, clickedItemFound) + 1); i < toItem; i++) {
                        betweenItems.push(visibleEntities[i].id);
                    }
                    if (previousItemFound > clickedItemFound) {
                        // always make sure that we add the items in the right order
                        betweenItems.reverse();
                    }
                    selection = selection.concat(betweenItems, selectedEntities);
                }
            }
            
            updateSelectedEntities(selection);

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
                entityIds: [this.dataset.entityID],
            }));
        }
        
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
        
        function updateEntityData(entityData) {
            entities = [];
            entitiesByID = {};
            visibleEntities = [];
            
            PROFILE("map-data", function() {
                entityData.forEach(function(entity) {
                    let type = entity.type;
                    let filename = getFilename(entity.url);
                    if (filename === IMAGE_MODEL_NAME) {
                        type = "Image";
                    }
            
                    let entityData = {
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
                        hasScript: entity.hasScript ? SCRIPT_GLYPH : null,
                        elRow: null,
                        selected: false
                    }
                    
                    entities.push(entityData);
                    entitiesByID[entityData.id] = entityData;
                });
            });
        }
        
        function refreshEntityList() {
            PROFILE("refresh-entity-list", function() {
                PROFILE("filter", function() {
                    let searchTerm = elFilter.value;
                    if (searchTerm === '') {
                        visibleEntities = entities.slice(0);
                    } else {
                        visibleEntities = entities.filter(function(e) {
                            return e.name.indexOf(searchTerm) > -1
                                || e.type.indexOf(searchTerm) > -1
                                || e.fullUrl.indexOf(searchTerm) > -1;
                        });
                    }
                });
                
                PROFILE("sort", function() {
                    let cmp = currentSortOrder === ASCENDING_SORT ? COMPARE_ASCENDING : COMPARE_DESCENDING;
                    visibleEntities.sort(cmp);
                });

                PROFILE("update-dom", function() {
                    entityList.setVisibleItemData(visibleEntities);
                    entityList.refresh();
                });
                
                refreshFooter();
            });
        }
        
        function removeEntities(deletedIDs) {
            // Loop from the back so we can pop items off while iterating
            for (let j = entities.length - 1; j >= 0; --j) {
                let id = entities[j].id;
                for (let i = 0, length = deletedIDs.length; i < length; ++i) {
                    if (id === deletedIDs[i]) {
                        entities.splice(j, 1);
                        if (entitiesByID[id].elRow) {
                            entitiesByID[id].elRow.dataset.entityID = 0;
                        }
                        delete entitiesByID[id];
                        break;
                    }
                }
            }

            var rowOffset = entityList.getRowOffset();
            for (let j = visibleEntities.length - 1; j >= 0; --j) {
                let id = visibleEntities[j].id;
                for (let i = 0, length = deletedIDs.length; i < length; ++i) {
                    if (id === deletedIDs[i]) {
                        if (j < rowOffset) { 
                            rowOffset = entityList.decrementRowOffset();
                        }
                        visibleEntities.splice(j, 1);
                        break;
                    }
                }
            }       
            
            entityList.setVisibleItemData(visibleEntities);
            entityList.refresh();
        }
        
        function clearEntities() {
            let firstVisibleRow = entityList.getRowOffset();
            let lastVisibleRow = firstVisibleRow + entityList.getNumRows() - 1;
            for (let i = firstVisibleRow; i <= lastVisibleRow && i < visibleEntities.length; i++) {
                let entity = visibleEntities[i];
                entity.elRow.dataset.entityID = 0;
            }
            
            entities = [];
            entitiesByID = {};
            visibleEntities = [];
            
            entityList.setVisibleItemData([]);
            entityList.resetRowOffset();
            entityList.refresh();
            
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
            PROFILE("set-sort-column", function() {
                if (currentSortColumn === column) {
                    currentSortOrder *= -1;
                } else {
                    elSortOrder[currentSortColumn].innerHTML = "";
                    currentSortColumn = column;
                    currentSortOrder = ASCENDING_SORT;
                }
                elSortOrder[column].innerHTML = currentSortOrder === ASCENDING_SORT ? ASCENDING_STRING : DESCENDING_STRING;
                refreshEntityList();
            });
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
            } else if (visibleEntities.length === 1) {
                elFooter.firstChild.nodeValue = "1 entity found";
            } else {
                elFooter.firstChild.nodeValue = visibleEntities.length + " entities found";
            }
        }
        
        function updateSelectedEntities(selectedIDs) {
            let notFound = false;
            
            let firstVisibleRow = entityList.getRowOffset();
            let lastVisibleRow = firstVisibleRow + entityList.getNumRows() - 1;
            for (let i = firstVisibleRow; i <= lastVisibleRow && i < visibleEntities.length; i++) {
                let entity = visibleEntities[i];
                entity.elRow.className = '';
            }
            
            for (let i = 0; i < selectedEntities.length; i++) {
                let id = selectedEntities[i];
                let entity = entitiesByID[id];
                if (entity) {
                    entitiesByID[id].selected = false;
                }
            }

            selectedEntities = [];
            for (let i = 0; i < selectedIDs.length; i++) {
                let id = selectedIDs[i];
                selectedEntities.push(id);
                if (id in entitiesByID) {
                    let entity = entitiesByID[id];
                    entity.selected = true;
                    if (entity.elRow) {
                        entity.elRow.className = 'selected';
                    }
                } else {
                    notFound = true;
                }
            }

            refreshFooter();

            return notFound;
        }
        
        function createRowFunction() {
            var row = document.createElement("tr");
            for (var i = 0; i < NUM_COLUMNS; i++) {
                var column = document.createElement("td");
                // locked, visible, hasTransparent, isBaked glyph columns
                if (i === 3 || i === 4 || i === 8 || i === 9) {
                    column.className = 'glyph';
                }
                row.appendChild(column);
            }
            row.onclick = onRowClicked;
            row.ondblclick = onRowDoubleClicked;
            return row;
        }
        
        function updateRowFunction(elRow, itemData) {
            var typeCell = elRow.childNodes[0];
            typeCell.innerText = itemData.type;
            var nameCell = elRow.childNodes[1];
            nameCell.innerText = itemData.name;
            var urlCell = elRow.childNodes[2];
            urlCell.innerText = itemData.url;
            var lockedCell = elRow.childNodes[3];
            lockedCell.innerHTML = itemData.locked;
            var visibleCell = elRow.childNodes[4];
            visibleCell.innerHTML = itemData.visible;
            var verticesCountCell = elRow.childNodes[5];
            verticesCountCell.innerText = itemData.verticesCount;
            var texturesCountCell = elRow.childNodes[6];
            texturesCountCell.innerText = itemData.texturesCount;
            var texturesSizeCell = elRow.childNodes[7];
            texturesSizeCell.innerText = itemData.texturesSize;
            var hasTransparentCell = elRow.childNodes[8];
            hasTransparentCell.innerHTML = itemData.hasTransparent;
            var isBakedCell = elRow.childNodes[9];
            isBakedCell.innerHTML = itemData.isBaked;
            var drawCallsCell = elRow.childNodes[10];
            drawCallsCell.innerText = itemData.drawCalls;
            var hasScriptCell = elRow.childNodes[11];
            hasScriptCell.innerText = itemData.hasScript;
            
            if (itemData.selected) {
                elRow.className = 'selected';
            } else {
                elRow.className = '';
            }
            
            var prevItemId = elRow.dataset.entityID;
            var newItemId = itemData.id;
            var validPrevItemID = prevItemId !== undefined && prevItemId > 0;
            if (validPrevItemID && prevItemId !== newItemId && entitiesByID[prevItemId].elRow === elRow) {
                elRow.dataset.entityID = 0;
                entitiesByID[prevItemId].elRow = null;
            }
            if (!validPrevItemID || prevItemId !== newItemId) {
                elRow.dataset.entityID = newItemId;
                entitiesByID[newItemId].elRow = elRow;
            }
        }
        
        function clearRowFunction(elRow) {
            for (var i = 0; i < NUM_COLUMNS; i++) {
                var cell = elRow.childNodes[i];
                cell.innerHTML = "";
            }
            
            var entityID = elRow.dataset.entityID;
            if (entityID && entitiesByID[entityID]) {
                entitiesByID[entityID].elRow = null;
            }
            
            elRow.dataset.entityID = 0;
        }

        function resize() {
            // Store current scroll position
            var prevScrollTop = elEntityTableScroll.scrollTop;
            
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
            
            // Reinitialize rows according to new table height and refresh row data
            entityList.resetRows(window.innerHeight - WINDOW_NONVARIABLE_HEIGHT);
            entityList.refresh();
            
            // Restore scrolling to previous scroll position before resetting rows
            elEntityTableScroll.scrollTop = prevScrollTop;
        };
        
        function toggleFilterInView() {
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
        
        function onFilterChange() {
            refreshEntityList();
            resize();
        }
        
        function onRadiusChange() {
            elRadius.value = Math.max(elRadius.value, 0);
            EventBridge.emitWebEvent(JSON.stringify({ type: 'radius', radius: elRadius.value }));
            refreshEntities();
            elNoEntitiesRadius.firstChild.nodeValue = elRadius.value;
        }

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
                        if (newEntities && newEntities.length === 0) {
                            elNoEntitiesMessage.style.display = "block";
                            elFooter.firstChild.nodeValue = "0 entities found";
                            clearEntities();
                        } else if (newEntities) {
                            elNoEntitiesMessage.style.display = "none";
                            updateEntityData(newEntities);
                            refreshEntityList();
                            updateSelectedEntities(data.selectedIDs);
                        }
                    });
                } else if (data.type === "removeEntities" && data.deletedIDs !== undefined && data.selectedIDs !== undefined) {
                    removeEntities(data.deletedIDs);
                    updateSelectedEntities(data.selectedIDs);
                } else if (data.type === "deleted" && data.ids) {
                    removeEntities(data.ids);
                    refreshFooter();
                }
            });
        }
        
        resize();
        setSortColumn('type');
        refreshEntities();
    });
    
    augmentSpinButtons();

    // Disable right-click context menu which is not visible in the HMD and makes it seem like the app has locked
    document.addEventListener("contextmenu", function (event) {
        event.preventDefault();
    }, false);
}
