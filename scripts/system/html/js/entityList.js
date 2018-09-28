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
const WINDOW_NONVARIABLE_HEIGHT = 207;
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
// The filtered and sorted list of entities passed to ListView
var visibleEntities = [];
// List of all entities that are currently selected
var selectedEntities = [];

var entityList = null; // The ListView

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
        elEntityTableHeaderRow = document.querySelectorAll("#entity-table thead th");
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
        
        elNoEntitiesInView.style.display = "none";
        
        entityList = new ListView(elEntityTableBody, elEntityTableScroll, elEntityTableHeaderRow,
                                  createRow, updateRow, clearRow, WINDOW_NONVARIABLE_HEIGHT);
        
        function onRowClicked(clickEvent) {
            let entityID = this.dataset.entityID;
            let selection = [entityID];
            
            if (clickEvent.ctrlKey) {
                let selectedIndex = selectedEntities.indexOf(entityID);
                if (selectedIndex >= 0) {
                    selection = [];
                    selection = selection.concat(selectedEntities);
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
                    selection = [];
                    let toItem = Math.max(previousItemFound, clickedItemFound);
                    for (let i = Math.min(previousItemFound, clickedItemFound); i <= toItem; i++) {
                        selection.push(visibleEntities[i].id);
                    }
                    if (previousItemFound > clickedItemFound) {
                        // always make sure that we add the items in the right order
                        selection.reverse();
                    }
                }
            } else if (!clickEvent.ctrlKey && !clickEvent.shiftKey && selectedEntities.length === 1) {
                // if reselecting the same entity then deselect it
                if (selectedEntities[0] === entityID) {
                    selection = [];
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
            visibleEntities.length = 0; // maintains itemData reference in ListView
            
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
                        locked: entity.locked,
                        visible: entity.visible,
                        verticesCount: displayIfNonZero(entity.verticesCount),
                        texturesCount: displayIfNonZero(entity.texturesCount),
                        texturesSize: decimalMegabytes(entity.texturesSize),
                        hasTransparent: entity.hasTransparent,
                        isBaked: entity.isBaked,
                        drawCalls: displayIfNonZero(entity.drawCalls),
                        hasScript: entity.hasScript,
                        elRow: null, // if this entity has a visible row element assigned to it
                        selected: false // if this entity is selected for edit regardless of having a visible row
                    }
                    
                    entities.push(entityData);
                    entitiesByID[entityData.id] = entityData;
                });
            });
            
            refreshEntityList();
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
                    entityList.itemData = visibleEntities;
                    entityList.refresh();
                });
                
                refreshFooter();
                refreshNoEntitiesMessage();
            });
        }
        
        function removeEntities(deletedIDs) {
            // Loop from the back so we can pop items off while iterating
            
            // delete any entities matching deletedIDs list from entities and entitiesByID lists
            // if the entity had an associated row element then ensure row is unselected and clear it's entity
            for (let j = entities.length - 1; j >= 0; --j) {
                let id = entities[j].id;
                for (let i = 0, length = deletedIDs.length; i < length; ++i) {
                    if (id === deletedIDs[i]) {
                        let elRow = entities[j].elRow;
                        if (elRow) {
                            elRow.className = '';
                            elRow.dataset.entityID = EMPTY_ENTITY_ID;
                        }
                        entities.splice(j, 1);
                        delete entitiesByID[id];
                        break;
                    }
                }
            }
            
            // delete any entities matching deletedIDs list from selectedEntities list
            for (let j = selectedEntities.length - 1; j >= 0; --j) {
                let id = selectedEntities[j].id;
                for (let i = 0, length = deletedIDs.length; i < length; ++i) {
                    if (id === deletedIDs[i]) {
                        selectedEntities.splice(j, 1);
                        break;
                    }
                }
            }

            // delete any entities matching deletedIDs list from visibleEntities list
            // if this was a row that was above our current row offset (a hidden top row in the top buffer),
            // then decrease row offset accordingly
            let firstVisibleRow = entityList.getFirstVisibleRowIndex();
            for (let j = visibleEntities.length - 1; j >= 0; --j) {
                let id = visibleEntities[j].id;
                for (let i = 0, length = deletedIDs.length; i < length; ++i) {
                    if (id === deletedIDs[i]) {
                        if (j < firstVisibleRow && entityList.rowOffset > 0) {
                            entityList.rowOffset--;
                        }
                        visibleEntities.splice(j, 1);
                        break;
                    }
                }
            }
            
            entityList.refresh();
            
            refreshFooter();
            refreshNoEntitiesMessage();
        }
        
        function clearEntities() {
            // clear the associated entity ID from all visible row elements
            let firstVisibleRow = entityList.getFirstVisibleRowIndex();
            let lastVisibleRow = entityList.getLastVisibleRowIndex();
            for (let i = firstVisibleRow; i <= lastVisibleRow && i < visibleEntities.length; i++) {
                let entity = visibleEntities[i];
                entity.elRow.dataset.entityID = EMPTY_ENTITY_ID;
            }
            
            entities = [];
            entitiesByID = {};
            visibleEntities.length = 0; // maintains itemData reference in ListView
            
            entityList.resetToTop();
            entityList.clear();
            
            refreshFooter();
            refreshNoEntitiesMessage();
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
                refreshSortOrder();
                refreshEntityList();
            });
        }
        function refreshSortOrder() {
            elSortOrder[currentSortColumn].innerHTML = currentSortOrder === ASCENDING_SORT ? ASCENDING_STRING : DESCENDING_STRING;
        }
        
        function refreshEntities() {
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
        
        function refreshNoEntitiesMessage() {
            if (visibleEntities.length > 0) {
                elNoEntitiesMessage.style.display = "none";
            } else {
                elNoEntitiesMessage.style.display = "block";
            }
        }
        
        function updateSelectedEntities(selectedIDs) {
            let notFound = false;
            
            // reset all currently selected entities and their rows first
            selectedEntities.forEach(function(id) {
                let entity = entitiesByID[id];
                if (entity !== undefined) {
                    entity.selected = false;
                    if (entity.elRow) {
                        entity.elRow.className = '';
                    }
                }
            });
            
            // then reset selected entities list with newly selected entities and set them selected
            selectedEntities = [];
            selectedIDs.forEach(function(id) {
                selectedEntities.push(id);
                let entity = entitiesByID[id];
                if (entity !== undefined) {
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
        
        function isGlyphColumn(columnIndex) {
            return columnIndex === COLUMN_INDEX.LOCKED || columnIndex === COLUMN_INDEX.VISIBLE || 
                   columnIndex === COLUMN_INDEX.HAS_TRANSPARENT || columnIndex === COLUMN_INDEX.IS_BAKED || 
                   columnIndex === COLUMN_INDEX.HAS_SCRIPT;
        }
        
        function createRow() {
            let row = document.createElement("tr");
            for (let i = 0; i < NUM_COLUMNS; i++) {
                let column = document.createElement("td");
                if (isGlyphColumn(i)) {
                    column.className = 'glyph';
                }
                row.appendChild(column);
            }
            row.onclick = onRowClicked;
            row.ondblclick = onRowDoubleClicked;
            return row;
        }
        
        function updateRow(elRow, itemData) {
            // update all column texts and glyphs to this entity's data
            let typeCell = elRow.childNodes[COLUMN_INDEX.TYPE];
            typeCell.innerText = itemData.type;
            let nameCell = elRow.childNodes[COLUMN_INDEX.NAME];
            nameCell.innerText = itemData.name;
            let urlCell = elRow.childNodes[COLUMN_INDEX.URL];
            urlCell.innerText = itemData.url;
            let lockedCell = elRow.childNodes[COLUMN_INDEX.LOCKED];
            lockedCell.innerHTML = itemData.locked ? LOCKED_GLYPH : null;
            let visibleCell = elRow.childNodes[COLUMN_INDEX.VISIBLE];
            visibleCell.innerHTML = itemData.visible ? VISIBLE_GLYPH : null;
            let verticesCountCell = elRow.childNodes[COLUMN_INDEX.VERTICLES_COUNT];
            verticesCountCell.innerText = itemData.verticesCount;
            let texturesCountCell = elRow.childNodes[COLUMN_INDEX.TEXTURES_COUNT];
            texturesCountCell.innerText = itemData.texturesCount;
            let texturesSizeCell = elRow.childNodes[COLUMN_INDEX.TEXTURES_SIZE];
            texturesSizeCell.innerText = itemData.texturesSize;
            let hasTransparentCell = elRow.childNodes[COLUMN_INDEX.HAS_TRANSPARENT];
            hasTransparentCell.innerHTML = itemData.hasTransparent ? TRANSPARENCY_GLYPH : null;
            let isBakedCell = elRow.childNodes[COLUMN_INDEX.IS_BAKED];
            isBakedCell.innerHTML = itemData.isBaked ? BAKED_GLYPH : null;
            let drawCallsCell = elRow.childNodes[COLUMN_INDEX.DRAW_CALLS];
            drawCallsCell.innerText = itemData.drawCalls;
            let hasScriptCell = elRow.childNodes[COLUMN_INDEX.HAS_SCRIPT];
            hasScriptCell.innerHTML = itemData.hasScript ? SCRIPT_GLYPH : null;
            
            // if this entity was previously selected flag it's row as selected
            if (itemData.selected) {
                elRow.className = 'selected';
            } else {
                elRow.className = '';
            }

            // if this row previously had an associated entity ID that wasn't the new entity ID then clear
            // the ID from the row and the row element from the previous entity's data, then set the new 
            // entity ID to the row and the row element to the new entity's data
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
            // reset all texts and glyphs for each of the row's column
            for (let i = 0; i < NUM_COLUMNS; i++) {
                let cell = elRow.childNodes[i];
                if (isGlyphColumn(i)) {
                    cell.innerHTML = "";
                } else {
                    cell.innerText = "";
                }
            }
            
            // clear the row from any associated entity
            let entityID = elRow.dataset.entityID;
            if (entityID && entitiesByID[entityID]) {
                entitiesByID[entityID].elRow = null;
            }
            
            // reset the row to hidden and clear the entity from the row
            elRow.className = '';
            elRow.dataset.entityID = EMPTY_ENTITY_ID;
        }
        
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
            entityList.resize();
        }
        
        function onRadiusChange() {
            elRadius.value = Math.max(elRadius.value, 0);
            elNoEntitiesRadius.firstChild.nodeValue = elRadius.value;
            elNoEntitiesMessage.style.display = "none";
            EventBridge.emitWebEvent(JSON.stringify({ type: 'radius', radius: elRadius.value }));
            refreshEntities();
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
            entityList.resize();
            event.stopPropagation();
        }

        document.addEventListener("keydown", function (keyDownEvent) {
            if (keyDownEvent.target.nodeName === "INPUT") {
                return;
            }
            let keyCode = keyDownEvent.keyCode;
            if (keyCode === DELETE) {
                EventBridge.emitWebEvent(JSON.stringify({ type: 'delete' }));
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
                        if (newEntities) {
                            if (newEntities.length === 0) {
                                clearEntities();
                            } else {
                                updateEntityData(newEntities);
                                updateSelectedEntities(data.selectedIDs);
                            }
                        }
                    });
                } else if (data.type === "removeEntities" && data.deletedIDs !== undefined && data.selectedIDs !== undefined) {
                    removeEntities(data.deletedIDs);
                    updateSelectedEntities(data.selectedIDs);
                } else if (data.type === "deleted" && data.ids) {
                    removeEntities(data.ids);
                }
            });
        }
        
        refreshSortOrder();
        refreshEntities();
    });
    
    augmentSpinButtons();

    // Disable right-click context menu which is not visible in the HMD and makes it seem like the app has locked
    document.addEventListener("contextmenu", function (event) {
        event.preventDefault();
    }, false);
}
