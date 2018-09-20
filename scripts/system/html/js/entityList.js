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
const EMPTY_ENTITY_ID = "0";
const DELETE = 46; // Key code for the delete key.
const KEY_P = 80; // Key code for letter p used for Parenting hotkey.

const COLUMN_INDEX = {
    TYPE: 0,
    NAME: 1,
    URL: 2,
    LOCKED: 3,
    VISIBLE: 4,
    VERTICLES_COUNT: 5,
    TEXTURES_COUNT: 6,
    TEXTURES_SIZE: 7,
    HAS_TRANSPARENT: 8,
    IS_BAKED: 9,
    DRAW_CALLS: 10,
    HAS_SCRIPT: 11
};

const COMPARE_ASCENDING = function(a, b) {
    let va = a[currentSortColumn];
    let vb = b[currentSortColumn];

    if (va < vb) {
        return -1;
    }  else if (va > vb) {
        return 1;
    } else if (a.id < b.id) {
        return -1;
    }

    return 1;
}
const COMPARE_DESCENDING = function(a, b) {
    return COMPARE_ASCENDING(b, a);
}

// List of all entities
var entities = []
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
    let previousIndent = profileIndent;
    profileIndent += '  ';
    let before = Date.now();
    fn.apply(this, args);
    let delta = Date.now() - before;
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
        
        entityList = new ListView("entity-table", "entity-table-body", "entity-table-scroll", createRow, updateRow, clearRow);
        
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
                    let searchTerm = elFilter.value.toLowerCase();
                    if (searchTerm === '') {
                        visibleEntities = entities.slice(0);
                    } else {
                        visibleEntities = entities.filter(function(e) {
                            return e.name.toLowerCase().indexOf(searchTerm) > -1
                                || e.type.toLowerCase().indexOf(searchTerm) > -1
                                || e.fullUrl.toLowerCase().indexOf(searchTerm) > -1
                                || e.id.toLowerCase().indexOf(searchTerm) > -1;
                        });
                    }
                });
                
                PROFILE("sort", function() {
                    let cmp = currentSortOrder === ASCENDING_SORT ? COMPARE_ASCENDING : COMPARE_DESCENDING;
                    visibleEntities.sort(cmp);
                });

                PROFILE("update-dom", function() {
                    entityList.setItemData(visibleEntities);
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
                            entitiesByID[id].elRow.dataset.entityID = EMPTY_ENTITY_ID;
                        }
                        delete entitiesByID[id];
                        break;
                    }
                }
            }

            let rowOffset = entityList.rowOffset;
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
            
            entityList.refresh();
        }
        
        function clearEntities() {
            let firstVisibleRow = entityList.rowOffset;
            let lastVisibleRow = firstVisibleRow + entityList.getNumRows() - 1;
            for (let i = firstVisibleRow; i <= lastVisibleRow && i < visibleEntities.length; i++) {
                let entity = visibleEntities[i];
                entity.elRow.dataset.entityID = EMPTY_ENTITY_ID;
            }
            
            entities = [];
            entitiesByID = {};
            visibleEntities = [];
            
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
            
            selectedEntities.forEach(function(id) {
                let entity = entitiesByID[id];
                if (entity !== undefined) {
                    entity.selected = false;
                    entity.elRow.className = '';
                }
            });
            
            selectedEntities = [];
            
            selectedIDs.forEach(function(id) {
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
            });

            refreshFooter();

            return notFound;
        }
        
        function createRow() {
            let row = document.createElement("tr");
            for (let i = 0; i < NUM_COLUMNS; i++) {
                let column = document.createElement("td");
                if (i === COLUMN_INDEX.LOCKED || i === COLUMN_INDEX.VISIBLE || 
                    i === COLUMN_INDEX.HAS_TRANSPARENT || i === COLUMN_INDEX.IS_BAKED) {
                    column.className = 'glyph';
                }
                row.appendChild(column);
            }
            row.onclick = onRowClicked;
            row.ondblclick = onRowDoubleClicked;
            return row;
        }
        
        function updateRow(elRow, itemData) {
            let typeCell = elRow.childNodes[COLUMN_INDEX.TYPE];
            typeCell.innerText = itemData.type;
            let nameCell = elRow.childNodes[COLUMN_INDEX.NAME];
            nameCell.innerText = itemData.name;
            let urlCell = elRow.childNodes[COLUMN_INDEX.URL];
            urlCell.innerText = itemData.url;
            let lockedCell = elRow.childNodes[COLUMN_INDEX.LOCKED];
            lockedCell.innerHTML = itemData.locked;
            let visibleCell = elRow.childNodes[COLUMN_INDEX.VISIBLE];
            visibleCell.innerHTML = itemData.visible;
            let verticesCountCell = elRow.childNodes[COLUMN_INDEX.VERTICLES_COUNT];
            verticesCountCell.innerText = itemData.verticesCount;
            let texturesCountCell = elRow.childNodes[COLUMN_INDEX.TEXTURES_COUNT];
            texturesCountCell.innerText = itemData.texturesCount;
            let texturesSizeCell = elRow.childNodes[COLUMN_INDEX.TEXTURES_SIZE];
            texturesSizeCell.innerText = itemData.texturesSize;
            let hasTransparentCell = elRow.childNodes[COLUMN_INDEX.HAS_TRANSPARENT];
            hasTransparentCell.innerHTML = itemData.hasTransparent;
            let isBakedCell = elRow.childNodes[COLUMN_INDEX.IS_BAKED];
            isBakedCell.innerHTML = itemData.isBaked;
            let drawCallsCell = elRow.childNodes[COLUMN_INDEX.DRAW_CALLS];
            drawCallsCell.innerText = itemData.drawCalls;
            let hasScriptCell = elRow.childNodes[COLUMN_INDEX.HAS_SCRIPT];
            hasScriptCell.innerText = itemData.hasScript;
            
            if (itemData.selected) {
                elRow.className = 'selected';
            } else {
                elRow.className = '';
            }
            
            let prevEntityID = elRow.dataset.entityID;
            let newEntityID = itemData.id;
            let validPrevItemID = prevEntityID !== undefined && prevEntityID !== EMPTY_ENTITY_ID;
            if (validPrevItemID && prevEntityID !== newEntityID && entitiesByID[prevEntityID].elRow === elRow) {
                elRow.dataset.entityID = EMPTY_ENTITY_ID;
                entitiesByID[prevEntityID].elRow = null;
            }
            if (!validPrevItemID || prevEntityID !== newEntityID) {
                elRow.dataset.entityID = newEntityID;
                entitiesByID[newEntityID].elRow = elRow;
            }
        }
        
        function clearRow(elRow) {
            for (let i = 0; i < NUM_COLUMNS; i++) {
                let cell = elRow.childNodes[i];
                cell.innerHTML = "";
            }
            
            let entityID = elRow.dataset.entityID;
            if (entityID && entitiesByID[entityID]) {
                entitiesByID[entityID].elRow = null;
            }
            
            elRow.dataset.entityID = EMPTY_ENTITY_ID;
        }

        function resize() {
            // Store current scroll position
            let prevScrollTop = elEntityTableScroll.scrollTop;
            
            // Take up available window space
            elEntityTableScroll.style.height = window.innerHeight - 207;
            
            let SCROLLABAR_WIDTH = 21;
            let tds = document.querySelectorAll("#entity-table-body tr:first-child td");
            let ths = document.querySelectorAll("#entity-table thead th");
            if (tds.length >= ths.length) {
                // Update the widths of the header cells to match the body
                for (let i = 0; i < ths.length; i++) {
                    ths[i].width = tds[i].offsetWidth;
                }
            } else {
                // Reasonable widths if nothing is displayed
                let tableWidth = document.getElementById("entity-table").offsetWidth - SCROLLABAR_WIDTH;
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
            let keyCode = keyDownEvent.keyCode;
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
                    let notFound = updateSelectedEntities(data.selectedIDs);
                    if (notFound) {
                        refreshEntities();
                    }
                } else if (data.type === "update" && data.selectedIDs !== undefined) {
                    PROFILE("update", function() {
                        let newEntities = data.entities;
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
