//
//  entityListContextMenu.js
//
//  exampleContextMenus.js was originally created by David Rowe on 22 Aug 2018.
//  Modified to entityListContextMenu.js by Thijs Wenker on 10 Oct 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* eslint-env browser */
const CONTEXT_MENU_CLASS = "context-menu";

/**
 * ContextMenu class for EntityList
 * @constructor
 */
function EntityListContextMenu() {
    this._elContextMenu = null;
    this._callback = null;
}

EntityListContextMenu.prototype = {

    /**
     * @private
     */
    _elContextMenu: null,

    /**
     * @private
     */
    _callback: null,

    /**
     * @private
     */
    _selectedEntityID: null,

    /**
     * Close the context menu
     */
    close: function() {
        if (this.isContextMenuOpen()) {
            this._elContextMenu.style.display = "none";
        }
    },

    isContextMenuOpen: function() {
        return this._elContextMenu.style.display === "block";
    },

    /**
     * Open the context menu
     * @param clickEvent
     * @param selectedEntityID
     */
    open: function(clickEvent, selectedEntityID) {
        this._selectedEntityID = selectedEntityID;
        // If the right-clicked item has a context menu open it.
        this._elContextMenu.style.display = "block";
        this._elContextMenu.style.left
            = Math.min(clickEvent.pageX, document.body.offsetWidth - this._elContextMenu.offsetWidth).toString() + "px";
        this._elContextMenu.style.top
            = Math.min(clickEvent.pageY, document.body.clientHeight - this._elContextMenu.offsetHeight).toString() + "px";
        clickEvent.stopPropagation();
    },

    /**
     * Set the event callback
     * @param callback
     */
    setCallback: function(callback) {
        this._callback = callback;
    },

    /**
     * Add a labeled item to the context menu
     * @param itemLabel
     * @param isEnabled
     * @private
     */
    _addListItem: function(itemLabel, isEnabled) {
        let elListItem = document.createElement("li");
        elListItem.innerText = itemLabel;

        if (isEnabled === undefined || isEnabled) {
            elListItem.addEventListener("click", function () {
                if (this._callback) {
                    this._callback.call(this, itemLabel, this._selectedEntityID);
                }
            }.bind(this), false);
        } else {
            elListItem.setAttribute('class', 'disabled');
        }
        this._elContextMenu.appendChild(elListItem);
    },

    /**
     * Add a separator item to the context menu
     * @private
     */
    _addListSeparator: function() {
        let elListItem = document.createElement("li");
        elListItem.setAttribute('class', 'separator');
        this._elContextMenu.appendChild(elListItem);
    },

    /**
     * Initialize the context menu.
     */
    initialize: function() {
        this._elContextMenu = document.createElement("ul");
        this._elContextMenu.setAttribute("class", CONTEXT_MENU_CLASS);
        document.body.appendChild(this._elContextMenu);

        // TODO: enable Copy, Paste and Duplicate items once implemented
        this._addListItem("Copy", false);
        this._addListItem("Paste", false);
        this._addListSeparator();
        this._addListItem("Rename");
        this._addListItem("Duplicate", false);
        this._addListItem("Delete");

        // Ignore clicks on context menu background or separator.
        this._elContextMenu.addEventListener("click", function(event) {
            // Sink clicks on context menu background or separator but let context menu item clicks through.
            if (event.target.classList.contains(CONTEXT_MENU_CLASS)) {
                event.stopPropagation();
            }
        });

        // Provide means to close context menu without clicking menu item.
        document.body.addEventListener("click", this.close.bind(this));
        document.body.addEventListener("keydown", function(event) {
            // Close context menu with Esc key.
            if (this.isContextMenuOpen() && event.key === "Escape") {
                this.close();
            }
        }.bind(this));
    }
};
