//  listView.js
//
//  Created by David Back on 27 Aug 2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

const SCROLL_ROWS = 2; // number of rows used as scrolling buffer, each time we pass this number of rows we scroll
const FIRST_ROW_INDEX = 3; // the first elRow element's index in the child nodes of the table body

debugPrint = function (message) {
    console.log(message);
};

function ListView(tableId, tableBodyId, tableScrollId, createRowFunction, updateRowFunction, clearRowFunction) {      
    this.elTable = document.getElementById(tableId);
    this.elTableBody = document.getElementById(tableBodyId);
    this.elTableScroll = document.getElementById(tableScrollId);
    
    this.elTopBuffer = null;
    this.elBottomBuffer = null;
    
    this.elRows = [];
    this.itemData = [];
    
    this.createRowFunction = createRowFunction;
    this.updateRowFunction = updateRowFunction;
    this.clearRowFunction = clearRowFunction;
    
    this.rowOffset = 0; // the current index within the itemData list used for the top most elRow element
    this.rowHeight = 0; // height of the elRow elements
    this.lastRowChangeScrollTop = 0; // the previous elTableScroll.scrollTop value when the rows were scrolled

    this.initialize();
};
    
ListView.prototype = {
    getNumRows: function() {
        return this.elRows.length;
    },
    
    getScrollHeight: function() {
        return this.rowHeight * SCROLL_ROWS;
    },
    
    getFirstVisibleRowIndex: function() {
        return this.rowOffset;
    },
    
    getLastVisibleRowIndex: function() {
        return this.getFirstVisibleRowIndex() + entityList.getNumRows() - 1;
    },
    
    resetRowOffset: function() {
        this.rowOffset = 0;
        this.lastRowChangeScrollTop = 0;
    },
    
    removeItems: function(removeItems) {
        let firstVisibleRow = this.getFirstVisibleRowIndex();
        let lastVisibleRow = this.getLastVisibleRowIndex();
        let prevRowOffset = this.rowOffset;
        let prevTopHeight = parseInt(this.elTopBuffer.getAttribute("height"));          
        
		// remove items from our itemData list that match anything in the removeItems list
		// if this was a row that was above our current row offset (a hidden top row in the top buffer),
		// then decrease row offset accordingly
        let deletedHiddenTopRows = false;
        let deletedVisibleRows = false;
        for (let j = this.itemData.length - 1; j >= 0; --j) {
            let id = this.itemData[j].id;
            for (let i = 0, length = removeItems.length; i < length; ++i) {
                if (id === removeItems[i]) {
                    if (j < firstVisibleRow) {
                        this.rowOffset--;
                        deletedHiddenTopRows = true;
                    } else if (j >= firstVisibleRow && j < lastVisibleRow) {
                        deletedVisibleRows = true;
                    }
                    this.itemData.splice(j, 1);
                    break;
                }
            }
        }
        
		// if we deleted both hidden rows in the top buffer above our row offset as well as rows that are currently being
		// shown, then we need to adjust the scroll position to compensate unless we are at the bottom of the list
        let adjustScrollTop = deletedHiddenTopRows && deletedVisibleRows;
        if (this.rowOffset + this.getNumRows() > this.itemData.length) {
            this.rowOffset = this.itemData.length - this.getNumRows();
            adjustScrollTop = false;
        }
        
        this.refresh();
        
		// decrease the last scrolling point to adjust for the amount of row space that was lost from the removed items
        let newTopHeight = parseInt(this.elTopBuffer.getAttribute("height"));
        let topHeightChange = prevTopHeight - newTopHeight;
        this.lastRowChangeScrollTop -= topHeightChange; 
        if (adjustScrollTop) {
            this.elTableScroll.scrollTop -= topHeightChange;
        }
    },

    clear: function() {
        for (let i = 0; i < this.getNumRows(); i++) {
            let elRow = this.elTableBody.childNodes[i + FIRST_ROW_INDEX];
            this.clearRowFunction(elRow);
            elRow.style.display = "none";
        }
    },
    
    scrollDown: function(numScrollRows) {   
        let prevTopHeight = parseInt(this.elTopBuffer.getAttribute("height"));
        let prevBottomHeight =  parseInt(this.elBottomBuffer.getAttribute("height"));
        
        // for each row to scroll down, move the top row element to the bottom of the
        // table before the bottom buffer and reset it's row data to the new item
        for (let i = 0; i < numScrollRows; i++) {
            let rowMovedTopToBottom = this.elTableBody.childNodes[FIRST_ROW_INDEX];
            let rowIndex = this.getNumRows() + this.rowOffset;
            this.elTableBody.removeChild(rowMovedTopToBottom);
            this.elTableBody.insertBefore(rowMovedTopToBottom, this.elBottomBuffer);
            this.updateRowFunction(rowMovedTopToBottom, this.itemData[rowIndex]);
            this.rowOffset++;
        }
        
        // add the row space that was scrolled away to the top buffer height and last scroll point
        // remove the row space that was scrolled away from the bottom buffer height 
        let scrolledSpace = this.rowHeight * numScrollRows;
        let newTopHeight = prevTopHeight + scrolledSpace;
        let newBottomHeight = prevBottomHeight - scrolledSpace;
        this.elTopBuffer.setAttribute("height", newTopHeight);
        this.elBottomBuffer.setAttribute("height", newBottomHeight);
        this.lastRowChangeScrollTop += scrolledSpace;
    },
    
    scrollUp: function(numScrollRows) {
        let prevTopHeight = parseInt(this.elTopBuffer.getAttribute("height"));
        let prevBottomHeight =  parseInt(this.elBottomBuffer.getAttribute("height"));

        // for each row to scroll up, move the bottom row element to the top of
        // the table before the top row and reset it's row data to the new item
        for (let i = 0; i < numScrollRows; i++) {
            let topRow = this.elTableBody.childNodes[FIRST_ROW_INDEX];
            let rowMovedBottomToTop = this.elTableBody.childNodes[FIRST_ROW_INDEX + this.getNumRows() - 1];
            let rowIndex = this.rowOffset - 1;
            this.elTableBody.removeChild(rowMovedBottomToTop);
            this.elTableBody.insertBefore(rowMovedBottomToTop, topRow);
            this.updateRowFunction(rowMovedBottomToTop, this.itemData[rowIndex]);
            this.rowOffset--;
        }
        
        // remove the row space that was scrolled away from the top buffer height and last scroll point
        // add the row space that was scrolled away to the bottom buffer height
        let scrolledSpace = this.rowHeight * numScrollRows;
        let newTopHeight = prevTopHeight - scrolledSpace;
        let newBottomHeight = prevBottomHeight + scrolledSpace;
        this.elTopBuffer.setAttribute("height", newTopHeight);
        this.elBottomBuffer.setAttribute("height", newBottomHeight);
        this.lastRowChangeScrollTop -= scrolledSpace;
    },
    
    onScroll: function()  {
        var that = this.listView;
        
        let scrollTop = that.elTableScroll.scrollTop;
        let scrollHeight = that.getScrollHeight();
        let nextRowChangeScrollTop = that.lastRowChangeScrollTop + scrollHeight;
        let totalItems = that.itemData.length;
        let numRows = that.getNumRows();

        // if the top of the scroll area has past the amount of scroll row space since the last point of scrolling and there
        // are still more rows to scroll to then trigger a scroll down by the min of the scroll row space or number of
        // remaining rows below
        // if the top of the scroll area has gone back above the last point of scrolling then trigger a scroll up by min of
        // the scroll row space or number of rows above
        if (scrollTop >= nextRowChangeScrollTop && numRows + that.rowOffset < totalItems) {
            let numScrolls = Math.ceil((scrollTop - nextRowChangeScrollTop) / scrollHeight);
            let numScrollRows = numScrolls * SCROLL_ROWS;
            if (numScrollRows + that.rowOffset + numRows > totalItems) {
                numScrollRows = totalItems - that.rowOffset - numRows;
            }
            that.scrollDown(numScrollRows);
        } else if (scrollTop < that.lastRowChangeScrollTop) {
            let numScrolls = Math.ceil((that.lastRowChangeScrollTop - scrollTop) / scrollHeight);
            let numScrollRows = numScrolls * SCROLL_ROWS;
            if (that.rowOffset - numScrollRows < 0) {
                numScrollRows = that.rowOffset;
            }
            that.scrollUp(numScrollRows);
        }
    },
    
    refresh: function() {
        // block refreshing before rows are initialized
        let numRows = this.getNumRows();
        if (numRows === 0) {
            return;
        }
        
        // start with all row data cleared and initially set to invisible
        this.clear();
        
        // update all row data and set rows visible until max visible items reached
        for (let i = 0; i < numRows; i++) {
            let rowIndex = i + this.rowOffset;
            if (rowIndex >= this.itemData.length) {
                break;
            }
            let rowElementIndex = i + FIRST_ROW_INDEX;
            let elRow = this.elTableBody.childNodes[rowElementIndex];
            let itemData = this.itemData[rowIndex];
            this.updateRowFunction(elRow, itemData);
            elRow.style.display = "";
        }
        
        // top buffer height is the number of hidden rows above the top row
        let topHiddenRows = this.rowOffset;
        let topBufferHeight = this.rowHeight * topHiddenRows;
        this.elTopBuffer.setAttribute("height", topBufferHeight);
        
        // bottom buffer height is the number of hidden rows below the bottom row (last scroll buffer row)
        let bottomHiddenRows = this.itemData.length - numRows - this.rowOffset;
        let bottomBufferHeight = this.rowHeight * bottomHiddenRows;
        if (bottomHiddenRows < 0) {
            bottomBufferHeight = 0;
        }
        this.elBottomBuffer.setAttribute("height", bottomBufferHeight);
    },
    
    resetRows: function(viewableHeight) {
        if (!this.elTableBody) {
            debugPrint("ListView.resetRows - no valid table body element");
            return;
        }
        
        this.resetRowOffset();
        
        this.elTopBuffer.setAttribute("height", 0);
        this.elBottomBuffer.setAttribute("height", 0);

        // clear out any existing rows
        for (let i = 0; i < this.getNumRows(); i++) {
            let elRow = elRows[i];
            this.elTableBody.removeChild(elRow);
        }
        this.elRows = [];
        
        // create new row elements inserted between the top and bottom buffers that can fit into viewable scroll area
        let usedHeight = 0;
        while(usedHeight < viewableHeight) {
            let newRow = this.createRowFunction();
            this.elTableBody.insertBefore(newRow, this.elBottomBuffer);
            this.rowHeight = this.elTableBody.offsetHeight - usedHeight;
            usedHeight = this.elTableBody.offsetHeight;     
            this.elRows.push(newRow);
        }
        
        // add extras rows for scrolling buffer purposes
        for (let i = 0; i < SCROLL_ROWS; i++) {
            let scrollRow = this.createRowFunction();
            this.elTableBody.insertBefore(scrollRow, this.elBottomBuffer);
            this.elRows.push(scrollRow);
        }
    },
    
    initialize: function() {
        if (!this.elTableBody || !this.elTableScroll) {
            debugPrint("ListView.initialize - no valid table body or table scroll element");
            return;
        }
        
        // delete initial blank row
        this.elTableBody.deleteRow(0);
        
        this.elTopBuffer = document.createElement("tr");
        this.elTableBody.appendChild(this.elTopBuffer);
        this.elTopBuffer.setAttribute("height", 0);
        
        this.elBottomBuffer = document.createElement("tr");
        this.elTableBody.appendChild(this.elBottomBuffer);
        this.elBottomBuffer.setAttribute("height", 0);

        this.elTableScroll.listView = this;
        this.elTableScroll.onscroll = this.onScroll;
    }
};
