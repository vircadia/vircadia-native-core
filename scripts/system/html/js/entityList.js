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
const BYTES_PER_MEGABYTE = 1024 * 1024;
const IMAGE_MODEL_NAME = 'default-image-model.fbx';
const COLLAPSE_EXTRA_INFO = "E";
const EXPAND_EXTRA_INFO = "D";
const FILTER_IN_VIEW_ATTRIBUTE = "pressed";
const WINDOW_NONVARIABLE_HEIGHT = 227;
const EMPTY_ENTITY_ID = "0";
const MAX_LENGTH_RADIUS = 9;
const MINIMUM_COLUMN_WIDTH = 24;
const SCROLLBAR_WIDTH = 20;
const RESIZER_WIDTH = 10;

const COLUMNS = {
    type: {
        columnHeader: "Type",
        propertyID: "type",
        initialWidth: 0.16,
        initiallyShown: true,
        alwaysShown: true,
    },
    name: {
        columnHeader: "Name",
        propertyID: "name",
        initialWidth: 0.34,
        initiallyShown: true,
        alwaysShown: true,
    },
    url: {
        columnHeader: "File",
        dropdownLabel: "File",
        propertyID: "url",
        initialWidth: 0.34,
        initiallyShown: true,
    },
    locked: {
        columnHeader: "&#xe006;",
        glyph: true,
        propertyID: "locked",
        initialWidth: 0.08,
        initiallyShown: true,
        alwaysShown: true,
    },
    visible: {
        columnHeader: "&#xe007;",
        glyph: true,
        propertyID: "visible",
        initialWidth: 0.08,
        initiallyShown: true,
        alwaysShown: true,
    },
    verticesCount: {
        columnHeader: "Verts",
        dropdownLabel: "Vertices",
        propertyID: "verticesCount",
        initialWidth: 0.08,
    },
    texturesCount: {
        columnHeader: "Texts",
        dropdownLabel: "Textures",
        propertyID: "texturesCount",
        initialWidth: 0.08,
    },
    texturesSize: {
        columnHeader: "Text MB",
        dropdownLabel: "Texture Size",
        propertyID: "texturesSize",
        initialWidth: 0.10,
    },
    hasTransparent: {
        columnHeader: "&#xe00b;",
        glyph: true,
        dropdownLabel: "Transparency",
        propertyID: "hasTransparent",
        initialWidth: 0.04,
    },
    isBaked: {
        columnHeader: "&#xe01a;",
        glyph: true,
        dropdownLabel: "Baked",
        propertyID: "isBaked",
        initialWidth: 0.08,
    },
    drawCalls: {
        columnHeader: "Draws",
        dropdownLabel: "Draws",
        propertyID: "drawCalls",
        initialWidth: 0.08,
    },
    hasScript: {
        columnHeader: "k",
        glyph: true,
        dropdownLabel: "Script",
        propertyID: "hasScript",
        initialWidth: 0.06,
    },
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
};
const COMPARE_DESCENDING = function(a, b) {
    return COMPARE_ASCENDING(b, a);
};

const FILTER_TYPES = [
    "Shape",
    "Model",
    "Image",
    "Light",
    "Zone",
    "Web",
    "Material",
    "ParticleEffect",
    "PolyLine",
    "PolyVox",
    "Text",
];

const ICON_FOR_TYPE = {
    Shape: "n",
    Model: "&#xe008;",
    Image: "&#xe02a;",
    Light: "p",
    Zone: "o",
    Web: "q",
    Material: "&#xe00b;",
    ParticleEffect: "&#xe004;",
    PolyLine: "&#xe01b;",
    PolyVox: "&#xe005;",
    Text: "l",
};

const DOUBLE_CLICK_TIMEOUT = 300; // ms
const RENAME_COOLDOWN = 400; // ms

// List of all entities
let entities = [];
// List of all entities, indexed by Entity ID
let entitiesByID = {};
// The filtered and sorted list of entities passed to ListView
let visibleEntities = [];
// List of all entities that are currently selected
let selectedEntities = [];

let entityList = null; // The ListView

/**
 * @type EntityListContextMenu
 */
let entityListContextMenu = null;

let currentSortColumn = 'type';
let currentSortOrder = ASCENDING_SORT;
let elSortOrders = {};
let typeFilters = [];
let isFilterInView = false;

let columns = [];
let columnsByID = {};
let currentResizeEl = null;
let startResizeEvent = null;
let resizeColumnIndex = 0;
let startThClick = null;
let renameTimeout = null;
let renameLastBlur = null;
let renameLastEntityID = null;
let isRenameFieldBeingMoved = false;

let elEntityTable,
    elEntityTableHeader,
    elEntityTableBody,
    elEntityTableScroll,
    elEntityTableHeaderRow,
    elRefresh,
    elToggleLocked,
    elToggleVisible,
    elDelete,
    elFilterTypeMultiselectBox,
    elFilterTypeText,
    elFilterTypeOptions,
    elFilterSearch,
    elFilterInView,
    elFilterRadius,
    elExport,
    elPal,
    elSelectedEntitiesCount,
    elVisibleEntitiesCount,
    elNoEntitiesMessage,
    elColumnsMultiselectBox,
    elColumnsOptions,
    elToggleSpaceMode,
    elRenameInput;

const ENABLE_PROFILING = false;
let profileIndent = '';
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
        elEntityTableHeader = document.getElementById("entity-table-header");
        elEntityTableBody = document.getElementById("entity-table-body");
        elEntityTableScroll = document.getElementById("entity-table-scroll");
        elRefresh = document.getElementById("refresh");
        elToggleLocked = document.getElementById("locked");
        elToggleVisible = document.getElementById("visible");
        elDelete = document.getElementById("delete");
        elFilterTypeMultiselectBox = document.getElementById("filter-type-multiselect-box");
        elFilterTypeText = document.getElementById("filter-type-text");
        elFilterTypeOptions = document.getElementById("filter-type-options");
        elFilterSearch = document.getElementById("filter-search");
        elFilterInView = document.getElementById("filter-in-view");
        elFilterRadius = document.getElementById("filter-radius");
        elExport = document.getElementById("export");
        elPal = document.getElementById("pal");
        elSelectedEntitiesCount = document.getElementById("selected-entities-count");
        elVisibleEntitiesCount = document.getElementById("visible-entities-count");
        elNoEntitiesMessage = document.getElementById("no-entities");
        elColumnsMultiselectBox = document.getElementById("entity-table-columns-multiselect-box");
        elColumnsOptions = document.getElementById("entity-table-columns-options");
        elToggleSpaceMode = document.getElementById('toggle-space-mode');
        
        document.body.onclick = onBodyClick;
        elToggleLocked.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'toggleLocked' }));
        };
        elToggleVisible.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'toggleVisible' }));
        };
        elExport.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'export'}));
        };
        elPal.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'pal' }));
        };
        elDelete.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'delete' }));
        };
        elToggleSpaceMode.onclick = function() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'toggleSpaceMode' }));
        };
        elRefresh.onclick = refreshEntities;
        elFilterTypeMultiselectBox.onclick = onToggleTypeDropdown;
        elFilterSearch.onkeyup = refreshEntityList;
        elFilterSearch.onsearch = refreshEntityList;
        elFilterInView.onclick = onToggleFilterInView;
        elFilterRadius.onkeyup = onRadiusChange;
        elFilterRadius.onchange = onRadiusChange;
        elColumnsMultiselectBox.onclick = onToggleColumnsDropdown;
        
        // create filter type dropdown checkboxes with label and icon for each type
        for (let i = 0; i < FILTER_TYPES.length; ++i) {
            let type = FILTER_TYPES[i];
            let typeFilterID = "filter-type-" + type;
            
            let elDiv = document.createElement('div');
            elDiv.onclick = onToggleTypeFilter;
            elFilterTypeOptions.appendChild(elDiv);
            
            let elInput = document.createElement('input');
            elInput.setAttribute("type", "checkbox");
            elInput.setAttribute("id", typeFilterID);
            elInput.setAttribute("filterType", type);
            elInput.checked = true; // all types are checked initially
            elDiv.appendChild(elInput);
            
            let elLabel = document.createElement('label');
            elLabel.setAttribute("for", typeFilterID);
            elLabel.innerText = type;
            elDiv.appendChild(elLabel);
            
            let elSpan = document.createElement('span');
            elSpan.setAttribute("class", "typeIcon");
            elSpan.innerHTML = ICON_FOR_TYPE[type];

            elLabel.insertBefore(elSpan, elLabel.childNodes[0]);
            
            toggleTypeFilter(elInput, false); // add all types to the initial types filter
        }
        
        // create columns
        elHeaderTr = document.createElement("tr");
        elEntityTableHeader.appendChild(elHeaderTr);
        let columnIndex = 0;
        for (let columnID in COLUMNS) {
            let columnData = COLUMNS[columnID];
            
            let thID = "entity-" + columnID;
            let elTh = document.createElement("th");
            elTh.setAttribute("id", thID);
            elTh.setAttribute("data-resizable-column-id", thID);
            if (columnData.glyph) {
                let elGlyph = document.createElement("span");
                elGlyph.className = "glyph";
                elGlyph.innerHTML = columnData.columnHeader;
                elTh.appendChild(elGlyph);
            } else {
                elTh.innerText = columnData.columnHeader;
            }
            elTh.onmousedown = function() {
                startThClick = this;
            };
            elTh.onmouseup = function() {
                if (startThClick === this) {
                    setSortColumn(columnID);
                }
                startThClick = null;
            };

            let elResizer = document.createElement("span");
            elResizer.className = "resizer";
            elResizer.innerHTML = "&nbsp;";
            elResizer.setAttribute("columnIndex", columnIndex);
            elResizer.onmousedown = onStartResize;
            elTh.appendChild(elResizer);

            let elSortOrder = document.createElement("span");
            elSortOrder.className = "sort-order";
            elTh.appendChild(elSortOrder);
            elHeaderTr.appendChild(elTh);
                        
            elSortOrders[columnID] = elSortOrder;
            
            // add column to columns dropdown if it is not set to be always shown
            if (columnData.alwaysShown !== true) { 
                let columnDropdownID = "entity-table-column-" + columnID;
                
                let elDiv = document.createElement('div');
                elDiv.onclick = onToggleColumn;
                elColumnsOptions.appendChild(elDiv);
                
                let elInput = document.createElement('input');
                elInput.setAttribute("type", "checkbox");
                elInput.setAttribute("id", columnDropdownID);
                elInput.setAttribute("columnID", columnID);
                elInput.checked = columnData.initiallyShown === true;
                elDiv.appendChild(elInput);
                
                let elLabel = document.createElement('label');
                elLabel.setAttribute("for", columnDropdownID);
                elLabel.innerText = columnData.dropdownLabel;
                elDiv.appendChild(elLabel);
            }
            
            let initialWidth = columnData.initiallyShown === true ? columnData.initialWidth : 0;
            columns.push({
                columnID: columnID,
                elTh: elTh,
                elResizer: elResizer,
                width: initialWidth,
                data: columnData
            });
            columnsByID[columnID] = columns[columnIndex];
            
            ++columnIndex;
        }
        
        elEntityTableHeaderRow = document.querySelectorAll("#entity-table thead th");
        
        entityList = new ListView(elEntityTableBody, elEntityTableScroll, elEntityTableHeaderRow, createRow, updateRow,
                                  clearRow, preRefresh, postRefresh, preRefresh, WINDOW_NONVARIABLE_HEIGHT);

        entityListContextMenu = new EntityListContextMenu();

        function startRenamingEntity(entityID) {
            renameLastEntityID = entityID;
            let entity = entitiesByID[entityID];
            if (!entity || entity.locked || !entity.elRow) {
                return;
            }

            let elCell = entity.elRow.childNodes[getColumnIndex("name")];
            elRenameInput = document.createElement("input");
            elRenameInput.setAttribute('class', 'rename-entity');
            elRenameInput.value = entity.name;
            let ignoreClicks = function(event) {
                event.stopPropagation();
            };
            elRenameInput.onclick = ignoreClicks;
            elRenameInput.ondblclick = ignoreClicks;
            elRenameInput.onkeyup = function(keyEvent) {
                if (keyEvent.key === "Enter") {
                    elRenameInput.blur();
                }
            };

            elRenameInput.onblur = function(event) {
                if (isRenameFieldBeingMoved) {
                    return;
                }
                let value = elRenameInput.value;
                EventBridge.emitWebEvent(JSON.stringify({
                    type: 'rename',
                    entityID: entityID,
                    name: value
                }));
                entity.name = value;
                elRenameInput.parentElement.innerText = value;

                renameLastBlur = Date.now();
                elRenameInput = null;
            };

            elCell.innerHTML = "";
            elCell.appendChild(elRenameInput);

            elRenameInput.select();
        }

        function preRefresh() {
            // move the rename input to the body
            if (!isRenameFieldBeingMoved && elRenameInput) {
                isRenameFieldBeingMoved = true;
                document.body.appendChild(elRenameInput);
                // keep the focus
                elRenameInput.select();
            }
        }

        function postRefresh() {
            if (!elRenameInput || !isRenameFieldBeingMoved) {
                return;
            }
            let entity = entitiesByID[renameLastEntityID];
            if (!entity || entity.locked || !entity.elRow) {
                return;
            }
            let elCell = entity.elRow.childNodes[getColumnIndex("name")];
            elCell.innerHTML = "";
            elCell.appendChild(elRenameInput);
            // keep the focus
            elRenameInput.select();
            isRenameFieldBeingMoved = false;
        }

        entityListContextMenu.setOnSelectedCallback(function(optionName, selectedEntityID) {
            switch (optionName) {
                case "Cut":
                    EventBridge.emitWebEvent(JSON.stringify({ type: 'cut' }));
                    break;
                case "Copy":
                    EventBridge.emitWebEvent(JSON.stringify({ type: 'copy' }));
                    break;
                case "Paste":
                    EventBridge.emitWebEvent(JSON.stringify({ type: 'paste' }));
                    break;
                case "Rename":
                    startRenamingEntity(selectedEntityID);
                    break;
                case "Duplicate":
                    EventBridge.emitWebEvent(JSON.stringify({ type: 'duplicate' }));
                    break;
                case "Delete":
                    EventBridge.emitWebEvent(JSON.stringify({ type: 'delete' }));
                    break;
            }
        });

        function onRowContextMenu(clickEvent) {
            if (elRenameInput) {
                // disallow the context menu from popping up while renaming
                return;
            }

            let entityID = this.dataset.entityID;

            if (!selectedEntities.includes(entityID)) {
                let selection = [entityID];
                updateSelectedEntities(selection);

                EventBridge.emitWebEvent(JSON.stringify({
                    type: "selectionUpdate",
                    focus: false,
                    entityIds: selection,
                }));
            }

            let enabledContextMenuItems = ['Copy', 'Paste', 'Duplicate'];
            if (entitiesByID[entityID] && !entitiesByID[entityID].locked) {
                enabledContextMenuItems.push('Cut');
                enabledContextMenuItems.push('Rename');
                enabledContextMenuItems.push('Delete');
            }

            entityListContextMenu.open(clickEvent, entityID, enabledContextMenuItems);
        }

        let clearRenameTimeout = () => {
            if (renameTimeout !== null) {
                window.clearTimeout(renameTimeout);
                renameTimeout = null;
            }
        };

        function onRowClicked(clickEvent) {
            let entityID = this.dataset.entityID;
            let selection = [entityID];

            if (clickEvent.ctrlKey) {
                let selectedIndex = selectedEntities.indexOf(entityID);
                if (selectedIndex >= 0) {
                    selection = [];
                    selection = selection.concat(selectedEntities);
                    selection.splice(selectedIndex, 1);
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
                // if reselecting the same entity then start renaming it
                if (selectedEntities[0] === entityID) {
                    if (renameLastBlur && renameLastEntityID === entityID && (Date.now() - renameLastBlur) < RENAME_COOLDOWN) {

                        return;
                    }
                    clearRenameTimeout();
                    renameTimeout = window.setTimeout(() => {
                        renameTimeout = null;
                        startRenamingEntity(entityID);
                    }, DOUBLE_CLICK_TIMEOUT);
                }
            }
            
            updateSelectedEntities(selection, false);

            EventBridge.emitWebEvent(JSON.stringify({
                type: "selectionUpdate",
                focus: false,
                entityIds: selection,
            }));
        }

        function onRowDoubleClicked() {
            clearRenameTimeout();

            let selection = [this.dataset.entityID];
            updateSelectedEntities(selection, false);

            EventBridge.emitWebEvent(JSON.stringify({
                type: "selectionUpdate",
                focus: true,
                entityIds: selection,
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
                    };
                    
                    entities.push(entityData);
                    entitiesByID[entityData.id] = entityData;
                });
            });
            
            refreshEntityList();
        }
        
        function refreshEntityList() {
            PROFILE("refresh-entity-list", function() {
                PROFILE("filter", function() {
                    let searchTerm = elFilterSearch.value.toLowerCase();
                    visibleEntities = entities.filter(function(e) {
                        let type = e.type === "Box" || e.type === "Sphere" ? "Shape" : e.type;
                        let typeFilter = typeFilters.indexOf(type) > -1;
                        let searchFilter = searchTerm === '' || (e.name.toLowerCase().indexOf(searchTerm) > -1 ||
                                                                 e.type.toLowerCase().indexOf(searchTerm) > -1 ||
                                                                 e.fullUrl.toLowerCase().indexOf(searchTerm) > -1 ||
                                                                 e.id.toLowerCase().indexOf(searchTerm) > -1);
                        return typeFilter && searchFilter;
                    });
                });
                
                PROFILE("sort", function() {
                    let cmp = currentSortOrder === ASCENDING_SORT ? COMPARE_ASCENDING : COMPARE_DESCENDING;
                    visibleEntities.sort(cmp);
                });

                PROFILE("update-dom", function() {
                    entityList.itemData = visibleEntities;
                    entityList.refresh();
                    updateColumnWidths();
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

        function setSortColumn(column) {
            PROFILE("set-sort-column", function() {
                if (currentSortColumn === column) {
                    currentSortOrder *= -1;
                } else {
                    elSortOrders[currentSortColumn].innerHTML = "";
                    currentSortColumn = column;
                    currentSortOrder = ASCENDING_SORT;
                }
                refreshSortOrder();
                refreshEntityList();
            });
        }
        
        function refreshSortOrder() {
            elSortOrders[currentSortColumn].innerHTML = currentSortOrder === ASCENDING_SORT ? ASCENDING_STRING : DESCENDING_STRING;
        }
        
        function refreshEntities() {
            EventBridge.emitWebEvent(JSON.stringify({ type: 'refresh' }));
        }
        
        function refreshFooter() {
            elSelectedEntitiesCount.innerText = selectedEntities.length;
            elVisibleEntitiesCount.innerText = visibleEntities.length;
        }
        
        function refreshNoEntitiesMessage() {
            if (visibleEntities.length > 0) {
                elNoEntitiesMessage.style.display = "none";
            } else {
                elNoEntitiesMessage.style.display = "block";
            }
        }
        
        function updateSelectedEntities(selectedIDs, autoScroll) {
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

            if (autoScroll && selectedIDs.length > 0) {
                let firstItem = Number.MAX_VALUE;
                let lastItem = -1;
                let itemFound = false;
                visibleEntities.forEach(function(entity, index) {
                    if (selectedIDs.indexOf(entity.id) !== -1) {
                        if (firstItem > index) {
                            firstItem = index;
                        }
                        if (lastItem < index) {
                            lastItem = index;
                        }
                        itemFound = true;
                    }
                });
                if (itemFound) {
                    entityList.scrollToRow(firstItem, lastItem);
                }
            }

            elToggleSpaceMode.disabled = selectedIDs.length > 1;

            refreshFooter();

            return notFound;
        }
        
        function createRow() {
            let elRow = document.createElement("tr");
            columns.forEach(function(column) {
                let elRowColumn = document.createElement("td");
                elRowColumn.className = createColumnClassName(column.columnID);
                elRow.appendChild(elRowColumn);
            });
            elRow.oncontextmenu = onRowContextMenu;
            elRow.onclick = onRowClicked;
            elRow.ondblclick = onRowDoubleClicked;
            return elRow;
        }
        
        function updateRow(elRow, itemData) {
            // update all column texts and glyphs to this entity's data
            for (let i = 0; i < columns.length; ++i) {
                let column = columns[i];
                let elCell = elRow.childNodes[i];
                if (column.data.glyph) {
                    elCell.innerHTML = itemData[column.data.propertyID] ? column.data.columnHeader : null;
                } else {
                    elCell.innerText = itemData[column.data.propertyID];
                }
                elCell.style = "min-width:" + column.widthPx + "px;" + "max-width:" + column.widthPx + "px;";
                elCell.className = createColumnClassName(column.columnID);
            }

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
            // reset all texts and glyphs for each of the row's columns
            for (let i = 0; i < columns.length; ++i) {
                let cell = elRow.childNodes[i];
                if (columns[i].data.glyph) {
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
        
        function onToggleFilterInView() {
            isFilterInView = !isFilterInView;
            if (isFilterInView) {
                elFilterInView.setAttribute(FILTER_IN_VIEW_ATTRIBUTE, FILTER_IN_VIEW_ATTRIBUTE);
            } else {
                elFilterInView.removeAttribute(FILTER_IN_VIEW_ATTRIBUTE);
            }
            EventBridge.emitWebEvent(JSON.stringify({ type: "filterInView", filterInView: isFilterInView }));
            refreshEntities();
        }
        
        function onRadiusChange() {
            elFilterRadius.value = elFilterRadius.value.replace(/[^0-9]/g, '');
            elFilterRadius.value = Math.max(elFilterRadius.value, 0);
            EventBridge.emitWebEvent(JSON.stringify({ type: 'radius', radius: elFilterRadius.value }));
            refreshEntities();
        }
        
        function getColumnIndex(columnID) {
            for (let i = 0; i < columns.length; ++i) {
                if (columns[i].columnID === columnID) {
                    return i;
                }
            }
            return -1;
        }
        
        function createColumnClassName(columnID) {
            let column = columnsByID[columnID];
            let visible = column.elTh.style.visibility !== "hidden";
            let className = column.data.glyph ? "glyph" : "";
            className += visible ? "" : " hidden";
            return className;
        }
        
        function isColumnsDropdownVisible() {
            return elColumnsOptions.style.display === "block";
        }
        
        function toggleColumnsDropdown() {
            elColumnsOptions.style.display = isColumnsDropdownVisible() ? "none" : "block";
        }
        
        function onToggleColumnsDropdown(event) {
            toggleColumnsDropdown();
            if (isTypeDropdownVisible()) {
                toggleTypeDropdown();
            }
            event.stopPropagation();
        }
        
        function toggleColumn(elInput, refresh) {
            let columnID = elInput.getAttribute("columnID");
            let columnChecked = elInput.checked;
            
            if (columnChecked) {
                let widthNeeded = columnsByID[columnID].data.initialWidth;
                
                let numberVisibleColumns = 0;
                for (let i = 0; i < columns.length; ++i) {
                    let column = columns[i];
                    if (column.columnID === columnID) {
                        column.width = widthNeeded;
                    } else if (column.width > 0) {
                        ++numberVisibleColumns;
                    }
                }
                
                for (let i = 0; i < columns.length; ++i) {
                    let column = columns[i];
                    if (column.columnID !== columnID && column.width > 0) {
                        column.width -= column.width * widthNeeded;
                    }
                }
            } else {
                let widthLoss = 0;
                
                let numberVisibleColumns = 0;
                for (let i = 0; i < columns.length; ++i) {
                    let column = columns[i];
                    if (column.columnID === columnID) {
                        widthLoss = column.width;
                        column.width = 0;
                    } else if (column.width > 0) {
                        ++numberVisibleColumns;
                    }
                }
                
                for (let i = 0; i < columns.length; ++i) {
                    let column = columns[i];
                    if (column.columnID !== columnID && column.width > 0) {
                        let newTotalWidth = (1 - widthLoss);
                        column.width += (column.width / newTotalWidth) * widthLoss;
                    }
                }
            }
            
            updateColumnWidths();
        }
        
        function onToggleColumn(event) {
            let elTarget = event.target;
            if (elTarget instanceof HTMLInputElement) {
                toggleColumn(elTarget, true);
            }
            event.stopPropagation();
        }
        
        function isTypeDropdownVisible() {
            return elFilterTypeOptions.style.display === "block";
        }
        
        function toggleTypeDropdown() {
            elFilterTypeOptions.style.display = isTypeDropdownVisible() ? "none" : "block";
        }
        
        function onToggleTypeDropdown(event) {
            toggleTypeDropdown();
            if (isColumnsDropdownVisible()) {
                toggleColumnsDropdown();
            }
            event.stopPropagation();
        }
        
        function toggleTypeFilter(elInput, refresh) {
            let type = elInput.getAttribute("filterType");
            let typeChecked = elInput.checked;
            
            let typeFilterIndex = typeFilters.indexOf(type);
            if (!typeChecked && typeFilterIndex > -1) {
                typeFilters.splice(typeFilterIndex, 1);
            } else if (typeChecked && typeFilterIndex === -1) {
                typeFilters.push(type);
            }
            
            if (typeFilters.length === 0) {
                elFilterTypeText.innerText = "No Types";
            } else if (typeFilters.length === FILTER_TYPES.length) {
                elFilterTypeText.innerText = "All Types";
            } else {
                elFilterTypeText.innerText = "Types...";
            }
            
            if (refresh) {
                refreshEntityList();
            }
        }
        
        function onToggleTypeFilter(event) {
            let elTarget = event.target;
            if (elTarget instanceof HTMLInputElement) {
                toggleTypeFilter(elTarget, true);
            }
            event.stopPropagation();
        }
        
        function onBodyClick(event) {
            // if clicking anywhere outside of the multiselect dropdowns (since click event bubbled up to onBodyClick and
            // propagation wasn't stopped in the toggle type/column callbacks) and the dropdown is open then close it
            if (isTypeDropdownVisible()) {
                toggleTypeDropdown();
            }
            if (isColumnsDropdownVisible()) {
                toggleColumnsDropdown();
            }
        }
        
        function onStartResize(event) {
            startResizeEvent = event;
            resizeColumnIndex = parseInt(this.getAttribute("columnIndex"));
            event.stopPropagation();
        }
        
        function updateColumnWidths() {
            let fullWidth = elEntityTableBody.offsetWidth;
            let remainingWidth = fullWidth;
            let scrollbarVisible = elEntityTableScroll.scrollHeight > elEntityTableScroll.clientHeight;
            let resizerRight = scrollbarVisible ? SCROLLBAR_WIDTH - RESIZER_WIDTH/2 : -RESIZER_WIDTH/2;
            let visibleColumns = 0;
                        
            for (let i = columns.length - 1; i > 0; --i) {
                let column = columns[i];
                column.widthPx = Math.ceil(column.width * fullWidth);
                column.elTh.style = "min-width:" + column.widthPx + "px;" + "max-width:" + column.widthPx + "px;";
                let columnVisible = column.width > 0;
                column.elTh.style.visibility = columnVisible ? "visible" : "hidden";
                if (column.elResizer) {
                    column.elResizer.style = "right:" + resizerRight + "px;";
                    column.elResizer.style.visibility = columnVisible && visibleColumns > 0 ? "visible" : "hidden";
                }
                resizerRight += column.widthPx;
                remainingWidth -= column.widthPx;
                if (columnVisible) {
                    ++visibleColumns;
                }
            }
            
            // assign all remaining space to the first column
            let column = columns[0];
            column.widthPx = remainingWidth;
            column.width = remainingWidth / fullWidth;
            column.elTh.style = "min-width:" + column.widthPx + "px;" + "max-width:" + column.widthPx + "px;";
            let columnVisible = column.width > 0;
            column.elTh.style.visibility = columnVisible ? "visible" : "hidden";
            if (column.elResizer) {
                column.elResizer.style = "right:" + resizerRight + "px;";
                column.elResizer.style.visibility = columnVisible && visibleColumns > 0 ? "visible" : "hidden";
            }
            
            entityList.refresh();
        }
        
        document.onmousemove = function(ev) {
            if (startResizeEvent) {
                startTh = null;
                
                let column = columns[resizeColumnIndex];
                
                let nextColumnIndex = resizeColumnIndex + 1;
                let nextColumn = columns[nextColumnIndex];
                while (nextColumn.width === 0) {
                    nextColumn = columns[++nextColumnIndex];
                }

                let fullWidth = elEntityTableBody.offsetWidth;
                let dx = ev.clientX - startResizeEvent.clientX;
                let dPct = dx / fullWidth;
                
                let newColWidth = column.width + dPct;
                let newNextColWidth = nextColumn.width - dPct;
                
                if (newColWidth * fullWidth >= MINIMUM_COLUMN_WIDTH && newNextColWidth * fullWidth >= MINIMUM_COLUMN_WIDTH) {
                    column.width += dPct;
                    nextColumn.width -= dPct;
                    updateColumnWidths();
                    startResizeEvent = ev;
                }
            }
        };
        
        document.onmouseup = function(ev) {
            startResizeEvent = null;
            ev.stopPropagation();
        };

        function setSpaceMode(spaceMode) {
            if (spaceMode === "local") {
                elToggleSpaceMode.className = "space-mode-local hifi-edit-button";
                elToggleSpaceMode.innerText = "Local";
            } else {
                elToggleSpaceMode.className = "space-mode-world hifi-edit-button";
                elToggleSpaceMode.innerText = "World";
            }
        }

        const KEY_CODES = {
            BACKSPACE: 8,
            DELETE: 46
        };
    
        document.addEventListener("keyup", function (keyUpEvent) {
            if (keyUpEvent.target.nodeName === "INPUT") {
                return;
            }

            let {code, key, keyCode, altKey, ctrlKey, metaKey, shiftKey} = keyUpEvent;

            let controlKey = window.navigator.platform.startsWith("Mac") ? metaKey : ctrlKey;

            let keyCodeString;
            switch (keyCode) {
                case KEY_CODES.DELETE:
                    keyCodeString = "Delete";
                    break;
                case KEY_CODES.BACKSPACE:
                    keyCodeString = "Backspace";
                    break;
                default:
                    keyCodeString = String.fromCharCode(keyUpEvent.keyCode);
                    break;
            }

            if (controlKey && keyCodeString === "A") {
                let visibleEntityIDs = visibleEntities.map(visibleEntity => visibleEntity.id);
                let selectionIncludesAllVisibleEntityIDs = visibleEntityIDs.every(visibleEntityID => {
                    return selectedEntities.includes(visibleEntityID);
                });

                let selection = [];

                if (!selectionIncludesAllVisibleEntityIDs) {
                    selection = visibleEntityIDs;
                }

                updateSelectedEntities(selection);

                EventBridge.emitWebEvent(JSON.stringify({
                    type: "selectionUpdate",
                    focus: false,
                    entityIds: selection,
                }));

                return;
            }


            EventBridge.emitWebEvent(JSON.stringify({
                type: 'keyUpEvent',
                keyUpEvent: {
                    code,
                    key,
                    keyCode,
                    keyCodeString,
                    altKey,
                    controlKey,
                    shiftKey,
                }
            }));
        }, false);
        
        if (window.EventBridge !== undefined) {
            EventBridge.scriptEventReceived.connect(function(data) {
                data = JSON.parse(data);
                if (data.type === "clearEntityList") {
                    clearEntities();
                } else if (data.type === "selectionUpdate") {
                    let notFound = updateSelectedEntities(data.selectedIDs, true);
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
                                updateSelectedEntities(data.selectedIDs, true);
                            }
                        }
                        setSpaceMode(data.spaceMode);
                    });
                } else if (data.type === "removeEntities" && data.deletedIDs !== undefined && data.selectedIDs !== undefined) {
                    removeEntities(data.deletedIDs);
                    updateSelectedEntities(data.selectedIDs, true);
                } else if (data.type === "deleted" && data.ids) {
                    removeEntities(data.ids);
                } else if (data.type === "setSpaceMode") {
                    setSpaceMode(data.spaceMode);
                }
            });
        }
        
        refreshSortOrder();
        refreshEntities();
        
        window.addEventListener("resize", updateColumnWidths);
    });
    
    augmentSpinButtons();

    document.addEventListener("contextmenu", function (event) {
        entityListContextMenu.close();

        // Disable default right-click context menu which is not visible in the HMD and makes it seem like the app has locked
        event.preventDefault();
    }, false);

    // close context menu when switching focus to another window
    $(window).blur(function() {
        entityListContextMenu.close();
    });
}
